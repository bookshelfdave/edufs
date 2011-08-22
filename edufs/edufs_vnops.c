/*
 * Copyright (c) 2003 David Parfitt
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *	  notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *	  notice, this list of conditions and the following disclaimer in the
 *	  documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *	  derived from this software without specific prior written permission.
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
#include <sys/kernel.h>
#include <sys/lock.h>
#include <sys/malloc.h>
#include <sys/mount.h>
#include <sys/mutex.h>
#include <sys/namei.h>
#include <sys/sysctl.h>
#include <sys/vnode.h>
#include <sys/stat.h>
#include <sys/unistd.h>
#include <sys/fcntl.h>
#include <sys/bio.h>
#include <sys/buf.h>
#include <sys/dirent.h>
#include <fs/edufs/edufs_enode.h>
#include <fs/edufs/edufs_denode.h>
#include <fs/edufs/edufs.h>
#include <fs/edufs/edufs_dir.h>
#include <fs/edufs/edufs_mount.h>
#include <vm/vm.h>
#include <vm/uma.h>
#include <vm/vm_extern.h>
#include <vm/vm_object.h>
#include <vm/vnode_pager.h>


static int edufs_access(struct vop_access_args *ap);
static int edufs_bmap(struct vop_bmap_args *ap);
static int edufs_lookup(struct vop_cachedlookup_args *ap);
static int edufs_close(struct vop_close_args *ap);
static int edufs_create(struct vop_create_args *ap);
static int edufs_fsync(struct vop_fsync_args *ap);
static int edufs_getattr(struct vop_getattr_args *ap);
static int edufs_inactive(struct vop_inactive_args *ap);
static int edufs_link (struct vop_link_args *ap);
static int edufs_mkdir (struct vop_mkdir_args *ap);
static int edufs_mknod(struct vop_mknod_args *ap);
static int edufs_pathconf(struct vop_pathconf_args *ap);
static int edufs_print(struct vop_print_args *ap);
static int edufs_read(struct vop_read_args *ap);
static int edufs_readdir(struct vop_readdir_args *ap);
static int edufs_reclaim(struct vop_reclaim_args *ap);
static int edufs_remove(struct vop_remove_args *ap);
static int edufs_rename(struct vop_rename_args *ap);
static int edufs_rmdir(struct vop_rmdir_args *ap);
static int edufs_setattr(struct vop_setattr_args *ap);
static int edufs_strategy(struct vop_strategy_args *ap);
static int edufs_symlink(struct vop_symlink_args *ap);
static int edufs_write(struct vop_write_args *ap);
static int edufs_open(struct vop_open_args *ap);

extern vfs_vget_t edufs_vget;


void edufs_etimes(struct vnode *vp);

/* the remaining functions should probably be broken out */

/* allocation stuff */
static int edufs_findfreeenode(struct edufsmount *emp, uint32_t *fenum,uint32_t *fcg);
/*static int edufs_findfreeblock(struct edufsmount *emp, uint32_t *fbnum);*/
/* not used? */
/*atic int edufs_bmaparray(struct vnode *vp,int64_t bn,int *blk);*/

/* unfinished */
static int edufs_valloc(struct vnode *pvp,int mode,struct ucred *cred,struct vnode **vpp);
static int edufs_makeenode(int mode,struct vnode *dvp,struct vnode **vpp,struct componentname *cnp);


/*static int edufs_readdenode(uint32_t denum, struct denode **de, struct edufsmount *emp);*/
/*static int edufs_writeenode(uint32_t denum, struct denode *de,  struct edufsmount *emp);*/



extern uma_zone_t uma_enode;
extern uma_zone_t uma_denode;



extern int64_t startingoffsets[NIADDR]; 



/*
 * Create a regular file. On entry the directory to contain the file being
 * created is locked.  We must release before we return. We must also free
 * the pathname buffer pointed at by cnp->cn_pnbuf, always on error, or
 * only if the SAVESTART bit in cn_flags is clear on success.
 */
static int
edufs_create(ap)
	 struct vop_create_args /* {
							   struct vnode *a_dvp;
							   struct vnode **a_vpp;
							   struct componentname *a_cnp;
							   struct vattr *a_vap;
							   } */ *ap;
{
  uprintf("EDUFS_CREATE\n");
 
  int error;


  /*struct enode *en = VTOE(ap->a_dvp);*/
  struct edufsmount *emp;  
  emp = VFSTOEDUFS(ap->a_dvp->v_mount);


  /* do i still need the freecg?? */
  error = edufs_makeenode(MAKEIMODE(ap->a_vap->va_type, ap->a_vap->va_mode),
						  ap->a_dvp, ap->a_vpp, ap->a_cnp);
  return (ENOSYS);
}


static int
edufs_mknod(ap)
	 struct vop_mknod_args /* {
							  struct vnode *a_dvp;
							  struct vnode **a_vpp;
							  struct componentname *a_cnp;
							  struct vattr *a_vap;
							  } */ *ap;
{
  uprintf("EDUFS_MKNOD\n");
  return (ENOSYS);
}

static int
edufs_open(ap)
	 struct vop_open_args /* {
							 struct vnode *a_vp;
							 int  a_mode;
							 struct ucred *a_cred;
							 struct thread *a_td;
							 } */ *ap;
{
  uprintf("EDUFS_OPEN\n");
  /*if(ap->a_vp->filename != null) {
	uprintf("--->FILENAME %d<---\n",ap->a_vp->filename);
	}*/
  return (0);
}


static int
edufs_close(ap)
	 struct vop_close_args /* {
							  struct vnode *a_vp;
							  int a_fflag;
							  struct ucred *a_cred;
							  struct thread *a_td;
							  } */ *ap;
{
  uprintf("EDUFS_CLOSE\n");
  struct vnode *vp = ap->a_vp;
  struct mount *mp;
  
  VI_LOCK(vp);
  if (vp->v_usecount > 1) {
	edufs_etimes(vp);
	VI_UNLOCK(vp);
  } else {
	VI_UNLOCK(vp);
	/*
	 * If we are closing the last reference to an unlinked
	 * file, then it will be freed by the inactive routine.
	 * Because the freeing causes a the filesystem to be
	 * modified, it must be held up during periods when the
	 * filesystem is suspended.
	 *
	 * XXX - EAGAIN is returned to prevent vn_close from
	 * repeating the vrele operation.
	 */
	if (vp->v_type == VREG && VTOE(vp)->e_effnlink == 0) {
	  (void) vn_start_write(vp, &mp, V_WAIT);
	  uprintf("Calling rele!\n");
	  vrele(vp);
	  vn_finished_write(mp);
	  return (EAGAIN);
	}
  }
  return (0); 
}



