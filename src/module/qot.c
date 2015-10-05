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

// Expose qot module to userspace through ioctl
#include "qot_ioctl.h"

// Module information
#define MODULE_NAME "qot"
#define FIRST_MINOR 0
#define MINOR_CNT 1
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Fatima Anwar");
MODULE_DESCRIPTION("QoT timelines maintenance");
MODULE_VERSION("0.3.0");

// hashtable for maintaining timelines
DEFINE_HASHTABLE(qot_timelines_hash, ID_LENGTH);

// Maximum number of clock ids and bindind ids supported
#define ID_LENGTH 16

// Required for ioctl
static dev_t dev;
static struct cdev c_dev;
static struct class *cl;

// binding id for the applications
uint16_t bid = 0;

// head for resolution sorting using list
struct list_head head;

// collection of clocks / bindings
struct qot_clock_bindings {
	qot_timeline_data clk[1 << (ID_LENGTH))];
};

// pointers in an array to a clock binding data
struct qot_clock_bindings *bindings = NULL;

static inline void add_sorted_hash_member(qot_clock *new, qot_timeline_data *new_data){
	struct qot_timeline_data *obj;
        hlist_for_each_entry(obj, &qot_timelines_hash[new->timeline], accuracy_sort_hash){
            // manage pointers for accuracy
            if(new->accuracy < obj->info->accuracy && obj->info != NULL){
                hlist_add_before(&new_data->accuracy_sort_hash, &obj->accuracy_sort_hash);
                return;
            }
        }
}

static inline void add_sorted_list_member(qot_clock *new, qot_timeline_data *new_data){
	struct qot_timeline_data *obj;
		hlist_for_each_entry(obj, &qot_timelines_hash[new->timeline], accuracy_sort_hash){
            // manage pointers for resolution
            if(new->resolution < obj->info->resolution && obj->info != NULL){
                    INIT_LIST_HEAD(&new_data->resolution_sort_list);
                    list_add_tail(&new_data->resolution_sort_list, &obj->resolution_sort_list);
                    return;
            }
        }
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
	int err;
	uint16_t bind_id;
	struct qot_timeline_data *timeline_data;
	struct qot_timeline_data *current;
	struct qot_timeline_data *hash_head;
	qot_clock tmp_clk;

	switch (cmd)
	{

	// create a new clock and register a timeline corresponding to it
	case QOT_INIT_CLOCK:

		// Memcpy the current clock parameters into a local structure
		if (copy_from_user(&tmp_clk, (qot_clock*)arg, sizeof(qot_clock)))
			return -EACCES;
		tmp_clk.binding_id = bid;

		// check if the timeline is already in the hashtable
		if(!hlist_empty(&qot_timelines_hash[tmp_clk.timeline])){

			// initialize the clock object
			timeline_data = qot_clock_register(&tmp_clk, 0);
			if(IS_ERR(timeline_data)){
				err = PTR_ERR(timeline_data);
				timeline_data = NULL;
				return err;
			}

			// insert new clock into the list by sorting according to accuracy and resolution
			add_sorted_hash_member(&tmp_clk, timeline_data);
			add_sorted_list_member(&tmp_clk, timeline_data);

		} else {
			// expose timeline to userspace if it's a new timeline
			hash_head = qot_clock_register(NULL, 1);
			if(IS_ERR(hash_head)){
				err = PTR_ERR(hash_head);
				hash_head = NULL;
				return err;
			}
			// add a head as the first element of hash table
			hash_add(qot_timelines_hash, &hash_head->accuracy_sort_hash, tmp_clk.timeline);

			// add the new data to the hashtable, after the hash head
			timeline_data = qot_clock_register(&tmp_clk, 0);
			if(IS_ERR(timeline_data)){
				err = PTR_ERR(timeline_data);
				timeline_data = NULL;
				return err;
			}
			hlist_add_behind(&timeline_data->accuracy_sort_hash, &hash_head->accuracy_sort_hash);

			// list manages resolution sorting
			INIT_LIST_HEAD(&timeline_data->resolution_sort_list);
			list_add(&timeline_data->resolution_sort_list, head);
		}

		// add binding pointer to the new clock
        &(bindings->clk[bid]) = timeline_data;

		break;

	case QOT_RETURN_BINDING:

		if (copy_to_user((uint16_t*)arg, &bid, sizeof(uint16_t)))
				return -EACCES;
		bid = bid + 1;

		break;

	case QOT_RELEASE_CLOCK:

		if (copy_from_user(&bind_id, (uint16_t*)arg, sizeof(uint16_t)))
			return -EACCES;

		// current data pointed to by the bind id
		current = &(bindings->clk[bind_id]);

		// sanity check for binding id
		if(bind_id != current->info->binding_id)
			return -1;

		// delete accuracy pointers of hashtable entry
	    hlist_node *node = &current->accuracy_sort_hash;
		hlist_del_init(node);

		// delete resolution pointers of hashtable entry
		list_del(&current->resolution_sort_list);
		kfree(current);

		// bucket to which the list element belongs
		struct hlist_head *bkt = &qot_timelines_hash[current->info->timeline];

		// check if it was the last element in the hash list
		// if yes, free up the hash head and unregister timeline
		if(!bkt->first->next){
			struct qot_timeline_data *obj;
			obj = hlist_entry(bkt->first, struct qot_timeline_data, accuracy_sort_hash)		
			hash_del(obj->accuracy_sort_hash);		// free up hash head
			list_del(obj->resolution_sort_list);	// free up hash head's list
			qot_clock_unregister(obj);				// unregister the timeline				
		}
		
		break;

    // updates the accuracy of a given clock
  	case QOT_SET_ACCURACY:

 		if (copy_from_user(&tmp_clk, (qot_clock*)arg, sizeof(qot_clock)))
			return -EACCES;

		bind_id = tmp_clk.binding_id;
		// current data pointed to by the bind id
        current = &(bindings->clk[bind_id]);

        // sanity check for binding id
		if(bind_id != current->info->binding_id)
			return -1;

		current->info->accuracy = tmp_clk.accuracy;
		
		// sort the hash list according to new accuracy
		hlist_del_init(&current->accuracy_sort_hash); // it reinitializes the pointers
		add_sorted_hash_member(current->info, current);

      	break;

    // updates the resolution of a given clock
	case QOT_SET_RESOLUTION:

        if (copy_from_user(&tmp_clk, (qot_clock*)arg, sizeof(qot_clock)))
			return -EACCES;

		bind_id = tmp_clk.binding_id;
		// current data pointed to by the bind id
        current = &(bindings->clk[bind_id]);

        // sanity check for binding id
		if(bind_id != current->info->binding_id)
			return -1;

        current->info->resolution = resolution;
                
        // sort the list according to new resolution
        list_del(&current->resolution_sort_list);
        hlist_del_init(&current->resolution_sort_list); // to reinitialize the pointers
        add_sorted_list_member(current->info, current);

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
	struct pinctrl *pinctrl;
	int i = 0;

  	// Try and find a device that matches "compatible: qot"
	match = of_match_device(qot_dt_ids, &pdev->dev);
	//pr_err("of_match_device failed\n");

	// Initialise the clock and timeline list head
	INIT_LIST_HEAD(&head);

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
	if (qot_pdata)
	{
	//TODO: free all clock data
    	// Free the platform data
		devm_kfree(&pdev->dev, qot_pdata);
		pdev->dev.platform_data = NULL;
	}

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
