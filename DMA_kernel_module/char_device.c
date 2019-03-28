/* @author Derek Molloy
* @date   7 April 2015
* @version 0.1
* @brief   An introductory character driver to support the second article of my series on
* Linux loadable kernel module (LKM) development. This module maps to /dev/ebbchar and
* comes with a helper C program that can be run in Linux user space to communicate with
* this the LKM.
* @see http://www.derekmolloy.ie/ for a full description and follow-up descriptions.
*/
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
#include <asm/uaccess.h>
#include <linux/io.h>
#include "dma_proxy.h"
#include <asm/cacheflush.h>
#include <asm/outercache.h>
#include <linux/ioctl.h>
#include <asm/page.h>

#include <linux/init.h>           // Macros used to mark up functions e.g. __init __exit
#include <linux/module.h>         // Core header for loading LKMs into the kernel
#include <linux/device.h>         // Header to support the kernel Driver Model
#include <linux/kernel.h>         // Contains types, macros, functions for the kernel
#include <linux/fs.h>             // Header for the Linux file system support
#include <linux/uaccess.h>          // Required for the copy to user function
#define  DEVICE_NAME "ebbchar"    ///< The device will appear at /dev/ebbchar using this value
#define  CLASS_NAME  "ebb"        ///< The device class -- this is a character device driver

MODULE_LICENSE("GPL");            ///< The license type -- this affects available functionality
MODULE_AUTHOR("Derek Molloy");    ///< The author -- visible when you use modinfo
MODULE_DESCRIPTION("A simple Linux char driver for the BBB");  ///< The description -- see modinfo
MODULE_VERSION("0.1");            ///< A version number to inform users

struct channel_data{
	int    majorNumber;                  ///< Stores the device number -- determined automatically
	char   message[256];           ///< Memory for the string that is passed from userspace
	short  size_of_message;              ///< Used to remember the size of the string stored
	int    numberOpens;              ///< Counts the number of times the device is opened
	struct class*  ebbcharClass; ///< The device-driver class struct pointer
	struct device* ebbcharDevice; ///< The device-driver device struct pointer
	struct cdev cdev;
	struct dma_proxy_channel_interface *interface_p;
	dma_addr_t interface_phys_addr;
};

static struct channel_data dma_channel[2]; //0=send 1=recieve

// The prototype functions for the character driver -- must come before the struct definition
static int     dev_open(struct inode *, struct file *);
static int     dev_release(struct inode *, struct file *);
static ssize_t dev_read(struct file *, char *, size_t, loff_t *);
static ssize_t dev_write(struct file *, const char *, size_t, loff_t *);



static int mmap(struct file *file_p, struct vm_area_struct *vma)
{

printk(KERN_INFO "Remapping\n");
	/* The virtual address to map into is good, but the page frame will not be good since
	 * user space passes a physical address of 0, so get the physical address of the buffer
	 * that was allocated and convert to a page frame number.
	 */

	 struct inode *inode = (struct inode *)file_p->private_data;
	 printk(KERN_INFO "ebbcharClass: %d, allocating: %ld Bytes\n", imajor(inode), vma->vm_end - vma->vm_start);


	 if(imajor(inode)==dma_channel[0].majorNumber){//send
		 remap_pfn_range(vma, vma->vm_start,
								0x09000000>>PAGE_SHIFT,
								vma->vm_end - vma->vm_start, vma->vm_page_prot);
	 }
		else{//reieve

			remap_pfn_range(vma, vma->vm_start,
 								0x0ffff000>>PAGE_SHIFT,
 								vma->vm_end - vma->vm_start, vma->vm_page_prot);

		}

	 // printk(KERN_INFO "EBBChar: vma=%p, vma->vm_start=%ld, virt_to_phys((void *)dma_chan->interface_p=0x%08x, vma->vm_end - vma->vm_start=%ld\n", vma, vma->vm_start, virt_to_phys((void *)dma_chan->interface_p)>>PAGE_SHIFT, vma->vm_end - vma->vm_start);
	 //
	 //
	 // if(dma_chan!=NULL){
		//  remap_pfn_range(vma, vma->vm_start,
 		// 					virt_to_phys((void *)dma_chan->interface_p)>>PAGE_SHIFT,
 		// 					vma->vm_end - vma->vm_start, vma->vm_page_prot);
	 // }
		// else{
		// 	printk(KERN_INFO "EBBChar: mmap NULL pointer \n");
		// }


}

/* Perform I/O control to start a DMA transfer.
 */

// static long ioctl(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg)
static long ioctl(struct file* file, unsigned int cmd, unsigned long arg)
{
	printk(KERN_INFO "EBBChar: cmd=%d, arg=%ld, sizeof(struct dma_proxy_channel_interface)=%d \n", cmd, arg, sizeof(struct dma_proxy_channel_interface));

	if(cmd==0){//Clean send cache
		__cpuc_flush_dcache_area(dma_channel[0].interface_p, sizeof(struct dma_proxy_channel_interface));
		__cpuc_flush_dcache_area(dma_channel[1].interface_p, sizeof(struct dma_proxy_channel_interface));
		outer_clean_range(0x09000000, 0x19000000);
	}
	else{//Invalidate recieve
		outer_inv_range(0x0ffff000, 19000000);
		__cpuc_flush_dcache_area(dma_channel[1].interface_p, sizeof(struct dma_proxy_channel_interface));
	}
	return 0;
}