static int
edufs_lookup(ap)
	 struct vop_cachedlookup_args /* {
									 struct vnode *a_dvp;
									 struct vnode **a_vpp;
									 struct componentname *a_cnp;
									 } */ *ap;
{
  uprintf("EDUFS_LOOKUP\n");
  panic("edufs_lookup");
  return (ENOSYS);  
}


static int
edufs_access(ap)
	 struct vop_access_args /* {
							   struct vnode *a_vp;
							   int a_mode;
							   struct ucred *a_cred;
							   struct thread *a_td;
							   } */ *ap;
{
  uprintf("EDUFS_ACCESS\n");
  struct vnode *vp = ap->a_vp;
  struct enode *ep = VTOE(vp);
  mode_t mode = ap->a_mode;
  int error;

  
  /*
   * Disallow write attempts on read-only filesystems;
   * unless the file is a socket, fifo, or a block or
   * character device resident on the filesystem.
   */
  if (mode & VWRITE) {
	switch (vp->v_type) {
	case VDIR:
	case VLNK:
	case VREG:
	  if (vp->v_mount->mnt_flag & MNT_RDONLY) {
		uprintf("ERROR here\n");
		return (EROFS);
	  }
	  
	  break;
	default:
	  break;
	}
  }

  uprintf("requested mode: ");
  if(ap->a_mode & S_IRUSR) {
	uprintf("r");
  } else {
	uprintf("-");
  }
  if(ap->a_mode & S_IWUSR) {
	uprintf("w");
  } else {
	uprintf("-");
  }
  if(ap->a_mode & S_IXUSR) {
	uprintf("x");
  } else {
	uprintf("-");
  }
  
  if(ap->a_mode & S_IRGRP){
	uprintf("r");
  } else {
	uprintf("-");
  }
  if(ap->a_mode & S_IWGRP){
	uprintf("w");
  } else {
	uprintf("-");
  }
  if(ap->a_mode & S_IXGRP){
	uprintf("x");
  } else {
	uprintf("-");
  }


  if(ap->a_mode & S_IROTH){
	uprintf("r");
  } else {
	uprintf("-");
  }
  if(ap->a_mode & S_IWOTH){
	uprintf("w");
  } else {
	uprintf("-");
  }
  if(ap->a_mode & S_IXOTH){
	uprintf("x");
  } else {
	uprintf("-");
  }
  uprintf("\n");




  uprintf("enode mode: ");
  if(ep->e_mode & S_IRUSR) {
	uprintf("r");
  } else {
	uprintf("-");
  }
  if(ep->e_mode & S_IWUSR) {
	uprintf("w");
  } else {
	uprintf("-");
  }
  if(ep->e_mode & S_IXUSR) {
	uprintf("x");
  } else {
	uprintf("-");
  }
  
  if(ep->e_mode & S_IRGRP){
	uprintf("r");
  } else {
	uprintf("-");
  }
  if(ep->e_mode & S_IWGRP){
	uprintf("w");
  } else {
	uprintf("-");
  }
  if(ep->e_mode & S_IXGRP){
	uprintf("x");
  } else {
	uprintf("-");
  }


  if(ep->e_mode & S_IROTH){
	uprintf("r");
  } else {
	uprintf("-");
  }
  if(ep->e_mode & S_IWOTH){
	uprintf("w");
  } else {
	uprintf("-");
  }
  if(ep->e_mode & S_IXOTH){
	uprintf("x");
  } else {
	uprintf("-");
  }
  uprintf("\n");
	
  uprintf("file uid %d gid %d \n",ep->e_uid, ep->e_gid);  
  uprintf("		uid %d gid %d \n",ap->a_cred->cr_uid,ap->a_cred->cr_gid);
  uprintf("ruid uid %d gid %d \n",ap->a_cred->cr_ruid,ap->a_cred->cr_rgid);
  error = vaccess(vp->v_type, ep->e_mode, ep->e_uid, ep->e_gid,
				  ap->a_mode, ap->a_cred, NULL);
  uprintf("Error = %d",error);
  return (error);
	
}

static int
edufs_getattr(ap)
	 struct vop_getattr_args /* {
								struct vnode *a_vp;
								struct vattr *a_vap;
								struct ucred *a_cred;
								struct thread *a_td;
								} */ *ap;
{
  uprintf("EDUFS_GETATTR");

  struct vnode *vp = ap->a_vp;
  struct enode *ep = VTOE(vp);
  struct vattr *vap = ap->a_vap;

  edufs_etimes(vp);

  /*
   * Copy from enode table
   */
  vap->va_fsid = dev2udev(ep->e_dev);
  vap->va_fileid = ep->e_number;
  vap->va_mode = ep->e_mode & ~DIFMT;
  vap->va_nlink = ep->e_effnlink;
  vap->va_uid = ep->e_uid;
  vap->va_gid = ep->e_gid;
  
  vap->va_rdev = ep->den->de_db[0];
  vap->va_size = ep->den->de_size;
  vap->va_atime.tv_sec = ep->den->de_atime;
  vap->va_atime.tv_nsec = ep->den->de_atimensec;
  vap->va_mtime.tv_sec = ep->den->de_mtime;
  vap->va_mtime.tv_nsec = ep->den->de_mtimensec;
  vap->va_ctime.tv_sec = ep->den->de_ctime;
  vap->va_ctime.tv_nsec = ep->den->de_ctimensec;
  vap->va_birthtime.tv_sec = 0;
  vap->va_birthtime.tv_nsec = 0;
  vap->va_bytes = dbtob((u_quad_t)ep->den->de_blocks);
  
  vap->va_flags = 65536;/*ep->e_flags;*/
  vap->va_gen = 0;/*ep->e_gen;*/
  vap->va_blocksize = vp->v_mount->mnt_stat.f_iosize;

  vap->va_type = ap->a_vp->v_type;/*IFTOVT(ep->e_mode);*/
  /*  
	  uprintf("| 1)%d",vap->va_fsid);
	  uprintf("| 2)%ld",vap->va_fileid);
	  uprintf("| 3)%d",vap->va_mode);
	  uprintf("| 4)%d",vap->va_nlink);
	  uprintf("| 5)%d",vap->va_uid);
	  uprintf("| 6)%d",vap->va_gid);   
	  uprintf("| 7)%d",vap->va_rdev);
	  uprintf("| 8)%lld",vap->va_size);
	  uprintf("| 9)%d",vap->va_atime.tv_sec);
	  uprintf("|10)%ld",vap->va_atime.tv_nsec);
	  uprintf("|11)%d",vap->va_mtime.tv_sec);
	  uprintf("|12)%ld",vap->va_mtime.tv_nsec);
	  uprintf("|13)%d",vap->va_ctime.tv_sec);
	  uprintf("|14)%ld",vap->va_ctime.tv_nsec);
	  uprintf("|15)%d",vap->va_birthtime.tv_sec);
	  uprintf("|16)%ld",vap->va_birthtime.tv_nsec);
	  uprintf("|17)%lld",vap->va_bytes);  
	  uprintf("|18)%ld",vap->va_flags);
	  uprintf("|19)%ld",vap->va_gen);
	  uprintf("|20)%ld",vap->va_blocksize);
	  uprintf("|21)%d\n",vap->va_type);*/  
  /*vap->va_filerev = ep->e_modrev;*/
  return (0);

}


