#include <sys/proc.h>
#include <sys/conf.h>
#include <sys/systm.h>

#include <sys/kmem.h>
#include <sys/errno.h>

#include "accel.h"
#include "virtio_accel-common.h"

void virtaccel_clear_req(struct virtio_accel_req *req)
{
	struct virtio_accel_hdr *h = &req->hdr;
	int i;

	switch (h->op) {
	case VIRTIO_ACCEL_G_OP_CREATE_SESSION:
		if (h->u.gen_op.out) {
			for (i = 0; i < h->u.gen_op.out_nr; i++) {
				if (h->u.gen_op.out[i].buf)
					kmem_free(h->u.gen_op.out[i].buf, h->u.gen_op.out[i].len);
			}
			kmem_free(h->u.gen_op.out, sizeof(*h->u.gen_op.out));
		}
		if (h->u.gen_op.in) {
			for (i = 0; i < h->u.gen_op.in_nr; i++) {
				if (h->u.gen_op.in[i].buf)
					kmem_free(h->u.gen_op.in[i].buf, h->u.gen_op.in[i].len);
			}
			kmem_free(h->u.gen_op.in, sizeof(*h->u.gen_op.in));
		}
	case VIRTIO_ACCEL_G_OP_DESTROY_SESSION:
		kmem_free((struct accel_session *)req->priv, sizeof(struct accel_session));
		break;
	case VIRTIO_ACCEL_G_OP_DO_OP:
		if (h->u.gen_op.out) {
			for (i = 0; i < h->u.gen_op.out_nr; i++) {
				if (h->u.gen_op.out[i].buf)
					kmem_free(h->u.gen_op.out[i].buf, h->u.gen_op.out[i].len);
			}
			kmem_free(h->u.gen_op.out, sizeof(*h->u.gen_op.out));
		}
		if (h->u.gen_op.in) {
			for (i = 0; i < h->u.gen_op.in_nr; i++) {
				if (h->u.gen_op.in[i].buf)
					kmem_free(h->u.gen_op.in[i].buf, h->u.gen_op.in[i].len);
			}
			kmem_free(h->u.gen_op.in, sizeof(*h->u.gen_op.in));
		}
		kmem_free((struct accel_op *)req->priv, sizeof(struct accel_op));
		break;
	default:
		printf("clear req: invalid op returned\n");
	}
	req->usr = NULL;
}

void virtaccel_handle_req_result(struct virtio_accel_req *req)
{
	struct virtio_accel_hdr *h = &req->hdr;
	struct vaccel_softc *sc =req->vaccel;
	int i;
	struct accel_session *sess;

	if (req->status != VIRTIO_ACCEL_OK)
		return;
	switch (h->op) {
	case VIRTIO_ACCEL_G_OP_CREATE_SESSION:
		sess = req->priv;
		printf("handle req: succesfully created session %u\n", sess->id);
		if (h->u.gen_op.in) {
			for (i = 0; i < h->u.gen_op.in_nr; i++) {
				if (!h->u.gen_op.in[i].buf)
					continue;
				memcpy(h->u.gen_op.in[i].usr_buf, h->u.gen_op.in[i].buf,
						h->u.gen_op.in[i].len);
			}
		}
		memcpy(req->usr, sess, sizeof(*sess));
		break;
	case VIRTIO_ACCEL_G_OP_DO_OP:
		if (!h->u.gen_op.in)
			break;

		for (i = 0; i < h->u.gen_op.in_nr; i++) {
			printf("yoyo %p\n", h->u.gen_op.in[i].buf);
			if (!h->u.gen_op.in[i].buf)
				continue;
			printf("yoyo %s\n", sc->sc_arg_in[i].buf);
			memcpy(h->u.gen_op.in[i].usr_buf, sc->sc_arg_in[i].buf, sc->sc_arg_in[i].len);
		}
		break;
	default:
		printf("hadle req: invalid op returned\n");
	}
}

