#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>

#include <linux/types.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <asm/uaccess.h>
#include <linux/uaccess.h>
#include <linux/mutex.h>

#include "data.h"
#define DEVNAME   "ppgmod_dev"
#define CLASSNAME "ppgmod_sys"

static dev_t ppgmod_dev;
struct cdev ppgmod_cdev;

struct device *dev = NULL; 
struct class *dev_class = NULL;
static char buffer[64];

static int counter;
static DEFINE_MUTEX(dev_mutex);

ssize_t ppgmod_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
    int val = ppg[counter++];

    if(copy_to_user((void*)buf, (void*)&(val), sizeof(int))){
        printk(KERN_INFO "failed to send value to the user\n");
        return -EFAULT;
    }
    counter  %= 2048;
    
    printk(KERN_INFO "[ppgmod] read value=%d\n", val);
    return sizeof(int);
}


int ppgmod_open(struct inode *inodep, struct file *filep){
    if(!mutex_trylock(&dev_mutex)){ // Try to aquire the mutex
        printk(KERN_ALERT "Device can't be accessed, already in use");
        return -EBUSY;
    }
    counter = 0;
    printk(KERN_INFO "device successfully opened\n");
    return 0;
}


int ppgmod_release(struct inode *inodep, struct file *filep){
    mutex_unlock(&dev_mutex); // Release the mutex
    printk(KERN_INFO "device successfully closed\n");
    return 0;
}


struct file_operations ppgmod_fops = {
    .owner   = THIS_MODULE,
    .open    = ppgmod_open,
    .read    = ppgmod_read,
    .release = ppgmod_release
};

/**
 *  @brief inits the module
 **/
static int __init ppgmod_module_init(void)
{
    printk(KERN_INFO "Loading ppgmod_module\n");

    alloc_chrdev_region(&ppgmod_dev, 0, 1, DEVNAME);
    printk(KERN_INFO "%s\n", format_dev_t(buffer, ppgmod_dev));

    // device class creation
    dev_class = class_create(THIS_MODULE, CLASSNAME);
    if(IS_ERR(dev_class)){
        printk(KERN_ALERT "Failed to register device class\n");
        return PTR_ERR(dev_class);
    }

    // device creation
    dev = device_create(dev_class, NULL, ppgmod_dev, NULL, DEVNAME);
    if(IS_ERR(dev)){
        class_destroy(dev_class);
        printk(KERN_ALERT "Failed to create device\n");
        return PTR_ERR(dev);
    }
    mutex_init(&dev_mutex);

    cdev_init(&ppgmod_cdev, &ppgmod_fops);
    ppgmod_cdev.owner = THIS_MODULE;
    cdev_add(&ppgmod_cdev, ppgmod_dev, 1);

    return 0;
}


static void __exit ppgmod_module_cleanup(void)
{
    printk(KERN_INFO "Cleaning-up ppgmod_dev.\n");
    
    mutex_destroy(&dev_mutex);
    device_destroy(dev_class, ppgmod_dev );
    cdev_del(&ppgmod_cdev);
    class_destroy(dev_class);
    unregister_chrdev_region(ppgmod_dev, 1);
}

module_init(ppgmod_module_init);
module_exit(ppgmod_module_cleanup);

MODULE_AUTHOR("Massimiliano Pronesti");
MODULE_LICENSE("GPL");

