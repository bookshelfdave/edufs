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
 *  Pieces of this program were taken from newfs_msdos, written by Robert Nordier
 */

#include "utils.h"
#include "newfs_edufs.h"

extern struct edufs_superblock esb;
extern struct disklabel *lp, dlp;
extern int verbose;



/*
 * Exit with error if file system is mounted.
 */
void
check_mounted(const char *fname, mode_t mode)
{
    struct statfs *mp;
    const char *s1, *s2;
    size_t len;
    int n, r;

	/* getmntinfo returns a list of mounted devices */
	/* make sure at least one came back */
    if (!(n = getmntinfo(&mp, MNT_NOWAIT)))
	  err(1, "getmntinfo");

	/* _PATH_DEV=="/dev/" */
	len = strlen(_PATH_DEV);
    s1 = fname;

	/* see if the filename starts with /dev/ */
	/* if it does, chop off /dev/ */
	if (!strncmp(s1, _PATH_DEV, len))
	  s1 += len;

	/* make sure its a char device, etc */
	/* i have no idea why you would check and see if
	   s1 == 'r' */
	r = S_ISCHR(mode) &&
	  s1 != fname &&
	  *s1 == 'r';
	
	/* go through each mount point and see if our device is mounted*/
    for (; n--; mp++) {
	  s2 = mp->f_mntfromname;
	  if (!strncmp(s2, _PATH_DEV, len))
		s2 += len;
	  if ((r && s2 != mp->f_mntfromname && !strcmp(s1 + 1, s2)) ||
		  !strcmp(s1, s2))
		errx(1, "%s is mounted on %s", fname, mp->f_mntonname);
    }
}



void getdiskstats(int fd, const char *fname, int oflag) {

  struct fd_type type;
  off_t ms, hs;
  
  lp = NULL;

  /* probably dont need to do this but i didnt want
	 bad values in the dlp structure */
  memset(&dlp,0,sizeof(struct disklabel));
  
  if(ioctl(fd,DIOCGMEDIASIZE, &ms) == -1) 
	errx(1,"Cannot get disk size, %s\n", strerror(errno));

  if(ioctl(fd, FD_GTYPE, &type) != -1) {
	/* yes its a floppy drive, get the parameters */
	SDBG("The disk is a floppy drive\n");
	dlp.d_secsize = 128 << type.secsize;
	SDBG("\tSector size = %d\n", type.secsize);
	SDBG("\tBytes per sector << 128 = %d\n", dlp.d_secsize);
	
	SDBG("\tSectors per track = %d\n", type.sectrac);	
	dlp.d_nsectors = type.sectrac;

	SDBG("\tTracks per cylinder = %d\n", type.heads);
	dlp.d_ntracks = type.heads;

	dlp.d_secperunit = ms / dlp.d_secsize;
	SDBG("\tSectors per unit = %d\n", dlp.d_secperunit);

	SDBG("\tDisk size =  %lld\n", ms);

	lp = &dlp;
	hs = 0;
  }
  
  /* Maybe it's a fixed drive */
  if (lp == NULL) {

	/* get and set disklabel */
	if (!ioctl(fd, DIOCGDINFO, &dlp)) {

	  /*-
	   * Get the sectorsize of the device in bytes.  The sectorsize is the
	   * smallest unit of data which can be transfered from this device.
	   * Usually this is a power of two but it may not be. (ie: CDROM audio)
	   */	  

	  if (ioctl(fd, DIOCGSECTORSIZE, &dlp.d_secsize) == -1)
		errx(1, "Cannot get sector size, %s\n", strerror(errno));

	  
	  /*-
	   * Get the firmwares notion of number of sectors per track.  This
	   * value is mostly used for compatibility with various ill designed
	   * disk label formats.  Don't use it unless you have to.
	   */

	  if (ioctl(fd, DIOCGFWSECTORS, &dlp.d_nsectors) == -1)
		errx(1, "Cannot get number of sectors, %s\n", strerror(errno));

		
        /*-
         * Get the firmwares notion of number of heads per cylinder.  This
         * value is mostly used for compatibility with various ill designed
         * disk label formats.  Don't use it unless you have to.
         */	  

	  if (ioctl(fd, DIOCGFWHEADS, &dlp.d_ntracks)== -1)
		errx(1, "Cannot get number of heads, %s\n", strerror(errno));
	  
	  /* sec per unit = (disk size / sector size) */
	  dlp.d_secperunit = ms / dlp.d_secsize;
	  SDBG("CHECK - sectors per unit = %d\n",dlp.d_secperunit);
	} 
	
	/*hs = (ms / dlp.d_secsize) - dlp.d_secperunit;*/
	lp = &dlp;
	SDBG("The disk is a fixed drive\n");
  }

  DBG("Media size = %lld\n",ms);
  
  esb.fs_bps = lp->d_secsize;  
  esb.fs_bsize = BLOCKSIZE;  
  esb.fs_nsect = lp->d_nsectors;
  esb.fs_size = lp->d_secperunit;    


  /*  DBG("Disk RPM %d \n", lp->d_rpm);*/
  esb.fs_interleave = lp->d_interleave;    
}


void printsuper() {
  printf("-------------------------------------------------\n");
  printf("-- SUPERBLOCK INFO                             --\n");
  printf("-------------------------------------------------\n");
  printf("\tSize of filesystem blocks %d\n",esb.fs_bsize);
  printf("\tNumber of blocks in filesystem %d\n",esb.fs_size);
  printf("\tNumber of tracks %d\n",esb.fs_nsect);
  printf("\tEach cylinder has %d sectors\n",esb.fs_spc);
  printf("\tThis disk has %d cylinders\n",esb.fs_ncyl);
  printf("\tNumber of cylinder groups = %d\n",esb.fs_ncg);
  printf("\tNumber of cylinders is each group = %d\n",esb.fs_cpg);
  printf("\tDisk size = %d\n",esb.fs_size * esb.fs_bps);
  printf("\tInterleave %d\n",esb.fs_interleave);
  printf("\tSize of superblock = %d\n",esb.fs_sbsize);
}


