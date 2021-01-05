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
#include "virtio_accel-common.h"

struct vaccel_softc {
	device_t		sc_dev;
	struct virtio_softc	*sc_virtio;
	struct virtqueue	sc_vq;
	struct virtio_accel	vaccel;

	kmutex_t		sc_mutex;
	bool			sc_active;

	void			*sc_buf;
	bus_dmamap_t		sc_dmamap;
};

static int	vaccel_match(device_t, cfdata_t, void *);
static void	vaccel_attach(device_t, device_t, void *);
//static int	vaccel_detach(device_t, int);
static int      vaccel_config_change(struct virtio_softc *);
static int	virtaccel_update_status(struct virtio_softc *);
//static int	virtaccel_update_status(struct vaccel_softc *sc,
//				   struct virtio_softc *vsc);
int vaccel_vq_done(struct virtqueue *vq);


CFATTACH_DECL_NEW(vaccel, sizeof(struct vaccel_softc),
		  vaccel_match, vaccel_attach, NULL, NULL);

static void vaccel_attach(device_t parent, device_t self, void *aux)
{
	//struct virtio_accel *vaccel;
	struct vaccel_softc *sc = device_private(self);
	struct virtio_softc *vsc = device_private(parent);
	//int nsegs;
	int error;
	//uint32_t features;

	printf("hello from attach\n");

	if (virtio_child(vsc) != NULL)
		panic("already attached to something else");
	
	sc->sc_dev = self;
	sc->sc_virtio = vsc;

	virtio_child_attach_start(vsc, self, IPL_NET, &sc->sc_vq,
				  vaccel_config_change, virtio_vq_intr, 0,
				  0, VIRTIO_COMMON_FLAG_BITS);

	//features = virtio_features(vsc);
	
	error = virtio_alloc_vq(vsc, &sc->sc_vq, 0, 1024, 1, "vaccel vq");
	if (error) {
		aprint_error_dev(sc->sc_dev, "can't alloc virtqueue: %d\n",
				error);
		goto failed;
	}

	sc->sc_vq.vq_done = vaccel_vq_done;

	virtaccel_update_status(vsc);

	if (virtio_child_attach_finish(vsc) != 0) {
		virtio_free_vq(vsc, &sc->sc_vq);
		goto failed;
	}

	return;
failed:
	virtio_child_attach_failed(vsc);
	return;
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

static int vaccel_config_change(struct virtio_softc *vsc)
{
	//struct vaccel_softc *sc = device_private(virtio_child(vsc));

	//virtaccel_update_status(sc, vsc);
	virtaccel_update_status(vsc);

	return 0;
}

static int virtaccel_update_status(struct virtio_softc *vsc)
{
	uint32_t status;
	struct vaccel_softc *sc = device_private(virtio_child(vsc));
	struct virtio_accel *vaccel = &sc->vaccel;
	//int err;

	printf("config changed in vaccel\n");
	status = virtio_read_device_config_4(vsc, 0);
	/*
	 * Unknown status bits would be a host error and the driver
	 * should consider the device to be broken.
	 */
	if (status & (~VIRTIO_ACCEL_S_HW_READY)) {
		aprint_error_dev(sc->sc_dev, 
				  "vaccel: Unknown status bits: 0x%x\n", status);
		return -EPERM;
	}

	if (vaccel->status == status)
		return 0;

	vaccel->status = status;

	if (vaccel->status & VIRTIO_ACCEL_S_HW_READY) {
		//err = virtaccel_dev_start(vaccel);
		//if (err) {
		//	dev_err(&vaccel->vdev->dev,
		//		"Failed to start virtio accel device.\n");

		//	return -EPERM;
		//}
		//dev_info(&vaccel->vdev->dev, "Accelerator is ready\n");
		aprint_normal_dev(sc->sc_dev, "Accelerator is ready\n");
	} else {
		//virtaccel_dev_stop(vaccel);
		//dev_info(&vaccel->vdev->dev, "Accelerator is not ready\n");
		aprint_error_dev(sc->sc_dev, "Accelerator is not ready\n");
	}

	return 0;
}

int vaccel_vq_done(struct virtqueue *vq)
{
	printf("hello from virtqueu done\n");
	return 1;
}