int virtaccel_req_gen_operation(struct virtio_accel_req *req)
{
	struct virtio_accel_hdr *h = &req->hdr;
	struct vaccel_softc *vaccel_sc = req->vaccel;
	struct accel_op *op = req->priv;
	struct accel_gen_op *gen = &op->u.gen;
	struct accel_gen_op_arg *g_arg = NULL;
	int ret = 0, i, total_sgs = 2 + gen->in_nr + gen->out_nr;

	h->op = VIRTIO_ACCEL_G_OP_DO_OP;
	h->session_id = op->session_id;
	h->u.gen_op.in_nr = gen->in_nr;
	h->u.gen_op.out_nr = gen->out_nr;
	vaccel_sc->sc_arg_in = NULL;
	vaccel_sc->sc_arg_out = NULL;

	if (h->u.gen_op.in_nr > 0) {
		g_arg = kmem_zalloc(gen->in_nr * sizeof(*gen->in),KM_SLEEP);
		if (!g_arg)
			return -ENOMEM;
		
		memcpy(g_arg, gen->in, gen->in_nr * sizeof(*gen->in));
		h->u.gen_op.in = kmem_zalloc(h->u.gen_op.in_nr * sizeof(*h->u.gen_op.in),KM_SLEEP);
		if (!h->u.gen_op.in) {
			ret = -ENOMEM;
			goto free;
		}
		
		for (i = 0; i < gen->in_nr; i++) {
			h->u.gen_op.in[i].len = g_arg[i].len;
			h->u.gen_op.in[i].usr_buf = g_arg[i].buf;
			printf("hoola-hoop %p\n", h->u.gen_op.in[i].usr_buf);
			memcpy(g_arg[i].buf, "hoola", 5);
			h->u.gen_op.in[i].buf = kmem_zalloc(h->u.gen_op.in[i].len, KM_SLEEP);
			if (!h->u.gen_op.in[i].buf) {
				ret = -ENOMEM;
				goto free_in;
			}
			/*
			if (unlikely(copy_from_user(h->u.gen_op.in[i].buf, g_arg[i].buf,
								g_arg[i].len))) {
				ret = -EFAULT;
				goto free_in;
			}
			*/
		}
		kmem_free(g_arg, sizeof(*g_arg));
		vaccel_sc->sc_arg_in = h->u.gen_op.in;
	}

	if (h->u.gen_op.out_nr > 0) {
		g_arg = kmem_zalloc(gen->out_nr * sizeof(*gen->out), KM_SLEEP);
		if (!g_arg)
			return -ENOMEM;
		
		memcpy(g_arg, gen->out,	gen->out_nr * sizeof(*gen->out));
		h->u.gen_op.out = kmem_zalloc(h->u.gen_op.out_nr * sizeof(*h->u.gen_op.out), KM_SLEEP);
		if (!h->u.gen_op.out) {
			ret = -ENOMEM;
			goto free_in;
		}
		
		for (i = 0; i < gen->out_nr; i++) {
			h->u.gen_op.out[i].len = g_arg[i].len;
			h->u.gen_op.out[i].usr_buf = g_arg[i].buf;
			h->u.gen_op.out[i].buf = kmem_zalloc(h->u.gen_op.out[i].len, KM_SLEEP);
			if (!h->u.gen_op.out[i].buf) {
				ret = -ENOMEM;
				goto free_out;
			}
			memcpy(h->u.gen_op.out[i].buf, g_arg[i].buf, g_arg[i].len);
		}
		kmem_free(g_arg, sizeof(*g_arg));
		vaccel_sc->sc_arg_out = h->u.gen_op.out;
	}
	ret = vaccel_send_request(vaccel_sc, h, req, total_sgs);
	return ret;
free_out:
	if (h->u.gen_op.out) {
		for (i = 0; i < h->u.gen_op.out_nr; i++) {
			if (h->u.gen_op.out[i].buf)
				kmem_free(h->u.gen_op.out[i].buf, h->u.gen_op.out[i].len);
		}
		kmem_free(h->u.gen_op.out, sizeof(*h->u.gen_op.out));
	}
free_in:
	if (h->u.gen_op.in) {
		for (i = 0; i < h->u.gen_op.in_nr; i++) {
			if (h->u.gen_op.in[i].buf)
				kmem_free(h->u.gen_op.in[i].buf, h->u.gen_op.in[i].len);
		}
		kmem_free(h->u.gen_op.in, sizeof(*h->u.gen_op.in));
	}
free:
	if (g_arg)
		kmem_free(g_arg, sizeof(*g_arg));
	return ret;
}

