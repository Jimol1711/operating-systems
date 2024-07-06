/* Necessary includes for device drivers */
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h> /* printk() */
#include <linux/slab.h> /* kmalloc() */
#include <linux/fs.h> /* everything... */
#include <linux/errno.h> /* error codes */
#include <linux/types.h> /* size_t */
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

/* Estructura para el pipe */
struct disco_pipe {
    char *buffer;
    int in, out, size;
    int closed; // Indica si el escritor cerró el pipe
    struct file *writer; // Referencia al file descriptor del escritor
    struct file *reader; // Referencia al file descriptor del lector
    KMutex mutex;
    KCondition cond;
};

/* Variables globales */
int disco_major = 61;     /* Major number */
#define MAX_SIZE 8192
struct disco_pipe *waiting_reader = NULL;
struct disco_pipe *waiting_writer = NULL;

KMutex global_mutex;
KCondition global_cond;

int disco_init(void) {
    int rc;

    /* Registering device */
    rc = register_chrdev(disco_major, "disco", &disco_fops);
    if (rc < 0) {
        printk(KERN_ALERT "disco: cannot obtain major number %d\n on line 63", disco_major);
        return rc;
    }

    m_init(&global_mutex);
    c_init(&global_cond);

    printk(KERN_INFO "Inserting disco module on line 70\n");
    return 0;
}

void disco_exit(void) {
    /* Freeing the major number */
    unregister_chrdev(disco_major, "disco");

    printk(KERN_INFO "Removing disco module on line 78\n");
}

int disco_open(struct inode *inode, struct file *filp) {
    struct disco_pipe *pipe;
    int is_writer = filp->f_mode & FMODE_WRITE;

    printk(KERN_INFO "disco_open: is_writer=%d on line 85\n", is_writer);

    m_lock(&global_mutex);

    if (is_writer) {
        if (waiting_reader) {
            printk(KERN_INFO "disco_open: there is a waiting_reader on line 91: %d\n", waiting_reader != 0);
            pipe = waiting_reader;
            pipe->writer = filp;
            waiting_reader = NULL;
            filp->private_data = pipe;
            c_broadcast(&pipe->cond);
            m_unlock(&global_mutex);
            printk(KERN_INFO "disco_open: matched writer with reader on line 98\n");
        } else {
            printk(KERN_INFO "disco_open: there isn't a waiting_reader line 100: %d\n", waiting_reader != 0);
            pipe = kmalloc(sizeof(struct disco_pipe), GFP_KERNEL);
            if (!pipe) {
                m_unlock(&pipe->mutex);
                return -ENOMEM;
            }
            memset(pipe, 0, sizeof(struct disco_pipe));
            pipe->buffer = kmalloc(MAX_SIZE, GFP_KERNEL);
            if (!pipe->buffer) {
                kfree(pipe);
                m_unlock(&pipe->mutex);
                return -ENOMEM;
            }
            m_init(&pipe->mutex);
            c_init(&pipe->cond);
            pipe->in = pipe->out = pipe->size = 0;
            pipe->closed = 0;
            pipe->writer = filp;
            pipe->reader = NULL;
            waiting_writer = pipe;
            filp->private_data = pipe;
            printk(KERN_INFO "disco_open: writer waiting for reader on line 121 (pipe->reader=%d)\n", pipe->reader != 0);
            m_unlock(&global_mutex);
            m_lock(&pipe->mutex);
            while (!pipe->reader) {
                if (c_wait(&pipe->cond, &pipe->mutex)) {
                    waiting_writer = NULL;
                    m_unlock(&pipe->mutex);
                    kfree(pipe->buffer);
                    kfree(pipe);
                    printk(KERN_INFO "disco_open: writer interrupted on line 130\n");
                    return -EINTR;
                }
            }
            m_unlock(&pipe->mutex);
            printk(KERN_INFO "disco_open: writer no longer waiting for reader on line 135\n");
        }
    } else {
        if (waiting_writer) {
            printk(KERN_INFO "disco_open: there is a waiting_writer on line 139: %d\n", waiting_writer != 0);
            pipe = waiting_writer;
            pipe->reader = filp;
            waiting_writer = NULL;
            filp->private_data = pipe;
            c_broadcast(&pipe->cond);
            m_unlock(&global_mutex);
            printk(KERN_INFO "disco_open: matched reader with writer on line 146\n");
        } else {
            printk(KERN_INFO "disco_open: there isn't a waiting_writer on line 148: %d\n", waiting_writer != 0);
            pipe = kmalloc(sizeof(struct disco_pipe), GFP_KERNEL);
            if (!pipe) {
                m_unlock(&pipe->mutex);
                return -ENOMEM;
            }
            memset(pipe, 0, sizeof(struct disco_pipe));
            pipe->buffer = kmalloc(MAX_SIZE, GFP_KERNEL);
            if (!pipe->buffer) {
                kfree(pipe);
                m_unlock(&pipe->mutex);
                return -ENOMEM;
            }
            m_init(&pipe->mutex);
            c_init(&pipe->cond);
            pipe->in = pipe->out = pipe->size = 0;
            pipe->closed = 0;
            pipe->reader = filp;
            pipe->writer = NULL;
            waiting_reader = pipe;
            filp->private_data = pipe;
            printk(KERN_INFO "disco_open: reader waiting for writer on line 169 (pipe->writer=%d)\n", pipe->writer != 0);
            m_unlock(&global_mutex);
            m_lock(&pipe->mutex);
            while (!pipe->writer) {
                if (c_wait(&pipe->cond, &pipe->mutex)) {
                    waiting_reader = NULL;
                    m_unlock(&pipe->mutex);
                    kfree(pipe->buffer);
                    kfree(pipe);
                    printk(KERN_INFO "disco_open: reader interrupted on line 178\n");
                    return -EINTR;
                }
            }
            m_unlock(&pipe->mutex);
            printk(KERN_INFO "disco_open: reader no longer waiting for writer on line 183\n");
        }
    }

    // m_unlock(&global_mutex); ESTE PROBABLEMENTE NO VA
    printk(KERN_INFO "disco_open: pipe initialized on line 188. is_writer = %d\n", is_writer);
    return 0;
}

