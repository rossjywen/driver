// 2025/02/09
// Ross Wen
#include <linux/init.h>
#include <linux/module.h>

#include <linux/moduleparam.h>

#include <linux/fs.h>
#include <linux/types.h>
#include <linux/kdev_t.h>

#include <linux/cdev.h>

#include <linux/slab.h>

#include <linux/device.h>

#include <linux/uaccess.h>


#define DEV_AMNT	2

#define QUANTUM_SIZE	4000



int dev_major_g = 0;
module_param(dev_major_g, int, 0644);


int scull_open (struct inode *ip, struct file *fp);
ssize_t scull_read(struct file* fp, char __user *user_data, size_t count, loff_t *f_pos);
ssize_t scull_write(struct file *fp, const char __user *user_data, size_t count, loff_t *f_pos);
loff_t scull_llseek (struct file *fp, loff_t off, int whence);
long scull_ioctl(struct file *fp, unsigned int ioctl_num, unsigned long ioctl_param);
int scull_release(struct inode *ip, struct file *fp);


struct file_operations scull_opts = {
	.owner = THIS_MODULE,
	.open = scull_open,
	.read = scull_read,
	.write = scull_write,
	.llseek = scull_llseek,
	.compat_ioctl = scull_ioctl,
	.unlocked_ioctl = scull_ioctl,
	.release = scull_release,
};


// every scull has a scull_dev struct
struct scull_dev {
	void *data;
	long long current_size;
	struct cdev chrdev;
};


struct scull_dev sculls[DEV_AMNT];




int scull_open (struct inode *ip, struct file *fp) {
	struct scull_dev *dev;


	printk(KERN_ALERT "call open()\n");
	dev = container_of(ip->i_cdev, struct scull_dev, chrdev); // or this can be judged by the MINOR devno in inode
	fp->private_data = dev;

	if((fp->f_flags & O_ACCMODE) != O_RDONLY) // if opened with write flags
	{
		if(fp->f_flags & O_TRUNC)
		{
			kfree(dev->data);
			dev->data = 0;
			dev->current_size = 0;
		}
	}

	return 0;
}

ssize_t scull_read(struct file* fp, char __user *user_data, size_t count, loff_t *f_pos) {
	struct scull_dev *dev = fp->private_data;
	loff_t des_addr = *f_pos + count;
	size_t des_count = count;
	int retval;


	printk(KERN_ALERT "call scull_read()\n");

	if(dev->data == NULL)
	{
		printk(KERN_ALERT "in read() hasn't initiated data\n");
		return 0;
	}

	if(des_addr > QUANTUM_SIZE)
	{
		des_count = QUANTUM_SIZE - *f_pos;
		if(des_count == 0)
		{
			printk(KERN_ALERT "in read() reached EOF\n");
			return 0;
		}
	}

	if(copy_to_user(user_data, dev->data + (*f_pos), des_count))
	{
		printk(KERN_ERR "in read() copy_to_user() failed\n");
		retval = -EFAULT;
		goto fail_out;
	}


	*f_pos += des_count;
	printk(KERN_ALERT "f_pos updated as %lld\n", *f_pos);

	return des_count;

fail_out:
	return retval;
}

ssize_t scull_write(struct file *fp, const char __user *user_data, size_t count, loff_t *f_pos) {
	struct scull_dev *dev= fp->private_data;
	loff_t des_addr = *f_pos + count;
	size_t des_count = count;
	int retval;


	printk(KERN_ALERT "call scull_write()\n");
	
	if(dev->data == NULL)	// that means need to allocate memory on data
	{
		dev->data = kmalloc(QUANTUM_SIZE, GFP_KERNEL);
		if(!(dev->data))
		{
			printk(KERN_ERR "in write() not enough memory to allocate\n");
			retval = -ENOMEM;
			goto fail_out;
		}

		printk(KERN_ALERT "in write() init dev->data\n");
	}

	if(des_addr > QUANTUM_SIZE) // that means can not reach the des_addr
	{
		des_count = QUANTUM_SIZE - *f_pos;
		if(des_count == 0) // means has reached the end of file
		{
			printk(KERN_ALERT "in write() reached EOF\n");
			return 0;
		}
	}

	if(copy_from_user(dev->data + (*f_pos), user_data, des_count))
	{
		printk(KERN_ERR "in write() copy_from_user() failed\n");
		retval = -EFAULT;
		goto fail_out;
	}

	*f_pos += des_count;
	printk(KERN_ALERT "f_pos updated as %lld\n", *f_pos);

	return des_count;

fail_out:
	return retval;
}

loff_t scull_llseek (struct file *fp, loff_t off, int whence) {
	return 0;
}

long scull_ioctl(struct file *fp, unsigned int ioctl_num, unsigned long ioctl_param) {
	return 0;
}

int scull_release(struct inode *ip, struct file *fp) {
	printk(KERN_ALERT "call scull_release()\n");
	return 0;
}


int __init scull_init(void)
{
	int ret;
	dev_t tmp_no;
	int i;


	printk(KERN_ALERT "scull init\n");

	// init sculls
	for(i = 0; i < DEV_AMNT; i++)
	{
		sculls[i].data = NULL;
	}
	
	// start allocating device number
	if(dev_major_g == 0)	// dynamically allocate
	{
		ret = alloc_chrdev_region(&tmp_no, 0, DEV_AMNT, "scull");
		dev_major_g = MAJOR(tmp_no);
	}
	else	// statically allocate
	{
		tmp_no = MKDEV(dev_major_g, 0);
		ret = register_chrdev_region(tmp_no, DEV_AMNT, "scull");
	}

	if(ret < 0)
	{
		printk(KERN_ALERT "device number allocation failure\n");
		return ret;
	}

	printk(KERN_ALERT "dev_major_g: %d\n", dev_major_g);
	printk(KERN_ALERT "DEV_AMNT: %d\n", DEV_AMNT);

	//device_destroy(scull_class, MKDEV(dev_major_g, i));

	// start allocating char device in kernel
	for(i = 0; i < DEV_AMNT; i++)
	{
		cdev_init(&(sculls[i].chrdev), &scull_opts);
		sculls[i].chrdev.owner = THIS_MODULE;

		ret = cdev_add(&(sculls[i].chrdev), MKDEV(dev_major_g, i), 1);
		if(ret != 0)
		{
			printk(KERN_ERR "fail to register char device: %d\n", i);
			goto failout;
		}
	}


	return 0;

failout:
	unregister_chrdev_region(MKDEV(dev_major_g, 0), DEV_AMNT);

	return -1;
}


void __exit scull_exit(void)
{
	int i;

	
	for(i = 0; i < DEV_AMNT; i++)
	{
		kfree(sculls[i].data);
	}

	printk(KERN_ALERT "scull exit\n");

	unregister_chrdev_region(MKDEV(dev_major_g, 0), DEV_AMNT);

	for(i = 0; i < DEV_AMNT; i++)
	{
		cdev_del(&(sculls[i].chrdev));
	}


	return ;
}


module_init(scull_init);
module_exit(scull_exit);



