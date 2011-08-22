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


#include "newfs_edufs.h"
#include "utils.h"

struct edufs_superblock esb;
struct disklabel *lp, dlp;
struct cg *allcg;

#ifndef VERBOSE
#define VERBOSE 1
#endif

#define UMASK            0755

int verbose = VERBOSE; /* on by default */

/* ok to use ino_t - just turns out to be an
   unsigned int */
#define ROOTENO ((ino_t)2)

#define PREDEFDIR 2
#define FIRSTBLOCK 0

/* root dir info */
/*struct directblock root_dir[] = {
  { ROOTENO, sizeof(struct directblock), DT_DIR, 1, "." },
  { ROOTENO, sizeof(struct directblock), DT_DIR, 2, ".." },
  };*/


/* dir blocks are always 32 bytes in EDUFS */
struct edufs_dirblock dots[] = {
  { ROOTENO, DT_DIR, 1, "." },
  { ROOTENO, DT_DIR, 2, ".." },
};


#define MAGIC 0x5DFB
#define VERSION "NEWFS_EDUFS v0.7"

void adjustcg(int *bytespercg, int *numenodes, int *enodeheaderlen, int start);
void createenodes(int fd,uint32_t numenodes, uint32_t *enodeindex);
void enodechunk(int fd,int num,uint32_t *enodeindex);
void initdenode(struct denode* pde,int index);
void initfs();
void writeblock(int blocknum,char *buf, size_t size);
void writeenode(int blocknum,struct denode *de);
off_t blockoff(int blocknum);
off_t enodeoff(int enodenum);
void deprint(struct denode *dp);

int fd;
time_t utime;

