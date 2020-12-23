/*-
 * A simple character device driver that implements a socket communication
 *
 */

#include <sys/proc.h>
#include <sys/conf.h>

#include <sys/types.h>
#include <sys/filedesc.h>
#include <sys/malloc.h>

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
	int error;

	error = 0;
	printf("Accel ioctl\n");

	return (error);
}
