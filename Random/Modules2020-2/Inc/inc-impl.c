#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/semaphore.h>

#define DEVICE_NAME "inc"
#define BUF_LEN 8192

static int major = 61;
static struct semaphore sr, sw;
static char inc_buf[BUF_LEN];
static int size = 0;

static ssize_t inc_read(struct file *filp, char *buf, size_t cnt, loff_t *f_pos);
static ssize_t inc_write(struct file *filp, const char *buf, size_t cnt, loff_t *f_pos);
static int inc_open(struct inode *inode, struct file *filp);
static int inc_release(struct inode *inode, struct file *filp);

static struct file_operations fops = {
    .read = inc_read,
    .write = inc_write,
    .open = inc_open,
    .release = inc_release
};

static int __init inc_init(void) {
    int rc;
    rc = register_chrdev(major, DEVICE_NAME, &fops);
    if (rc < 0) {
        printk(KERN_ALERT "Registering char device failed with %d\n", rc);
        return rc;
    }

    sema_init(&sr, 0);
    sema_init(&sw, 0);

    printk(KERN_INFO "I was assigned major number %d. To talk to\n", major);
    printk(KERN_INFO "the driver, create a dev file with\n");
    printk(KERN_INFO "'mknod /dev/%s c %d 0'.\n", DEVICE_NAME, major);
    printk(KERN_INFO "Try various minor numbers. Try to cat and echo to\n");
    printk(KERN_INFO "the device file.\n");

    return 0;
}

static void __exit inc_exit(void) {
    unregister_chrdev(major, DEVICE_NAME);
    printk(KERN_INFO "Goodbye, world!\n");
}

static int inc_open(struct inode *inode, struct file *filp) {
    try_module_get(THIS_MODULE);
    return 0;
}

static int inc_release(struct inode *inode, struct file *filp) {
    module_put(THIS_MODULE);
    return 0;
}

static ssize_t inc_read(struct file *filp, char *buf, size_t cnt, loff_t *f_pos) {
    down(&sr);
    if (cnt > size) cnt = size;
    if (copy_to_user(buf, inc_buf, cnt)) {
        return -EFAULT;
    }
    inc_buf[0] += 1;
    up(&sw);
    return cnt;
}

static ssize_t inc_write(struct file *filp, const char *buf, size_t cnt, loff_t *f_pos) {
    if (cnt > BUF_LEN) cnt = BUF_LEN;
    if (copy_from_user(inc_buf, buf, cnt)) {
        return -EFAULT;
    }
    size = cnt;
    up(&sr); up(&sr); up(&sr);
    down(&sw);
    return cnt;
}

module_init(inc_init);
module_exit(inc_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Juan Molina");
MODULE_DESCRIPTION("A simple /dev/inc device driver example");
MODULE_VERSION("1.0");
