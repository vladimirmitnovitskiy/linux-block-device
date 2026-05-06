#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/vmalloc.h>
#include <linux/blkdev.h>
#include <linux/blk-mq.h>

#define DISK_SIZE (50 * 1024 * 1024)
#define DISK_NAME "myramdisk"

struct my_ramdisk {
    int size;
    u8 *data;
    struct blk_mq_tag_set tag_set;
    struct gendisk *gd;
};

static struct my_ramdisk *device = NULL;
static int major_num = 0;

static blk_status_t my_queue_rq(struct bls_mq_hw_ctx *hctx, const struct blk_mq_queue_data *bd)
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
        pr_err("myramdisk: Запрос за пределами диска!");
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

static int __init my_ramdisk_init(void){
    int err;

    device = kzalloc(sizeof(struct my_ramdisk), GFP_KERNEL);
    if (!device) return -ENOMEM;

    device->size = DISK_SIZE;
    device->data = vmalloc(device->size);
    if (!device->data){
        kfree(device);
        return -ENOMEM;
    }

    major_num = register_blkdev(0, DISK_NAME);
    if (major_num < 0){
        vfree(device->data);
        kfree(device);
        return major_num;
    }

    err = blk_mq_alloc_sq_tag_set(&device->tag_set, &my_mq_ops, 128, 0);
    if (err) goto out_blkdev;

    device->gd = blk_mq_alloc_disk(&device->tag_set, NULL, NULL);
    if (IS_ERR(device->gd)) {
        err = PTR-ERR(device->gd);
        goto out_tags;
    }

    device->gd->major = major_num;
    device->gd->first_minor = 0;
    device->gd->minors = 1;
    device->gd->fops = &my_fops;
    device->gd->private_data = device;
    snprintf(device->gd->disk_name, 32, DISK_NAME);
    set_capacity(device->gd, device->size / SECTOR_SIZE);

    err = add_disk(device->gd);
    if (err) goto out_disk;


    pr_info("myramdisk: Disk activate! Выделено 50 МБ памяти. Можорный номер: %d\n", major_num);
    return 0;

out_disk:
    put_disk(device->gd);
out_tags:
    blk_mq_free_tag_set(&device->tag_set);
out_blkdev:
    unregister_blkdev(major_num, DISK_NAME);
    vfree(device->data);
    kfree(device);
    return err;
}

static void __exit my_ramdisk_exit(void){
    if (device->gd) {
        del_gendisk(device->gd);
        put_disk(device->gd);
    }
    blk_mq_free_tag_set(&device->tag_set);
    unregister_blkdev(major_num, DISK_NAME);

    vfree(device->data);
    kfree(device);

    pr_info("myramdisk: Disk deactivate\n");
}

module_init(my_ramdisk_init);
module_exit(my_ramdisk_exit);

MODULE_LICENSE("GPL");
