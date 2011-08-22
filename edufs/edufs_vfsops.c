
/*
 * Copyright (c) 2003 David Parfitt
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/conf.h>
#include <sys/namei.h>
#include <sys/proc.h>
#include <sys/kernel.h>
#include <sys/vnode.h>
#include <sys/mount.h>
#include <sys/bio.h>
#include <sys/buf.h>
#include <sys/fcntl.h>
#include <sys/malloc.h>
#include <sys/stat.h> 				/* defines ALLPERMS */
#include <sys/mutex.h>
#include <sys/lock.h>
#include <sys/errno.h>
#include <sys/types.h>
#include <sys/queue.h>
#include <vm/uma.h>

#include <fs/edufs/edufs_mount.h>
#include <fs/edufs/edufs_enode.h>
#include <fs/edufs/edufs_denode.h>
#include <fs/edufs/edufs.h>

extern vop_t **edufs_vnodeop_p;
#define ROOTENO 2

static MALLOC_DEFINE(M_EDUFSMNT, "EDUFS mount", "EDUFS mount structure");
static MALLOC_DEFINE(M_EDUFSNODE, "EDUFS node", "EDUFS vnode private part");


vfs_mount_t   edufs_mount;
vfs_root_t    edufs_root;
vfs_unmount_t edufs_unmount;
vfs_statfs_t  edufs_statfs;
vfs_init_t    edufs_init;
vfs_uninit_t  edufs_uninit;
vfs_vget_t    edufs_vget;


uma_zone_t uma_enode,uma_denode; /*, uma_edufs;*/

int64_t startingoffsets[NIADDR]; 


/* do the actual mounting */
static int domount(struct vnode *devvp, struct mount *mp, struct thread *td);


int edufs_vinit(struct mount *, vop_t **, vop_t **, struct vnode **);
int edufs_flushfiles(struct mount *, int, struct thread*);
int edufs_sync(struct mount *mp, int waitfor,struct ucred *cred,struct thread *td);
void printsuper2(struct edufs_superblock *esb);
off_t enodeoff(int enodenum, struct edufsmount *emp);
off_t enodechunkoff(int enodenum, struct edufsmount *emp);
void edufs_loadenode(struct buf *bp,struct enode *ep, struct edufs_superblock *esb, ino_t ino);

int domount(devvp,mp,td)
	 struct vnode *devvp;
	 struct mount *mp;
	 struct thread *td;


{
  dev_t dev;

  int error;
  struct buf *bp;
  struct edufs_superblock *esb; /* same as "fs" */
  struct edufsmount *emp;       /* ump */
  size_t strsize;
  struct ucred *cred;
  struct cg *allcg;
  cred = td ? td->td_ucred : NOCRED;
  uprintf("mount1"); 
  error = vfs_mountedon(devvp);
  if(error) {
	return (error);
  }
  
  if(vcount(devvp) > 1 && devvp != rootvp) {	
    return (EBUSY);
  }
  
  uprintf("2");
  vn_lock(devvp, LK_EXCLUSIVE | LK_RETRY, td);


  error=vinvalbuf(devvp, V_SAVE, td->td_ucred, td,0,0);
  uprintf("3");
  VOP_UNLOCK(devvp,0,td);
  if(error) {
	return (error);
  }


  /*
   * Only VMIO the backing device if the backing device is a real
   * block device.
   * Note that it is optional that the backing device be VMIOed.  This
   * increases the opportunity for metadata caching.
   */
  if (vn_isdisk(devvp, NULL)) {
	uprintf("4");
	vn_lock(devvp, LK_EXCLUSIVE | LK_RETRY, td);
	vfs_object_create(devvp, td, cred);
	VOP_UNLOCK(devvp,0,td);
  }
  
   uprintf("5"); 
  vn_lock(devvp, LK_EXCLUSIVE | LK_RETRY, td);
   uprintf("6"); 
  error = VOP_OPEN(devvp,FREAD|FWRITE,FSCRED,td,-1);
  /* -1 is "fdidx", currently not in the man page */