static int
edufs_setattr(ap)
	 struct vop_setattr_args /* {
								struct vnode *a_vp;
								struct vattr *a_vap;
								struct ucred *a_cred;
								struct thread *a_td;
								} */ *ap;
{
  uprintf("EDUFS_SETATTR\n");
  return (ENOSYS);
}





static int
edufs_read(ap)
	 struct vop_read_args /* {
							 struct vnode *a_vp;
							 struct uio *a_uio;
							 int a_ioflag;
							 struct ucred *a_cred;
							 } */ *ap;
{
  uprintf("EDUFS_READ\n");	
  struct vnode *vp;
  struct enode *ep;
  struct uio *uio;
  struct edufs_superblock *esb;
  struct buf *bp;
  /*ufs_lbn_t lbn, nextlbn;*/
  uint64_t lbn, nextlbn;
  off_t bytesinfile;
  long size, xfersize, blkoffset;
  int error, orig_resid;
  mode_t mode;
  int seqcount;
  int ioflag;
  vm_object_t object;
  uprintf("r1");
  vp = ap->a_vp;
  uio = ap->a_uio;
  ioflag = ap->a_ioflag;
  if (ap->a_ioflag & IO_EXT)

	GIANT_REQUIRED;

  panic("edufs_read");
  
  seqcount = ap->a_ioflag >> 16;
  ep = VTOE(vp);
  mode = ep->e_mode;

#ifdef DIAGNOSTIC
  if (uio->uio_rw != UIO_READ)
	panic("ffs_read: mode");

  if (vp->v_type == VLNK) {
	if ((int)ep->e_size < vp->v_mount->mnt_maxsymlinklen)
	  panic("ffs_read: short symlink");
  } else if (vp->v_type != VREG && vp->v_type != VDIR)
	panic("ffs_read: type %d",	vp->v_type);
#endif
  esb = ep->e_fs;
  /* check and see if the offset is > max file size */
  if ((u_int64_t)uio->uio_offset > esb->fs_maxfilesize)
	return (EFBIG);

  uprintf("r2");
  orig_resid = uio->uio_resid;
  if (orig_resid <= 0) {
	uprintf("r3");
	return (0);
  }

  object = vp->v_object;
	
  bytesinfile = ep->e_size - uio->uio_offset;
  if (bytesinfile <= 0) {
	uprintf("r4");
	if ((vp->v_mount->mnt_flag & MNT_NOATIME) == 0) {
	  uprintf("in access?\n");
	  ep->e_flag |= EN_ACCESS;
	}
	uprintf("bytes in file <= 0\n");	  
	return 0;
  }

  if (object) {
	vm_object_reference(object);
  }

  /*
   * Ok so we couldn't do it all in one vm trick...
   * so cycle around trying smaller bites..
   */
  for (error = 0, bp = NULL; uio->uio_resid > 0; bp = NULL) {
	uprintf("r5");
	/* check and see if we got to the end of data */
	if ((bytesinfile = ep->e_size - uio->uio_offset) <= 0)
	  break;
	uprintf("r6");
	/* calculates (loc / fs->fs_bsize) */
	/*lbn = lblkno(esb, uio->uio_offset);*/
	lbn = uio->uio_offset / esb->fs_bsize;
	nextlbn = lbn + 1;
	  
	/*
	 * size of buffer.  The buffer representing the
	 * end of the file is rounded up to the size of
	 * the block type ( fragment or full block, 
	 * depending ).
	 */
	size = esb->fs_bsize; /* doesnt do frags yet *//*blksize(fs, ep, lbn);*/
	/*blkoffset = blkoff(esb, uio->uio_offset);*/ /* loc % fs->fs_bsize */
	blkoffset = uio->uio_offset % esb->fs_bsize;
	  
	/*
	 * The amount we want to transfer in this iteration is
	 * one FS block less the amount of the data before
	 * our startpoint (duh!)
	 */
	xfersize = esb->fs_bsize - blkoffset;
	  
	/*
	 * But if we actually want less than the block,
	 * or the file doesn't have a whole block more of data,
	 * then use the lesser number.
	 */
	if (uio->uio_resid < xfersize)
	  xfersize = uio->uio_resid;
	if (bytesinfile < xfersize)
	  xfersize = bytesinfile;
	uprintf("r7");
	/* calculates ((off_t)blk * fs->fs_bsize) */
	/*if (lblktosize(esb, nextlbn) >= ep->e_size) {*/
	if(nextlbn * esb->fs_bsize >= ep->e_size) {
	  /*
	   * Don't do readahead if this is the end of the file.
	   */
	  uprintf("r8");
	  /**** error in here ****/
	  error = bread(vp, lbn, size, NOCRED, &bp);
	  uprintf("THIS bread error = [%d]",error);

	} else if ((vp->v_mount->mnt_flag & MNT_NOCLUSTERR) == 0) {
	  /* 
	   * Otherwise if we are allowed to cluster,
	   * grab as much as we can.
	   *
	   * XXX	This may not be a win if we are not
	   * doing sequential access.
	   */
	  uprintf("r9");
	  error = cluster_read(vp, ep->e_size, lbn,
						   size, NOCRED, uio->uio_resid, seqcount, &bp);
	} else if (seqcount > 1) {
	  /*
	   * If we are NOT allowed to cluster, then
	   * if we appear to be acting sequentially,
	   * fire off a request for a readahead
	   * as well as a read. Note that the 4th and 5th
	   * arguments point to arrays of the size specified in
	   * the 6th argument.
	   */
	  int nextsize = esb->fs_bsize;/*blksize(esb, ep, nextlbn);*/
	  uprintf("r10");
	  error = breadn(vp, lbn,
					 size, &nextlbn, &nextsize, 1, NOCRED, &bp);
	} else {
	  /*
	   * Failing all of the above, just read what the 
	   * user asked for. Interestingly, the same as
	   * the first option above.
	   */
	  uprintf("r11");
	  error = bread(vp, lbn, size, NOCRED, &bp);
	}
	if (error) {
	  uprintf("r12: error code = %d\n",error);
		
	  brelse(bp);
	  bp = NULL;
	  break;
	}
	  
	/*
	 * If IO_DIRECT then set B_DIRECT for the buffer.	 This
	 * will cause us to attempt to release the buffer later on
	 * and will cause the buffer cache to attempt to free the
	 * underlying pages.
	 */
	if (ioflag & IO_DIRECT)
	  bp->b_flags |= B_DIRECT;
	  
	/*
	 * We should only get non-zero b_resid when an I/O error
	 * has occurred, which should cause us to break above.
	 * However, if the short read did not cause an error,
	 * then we want to ensure that we do not uiomove bad
	 * or uninitialized data.
	 */
	size -= bp->b_resid;
	if (size < xfersize) {
	  if (size == 0)
		break;
	  xfersize = size;
	}
	  
	{
	  /*
	   * otherwise use the general form
	   */
	  error =
		uprintf("r13");
	  uprintf("xfersize = [%ld]",xfersize);		
	  uiomove((char *)bp->b_data /*+ blkoffset*/,
			  (int)xfersize, uio);
		
	  uprintf("iomove error = [%d]  ",error);
	}
	  
	if (error)
	  break;
	  
	if ((ioflag & (IO_VMIO|IO_DIRECT)) &&
		(LIST_FIRST(&bp->b_dep) == NULL)) {
	  /*
	   * If there are no dependencies, and it's VMIO,
	   * then we don't need the buf, mark it available
	   * for freeing. The VM has the data.
	   */
	  uprintf("r14");
	  bp->b_flags |= B_RELBUF;
	  brelse(bp);
	} else {
	  /*
	   * Otherwise let whoever
	   * made the request take care of
	   * freeing it. We just queue
	   * it onto another list.
	   */
	  uprintf("r15");
	  bqrelse(bp);
	}
  } /* end for loop */
	
	/* 
	 * This can only happen in the case of an error
	 * because the loop above resets bp to NULL on each iteration
	 * and on normal completion has not set a new value into it.
	 * so it must have come from a 'break' statement
	 */
  if (bp != NULL) {
	if ((ioflag & (IO_VMIO|IO_DIRECT)) &&
		(LIST_FIRST(&bp->b_dep) == NULL)) {
	  uprintf("r16");
	  bp->b_flags |= B_RELBUF;
	  brelse(bp);
	} else {
	  uprintf("r17");
	  bqrelse(bp);
	}
  }
	
  if (object) {
	uprintf("r18");
	VM_OBJECT_LOCK(object);
	vm_object_vndeallocate(object);
  }
  if ((error == 0 || uio->uio_resid != orig_resid) &&
	  (vp->v_mount->mnt_flag & MNT_NOATIME) == 0)
	ep->e_flag |= EN_ACCESS;
  uprintf("EXIT--");
  return (error);
}


