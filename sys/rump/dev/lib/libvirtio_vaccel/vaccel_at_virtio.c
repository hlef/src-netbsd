#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ld_at_virtio.c,v 1.4 2017/05/10 06:22:15 sevan Exp $");

#include <sys/param.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/bus.h>
#include <sys/stat.h>
#include <sys/disklabel.h>

#include <rump-sys/kern.h>
#include <rump-sys/vfs.h>

#include "ioconf.c"

RUMP_COMPONENT(RUMP_COMPONENT_DEV)
{

	config_init_component(cfdriver_ioconf_vaccel_virtio,
	    cfattach_ioconf_vaccel_virtio, cfdata_ioconf_vaccel_virtio);
}

/*
 * Pseudo-devfs.  Since creating device nodes is non-free, don't
 * speculatively create hundreds of them (= milliseconds slower
 * bootstrap).  Instead, after the probe is done, see which units
 * were found and create nodes only for them.
 */
RUMP_COMPONENT(RUMP_COMPONENT_POSTINIT)
{
	extern const struct cdevsw vaccel_cdevsw;
	devmajor_t bmaj = -1, cmaj = -1;
	int error;

	if ((error = devsw_attach("vaccel", NULL, &bmaj,
	    &vaccel_cdevsw, &cmaj)) != 0)
		panic("cannot attach vaccel: %d", error);
        
	if ((error = rump_vfs_makeonedevnode(S_IFCHR, "/dev/vaccel", 
	    cmaj, 5)) != 0)
		panic("cannot create raw ld dev nodes: %d", error);
}
