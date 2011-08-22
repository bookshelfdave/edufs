PROG=	mount_edufs
SRCS=	mount_edufs.c
DPADD=	${LIBUTIL}
LDADD=	-lutil

MOUNT=	${.CURDIR}/../mount
CFLAGS+= -I${MOUNT}
WARNS=	0

.PATH:	${MOUNT}
.include <bsd.prog.mk>
