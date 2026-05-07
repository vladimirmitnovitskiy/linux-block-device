#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/vmalloc.h>
#include <linux/blkdev.h>
#include <linux/blk-mq.h>

static char *disk_name = "myramdisk";
module_param(disk_name, charp, 0444);
MODULE_PARM_DESC(disk_name, "Name of the virtual disk (will appear in lsblk)");

static int disk_size_mb = 50;
module_param(disk_size_mb, int, 0444);
MODULE_PARM_DESC(disk_size_mb, "RAM disk size in megabytes");

struct my_ramdisk {
	size_t size;
	u8 *data;
	struct blk_mq_tag_set tag_set;
	struct gendisk *gd;
};

static struct my_ramdisk *device;
static int major_num;

static blk_status_t my_queue_rq(struct blk_mq_hw_ctx *hctx, const struct blk_mq_queue_data *bd)
{
	struct request *req = bd->rq;
	struct bio_vec bvec;
	struct req_iterator iter;
	sector_t pos_sector = blk_rq_pos(req);
	void *buffer;
	u32 len;
	int dir = rq_data_dir(req);

	blk_mq_start_request(req);

	if (pos_sector + blk_rq_sectors(req) > (device->size / SECTOR_SIZE)) {
	pr_err("%s: Request out of disk space!", disk_name);
	blk_mq_end_request(req, BLK_STS_IOERR);
	return BLK_STS_OK;
	}

	rq_for_each_segment(bvec, req, iter) {
	len = bvec.bv_len;
	buffer = page_address(bvec.bv_page) + bvec.bv_offset;

	if (dir == WRITE) {
	    memcpy(device->data + (pos_sector * SECTOR_SIZE), buffer, len);
	} else {
	    memcpy(buffer, device->data + (pos_sector * SECTOR_SIZE), len);
	}
	pos_sector += len / SECTOR_SIZE;
	}

	blk_mq_end_request(req, BLK_STS_OK);
	return BLK_STS_OK;
}

static const struct blk_mq_ops my_mq_ops = {
	.queue_rq = my_queue_rq,
};

static const struct block_device_operations my_fops = {
	.owner = THIS_MODULE,
};

static int __init my_ramdisk_init(void)
{
	int err;

	if (disk_size_mb <= 0) {
	pr_err("%s: ERROR! Invalid disk size: %d MB\n", disk_name, disk_size_mb);
	return -EINVAL;
	}

	device = kzalloc(sizeof(struct my_ramdisk), GFP_KERNEL);
	if (!device)
	return -ENOMEM;

	device->size = (size_t)disk_size_mb * 1024 * 1024;
	device->data = vmalloc(device->size);
	if (!device->data) {
	pr_err("%s: ERROR! Failed to allocate %d MB of RAM.\n", disk_name, disk_size_mb);
	kfree(device);
	return -ENOMEM;
	}

	major_num = register_blkdev(0, disk_name);
	if (major_num < 0) {
	vfree(device->data);
	kfree(device);
	return major_num;
	}

	err = blk_mq_alloc_sq_tag_set(&device->tag_set, &my_mq_ops, 128, 0);
	if (err)
	goto out_blkdev;

	device->gd = blk_mq_alloc_disk(&device->tag_set, NULL, NULL);
	if (IS_ERR(device->gd)) {
	err = PTR_ERR(device->gd);
	goto out_tags;
	}

	device->gd->major = major_num;
	device->gd->first_minor = 0;
	device->gd->minors = 1;
	device->gd->fops = &my_fops;
	device->gd->private_data = device;
	snprintf(device->gd->disk_name, 32, disk_name);
	set_capacity(device->gd, device->size / SECTOR_SIZE);

	err = add_disk(device->gd);
	if (err)
	goto out_disk;


	pr_info("%s: Disk activate! Size: %zu MB\n", disk_name, device->size / 1024 / 1024);
	return 0;

out_disk:
	put_disk(device->gd);
out_tags:
	blk_mq_free_tag_set(&device->tag_set);
out_blkdev:
	unregister_blkdev(major_num, disk_name);
	vfree(device->data);
	kfree(device);
	return err;
}

static void __exit my_ramdisk_exit(void)
{
	if (device->gd) {
	del_gendisk(device->gd);
	put_disk(device->gd);
	}
	blk_mq_free_tag_set(&device->tag_set);
	unregister_blkdev(major_num, disk_name);

	vfree(device->data);
	kfree(device);

	pr_info("%s: Disk deactivate\n", disk_name);
}

module_init(my_ramdisk_init);
module_exit(my_ramdisk_exit);

MODULE_LICENSE("GPL");