static int
edufs_write(ap)
	 struct vop_write_args /* {
							  struct vnode *a_vp;
							  struct uio *a_uio;
							  int a_ioflag;
							  struct ucred *a_cred;
							  } */ *ap;
{
  uprintf("EDUFS_WRITE\n");
  return (ENOSYS);
}


static int
edufs_fsync(ap)
	 struct vop_fsync_args /* {
							  struct vnode *a_vp;
							  struct ucred *a_cred;
							  int a_waitfor;
							  struct thread *a_td;
							  } */ *ap;
{
  uprintf("EDUFS_FSYNC\n");
  return (ENOSYS);
}

static int
edufs_remove(ap)
	 struct vop_remove_args /* {
							   struct vnode *a_dvp;
							   struct vnode *a_vp;
							   struct componentname *a_cnp;
							   } */ *ap;
{
  uprintf("EDUFS_REMOVE\n");
  return (EOPNOTSUPP);
}

static int
edufs_link(ap)
	 struct vop_link_args /* {
							 struct vnode *a_tdvp;
							 struct vnode *a_vp;
							 struct componentname *a_cnp;
							 } */ *ap;
{
  uprintf("EDUFS_LINK\n");
  return (EOPNOTSUPP);
}


static int
edufs_rename(ap)
	 struct vop_rename_args /* {
							   struct vnode *a_fdvp;
							   struct vnode *a_fvp;
							   struct componentname *a_fcnp;
							   struct vnode *a_tdvp;
							   struct vnode *a_tvp;
							   struct componentname *a_tcnp;
							   } */ *ap;
{
  uprintf("EDUFS_RENAME\n");
  return (EOPNOTSUPP);
}

static int
edufs_mkdir(ap)
	 struct vop_mkdir_args /* {
							  struct vnode *a_dvp;
							  struvt vnode **a_vpp;
							  struvt componentname *a_cnp;
							  struct vattr *a_vap;
							  } */ *ap;
{
  uprintf("EDUFS_MKDIR\n");
  return (EOPNOTSUPP);
}

