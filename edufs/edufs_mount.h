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

#ifndef _EDUFS_EDUFSMOUNT_H_
#define	_EDUFS_EDUFSMOUNT_H_

/* arguments to mount */
/* this needs a bit of work */
struct edufs_args {
  uid_t	  uid;		/* uid that owns edufs files */
  gid_t	  gid;		/* gid that owns edufs files */
  mode_t  mask;		/* mask to be applied for edufs perms */
  int	  flags;	/* flags - not used yet... */
  int     magic;	/* version number */
  char    *fspec;    /* where to mount */
};


/* the kernel mount structure */
struct edufsmount {
  struct	mount *e_mountp;                    /* filesystem vfs structure */
  dev_t	    e_dev;				                /* device mounted */
  struct	vnode *e_devvp;		                /* block device mounted vnode */
  u_long	e_fstype;			                /* type of filesystem */
  struct	edufs_superblock *e_esb;			/* pointer to superblock */
  struct    cg *cglist;
};

/* this macro converts the data stored in the struct mount to a struct edufsmount */
#define VFSTOEDUFS(mp)                  ((struct edufsmount*)mp->mnt_data)

#endif
