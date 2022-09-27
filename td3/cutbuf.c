#include <linux/module.h>
#include <linux/cdev.h>
#include <linux/slab.h>
#include <linux/kernel.h>
#include <linux/uaccess.h>
#include <stdbool.h>

MODULE_LICENSE("GPL");

#define MAX_BUF 4
#define MAX_DEV (MAX_BUF + 1)
#define BUF_SIZE 4096

#define TST_GETDBG   0
#define TST_SETDBG   1
#define TST_GETBLOCK 3
#define TST_SETBLOCK 4

struct cutbuf_device_data {
    struct cdev cdev;
    char* buf;
    size_t len;
    bool locked;
    bool mode_debug;
    bool mode_block;
} cutbuf_dev[MAX_DEV];

static int dev_major = 0;

static int cutbuf_open(struct inode *inode, struct file *file)
{
    printk("cutbuf: Device open\n");
    int minor = iminor(inode);
    struct cutbuf_device_data *dev = &cutbuf_dev[minor];

    if (dev->locked) {
        printk("cutbuf: Device %d is locked\n", minor);
        return dev->mode_block ? -EAGAIN : -EBUSY;
    }

    if (!dev->buf) {
        dev->len = 0;
        if (!(dev->buf = kmalloc(BUF_SIZE, GFP_KERNEL))) {
            printk("cutbuf: Failed to allocate buffer\n");
            return -ENOMEM;
        }
    }

    dev->locked = true;

    return 0;
}

static int cutbuf_release(struct inode *inode, struct file *file)
{
    printk("cutbuf: Device close\n");
    int minor = iminor(inode);
    struct cutbuf_device_data *dev = &cutbuf_dev[minor];

    dev->locked = false;

    return 0;
}

static long cutbuf_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    printk("cutbuf: Device ioctl\n");
    int minor = iminor(file_inode(file));
    struct cutbuf_device_data *dev = &cutbuf_dev[minor];

    switch (cmd) {
    case TST_GETDBG:
        printk("cutbuf: TST_GETDBG\n");
        int res = put_user(42, (int __user *)arg);
        return res ? res : dev->mode_debug;
    case TST_SETDBG:
        printk("cutbuf: TST_SETDBG\n");
        dev->mode_debug = arg;
        return 0;
    case TST_GETBLOCK:
        printk("cutbuf: TST_GETBLOCK\n");
        return dev->mode_block;
    case TST_SETBLOCK:
        printk("cutbuf: TST_SETBLOCK\n");
        dev->mode_block = arg;
        return 0;
    default:
        printk("cutbuf: Unknown ioctl\n");
        return -ENOTTY;
    }

    return 0;
}

static ssize_t cutbuf_read(struct file *file, char __user *buf, size_t count, loff_t *offset)
{
    printk("cutbuf: Device read\n");
    int minor = iminor(file_inode(file));
    struct cutbuf_device_data *dev = &cutbuf_dev[minor];

    if (*offset >= dev->len) {
        return 0;
    }

    if (*offset + count > dev->len) {
        count = dev->len - *offset;
    }

    if (copy_to_user(buf, dev->buf + *offset, count)) {
        return -EFAULT;
    }

    *offset += count;

    return count;
}

static ssize_t cutbuf_write(struct file *file, const char __user *buf, size_t count, loff_t *offset)
{
    printk("cutbuf: Device write\n");
    int minor = iminor(file_inode(file));
    struct cutbuf_device_data *dev = &cutbuf_dev[minor];

    if (*offset >= BUF_SIZE) {
        return 0;
    }

    if (*offset + count > BUF_SIZE) {
        count = BUF_SIZE - *offset;
    }

    if (copy_from_user(dev->buf + *offset, buf, count)) {
        return -EFAULT;
    }

    dev->len = (*offset += count);

    return count;
}

#define STATS_SIZE 1024
char stats_buf[STATS_SIZE];
size_t stats_len = 0;

static int cbstat_open(struct inode *inode, struct file *file)
{
    printk("cutbuf: Device open\n");
    int minor = iminor(inode);
    struct cutbuf_device_data *dev = &cutbuf_dev[minor];

    int open = cutbuf_open(inode, file);
    if (open) {
        return open;
    }

    dev->buf[0] = 0;
    dev->len = 0;
    for (int i = 0; i < MAX_BUF; i++) {
        struct cutbuf_device_data* cdev = &cutbuf_dev[i + 1];
        dev->len += snprintf(dev->buf + dev->len, STATS_SIZE - dev->len, "cutbuf%d: buffer=%p %zu char. in_use=%d\n", i + 1, 
            cdev->buf, cdev->len, cdev->locked);
    }
    return 0;
}

static const struct file_operations cutbuf_fops = {
    .owner      = THIS_MODULE,
    .open       = cutbuf_open,
    .release    = cutbuf_release,
    .unlocked_ioctl = cutbuf_ioctl,
    .read       = cutbuf_read,
    .write       = cutbuf_write
};

static const struct file_operations cbstat_fops = {
    .owner      = THIS_MODULE,
    .open       = cbstat_open,
    .release    = cutbuf_release,
    .unlocked_ioctl = cutbuf_ioctl,
    .read       = cutbuf_read
};

static struct class *cutbuf_class = NULL;

static int __init init(void)
{
	printk("cutbuf: loaded\n");
    dev_t dev;
    int err = alloc_chrdev_region(&dev, 0, MAX_DEV, "cutbuf");
    if (err < 0) {
        printk(KERN_ALERT "cutbuf: can't get major\n");
        return err;
    }
    dev_major = MAJOR(dev);
    cutbuf_class = class_create(THIS_MODULE, "cutbuf");
    if (IS_ERR(cutbuf_class)) {
        printk(KERN_ALERT "cutbuf: can't create class\n");
        return PTR_ERR(cutbuf_class);
    }
    for (int i = 0; i < MAX_DEV; i++) {
        cdev_init(&cutbuf_dev[i].cdev, i == 0 ? &cbstat_fops : &cutbuf_fops);
        cutbuf_dev[i].cdev.owner = THIS_MODULE;
        err = cdev_add(&cutbuf_dev[i].cdev, MKDEV(dev_major, i), 1);
        if (err < 0) {
            printk(KERN_ALERT "cutbuf: can't add device\n");
            return err;
        }
        if (i == 0) {
            device_create(cutbuf_class, NULL, MKDEV(dev_major, i), NULL, "cbstat");
        } else {
            device_create(cutbuf_class, NULL, MKDEV(dev_major, i), NULL, "cutbuf%d", i);
            cutbuf_dev[i].buf = NULL;
        }
        cutbuf_dev[i].locked = false;
        cutbuf_dev[i].mode_debug = false;
        cutbuf_dev[i].mode_block = false;
    }
	return 0;
}

static void __exit cleanup(void)
{
	printk("cutbuf: cleanup\n");
    for (int i = 0; i < MAX_DEV; i++) {
        device_destroy(cutbuf_class, MKDEV(dev_major, i));
        cdev_del(&cutbuf_dev[i].cdev);
        if (i > 0 && cutbuf_dev[i].buf != NULL) {
            kfree(cutbuf_dev[i].buf);
        }
    }
    class_unregister(cutbuf_class);
    class_destroy(cutbuf_class);
    unregister_chrdev_region(MKDEV(dev_major, 0), MAX_DEV);
}

module_init(init);
module_exit(cleanup);