   uprintf("7"); 
  VOP_UNLOCK(devvp,0,td);
  if(error) {	
	return (error);
  }
  if (devvp->v_rdev->si_iosize_max != 0)
	mp->mnt_iosize_max = devvp->v_rdev->si_iosize_max;
  if (mp->mnt_iosize_max > MAXPHYS)
	mp->mnt_iosize_max = MAXPHYS;
  
  
  bp = NULL;

   uprintf("8"); 
  /* should clean up the bread code here */
  error = bread(devvp, 0, 1024, NOCRED, &bp);

  if(error) {
	return (error);
  }

  bp->b_flags |= B_AGE;
  esb = (struct edufs_superblock *)bp->b_data;

  uprintf("Successfully read the EDUFS superblock\n");  

  printf("MAGIC = %d\n",esb->fs_magic);
  
  MALLOC(emp, struct edufsmount *, sizeof(struct edufsmount),
		 M_EDUFSMNT, M_WAITOK);
  
  dev = devvp->v_rdev;  
  emp->e_devvp = devvp;
  emp->e_dev = dev;
  emp->e_esb = malloc((u_long)esb->fs_sbsize, M_EDUFSMNT,M_WAITOK);

  /* save a copy of the superblock! */
  bcopy(bp->b_data,emp->e_esb, (u_int)esb->fs_sbsize);
  
  /*if (esb->fs_sbsize < SBLOCKSIZE)
	bp->b_flags |= B_INVAL | B_NOCACHE;*/ /* ????? */
  /* B_INVAL = "does not contain valid info"
	 B_NOCACHE = dont cache */  
  
  mp->mnt_data =(qaddr_t) emp;
  
  mp->mnt_stat.f_fsid.val[0] = esb->fs_id[0];
  mp->mnt_stat.f_fsid.val[1] = esb->fs_id[1];
  mp->mnt_flag |= MNT_LOCAL;

  
  if (esb->fs_id[0] == 0 || esb->fs_id[1] == 0 || 
	  vfs_getvfs(&mp->mnt_stat.f_fsid)) 
	vfs_getnewfsid(mp);

  devvp->v_rdev->si_mountpoint = mp;  
  
  copystr(	mp->mnt_stat.f_mntonname,	   /* mount point*/
			esb->fs_fsmnt,			       /* copy area*/
			sizeof(esb->fs_fsmnt) - 1,	   /* max size*/
			&strsize);			           /* real size*/
  bzero( esb->fs_fsmnt + strsize, sizeof(esb->fs_fsmnt) - strsize);
    
  brelse(bp);  
  bp = NULL;

  MALLOC(allcg,struct cg*,esb->fs_ncg * sizeof(struct cg),M_EDUFSMNT,M_WAITOK);
  
  
  /*-------------------------------- */
  /* read all the cylinder groups in */
  /*-------------------------------- */
  int cgcounter = 0;
  daddr_t nextblock = esb->fs_cblkno/esb->fs_bps;   
  struct cg *cgs = allcg;
  
  for(cgcounter = 0;cgcounter < esb->fs_ncg;cgcounter++) {		
	error = bread(devvp,nextblock,esb->fs_bsize,NOCRED,&bp);
	
	struct cg *thiscg = (struct cg *)bp->b_data;		
	memcpy(cgs,thiscg,sizeof(struct cg));	
	/* TODO: be sure to check cg magic!! */	
	bp->b_flags |= B_AGE;	
	brelse(bp);
	bp = NULL;		
	nextblock = thiscg->cg_next / esb->fs_bps;	
	cgs++;
  }
  
  emp->cglist = allcg;
  /* TODO: NEED TO DO SOMETHING WITH EMP, ESB */
  /* UNMOUNT SHOULD FREE MEMORY... */  
  
   uprintf("9"); 
  return 0;
}

/*
 * mp - path - addr in user space of mount point (ie /usr or whatever)
 * data - addr in user space of mount params including the name of the block
 * special file to treat as a filesystem.
 */
 int
edufs_mount(mp, path, data, ndp, td)
	struct mount *mp;
	char *path;
	caddr_t data;
	struct nameidata *ndp;
	struct thread *td;
{

  int error;
  struct edufs_args ea;
  struct vnode *devvp;
  struct ucred *cred;
  size_t size;
  mode_t accessmode;
  /*
	struct edufs_args args;*/    	
  uprintf("EDUFS_MOUNT "); 
  cred = td ? td->td_ucred : NOCRED;
  prtactive = 1;
  
