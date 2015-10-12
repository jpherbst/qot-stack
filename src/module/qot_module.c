#include <linux/version.h>
#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/list.h>
#include <linux/hashtable.h>
#include <linux/dcache.h>
#include <asm/uaccess.h>

// Expose  module to userspace (ioctl) and other modules (export)
#include "qot_ioctl.h"
#include "qot_clock.h"

// Module information
#define MODULE_NAME "qot"
#define FIRST_MINOR 0
#define MINOR_CNT 1

// Stores information about a timeline
struct qot_timeline {
	struct posix_clock clock;			// posix clock
    char uuid[QOT_MAX_UUIDLEN];			// unique id for a timeline
    struct hlist_node collision_hash;	// pointer to next timeline for the same hash key
    struct list_head head_acc;			// head pointing to maximum accuracy structure
    struct list_head head_res;			// head pointing to maximum resolution structure
};

// Stores information about an application binding to a timeline
struct qot_binding {
	uint64_t res;						// Desired resolution
	uint64_t acc;						// Desired accuracy
    struct list_head res_list;			// Next resolution
    struct list_head acc_list;			// Next accuracy
    struct qot_timeline* timeline;		// Parent timeline
};

// An array of bindings (one for each application request)
static struct qot_binding *bindings[QOT_MAX_BINDINGS+1];

// The next available binding ID, as well as the total number alloacted
static int binding_next = 0;
static int binding_size = 0;

// A hash table designed to quickly find timeline based on UUID
DEFINE_HASHTABLE(qot_timelines_hash, QOT_HASHTABLE_BITS);

// DATA STRUCTURE MANAGEMENT ///////////////////////////////////////////////////

// Generate an unsigned int hash from a null terminated string
static unsigned int qot_hash_uuid(char *uuid)
{
	unsigned long hash = init_name_hash();
	while (*uuid)
		hash = partial_name_hash(*uuid++, hash);
	return end_name_hash(hash);
}

static inline void qot_add_acc(struct qot_binding *binding_list, struct list_head *head)
{
	struct qot_binding *bind_obj;
	list_for_each_entry(bind_obj, head, acc_list)
	{
		if (bind_obj->acc > binding_list->acc)
		{
			list_add_tail(&binding_list->acc_list, &bind_obj->acc_list);
			return;
		}
	}
	list_add_tail(&binding_list->acc_list, head);
}

