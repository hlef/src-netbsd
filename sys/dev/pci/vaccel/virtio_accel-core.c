#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/callout.h>
#include <sys/rndsource.h>
#include <sys/mutex.h>
#include <dev/pci/virtioreg.h>
#include <dev/pci/virtiovar.h>

#include "virtio_accel.h"

struct vaccel_softc {
	device_t		sc_dev;
	struct virtio_softc	*sc_virtio;
	struct virtqueue	sc_vq;

	kmutex_t		sc_mutex;
	bool			sc_active;

	void			*sc_buf;
	bus_dmamap_t		sc_dmamap;
};

static int	vaccel_match(device_t, cfdata_t, void *);
static void	vaccel_attach(device_t, device_t, void *);
//static int	vaccel_detach(device_t, int);

CFATTACH_DECL_NEW(vaccel, sizeof(struct vaccel_softc),
		  vaccel_match, vaccel_attach, NULL, NULL);

static void vaccel_attach(device_t parent, device_t self, void *aux)
{
	printf("hello from attach\n");
}

static int vaccel_match(device_t parent, cfdata_t match, void *aux)
{
	struct virtio_attach_args *va = aux;

	printf("virtaccel devid = %d vaccel_id = %d\n", va->sc_childdevid, VIRTIO_ID_ACCEL); 
	if (va->sc_childdevid == VIRTIO_ID_ACCEL) {
		printf("kanw match\n");
		return 1;
	}

	return 0;
}

//static int vaccel_detach(device_t self, int flags)
//{
//	printf("hello from detach\n");
//	return 0;
//}

