#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>

#include <linux/types.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <asm/uaccess.h>

#include "data.h"

static dev_t ppgmod_dev;
struct cdev ppgmod_cdev;

struct class *myclass = NULL;

static char buffer[64];

ssize_t ppgmod_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
   // printk(KERN_INFO "[ppgmod] read (count=%d, offset=%d)\n", (int)count, (int)*f_pos );
   // TODO: to be implemented
    return count;
}

struct file_operations ppgmod_fops = {
    .owner = THIS_MODULE,
    .read = ppgmod_read,
};

static int __init ppgmod_module_init(void)
{
    printk(KERN_INFO "Loading ppgmod_module\n");

    alloc_chrdev_region(&ppgmod_dev, 0, 1, "ppgmod_dev");
    printk(KERN_INFO "%s\n", format_dev_t(buffer, ppgmod_dev));

    myclass = class_create(THIS_MODULE, "ppgmod_sys");
    device_create(myclass, NULL, ppgmod_dev, NULL, "ppgmod_dev");

    cdev_init(&ppgmod_cdev, &ppgmod_fops);
    ppgmod_cdev.owner = THIS_MODULE;
    cdev_add(&ppgmod_cdev, ppgmod_dev, 1);

    return 0;
}

static void __exit ppgmod_module_cleanup(void)
{
    printk(KERN_INFO "Cleaning-up ppgmod_dev.\n");

    device_destroy(myclass, ppgmod_dev );
    cdev_del(&ppgmod_cdev);
    class_destroy(myclass);
    unregister_chrdev_region(ppgmod_dev, 1);
}

module_init(ppgmod_module_init);
module_exit(ppgmod_module_cleanup);

MODULE_AUTHOR("Massimiliano Pronesti");
MODULE_LICENSE("GPL");