static inline void qot_add_res(struct qot_binding *binding_list, struct list_head *head)
{
	struct qot_binding *bind_obj;
	list_for_each_entry(bind_obj, head, res_list)
	{
		if (bind_obj->res > binding_list->res)
		{
			list_add_tail(&binding_list->res_list, &bind_obj->res_list);
			return;
		}
	}
	list_add_tail(&binding_list->res_list, head);
}

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
	qot_message msg;			// ioctl message
	struct qot_timeline *obj;	// timeline object
	unsigned int key;			// hash key

	switch (cmd)
	{

	case QOT_BIND_TIMELINE:
	
		// Get the parameters passed into the ioctl
		if (copy_from_user(&msg, (qot_message*)arg, sizeof(qot_message)))
			return -EACCES;

		// Allocate memory for the new binding
		bindings[binding_next] = kzalloc(sizeof(struct qot_binding), GFP_KERNEL);
		if (bindings[binding_next] == NULL)
			return -ENOMEM;

		// Find the next available binding (not necessarily contiguous)
		for (msg.bid = binding_next; binding_next <= QOT_MAX_BINDINGS; binding_next++)
			if (!bindings[binding_next])
				break;

		// Make sure we have the binding size
		if (binding_size < binding_next)
			binding_size = binding_next;		

		// Copy over data accuracy and resolution requirements
		bindings[msg.bid]->acc = msg.acc;
		bindings[msg.bid]->res = msg.res;

		// Generate a key for the UUID
		key = qot_hash_uuid(msg.uuid);

		// Presume non-existent until found
		bindings[msg.bid]->timeline = NULL;

		// Check to see if this hash element exists
		hash_for_each_possible(qot_timelines_hash, obj, collision_hash, key)
			if (!strcmp(obj->uuid, msg.uuid))
				bindings[msg.bid]->timeline = obj;

		// If the hash element does not exist, add it
		if (!bindings[msg.bid]->timeline)
		{
			// If we get to this point, there is no corresponding UUID in the hash
			bindings[msg.bid]->timeline = kzalloc(sizeof(struct qot_timeline), GFP_KERNEL);
			
			// Copy over the UUID
			strncpy(bindings[msg.bid]->timeline->uuid, msg.uuid, QOT_MAX_UUIDLEN);

			// TODO: Create the POSIX clock

			// Add the timeline
			hash_add(qot_timelines_hash, &bindings[msg.bid]->timeline->collision_hash, key);

			// Initialize list heads
			INIT_LIST_HEAD(&bindings[msg.bid]->timeline->head_acc);
			INIT_LIST_HEAD(&bindings[msg.bid]->timeline->head_res);
		}

		// In both cases, we must insert and sort the two linked lists
		qot_add_acc(bindings[msg.bid], &bindings[msg.bid]->timeline->head_acc);
		qot_add_res(bindings[msg.bid], &bindings[msg.bid]->timeline->head_res);

		// Send back the data structure with the new binding and clock info
		if (copy_to_user((qot_message*)arg, &msg, sizeof(qot_message)))
			return -EACCES;

		break;

	case QOT_UNBIND_TIMELINE:

		// Get the parameters passed into the ioctl
		if (copy_from_user(&msg, (qot_message*)arg, sizeof(qot_message)))
			return -EACCES;

		// Sanity check
		if (msg.bid < 0 || msg.bid > QOT_MAX_BINDINGS || !bindings[msg.bid])
			return -EACCES;

		// Remove the entries from the sort tables (ordering will be updated)
		list_del(&bindings[msg.bid]->res_list);
		list_del(&bindings[msg.bid]->acc_list);

		// If we have removed the last binding from a timeline, then the lists
		// associated with the timeline should both be empty
		if (	list_empty(&bindings[msg.bid]->timeline->head_acc) 
			&&  list_empty(&bindings[msg.bid]->timeline->head_res))
		{
			// Remove element from the hashmap
			hash_del(&bindings[msg.bid]->timeline->collision_hash);

			// TODO: remove the POSIX clock

			// Free the timeline entry memory
			kfree(bindings[msg.bid]->timeline);
		}

		// Free the memory used by this binding
		kfree(bindings[msg.bid]);

		// Set to null to indicate availability
		bindings[msg.bid] = NULL;

		// Indicate that we now have a free binding
		binding_next = msg.bid;

		break;

	case QOT_GET_ACHIEVED:

		// NOT IMPLEMENTED YET

		break;

	case QOT_SET_ACCURACY:

		// Get the parameters passed into the ioctl
		if (copy_from_user(&msg, (qot_message*)arg, sizeof(qot_message)))
			return -EACCES;

		// Sanity check
		if (msg.bid < 0 || msg.bid > QOT_MAX_BINDINGS || !bindings[msg.bid])
			return -EACCES;

		// Copy over the accuracy
		bindings[msg.bid]->acc = msg.acc;

		// Delete the item
		list_del(&bindings[msg.bid]->acc_list);

		// Add the item back
		qot_add_acc(bindings[msg.bid], &bindings[msg.bid]->timeline->head_acc);

		break;

	case QOT_SET_RESOLUTION:

		// Get the parameters passed into the ioctl
		if (copy_from_user(&msg, (qot_message*)arg, sizeof(qot_message)))
			return -EACCES;

		// Sanity check
		if (msg.bid < 0 || msg.bid > QOT_MAX_BINDINGS || !bindings[msg.bid])
			return -EACCES;

		// Copy over the resolution
		bindings[msg.bid]->res = msg.res;

		// Delete the item
		list_del(&bindings[msg.bid]->res_list);

		// Add the item back
		qot_add_res(bindings[msg.bid], &bindings[msg.bid]->timeline->head_res);

		break;

	case QOT_WAIT_UNTIL:

		// NOT IMPLEMENTED YET

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
int init_module(void)
{
	int ret;
    struct device *dev_ret;

	// Intiialize our timeline hash table
	hash_init(qot_timelines_hash);

	// Initialize ioctl
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
 	
    // SUCCESS
 	return 0;
} 

// Called on exit
void cleanup_module(void)
{
	// Stop ioctl
	printk(KERN_WARNING "Freeing QoT stack\n");
    device_destroy(cl, dev);
    class_destroy(cl);
    cdev_del(&c_dev);
    unregister_chrdev_region(dev, MINOR_CNT);
}

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Andrew Symington <asymingt@ucla.edu>");
MODULE_DESCRIPTION("QoT ioctl driver");