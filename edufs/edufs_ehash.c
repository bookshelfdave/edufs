/*
 * Copyright (c) 1982, 1986, 1989, 1991, 1993, 1995
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * Modified by Dave Parfitt - this came from the UFS implementation
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/lock.h>
#include <sys/vnode.h>
#include <sys/malloc.h>
#include <sys/proc.h>
#include <sys/mutex.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/queue.h>
#include <fs/edufs/edufs_enode.h>


static MALLOC_DEFINE(M_EDUFSEHASH, "EDUFS ehash", "EDUFS Enode hashtables");

/*
 * Structures associated with enode cacheing.
 */
static LIST_HEAD(ehashhead, enode) *ehashtbl;
static u_long	ehash;		/* size of hash table - 1 */
/* didnt want to rename inum to enum because enum is reserved... duh */
#define	ENOHASH(device, inum)	(&ehashtbl[(minor(device) + (inum)) & ehash])
static struct mtx edufs_ehash_mtx;



/*
 * Initialize enode hash table.
 */
void
edufs_ehashinit()
{

	ehashtbl = hashinit(desiredvnodes, M_EDUFSEHASH, &ehash);
	mtx_init(&edufs_ehash_mtx, "ufs ehash", NULL, MTX_DEF);
}

/*
 * Destroy the enode hash table.
 */
void
edufs_ehashuninit()
{

	hashdestroy(ehashtbl, M_EDUFSEHASH, ehash);
	mtx_destroy(&edufs_ehash_mtx);
}

/*
 * Use the device/inum pair to find the incore enode, and return a pointer
 * to it. If it is in core, return it, even if it is locked.
 */
struct vnode *
edufs_ehashlookup(dev, inum)
	dev_t dev;
	ino_t inum;
{
	struct enode *ep;

	mtx_lock(&edufs_ehash_mtx);
	uprintf("LOOKUP ENOHASH = %lu\n",(minor(dev) + (inum)) & ehash);
	LIST_FOREACH(ep, ENOHASH(dev, inum), e_hash)
		if (inum == ep->e_number && dev == ep->e_dev)
			break;
	mtx_unlock(&edufs_ehash_mtx);

	if (ep)
		return (ETOV(ep));
	return (NULLVP);
}


/*
 * dump the enode list
 */
void
edufs_ehashdump(dev)
	dev_t dev;
{  
  struct enode *ep;
  struct ehashhead *headp = ehashtbl;
  
  uprintf("Dumping enode list\n");
  mtx_lock(&edufs_ehash_mtx);
  LIST_FOREACH(ep,headp,e_hash) {
	  uprintf("--->ENODE [%d]\n",ep->e_number);
  }				 
  mtx_unlock(&edufs_ehash_mtx);
  return;
}

/*
 * Use the device/inum pair to find the incore enode, and return a pointer
 * to it. If it is in core, but locked, wait for it.
 */
int
edufs_ehashget(dev, inum, flags, vpp)
	dev_t dev;
	ino_t inum;
	int flags;
	struct vnode **vpp;
{
	struct thread *td = curthread;	/* XXX */
	struct enode *ep;
	struct vnode *vp;
	int error;

	*vpp = NULL;
loop:
	uprintf("Inside ehashget\n");
	uprintf("HASHGET ENOHASH = %lu\n",(minor(dev) + (inum)) & ehash);
	mtx_lock(&edufs_ehash_mtx);
	LIST_FOREACH(ep, ENOHASH(dev, inum), e_hash) {
	  uprintf(".");
	  if (inum == ep->e_number && dev == ep->e_dev) {
			vp = ETOV(ep);
			/*mtx_lock(&vp->v_interlock);*/
			VI_LOCK(vp);
			mtx_unlock(&edufs_ehash_mtx);
			error = vget(vp, flags | LK_INTERLOCK, td);
			uprintf("got the enode");
			if (error == ENOENT)
			  goto loop;
			if (error) {

			  return (error);
			}
			*vpp = vp;
			return (0);
	  }
	}
	mtx_unlock(&edufs_ehash_mtx);

	return (0);
}

/*
 * Check hash for duplicate of passed enode, and add if there is no one.
 * if there is a duplicate, vget() it and return to the caller.
 */
int
edufs_ehashins(ep, flags, ovpp)
	struct enode *ep;
	int flags;
	struct vnode **ovpp;
{
	struct thread *td = curthread;		/* XXX */
	struct ehashhead *epp;
	struct enode *oep;
	struct vnode *ovp;
	int error;

	
loop:
	uprintf("EDUFS_HASHINS\n");
	mtx_lock(&edufs_ehash_mtx);
	epp = ENOHASH(ep->e_dev, ep->e_number);
	uprintf("ENOHASH = %lu\n",(minor(ep->e_dev) + (ep->e_number)) & ehash);
	
	LIST_FOREACH(oep, epp, e_hash) {

	  if (ep->e_number == oep->e_number && ep->e_dev == oep->e_dev) {
			ovp = ETOV(oep);

			mtx_lock(&ovp->v_interlock);
			mtx_unlock(&edufs_ehash_mtx);
			uprintf("hashins flags=[%d]",flags);
			error = vget(ovp, flags | LK_INTERLOCK, td);
			if (error == ENOENT) {
			  goto loop;
			}
			if (error) {
			  return (error);
			}
			*ovpp = ovp;
			return (0);
		}
	}

	uprintf("NEW ENODE INFO #[%d]\n",ep->e_number);
	LIST_INSERT_HEAD(epp, ep, e_hash);
	ep->e_flag |= EN_HASHED;
	mtx_unlock(&edufs_ehash_mtx);

	*ovpp = NULL;
	return (0);
}

/*
 * Remove the enode from the hash table.
 */
void
edufs_ehashrem(ep)
	struct enode *ep;
{
	mtx_lock(&edufs_ehash_mtx);
	if (ep->e_flag & EN_HASHED) {
		ep->e_flag &= ~EN_HASHED;
		LIST_REMOVE(ep, e_hash);
	}
	mtx_unlock(&edufs_ehash_mtx);
}
