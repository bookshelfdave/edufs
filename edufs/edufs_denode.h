/*
 * Copyright (c) 2003 David Parfitt
 * All rights reserved.
 * 
 * Copyright (c) 2002 Networks Associates Technology, Inc.
 * All rights reserved.
 *
 * This software was developed for the FreeBSD Project by Marshall
 * Kirk McKusick and Network Associates Laboratories, the Security
 * Research Division of Network Associates, Inc. under DARPA/SPAWAR
 * contract N66001-01-C-8035 ("CBOSS"), as part of the DARPA CHATS
 * research program
 *
 * Copyright (c) 1982, 1989, 1993
 *	The Regents of the University of California.  All rights reserved.
 * (c) UNIX System Laboratories, Inc.
 * All or some portions of this file are derived from material licensed
 * to the University of California by American Telephone and Telegraph
 * Co. or Unix System Laboratories, Inc. and are reproduced herein with
 * the permission of UNIX System Laboratories, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The names of the authors may not be used to endorse or promote
 *    products derived from this software without specific prior written
 *    permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * Just a modified/simplified version of dinode.h 
 */

#ifndef _EDUFS_DENODE_H_
#define	_EDUFS_DENODE_H_


/* File permissions. */
#define	DIEXEC		0000100		/* Executable. */
#define	DIWRITE		0000200		/* Writeable. */
#define	DIREAD		0000400		/* Readable. */
#define	DISVTX		0001000		/* Sticky bit. */
#define	DISGID		0002000		/* Set-gid. */
#define	DISUID		0004000		/* Set-uid. */

/* File types. */
#define	DIFMT		0170000		/* Mask of file type. */
#define	DIFIFO		0010000		/* Named pipe (fifo). */
#define	DIFCHR		0020000		/* Character device. */
#define	DIFDIR		0040000		/* Directory file. */
#define	DIFBLK		0060000		/* Block device. */
#define	DIFREG		0100000		/* Regular file. */
#define	DIFLNK		0120000		/* Symbolic link. */
#define	DIFSOCK		0140000		/* UNIX domain socket. */
#define	DIFWHT		0160000		/* Whiteout. */


/*
 * The size of physical and logical block numbers and time fields in EDUFS.
 */
typedef	int32_t	edufs_daddr_t;

#define	NXADDR	2			/* External addresses in inode. */
#define	NDADDR	12			/* Direct addresses in inode. */
#define	NIADDR	3			/* Indirect addresses in inode. */


struct denode {
  u_int16_t	    de_mode;	    /* 0: IFMT, permissions; see below. */
  int16_t		de_nlink;	    /* 2: File link count. */
  u_int64_t	    de_size;	    /* : File byte count. */
  uint32_t		de_atime;	    /* : Last access time. */
  uint32_t		de_atimensec;	/* : Last access time. */
  uint32_t		de_mtime;	    /* : Last modified time. */
  uint32_t		de_mtimensec;	/* : Last modified time. */
  uint32_t		de_ctime;	    /* : Last inode change time. */
  uint32_t		de_ctimensec;	/* : Last inode change time. */
  edufs_daddr_t	de_db[NDADDR];	/* : Direct disk blocks. */
  edufs_daddr_t	de_ib[NIADDR];	/* : Indirect disk blocks. */
  u_int32_t	    de_flags;	    /* : Status flags (chflags). */
  int32_t		de_blocks;	    /* : Blocks actually held. */
  int32_t		de_gen;		    /* : Generation number. */
  u_int32_t	    de_uid;		    /* : File owner. */
  u_int32_t	    de_gid;		    /* : File group. */
  int32_t		de_spare[3];	/* : Reserved; currently unused */
};

#endif /* _EDUFS_DENODE_H_ */