  error = copyin(data, (caddr_t)&ea, sizeof(struct edufs_args));
  if (error)
	return (error);
  
  /*uprintf("Mounting device %s\n",ea.fspec);*/

  /* initialize memory */
  if (uma_enode == NULL) {
	uma_enode = uma_zcreate("EDUFS enode",
							sizeof(struct enode), NULL, NULL, NULL, NULL,
							UMA_ALIGN_PTR, 0);
	uma_denode = uma_zcreate("EDUFS denode",
							sizeof(struct denode), NULL, NULL, NULL, NULL,
							UMA_ALIGN_PTR, 0);	
  }


  /* TODO: check the magic
	 if (args.magic != MSDOSFS_ARGSMAGIC)
	 args.flags = 0;
  */
  
  NDINIT(ndp, LOOKUP, FOLLOW, UIO_USERSPACE, ea.fspec, td);
  
  if((error = namei(ndp)) != 0) {
	return (error);
  }

  NDFREE(ndp, NDF_ONLY_PNBUF);
  devvp = ndp->ni_vp;      
  if (!vn_isdisk(devvp, &error)) {
	vrele(devvp);
	return (error);
  }

  if (suser(td)) {
	accessmode = VREAD;
	if ((mp->mnt_flag & MNT_RDONLY) == 0)
	  accessmode |= VWRITE;
	vn_lock(devvp, LK_EXCLUSIVE | LK_RETRY, td);
	if ((error = VOP_ACCESS(devvp, accessmode, td->td_ucred, td))!= 0){
	  vput(devvp);
	  return (error);
	}
	VOP_UNLOCK(devvp, 0, td);
  }

  /*
  error = vfs_mountedon(devvp);
  if(error) {
	uprintf("Returning from mountedon\n");
	return (error);
  }
  
  if(vcount(devvp) > 1 && devvp != rootvp) {
	uprintf("mount - Returning EBUSY...\n");
    return (EBUSY);
  }
  
  uprintf("m4");
  vn_lock(devvp, LK_EXCLUSIVE | LK_RETRY, td);
  uprintf("m4a");

  error=vinvalbuf(devvp, V_SAVE, td->td_ucred, td,0,0);
  VOP_UNLOCK(devvp,0,td);
  if(error) {
	uprintf("mount - error calling vinvalbuf\n");
	return (error);
  }

  vn_lock(devvp, LK_EXCLUSIVE | LK_RETRY, td);
  error = VOP_OPEN(devvp,FREAD|FWRITE,FSCRED,td);
  VOP_UNLOCK(devvp,0,td);
  if(error) {
	uprintf("mount - error calling vop_open\n");
	return (error);
  }
  */

  error = domount(devvp,mp,td);
  if(error) {
	vrele(devvp);
	return (error);
  }

  
  copyinstr(ea.fspec, mp->mnt_stat.f_mntfromname, MNAMELEN - 1, &size);
  /*uprintf("FSPEC = %s\n",mp->mnt_stat.f_mntfromname);*/
  bzero( mp->mnt_stat.f_mntfromname + size, MNAMELEN - size);
  
  (void)VFS_STATFS(mp,&mp->mnt_stat,td);   
  return 0; 
  
}


int
edufs_unmount(mp, mntflags, td)
	 struct mount *mp;
	 int mntflags;
	 struct thread *td;
{
  int error, flags;
  struct edufsmount *emp = VFSTOEDUFS(mp);
  
  uprintf("edufs_unmount");
    
  flags = 0;
  if (mntflags & MNT_FORCE)
	flags |= FORCECLOSE;
  error = vflush(mp,0,flags);
  if(error) {
	return (error);
  }
  
  
  emp->e_devvp->v_rdev->si_mountpoint = NULL;      
  error = VOP_CLOSE(emp->e_devvp, FREAD|FWRITE, NOCRED, td);

  vrele(emp->e_devvp);
  free(emp->cglist, M_EDUFSMNT);
  free(emp->e_esb, M_EDUFSMNT);
  free(emp, M_EDUFSMNT);
  mp->mnt_data = (qaddr_t)0;
  mp->mnt_flag &= ~MNT_LOCAL;
  uprintf("EDUFS UNMOUNTED");
  return (error);    
}

 int
