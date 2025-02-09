// 2025/02/09
// Ross Wen
#include <linux/init.h>
#include <linux/module.h>

#include <linux/moduleparam.h>

#include <linux/fs.h>
#include <linux/types.h>
#include <linux/kdev_t.h>


int dev_major_g = 0;
int dev_amnt_g = 4;


module_param(dev_major_g, int, 0644);
module_param(dev_amnt_g, int, 0644);



int __init scull_init(void)
{
	int ret;
	dev_t tmp_no;


	printk(KERN_ALERT"scull init\n");	
	
	if(dev_major_g == 0)	// dynamically allocate
	{
		ret = alloc_chrdev_region(&tmp_no, 0, dev_amnt_g, "scull");
		dev_major_g = MAJOR(tmp_no);
	}
	else	// statically allocate
	{
		tmp_no = MKDEV(dev_major_g, 0);
		ret = register_chrdev_region(tmp_no, dev_amnt_g, "scull");
	}

	if(ret < 0)
	{
		printk(KERN_ALERT"device number allocation failure\n");
		return ret;
	}

	printk(KERN_ALERT"dev_major_g: %d\n", dev_major_g);
	printk(KERN_ALERT"dev_amnt_g: %d\n", dev_amnt_g);


	return 0;
}


void __exit scull_exit(void)
{
	dev_t tmp_no = MKDEV(dev_major_g, 0);


	printk(KERN_ALERT"scull exit\n");

	unregister_chrdev_region(tmp_no, dev_amnt_g);

	return ;
}


module_init(scull_init);
module_exit(scull_exit);



