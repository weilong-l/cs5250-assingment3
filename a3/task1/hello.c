#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
MODULE_LICENSE("GPL");
static char *mystring = "blah";
module_param(mystring, charp, 0000);
MODULE_PARM_DESC(mystring, "A character string");
static int hello_init(void)
{
     printk(KERN_ALERT "Hello, %s\n", mystring);
return 0; }
static void hello_exit(void)
{
     printk(KERN_ALERT "Goodbye, cruel world\n");
}
module_init(hello_init);
module_exit(hello_exit);
