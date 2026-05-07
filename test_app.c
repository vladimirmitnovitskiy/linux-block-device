#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <string.h>

#define DEVICE "/dev/myramdisk"
#define BUF_SIZE (10 * 1024 * 1024) // Пишем и читаем куском в 10 Мегабайт

// Функция для вычисления разницы во времени в секундах
double get_time_diff(struct timespec start, struct timespec end) {
    return (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
}

int main(int argc, char *argv[]) {
    int fd;
    char *write_buf, *read_buf;
    struct timespec start, end;
    double write_time, read_time;

    char *dev_name = DEVICE;
    if (argc > 1) dev_name = argv[1];

    printf("--- Тестирование устройства: %s ---\n", dev_name);

    // Открываем диск как сырой файл
    fd = open(dev_name, O_RDWR);
    if (fd < 0) {
        perror("ОШИБКА: Не удалось открыть устройство. Вы забыли sudo?");
        return EXIT_FAILURE;
    }

    write_buf = malloc(BUF_SIZE);
    read_buf = malloc(BUF_SIZE);
    memset(write_buf, 'Z', BUF_SIZE); // Заполняем память буквами 'Z'
    memset(read_buf, 0, BUF_SIZE);

    // 1. ТЕСТ ЗАПИСИ
    clock_gettime(CLOCK_MONOTONIC, &start);
    ssize_t bytes_written = write(fd, write_buf, BUF_SIZE);
    clock_gettime(CLOCK_MONOTONIC, &end);
    
    if (bytes_written < 0) {
        perror("Ошибка записи");
        close(fd);
        return EXIT_FAILURE;
    }
    write_time = get_time_diff(start, end);
    printf("[ЗАПИСЬ] %zd байт за %f сек (Скорость: %.2f MB/s)\n", 
           bytes_written, write_time, (bytes_written / 1024.0 / 1024.0) / write_time);

    // Перематываем указатель обратно в начало диска
    lseek(fd, 0, SEEK_SET);

    // 2. ТЕСТ ЧТЕНИЯ
    clock_gettime(CLOCK_MONOTONIC, &start);
    ssize_t bytes_read = read(fd, read_buf, BUF_SIZE);
    clock_gettime(CLOCK_MONOTONIC, &end);

    if (bytes_read < 0) {
        perror("Ошибка чтения");
        close(fd);
        return EXIT_FAILURE;
    }
    read_time = get_time_diff(start, end);
    printf("[ЧТЕНИЕ] %zd байт за %f сек (Скорость: %.2f MB/s)\n", 
           bytes_read, read_time, (bytes_read / 1024.0 / 1024.0) / read_time);

    // 3. ПРОВЕРКА ДАННЫХ
    if (memcmp(write_buf, read_buf, BUF_SIZE) == 0) {
        printf("УСПЕХ: Прочитанные данные полностью совпадают с записанными!\n");
    } else {
        printf("ОШИБКА: Данные повреждены!\n");
    }

    free(write_buf);
    free(read_buf);
    close(fd);
    return EXIT_SUCCESS;
}