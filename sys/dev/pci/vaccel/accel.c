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

/*
 * Handle an open request on the device.
 */
int vaccelopen(dev_t device, int flags, int fmt, struct lwp *process)
{
	int ret = 0;

	//int unit = DISKUNIT(device);
	//struct vaccel_softc *sc = device_lookup_private(&vaccel_cd, unit);
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
int vaccelclose(dev_t device, int flags, int fmt, struct lwp *process)
{
	printf("Accel closed\n");
	return 0; /*this always succeeds */
}

/*
 * Handle the ioctl for the device
 */
int vaccelioctl(dev_t device, u_long command, void *data, int flags,
	      struct lwp *process)
{
	struct virtio_accel_req *req;
	struct accel_session *sess = NULL;
	struct accel_op *op = NULL;
	int ret = 0;
	int unit = DISKUNIT(device);
	struct vaccel_softc *sc = device_lookup_private(&vaccel_cd, unit);

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
		if (!req) {
			ret = -ENOMEM;
			goto err_mem;
		}
		sess = kmem_zalloc(sizeof(struct accel_session), KM_SLEEP);
		if (!sess) {
			ret = -ENOMEM;
			goto err_mem_req;
		}

		memcpy(sess, data, sizeof(struct accel_session));
		//sess = (struct accel_session *) data;
		req->usr = data;
		req->priv = sess;
		req->vaccel = sc;

		if (command == ACCIOC_GEN_SESS_CREATE)
			ret = virtaccel_req_gen_create_session(req);
		//else
		//	ret = virtaccel_req_gen_destroy_session(req);

		if (ret) {
			ret = -EFAULT;
			goto err_mem_req;
		}
		break;
	case ACCIOC_GEN_DO_OP:
		printf("tetarto\n");
		req = kmem_zalloc(sizeof(*req), KM_SLEEP);
		if (!req) {
			ret = -ENOMEM;
			goto err_mem;
		}
		op = kmem_zalloc(sizeof(*op), KM_SLEEP);
		if (!op) {
			ret = -ENOMEM;
			goto err_mem_req;
		}

		memcpy(op, data, sizeof(*op));

		req->usr = data;
		req->priv = op;
		req->vaccel = sc;
		
		printf("hoola-hoop ho %p\n", op->u.gen.in[0].buf);
		printf("hoola-hoop ho %p\n", op->u.gen.in[1].buf);
		ret = virtaccel_req_gen_operation(req);
		if (ret != 0)
			goto err_mem_req;

		break;
	default:
		printf("Invalid IOCTL\n\n");
		ret = -EFAULT;
	}

	printf("Waiting for request to complete\n");
	mutex_enter(&sc->sc_mutex);
	cv_wait(&sc->sc_sync_wait, &sc->sc_mutex);
	req->status = sc->req_status;
	mutex_exit(&sc->sc_mutex);
	virtaccel_handle_req_result(req);
	virtaccel_clear_req(req);
	printf("request completed\n");

	return ret;

err_mem_req:
	kmem_free(req, sizeof(struct virtio_accel_req));
err_mem:
	return -ENOMEM;

}