static int
edufs_rmdir(ap)
	 struct vop_rmdir_args /* {
							  struct vnode *a_dvp;
							  struct vnode *a_vp;
							  struct componentname *a_cnp;
							  } */ *ap;
{
  uprintf("EDUFS_RMDIR\n");
  return (EOPNOTSUPP);
}




static int
edufs_symlink(ap)
	 struct vop_symlink_args /* {
								struct vnode *a_dvp;
								struct vnode **a_vpp;
								struct componentname *a_cnp;
								struct vattr *a_vap;
								char *a_target;
								} */ *ap;
{
  uprintf("EDUFS_SYMLINK\n");
  return (EOPNOTSUPP);
}




static int
edufs_readdir(ap)
	 struct vop_readdir_args /* {
								struct vnode *a_vp;
								struct uio *a_uio;
								struct ucred *a_cred;
								int *a_eofflag;
								int *a_ncookies;
								u_long **a_cookies;
								} */ *ap;
{
  uprintf("EDUFS_READDIR\n");
  
  struct uio *uio  = ap->a_uio;
  struct vnode *vp = ap->a_vp;

  struct dirent dirbuf;
  off_t offset, dataoffset;
  u_long dpb; /* directories per block */
  struct edufsmount *emp;
  struct edufs_superblock *esb;
  struct buf *bp;
  struct enode *ep;
  struct edufs_dirblock *db;
  uint32_t logblock;
  int error;
  int dn;     /* onum, dir num */
  int diff;	 /* used to see if the offset is >= file size */
  error = 0;
  int emptydirslots; /* number of empty directory slots this block */
  int count = 0;
  emp = VFSTOEDUFS(vp->v_mount);
  esb = emp->e_esb;
  ep = VTOE(vp);


  
  
  offset = uio->uio_offset;
  
  if (uio->uio_resid < sizeof(struct dirent) ||
	  (offset & (sizeof(struct dirent) - 1))) {
	uprintf("returning here");
	return (EINVAL);
  }

  
  if (ap->a_ncookies) {
	uprintf("NOT NFS ENABLED");
	return (EINVAL);
  }
  
  dpb = esb->fs_bps / sizeof(struct edufs_dirblock);

  uprintf("dirblocks per block = %ld\n",dpb);
  
  while (uio->uio_resid > 0) { 
    uprintf("residue = %d\n",uio->uio_resid);
	count++;
    if(count == 5000) 
      panic("readdir count == 5000");

    /* check for offset greater than file size */  
    diff = ep->e_size - offset;
    if (diff <= 0) {
      uprintf("breaking out of loop!\n");
      goto out;
    }

    
    /* figure out what logical block we are on */
	/* this code needs to be in a function etc */
    logblock = offset / esb->fs_bsize;

  
    /* read in a block */					
    uprintf("Calling bread with log %d %d\n",logblock,esb->fs_bps);
    error = bread(vp,logblock,esb->fs_bps,NOCRED,&bp);
    uprintf("after bread");

    if(error) {
      brelse(bp);
      return(error);
    } else {
      uprintf("READ BLOCK %d OK\n",logblock);
    }
    
    /* read the dirs on the block */
	dataoffset = offset % esb->fs_bps;
	uprintf("dataoffset = %lld\n",dataoffset);
	uprintf("\t\t\t offset %lld physblock %lld  d\n",bp->b_offset,bp->b_blkno);
	db = (struct edufs_dirblock *)(bp->b_data + dataoffset);

    for(dn = 0,emptydirslots=0; dn < dpb; dn++,db++) {
      if(offset > esb->fs_bps) {
		uprintf("DIR %d BIGGER THAN 1 DISK BLOCK\n",dn);		
		goto out;
      }
	  
      if(db->d_type == 0) {
		/* found an empty dir slot */
		emptydirslots++;
		uprintf("emptydir.");
		/*--------------------------*/
		/* NEED to update offset... */
		/*--------------------------*/
		offset += sizeof(struct edufs_dirblock);      
		continue;		
      } else {
		uprintf("realdir.");
	  }
      
      /*bzero(dirbuf.d_name, sizeof(dirbuf.d_name));*/
	/* clear out the old stuff...*/
      bzero(&dirbuf, sizeof(struct dirent));

      dirbuf.d_fileno = db->d_eno;
      dirbuf.d_type   =	db->d_type;
      dirbuf.d_namlen = db->d_namelen;      
	  
	/* max filename is 25 chars... should always
           have 1 extra byte on the end*/
      memcpy(dirbuf.d_name, db->d_name,db->d_namelen+1);
      /*dirbuf.d_name[dirbuf.d_namlen] = 0;*/				  
      dirbuf.d_reclen = GENERIC_DIRSIZ(&dirbuf);
      
      uprintf("\n directory block enode = %d \n",dirbuf.d_fileno);
      uprintf("d_type= %d \n",dirbuf.d_type);
      uprintf("name len = %d \n",dirbuf.d_namlen);
      uprintf("name = %s \n",dirbuf.d_name);
      uprintf("reclen = %d \n",dirbuf.d_reclen);

      if (uio->uio_resid < dirbuf.d_reclen) {
		uprintf("resid < reclen	 ");
		brelse(bp);
		goto out;
      }	
	  
      error = uiomove(&dirbuf, dirbuf.d_reclen, uio);
      
      if(error) {
		uprintf("uiomove error\n");
		brelse(bp);
		goto out;	  
      }
      
      offset += sizeof(struct edufs_dirblock);      
    }
            
    brelse(bp);	
  }
  uprintf("getting out!");
 out:	
  uio->uio_offset = offset;		
  uprintf("new uio offset = %lld\n",uio->uio_offset);
  return (error);
}

static int
edufs_bmap(ap)
	 struct vop_bmap_args /* {
							 struct vnode *a_vp;
							 daddr_t a_bn;
							 struct vnode **a_vpp;
							 daddr_t *a_bnp;
							 int *a_runp;
							 int *a_runb;
							 } */ *ap;
{
  uprintf("EDUFS_BMP\n");
  return (ENOSYS);
}


