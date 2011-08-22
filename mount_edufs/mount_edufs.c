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
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <ctype.h>
#include <err.h>
#include <grp.h>
#include <locale.h>
#include <pwd.h>
#include <stdio.h>
/* must be after stdio to declare fparseln */
#include <libutil.h>
#include <stdlib.h>
#include <string.h>
#include <sysexits.h>
#include <unistd.h>


#include "../sys/fs/edufs/edufs_mount.h"

int
main(int argc, char *argv[])
{           
  struct edufs_args ea;
  char mntpath[MAXPATHLEN];
  char *device;
  char *dir;
	
  
  /* device, then dir */
  /* mount_edufs /dev/ad0s2d /scratch */
  if(argc < 3) {
	printf("Usage: mount_edufs device dir\n");
	exit(1);
  }

  device = argv[1];
  dir    = argv[2];
  
  
  /*(void)checkpath(dir, mntpath);*/
  /*(void)rmslashes(device, device);*/
  
  bzero(&ea,sizeof(ea));
  ea.fspec = malloc(strlen(device));
  strlcpy(ea.fspec,device,MAXPATHLEN);

  strlcpy((char*)&mntpath,dir,MAXPATHLEN);
  
  
  ea.uid = getuid();
  ea.gid = getgid();
  ea.flags = 0;
  ea.magic = 9250; /* remove this */
  
  if (mount("edufs", mntpath, 0/*MNT_RDONLY*/, &ea) < 0)
	perror("Foo");

  exit (0);
}

