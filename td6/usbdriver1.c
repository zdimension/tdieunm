#include <linux/module.h>
#include <linux/usb.h>
#include <linux/slab.h>

#include "phidget.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Tom");
MODULE_DESCRIPTION("IeEUNM - driver for Phidget 8-8-8 USB card");
MODULE_VERSION("0.1");

#define VENDOR_ID 0x06c2
#define PRODUCT_ID 0x0045
static struct usb_device_id skel_table[] = {
	{ USB_DEVICE(VENDOR_ID, PRODUCT_ID) },
	{ } // Termine la structure
};
MODULE_DEVICE_TABLE(usb, skel_table);

#define USB_SKEL_MINOR_BASE 0

struct skel_device {
	char name[128];
	char phys[64];
	struct usb_device* udev;
	struct usb_interface* interface;
	wait_queue_head_t wait_q;
	bool reading;
	struct urb* urb;
	struct usb_endpoint_descriptor* endpoint;
	int size;
	char* buf;
	int valid;
	int used;
};

// try operator
#define TRY(x) do { int __ret = (x); if (__ret < 0) { printk(KERN_ERR "%s:Error %d in %s\n", __FUNCTION__, __ret, #x); return __ret; } } while (0)

static ssize_t skel_fill(struct skel_device* dev)
{
	printk(KERN_INFO "skel_fill\n");

	if (dev->valid != dev->used) {
		printk(KERN_INFO "skel_fill: valid != used\n");
		return 0;
	}

	dev->reading = true;
	int result = usb_submit_urb(dev->urb, GFP_KERNEL);
	if (result) {
		printk(KERN_ERR "usb_submit_urb failed: %d\n", result);
		result = usb_translate_errors(result);
		dev->reading = false;
		return result;
	}

	result = wait_event_interruptible_timeout(dev->wait_q, !dev->reading, 5 * HZ);

	if (result == 0) {
		printk(KERN_ERR "wait_event_interruptible_timeout: timeout\n");
		usb_kill_urb(dev->urb);
		return -ETIMEDOUT;
	}

	if (result < 0) {
		printk(KERN_ERR "wait_event_interruptible_timeout: error\n");
		usb_kill_urb(dev->urb);
		return result;
	}

	printk(KERN_INFO "skel_fill: valid = %d, used = %d\n", dev->valid, dev->used);

	return dev->valid;
}

static ssize_t skel_read(struct file* file, char __user* buf, size_t count, loff_t* offset)
{
	printk(KERN_INFO "skel_read called with %zu\n", count);

	struct skel_device* dev = file->private_data;
	if (!dev) {
		printk(KERN_ERR "skel_read: file->private_data is NULL !\n");
		return -ENODEV;
	}

	printk(KERN_INFO "skel_read: count = %zu, dev->valid = %d, dev->used = %d\n", count, dev->valid, dev->used);

	if (*offset) {
		return 0;
	}

	if (dev->valid == dev->used) {
		TRY(skel_fill(dev));
	}

	int size = min(count, (size_t)(dev->valid - dev->used));

	count -= size;
	dev->used += size;

	char b[512];
	char* p = b;
	for (int i = 0; i < size; i++) {
		p += sprintf(p, "%02x ", (unsigned char)dev->buf[i]);
	}
	*p = 0;
	printk(KERN_INFO "skel_read: %s\n", b);

	struct PhidgetInterfaceKit* kit = phidget_ifkit_init();
	phidget_parse_packet(kit, dev->buf, dev->valid);

	char* user_buffer = kmalloc(1024, GFP_KERNEL);
	size_t len = sprintf(user_buffer, "PhidgetInterfaceKit: %4d %4d %4d %4d %4d %4d %4d %4d\n",
		kit->sensorValue[0][0], kit->sensorValue[1][0], kit->sensorValue[2][0], kit->sensorValue[3][0],
		kit->sensorValue[4][0], kit->sensorValue[5][0], kit->sensorValue[6][0], kit->sensorValue[7][0]);
	printk(KERN_INFO "skel_read: %s\n", user_buffer);
	int res = copy_to_user(buf, user_buffer, len);
	if (res) {
		printk(KERN_ERR "copy_to_user failed\n");
		return -EFAULT;
	}
	kfree(user_buffer);
	phidget_ifkit_free(kit);
	printk(KERN_INFO "skel_read: %zu bytes read\n", len);
	*offset += len;
	return len;
}

static ssize_t skel_write(struct file* file, const char __user* buf, size_t count, loff_t* offset)
{
	printk(KERN_INFO "skel_write called with %zu\n", count);
	return 0;
}

static int skel_open(struct inode* inode, struct file* file);

