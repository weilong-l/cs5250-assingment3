#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/proc_fs.h>
#include <asm/uaccess.h>

#define MAJOR_NUMBER 62
// Device size of 4MB char device
#define DEVICE_SIZE 4*1024*1024

int currentPostionRead = 0;
int currentPostionWrite = 0;
 
/* forward declaration */
int fourmb_open(struct inode *inode, struct file *filep);
int fourmb_release(struct inode *inode, struct file *filep);
ssize_t fourmb_read(struct file *filep, char *buf, size_t count, loff_t *f_pos);
ssize_t fourmb_write(struct file *filep, const char *buf, size_t count, loff_t *f_pos);
static void fourmb_exit(void);

/* definition of file_operation structure */
struct file_operations fourmb_fops = {
     read:     fourmb_read,
     write:    fourmb_write,
     open:     fourmb_open,
     release: fourmb_release
};

char *fourmb_data = NULL;

int fourmb_open(struct inode *inode, struct file *filep)
{
     return 0; // always successful
}
int fourmb_release(struct inode *inode, struct file *filep)
{
     return 0; // always successful
}

// Read from a fourmb driver
ssize_t fourmb_read(struct file *filep, char *buf, size_t count, loff_t *f_pos) {
     /*
     Currntly, the device reading position is at f_pos, 
     */

     int readSize = count;
     int bytesLeft = DEVICE_SIZE - *f_pos;
     if (readSize > bytesLeft) readSize = bytesLeft;

     if (0 == *f_pos) {
          copy_to_user(buf, fourmb_data, 1);
          (*f_pos)++;
     } else {
          return 0;
     }
     return 1;
}

ssize_t fourmb_write(struct file *filep, const char *buf, size_t count, loff_t *f_pos)
{
     /*please complete the function on your own*/
     if (copy_from_user(fourmb_data, buf, 1)) {
          return -1;
     }
     if (count > 1) {
          //printk(KERN_ALERT "Write error: No space left on device\n");
          return -ENOSPC;
     }
     return count;
}

static int fourmb_init(void)
{
     int result;
     // register the device
     result = register_chrdev(MAJOR_NUMBER, "fourmb", &fourmb_fops);
     if (result < 0) {
          return result;
     }
     // allocate one byte of memory for storage
     // kmalloc is just like malloc, the second parameter is
     // the type of memory to be allocated.
     // To release the memory allocated by kmalloc, use kfree.
     fourmb_data = kmalloc(DEVICE_SIZE * sizeof(char), GFP_KERNEL);
     memset(fourmb_data, 0, DEVICE_SIZE * sizeof(char));

     if (!fourmb_data) {
          fourmb_exit();
          // cannot allocate memory
          // return no memory error, negative signify a failure
          return -ENOMEM;
     }

     // initialize the value to be X
     *fourmb_data = 'X';

     printk(KERN_ALERT "This is a fourmb device module\n");
     return 0;
}

static void fourmb_exit(void)
{
     // if the pointer is pointing to something
     if (fourmb_data) {
          // free the memory and assign the pointer to NULL
          kfree(fourmb_data);
     }    fourmb_data = NULL;
     // unregister the device
     unregister_chrdev(MAJOR_NUMBER, "fourmb");
     printk(KERN_ALERT "fourmb device module is unloaded\n");
}
MODULE_LICENSE("GPL");
module_init(fourmb_init);
module_exit(fourmb_exit);