int main(int argc, char *argv[]) {
  static char opts[] = "Nv";
  const char *fname;
  char buf[MAXPATHLEN];
  int ch, n;
  int fakeit = 0;
  struct stat sb;  
  char *superblock;
  struct cg *ncg;

  
  uint32_t cgoffset  = 0;
  uint32_t cgloop;
  uint32_t enodeindex = 0;
  
  int numenodes  = 0;     /* number of enodes per cg */ /* this should be somewhere else */
  int enodeheaderlen = 0; /* this should be somewhere else */  
  int preferredblocks = 0;
  
  while((ch = getopt(argc, argv, opts)) != -1) {
	switch(ch) {
	case 'N':
	  printf("Not really creating the filesystem\n");
	  fakeit = 1;
	  break;
	
	case 'v':
	  printf("Verbose on\n");
	  break;
	default:
	  printusage();
	}
	
  }

  /* finish up with getopt */
  argc -= optind;
  argv += optind;

  if (argc < 1 || argc > 2)
	printusage();

  
  /* get the name of the device... */
  fname = *argv++;  
  if (!strchr(fname, '/')) {
	snprintf(buf, sizeof(buf), "%s%s", _PATH_DEV, fname);
	if (!(fname = strdup(buf)))
	  err(1, NULL);
  } else {
	
  }

  if(fakeit)
	printf("Faking filesystem creation\n");
  
  if ((fd = open(fname, fakeit ? O_RDONLY : O_RDWR)) == -1  
	  || fstat(fd, &sb))
	err(1, "%s", fname);
  

  if(!fakeit)
	check_mounted(fname, sb.st_mode);
  
  if (!S_ISCHR(sb.st_mode))
	warnx("warning: %s is not a character device", fname);


  /* get the time for later use */
  time(&utime);

  /* ---------------- */
  /* ---------------- */
  /* build superblock */
  /* ---------------- */
  /* ---------------- */  
  
  bzero(&esb,sizeof(struct edufs_superblock));
  /* get disk stats */

  DBG(VERSION);DBG("\n\n");
  getdiskstats(fd,fname,0);

  esb.fs_magic = MAGIC;
  /* calculate cylindercount this way because floppy disks don't return
	 sectors/cylinder */  
  esb.fs_ncyl = lp->d_secperunit / (lp->d_nsectors * lp->d_ntracks);  

  /* calculate the size of cylinders in sectors */
  esb.fs_spc  = (lp->d_secperunit / esb.fs_ncyl); /* * lp->d_secsize;*/
  esb.fs_ncg = 4;
  esb.fs_cpg =  esb.fs_ncyl / esb.fs_ncg;
  esb.fs_sbsize = sizeof(struct edufs_superblock); /* + anything else */
  
  /* grab 2 sectors for the superblock */
  int sblocksize = lp->d_secsize *2;
  superblock = malloc(sblocksize);
  bzero(superblock,sblocksize);  
  memcpy(superblock,&esb,sizeof(struct edufs_superblock));		 

  /* move past the superblock for now and do all the cyl groups first */
  if((n = lseek(fd,sblocksize,SEEK_SET)) < 0) {
	perror("Error seeking to first cg header location\n");
  }
  /*  if((n = write(fd,superblock,sblocksize)) < 0) {
	perror("Error writing superblock");
	}*/
  
  SDBG("Superblock bytes written %d\n",n);

  /* this is the offset AFTER the superblock */
  /* start the CG's off here */
  int start = lp->d_secsize *2;
  SDBG("Offset after superblock = %d\n",start);

  /* divide the drive into #cg equal parts, minus the superblock */
  int bytespercg = ((esb.fs_size * esb.fs_bps) - start) / esb.fs_ncg;




  numenodes = bytespercg / (BLOCKSIZE *2);
  /* calculate the size of all the enode structs on disk */
  enodeheaderlen = numenodes * sizeof(struct denode);
  
  
  SDBG("Before adjusting: numenodes = %d ",numenodes);
  SDBG(" will occupy %d bytes\n",enodeheaderlen);
  

  /* call this function to see if the free block list
	 can handle the size of the CG */
  /* adjust cg */
  adjustcg(&bytespercg,&numenodes,&enodeheaderlen,start);

  allcg = (struct cg*)malloc(sizeof(struct cg) * esb.fs_ncg);
  
  DBG("Numenodes = %d ",numenodes);
  DBG(" will occupy %d bytes\n",enodeheaderlen);

  printf("Each cylinder group will have %d bytes (%d Mb)\n",
		 bytespercg,bytespercg / 1024 / 1024);
  printf("Each cylinder group will have %d cylinders\n",esb.fs_cpg);

  

  SDBG("Maximum # of blocks that the free block list can hold = %d\n",
		 esb.fs_bsize * 8);

  /* calc cyls / group */
  esb.fs_cpg =  esb.fs_ncyl / esb.fs_ncg;
  esb.fs_cstotal.cs_ndir = 1;
  esb.fs_cstotal.cs_nbfree = 0;
  esb.fs_cstotal.cs_nefree = 0;
    

  /* start right after the superblock */
  cgoffset = start;
  
  
  /************************/
  /************************/
  /* NOW CREATE EACH CG */
  /************************/
  /************************/      
  ncg = allcg;
  for(cgloop = 0; cgloop < esb.fs_ncg; cgloop++) {
	DBG("[CG %d]\n",cgloop);

	/* ------------------------------------ */
	/* ------------------------------------ */
	/* create the cg struct and populate it */
	/* ------------------------------------ */
	/* ------------------------------------ */	
	
	bzero(ncg,sizeof(struct cg));
	ncg->cg_magic = MAGIC;     
	ncg->cg_cgx  = cgloop;	   /* this is cylinder # cgloop */
	ncg->cg_ncyl = esb.fs_cpg; /* cylinders per group */

	
	/* (number of cylinders in this group) * (sectors per cylinder) *
	   (bytes per sector) */
	
	int bytesthisgrp = ncg->cg_ncyl * esb.fs_spc * esb.fs_bps;		
	
	/* the first cylinder group has sblocksize less bytes...*/
	if(!cgloop)
	  bytesthisgrp -= sblocksize;	
	
	/* check and see if the cylinder group starts on a bps boundary */
	if((cgoffset % esb.fs_bps) > 0) {
	  /* subtract out the offset... */
	  int tweak = (esb.fs_bps - (cgoffset % esb.fs_bps));
	  cgoffset += tweak; /* move the offset up a little bit */ 
	  bytesthisgrp -= tweak; /* move the offset down a little bit */	  
	}

	/* calculate the positions of the free block list,
	   enode list, and enodes */
	ncg->cg_freeoff  = cgoffset + esb.fs_bsize; /* next block */
	ncg->cg_eusedoff = ncg->cg_freeoff + esb.fs_bsize;
	ncg->cg_enodeoff = ncg->cg_eusedoff + esb.fs_bsize;
	ncg->cg_next = cgoffset + bytesthisgrp;


	/* already 0 (bzero'd). But if cg is 0 then root directory */
	if(!cgloop)
	  ncg->cg_cs.cs_ndir = 1;
		
	SDBG("Current [%d] Next [%d] Diff [%d]\n",cgoffset,ncg->cg_next,ncg->cg_next-cgoffset);
	
	/* ----------- */
	/* ----------- */
	/* block list  */
	/* ----------- */
	/* ----------- */					
	int blockbytes = bytesthisgrp
	  - esb.fs_bsize     /* for cg header  */
	  - esb.fs_bsize     /* for block list */
	  - esb.fs_bsize     /* for enode list */
	  - enodeheaderlen;  /* enodes         */
	/* the first cg is smallest because of the superblock */
	/* if there is 1 less block in this group, make all the other groups
	   be the same size (to make things simple when looking for a specific
	   block */
	/* using this, you can get the cyl group that a block is in by
	   dividing the block # by the number of blocks per cg */
	if(cgloop == 0) 
	  preferredblocks = blockbytes / esb.fs_bsize;

	/* take note of the number of blocks per cg */
	/* a weird place to record this # but it works for now */
	esb.fs_bpg = preferredblocks;
	
	if((blockbytes / esb.fs_bsize) > preferredblocks)
	  SDBG("lost a block...\n");
	else if((blockbytes / esb.fs_bsize) < preferredblocks) {
	  printf("Too many blocks for this group. PANIC!\n");
	  exit(-1);
	}	  	
		
	/*ncg->cg_ndblk = blockbytes / esb.fs_bsize; *//* divide by the block size */
	ncg->cg_ndblk = preferredblocks; /* divide by the block size */
	ncg->cg_neblk = numenodes;	
	SDBG("Blocks this group = %d\n",ncg->cg_ndblk);

	ncg->cg_cs.cs_nbfree = ncg->cg_ndblk;
	ncg->cg_cs.cs_nefree = numenodes;
	
	if(!cgloop) {
	  /* first cg gets the root dir */
	  ncg->cg_cs.cs_nbfree -= 1;
	  ncg->cg_cs.cs_nefree -= 1;
	} 
	  
	SDBG("Seeking to CG start %d\n",cgoffset);
	/* just to see if it works... */
	if(lseek(fd,cgoffset,SEEK_SET) != cgoffset) {
	  perror("Error seeking to CG start");
	  exit(-1);
	}

	/* create the cylinder group
	   this doesnt include the enodes yet,
	   just the cg struct and 2 lists */
	int cgtopblocks = esb.fs_bsize * 3;
	
	char *cgtop = malloc(cgtopblocks);

	/* cg header + free list +  enode list */
	bzero(cgtop,cgtopblocks);

	/* the only thing that goes in is the actual cg struct,
	   everything else is blank */
	memcpy(cgtop,ncg,sizeof(struct cg));
	/* the rest of the space is left blank for the lists */
	printf("Writing CG @ %d\n",lseek(fd,0,SEEK_CUR));
	if((n = write(fd,cgtop,cgtopblocks)) != cgtopblocks) {
	  perror("Error writing cylinder group header info");
	  exit(-1);
	}		
	free(cgtop); 
	
	
	SDBG("new offset after CG struct (- enodes)= %d\n",cgoffset + cgtopblocks);
	
	/* just for testing ...
	   DBG("Seeking to the end of the CG %d\n",cgoffset + bytesthisgrp);
	   if(lseek(fd,bytesthisgrp,SEEK_CUR) < 0) {
	   DBG("Error seeking to end of CG\n");
	   }
	*/

	
	/* ----------------- */
	/* ----------------- */
	/* create the DENODES */
	/* ----------------- */
	/* ----------------- */
	
	/*int eoff = cgoffset + cgtopblocks;*/	
	/* seek to start of enodes */
	if(ncg->cg_enodeoff != lseek(fd,ncg->cg_enodeoff,SEEK_SET)) {
	  perror("Error seeking to first enode");
	  exit(-1);
	}
	
	createenodes(fd,numenodes,&enodeindex);

	
	/* **** just start the data blocks after the dinodes ??? */
	/* need to think about this one... */	
	ncg->cg_dboff = lseek(fd,0,SEEK_CUR);
	SDBG("First block offset in the group %d\n",ncg->cg_dboff);
	
#ifndef NDEBUG	
	int sanity = enodeheaderlen;
	if(enodeheaderlen % esb.fs_bps) 
	  sanity += esb.fs_bps;

	/* assert that the space for free blocks is what we calculated */
	assert(((ncg->cg_next - ncg->cg_dboff) / sanity) > 0);
#endif
	
	/*
	  if(cgloop == (esb.fs_ncg-1)) 
	  DBG("Sorry my fs left %dkb of unused disk space at the end of your drive \n",
	  ((esb.fs_size * esb.fs_bps) -  (cgoffset + bytesthisgrp)) / 1024);
	*/
	
	cgoffset += bytesthisgrp;
	ncg++;
  }
    
  
  /* populate superblock fields etc */
  esb.fs_cblkno = esb.fs_bps *2;  
  esb.fs_epg = numenodes;
  esb.fs_time = utime;
  esb.fs_cssize = esb.fs_bsize * 3; /* cg + blockfree + enodelist */
  esb.fs_fmod = 0;
  esb.fs_clean = 1;
  esb.fs_ronly = 0;
  esb.fs_cgrotor = 0;
  esb.fs_cstotal.cs_ndir = 1;
  
  int j;
  struct cg *cgp = allcg;


  for(j = 0; j < esb.fs_ncg;j++) {
	esb.fs_dsize += cgp->cg_ndblk;
	esb.fs_cstotal.cs_nbfree += cgp->cg_cs.cs_nbfree;
	esb.fs_cstotal.cs_nefree += cgp->cg_cs.cs_nefree;	  
	cgp++;
  }

  bzero(superblock,sblocksize);  
  memcpy(superblock,&esb,sizeof(struct edufs_superblock));
  
  /* write out the superblock! */
  if((n = lseek(fd,0,SEEK_SET)) < 0) {
	perror("Error seeking to superblock location\n");
  }
  if((n = write(fd,superblock,sblocksize)) < 0) {
	perror("Error writing superblock");
  }

  /* build root etc */
  initfs();

  
  if(verbose ==2)
	printsuper();

  /* just to print them all out */
  cgp = allcg;
  for(j = 0;j<esb.fs_ncg;j++) {
	/*printcg(cgp);*/
	cgp++;
  }    

  free(allcg);  
  close(fd);
  
  printf("Check free memory, dummy. FIX INTS\n");
  return 0;
}