static int
edufs_strategy(ap)
	 struct vop_strategy_args /* {
								 struct vnode *a_vp;
								 struct buf *a_bp;
								 } */ *ap;
{

  struct buf *bp = ap->a_bp;
  struct vnode *vp = ap->a_vp;
  struct enode *ep;

  struct vnode *dvp; /* device vnode ptr */
  
  uint32_t logblock = 0;
  int bn = 0;
  

  uprintf("EDUFS_STRATEGY\n");  
  
  ep = VTOE(vp);
  
  /*if(bp->b_iocmd == BIO_READ) {*/

  if (vp->v_type == VBLK || vp->v_type == VCHR)
	panic("edufs_strategy: spec");
  
  if(bp->b_blkno == bp->b_lblkno) {
	logblock = bp->b_offset / ep->e_fs->fs_bsize;
	uprintf("logical block = %d\n",logblock);
	bn = ep->den->de_db[logblock] / ep->e_fs->fs_bps;
	uprintf("Try to read %d\n",bn);
	/* set physical block number */
	bp->b_blkno = bn;	
	
	/* set logical block number */	
	bp->b_lblkno = logblock;

	if((long)bp->b_blkno == -1) {
	  uprintf("clrbuf");
	  vfs_bio_clrbuf(bp);
	}
	
  }
  uprintf("here");
  if((long)bp->b_blkno == -1) {
	uprintf("blkno==-1");
	bufdone(bp);
	return(0);
  }

  uprintf("strategy foo");
  dvp=ep->e_devvp;
  bp->b_dev = dvp->v_rdev;
  bp->b_iooffset = dbtob(bp->b_blkno);
  VOP_SPECSTRATEGY(dvp,bp);
  
  uprintf("strategy done");
  return (0);	 
}


static int
edufs_print(ap)
	 struct vop_print_args /* {
							  struct vnode *vp;
							  } */ *ap;
{
  uprintf("EDUFS_PRINT\n");
  return (0);
}

static int
edufs_pathconf(ap)
	 struct vop_pathconf_args /* {
								 struct vnode *a_vp;
								 int a_name;
								 int *a_retval;
								 } */ *ap;
{
  uprintf("EDUFS_PATHCONF\n");
  int error;
  
  error = 0;
  switch (ap->a_name) {
  case _PC_LINK_MAX:
	*ap->a_retval = LINK_MAX;
	break;
  case _PC_NAME_MAX:
	*ap->a_retval = NAME_MAX;
	break;
  case _PC_PATH_MAX:
	*ap->a_retval = PATH_MAX;
	break;
  case _PC_PIPE_BUF:
	*ap->a_retval = PIPE_BUF;
	break;
  case _PC_CHOWN_RESTRICTED:
	*ap->a_retval = 1;
	break;
  case _PC_NO_TRUNC:
	*ap->a_retval = 1;
	break;
  case _PC_ACL_EXTENDED:
	*ap->a_retval = 0;
	break;
  case _PC_ACL_PATH_MAX:
	*ap->a_retval = 3;
	break;
  case _PC_MAC_PRESENT:
	*ap->a_retval = 0;
	break;
  case _PC_ASYNC_IO:
	/* _PC_ASYNC_IO should have been handled by upper layers. */
	KASSERT(0, ("_PC_ASYNC_IO should not get here"));
	error = EINVAL;
	break;
  case _PC_PRIO_IO:
	*ap->a_retval = 0;
	break;
  case _PC_SYNC_IO:
	*ap->a_retval = 0;
	break;
  case _PC_ALLOC_SIZE_MIN:
	*ap->a_retval = ap->a_vp->v_mount->mnt_stat.f_bsize;
	break;
  case _PC_FILESIZEBITS:
	*ap->a_retval = 32;	 /* doesnt support 64 bits yet */
	break;
  case _PC_REC_INCR_XFER_SIZE:
	*ap->a_retval = ap->a_vp->v_mount->mnt_stat.f_iosize;
	break;
  case _PC_REC_MAX_XFER_SIZE:
	*ap->a_retval = -1; /* means ``unlimited'' */
	break;
  case _PC_REC_MIN_XFER_SIZE:
	*ap->a_retval = ap->a_vp->v_mount->mnt_stat.f_iosize;
	break;
  case _PC_REC_XFER_ALIGN:
	*ap->a_retval = PAGE_SIZE;
	break;
  case _PC_SYMLINK_MAX:
	*ap->a_retval = MAXPATHLEN;
	break;
	
  default:
	error = EINVAL;
	break;
  }
  return (error);

}



static int
edufs_inactive(ap)
	 struct vop_inactive_args /* {
								 struct vnode *a_vp;
								 struct thread *a_td;
								 } */ *ap;
{
  uprintf("VNOPS::INACTIVE\n");	 

  struct vnode *vp = ap->a_vp;
  struct enode *ep = VTOE(vp);
  struct thread *td = ap->a_td;
  mode_t mode;
  int error = 0;
	
  VI_LOCK(vp);
  if (prtactive && vp->v_usecount != 0)
	uprintf("edufs_inactive: pushing active");
  VI_UNLOCK(vp);
	
  /*
   * Ignore inodes related to stale file handles.
   */
  if (ep->e_mode == 0) {
	uprintf("MODE is 0 - go to out\n");
	goto out;
  }
  if (ep->e_nlink <= 0) {
	(void) vn_write_suspend_wait(vp, NULL, V_WAIT);

	mode = ep->e_mode;
	ep->e_mode = 0;
	ep->den->de_mode = 0;
	ep->e_flag |= EN_CHANGE | EN_UPDATE;
	/*edufs_vfree(vp, ep->e_number, mode);*/
	uprintf("************Supposed to call edufs vfree\n");
  }

  /*
    if (ep->e_flag & (EN_ACCESS | EN_CHANGE | EN_MODIFIED | EN_UPDATE)) {
    uprintf("IN: 1:");
    if ((ep->e_flag & (EN_CHANGE | EN_UPDATE | EN_MODIFIED)) == 0 &&
    vn_write_suspend_wait(vp, NULL, V_NOWAIT)) {
    ep->e_flag &= ~EN_ACCESS;
    uprintf("IN: 2:");
    } else {
    (void) vn_write_suspend_wait(vp, NULL, V_WAIT);
    
    deupdate(vp, 0);
    uprintf("supposed to update timestamps etc\n");		
    }
    }*/
  
 out:  
  VOP_UNLOCK(vp, 0, td);
  /*
   * If we are done with the inode, reclaim it
   * so that it can be reused immediately.
   */
  if (ep->e_mode == 0) {
	vrecycle(vp, NULL, td);
	uprintf("INACTIVE_>RECYCLE");
  }
  uprintf("IN-error[%d]\n",error);
  return (error);
		  
}


