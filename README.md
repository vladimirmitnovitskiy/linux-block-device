# RAMDISK

A simple **Linux kernel block device driver** implementing a RAM-backed virtual disk.

# Kernel version

Driver was made for Linux kernel version 6.18.13

# Build

Run from repository root:

```bash
make
```

# Load

Run `sudo insmod ramdisk.ko disk_name="<your_disk_name>" disk_size_mb=<your_disk_size_in_mb>`

You will see in `dmesg`: `<your_disk_name>: Disk activate! Size: <your_disk_size_in_mb>` and you will see `<your_disk_name>` in `lsblk`.



# Test

Run `sudo insmod ramdisk.ko` and run `./test_app`, then run `rmmod ramdisk`.

## License

GPL-2.0     