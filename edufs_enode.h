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

#ifndef _EDUFS_ENODE_H_
#define	_EDUFS_ENODE_H_

#include <sys/lock.h>
#include <sys/queue.h>


#define doff_t long

/* most of this code if from the struct inode definition */
struct enode {
  LIST_ENTRY(enode) e_hash;            /* Hash chain. */
  struct	 vnode  *e_vnode;          /* Vnode associated with this inode. */
  struct	 edufsmount *e_emp;        /* Ufsmount point associated with this inode. */
  struct	 vnode  *e_devvp;          /* Vnode for block I/O. */
  u_int32_t  e_flag;	               /* flags, see below */
  struct     cdev *e_dev;	           /* Device associated with the inode. */
  ino_t	     e_number;	               /* The identity of the inode. */
  int	     e_effnlink;	           /* i_nlink when I/O completes */  
  struct	 edufs_superblock *e_fs;   /* Associated filesystem superblock. */
  struct	 lockf *e_lockf;           /* Head of byte-level lock list. */

  /*
   * Side effects; used during directory lookup.
   */  
  int32_t	 e_count;	               /* Size of free slot in directory. */
  doff_t	 e_endoff;	               /* End of useful stuff in directory. */

  int32_t	 e_freebn;	               /* logical block number of found dir entry */
  doff_t	 e_freediroff;	           /* offset withing freebn of free dir entry */

  
  ino_t	     e_ino;	                   /* Inode number of found directory. */
  u_int32_t  e_reclen;	               /* Size of found directory entry. */

  /* unused at the moment */
  struct dirhash *dirhash; /* Hashing for large directories. */

  /*
   * Copies from the on-disk dinode itself.
   */
  u_int16_t e_mode;	                   /* IFMT, permissions; see below. */
  int16_t	e_nlink;	               /* File link count. */
  u_int64_t e_size;	                   /* File byte count. */
  u_int32_t e_flags;	               /* Status flags (chflags). */
  int64_t	e_gen;	                   /* Generation number. */
  u_int32_t e_uid;	                   /* File owner. */
  u_int32_t e_gid;	                   /* File group. */

  struct denode *den;
};

/* renamed these for edufs so they were easier to find in the headers */
#define	EN_ACCESS	0x0001		/* Access time update request. */
#define	EN_CHANGE	0x0002		/* Inode change time update request. */
#define	EN_UPDATE	0x0004		/* Modification time update request. */
#define	EN_MODIFIED	0x0008		/* Inode has been modified. */
#define	EN_RENAME	0x0010		/* Inode is being renamed. */
#define	EN_HASHED	0x0020		/* Inode is on hash list */
#define	EN_LAZYMOD	0x0040		/* Modified, but don't write yet. */
#define	EN_SPACECOUNTED	0x0080		/* Blocks to be freed in free count. */

void edufs_ehashinit(void);
void edufs_ehashuninit(void);
struct vnode *edufs_ehashlookup(dev_t dev, ino_t inum);
int edufs_ehashget(dev_t dev, ino_t inum, int flags, struct vnode **vpp);
int edufs_ehashins(struct enode *ep, int flags, struct vnode **ovpp);
void edufs_ehashrem(struct enode *ep);
void edufs_ehashdump(dev_t dev);
/*int edufs_update(struct vnode *vp, int waitfor);*/

#ifdef _KERNEL

/* Convert between inode pointers and vnode pointers. */
#define VTOE(vp)	((struct enode *)(vp)->v_data)
#define ETOV(ep)	((ep)->e_vnode)


#endif /* _KERNEL */

#endif /* !_EDUFS_ENODE_H_ */