static int
edufs_reclaim(ap)
	 struct vop_reclaim_args /* {
								struct vnode *a_vp;
								struct thread *a_td;
								} */ *ap;
{
  struct vnode *vp = ap->a_vp;
  struct enode *ep = VTOE(vp);
  /*	struct edufsmount *emp = ep->e_emp;*/
  uprintf("VNOPS::RECLAIM\n");

  if (prtactive && vrefcnt(vp) != 0)
	vprint("edufs_reclaim(): pushing active", vp);
  /*
   * Remove the enode from its hash chain.
   */
  edufs_ehashrem(ep);
  /*
   * Purge old data structures associated with the denode.
   */
  cache_purge(vp);
  if (ep->e_devvp) {
	vrele(ep->e_devvp);
	ep->e_devvp = 0;
  }
  
  uma_zfree(uma_denode,ep->den);
  uma_zfree(uma_enode,ep);	
  vp->v_data = NULL;
  uprintf("reclaimed!");
  return (0);	 
}






/*
  static void
  edufs_efree(struct edufsmount *emp, struct enode *ep)
  {
  uprintf("EDUFS_EFREE\n");
  uma_zfree(uma_enode, ep);
  }
*/


/* can probably break the enode stuff out of here */

static int edufs_findfreeenode(struct edufsmount *emp, uint32_t *fenum,uint32_t *fcg) {
  struct edufs_superblock *esb = emp->e_esb;
  struct cg *acg = emp->cglist;
  struct buf *bp;
  int error;
  int cgcount;
  unsigned char *bm;
  int bmcount;
  int bit;
  uint32_t fblockct, fsblock;
  
  if(emp->e_esb->fs_cstotal.cs_nefree < 1 ||
	 emp->e_esb->fs_cstotal.cs_nbfree < 1) {
	uprintf("NO BLOCKS/ENODES LEFT\n");
	return 1;
  }

	 
  for(cgcount = 0; cgcount < esb->fs_ncg;cgcount++,acg++) {
	if(acg->cg_cs.cs_nefree > 0 && acg->cg_cs.cs_nbfree) {
	  /* check and see if this cyl group has free enodes and free blocks */
	  uprintf("Cyl group %d has free space	 ",cgcount);
	  uprintf("enode used list is at phys block %d\n",acg->cg_eusedoff / esb->fs_bps);


	  /* phys blocks per filesystem block */
	  /* bread can only read 512 bytes blocks, but FS blocks are probably 4096 or
		 something... so we need to read several phys blocks to check the free list */
	  for(fblockct = 0; fblockct < (esb->fs_bsize / esb->fs_bps); fblockct++) {
		
		fsblock = (acg->cg_eusedoff / esb->fs_bps) + fblockct;
		error = bread(emp->e_devvp,fsblock,esb->fs_bsize,NOCRED,&bp);
		if(error) {
		  brelse(bp);
		  uprintf("cant read in used enode map in cg # %d\n",cgcount);
		  return 1;
		}
		
		uprintf("searching for free enode\n");
		for(bmcount = 0,bm = (char*)bp->b_data;
			bmcount < esb->fs_bsize;bmcount++,bm++) {
		  uprintf("this byte looks like this: %d\n",*bm);
		  if(*bm != 0xFF) {
			uprintf("found a byte thats not full  ");
			
			/* dont let people pick enodes 0,1, or 2 */
			if(bmcount == 0 && cgcount == 0) {
			  bit = 4;
			} else {
			  bit = 7;
			}
			
			for(; bit >= 0; bit--) {
			  uprintf("b%d",bit);
			  if((*bm & (1 << bit)) == 0) {
				uprintf("found free bit: %d bit #%d \n",bmcount,bit);
				
				*fenum = (fblockct * esb->fs_bps) +		  /* phys block of enode list		*/
				  (8 * bmcount) +						  /* byte number in this phys block */
				  (8-bit-1);							  /* bits 7..0						*/
				*fcg = cgcount;
				return 0;
			  }
			}  
		  }
		}
		uprintf("\n");
		bqrelse(bp);	
		
	  } /* for(fblockct) */	  
	}
  }
  /* enospc probably isnt the correct error... */
  return ENOSPC;
}


static int edufs_valloc(pvp, mode, cred, vpp)
	 struct vnode *pvp;
	 int mode;
	 struct ucred *cred;
	 struct vnode **vpp;
{
  struct enode *pep;  
  /*struct enode *ep;*/
  /*struct timespec ts;*/
  uint32_t fenum, fcg;
  struct edufsmount *emp;  
  int error;
  uprintf("edufs_valloc ");
  emp = VFSTOEDUFS(pvp->v_mount); 
  *vpp = NULL;
  pep = VTOE(pvp);
  /* check for free enodes */

  error = edufs_findfreeenode(emp,&fenum,&fcg);
  if(error)
	return error;
  uprintf("found free enode - calling vget");
  /* mark this enode used
	 on error, free up the enode */
  error = edufs_vget(pvp->v_mount, fenum, LK_EXCLUSIVE, vpp);	   
  if (error) {
	/*UFS_VFREE(pvp, fenum, mode);*/
	uprintf("error from vget");
	return (error);
  }
  uprintf("no error in valloc");
  return 3;
  
  /*ep = VTOI(*vpp);
	ip->i_flags = 0;
	DIP(ip, i_flags) = 0;  
	if (ip->i_gen == 0 || ++ip->i_gen == 0)
	ip->i_gen = arc4random() / 2 + 1;
	DP(ip, i_gen) = ip->i_gen;*/
  /* send return (ENOSPC) if no enodes left */
}


/*
 * Allocate a new enode.
 * Vnode dvp must be locked.
 */
static int
edufs_makeenode(mode, dvp, vpp, cnp)
	 int mode;
	 struct vnode *dvp;
	 struct vnode **vpp;
	 struct componentname *cnp;
{
  struct enode *pdir;
  /*struct direct newdir;*/
  struct vnode *tvp;

  int error;

