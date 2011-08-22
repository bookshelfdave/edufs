PROG=	newfs_edufs
MAN=	newfs_edufs.8

WARNS=	0
OPTS= -Wmissing-declarations -Wall -Wunused
SRCS= newfs_edufs.c newfs_utils.c
.include <bsd.prog.mk>

# using this to custom compile 
#debugbin:
#	cc -g -pipe -mcpu=pentiumpro    -c newfs_edufs.c
#	cc -g -pipe -mcpu=pentiumpro     -o newfs_edufs newfs_edufs.o
#	gcc -c newfs_utils.c -o utils.o
#	gcc utils.o newfs_edufs.c  -o newfs_edufs
