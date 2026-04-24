#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/vmalloc.h>
#include <linux/blkdev.h>
#include <linux/blk-mq.h>

#define DISK_NAME "myramdisk"
#define DISK_SIZE (50 * 1024 * 1024) // Размер нашего диска: 50 Мегабайт
#define SECTOR_SIZE 512              // Размер одного сектора (стандарт для Linux)

// Структура, которая описывает наше устройство
struct my_ramdisk {
    int size;                       // Размер диска в байтах
    u8 *data;                       // Указатель на выделенную память (наши данные)
    struct blk_mq_tag_set tag_set;  // Структура для управления очередью (blk-mq)
    struct gendisk *gd;             // Структура самого диска (то, что видит ОС)
};

static struct my_ramdisk *device = NULL;
static int major_num = 0; // Мажорный номер устройства

// -------------------------------------------------------------------------
// САМОЕ ГЛАВНОЕ: Функция обработки запросов на чтение и запись
// -------------------------------------------------------------------------
static blk_status_t my_queue_rq(struct blk_mq_hw_ctx *hctx, const struct blk_mq_queue_data *bd)
{
    struct request *req = bd->rq;
    struct bio_vec bvec;
    struct req_iterator iter;
    sector_t pos_sector = blk_rq_pos(req); // С какого сектора нужно читать/писать
    void *buffer;
    u32 len;
    int dir = rq_data_dir(req); // Узнаем направление: WRITE (запись) или READ (чтение)

    // Сообщаем ядру, что мы начали обрабатывать запрос
    blk_mq_start_request(req);

    // Проверка на выход за пределы нашего диска
    if (pos_sector + blk_rq_sectors(req) > (device->size / SECTOR_SIZE)) {
        pr_err("myramdisk: запрос за пределами диска\n");
        blk_mq_end_request(req, BLK_STS_IOERR);
        return BLK_STS_OK;
    }

    // Идем по всем кусочкам данных, которые передало ядро
    rq_for_each_segment(bvec, req, iter) {
        len = bvec.bv_len; // Длина кусочка
        buffer = page_address(bvec.bv_page) + bvec.bv_offset; // Откуда берем данные

        // Выполняем саму работу (копирование байтов)
        if (dir == WRITE) {
            // Если ЗАПИСЬ: копируем ИЗ буфера ядра В нашу память
            memcpy(device->data + (pos_sector * SECTOR_SIZE), buffer, len);
        } else {
            // Если ЧТЕНИЕ: копируем ИЗ нашей памяти В буфер ядра
            memcpy(buffer, device->data + (pos_sector * SECTOR_SIZE), len);
        }
        pos_sector += len / SECTOR_SIZE; // Сдвигаем позицию
    }

    // Сообщаем ядру, что запрос успешно выполнен
    blk_mq_end_request(req, BLK_STS_OK);
    return BLK_STS_OK;
}

// Связываем нашу функцию обработки с интерфейсом blk-mq
static const struct blk_mq_ops my_mq_ops = {
    .queue_rq = my_queue_rq,
};

// Пустая структура файловых операций (ядро будет использовать базовые)
static const struct block_device_operations my_fops = {
    .owner = THIS_MODULE,
};

// -------------------------------------------------------------------------
// ИНИЦИАЛИЗАЦИЯ (срабатывает при insmod)
// -------------------------------------------------------------------------
static int __init my_ramdisk_init(void)
{
    int err;

    // 1. Выделяем память под структуру устройства
    device = kzalloc(sizeof(struct my_ramdisk), GFP_KERNEL);
    if (!device) return -ENOMEM;

    // 2. Выделяем оперативную память под сам "жесткий диск" (50 МБ)
    device->size = DISK_SIZE;
    device->data = vmalloc(device->size);
    if (!device->data) {
        kfree(device);
        return -ENOMEM;
    }

    // 3. Регистрируем блочное устройство (получаем мажорный номер)
    major_num = register_blkdev(0, DISK_NAME);
    if (major_num < 0) {
        vfree(device->data);
        kfree(device);
        return major_num;
    }

    // 4. Настраиваем очередь запросов blk-mq
    err = blk_mq_alloc_sq_tag_set(&device->tag_set, &my_mq_ops, 128, BLK_MQ_F_SHOULD_MERGE);
    if (err) goto out_blkdev;

    // 5. Создаем структуру диска (gendisk) в ядре (Новый API для ядра 6.x)
    device->gd = blk_mq_alloc_disk(&device->tag_set, NULL);
    if (IS_ERR(device->gd)) {
        err = PTR_ERR(device->gd);
        goto out_tags;
    }

    // 6. Заполняем информацию о диске
    device->gd->major = major_num;
    device->gd->first_minor = 0;
    device->gd->minors = 1;
    device->gd->fops = &my_fops;
    device->gd->private_data = device;
    snprintf(device->gd->disk_name, 32, DISK_NAME);
    
    // Устанавливаем вместимость диска в секторах
    set_capacity(device->gd, device->size / SECTOR_SIZE);

    // 7. Сообщаем ядру: "Диск готов, можно пользоваться!"
    err = add_disk(device->gd);
    if (err) goto out_disk;

    pr_info("myramdisk: успешно загружен (50 МБ)\n");
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

// -------------------------------------------------------------------------
// ВЫГРУЗКА (срабатывает при rmmod)
// -------------------------------------------------------------------------
static void __exit my_ramdisk_exit(void)
{
    // Удаляем всё в обратном порядке
    if (device->gd) {
        del_gendisk(device->gd); // Удаляем диск из системы
        put_disk(device->gd);    // Освобождаем структуру
    }
    blk_mq_free_tag_set(&device->tag_set);
    unregister_blkdev(major_num, DISK_NAME);
    
    // Обязательно освобождаем память, иначе утечка!
    vfree(device->data);
    kfree(device);
    
    pr_info("myramdisk: успешно выгружен\n");
}

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Student");
MODULE_DESCRIPTION("Simple RAM Block Device");

module_init(my_ramdisk_init);
module_exit(my_ramdisk_exit);