  uprintf("makeenode");
	
  pdir = VTOE(dvp);
  uprintf("dir enode # is %d\n",pdir->e_number);
  *vpp = NULL;

  if ((mode & DIFMT) == 0)
	mode |= DIFREG;
	
  error = edufs_valloc(dvp, mode, cnp->cn_cred, &tvp);
	
  return 3;
}

/*	
  if (error)
  return (error);
  ip = VTOI(tvp);
  ip->i_gid = pdir->i_gid;
  DIP(ip, i_gid) = pdir->i_gid;

  } else {
  ip->i_uid = cnp->cn_cred->cr_uid;
  DIP(ip, i_uid) = ip->i_uid;
  }
  }

  ip->i_uid = cnp->cn_cred->cr_uid;
  DIP(ip, i_uid) = ip->i_uid;


  ip->i_flag |= IN_ACCESS | IN_CHANGE | IN_UPDATE;
  ip->i_mode = mode;
  DIP(ip, i_mode) = mode;

  tvp->v_type = IFTOVT(mode);	 Rest init'd in getnewvnode(). 
  ip->i_effnlink = 1;
  ip->i_nlink = 1;
  DIP(ip, i_nlink) = 1;
  if (DOINGSOFTDEP(tvp))
  softdep_change_linkcnt(ip);
  if ((ip->i_mode & ISGID) && !groupmember(ip->i_gid, cnp->cn_cred) &&
  suser_cred(cnp->cn_cred, PRISON_ROOT)) {
  ip->i_mode &= ~ISGID;
  DIP(ip, i_mode) = ip->i_mode;
  }

  if (cnp->cn_flags & ISWHITEOUT) {
  ip->i_flags |= UF_OPAQUE;
  DIP(ip, i_flags) = ip->i_flags;
  }

	
  Make sure inode goes to disk before directory entry.
	 
  error = UFS_UPDATE(tvp, !(DOINGSOFTDEP(tvp) | DOINGASYNC(tvp)));
  if (error)
  goto bad;

  ufs_makedirentry(ip, cnp, &newdir);
  error = ufs_direnter(dvp, tvp, &newdir, cnp, NULL);
  if (error)
  goto bad;
  *vpp = tvp;
  return (0);

  bad:
	
  Write error occurred trying to update the inode
  or the directory so must deallocate the inode.
	 
  ip->i_effnlink = 0;
  ip->i_nlink = 0;
  DIP(ip, i_nlink) = 0;
  ip->i_flag |= IN_CHANGE;
  if (DOINGSOFTDEP(tvp))
  softdep_change_linkcnt(ip);
  vput(tvp);

*/

/*	
  return (error);
  }*/




void
edufs_etimes(vp)
	 struct vnode *vp;
{
  struct enode *ep;
  struct timespec ts;

  ep = VTOE(vp);
  if ((ep->e_flag & (EN_ACCESS | EN_CHANGE | EN_UPDATE)) == 0)
	return;

  /*if ((vp->v_type == VBLK || vp->v_type == VCHR) && !DOINGSOFTDEP(vp))
	ep->e_flag |= EN_LAZYMOD;
	else*/
	
  ep->e_flag |= EN_MODIFIED;
  if ((vp->v_mount->mnt_flag & MNT_RDONLY) == 0) {
	vfs_timestamp(&ts);
	if (ep->e_flag & EN_ACCESS) {
	  ep->den->de_atime = ts.tv_sec;
	  ep->den->de_atimensec = ts.tv_nsec;
	}
	if (ep->e_flag & EN_UPDATE) {
	  ep->den->de_mtime = ts.tv_sec;
	  ep->den->de_mtimensec = ts.tv_nsec;
	}
	if (ep->e_flag & EN_CHANGE) {
	  ep->den->de_ctime = ts.tv_sec;
	  ep->den->de_ctimensec = ts.tv_nsec;
	}
  }
  ep->e_flag &= ~(EN_ACCESS | EN_CHANGE | EN_UPDATE);
}



/*
 * Global vfs data structures
 */
vop_t **edufs_vnodeop_p;
static struct vnodeopv_entry_desc edufs_vnodeop_entries[] = {
  { &vop_default_desc,		(vop_t *) vop_defaultop },
  { &vop_access_desc,			(vop_t *) edufs_access },
  { &vop_bmap_desc,			(vop_t *) edufs_bmap },
  { &vop_cachedlookup_desc,	(vop_t *) edufs_lookup },
  { &vop_close_desc,			(vop_t *) edufs_close },
  { &vop_create_desc,			(vop_t *) edufs_create },
  { &vop_fsync_desc,			(vop_t *) edufs_fsync },
  { &vop_getattr_desc,		(vop_t *) edufs_getattr },
  { &vop_inactive_desc,		(vop_t *) edufs_inactive },
  { &vop_link_desc,			(vop_t *) edufs_link },
  { &vop_lookup_desc,			(vop_t *) vfs_cache_lookup },
  { &vop_mkdir_desc,			(vop_t *) edufs_mkdir },
  { &vop_mknod_desc,			(vop_t *) edufs_mknod },
  { &vop_open_desc,			(vop_t *) edufs_open },
  { &vop_pathconf_desc,		(vop_t *) edufs_pathconf },
  { &vop_print_desc,			(vop_t *) edufs_print },
  { &vop_read_desc,			(vop_t *) edufs_read },
  { &vop_readdir_desc,		(vop_t *) edufs_readdir },
  { &vop_reclaim_desc,		(vop_t *) edufs_reclaim },
  { &vop_remove_desc,			(vop_t *) edufs_remove },
  { &vop_rename_desc,			(vop_t *) edufs_rename },
  { &vop_rmdir_desc,			(vop_t *) edufs_rmdir },
  { &vop_setattr_desc,		(vop_t *) edufs_setattr },
  { &vop_strategy_desc,		(vop_t *) edufs_strategy },
  { &vop_symlink_desc,		(vop_t *) edufs_symlink },
  { &vop_write_desc,			(vop_t *) edufs_write },

  { NULL, NULL }
};

static struct vnodeopv_desc edufs_vnodeop_opv_desc =
  { &edufs_vnodeop_p, edufs_vnodeop_entries };

VNODEOP_SET(edufs_vnodeop_opv_desc);