/*
 * Check a disk geometry value.
 */
/*static u_int
  ckgeom(const char *fname, u_int val, const char *msg)
  {
  if (!val)
  errx(1, "%s: no default %s", fname, msg);
  if (val > MAXU16)
  errx(1, "%s: illegal %s %d", fname, msg, val);
  return val;
  }
*/







void printusage() {
  fprintf(stderr,
		  "usage: newfs_edufs [ -options ] special [disktype]\n");
  fprintf(stderr, "where the options are:\n");
  fprintf(stderr, "\t-N don't create file system\n");
  fprintf(stderr, "\t-v Verbose: \n");
  exit(1);
}




void adjustcg(int *bytespercg, int *numenodes, int *enodeheaderlen, int start) {
  /* take a guess at the number of blocks in this CG */
  int blockguess = 0;
  int cgguess = 1;
  SDBG("Adjusting CG count\n");
  while(cgguess) { /* stop at 100 if things arent working out here */
	blockguess = *bytespercg
	  - esb.fs_bsize  /* for cg header  */
	  - esb.fs_bsize  /* for block list */
	  - esb.fs_bsize /* for enode list */
	  - *enodeheaderlen;
	if((esb.fs_bsize * 8) < (blockguess / esb.fs_bsize)) {
	  esb.fs_ncg++;
	  *bytespercg = ((esb.fs_size * esb.fs_bps) - start) / esb.fs_ncg;	
	  *numenodes = *bytespercg / (BLOCKSIZE *2);
	  /* calculate the size of all the enode structs on disk */
	  *enodeheaderlen = *numenodes * sizeof(struct denode);	  
	} else {
	  cgguess = 0; /* done */
	}
	if(esb.fs_ncg > 99) {
	  /* something is not working out here */
	  DBG("Can't calculate the number of CG's\n");
	  exit(-1);
	}
  }  
  esb.fs_cpg = esb.fs_ncyl / esb.fs_ncg;
  
}


