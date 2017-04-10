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

#define SCULL_IOC_MAGIC 'k'
#define SCULL_IOC_MAXNR 4
#define SCULL_HELLO _IO(SCULL_IOC_MAGIC, 1)
#define SCULL_SETMSG _IOW(SCULL_IOC_MAGIC, 2, char *)
#define SCULL_GETMSG _IOR(SCULL_IOC_MAGIC, 3, char *)
#define SCULL_SET_GETMSG _IOWR(SCULL_IOC_MAGIC, 4, char *)

/* Set the message of the device driver */
// #define IOCTL_SET_MSG _IOR(MAJOR_NUM, 0, char *)
/* _IOR means that we're creating an ioctl command 
 * number for passing information from a user process
 * to the kernel module. 
 *
 * The first arguments, MAJOR_NUM, is the major device 
 * number we're using.
 *
 * The second argument is the number of the command 
 * (there could be several with different meanings).
 *
 * The third argument is the type we want to get from 
 * the process to the kernel.
 */

/* Get the message of the device driver */
// #define IOCTL_GET_MSG _IOR(MAJOR_NUM, 1, char *)
 /* This IOCTL is used for output, to get the message 
  * of the device driver. However, we still need the 
  * buffer to place the message in to be input, 
  * as it is allocated by the process.
  */


/* Get the n'th byte of the message */
// #define IOCTL_GET_NTH_BYTE _IOWR(MAJOR_NUM, 2, int)
 /* The IOCTL is used for both input and output. It 
  * receives from the user a number, n, and returns 
  * Message[n]. */
char *fourmb_data = NULL;
int total_size = 0;
static char dev_msg[DEVICE_SIZE];
 
/* forward declaration */
int fourmb_open(struct inode *inode, struct file *filep);
int fourmb_release(struct inode *inode, struct file *filep);
ssize_t fourmb_read(struct file *filep, char *buf, size_t count, loff_t *f_pos);
ssize_t fourmb_write(struct file *filep, const char *buf, size_t count, loff_t *f_pos);
loff_t fourmb_lseek(struct file *filp, loff_t off, int whence);
static void fourmb_exit(void);
long fourmb_ioctl(struct file *file, unsigned int ioctl_num, unsigned long ioctl_param);

/* definition of file_operation structure */
struct file_operations fourmb_fops = {
     read:     fourmb_read,
     write:    fourmb_write,
     open:     fourmb_open,
     release: fourmb_release,
     llseek: fourmb_lseek,
     unlocked_ioctl : fourmb_ioctl
};

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
}

loff_t fourmb_lseek(struct file *filp, loff_t off, int whence) {
     loff_t newpos;

     switch(whence) {
          case 0: /* SEEK_SET */
               newpos = off;
               break;

          case 1: /* SEEK_CUR */
               newpos = filp->f_pos + off;
               break;

          case 2: /* SEEK_END */
               newpos = total_size + off;
               break;

          default: /* can't happen */
               return -EINVAL;
     }
     if (newpos < 0) return -EINVAL;
     filp->f_pos = newpos;
     return newpos;
}

long fourmb_ioctl(struct file *file, unsigned int ioctl_num, unsigned long ioctl_param){
     int err = 0;
     int retval = 0;
     
     /*
     * extract the type and number bitfields, and don't decode
     * wrong cmds: return ENOTTY (inappropriate ioctl) before access_ok()
     */

     if (_IOC_TYPE(ioctl_num) != SCULL_IOC_MAGIC) return -ENOTTY;
     if (_IOC_NR(ioctl_num) > SCULL_IOC_MAXNR) return -ENOTTY;

     /*
     * the direction is a bitmask, and VERIFY_WRITE catches R/W
     * transfers. 'Type' is user‐oriented, while
     * access_ok is kernel‐oriented, so the concept of "read" and
     * "write" is reversed
     */
     
     if (_IOC_DIR(ioctl_num) & _IOC_READ)
          err = !access_ok(VERIFY_WRITE, (void __user *)ioctl_param, _IOC_SIZE(ioctl_num));
     else if (_IOC_DIR(ioctl_num) & _IOC_WRITE)
          err = !access_ok(VERIFY_READ, (void __user *)ioctl_param, _IOC_SIZE(ioctl_num));

     if (err) return -EFAULT;
     
     int i;
     char ch;
     char *temp;
     char *tmp;

     /* Switch according to the ioctl called */
     switch (ioctl_num) {
          case SCULL_HELLO:
               printk(KERN_WARNING "hello\n");
               break;
          case SCULL_SETMSG:
               /* Receive a pointer to a message (in user space) 
               * and set that to be the device's message. */ 

               /* Get the parameter given to ioctl by the process */
               temp = (char *) ioctl_param;
   
               /* Find the length of the message */
               get_user(ch, temp);
               for (i=0; ch && i<DEVICE_SIZE; i++, temp++)
                    get_user(ch, temp);
               if (copy_from_user(dev_msg, (char *) ioctl_param, i)) {
                    return -EFAULT;
               }
               printk(KERN_WARNING "Set device msg to %s\n", dev_msg);
               break;
          case SCULL_GETMSG:
               /* Give the current message to the calling 
               * process - the parameter we got is a pointer, 
               * fill it. */
               // i = device_read(file, (char *) ioctl_param, 99, 0); 
               if (copy_to_user((char *) ioctl_param, dev_msg, DEVICE_SIZE)) {
                    return -EFAULT;
               }

               /* Put a zero at the end of the buffer, so it 
               * will be properly terminated */
               put_user('\0', (char *) ioctl_param+i);
               break;
          
          case SCULL_SET_GETMSG:
               /* This ioctl is both input (ioctl_param) and 
               * output (the return value of this function) */
               if (copy_from_user(tmp, (char *) ioctl_param, DEVICE_SIZE)) {
                    return -EFAULT;
               }
               printk(KERN_WARNING "set new device msg to %s", tmp);
               if (copy_to_user((char *) ioctl_param, dev_msg, DEVICE_SIZE)) {
                    return -EFAULT;
               }
               printk(KERN_WARNING "get old device msg as %s\n", dev_msg);
               // put_user('\0', (char *) ioctl_param+i);
               break;
          default:
               return -ENOTTY;
  }

  return retval;
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