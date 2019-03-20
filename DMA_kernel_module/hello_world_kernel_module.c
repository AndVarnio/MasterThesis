// hello.c
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/dma-mapping.h>
#include <linux/dmaengine.h>
#include <linux/module.h>
#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/dma-mapping.h>
#include <linux/slab.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/workqueue.h>
#include "hello_world_kernel_module.h"


#define CACHED_BUFFERS
// #define INTERNAL_TEST

/* The following module parameter controls where the allocated interface memory area is cached or not
 * such that both can be illustrated.  Add cached_buffers=1 to the command line insert of the module
 * to cause the allocated memory to be cached.
 */
static unsigned cached_buffers = 0;
module_param(cached_buffers, int, S_IRUGO);

MODULE_LICENSE("GPL");

#define DRIVER_NAME 		"dma_proxy"
#define CHANNEL_COUNT 		2
#define ERROR 			-1
#define NOT_LAST_CHANNEL 	0
#define LAST_CHANNEL 		1

#define  DEVICE_NAME "cubedmachar"    ///< The device will appear at /dev/ebbchar using this value
#define  CLASS_NAME  "cubedma"        ///< The device class -- this is a character device driver

/* The following data structure represents a single channel of DMA, transmit or receive in the case
 * when using AXI DMA.  It contains all the data to be maintained for the channel.
 */
struct dma_proxy_channel {
	struct dma_proxy_channel_interface *interface_p;	/* user to kernel space interface */
	dma_addr_t interface_phys_addr;

	struct device *proxy_device_p;				/* character device support */
	struct device *dma_device_p;
	dev_t dev_node;
	struct cdev cdev;
	struct class *class_p;

	struct dma_chan *channel_p;				/* dma support */
	struct completion cmp;
	dma_cookie_t cookie;
	dma_addr_t dma_handle;
	u32 direction;						/* DMA_MEM_TO_DEV or DMA_DEV_TO_MEM */
};

static struct dma_proxy_channel pchannel_p;



static int local_open(struct inode *ino, struct file *file)
{
	//file->private_data = container_of(ino->i_cdev, struct dma_proxy_channel, cdev);

	return 0;
}

/* Close the file and there's nothing to do for it
 */
static int release(struct inode *ino, struct file *file)
{
	return 0;
}

/* Perform I/O control to start a DMA transfer.
 */
static long ioctl(struct file *file, unsigned int unused , unsigned long arg)
{

	return 0;
}

/* Map the memory for the channel interface into user space such that user space can
 * access it taking into account if the memory is not cached.
 */
static int mmap(struct file *file_p, struct vm_area_struct *vma)
{
	return 0;
}

static struct file_operations dm_fops = {
	.owner    = THIS_MODULE,
	.open     = local_open,
	.release  = release,
	.unlocked_ioctl = ioctl,
	.mmap	= mmap
};

static int __init init_kernel_module(void) {
    printk(KERN_INFO "Hello world kernel.\n");

    int rc;

    char device_name[32] = "dma_proxy";
  	static struct class *local_class_p = NULL;

  	/* Allocate a character device from the kernel for this
  	 * driver
  	 */
  	rc = alloc_chrdev_region((dev_t*)pchannel_p.dev_node, 0, 1, "dma_proxy");

  	if (rc) {
  		dev_err(pchannel_p.dma_device_p, "unable to get a char device number\n");
  		return rc;
  	}

    /* Initialize the ter device data structure before
  	 * registering the character device with the kernel
  	 */
  	cdev_init((struct cdev*)pchannel_p.cdev, &dm_fops);
  	pchannel_p.cdev.owner = THIS_MODULE;
  	rc = cdev_add((struct cdev*)pchannel_p.cdev, pchannel_p.dev_node, 1);

  	if (rc) {
  		dev_err((struct device*)pchannel_p.dma_device_p, "unable to add char device\n");
  		goto init_error1;
  	}

   //  // Register the device class
   // local_class_p = class_create(THIS_MODULE, CLASS_NAME);
   // if (IS_ERR(local_class_p)){                // Check for error and clean up if there is
   //    // unregister_chrdev(majorNumber, DEVICE_NAME);
   //    printk(KERN_ALERT "Failed to register device class\n");
   //    return PTR_ERR(local_class_p);          // Correct way to return an error on a pointer
   // }
   // printk(KERN_INFO "EBBChar: device class registered correctly\n");
   //
   //  // Register the device driver
   // pchannel_p->dma_device_p = device_create(local_class_p, NULL, pchannel_p->dev_node, NULL, DEVICE_NAME);
   // if (IS_ERR(pchannel_p->dma_device_p)){               // Clean up if there is an error
   //    class_destroy(local_class_p);           // Repeated code but the alternative is goto statements
   //    unregister_chrdev(pchannel_p->dev_node, DEVICE_NAME);
   //    printk(KERN_ALERT "Failed to create the device\n");
   //    return PTR_ERR(pchannel_p->dma_device_p);
   // }
   //
   //
   // pchannel_p->proxy_device_p = device_create(local_class_p, NULL,
   //                        pchannel_p->dev_node, NULL, device_name);
   //
   // if (IS_ERR(pchannel_p->proxy_device_p)) {
   //   dev_err(pchannel_p->dma_device_p, "unable to create the device\n");
   //   goto init_error3;
   // }

   // pchannel_p->interface_p = (struct dma_proxy_channel_interface *)
   //   kzalloc(sizeof(struct dma_proxy_channel_interface),
   //       GFP_KERNEL);
   // printk(KERN_INFO "Allocating cached memory at 0x%08X\n",
   //      (unsigned int)pchannel_p->interface_p);

    return 0;

    init_error3:
    	class_destroy(pchannel_p.class_p);

    init_error2:
    	cdev_del(&pchannel_p.cdev);

    init_error1:
    	unregister_chrdev_region(pchannel_p.dev_node, 1);
    	return rc;

}

static void __exit cleanup_kernel_module(void) {
    printk(KERN_INFO "Goodbye world kernel.\n");
}

module_init(init_kernel_module);
module_exit(cleanup_kernel_module);
MODULE_LICENSE("GPL");