void printsuper2() {

  printf("-------------------------------------------------\n");
  printf("-- SUPERBLOCK INFO                             --\n");
  printf("-------------------------------------------------\n");
  printf("\tSize of filesystem blocks %d\n",esb.fs_bsize);
  printf("\tNumber of blocks in filesystem %d\n",esb.fs_size);
  printf("\tNumber of tracks %d\n",esb.fs_nsect);
  printf("\tEach cylinder has %d sectors\n",esb.fs_spc);
  printf("\tThis disk has %d cylinders\n",esb.fs_ncyl);
  printf("\tNumber of cylinder groups = %d\n",esb.fs_ncg);
  printf("\tNumber of cylinders is each group = %d\n",esb.fs_cpg);
  printf("\tDisk size = %d\n",esb.fs_size * esb.fs_bps);
  printf("\tInterleave %d\n",esb.fs_interleave);
  printf("\tSize of superblock = %d\n",esb.fs_sbsize);


  printf("\tHistoric filesystem linked list: %d\n",esb.fs_firstfield);  
  printf("\tOffset of superblock in filesys: %d\n",esb.fs_sblkno);
  printf("\tOffset of first cyl block: %d\n",esb.fs_cblkno);
  printf("\tOffset of inode blocks: %d\n",esb.fs_iblkno);
  printf("\tOffset of first data after cg: %d\n",esb.fs_dblkno);
  
  printf("\tBytes per sector: %d\n",esb.fs_bps);
  printf("\tEnodes per group: %d\n",esb.fs_epg);  
  printf("\tNumber of blocks in fs: %d\n",esb.fs_size);
  printf("\tNumber of data blocks in fs: %d\n",esb.fs_dsize);
  printf("\tCG Offset in cylinder: %d\n",esb.fs__cgoffset);
  printf("\tLast time written: %d\n",esb.fs_time);
  printf("\tInterleave: %d\n",esb.fs_interleave);
  /*u_char	 fs_fsmnt[MAXMNTLEN];*/	/* name mounted on */
  /*u_char	 fs_volname[MAXVOLLEN];*/	/* volume name */

  /*printf("\tNumber of indirects: %d\n",esb._nindir);*/
  printf("\tBlock add of cyl grp summ area: %d\n",esb.fs_csaddr);
  printf("\tSize of cyl grp summary area: %d\n",esb.fs_cssize);
  printf("\tCylinder group size: %d\n",esb.fs_cgsize);

  /* cleared at runtime */
  printf("\tSuper block modified flag: %d\n",esb.fs_fmod);
  printf("\tFilesystem is clean flag: %d\n",esb.fs_clean);
  printf("\tMounted read only %d\n",esb.fs_ronly);          
  /*int32_t	 fs_id[2];*/		/* unique filesystem id */
  
      
  /* these fields retain the current block allocation info */
  printf("\tLast cylinder group searched: %d\n",esb.fs_cgrotor);

  /*u_int8_t *fs_contigdirs;*/	/* # of contiguously allocated dirs */
  
  /*int64_t	 fs_sblockloc;		*//* byte offset of standard superblock */

  printf("\tCylinder summary:\n");
  printf("\t-->Number of directories: %lld\n",esb.fs_cstotal.cs_ndir);
  printf("\t-->Number of free blocks: %lld\n",esb.fs_cstotal.cs_nbfree);
  printf("\t-->Number of free enodes: %lld\n",esb.fs_cstotal.cs_nefree);  

  printf("\tMax length internal sym link: %d\n",esb.fs_maxsymlinklen);
  printf("\tMax representable file size: %d\n",0);
  printf("\tMagic number: %d\n",esb.fs_magic);

}


void printcg(struct cg *g) {	
  printf("===================================\n");
  printf("Cylinder group #%d\n",g->cg_cgx);
  printf("\tOffset of next cg: %d\n",g->cg_next);
  printf("\tMagic #%d\n",g->cg_magic);
  printf("\tNumer of cyls this group: %d\n",g->cg_ncyl);
  printf("\tNumer of data blocks this cyl: %d\n",g->cg_ndblk);
  printf("\tNumer of enode blocks this group: %d\n",g->cg_neblk);
  printf("\t-->Numer of directories: %d\n",g->cg_cs.cs_ndir);
  printf("\t-->Numer of free blocks: %d\n",g->cg_cs.cs_nbfree);
  printf("\t-->Numer of free denodes: %d\n",g->cg_cs.cs_nefree);  
  printf("\tOffset of used enode map: %d\n",g->cg_eusedoff);
  printf("\tOffset of fre block map:  %d\n",g->cg_freeoff);
  printf("\tOffset of enodes: %d\n",g->cg_enodeoff);
  printf("\tPosition of last used block: %d\n",g->cg_rotor);
  printf("\tPosition of last used enode: %d\n",g->cg_irotor);
  printf("\tNext free offset: %d\n",g->cg_nextfreeoff);
  printf("\tLast initialized enode: %d\n",g->cg_initediblk);
  printf("\tLast time written: %d\n",g->cg_time);
  printf("Size of cg struct: %d\n",sizeof(struct cg));
}

char* blockstuff(int len, char *data) {  
  char *buf;
  if(len > esb.fs_bsize) {
	SDBG("block stuff reports a block too big...\n");
	return 0;
  }
      
  buf = malloc(esb.fs_bsize);
  bzero(buf,esb.fs_bsize);    
  memcpy(buf,data,len);
  
  return buf;
}