void createenodes(int fd,uint32_t numenodes, uint32_t *enodeindex) {

  /* loop stuff */
  int ecount, n;
  int ebcount; /* enode counter per sect */

  int des = sizeof(struct denode);	
  int eps = esb.fs_bps / des;

  
  SDBG("starting at %d\n",*enodeindex);  
  /* how many enodes per disk sect? */

  /* this will fail if bps is a weird number (not a power of 2) */
  assert((esb.fs_bps % des) == 0);

    
  SDBG("Working with groups of %d enodes\n",eps);
  for(ecount = 0;(ecount + eps)< numenodes; ecount+=eps) {	  
	/* allocate a block of enodes that will fit into a sect */	
	enodechunk(fd,eps,enodeindex);	
	if(!(ecount % 1024)) {
	  SDBG("%d ",ecount);
	}
  }
  SDBG("%d ",ecount);
  if(numenodes % eps) {
	/* we have < eps enodes to write at the end */
	SDBG("stragglers %d\n",numenodes % eps);			
	enodechunk(fd,numenodes % eps,enodeindex);
	ecount += (numenodes % eps);
  }

  SDBG("%d\n ",ecount);  
  SDBG("enodes = %d\n",numenodes);
  SDBG("ecount = %d\n",ecount);

}


/* create a fs_bps sized group of enodes and write them to disk */
/* num is the number of enodes to stuff in this sect */
void enodechunk(int fd,int num,uint32_t *enodeindex) {
  int n,  ebcount;
    
  char *sect = malloc(esb.fs_bps);
  bzero(sect,esb.fs_bps);

  
  struct denode *de = (struct denode *)malloc(sizeof(struct denode)*num);
  bzero(de,sizeof(struct denode)*num);

  struct denode *pde = de;


  if(*enodeindex == 0) {
	SDBG("ENODE [0] OFFSET = %lld\n",lseek(fd,0,SEEK_CUR));
  }
  
  /* create all generation numbers */
  for(ebcount = 0;ebcount < num;ebcount++) {		  

	initdenode(pde,*enodeindex);
	
	if(verbose== 2) {
	  if(*enodeindex == 0)
		deprint(pde);
	}
	pde++;
	(*enodeindex)++;
  }
  
  memcpy(sect,de,sizeof(struct denode) * num);
	
  if((esb.fs_bps != write(fd,sect,esb.fs_bps))) {
	perror("Error writing a block of enodes");
	exit(-1);
  }
  
  
  free(sect);
  free(de);
  
}


