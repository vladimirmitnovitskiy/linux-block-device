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

```bash
sudo insmod ramdisk.ko disk_name="<your_disk_name>" disk_size_mb=<size_in_mb>
```
After loading you should see in `dmesg`:

```bash
<your_disk_name>: Disk activate! Size: <your_disk_size_in_mb>
```
and the device will appear in:

```bash
lsblk
```



# Test

```bash
sudo insmod ramdisk.ko
./test_app
sudo rmmod ramdisk
```

# Unload

```bash
sudo rmmod ramdisk
```

## License

GPL-2.0     