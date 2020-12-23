#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$Comso: comso_component.c,v 0.1 2018/06/01$");

#include <sys/param.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/stat.h>

#include <rump-sys/kern.h>
#include <rump-sys/dev.h>
#include <rump-sys/vfs.h>

#include "ioconf.h"

RUMP_COMPONENT(RUMP_COMPONENT_DEV)
{
	printf("vaccelvaccelvaccel\n");
	extern const struct cdevsw accel_cdevsw;
	devmajor_t bmaj, cmaj;
	int error;

	cmaj = cdevsw_lookup_major(&accel_cdevsw);

	if ((error = devsw_attach("accel", NULL, &bmaj,
					&accel_cdevsw, &cmaj)) != 0)
		panic("cannot attach accel: %d", error);

	if ((error = rump_vfs_makeonedevnode(S_IFCHR, "/dev/accel",
	    cmaj, 0)) != 0)
		panic("cannot create /dev/accel: %d", error);

	rump_pdev_add(accelattach, 4);
}

