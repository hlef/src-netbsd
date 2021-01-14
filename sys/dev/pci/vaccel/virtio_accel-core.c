#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/callout.h>
#include <sys/rndsource.h>
#include <sys/mutex.h>
#include <dev/pci/virtioreg.h>
#include <dev/pci/virtiovar.h>

#include <sys/proc.h>
#include <sys/conf.h>

#include <sys/types.h>
#include <sys/filedesc.h>
#include <sys/kmem.h>

//dev_type_open(vacceloopen);
//dev_type_close(vaccelclose);
//dev_type_ioctl(vaccelioctl);

#include "virtio_accel.h"
#include "virtio_accel-common.h"

const struct cdevsw vaccel_cdevsw = {
        .d_open = vaccelopen,
	.d_close = vaccelclose,
        .d_ioctl = vaccelioctl,

	.d_write = nowrite,
	.d_read = noread,
	.d_stop = nostop,
	.d_tty = notty,
	.d_poll = nopoll,
	.d_mmap = nommap,
	.d_kqfilter = nokqfilter,
	.d_discard = nodiscard,
	.d_flag = D_OTHER
};

static int	vaccel_match(device_t, cfdata_t, void *);
static void	vaccel_attach(device_t, device_t, void *);
//static int	vaccel_detach(device_t, int);
static int      vaccel_config_change(struct virtio_softc *);
static int	virtaccel_update_status(struct virtio_softc *);
//static int	virtaccel_update_status(struct vaccel_softc *sc,
//				   struct virtio_softc *vsc);
int		vaccel_vq_done(struct virtqueue *vq);
static int	vaccel_virtio_alloc_reqs(struct vaccel_softc *sc, int qsize);


CFATTACH_DECL_NEW(vaccel, sizeof(struct vaccel_softc),
		  vaccel_match, vaccel_attach, NULL, NULL);

static void vaccel_attach(device_t parent, device_t self, void *aux)
{
	//struct virtio_accel *vaccel;
	struct vaccel_softc *sc = device_private(self);
	struct virtio_softc *vsc = device_private(parent);
	int error;
	//uint32_t features;
	/* dma things */
	//bus_dma_segment_t segs[1];
	//int nsegs;

	if (virtio_child(vsc) != NULL)
		panic("already attached to something else");
	
	sc->sc_dev = self;
	sc->sc_virtio = vsc;
	mutex_init(&sc->sc_mutex, MUTEX_DEFAULT, IPL_VM);
	cv_init(&sc->sc_sync_wait, "vaccelsync");

	/* dma things */
	//error = bus_dmamem_alloc(virtio_dmat(vsc),
	//		VIRTIO_PAGE_SIZE, 0, 0, segs, 1, &nsegs,
	//		BUS_DMA_NOWAIT|BUS_DMA_ALLOCNOW);
	//if (error) {
	//	aprint_error_dev(sc->sc_dev, "can't alloc dmamem: %d\n", error);
	//	goto alloc_failed;
	//}

	//error = bus_dmamem_map(virtio_dmat(vsc), segs, nsegs, 1024,
	//		&sc->sc_buf, BUS_DMA_NOWAIT);
	//if (error) {
	//	aprint_error_dev(sc->sc_dev, "can't map dmamem: %d\n", error);
	//	goto map_failed;
	//}

	//error = bus_dmamap_create(virtio_dmat(vsc), 1024, 1, 1024, 0,
	//		BUS_DMA_NOWAIT|BUS_DMA_ALLOCNOW,
	//		&sc->sc_dmamap);
	//if (error) {
	//	aprint_error_dev(sc->sc_dev, "can't create dmamem: %d\n", error);
	//	goto create_failed;
	//}

	//error = bus_dmamap_load(virtio_dmat(vsc), sc->sc_dmamap,
	//		sc->sc_buf, 1024, NULL,
	//		BUS_DMA_NOWAIT|BUS_DMA_READ);
	//if (error) {
	//	aprint_error_dev(sc->sc_dev, "can't load dmamem: %d\n", error);
	//	goto load_failed;
	//}

	/* virtio things */
	virtio_child_attach_start(vsc, self, IPL_NET, &sc->sc_vq,
				  vaccel_config_change, virtio_vq_intr, 0,
				  0, VIRTIO_COMMON_FLAG_BITS);

	//features = virtio_features(vsc);
	
	error = virtio_alloc_vq(vsc, &sc->sc_vq, 0, 1024, 1024, "vaccel vq");
	if (error) {
		aprint_error_dev(sc->sc_dev, "can't alloc virtqueue: %d\n",
				error);
		goto failed;
	}

	if (vaccel_virtio_alloc_reqs(sc, sc->sc_vq.vq_num))
		goto failed;

	sc->sc_vq.vq_done = vaccel_vq_done;

	virtaccel_update_status(vsc);

	if (virtio_child_attach_finish(vsc) != 0) {
		virtio_free_vq(vsc, &sc->sc_vq);
		goto failed;
	}
	aprint_normal_dev(sc->sc_dev, "vAccel virtio device attached successfully\n");

	return;

failed:
//	bus_dmamap_unload(virtio_dmat(vsc), sc->sc_dmamap);
//load_failed:
//	bus_dmamap_destroy(virtio_dmat(vsc), sc->sc_dmamap);
//create_failed:
//	bus_dmamem_unmap(virtio_dmat(vsc), sc->sc_buf, 1024);
//map_failed:
//	bus_dmamem_free(virtio_dmat(vsc), segs, nsegs);
//alloc_failed:
	virtio_child_attach_failed(vsc);
	return;
}

