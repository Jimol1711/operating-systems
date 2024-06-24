/* Necessary includes for device drivers */
#include <linux/init.h>
/* #include <linux/config.h> */
#include <linux/module.h>
#include <linux/kernel.h> /* printk() */
#include <linux/slab.h> /* kmalloc() */
#include <linux/fs.h> /* everything... */
#include <linux/errno.h> /* error codes */
#include <linux/types.h> /* size_t */
#include <linux/proc_fs.h>
#include <linux/fcntl.h> /* O_ACCMODE */
#include <linux/uaccess.h> /* copy_from/to_user */

#include "kmutex.h"

MODULE_LICENSE("Dual BSD/GPL");

/* Declaration of disco.c functions */
int disco_open(struct inode *inode, struct file *filp);
int disco_release(struct inode *inode, struct file *filp);
ssize_t disco_read(struct file *filp, char *buf, size_t count, loff_t *f_pos);
ssize_t disco_write(struct file *filp, const char *buf, size_t count, loff_t *f_pos);

void disco_exit(void);
int disco_init(void);

/* Structure that declares the usual file */
/* access functions */
struct file_operations disco_fops = {
  read: disco_read,
  write: disco_write,
  open: disco_open,
  release: disco_release
};

/* Declaration of the init and exit functions */
module_init(disco_init);
module_exit(disco_exit);

/*** El driver para lecturas sincronas *************************************/

#define TRUE 1
#define FALSE 0

/* Global variables of the driver */

int disco_major = 61;     /* Major number */

/* Buffer to store data */
#define MAX_SIZE 8192

/* Estructura para el pipe */
struct disco_pipe {
    char *buffer;
    int in, out, size;
    int closed; // Indica si el escritor cerró el pipe
    KMutex mutex;
    KCondition cond;
};

/* Initialize the module */
int disco_init(void) {
    int rc;
    rc = register_chrdev(disco_major, "disco", &disco_fops);
    if (rc < 0) {
        printk("<1>disco: cannot obtain major number %d\n", disco_major);
        return rc;
    }
    printk("<1>Inserting disco module\n");
    return 0;
}

/* Clean up the module */
void disco_exit(void) {
    unregister_chrdev(disco_major, "disco");
    printk("<1>Removing disco module\n");
}

/* Open the device */
int disco_open(struct inode *inode, struct file *filp) {
    struct disco_pipe *pipe;
    pipe = kmalloc(sizeof(struct disco_pipe), GFP_KERNEL);
    if (!pipe) return -ENOMEM;

    pipe->buffer = kmalloc(MAX_SIZE, GFP_KERNEL);
    if (!pipe->buffer) {
        kfree(pipe);
        return -ENOMEM;
    }
    pipe->in = pipe->out = pipe->size = 0;
    pipe->closed = FALSE;
    m_init(&pipe->mutex);
    c_init(&pipe->cond);

    filp->private_data = pipe;

    m_lock(&pipe->mutex);
    // Logic to pair reader and writer
    if (filp->f_mode & FMODE_WRITE) {
        // Handle writer
    } else if (filp->f_mode & FMODE_READ) {
        // Handle reader
    }
    m_unlock(&pipe->mutex);

    return 0;
}

/* Release the device */
int disco_release(struct inode *inode, struct file *filp) {
    struct disco_pipe *pipe = filp->private_data;

    m_lock(&pipe->mutex);
    if (filp->f_mode & FMODE_WRITE) {
        pipe->closed = TRUE;
        c_broadcast(&pipe->cond);
    }
    m_unlock(&pipe->mutex);

    kfree(pipe->buffer);
    kfree(pipe);
    return 0;
}

/* Read from the device */
ssize_t disco_read(struct file *filp, char *buf, size_t count, loff_t *f_pos) {
    struct disco_pipe *pipe = filp->private_data;
    ssize_t ret = 0;

    m_lock(&pipe->mutex);

    while (pipe->size == 0 && !pipe->closed) {
        if (c_wait(&pipe->cond, &pipe->mutex)) {
            ret = -EINTR;
            goto epilog;
        }
    }

    if (pipe->size > 0) {
        if (count > pipe->size) count = pipe->size;
        for (ret = 0; ret < count; ret++) {
            if (copy_to_user(buf + ret, &pipe->buffer[pipe->out], 1)) {
                ret = -EFAULT;
                goto epilog;
            }
            pipe->out = (pipe->out + 1) % MAX_SIZE;
            pipe->size--;
        }
    } else if (pipe->closed) {
        ret = 0; // Indicate end of file
    }

epilog:
    m_unlock(&pipe->mutex);
    return ret;
}

/* Write to the device */
ssize_t disco_write(struct file *filp, const char *buf, size_t count, loff_t *f_pos) {
    struct disco_pipe *pipe = filp->private_data;
    ssize_t ret = 0;

    m_lock(&pipe->mutex);

    for (ret = 0; ret < count; ret++) {
        while (pipe->size == MAX_SIZE) {
            if (c_wait(&pipe->cond, &pipe->mutex)) {
                ret = -EINTR;
                goto epilog;
            }
        }
        if (copy_from_user(&pipe->buffer[pipe->in], buf + ret, 1)) {
            ret = -EFAULT;
            goto epilog;
        }
        pipe->in = (pipe->in + 1) % MAX_SIZE;
        pipe->size++;
        c_broadcast(&pipe->cond);
    }
    
epilog:
    m_unlock(&pipe->mutex);
    return ret;
}