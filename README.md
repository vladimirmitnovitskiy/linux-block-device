# RAMDISK

A simple **Linux kernel block device driver** implementing a RAM-backed virtual disk.

# Kernel version

Driver was made for Linux kernel version 6.18.13

# Build

Run from repository root:

```bash
make
```

# Usage

### 1. Load the module

You can specify the disk name and size (in megabytes) via module parameters. By default, it creates a 50MB disk named `myramdisk`.

```bash
sudo insmod ramdisk.ko disk_name="<your_disk_name>" disk_size_mb=<size_in_mb>
```

Check the kernel logs to verify succeful initialization:

```bash
dmesg | tail -n 2
# Expected output: <your_disk_name>: Disk activate! Size: <size_in_mb> MB
```

### 2. Format and Mount

Before you can use the disk to store files, you need to create file system on it and mount it to a directory:

```bash
sudo mkfs.ext4 /dev/<your_disk_name>
sudo mkdir -p /mnt/ramdisk
sudo mount /dev/<your_disk_name> /mnt/ramdisk
```

Now you can read and write files inside `/mnt/ramdisk`

### 3. Unload

WARNING: Always unmount the device BEFORE removing the kernel module to prevent Kernel Panic!

```bash
sudo unmount /mnt/ramdisk
sudo rmmod ramdisk
```

# Test

The repository includes a C-based userspace testing utility (test_app) that writes 10MB of data to the raw block device, reads it back, verifies data integrity (encryption/decryption logic), and measures I/O speed.

```bash
sudo insmod ramdisk.ko
sudo ./test_app
sudo rmmod ramdisk
```

## License

GPL-2.0     