int virtaccel_req_gen_create_session(struct virtio_accel_req *req)
{
	struct vaccel_softc *vaccel_sc = req->vaccel;
	struct virtio_accel_hdr *h = &req->hdr;
	struct accel_session *sess = req->priv;
	struct accel_gen_op *gen = &sess->u.gen;
	struct accel_gen_op_arg *g_arg = NULL;
	int ret = 0, out_nsgs = 0, in_nsgs = 0, i,
	    total_sgs = 3 + gen->in_nr + gen->out_nr;

	printf("total_sgs = %d ----%d-%d\n", total_sgs, in_nsgs, out_nsgs);
	h->op = VIRTIO_ACCEL_G_OP_CREATE_SESSION;
	h->u.gen_op.in_nr = gen->in_nr;
	h->u.gen_op.out_nr = gen->out_nr;
	vaccel_sc->sc_arg_in = NULL;
	vaccel_sc->sc_arg_out = NULL;

	if (h->u.gen_op.in_nr > 0) {
		g_arg = kmem_zalloc(gen->in_nr * sizeof(*gen->in),
						KM_SLEEP);
		if (!g_arg)
			return -ENOMEM;
		
		memcpy(g_arg, gen->in, gen->in_nr * sizeof(*gen->in));

		h->u.gen_op.in = kmem_zalloc(h->u.gen_op.in_nr * sizeof(*h->u.gen_op.in),
					KM_SLEEP);
		if (!h->u.gen_op.in) {
			ret = -ENOMEM;
			goto free;
		}
		
		for (i = 0; i < gen->in_nr; i++) {
			h->u.gen_op.in[i].len = g_arg[i].len;
			h->u.gen_op.in[i].usr_buf = g_arg[i].buf;
			h->u.gen_op.in[i].buf = kmem_zalloc(h->u.gen_op.in[i].len,
						KM_SLEEP);
			if (!h->u.gen_op.in[i].buf) {
				ret = -ENOMEM;
				goto free_in;
			}
			/*
			if (unlikely(copy_from_user(h->u.gen_op.in[i].buf, g_arg[i].buf,
								g_arg[i].len))) {
				ret = -EFAULT;
				goto free_in;
			}
			*/
		}
		kmem_free(g_arg, gen->in_nr * sizeof(*gen->in));
		vaccel_sc->sc_arg_in = h->u.gen_op.in;
	}

	if (h->u.gen_op.out_nr > 0) {
		g_arg = kmem_zalloc(gen->out_nr * sizeof(*gen->out),
						KM_SLEEP);
		if (!g_arg)
			return -ENOMEM;
		
		memcpy(g_arg, gen->out, gen->out_nr * sizeof(*gen->out));

		h->u.gen_op.out = kmem_zalloc(h->u.gen_op.out_nr * sizeof(*h->u.gen_op.out),
				KM_SLEEP);
		if (!h->u.gen_op.out) {
			ret = -ENOMEM;
			goto free_in;
		}
		
		for (i = 0; i < gen->out_nr; i++) {
			h->u.gen_op.out[i].len = g_arg[i].len;
			h->u.gen_op.out[i].usr_buf = g_arg[i].buf;
			h->u.gen_op.out[i].buf = kmem_zalloc(h->u.gen_op.out[i].len,
					KM_SLEEP);
			if (!h->u.gen_op.out[i].buf) {
				ret = -ENOMEM;
				goto free_out;
			}
			memcpy(h->u.gen_op.out[i].buf, g_arg[i].buf,
					h->u.gen_op.out[i].len);
		}
		kmem_free(g_arg, gen->out_nr * sizeof(*gen->out));
		vaccel_sc->sc_arg_out = h->u.gen_op.out;
	}

	ret = vaccel_send_request(vaccel_sc, h, req, total_sgs);
	return ret;

free_out:
	if (h->u.gen_op.out) {
		for (i = 0; i < h->u.gen_op.out_nr; i++) {
			if (h->u.gen_op.out[i].buf)
				kmem_free(h->u.gen_op.out[i].buf, h->u.gen_op.out[i].len);
		}
		kmem_free(h->u.gen_op.out, h->u.gen_op.out_nr * sizeof(*h->u.gen_op.out));
	}
free_in:
	if (h->u.gen_op.in) {
		for (i = 0; i < h->u.gen_op.in_nr; i++) {
			if (h->u.gen_op.in[i].buf)
				kmem_free(h->u.gen_op.in[i].buf, h->u.gen_op.in[i].len);
		}
		kmem_free(h->u.gen_op.in, h->u.gen_op.in_nr * sizeof(*h->u.gen_op.in));
	}
free:
	if (g_arg)
		kmem_free(g_arg, gen->in_nr * sizeof(*gen->in));
	return ret;
}
