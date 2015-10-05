#include <linux/slab.h>
#include <linux/syscalls.h>
#include <linux/uaccess.h>

#include "qot_ioctl.h"

static dev_t qot_clock_devt;
static struct class *qot_clock_class;

static DEFINE_IDA(qot_clocks_map);

static struct posix_clock_operations qot_clock_ops = {
	.owner			= THIS_MODULE,
	.clock_adjtime	= qot_clock_adjtime,
	.clock_gettime	= qot_clock_gettime,
	.clock_getres	= qot_clock_getres,
	.clock_settime	= qot_clock_settime,
	.ioctl			= qot_clock_ioctl,
	.open			= qot_clock_open,
	.release		= qot_clock_close,
};

/* ioctl implementation */

int qot_clock_open(struct posix_clock *pc, fmode_t fmode)
{
	return 0;
}

int qot_clock_close(struct posix_clock *pc, fmode_t fmode)
{
	return 0;
}

long qot_clock_ioctl(struct posix_clock *pc, unsigned int cmd, unsigned long arg)
{
	struct qot_clock clk;
	struct qot_timeline_data *qotclk = container_of(pc, struct qot_timeline_data, clock);
	struct qot_timeline_data *firstclk;
	int err = 0;

	switch (cmd) {

	case QOT_GET_TIMELINE_ID:
		
		uint16_t timeline_id;

		firstclk = hlist_entry(qotclk->accuracy_sort_hash->next, struct qot_timeline_data, accuracy_sort_hash);
		timeline_id = firstclk->info->timeline;
		if (copy_to_user((uint16_t*)arg, &timeline_id, sizeof(uint16_t)))
			err = -EFAULT;

		break;

	case QOT_GET_ACCURACY:

		struct timespec accuracy;

		firstclk = hlist_entry(qotclk->accuracy_sort_hash->next, struct qot_timeline_data, accuracy_sort_hash);
		accuracy = firstclk->info->accuracy;
		if (copy_to_user((struct timespec*)arg, &accuracy, sizeof(struct timespec)))
			err = -EFAULT;
		
		break;

	case QOT_GET_RESOLUTION:

		struct timespec resolution;

		firstclk = list_entry(qotclk->resolution_sort_list->next, struct qot_timeline_data, resolution_sort_list);
		resolution = firstclk->info->resolution;
		if (copy_to_user((struct timespec*)arg, &resolution, sizeof(struct timespec)))
			err = -EFAULT;

		break;

	default:
		err = -ENOTTY;
		break;
	}

	return err;
}


/* posix clock implementation */

static int qot_clock_getres(struct posix_clock *pc, struct timespec *tp)
{
	return 0;
}

static int qot_clock_settime(struct posix_clock *pc, const struct timespec *tp)
{
	return  0;
}

static int qot_clock_gettime(struct posix_clock *pc, struct timespec *tp)
{
	return 0;
}

static int qot_clock_adjtime(struct posix_clock *pc, struct timex *tx)
{
	return 0;
}

static void delete_qot_clock(struct posix_clock *pc)
{
	struct qot_timeline_data *qotclk = container_of(pc, struct qot_timeline_data, clock);
	ida_simple_remove(&qot_clocks_map, qotclk->index);
	kfree(qotclk);
}

/* public interface */

struct qot_timeline_data *qot_clock_register(struct qot_clock *info, int config)
{
	struct qot_timeline_data *qotclk;
	int err = 0, index, major = MAJOR(qot_clock_devt);

	/* Initialize a clock structure. */
	err = -ENOMEM;
	qotclk = kzalloc(sizeof(struct qot_timeline_data), GFP_KERNEL);
	if (qotclk == NULL)
		goto no_memory;

	qotclk->info = info;

	if(!config)
		goto return_data;

	index = ida_simple_get(&qot_clocks_map, 0, MINORMASK + 1, GFP_KERNEL);
	if (index < 0) {
		err = index;
		goto no_slot;
	}

	qotclk->clock.ops = qot_clock_ops;
	qotclk->clock.release = delete_qot_clock;	
	qotclk->devid = MKDEV(major, index);
	qotclk->index = index;

	/* Create a new device in our class. */
	qotclk->dev = device_create(qot_clock_class, NULL, qotclk->devid, qotclk,
				 "timeline/%d", qotclk->index);
	if (IS_ERR(qotclk->dev))
		goto no_slot;

	dev_set_drvdata(qotclk->dev, qotclk);

	/* Create a posix clock. */
	err = posix_clock_register(&qotclk->clock, qotclk->devid);
	if (err) {
		pr_err("failed to create posix clock\n");
		goto no_slot;
	}

return_data:
	return qotclk;

no_slot:
	kfree(qotclk);
no_memory:
	return ERR_PTR(err);
}
EXPORT_SYMBOL(qot_clock_register);

int qot_clock_unregister(struct qot_timeline_data *qotclk)
{
	qotclk->defunct = 1;

	/* Release the clock's resources. */
	device_destroy(qot_clock_class, qotclk->devid);

	posix_clock_unregister(&qotclk->clock);
	return 0;
}
EXPORT_SYMBOL(qot_clock_unregister);

int qot_clock_index(struct qot_timeline_data *qotclk)
{
	return qotclk->index;
}
EXPORT_SYMBOL(qot_clock_index);

/* module operations */

static void __exit qot_clock_exit(void)
{
	class_destroy(qot_clock_class);
	unregister_chrdev_region(qot_clock_devt, MINORMASK + 1);
	ida_destroy(&qot_clocks_map);
}

static int __init qot_clock_init(void)
{
	int err;

	qot_clock_class = class_create(THIS_MODULE, "qotclk");
	if (IS_ERR(qot_clock_class)) {
		pr_err("qotclk: failed to allocate class\n");
		return PTR_ERR(qot_clock_class);
	}

	err = alloc_chrdev_region(&qot_clock_devt, 0, MINORMASK + 1, "qotclk");
	if (err < 0) {
		pr_err("qotclk: failed to allocate device region\n");
		goto no_region;
	}

	pr_info("QoT clock support registered\n");
	return 0;

no_region:
	class_destroy(qot_clock_class);
	return err;
}

subsys_initcall(qot_clock_init);
module_exit(qot_clock_exit);

MODULE_AUTHOR("Fatima Anwar <fatimanwar@ucla.edu>");
MODULE_DESCRIPTION("QoT timelines support");
MODULE_LICENSE("GPL");