int disco_release(struct inode *inode, struct file *filp) {
    struct disco_pipe *pipe = filp->private_data;

    m_lock(&pipe->mutex);

    if (filp == pipe->writer) {
        printk(KERN_INFO "filp and pipe->writer are the same on line 198\n");
        pipe->closed = 1;
        pipe->writer = NULL;
        c_broadcast(&pipe->cond);
        printk(KERN_INFO "disco_release: writer closed on line 202\n");
    } else if (filp == pipe->reader) {
        printk(KERN_INFO "filp and pipe->reader are the same on line 204\n");
        pipe->reader = NULL;
        c_broadcast(&pipe->cond);
        printk(KERN_INFO "disco_release: reader closed on line 207\n");
    }

    if (!pipe->writer && !pipe->reader) {
        kfree(pipe->buffer);
        kfree(pipe);
        printk(KERN_INFO "disco_release: pipe deallocated on line 213\n");
    }

    m_unlock(&pipe->mutex);
    return 0;
}

ssize_t disco_read(struct file *filp, char *buf, size_t count, loff_t *f_pos) {
    struct disco_pipe *pipe = filp->private_data;
    int rc;

    printk(KERN_INFO "disco_read: requested count=%zu on line 224\n", count);

    m_lock(&pipe->mutex);

    while (pipe->size == 0 && !pipe->closed) {
        printk(KERN_INFO "disco_read: waiting for data on line 229 where pipe->size and pipe->closed are: %d and %d\n", pipe->size, pipe->closed);
        if (c_wait(&pipe->cond, &pipe->mutex)) {
            m_unlock(&pipe->mutex);
            printk(KERN_INFO "disco_read: interrupted on line 232\n");
            return -EINTR;
        }
    }

    if (pipe->size == 0 && pipe->closed) {
        m_unlock(&pipe->mutex);
        printk(KERN_INFO "disco_read: no data, writer closed on line 239 where pipe->size and pipe->closed are: %d and %d\n", pipe->size, pipe->closed);
        return 0;
    }

    if (count > pipe->size)
        count = pipe->size;

    if (copy_to_user(buf, pipe->buffer + pipe->out, count)) {
        m_unlock(&pipe->mutex);
        printk(KERN_ERR "disco_read: copy_to_user failed on line 248\n");
        return -EFAULT;
    }

    pipe->out = (pipe->out + count) % MAX_SIZE;
    pipe->size -= count;
    rc = count;

    printk(KERN_INFO "disco_read: read %d bytes, new size=%d on line 256\n", rc, pipe->size);

    c_broadcast(&pipe->cond);
    m_unlock(&pipe->mutex);

    return rc;
}

ssize_t disco_write(struct file *filp, const char *buf, size_t count, loff_t *f_pos) {
    struct disco_pipe *pipe = filp->private_data;
    int rc;

    printk(KERN_INFO "disco_write: requested count=%zu on line 268\n", count);

    m_lock(&pipe->mutex);

    while (pipe->size == MAX_SIZE) {
        printk(KERN_INFO "disco_write: waiting for space on line 273 where pipe->size is %d\n", pipe->size);
        if (c_wait(&pipe->cond, &pipe->mutex)) {
            m_unlock(&pipe->mutex);
            printk(KERN_INFO "disco_write: interrupted on line 276\n");
            return -EINTR;
        }
    }

    if (count > MAX_SIZE - pipe->size)
        count = MAX_SIZE - pipe->size;

    if (copy_from_user(pipe->buffer + pipe->in, buf, count)) {
        m_unlock(&pipe->mutex);
        printk(KERN_ERR "disco_write: copy_from_user failed on line 286\n");
        return -EFAULT;
    }

    pipe->in = (pipe->in + count) % MAX_SIZE;
    pipe->size += count;
    rc = count;

    printk(KERN_INFO "disco_write: wrote %d bytes, new size=%d on line 294\n", rc, pipe->size);

    c_broadcast(&pipe->cond);
    m_unlock(&pipe->mutex);

    return rc;
}