void initdenode(struct denode* pde,int index) {
  pde->de_gen = arc4random();	  
  pde->de_spare[0] = index;  /* set spare JUST for testing */		  
  pde->de_atime = utime;
  pde->de_ctime = utime;
  pde->de_mtime = utime;
  pde->de_uid   = getuid();
  pde->de_gid   = getgid();
  pde->de_flags = 0;
}

/*
void useblock(int blocknum) {
  
}

void freeblock(int blocknum) {
  
}
*/



void initfs() {
  struct denode node;
  char *dirbuf;
  struct cg *firstcg;

  /* things to write to the disk
	 1) free block bitmap
	 2) used enode map
	 3) denode
	 4) actual data
  */
  
  bzero(&node,sizeof(struct denode));
  initdenode(&node,ROOTENO);
  
  node.de_mode = DIFDIR | UMASK;
  node.de_nlink = PREDEFDIR;
  /* dirbuf holds the disk block with the directory in it */
  /*node.de_size = makedir(root_dir, &dirbuf,PREDEFDIR);  */
  /*node.de_size = DIRBLKSIZ;*/
  /* . and .. */
  node.de_size = 2 * sizeof(struct edufs_dirblock);

  dirbuf = malloc(esb.fs_bps);
  memcpy(dirbuf,dots,sizeof(struct edufs_dirblock) *2);
  
  
  printf("directory size = %lld\n",node.de_size);
  node.de_db[0] = blockoff(FIRSTBLOCK);
  node.de_blocks = 1;  
  
  /* block # of first available block is 0
	 ( in this fs anyways )
	 set bit 0 of free list to used 
	 to set the first bit in a byte, use 1 << 7
  */

  /* PLEASE MAKE THE FREE BLOCK AND ENODE LIST MANIP STUFF
	 INTO FUNCTIONS */
  
  /* 1) modify the free block list */
  DBG("Modifying free block list\n");
  if(allcg->cg_freeoff != lseek(fd,allcg->cg_freeoff,SEEK_SET)) {
	perror("Error seeking to free block list offset");
	exit(-1);
  }

  char *block;

  block = malloc(esb.fs_bps); 
  bzero(block,esb.fs_bps);

  if(esb.fs_bps != read(fd,block,esb.fs_bps)) {
	perror("Error reading free block list block");
	exit(-1);
  }
  
  block[0] = 1 << 7;
  /*printf("b0 = %d\n",block[0]);*/

  /* should probably use SEEK_CUR with -512 bytes (can i do that??) */
  if(allcg->cg_freeoff != lseek(fd,allcg->cg_freeoff,SEEK_SET)) {
	perror("Error seeking to free block list after modify");
	exit(-1);
  }

  if(esb.fs_bps != write(fd,block,esb.fs_bps)) {
	perror("Error writing free block map back to the disk");
	exit(-1);
  }
  SDBG("offset is %d\n",allcg->cg_freeoff);
  SDBG("First byte is %d\n",block[0]);
  free(block);



  
  /* 2) modify enode map */
  /* need to modify enode 2 (the root inode */

  DBG("Modifying enode list\n");
  if(allcg->cg_eusedoff != lseek(fd,allcg->cg_eusedoff,SEEK_SET)) {
	perror("Error seeking to enode list offset");
	exit(-1);
  }

  block = malloc(esb.fs_bps); 
  bzero(block,esb.fs_bps);

  if(esb.fs_bps != read(fd,block,esb.fs_bps)) {
	perror("Error reading enode list block");
	exit(-1);
  }
  
  block[0] = 1 << 5; /* set enode # 2:  1<<5 is the enode #2 */
  
  if(allcg->cg_eusedoff != lseek(fd,allcg->cg_eusedoff,SEEK_SET)) {
	perror("Error seeking to enode list after modify");
	exit(-1);
  }

  if(esb.fs_bps != write(fd,block,esb.fs_bps)) {
	perror("Error writing enode list back to the disk");
	exit(-1);
  }
  SDBG("offset is %d\n",allcg->cg_eusedoff);
  SDBG("First byte is %d\n",block[0]);
  free(block);


  
  /* 3) write denode */  
  /* get root enode offset */
  writeenode(ROOTENO,&node);
			 
  
  /* 4) write block data */  
  writeblock(FIRSTBLOCK,dirbuf,DIRBLKSIZ);  
  
}


