
#include <linux/cdev.h>
#include <linux/io.h>
#include <asm/cacheflush.h>
#include <asm/outercache.h>
#include <linux/ioctl.h>
#include <linux/module.h>         // Core header for loading LKMs into the kernel
#include <linux/device.h>         // Header to support the kernel Driver Model

#include "dma_parameters.h"



struct device_data{
	int    major_number;           	///< Stores the device number -- determined automatically
	char   message[256];           	///< Memory for the string that is passed from userspace
	short  size_of_message;					///< Used to remember the size of the string stored
	int    numberOpens;            	///< Counts the number of times the device is opened
	struct class*  p_device_class; 	///< The device-driver class struct pointer
	struct device* p_device; 				///< The device-driver device struct pointer
	struct cdev cdev;
	struct dma_data *p_dma_data;
};

static struct device_data dma_channel[2]; //0=send 1=recieve

static int     dev_open(struct inode *, struct file *);
static int     dev_release(struct inode *, struct file *);
static ssize_t dev_read(struct file *, char *, size_t, loff_t *);
static ssize_t dev_write(struct file *, const char *, size_t, loff_t *);

// Maps physical send or recieve adresses to user space
static int dev_mmap(struct file *file_p, struct vm_area_struct *vma){
	struct inode *inode = (struct inode *)file_p->private_data;

	if(imajor(inode)==dma_channel[0].major_number){//send
		remap_pfn_range(vma, vma->vm_start, SEND_PHYS_ADDR>>PAGE_SHIFT, vma->vm_end-vma->vm_start, vma->vm_page_prot);
	}
	else{//recieve
		remap_pfn_range(vma, vma->vm_start, RECIEVE_PHYS_ADDR>>PAGE_SHIFT, vma->vm_end-vma->vm_start, vma->vm_page_prot);
	}
	return 0;
}

// Clean send and recieve caches, or invalidate recieve buffer
static long dev_ioctl(struct file* file, unsigned int cmd, unsigned long arg){
	if(cmd==0){//Clean send cache
		__cpuc_flush_dcache_area(dma_channel[0].p_dma_data, sizeof(struct dma_data));
		__cpuc_flush_dcache_area(dma_channel[1].p_dma_data, sizeof(struct dma_data));
		outer_clean_range(SEND_PHYS_ADDR, 0x19000000);
	}
	else{//Invalidate recieve
		outer_inv_range(RECIEVE_PHYS_ADDR, 19000000);
		__cpuc_flush_dcache_area(dma_channel[1].p_dma_data, sizeof(struct dma_data));
	}
	return 0;
}

static struct file_operations fops = {
  .open = dev_open,
  .read = dev_read,
  .write = dev_write,
  .release = dev_release,
  .mmap = dev_mmap,
	.unlocked_ioctl = dev_ioctl
};

// Initialize both recieve and send devices
static int __init init_channel(struct device_data *dma_chan, char *name){
	int error_message;

	char dev_name[32] = DEVICE_NAME;
	strcat(dev_name, name);

	char class_name[32] = DEVICE_NAME;
	strcat(class_name, name);

	cdev_init(&dma_chan->cdev, &fops);
	dma_chan->cdev.owner = THIS_MODULE;

	error_message = cdev_add(&dma_chan->cdev, MKDEV(dma_chan->major_number, 0), 1);
	if (error_message) {
		dev_err(dma_chan->p_device, "unable to add char device\n");
	}

	dma_chan->major_number = register_chrdev(0, dev_name, &fops);
	if (dma_chan->major_number<0){
		printk(KERN_ALERT "Device failed to register a major number\n");
		return dma_chan->major_number;
	}

	dma_chan->p_device_class = class_create(THIS_MODULE, class_name);
	if (IS_ERR(dma_chan->p_device_class)){                // Check for error and clean up if there is
		unregister_chrdev(dma_chan->major_number, dev_name);
		printk(KERN_ALERT "Failed to register device class\n");
		return PTR_ERR(dma_chan->p_device_class);          // Correct way to return an error on a pointer
	}

	dma_chan->p_device = device_create(dma_chan->p_device_class, NULL, MKDEV(dma_chan->major_number, 0), NULL, dev_name);
	if (IS_ERR(dma_chan->p_device)){               // Clean up if there is an error
		class_destroy(dma_chan->p_device_class);           // Repeated code but the alternative is goto statements
		unregister_chrdev(dma_chan->major_number, dev_name);
		printk(KERN_ALERT "Failed to create the device\n");
		return PTR_ERR(dma_chan->p_device);
	}
	printk(KERN_INFO "Device registered correctly with major number %d\n", dma_chan->major_number);


	// Map memory to kernel space
	if(strcmp(name, "send")==0){
		dma_chan->p_dma_data = (struct dma_data *)memremap(SEND_PHYS_ADDR, sizeof(struct dma_data), MEMREMAP_WB);
	}
	else{
		dma_chan->p_dma_data = (struct dma_data *)memremap(RECIEVE_PHYS_ADDR, sizeof(struct dma_data), MEMREMAP_WB);
	}

	if(dma_chan->p_dma_data==NULL){
		printk(KERN_INFO "Failed to map memory to kernel module\n");
	}
	else{
		printk(KERN_INFO "Device allocated memory at 0x%08X\n", (unsigned int)dma_chan->p_dma_data);
	}
	return 0;
}

static int __init ebbchar_init(void){
	int ret;

	ret = init_channel(&dma_channel[0], "send");

	if (ret) {
		return ret;
	}

	ret = init_channel(&dma_channel[1], "recieve");
	if (ret) {
		return ret;
	}

	return 0;
}

// Cleanup
static void __exit ebbchar_exit(void){
  device_destroy(dma_channel[0].p_device_class, MKDEV(dma_channel[0].major_number, 0));     // remove the device
  class_unregister(dma_channel[0].p_device_class);                          // unregister the device class
  class_destroy(dma_channel[0].p_device_class);                             // remove the device class
  unregister_chrdev(dma_channel[0].major_number, "ebbcharsend");             // unregister the major number
	cdev_del(&dma_channel[0].cdev);

	device_destroy(dma_channel[1].p_device_class, MKDEV(dma_channel[1].major_number, 0));     // remove the device
  class_unregister(dma_channel[1].p_device_class);                          // unregister the device class
  class_destroy(dma_channel[1].p_device_class);                             // remove the device class
  unregister_chrdev(dma_channel[1].major_number, "ebbcharrecieve");             // unregister the major number
	cdev_del(&dma_channel[1].cdev);
  printk(KERN_INFO "EBBChar: Goodbye from the LKM!\n");
}

static int dev_open(struct inode *inodep, struct file *filep){
	// printk(KERN_INFO "iminor=%d\n", iminor(inodep));
	// printk(KERN_INFO "iminor=%d\n", imajor(inodep));
	filep->private_data = inodep;
	return 0;
}


static ssize_t dev_read(struct file *filep, char *buffer, size_t len, loff_t *offset){
	return 0;
}

static ssize_t dev_write(struct file *filep, const char *buffer, size_t len, loff_t *offset){
	return 0;
}

// TODO Figure out what this funct is doing
static int dev_release(struct inode *inodep, struct file *filep){
  printk(KERN_INFO "EBBChar: Device successfully closed\n");
  return 0;
}

module_init(ebbchar_init);
module_exit(ebbchar_exit);
MODULE_LICENSE("GPL");
