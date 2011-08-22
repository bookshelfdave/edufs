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

#ifndef _EDUFS_FS_H
#define _EDUFS_FS_H



#define EDUFS_VERSION 1

/* number of direct blocks kept inside of the inode */
#define DIRECTBLOCKS 12

/* goes up to triple indirect blocks */
#define NUMOFINDIRECTS 3

/* each block has 4096 bytes - default for now */
#define BLOCKSIZE 4096

/* frags are not supported */
#define MAXFRAG    8

/*
 * MINBSIZE is the smallest allowable block size.
 * In order to insure that it is possible to create files of size
 * 2^32 with only two levels of indirection, MINBSIZE is set to 4096.
 * MINBSIZE must be big enough to hold a cylinder group block,
 * thus changes to (struct cg) must keep its size within MINBSIZE.
 * Note that super blocks are always of size SBSIZE,
 * and that both SBSIZE and MAXBSIZE must be >= MINBSIZE.
 */
#define MINBSIZE        4096

/*
 * The path name on which the filesystem is mounted is maintained
 * in fs_fsmnt. MAXMNTLEN defines the amount of space allocated in
 * the super block for this name.
 */
#define MAXMNTLEN       468

/*
 * The volume name for this filesystem is maintained in fs_volname.
 * MAXVOLLEN defines the length of the buffer allocated.
 */
#define MAXVOLLEN       32

/*
 * There is a 128-byte region in the superblock reserved for in-core
 * pointers to summary information. Originally this included an array
 * of pointers to blocks of struct csum; now there are just a few
 * pointers and the remaining space is padded with fs_ocsp[].
 *
 * NOCSPTRS determines the size of this padding. One pointer (fs_csp)
 * is taken away to point to a contiguous array of struct csum for
 * all cylinder groups; a second (fs_maxcluster) points to an array
 * of cluster sizes that is computed as cylinder groups are inspected,
 * and the third points to an array that tracks the creation of new
 * directories. A fourth pointer, fs_active, is used when creating
 * snapshots; it points to a bitmap of cylinder groups for which the
 * free-block bitmap has changed since the snapshot operation began.
 */
#define NOCSPTRS        ((128 / sizeof(void *)) - 4)



typedef int64_t edufs_time_t;

/*
 * Per cylinder group information; summarized in blocks allocated
 * from first cylinder group data blocks.  These blocks have to be
 * read in from fs_csaddr (size fs_cssize) in addition to the
 * super block.
 */
struct ecsum {
  int32_t	cs_ndir;		/* number of directories */
  int32_t	cs_nbfree;		/* number of free blocks */
  int32_t	cs_nefree;		/* number of free inodes */
  /*int32_t	cs_nffree;*/		/* number of free frags */
};


/* superblock totals */
struct ecsum_total {
  int64_t	cs_ndir;		/* number of directories */
  int64_t	cs_nbfree;		/* number of free blocks */
  int64_t	cs_nefree;		/* number of free enodes */
  int64_t	cs_spare[3];		/* future expansion */
};


/* the actual superblock */
struct edufs_superblock {
  int32_t	 fs_firstfield;		/* historic filesystem linked list, */
  int32_t	 fs_unused_1;		/*     used for incore super blocks */
  int32_t	 fs_sblkno;		/* offset of super-block in filesys */
  int32_t	 fs_cblkno;		/* offset of cyl-block in filesys */
  int32_t	 fs_iblkno;		/* offset of inode-blocks in filesys */
  int32_t	 fs_dblkno;		/* offset of first data after cg */
  
  int32_t	 fs_bsize;		/* size of basic blocks in fs */
  int32_t    fs_bps;        /* bytes per sector */
  int32_t	 fs_nsect;		/* sectors per track */
  int32_t    fs_spc;		/* sectors per cylinder */
  int32_t	 fs_ncyl;		/* cylinders in filesystem */
  int32_t	 fs_cpg;		/* cylinders per group */
  int32_t	 fs_epg;		/* enodes per group */
  int32_t	 fs_bpg;		/* blocks per group */
  
  
  int32_t	 fs_size;		/* number of blocks in fs */
  int32_t	 fs_dsize;		/* number of data blocks in fs */
  int32_t	 fs_ncg;		/* number of cylinder groups */
  