/*
 * construct a set of directory entries in "buf".
 * return size of directory.
 */

/*int
makedir(struct directblock *protodir,char **buf,int entries)
{
	char *cp;
	int i, spcleft;

	*buf = malloc(DIRBLKSIZ);

	spcleft = DIRBLKSIZ;
	printf("space left = %d\n",spcleft);
	memset(*buf, 0, DIRBLKSIZ);
	for (cp = *buf, i = 0; i < entries - 1; i++) {	  
	  protodir[i].d_reclen = DIRSIZ(0, &protodir[i]);
	  printf("Reclen = %d\n",DIRSIZ(0, &protodir[i]));
	  memmove(cp, &protodir[i], protodir[i].d_reclen);
	  cp += protodir[i].d_reclen;
	  spcleft -= protodir[i].d_reclen;
	  printf("space left = %d\n",spcleft);
	}
	protodir[i].d_reclen = spcleft;
	memmove(cp, &protodir[i], DIRSIZ(0, &protodir[i]));
	return (DIRBLKSIZ);
}*/




void writeblock(int blocknum,char *buf, size_t size) {
  off_t offset = blockoff(blocknum);
  SDBG("writeblock #%d offset %lld\n",blocknum,offset);
  if(offset != lseek(fd,offset,SEEK_SET)) {
	perror("Error seeking for block write");
	exit(-1);
  }

  if(size != write(fd,buf,size)) {
	perror("Error writing disk block");
	exit(-1);
  }

}

