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
int total_size = 0;

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
     printk("read requested: %zu, current offset: %lld\n", count, *f_pos);
     
     if (*f_pos >= DEVICE_SIZE) return 0;
     
     int bytes_to_read = count;
     if (bytes_to_read > total_size - (*f_pos)) bytes_to_read = total_size - (*f_pos);
     
     if (copy_to_user(buf, fourmb_data + *(f_pos), bytes_to_read)) {
          return -EFAULT;
     }
     (*f_pos) += bytes_to_read;

     printk("read: %d, current offset: %lld\n", bytes_to_read, *f_pos);
     return bytes_to_read;
     // int bytes_read = 0;
     // if (*fourmb_data == 0) {
     //      printk("Reached the end of file.\n");
     //      return 0;
     // }
     // while (count && *fourmb_data) {
     //      copy_to_user(buf++, fourmb_data++, sizeof(char));
     //      count--;
     //      bytes_read++;
     // }
     // *f_pos = bytes_read;
     // printk("read: %d, current offset: %lld\n", bytes_read, *f_pos);
     // return bytes_read;
}

ssize_t fourmb_write(struct file *filep, const char *buf, size_t count, loff_t *f_pos)
{
     printk("requested: %zu, before offset: %lld\t", count, *f_pos);
     int bytes_to_write = count;
     if (bytes_to_write > (DEVICE_SIZE - (*f_pos))) {
          bytes_to_write = DEVICE_SIZE - (*f_pos);
     }

     if (copy_from_user(fourmb_data + (*f_pos), buf, bytes_to_write)) {
          return -EFAULT;
     }

     (*f_pos) += bytes_to_write;
     if (total_size < (*f_pos)) total_size = (*f_pos); //total_size = *f_pos;
     printk("bytes written: %d, requested: %zu, after offset: %lld\n", bytes_to_write, count, *f_pos);
     if (bytes_to_write < count) return -ENOSPC;
     return bytes_to_write;

     // int bytes_written = 0;
     // if (count == 0) {
     //      printk("You can't write nothing to the driver.\n");
     //      return 0;
     // }
     // while (count > 0 && ((*f_pos + bytes_written) < DEVICE_SIZE)) {
     //      printk("count: %zu, position: %d\n", count, (*f_pos + bytes_written));
     //      copy_from_user(fourmb_data++, buf++, sizeof(char));
     //      count--;
     //      bytes_written++;
     // }
     // if (count > 0) {
     //      printk("written: %d, left: %zu\n", bytes_written, count);
     //      return -ENOSPC;
     // }
     // printk("written: %d, current offset: %lld\n", bytes_written, *f_pos);
     // return bytes_written;
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
     total_size = 1;

     printk(KERN_ALERT "This is a fourmb device module\n");
     return 0;
}

static void fourmb_exit(void)
{
     // if the pointer is pointing to something
     if (fourmb_data) {
          // free the memory and assign the pointer to NULL
          kfree(fourmb_data);
     }    
     fourmb_data = NULL;
     // unregister the device
     unregister_chrdev(MAJOR_NUMBER, "fourmb");
     printk(KERN_ALERT "fourmb device module is unloaded\n");
}
MODULE_LICENSE("GPL");
module_init(fourmb_init);
module_exit(fourmb_exit);