edufs_root(mp, vpp)
	struct mount *mp;
	struct vnode **vpp;
{
  uprintf("EDUFS_ROOT");
  struct vnode *nvp;
  int error;


  error = edufs_vget(mp, (ino_t)ROOTENO, LK_EXCLUSIVE, &nvp);
  uprintf("FINISHED CALLING ROOT\n");
  if (error)
	return (error);
  
  *vpp = nvp; 
  return (0);
}

int
edufs_statfs(mp, sbp, td)
	struct mount *mp;
	struct statfs *sbp;
	struct thread *td;
{  
  struct edufsmount *emp = VFSTOEDUFS(mp);  
  struct edufs_superblock *esb;
  
  esb = emp->e_esb;
  
  /*  if (esb->fs_magic != FS_UFS1_MAGIC && esb->fs_magic != FS_UFS2_MAGIC)
	  panic("ffs_statfs");*/
  /* TODO: magic */
  
  uprintf("edufs_statfs");
  
  sbp->f_bsize = esb->fs_bsize;
  sbp->f_iosize = esb->fs_bps;
  sbp->f_blocks = esb->fs_dsize;
  sbp->f_bfree = esb->fs_cstotal.cs_nbfree; 
  sbp->f_bavail = esb->fs_cstotal.cs_nbfree; /* extra space for root? */
  sbp->f_files =  esb->fs_ncg * esb->fs_epg;  
  sbp->f_ffree = 0;/*TODO: esb->fs_cstotal.cs_nefree;*/
   
  if (sbp != &mp->mnt_stat) {
	sbp->f_type = mp->mnt_vfc->vfc_typenum;
	bcopy(mp->mnt_stat.f_mntonname, sbp->f_mntonname, MNAMELEN);
	bcopy(mp->mnt_stat.f_mntfromname, sbp->f_mntfromname, MNAMELEN);
  }
  strncpy(sbp->f_fstypename, mp->mnt_vfc->vfc_name, MFSNAMELEN);
  
  return(0);
}

int
edufs_init(vfsp)
	 struct vfsconf *vfsp;
{
  /* this shows up during system startup - type dmesg to see it*/
  printf("Initializing edufs\n");
  edufs_ehashinit();  
  return (0);
}


 int
edufs_uninit(vfsp)
	 struct vfsconf *vfsp;
{
  edufs_ehashuninit();
  return (0);
}


 int
edufs_vget(mp,ino,flags,vpp)
	 struct mount *mp;
	 ino_t ino;	 
	 int flags;
	 struct vnode **vpp;
{
  struct edufsmount *emp;
  struct thread *td = curthread;
  struct vnode *vp;
  struct enode *ep;
  struct buf *bp;
  dev_t dev;
  int error;

  off_t dechunkoff; /* offset of 512 byte block (chunk) that enode is in */
	
  uprintf("edufs_vget\n");
  uprintf("vget - requesting enode [%d] ",(int)ino);

  emp = VFSTOEDUFS(mp);
  dev = emp->e_dev;

  uprintf("calling hashget ");
  if ((error = edufs_ehashget(dev, ino, flags, vpp)) != 0)
	return (error);
  if (*vpp != NULL)
	return (0);  

  
  ep = uma_zalloc(uma_enode, M_WAITOK);
  
  
  printf("getnewvnode ");
  error = getnewvnode("edufs",mp,edufs_vnodeop_p,&vp);
  if(error) {
	*vpp = NULL;
	uma_zfree(uma_enode, ep);
	uprintf("vget - error1\n");
	return (error);
  }

  bzero((caddr_t)ep, sizeof(struct enode));

  vp->v_vnlock->lk_flags |= LK_CANRECURSE;
  vp->v_data = ep;
  ep->e_vnode = vp;
  ep->e_emp = emp;
  ep->e_fs = emp->e_esb;
  ep->e_dev = dev;
  ep->e_number = ino;
  uprintf("forcing vtype");
  vp->v_type=VDIR;
  /*
   * Exclusively lock the vnode before adding to hash. Note, that we
   * must not release nor downgrade the lock (despite flags argument
   * says) till it is fully initialized.
   */
  printf("lockmgr ");
  lockmgr(vp->v_vnlock, LK_EXCLUSIVE, (struct mtx *)0, td);
  