/** @brief Devices are represented as file structure in the kernel. The file_operations structure from
*  /linux/fs.h lists the callback functions that you wish to associated with your file operations
*  using a C99 syntax structure. char devices usually implement open, read, write and release calls
*/
static struct file_operations fops =
{
  .open = dev_open,
  .read = dev_read,
  .write = dev_write,
  .release = dev_release,
  .mmap = mmap,
	.unlocked_ioctl = ioctl
};

/** @brief The LKM initialization function
*  The static keyword restricts the visibility of the function to within this C file. The __init
*  macro means that for a built-in driver (not a LKM) the function is only used at initialization
*  time and that it can be discarded and its memory freed up after that point.
*  @return returns 0 if successful
*/
static int __init init_channel(struct channel_data *dma_chan, char *name){
  printk(KERN_INFO "EBBChar: Initializing the EBBChar LKM\n");

	char dev_name[32] = DEVICE_NAME;
	strcat(dev_name, name);

	char class_name[32] = DEVICE_NAME;
	strcat(class_name, name);


	cdev_init(&dma_chan->cdev, &fops);
	dma_chan->cdev.owner = THIS_MODULE;
	int rc = cdev_add(&dma_chan->cdev, MKDEV(dma_chan->majorNumber, 0), 1);

	if (rc) {
		dev_err(dma_chan->ebbcharDevice, "unable to add char device\n");
	}

  // Try to dynamically allocate a major number for the device -- more difficult but worth it
  dma_chan->majorNumber = register_chrdev(0, dev_name, &fops);
  if (dma_chan->majorNumber<0){
     printk(KERN_ALERT "EBBChar failed to register a major number\n");
     return dma_chan->majorNumber;
  }
  printk(KERN_INFO "EBBChar: registered correctly with major number %d\n", dma_chan->majorNumber);

  // Register the device class
  dma_chan->ebbcharClass = class_create(THIS_MODULE, class_name);
  if (IS_ERR(dma_chan->ebbcharClass)){                // Check for error and clean up if there is
     unregister_chrdev(dma_chan->majorNumber, dev_name);
     printk(KERN_ALERT "Failed to register device class\n");
     return PTR_ERR(dma_chan->ebbcharClass);          // Correct way to return an error on a pointer
  }
  printk(KERN_INFO "EBBChar: device class registered correctly\n");

  // Register the device driver

  dma_chan->ebbcharDevice = device_create(dma_chan->ebbcharClass, NULL, MKDEV(dma_chan->majorNumber, 0), NULL, dev_name);
  if (IS_ERR(dma_chan->ebbcharDevice)){               // Clean up if there is an error
     class_destroy(dma_chan->ebbcharClass);           // Repeated code but the alternative is goto statements
     unregister_chrdev(dma_chan->majorNumber, dev_name);
     printk(KERN_ALERT "Failed to create the device\n");
     return PTR_ERR(dma_chan->ebbcharDevice);
  }
  printk(KERN_INFO "EBBChar: device class created correctly\n"); // Made it! device was initialized

 //  interface_p = (struct dma_proxy_channel_interface *)
 //   kzalloc(sizeof(struct dma_proxy_channel_interface),
 //       GFP_KERNEL);
 // printk(KERN_INFO "Allocating cached memory at 0x%08X\n",
 //      (unsigned int)interface_p);


		// 	dma_set_coherent_mask(ebbcharDevice, 0xFFFFFFFF);
		// 	interface_p = (struct dma_proxy_channel_interface *)
		// 		dmam_alloc_coherent(ebbcharDevice,
		// 							sizeof(struct dma_proxy_channel_interface),
		// 							&interface_phys_addr, GFP_KERNEL);
		// 	printk(KERN_INFO "Allocating uncached memory at 0x%08X\n",
		// 			 (unsigned int)interface_p);
		//
		// if (!interface_p) {
		// 	dev_err(ebbcharDevice, "DMA allocation error\n");
		// 	return -1;
		// }

		if(strcmp(name, "send")==0){
			dma_chan->interface_p = (struct dma_proxy_channel_interface *)memremap(0x09000000, sizeof(struct dma_proxy_channel_interface), MEMREMAP_WB);
			printk(KERN_INFO"Actual phys: 0x%08X\n", virt_to_phys((void *)dma_chan->interface_p)>>PAGE_SHIFT);
			// uint32_t i;
			// for (i = 0; i < TEST_SIZE; i++) {
			// 	dma_chan->interface_p->buffer[i] = (uint8_t)i;
			// }
		}
		else{
			uint32_t i;
			dma_chan->interface_p = (struct dma_proxy_channel_interface *)memremap(0x0ffff000, sizeof(struct dma_proxy_channel_interface), MEMREMAP_WB);
			// for (i = 0; i < TEST_SIZE; i++) {
			// 	dma_chan->interface_p->buffer[i] = 0xF;
			// }
		}
		// interface_p = ioremap_wt(0x09000000, sizeof(struct dma_proxy_channel_interface));
		if(dma_chan->interface_p==NULL){
			printk(KERN_INFO "Failed to map memory to kernel module\n");
		}
		else{
			printk(KERN_INFO "Allocating memory at 0x%08X\n", (unsigned int)dma_chan->interface_p);
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
/** @brief The LKM cleanup function
*  Similar to the initialization function, it is static. The __exit macro notifies that if this
*  code is used for a built-in driver (not a LKM) that this function is not required.
*/
static void __exit ebbchar_exit(void){
  device_destroy(dma_channel[0].ebbcharClass, MKDEV(dma_channel[0].majorNumber, 0));     // remove the device
  class_unregister(dma_channel[0].ebbcharClass);                          // unregister the device class
  class_destroy(dma_channel[0].ebbcharClass);                             // remove the device class
  unregister_chrdev(dma_channel[0].majorNumber, "ebbcharsend");             // unregister the major number

	device_destroy(dma_channel[1].ebbcharClass, MKDEV(dma_channel[1].majorNumber, 0));     // remove the device
  class_unregister(dma_channel[1].ebbcharClass);                          // unregister the device class
  class_destroy(dma_channel[1].ebbcharClass);                             // remove the device class
  unregister_chrdev(dma_channel[1].majorNumber, "ebbcharrecieve");             // unregister the major number

  printk(KERN_INFO "EBBChar: Goodbye from the LKM!\n");
}

/** @brief The device open function that is called each time the device is opened
*  This will only increment the numberOpens counter in this case.
*  @param inodep A pointer to an inode object (defined in linux/fs.h)
*  @param filep A pointer to a file object (defined in linux/fs.h)
*/
static int dev_open(struct inode *inodep, struct file *filep){
  // numberOpens++;
  // printk(KERN_INFO "EBBChar: Device has been opened %d time(s)\n", numberOpens);
	struct channel_data* tmp_chn;
	tmp_chn = container_of(inodep->i_cdev, struct channel_data, cdev);
	printk(KERN_INFO "tmp_chn: 0x%08X\n", (unsigned int)tmp_chn);
	printk(KERN_INFO "iminor=%d\n", iminor(inodep));
	printk(KERN_INFO "iminor=%d\n", imajor(inodep));
	filep->private_data = inodep;
	return 0;
}

/** @brief This function is called whenever device is being read from user space i.e. data is
*  being sent from the device to the user. In this case is uses the copy_to_user() function to
*  send the buffer string to the user and captures any errors.
*  @param filep A pointer to a file object (defined in linux/fs.h)
*  @param buffer The pointer to the buffer to which this function writes the data
*  @param len The length of the b
*  @param offset The offset if required
*/
static ssize_t dev_read(struct file *filep, char *buffer, size_t len, loff_t *offset){
  // int error_count = 0;
	// int i;
	//
	// printk(KERN_INFO "kernel: interface_p-<length = %d\n", interface_p->length);
	//
	// for(i=0; i<20; i++){
	// 	printk(KERN_INFO "%d\n", interface_p->buffer[i]);
	// }
	//
  // // copy_to_user has the format ( * to, *from, size) and returns 0 on success
  // error_count = copy_to_user(buffer, interface_p, interface_p->length);
	//
  // if (error_count==0){            // if true then have success
  //    printk(KERN_INFO "EBBChar: Sent %d characters to the user\n", size_of_message);
  //    return (size_of_message=0);  // clear the position to the start and return 0
  // }
  // else {
  //    printk(KERN_INFO "EBBChar: Failed to send %d characters to the user\n", error_count);
  //    return -EFAULT;              // Failed -- return a bad address message (i.e. -14)
  // }
}

/** @brief This function is called whenever the device is being written to from user space i.e.
*  data is sent to the device from the user. The data is copied to the message[] array in this
*  LKM using the sprintf() function along with the length of the string.
*  @param filep A pointer to a file object
*  @param buffer The buffer to that contains the string to write to the device
*  @param len The length of the array of data that is being passed in the const char buffer
*  @param offset The offset if required
*/
static ssize_t dev_write(struct file *filep, const char *buffer, size_t len, loff_t *offset){
  // sprintf(message, "%s(%zu letters)", buffer, len);   // appending received string with its length
  // size_of_message = strlen(message);                 // store the length of the stored message
  // printk(KERN_INFO "EBBChar: Received %zu characters from the user\n", len);
  return len;
}

/** @brief The device release function that is called whenever the device is closed/released by
*  the userspace program
*  @param inodep A pointer to an inode object (defined in linux/fs.h)
*  @param filep A pointer to a file object (defined in linux/fs.h)
*/
static int dev_release(struct inode *inodep, struct file *filep){
  printk(KERN_INFO "EBBChar: Device successfully closed\n");
  return 0;
}

/** @brief A module must use the module_init() module_exit() macros from linux/init.h, which
*  identify the initialization function at insertion time and the cleanup function (as
*  listed above)
*/
module_init(ebbchar_init);
module_exit(ebbchar_exit);
MODULE_LICENSE("GPL");
