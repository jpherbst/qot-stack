#include <linux/idr.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/posix-clock.h>
#include <linux/errno.h>
#include <linux/list.h>
#include <linux/hashtable.h>
#include <linux/types.h>
#include <linux/hash.h>

// Basic kernel module includes
#include <asm/siginfo.h>
#include <linux/rcupdate.h>
#include <linux/sched.h>
#include <linux/clk.h>
#include <linux/string.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/version.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <asm/uaccess.h>
#include <linux/slab.h>

// Expose qot module to userspace through ioctl
#include "qot_ioctl.h"



struct qot_timeline
{
    struct posix_clock clock;
    struct device *dev;
	int references;						// number of references to the timeline
    dev_t devid;
    int index;                      	// index into clocks.map
    int defunct;                    	// tells readers to go away when clock is being removed
    char   uuid[MAX_UUIDLEN];			// unique id for a timeline
    struct hlist_node collision_hash;	// pointer to next timeline for the same hash key
    struct list_head head_acc;			// head pointing to maximum accuracy structure
    struct list_head head_res;			// head pointing to maximum resolution structure
};

struct qot_binding
{
	char   uuid[MAX_UUIDLEN]; 
	uint64_t resolution;				// Desired resolution
	uint64_t accuracy;					// Desired accuracy
    struct list_head res_sort_list;		// Next resolution
    struct list_head acc_sort_list;		// Next accuracy
};

// pointers in an array to a clock binding data
struct qot_binding *binding_map[MAX_BINDINGS];

// binding id for the applications
int next_binding = 0;

/** function calls to create / destroy QoT clock device **/

extern struct qot_timeline *qot_timeline_register(char uuid[]);
extern int qot_timeline_unregister(struct qot_timeline *qotclk);
extern int qot_timeline_index(struct qot_timeline *qotclk);

// Module information
#define MODULE_NAME "qot"
#define FIRST_MINOR 0
#define MINOR_CNT 1
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Fatima Anwar");
MODULE_DESCRIPTION("QoT timelines maintenance");
MODULE_VERSION("0.3.0");

// hashtable for maintaining timelines
DEFINE_HASHTABLE(qot_timelines_hash, MAX_UUIDLEN);

// Required for ioctl
static dev_t dev;
static struct cdev c_dev;
static struct class *cl;

static inline void add_sorted_acc_list(struct qot_binding *binding_list, struct list_head *head)
{
	struct qot_binding *bind_obj;
	// traverse through the entire list against this timeline for proper insertion
	// sorted w.r.t accuracy
	list_for_each_entry(bind_obj, head, acc_sort_list)
	{
		if (bind_obj->accuracy > binding_list->accuracy)
		{
			list_add_tail(&binding_list->acc_sort_list, &bind_obj->acc_sort_list);
			return;
		}
	}

	list_add_tail(&binding_list->acc_sort_list, head); // if its the last element in the list
}

static inline void add_sorted_res_list(struct qot_binding *binding_list, struct list_head *head)
{
	struct qot_binding *bind_obj;
	// traverse through the entire list against this timeline for proper insertion
	// sorted w.r.t accuracy
	list_for_each_entry(bind_obj, head, res_sort_list)
	{
		if (bind_obj->resolution > binding_list->resolution)
		{
			list_add_tail(&binding_list->res_sort_list, &bind_obj->res_sort_list);
			return;
		}
	}
	list_add_tail(&binding_list->res_sort_list, head); // if its the last element in the list
}

static inline void add_new_timeline(char uuid[], struct qot_binding *binding_list)
{
	struct qot_timeline *hash_timeline;
	hash_timeline = qot_timeline_register(uuid);
	if(IS_ERR(hash_timeline))
	{
		hash_timeline = NULL;
		return;
	}
	// add a posix clock for the new timeline as the first element of hash table
	hash_add(qot_timelines_hash, &hash_timeline->collision_hash, (unsigned long)uuid);			
	INIT_LIST_HEAD(&hash_timeline->head_acc); // initialize head for sorted accuracy list
	INIT_LIST_HEAD(&hash_timeline->head_res); // initialize head for sorted accuracy list

	// add the binding to the timeline
	list_add(&binding_list->acc_sort_list, &hash_timeline->head_acc);
	list_add(&binding_list->res_sort_list, &hash_timeline->head_res);
}