  /*
   * Atomicaly (in terms of ufs_hash operations) check the hash for
   * duplicate of vnode being created and add it to the hash. If a
   * duplicate vnode was found, it will be vget()ed from hash for us.
   */
  printf("hashins ");
  if ((error = edufs_ehashins(ep, flags, vpp)) != 0) {
	vput(vp);
	*vpp = NULL;
	uprintf("vget - error hash insert!\n");
	return (error);
  }  
  
  /* We lost the race, then throw away our vnode and return existing */
  if (*vpp != NULL) {
	vput(vp);
	uprintf("vget7");
	return (0);
  }


  /* READ IN CONTENTS OF ENODE HERE! */
  
  /*rintf("Enode #2 offset = %lld\n",enodeoff(ino,emp));*/

  dechunkoff = enodechunkoff(ino,emp);
  uprintf("enode block = %lld\n",dechunkoff);
  printf("bread ");  
  error = bread(emp->e_devvp,dechunkoff / emp->e_esb->fs_bps,(int)emp->e_esb->fs_bps, NOCRED, &bp);
  
  if(error) {
	uprintf("CANT BREAD!\n");
	brelse(bp);
	vput(vp);
	*vpp = NULL;
	return(error);
  } 
  
    
  ep->den = uma_zalloc(uma_denode,M_WAITOK);

  printf("vinit/load ");  
  /*error = edufs_vinit(mp, 0, 0, &vp);*/
  
  if (ep->e_number == ROOTENO)
    vp->v_vflag |= VV_ROOT;
  
  edufs_loadenode(bp,ep,emp->e_esb,ino);
  
  /* this needs to go into its own function...*/
  /* ??? */
  /*ep->e_mode = S_IRUSR;*/
  ep->e_effnlink = ep->e_nlink;
  brelse(bp);
      
  ASSERT_VOP_LOCKED(vp, "edufs_vinit");
  if(ep->e_number == ROOTENO)
	vp->v_vflag |= VV_ROOT;
    
  ep->e_devvp = emp->e_devvp;
  VREF(ep->e_devvp);
      
  *vpp = vp;    
  printf("return ");  
  return (0);
}



/*
 * Initialize the vnode associated with a new inode, handle aliased
 * vnodes.
 */
 int
edufs_vinit(mntp, specops, fifoops, vpp)
	struct mount *mntp;
	vop_t **specops;
	vop_t **fifoops;
	struct vnode **vpp;
{
	struct enode *ep;
	struct vnode *vp;
	
	vp = *vpp;
	ep = VTOE(vp);
	
	/* TODO: ???
	switch(vp->v_type = IFTOVT(ep->e_mode)) {	  
	case VCHR:
	case VBLK:
	  uprintf("vblk here: specops?");

	  vp = addaliasu(vp, ep->den->de_db[0]);
	  ep->e_vnode = vp;
	  break;
	case VFIFO:

	  break;
	default:
	  break;
	  
	}
	*/
	uprintf("vinit: v_type==%d\n",vp->v_type);
	ASSERT_VOP_LOCKED(vp, "edufs_vinit");
	
	if (ep->e_number == ROOTENO)
	  vp->v_vflag |= VV_ROOT;
	
	/*
	 * TODO: Initialize modrev times	 
	*/
		
	*vpp = vp;
	return (0);
}







