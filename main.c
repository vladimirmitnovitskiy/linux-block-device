#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/vmalloc.h>
#include <linux/blkdev.h>
#include <linux/blk-mq.h>

static int __init my_ramdisk_init(void){
    pr_info("Disk activate");
    return 0;
}

static void __exit my_ramdisk_exit(void){
    pr_info("Disk deactivate");
}

module_init(my_ramdisk_init);
module_exit(my_ramdisk_exit);

MODULE_LICENSE("GPL-2.0");