static inline struct qot_binding * add_new_binding(qot_clock *clk)
{
	struct qot_binding *binding_list;

	binding_list = kzalloc(sizeof(struct qot_binding), GFP_KERNEL);
	if (binding_list == NULL)
		return ERR_PTR(-ENOMEM);

	strncpy(binding_list->uuid, clk->timeline, MAX_UUIDLEN);
	binding_list->accuracy = clk->accuracy;
	binding_list->resolution = clk->resolution;
	INIT_LIST_HEAD(&binding_list->acc_sort_list);
	INIT_LIST_HEAD(&binding_list->res_sort_list);

	// add binding pointer to the new clock binding
	binding_map[next_binding] = binding_list;

	return binding_list;
}

static inline struct hlist_head *hash_element(char key[])
{
	return &qot_timelines_hash[hash_min((unsigned long)key, HASH_BITS(qot_timelines_hash))];
}

// What to do when the ioctl is started
static int qot_ioctl_open(struct inode *i, struct file *f)
{
	return 0;
}

// What to do when the ioctl is stopped
static int qot_ioctl_close(struct inode *i, struct file *f)
{
	return 0;
}

// What to do when a user queries the ioctl
static long qot_ioctl_access(struct file *f, unsigned int cmd, unsigned long arg)
{
	int bind_id;
	struct qot_binding *current_list;
	struct qot_binding *binding_list;

	struct hlist_head *hash_head;
	struct qot_timeline *obj;
	qot_clock tmp_clk;

	switch (cmd)
	{

	// create a new clock and register a timeline corresponding to it
	case QOT_INIT_CLOCK:

		// Memcpy the current clock parameters into a local structure
		if (copy_from_user(&tmp_clk, (qot_clock*)arg, sizeof(qot_clock)))
			return -EACCES;
		tmp_clk.binding_id = next_binding;

		// make a clock binding object
		binding_list = add_new_binding(&tmp_clk);	

		// get the hash key using uuid for the new object insertion
		hash_head = hash_element(tmp_clk.timeline);

		// check if the hash table is empty for the given key
		if(!hlist_empty(hash_head))
		{

			hlist_for_each_entry(obj, hash_head, collision_hash){
			
			// check if given timeline already exists inside the hashtable
			if(!strcmp(obj->uuid, tmp_clk.timeline))
			{
				// traverse through the entire list against this timeline for proper insertion
				// sorted w.r.t accuracy and resolution
				add_sorted_acc_list(binding_list, &obj->head_acc);
				add_sorted_res_list(binding_list, &obj->head_res);
				break;

			}
			else
			{
				// create a new timeline
				// expose timeline to userspace
				add_new_timeline(tmp_clk.timeline, binding_list);
			}
		}

		} else { // hashtable is emmpty
			// expose new timeline to userspace
			add_new_timeline(tmp_clk.timeline, binding_list);
		}	

		break;

	case QOT_RETURN_BINDING:

		if (copy_to_user((int*)arg, &next_binding, sizeof(int)))
			return -EACCES;
		next_binding = next_binding + 1;

		break;

	case QOT_RELEASE_CLOCK:

		if (copy_from_user(&bind_id, (int*)arg, sizeof(int)))
			return -EACCES;

		// current data pointed to by the bind id
		current_list = binding_map[bind_id];

		// delete list pointers and free up the binding
		list_del(&current_list->acc_sort_list);
		list_del(&current_list->res_sort_list);
		kfree(current_list);

		// get the hash key using uuid of the deleted binding
		hash_head = hash_element(current_list->uuid);

		// find the head for the deleted binding in the hash table
		hlist_for_each_entry(obj, hash_head, collision_hash)
		{
			// if the last binding for that timeline is deleted, delete the timeline and unregister posix clock
			if(!strcmp(obj->uuid, current_list->uuid) && list_empty(&obj->head_acc) && list_empty(&obj->head_res))
			{
				hash_del(&obj->collision_hash);	// delete corresponding hash element
				qot_timeline_unregister(obj); 		// unregister corresponding timeline
				break;
			}
		}

		break;

    // updates the accuracy of a given clock
	case QOT_SET_ACCURACY:

		if (copy_from_user(&tmp_clk, (qot_clock*)arg, sizeof(qot_clock)))
			return -EACCES;

		bind_id = tmp_clk.binding_id;

		// current data pointed to by the bind id
		current_list = binding_map[bind_id];

		// update accuracy in the binding
		current_list->accuracy = tmp_clk.accuracy;

		// reinitialize the binding
		list_del_init(&current_list->acc_sort_list);

		// get the hash key using uuid of updated binding
		hash_head = hash_element(current_list->uuid);

		// find the head for the updated binding in the hash table
		hlist_for_each_entry(obj, hash_head, collision_hash)
		{
			if (!strcmp(obj->uuid, current_list->uuid))
			{
				// sort the bindings according to new accuracy
				add_sorted_acc_list(current_list, &obj->head_acc);
				break;
			}
		}

		break;

    // updates the resolution of a given clock
	case QOT_SET_RESOLUTION:

		if (copy_from_user(&tmp_clk, (qot_clock*)arg, sizeof(qot_clock)))
			return -EACCES;

		bind_id = tmp_clk.binding_id;
                
        // current data pointed to by the bind id
		current_list = binding_map[bind_id];

        // update accuracy in the binding
		current_list->resolution = tmp_clk.resolution;

        // reinitialize the binding
		list_del_init(&current_list->res_sort_list); 

        // get the hash key using uuid of updated binding
		hash_head = hash_element(current_list->uuid);

        // find the head for the updated binding in the hash table
		hlist_for_each_entry(obj, hash_head, collision_hash)
		{
			if (!strcmp(obj->uuid, current_list->uuid))
			{
				// sort the bindings according to new resolution
				add_sorted_res_list(current_list, &obj->head_res);
				break;
			}
		}

		break;

	case QOT_WAIT:
		break;

	case QOT_SET_SCHED:
		break;

	case QOT_GET_SCHED:
		break;

	// Not a valid request
	default:
		return -EINVAL;
	}

	// Success!
	return 0;
}

