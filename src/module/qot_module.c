#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/posix-clock.h>
#include <linux/hashtable.h>
#include <asm/uaccess.h>

// Expose  module to userspace (ioctl) and other modules (export)
#include "qot_ioctl.h"

// Module information
#define MODULE_NAME "qot"
#define FIRST_MINOR 0
#define MINOR_CNT 1

// Stores information about a timeline
struct qot_timeline {
    struct posix_clock clock;
    struct device *dev;
	int references;						// number of references to the timeline
    dev_t devid;
    int index;                      	// index into clocks.map
    int defunct;                    	// tells readers to go away when clock is being removed
    char uuid[QOT_MAX_UUIDLEN];			// unique id for a timeline
    struct hlist_node collision_hash;	// pointer to next timeline for the same hash key
    struct list_head head_acc;			// head pointing to maximum accuracy structure
    struct list_head head_res;			// head pointing to maximum resolution structure
};

// Stores information about a binding
struct qot_binding {
	char   uuid[QOT_MAX_UUIDLEN]; 
	uint64_t resolution;				// Desired resolution
	uint64_t accuracy;					// Desired accuracy
    struct list_head res_sort_list;		// Next resolution
    struct list_head acc_sort_list;		// Next accuracy
};

// An array of binding pointers
static struct qot_binding *bindings[QOT_MAX_BINDINGS+1];

// The next available binding ID, as well as the total number alloacted
static int binding_point = 0;
static int binding_total = 0;

// A hash table designed to quickly find timeline based on UUID
DEFINE_HASHTABLE(qot_timelines_hash, QOT_HASHTABLE_BITS);

// IOCTL FUNCTIONALITY /////////////////////////////////////////////////////////

// Required for ioctl
static dev_t dev;
static struct cdev c_dev;
static struct class *cl;

static int qot_ioctl_open(struct inode *i, struct file *f)
{
    return 0;
}
static int qot_ioctl_close(struct inode *i, struct file *f)
{
    return 0;
}

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,35))
static int qot_ioctl_access(struct inode *i, struct file *f, unsigned int cmd, unsigned long arg)
#else
static long qot_ioctl_access(struct file *f, unsigned int cmd, unsigned long arg)
#endif
{
	switch (cmd)
	{
	case QOT_BIND_TIMELINE:
		break;
	case QOT_GET_ACHIEVED:
		break;
	case QOT_GET_TARGET:
		break;
	case QOT_SET_ACCURACY:
		break;
	case QOT_SET_RESOLUTION:
		break;
	case QOT_UNBIND_TIMELINE:
		break;
	case QOT_WAIT_UNTIL:
		break;
	default:
		return -EINVAL;
	}

	// Success!
	return 0;
}

// Define the file operations over th ioctl
static struct file_operations qot_fops = {
    .owner = THIS_MODULE,
    .open = qot_ioctl_open,
    .release = qot_ioctl_close,
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,35))
    .ioctl = qot_ioctl_access
#else
    .unlocked_ioctl = qot_ioctl_access
#endif
};

// Called on initialization
static int init_module(void)
{
    int ret;
    struct device *dev_ret;
	printk(KERN_WARNING "Intializing QoT stack\n");
    if ((ret = alloc_chrdev_region(&dev, FIRST_MINOR, MINOR_CNT, "qot_ioctl")) < 0)
    {
    	printk(KERN_WARNING "region alloc\n");
        return ret;
    }
    cdev_init(&c_dev, &qot_fops); 
    if ((ret = cdev_add(&c_dev, dev, MINOR_CNT)) < 0)
    {
    	printk(KERN_WARNING "cdev_add alloc\n");
        return ret;
    }
    if (IS_ERR(cl = class_create(THIS_MODULE, "char")))
    {
    	printk(KERN_WARNING "class_create\n");
        cdev_del(&c_dev);
        unregister_chrdev_region(dev, MINOR_CNT);
        return PTR_ERR(cl);
    }
    if (IS_ERR(dev_ret = device_create(cl, NULL, dev, NULL, "qot")))
    {
    	printk(KERN_WARNING "device_create\n");
        class_destroy(cl);
        cdev_del(&c_dev);
        unregister_chrdev_region(dev, MINOR_CNT);
        return PTR_ERR(dev_ret);
    }
    printk(KERN_WARNING "success\n");
 	return 0;
} 

// Called on exit
static void cleanup_module(void)
{
	printk(KERN_WARNING "Freeing QoT stack\n");
    device_destroy(cl, dev);
    class_destroy(cl);
    cdev_del(&c_dev);
    unregister_chrdev_region(dev, MINOR_CNT);
}

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Andrew Symington <asymingt@ucla.edu>");
MODULE_DESCRIPTION("QoT ioctl driver");