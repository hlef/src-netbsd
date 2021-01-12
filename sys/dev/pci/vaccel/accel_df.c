/*-
 * A simple character device driver that implements a socket communication
 *
 */

#include <sys/proc.h>
#include <sys/conf.h>

#include <sys/types.h>
#include <sys/filedesc.h>
#include <sys/kmem.h>

#include "accel.h"
#include "virtio_accel-common.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/queue.h>
#include <sys/proc.h>
#include <sys/buf.h>
#include <sys/bufq.h>
#include <sys/endian.h>
#include <sys/disklabel.h>
#include <sys/disk.h>
#include <sys/dkio.h>
#include <sys/stat.h>
#include <sys/conf.h>
#include <sys/fcntl.h>
#include <sys/vnode.h>
#include <sys/syslog.h>
#include <sys/mutex.h>
#include <sys/module.h>
#include <sys/reboot.h>

#include <dev/ldvar.h>

#include "ioconf.h"

dev_type_open(acceloopen);
dev_type_close(cccelomsoclose);
dev_type_write(accelwrite);
dev_type_read(accelread);

/* Autoconfiguration glue */
void accelattach(int num);
int accelopen(dev_t device, int flags, int fmt, struct lwp *process);
int accelread(dev_t device, struct uio *uio, int flags);
int accelwrite(dev_t device, struct uio *uio, int flags);
int accelclose(dev_t device, int flags, int fmt, struct lwp *process);
int accelioctl(dev_t device, u_long command, void *data, int flags,
	      struct lwp *process);

/* just define the character device handlers because that is all we need */
const struct cdevsw accel_cdevsw = {
        .d_open = accelopen,
	.d_close = accelclose,
	.d_read = accelread,
	.d_write = accelwrite,
        .d_ioctl = accelioctl,

	.d_stop = nostop,
	.d_tty = notty,
	.d_poll = nopoll,
	.d_mmap = nommap,
	.d_kqfilter = nokqfilter,
	.d_discard = nodiscard,
	.d_flag = D_OTHER
};

struct accel_softc {
	device_t                sc_dev;
	int			hello;
};

/*
 * Attach for autoconfig to find.  The parameter is the number given in
 * the configuration file, and defaults to 1.  New code should not expect
 * static configuration from the kernel config file, and thus ignore that
 * parameter.
 */
void accelattach(int num)
{
	printf("Hello from accelattach\n");
	return;
	  /* nothing to do for accel, this is where resources that
	     need to be allocated/initialised before open is called
	     can be set up */
}

/*
 * Handle an open request on the device.
 */
int accelopen(dev_t device, int flags, int fmt, struct lwp *process)
{
	int ret = 0;

	int unit = DISKUNIT(device);
	struct accel_softc *sc = device_lookup_private(&accel_cd, unit);
	//struct virtio_accel *vaccel = virtaccel_devmgr_get_first();
	//struct virtio_accel *vaccel = NULL;

	//if (!vaccel)
	//	return -ENODEV;


	printf("Accel opened\n");
	return ret;
}

/*
 * Handle the close request for the device.
 */
int accelclose(dev_t device, int flags, int fmt, struct lwp *process)
{
	printf("Accel closed\n");
	return 0; /*this always succeeds */
}

/*
 * Handle the read for the device
 */
int accelread(dev_t device, struct uio *uio, int flags)
{
	int ret = 0;

	printf("Accel read\n");
	return ret;
}

/*
 * Handle the write for the device
 */
int accelwrite(dev_t device, struct uio *uio, int flags)
{
	int ret = 0;
	printf("Accel write\n");
	return ret;
}


/*
 * Handle the ioctl for the device
 */
int accelioctl(dev_t device, u_long command, void *data, int flags,
	      struct lwp *process)
{
	struct virtio_accel_req *req;
	//struct accel_session *sess = NULL;
	//struct accel_op *op = NULL;
	int ret = 0;

	printf("Accel ioctl\n");

	switch (command) {
	case ACCIOC_CRYPTO_SESS_CREATE:
	case ACCIOC_CRYPTO_SESS_DESTROY:
		printf("prwto\n");
		break;
	case ACCIOC_CRYPTO_ENCRYPT:
	case ACCIOC_CRYPTO_DECRYPT:
		printf("deutero\n");
		break;
	case ACCIOC_GEN_SESS_CREATE:
	case ACCIOC_GEN_SESS_DESTROY:
		printf("trito\n");
		req = kmem_zalloc(sizeof(struct virtio_accel_req), KM_SLEEP);
		if (!req)
			goto err_mem;
		//sess = kmem_zalloc(sizeof(struct accel_session), KM_SLEEP);
		//if (!sess)
		//	goto err_mem_req;
		//sess = (struct accel_session *) data;
		break;
	case ACCIOC_GEN_DO_OP:
		printf("tetarto\n");
		break;
	default:
		printf("Invalid IOCTL\n\n");
		ret = -EFAULT;
	}

	return ret;

//err_mem_req:
	kmem_free(req, sizeof(struct virtio_accel_req));
err_mem:
	return -ENOMEM;

}
