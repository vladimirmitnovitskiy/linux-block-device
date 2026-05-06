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
};

static struct my_ramdisk *device = NULL;
static int major_num = 0;



static int __init my_ramdisk_init(void){
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

    pr_info("Disk activate! Выделено 50 МБ памяти. Можорный номер: %d\n", major_num);
    return 0;
}

static void __exit my_ramdisk_exit(void){
    unregister_blkdev(major_num, DISK_NAME);
    vfree(device->data);
    kfree(device);

    pr_info("Disk deactivate\n");
}

module_init(my_ramdisk_init);
module_exit(my_ramdisk_exit);

MODULE_LICENSE("GPL");