void printsuper2(struct edufs_superblock *esb) {

  uprintf("-------------------------------------------------\n");
  uprintf("-- SUPERBLOCK INFO                             --\n");
  uprintf("-------------------------------------------------\n");
  uprintf("\tSize of filesystem blocks %d\n",esb->fs_bsize);
  uprintf("\tNumber of blocks in filesystem %d\n",esb->fs_size);
  uprintf("\tNumber of cylinder groups = %d\n",esb->fs_ncg);
  uprintf("\tNumber of cylinders is each group = %d\n",esb->fs_cpg);
  uprintf("\tDisk size = %d\n",esb->fs_size * esb->fs_bps);
  uprintf("\tInterleave %d\n",esb->fs_interleave);
  uprintf("\tBytes per sector: %d\n",esb->fs_bps);
  uprintf("\tEnodes per group: %d\n",esb->fs_epg);
  uprintf("\tNumber of blocks in fs: %d\n",esb->fs_size);
  uprintf("\tNumber of data blocks in fs: %d\n",esb->fs_dsize);
  uprintf("\tCG Offset in cylinder: %d\n",esb->fs__cgoffset);
  uprintf("\tCylinder group size: %d\n",esb->fs_cgsize);
  uprintf("\t-->Number of directories: %lld\n",esb->fs_cstotal.cs_ndir);
  uprintf("\t-->Number of free blocks: %lld\n",esb->fs_cstotal.cs_nbfree);
  uprintf("\t-->Number of free enodes: %lld\n",esb->fs_cstotal.cs_nefree);

}


/* JUST FOR DEVELOPMENT! */
off_t enodeoff(int enodenum, struct edufsmount *emp) {
  struct edufs_superblock *esb = emp->e_esb;
  struct cg *ap = emp->cglist;
  off_t offset;

  int cg = enodenum / esb->fs_epg;
  ap += cg;
  offset = ap->cg_enodeoff + (sizeof(struct denode) * (enodenum % esb->fs_epg));
  return offset;
}


/* get the 512 byte block that an enode belongs to! */
/* im calling these "chunks" */
off_t enodechunkoff(int enodenum, struct edufsmount *emp) {
  struct edufs_superblock *esb = emp->e_esb;
  struct cg *ap = emp->cglist;
  off_t offset;
  
  int cg = enodenum / esb->fs_epg;
  ap += cg;
  /*offset = ap->cg_enodeoff + (sizeof(struct denode) * (enodenum % esb->fs_epg));*/
  offset = ap->cg_enodeoff + (sizeof(struct denode) * (enodenum / (esb->fs_bps / sizeof(struct denode))));
  return offset;    
}

/* need to move this somewhere else */
void edufs_loadenode(bp,ep,esb,ino)
	 struct buf *bp;
	 struct enode *ep;
	 struct edufs_superblock *esb;
	 ino_t ino;

{
  struct denode *dnode;
  /* enodes per "chunk" */
  /* is this offset correct??? */  
  int offset = ino % (esb->fs_bps / sizeof(struct denode));
  
  uprintf("offset into chunk is %d\n",offset);
  
  dnode = (struct denode*)bp->b_data;
  dnode += offset;
  uprintf("DENODE %d # %d\n",ino, dnode->de_spare[0]);
  
  *ep->den = *dnode;

  /*uprintf("Sneaky enode # = [%d]\n",ep->den->de_spare[0]);*/
  /*uprintf("NLINK test = [%d]\n",dnode->de_nlink);*/
  
   ep->e_mode = ep->den->de_mode;
   uprintf("EMODE= %d",ep->e_mode);
   uprintf("NLINK = %d",ep->e_nlink);
   ep->e_nlink = ep->den->de_nlink;
   /*********************************
    *********************************
    *********************************
    */
   uprintf("link hack? what is this?");
   if(ep->e_nlink < 1)
	 ep->e_nlink = 2;
   uprintf("NLINK = %d",ep->e_nlink);
   ep->e_size = ep->den->de_size;
   uprintf("enode size in bytes=%lld",ep->e_size);
   ep->e_flags = ep->den->de_flags;
   ep->e_gen = ep->den->de_gen;
   ep->e_uid = ep->den->de_uid;
   ep->e_gid = ep->den->de_gid;	
  
  return;
}


/* define the virtual filesystem operations */
static struct vfsops edufs_vfsops = {  
  edufs_mount, 
  vfs_stdstart,
  edufs_unmount,
  edufs_root,
  vfs_stdquotactl,
  edufs_statfs,
  vfs_stdsync,
  edufs_vget,
  vfs_stdfhtovp,
  vfs_stdcheckexp,
  vfs_stdvptofh,
  edufs_init,
  edufs_uninit,
  vfs_stdextattrctl,
};

VFS_SET(edufs_vfsops, edufs, 0);