static int skel_release(struct inode* inode, struct file* file)
{
	printk(KERN_INFO "skel_release called !\n");

	struct usb_skel* dev = file->private_data;
	if (!dev) {
		printk(KERN_ERR "skel_release: file->private_data is NULL !\n");
		return -ENODEV;
	}

	return 0;
}

static struct file_operations skel_fops = {
	.owner = THIS_MODULE,
	.read = skel_read,
	.write = skel_write,
	.open = skel_open,
	.release = skel_release
};

static struct usb_class_driver skel_class = {
	.name = "skel%d",
	.fops = &skel_fops,
	.minor_base = USB_SKEL_MINOR_BASE 
};

static void skel_completion(struct urb* urb)
{
	struct skel_device* dev = urb->context;

	int status = urb->status;

	printk(KERN_INFO "skel_completion called with status %d\n", status);

	if (status == 0) {
		dev->valid = urb->actual_length;
	} else {
		dev->valid = 0;
	}

	dev->used = 0;

	dev->reading = false;
	wake_up_interruptible(&dev->wait_q);
}

static int skel_probe(struct usb_interface* iface, const struct usb_device_id* id)
{
	printk(KERN_INFO "skel_probe called (%04x:%04x)\n", id->idVendor, id->idProduct);

	struct skel_device* dev = kzalloc(sizeof(*dev), GFP_KERNEL);
	if (dev == NULL) {
		printk(KERN_ERR "skel_probe: out of memory\n");
		return -ENOMEM;
	}

	init_waitqueue_head(&dev->wait_q);

	dev->udev = usb_get_dev(interface_to_usbdev(iface));
	dev->interface = iface;

	// find the interrupt endpoint
	struct usb_host_interface* host_iface = iface->cur_altsetting;
	struct usb_endpoint_descriptor* endpoint = NULL;
	for (int i = 0; i < host_iface->desc.bNumEndpoints; i++) {
		endpoint = &host_iface->endpoint[i].desc;
		if (usb_endpoint_is_int_in(endpoint)) {
			printk(KERN_INFO "skel_probe: found interrupt endpoint\n");
			break;
		}
	}
	if (!endpoint) {
		printk(KERN_ERR "skel_probe: can't find interrupt endpoint\n");
		kfree(dev);
		return -ENODEV;
	}
	dev->urb = usb_alloc_urb(0, GFP_KERNEL);
	if (!dev->urb) {
		printk(KERN_ERR "skel_probe: can't allocate urb\n");
		kfree(dev);
		return -ENOMEM;
	}
	
	dev->endpoint = endpoint;
	int size = usb_endpoint_maxp(endpoint);
	if (size > 64) size = 64;
	dev->size = size;
	dev->buf = kmalloc(size, GFP_KERNEL);
	if (!dev->buf) {
		printk(KERN_ERR "skel_probe: can't allocate buffer\n");
		usb_free_urb(dev->urb);
		kfree(dev);
		return -ENOMEM;
	}
	usb_fill_int_urb(dev->urb, dev->udev, usb_rcvintpipe(dev->udev, endpoint->bEndpointAddress),
		dev->buf, size, skel_completion, dev, endpoint->bInterval);

	usb_set_intfdata(iface, dev);

	int result = usb_register_dev(iface, &skel_class);

	if (result < 0) {
		printk(KERN_ERR "skel_probe: usb_register_dev failed\n");
		usb_set_intfdata(iface, NULL);
	    return result;
	}

	dev_info(&iface->dev, "USB Skeleton device now attached with minor %d\n", iface->minor);

	return 0;
}

static void skel_disconnect(struct usb_interface* iface)
{
	printk(KERN_INFO "skel_disconnect called !\n");

	int minor = iface->minor;

	struct skel_device* dev = usb_get_intfdata(iface);
	usb_set_intfdata(iface, NULL);

	usb_deregister_dev(iface, &skel_class);

	usb_kill_urb(dev->urb);

	dev_info(&iface->dev, "USB Skeleton device %d now disconnected\n", minor);
}


static struct usb_driver skel_driver = {
	.name = "skel_driver",
	.id_table = skel_table, // définit l’association id - driver
	.probe = skel_probe, // fonction appelée à la connexion
	.disconnect = skel_disconnect, // à la déconnexion
};

static int skel_open(struct inode* inode, struct file* file)
{
	printk(KERN_INFO "skel_open called !\n");

	int minor = iminor(inode);
	struct usb_interface* interface = usb_find_interface(&skel_driver, minor);
	if (!interface) {
		printk(KERN_ERR "skel_open: can't find device for minor %d\n", minor);
		return -ENODEV;
	}

	struct skel_device* dev = usb_get_intfdata(interface);
	if (!dev) {
		printk(KERN_ERR "skel_open: can't find device data for minor %d\n", minor);
		return -ENODEV;
	}

	file->private_data = dev;

	return 0;
}

module_usb_driver(skel_driver);