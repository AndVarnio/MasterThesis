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
#include "hello_world_kernel_module.h"

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

static int    majorNumber;                  ///< Stores the device number -- determined automatically
static char   message[256] = {0};           ///< Memory for the string that is passed from userspace
static short  size_of_message;              ///< Used to remember the size of the string stored
static int    numberOpens = 0;              ///< Counts the number of times the device is opened
static struct class*  ebbcharClass  = NULL; ///< The device-driver class struct pointer
static struct device* ebbcharDevice = NULL; ///< The device-driver device struct pointer

// The prototype functions for the character driver -- must come before the struct definition
static int     dev_open(struct inode *, struct file *);
static int     dev_release(struct inode *, struct file *);
static ssize_t dev_read(struct file *, char *, size_t, loff_t *);
static ssize_t dev_write(struct file *, const char *, size_t, loff_t *);


struct dma_proxy_channel_interface *interface_p;
dma_addr_t interface_phys_addr;

static int mmap(struct file *file_p, struct vm_area_struct *vma)
{

printk(KERN_INFO "Remapping\n");
	/* The virtual address to map into is good, but the page frame will not be good since
	 * user space passes a physical address of 0, so get the physical address of the buffer
	 * that was allocated and convert to a page frame number.
	 */

		remap_pfn_range(vma, vma->vm_start,
							virt_to_phys((void *)interface_p)>>PAGE_SHIFT,
							vma->vm_end - vma->vm_start, vma->vm_page_prot);


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
  .mmap = mmap
};

/** @brief The LKM initialization function
*  The static keyword restricts the visibility of the function to within this C file. The __init
*  macro means that for a built-in driver (not a LKM) the function is only used at initialization
*  time and that it can be discarded and its memory freed up after that point.
*  @return returns 0 if successful
*/
static int __init ebbchar_init(void){
  printk(KERN_INFO "EBBChar: Initializing the EBBChar LKM\n");

  // Try to dynamically allocate a major number for the device -- more difficult but worth it
  majorNumber = register_chrdev(0, DEVICE_NAME, &fops);
  if (majorNumber<0){
     printk(KERN_ALERT "EBBChar failed to register a major number\n");
     return majorNumber;
  }
  printk(KERN_INFO "EBBChar: registered correctly with major number %d\n", majorNumber);

  // Register the device class
  ebbcharClass = class_create(THIS_MODULE, CLASS_NAME);
  if (IS_ERR(ebbcharClass)){                // Check for error and clean up if there is
     unregister_chrdev(majorNumber, DEVICE_NAME);
     printk(KERN_ALERT "Failed to register device class\n");
     return PTR_ERR(ebbcharClass);          // Correct way to return an error on a pointer
  }
  printk(KERN_INFO "EBBChar: device class registered correctly\n");

  // Register the device driver
  ebbcharDevice = device_create(ebbcharClass, NULL, MKDEV(majorNumber, 0), NULL, DEVICE_NAME);
  if (IS_ERR(ebbcharDevice)){               // Clean up if there is an error
     class_destroy(ebbcharClass);           // Repeated code but the alternative is goto statements
     unregister_chrdev(majorNumber, DEVICE_NAME);
     printk(KERN_ALERT "Failed to create the device\n");
     return PTR_ERR(ebbcharDevice);
  }
  printk(KERN_INFO "EBBChar: device class created correctly\n"); // Made it! device was initialized

 //  interface_p = (struct dma_proxy_channel_interface *)
 //   kzalloc(sizeof(struct dma_proxy_channel_interface),
 //       GFP_KERNEL);
 // printk(KERN_INFO "Allocating cached memory at 0x%08X\n",
 //      (unsigned int)interface_p);


			dma_set_coherent_mask(ebbcharDevice, 0xFFFFFFFF);
			interface_p = (struct dma_proxy_channel_interface *)
				dmam_alloc_coherent(ebbcharDevice,
									sizeof(struct dma_proxy_channel_interface),
									&interface_phys_addr, GFP_KERNEL);
			printk(KERN_INFO "Allocating uncached memory at 0x%08X\n",
					 (unsigned int)interface_p);

		if (!interface_p) {
			dev_err(ebbcharDevice, "DMA allocation error\n");
			return -1;
		}
  return 0;
}

/** @brief The LKM cleanup function
*  Similar to the initialization function, it is static. The __exit macro notifies that if this
*  code is used for a built-in driver (not a LKM) that this function is not required.
*/
static void __exit ebbchar_exit(void){
  device_destroy(ebbcharClass, MKDEV(majorNumber, 0));     // remove the device
  class_unregister(ebbcharClass);                          // unregister the device class
  class_destroy(ebbcharClass);                             // remove the device class
  unregister_chrdev(majorNumber, DEVICE_NAME);             // unregister the major number
  printk(KERN_INFO "EBBChar: Goodbye from the LKM!\n");
}

/** @brief The device open function that is called each time the device is opened
*  This will only increment the numberOpens counter in this case.
*  @param inodep A pointer to an inode object (defined in linux/fs.h)
*  @param filep A pointer to a file object (defined in linux/fs.h)
*/
static int dev_open(struct inode *inodep, struct file *filep){
  numberOpens++;
  printk(KERN_INFO "EBBChar: Device has been opened %d time(s)\n", numberOpens);
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
  int error_count = 0;
	int i;

	printk(KERN_INFO "kernel: interface_p-<length = %d\n", interface_p->length);

	for(i=0; i<20; i++){
		printk(KERN_INFO "%d\n", interface_p->buffer[i]);
	}

  // copy_to_user has the format ( * to, *from, size) and returns 0 on success
  error_count = copy_to_user(buffer, interface_p, interface_p->length);

  if (error_count==0){            // if true then have success
     printk(KERN_INFO "EBBChar: Sent %d characters to the user\n", size_of_message);
     return (size_of_message=0);  // clear the position to the start and return 0
  }
  else {
     printk(KERN_INFO "EBBChar: Failed to send %d characters to the user\n", error_count);
     return -EFAULT;              // Failed -- return a bad address message (i.e. -14)
  }
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
  sprintf(message, "%s(%zu letters)", buffer, len);   // appending received string with its length
  size_of_message = strlen(message);                 // store the length of the stored message
  printk(KERN_INFO "EBBChar: Received %zu characters from the user\n", len);
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