  int32_t	 fs_fsize;		/* size of frag blocks in fs */
  int32_t	 fs_frag;		/* number of frags in a block in fs */
  /*int32_t	 fs_npsect;	*/	/* # sectors/track including spares */
  
  int32_t	 fs_sbsize;		/* actual size of super block */  
  
  int32_t	 fs__cgoffset;	/* cylinder group offset in cylinder */
  /*int32_t  fs_time;*/		/* last time written */
  int32_t	 fs_interleave;	/* hardware sector interleave */
  
  u_char	 fs_fsmnt[MAXMNTLEN];	/* name mounted on */
  u_char	 fs_volname[MAXVOLLEN];	/* volume name */
  
  
  int32_t	 fs_nindir;		/* value of NINDIR */
  
  int32_t	 fs_csaddr;		/* blk addr of cyl grp summary area */
  int32_t	 fs_cssize;		/* size of cyl grp summary area */
  int32_t	 fs_cgsize;		/* cylinder group size */
  
  
  /* these fields are cleared at mount time */
  int8_t   fs_fmod;		/* super block modified flag */
  int8_t   fs_clean;		/* filesystem is clean flag */
  int8_t 	 fs_ronly;		/* mounted read-only flag */
  
  
  
  
  int32_t	 fs_trackskew;	/* sector 0 skew, per track */
  int32_t	 fs_id[2];		/* unique filesystem id */
  
  /* sizes determined by number of cylinder groups and their sizes */  
  u_int64_t fs_swuid;		/* system-wide uid */
  int32_t  fs_pad;		/* due to alignment of fs_swuid */
  
  /* these fields retain the current block allocation info */
  int32_t	 fs_cgrotor;		/* last cg searched */
  void 	*fs_ocsp[NOCSPTRS];	/* padding; was list of fs_cs buffers */
  u_int8_t *fs_contigdirs;	/* # of contiguously allocated dirs */
  
  int64_t	 fs_sblockloc;		/* byte offset of standard superblock */
  struct	ecsum_total fs_cstotal; 	/* cylinder summary information */
  edufs_time_t fs_time;		/* last time written */
  
  /*ufs2_daddr_t fs_csaddr;*/		/* blk addr of cyl grp summary area */
  int32_t	 fs_maxsymlinklen;	/* max length of an internal symlink */
  
  u_int64_t fs_maxfilesize;	/* maximum representable file size */
  int32_t	 fs_magic;		/* magic number */
};

/* edufs cylinder group */
struct cg {
  int32_t	 cg_next;		    /* historic cyl groups linked list */
  int32_t	 cg_magic;		    /* magic number */
  int32_t	 cg_cgx;		    /* we are the cgx'th cylinder group */
  int16_t	 cg_ncyl;		    /* number of cyl's this cg */
  int32_t	 cg_ndblk;		    /* number of data blocks this cg */
  struct	ecsum cg_cs;		/* cylinder summary information */
  /* doesnt support frags... */
  /*int32_t	 cg_frsum[MAXFRAG];*/	/* counts of available frags */
  int32_t	 cg_old_btotoff;	/* (int32) block totals per cylinder */
  int32_t	 cg_dboff;		    /* first data block offset */

  int32_t	 cg_eusedoff;		/* (u_int8) used inode map */
  int32_t	 cg_freeoff;		/* (u_int8) free block map */
  int32_t    cg_enodeoff;       /* denodes */
  
  int32_t	 cg_rotor;		    /* position of last used block */
  int32_t	 cg_frotor;		    /* position of last used frag */
  int32_t	 cg_irotor;		    /* position of last used inode */
  int32_t	 cg_nextfreeoff;	/* (u_int8) next available space */
  int32_t	 cg_clustersumoff;	/* (u_int32) counts of avail clusters */
  int32_t	 cg_clusteroff;		/* (u_int8) free cluster map */
  int32_t	 cg_nclusterblks;	/* number of clusters this cg */
  int32_t    cg_neblk;		    /* number of enode blocks this cg */
  int32_t	 cg_initediblk;		/* last initialized inode */
  int32_t	 cg_sparecon32[3];	/* reserved for future use */
  edufs_time_t cg_time;		    /* time last written */
  int32_t    spacex;		    /* add this to the other padding */
  int64_t	 cg_sparecon64[3];	/* reserved for future use */
  u_int8_t cg_space[1];		    /* space for cylinder group maps */
};

#endif
