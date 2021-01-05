#	$NetBSD: Makefile,v 1.4 2015/11/16 23:27:08 pooka Exp $
#

RUMPTOP=${TOPRUMP}

.PATH:	${RUMPTOP}/../dev/pci/vaccel

LIB=	rumpdev_virtio_vaccel
COMMENT=virtio vAccel driver

IOCONF=	VIRTIO_VACCEL.ioconf
RUMP_COMPONENT=ioconf

SRCS+=	virtio_accel-core.c

CPPFLAGS+= -I${RUMPTOP}/librump/rumpkern
   
.include "${RUMPTOP}/Makefile.rump"
.include <bsd.lib.mk>
.include <bsd.klinks.mk>