void writeenode(int enodenum,struct denode* de) {
  /* this is a bit harder... need to read the block
	 starting at a esb.fs_bps boundary */

  /* denode size */  
  off_t offset = enodeoff(enodenum);
  off_t readoffset = offset;
  char *ebuf = malloc(esb.fs_bps);
  struct denode *dp;

  /* allocate enough space to hold an enode "chunk" */
  bzero(ebuf,esb.fs_bps);  
  
  /* see if its at a bps boundary */
  SDBG("Trying to write enode %d\n",enodenum);  


  readoffset -= offset % esb.fs_bps;
  SDBG("Offset = %lld\n",offset);
  SDBG("Beginning of enodechunk offset = %lld\n",readoffset);

  if(enodenum == 2) {
	printf("Offset = %lld\n",offset);
	printf("Beginning of enodechunk offset = %lld\n",readoffset);
  }
	
  SDBG("Seeking to %d\n",readoffset);
  if(readoffset != lseek(fd,readoffset,SEEK_SET)) {
	perror("cant seek to enode chunk offset");
	exit(-1);
  }
  
  /* 1) need to read out the full sector of enodes (ie. a chunk) */
  if(esb.fs_bps != read(fd,ebuf,esb.fs_bps)) {
	perror("Can't read enode chunk");
	exit(-1);
  }

  /* 2) find index into this group of enode to modify */
  int offinchunk = enodenum % (esb.fs_bps / sizeof(struct denode));
  SDBG("Offset into chunk = %d\n",offinchunk);
  dp = (struct denode*)ebuf;  
  dp += offinchunk;

  /* just replace the enode... */
  memcpy(dp,de,sizeof(struct denode));

	deprint(dp);    
  
  
  /* 3)	write enode "chunk" back to disk	 */
  if(readoffset != lseek(fd,readoffset,SEEK_SET)) {
	perror("cant seek to enode chunk offset");
	exit(-1);
  }  
  
  if(esb.fs_bps != write(fd,ebuf,esb.fs_bps)) {
	perror("error writing enode to disk");
	exit(-1);
  }

  
}
								   

/* get the offset of a block # on the disk */
off_t blockoff(int blocknum) {
  struct cg *ap = allcg;
  off_t offset;

  /* get the cylinder group */
  int cg = blocknum / esb.fs_bpg;  
  ap += cg;
  
  /* i dont think this is correct...*/
  offset = ap->cg_dboff + (esb.fs_bps * (blocknum % esb.fs_bpg)) ;
  printf("Block # %d found in cg %d at %lld\n",blocknum,cg,offset);
  return offset;
}


off_t enodeoff(int enodenum) {
  //numenodes;
  //ncg->cg_neblk;
  struct cg *ap = allcg;
  off_t offset;

  int cg = enodenum / esb.fs_epg;
  ap += cg;
  offset = ap->cg_enodeoff + (sizeof(struct denode) * (enodenum % esb.fs_epg));
  return offset;
}


void deprint(struct denode *dp) {
  printf("-->DENODE<<-\n");
  printf("mode %d ",dp->de_mode);	           
    printf("nlink %d ",dp->de_nlink);	       
  printf("size %d ",dp->de_size);	           
  printf("atime %d ",dp->de_atime);	       
  /*printf("atimensec %d\n",dp->de_atimensec);   */
  printf("mtime %d ",dp->de_mtime);	       
  /*printf("mtimensec %d\n",dp->de_mtimensec);   */
  printf("ctime %d \n",dp->de_ctime);	       
  /*printf("ctimensec %d\n",dp->de_ctimensec);   */
  printf("direct block[0] %d ",dp->de_db[0]/esb.fs_bps); 
  printf("direct block[1] %d ",dp->de_db[1]/esb.fs_bps); 
  /* printf("status %d\n",dp->de_flags);	       */
  printf("blocks %d ",dp->de_blocks);	       
  printf("gen %d ",dp->de_gen);		       
  printf("owner %d ",dp->de_uid);   		   
	printf("group %d ",dp->de_gid);		       
  printf("enode # %d ",dp->de_spare[0]);	   
  printf("\n-->END DENODE<<-\n");
}