// Define the file operations over the ioctl
static struct file_operations qot_fops =
{
	.owner = THIS_MODULE,
	.open = qot_ioctl_open,
	.release = qot_ioctl_close,
	.unlocked_ioctl = qot_ioctl_access
};

// Initialise the ioctl
static int qot_ioctl_init(const char *name)
{
	int ret;
	struct device *dev_ret;
	if ((ret = alloc_chrdev_region(&dev, FIRST_MINOR, MINOR_CNT, name)) < 0)
	{
		return ret;
	}
	cdev_init(&c_dev, &qot_fops);
	if ((ret = cdev_add(&c_dev, dev, MINOR_CNT)) < 0)
	{ 
		return ret;
	} 
	if (IS_ERR(cl = class_create(THIS_MODULE, name)))
	{
		cdev_del(&c_dev);
		unregister_chrdev_region(dev, MINOR_CNT);
		return PTR_ERR(cl);
	}
	if (IS_ERR(dev_ret = device_create(cl, NULL, dev, NULL, name)))
	{
		class_destroy(cl);
		cdev_del(&c_dev);
		unregister_chrdev_region(dev, MINOR_CNT);
		return PTR_ERR(dev_ret);
	}
	return 0;
}

// Destroy the IOCTL
static void qot_ioctl_exit(void)
{
	device_destroy(cl, dev);
	class_destroy(cl);
	cdev_del(&c_dev);
	unregister_chrdev_region(dev, MINOR_CNT);
}

static const struct of_device_id qot_dt_ids[] = {
	{ .compatible = "qot", },
  { /* sentinel */ }
};

MODULE_DEVICE_TABLE(of, qot_dt_ids);

// Initialise the kernel driver
static int qot_probe(struct platform_device *pdev)
{
	const struct of_device_id *match;

  	// Try and find a device that matches "compatible: qot"
	match = of_match_device(qot_dt_ids, &pdev->dev);
	//pr_err("of_match_device failed\n");

  	// Init the ioctl
	qot_ioctl_init("qot");

	return 0;
}

// Remove the kernel driver
static int qot_remove(struct platform_device *pdev)
{
  	// Kill the ioctl
	qot_ioctl_exit();

  	// Kill the platform data
	//if (qot_pdata)
	//{
	//TODO: free all clock data
    	// Free the platform data
	//	devm_kfree(&pdev->dev, qot_pdata);
	//	pdev->dev.platform_data = NULL;
	//}

  	// Free the driver data
	platform_set_drvdata(pdev, NULL);

  	// Success!
	return 0;
}

static struct platform_driver qot_driver = {
  .probe    = qot_probe,				/* Called on module init */
  .remove   = qot_remove,				/* Called on module kill */
	.driver   = {
		.name = MODULE_NAME,			
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(qot_dt_ids),
	},
};

module_platform_driver(qot_driver);