static int vaccel_match(device_t parent, cfdata_t match, void *aux)
{
	struct virtio_attach_args *va = aux;

	if (va->sc_childdevid == VIRTIO_ID_ACCEL) {
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
	//int err;

	aprint_normal_dev(sc->sc_dev, "Update config\n");
	status = virtio_read_device_config_4(vsc, 0);
	/*
	 * Unknown status bits would be a host error and the driver
	 * should consider the device to be broken.
	 */
	if (status & (~VIRTIO_ACCEL_S_HW_READY)) {
		aprint_error_dev(sc->sc_dev, 
				  "Unknown status bits: 0x%x\n", status);
		return -EPERM;
	}

	if (sc->status == status)
		return 0;

	sc->status = status;

	if (sc->status & VIRTIO_ACCEL_S_HW_READY) {
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
	struct virtio_softc *vsc = vq->vq_owner;
	struct vaccel_softc *sc = device_private(virtio_child(vsc));
	struct virtio_vaccel_req *vr;
	struct virtio_accel_hdr *h;
	int slot, len, r, ret = 0, i;

	mutex_enter(&sc->sc_mutex);
	//mutex_spin_enter(&sc->sc_mutex);
	
	for (;;) {
		r = virtio_dequeue(vsc, vq, &slot, &len);
		if (r != 0)
			break;

		aprint_normal_dev(sc->sc_dev, "Got slot %d with len %d\n", slot, len);

		vr = &sc->sc_reqs[slot];
		h = &vr->hdr;
		bus_dmamap_sync(virtio_dmat(vsc), vr->vr_cmds,
				0, sizeof(struct virtio_accel_hdr),
				BUS_DMASYNC_POSTWRITE);
		if (sc->sc_arg_out) {
			for (i = 0; i < h->u.gen_op.out_nr; i++) {
				bus_dmamap_sync(virtio_dmat(vsc), vr->vr_out_buf[i],
						0, sc->sc_arg_out[i].len*sizeof(uint8_t),
							BUS_DMASYNC_POSTWRITE);
			}
		}
		if (sc->sc_arg_in) {
			for (i = 0; i < h->u.gen_op.in_nr; i++) {
				//printf("vr_in_buf =%s\n", vr->in_buf[i]);
				memcpy(sc->sc_arg_in[i].buf, vr->in_buf[i],
						sc->sc_arg_in[i].len*sizeof(uint8_t));
				bus_dmamap_sync(virtio_dmat(vsc), vr->vr_in_buf[i],
						0, sc->sc_arg_in[i].len*sizeof(uint8_t),
						BUS_DMASYNC_POSTREAD);
			}
		}
		bus_dmamap_sync(virtio_dmat(vsc), vr->vr_cmds,
				offsetof(struct virtio_vaccel_req, sid), sizeof(uint32_t),
				BUS_DMASYNC_POSTREAD);
		bus_dmamap_sync(virtio_dmat(vsc), vr->vr_cmds,
				offsetof(struct virtio_vaccel_req, status), sizeof(uint32_t),
				BUS_DMASYNC_POSTREAD);
		if (sc->sc_arg_out) {
			for (i = 0; i < h->u.gen_op.out_nr; i++)
				bus_dmamap_unload(virtio_dmat(vsc), vr->vr_out_buf[i]);
		}
		if (sc->sc_arg_in) {
			for (i = 0; i < h->u.gen_op.in_nr; i++)
				bus_dmamap_unload(virtio_dmat(vsc), vr->vr_in_buf[i]);
		}
		switch(vr->status) {
		case VIRTIO_ACCEL_OK:
			sc->req_status = 0;
			break;
		case VIRTIO_ACCEL_INVSESS:
		case VIRTIO_ACCEL_ERR:
			//req->ret = -EINVAL;
			break;
		case VIRTIO_ACCEL_BADMSG:
			//req->ret = -EBADMSG;
			break;
		default:
			//req->ret = -EIO;
			break;
		}

		//vaccel_req_done(sc, vsc, &sc->sc_reqs[slot], vq, slot);
		ret = 1;
	} 
	cv_signal(&sc->sc_sync_wait);
	mutex_exit(&sc->sc_mutex);
	//mutex_spin_exit(&sc->sc_mutex);

	return ret;
}

static int vaccel_virtio_alloc_reqs(struct vaccel_softc *sc, int qsize)
{
	//int allocsize, r, rsegs;

	int allocsize, r, rsegs, i, j;
	void *vaddr;

	allocsize = sizeof(struct virtio_vaccel_req) * qsize;
	r = bus_dmamem_alloc(virtio_dmat(sc->sc_virtio), allocsize, 0, 0,
			&sc->sc_reqs_seg, 1, &rsegs, BUS_DMA_NOWAIT);
	if (r) {
		aprint_error_dev(sc->sc_dev,
				"DMA memory allocation failed, size %d, "
				"error code %d\n", allocsize, r);
		goto err_none;
	}
	r = bus_dmamem_map(virtio_dmat(sc->sc_virtio),
			&sc->sc_reqs_seg, 1, allocsize,
			&vaddr, BUS_DMA_NOWAIT);
	if (r) {
		aprint_error_dev(sc->sc_dev,
				"DMA memory map failed, "
				"error code %d\n", r);
		goto err_dmamem_alloc;
	}
	sc->sc_reqs = vaddr;
	memset(vaddr, 0, allocsize);
	sc->sc_reqs->hdr.op = 1;

	for (i = 0; i < qsize; i++) {
		struct virtio_vaccel_req *vr = &sc->sc_reqs[i];
		r = bus_dmamap_create(virtio_dmat(sc->sc_virtio),
				offsetof(struct virtio_vaccel_req, out_buf),
				1,
				offsetof(struct virtio_vaccel_req, out_buf),
				0,
				BUS_DMA_NOWAIT|BUS_DMA_ALLOCNOW,
				&vr->vr_cmds);
		if (r) {
			aprint_error_dev(sc->sc_dev,
					"command dmamap creation failed, "
					"error code %d\n", r);
			goto err_reqs;
		}
		r = bus_dmamap_load(virtio_dmat(sc->sc_virtio), vr->vr_cmds,
				&vr->hdr,
				offsetof(struct virtio_vaccel_req, out_buf),
				NULL, BUS_DMA_NOWAIT);
		if (r) {
			aprint_error_dev(sc->sc_dev,
					"command dmamap load failed, "
					"error code %d\n", r);
			goto err_reqs;
		}
		for (j=0; j< 5; j++) {
			r = bus_dmamap_create(virtio_dmat(sc->sc_virtio),
					100*VIRTIO_PAGE_SIZE,
					1,
					100*VIRTIO_PAGE_SIZE,
					0,
					BUS_DMA_NOWAIT|BUS_DMA_ALLOCNOW,
					&vr->vr_out_buf[j]);
			if (r) {
				aprint_error_dev(sc->sc_dev,
						"command dmamap creation failed, "
						"error code %d\n", r);
				goto err_reqs;
			}
			r = bus_dmamap_create(virtio_dmat(sc->sc_virtio),
					VIRTIO_PAGE_SIZE,
					10,
					VIRTIO_PAGE_SIZE,
					0,
					BUS_DMA_NOWAIT|BUS_DMA_ALLOCNOW,
					&vr->vr_in_buf[j]);
			if (r) {
				aprint_error_dev(sc->sc_dev,
						"command dmamap creation failed, "
						"error code %d\n", r);
				goto err_reqs;
			}
		}
	}

	return 0;
err_reqs:
	for (i = 0; i < qsize; i++) {
		struct virtio_vaccel_req *vr = &sc->sc_reqs[i];
		if (vr->vr_cmds) {
			bus_dmamap_destroy(virtio_dmat(sc->sc_virtio),
					   vr->vr_cmds);
			vr->vr_cmds = 0;
		}
		if (vr->vr_out_buf) {
			for (j=0; j< 5; j++) {
				bus_dmamap_destroy(virtio_dmat(sc->sc_virtio),
						   vr->vr_out_buf[j]);
				vr->vr_out_buf[j] = 0;
			}
		}
		if (vr->vr_in_buf) {
			for (j=0; j< 5; j++) {
				bus_dmamap_destroy(virtio_dmat(sc->sc_virtio),
						   vr->vr_in_buf[j]);
				vr->vr_in_buf[j] = 0;
			}
		}
	}
	bus_dmamem_unmap(virtio_dmat(sc->sc_virtio), sc->sc_reqs, allocsize);
err_dmamem_alloc:
	bus_dmamem_free(virtio_dmat(sc->sc_virtio), &sc->sc_reqs_seg, 1);
err_none:
	return 1;
}

int vaccel_send_request(struct vaccel_softc *sc, struct virtio_accel_hdr *h,
			struct virtio_accel_req *req, int total_segs)
{
	struct virtio_softc *vsc = sc->sc_virtio;
	struct virtqueue *vq = &sc->sc_vq;
	struct virtio_vaccel_req *vr;
	int slot;
	int i, segs = 2;
	int r = 0;
	uint32_t out_len = 0, in_len = 0;

	r = virtio_enqueue_prep(vsc, vq, &slot);
	if (r)
		return r;
	vr = &sc->sc_reqs[slot];

	for (i = 0; i < h->u.gen_op.out_nr; i++) {
		r = bus_dmamap_load(virtio_dmat(vsc), vr->vr_out_buf[i],
				h->u.gen_op.out[i].buf, h->u.gen_op.out[i].len, NULL,
				BUS_DMA_WRITE|BUS_DMA_NOWAIT);
		if (r) {
			aprint_error_dev(sc->sc_dev,
					"out_buf dmamap failed, error code %d\n", r);
			virtio_enqueue_abort(vsc, vq, slot);
			return r;
		}
		vr->out_buf[i] = h->u.gen_op.out[i].buf;
		out_len += h->u.gen_op.out[i].len;
		segs++;
	}

	for (i = 0; i < h->u.gen_op.in_nr; i++) {
		r = bus_dmamap_load(virtio_dmat(vsc), vr->vr_in_buf[i],
				h->u.gen_op.in[i].buf, h->u.gen_op.in[i].len, NULL,
				BUS_DMA_READ|BUS_DMA_NOWAIT);
		if (r) {
			aprint_error_dev(sc->sc_dev,
					"in_buf dmamap failed, error code %d\n", r);
			virtio_enqueue_abort(vsc, vq, slot);
			return r;
		}
		in_len += h->u.gen_op.in[i].len;
		vr->in_buf[i] = h->u.gen_op.in[i].buf;
		//segs += vr->vr_out_buf->dm_nsegs;
		segs++;
	}

	//if (h->u.gen_op.out_nr > 0) {
	//	segs += vr->vr_out_buf->dm_nsegs;
	//	//for (i = 0; i < h->u.gen_op.out[0].len; i++) {
	//	//	printf("%hhx ", h->u.gen_op.out[0].buf[i]);
	//	//	if ( i % 10 == 0)
	//	//		printf("\n");
	//	//}
	//	//r = virtio_enqueue_reserve(vsc, vq, slot, vr->vr_out_buf->dm_nsegs);
	//	//if (r) {
	//	//	aprint_error_dev(sc->sc_dev,
	//	//			"out_buf virtio_enqueue_reserve failed, error code %d\n", r);
	//	//	bus_dmamap_unload(virtio_dmat(vsc), vr->vr_out_buf);
	//	//	return r;
	//	//}
	//}
	//if (h->u.gen_op.in_nr > 0) {
	//	segs += vr->vr_in_buf->dm_nsegs;
	//	//r = virtio_enqueue_reserve(vsc, vq, slot, vr->vr_in_buf->dm_nsegs);
	//	//if (r) {
	//	//	aprint_error_dev(sc->sc_dev,
	//	//			"in_buf virtio_enqueue_reserve failed, error code %d\n", r);
	//	//	bus_dmamap_unload(virtio_dmat(vsc), vr->vr_in_buf);
	//	//	return r;
	//	//}
	//}

	//r = virtio_enqueue_reserve(vsc, vq, slot, 3);
	r = virtio_enqueue_reserve(vsc, vq, slot, total_segs);
	if (r) {
		aprint_error_dev(sc->sc_dev,
				"in_buf virtio_enqueue_reserve failed, error code %d\n", r);
		//bus_dmamap_unload(virtio_dmat(vsc), vr->vr_in_buf);
		return r;
	}

	memcpy(&vr->hdr, h, sizeof(struct virtio_accel_hdr));
	bus_dmamap_sync(virtio_dmat(vsc), vr->vr_cmds,
			0, sizeof(struct virtio_accel_hdr),
			BUS_DMASYNC_PREWRITE);
	if (h->u.gen_op.out_nr > 0) {
		for (i = 0; i < h->u.gen_op.out_nr; i++) {
			bus_dmamap_sync(virtio_dmat(vsc), vr->vr_out_buf[i],
					0, h->u.gen_op.out[i].len,
					BUS_DMASYNC_PREWRITE);
		}
	}
	if (h->u.gen_op.in_nr > 0) {
		for (i = 0; i < h->u.gen_op.in_nr; i++) {
			bus_dmamap_sync(virtio_dmat(vsc), vr->vr_cmds,
					0, h->u.gen_op.in[i].len,
					BUS_DMASYNC_PREREAD);
		}
	}
	if (total_segs - segs == 1)
		bus_dmamap_sync(virtio_dmat(vsc), vr->vr_cmds,
				offsetof(struct virtio_vaccel_req, sid), sizeof(uint32_t),
				BUS_DMASYNC_PREREAD);
	bus_dmamap_sync(virtio_dmat(vsc), vr->vr_cmds,
			offsetof(struct virtio_vaccel_req, status), sizeof(uint32_t),
			BUS_DMASYNC_PREREAD);

	virtio_enqueue_p(vsc, vq, slot, vr->vr_cmds,
			0, sizeof(struct virtio_accel_hdr),
			true);
	if (h->u.gen_op.out_nr > 0) {
		for (i = 0; i < h->u.gen_op.out_nr; i++) {
			virtio_enqueue(vsc, vq, slot, vr->vr_out_buf[i], true);
		}
	}
	if (h->u.gen_op.in_nr > 0) {
		for (i = 0; i < h->u.gen_op.in_nr; i++) {
			virtio_enqueue(vsc, vq, slot, vr->vr_in_buf[i], false);
		}
	}
	if (total_segs - segs == 1)
		virtio_enqueue_p(vsc, vq, slot, vr->vr_cmds,
				offsetof(struct virtio_vaccel_req, sid), sizeof(uint32_t),
				false);
	virtio_enqueue_p(vsc, vq, slot, vr->vr_cmds,
			offsetof(struct virtio_vaccel_req, status), sizeof(uint32_t),
			false);
	virtio_enqueue_commit(vsc, vq, slot, true);

	return 0;
}

