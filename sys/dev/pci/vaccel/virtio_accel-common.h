#ifndef _VIRTIO_ACCEL_COMMON_H
#define _VIRTIO_ACCEL_COMMON_H

#include "virtio_accel.h"
//#include <linux/completion.h>

#define VQ_NAME_LEN 16

struct sessions_list {
	struct list_head list;
	u32 id;
};

struct virtio_accel_vq {
    struct virtqueue *vq;
    spinlock_t lock;
    char name[VQ_NAME_LEN];
};

struct virtio_accel {
	struct virtio_device *vdev;
	struct virtio_accel_vq *vq;
	unsigned int dev_minor;
	unsigned long status;

	struct module *owner;
	struct list_head list;
	atomic_t ref_count;
	uint8_t dev_id;
};

struct virtio_accel_req {
	struct virtio_accel_hdr hdr;
	struct virtio_accel *vaccel;
	struct scatterlist **sgs;
	unsigned int out_sgs;
	unsigned int in_sgs;
	void *priv;
	void __user *usr;
	struct completion completion;
	u32 status;
	int ret;
};

struct virtio_accel_file {
	struct virtio_accel *vaccel;
	struct list_head sessions;
};
#endif /* _VIRTIO_ACCEL_COMMON_H */
