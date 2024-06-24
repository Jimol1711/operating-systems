#if 0
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

/* Variables globales */
static struct disco_pipe *waiting_writer = NULL;
static struct disco_pipe *waiting_reader = NULL;
static KMutex global_mutex;

/* Initialize the module */
int disco_init(void) {
    int rc;
    rc = register_chrdev(disco_major, "disco", &disco_fops);
    if (rc < 0) {
        printk("<1>disco: cannot obtain major number %d\n", disco_major);
        return rc;
    }

    waiting_writer = NULL;
    waiting_reader = NULL;
    m_init(&global_mutex);

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

    // filp->private_data = pipe; // COMENTADO

    m_lock(&global_mutex);

    // Logic to pair reader and writer
    if (filp->f_mode & FMODE_WRITE) {
        if (waiting_reader) {
            // Emparejar con el lector en espera
            filp->private_data = waiting_reader;
            waiting_reader = NULL;
            c_broadcast(&((struct disco_pipe *)filp->private_data)->cond);
        } else {
            // No hay lectores en espera, poner este escritor en espera
            waiting_writer = pipe;
            filp->private_data = pipe;
            while (waiting_writer == pipe) {
                if (c_wait(&global_mutex.cond, &global_mutex)) {
                    waiting_writer = NULL;
                    kfree(pipe->buffer);
                    kfree(pipe);
                    m_unlock(&global_mutex);
                    return -EINTR;
                }
            }
        }
    } else if (filp->f_mode & FMODE_READ) {
        if (waiting_writer) {
            // Emparejar con el escritor en espera
            filp->private_data = waiting_writer;
            waiting_writer = NULL;
            c_broadcast(&((struct disco_pipe *)filp->private_data)->cond);
        } else {
            // No hay escritores en espera, poner este lector en espera
            waiting_reader = pipe;
            filp->private_data = pipe;
            while (waiting_reader == pipe) {
                if (c_wait(&global_mutex.cond, &global_mutex)) {
                    waiting_reader = NULL;
                    kfree(pipe->buffer);
                    kfree(pipe);
                    m_unlock(&global_mutex);
                    return -EINTR;
                }
            }
        }
    }

    m_unlock(&global_mutex);

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

    while (count > 0) {
        while (pipe->size == MAX_SIZE) {
            if (c_wait(&pipe->cond, &pipe->mutex)) {
                ret = -EINTR;
                goto epilog;
            }
        }

        pipe->buffer[pipe->in] = *buf;
        pipe->in = (pipe->in + 1) % MAX_SIZE;
        pipe->size++;
        buf++;
        count--;
        ret++;

        c_broadcast(&pipe->cond);
    }

epilog:
    m_unlock(&pipe->mutex);
    return ret;
}
#endif

/* Necessary includes for device drivers */
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h> /* printk() */
#include <linux/slab.h> /* kmalloc() */
#include <linux/fs.h> /* everything... */
#include <linux/errno.h> /* error codes */
#include <linux/types.h> /* size_t */
#include <linux/proc_fs.h>
#include <linux/fcntl.h> /* O_ACCMODE */
#include <linux/uaccess.h> /* copy_from/to_user */
#include <linux/wait.h> /* wait queues */

#include "kmutex.h"

MODULE_LICENSE("Dual BSD/GPL");

/* Declaration of disco.c functions */
static int disco_open(struct inode *inode, struct file *filp);
static int disco_release(struct inode *inode, struct file *filp);
static ssize_t disco_read(struct file *filp, char *buf, size_t count, loff_t *f_pos);
static ssize_t disco_write(struct file *filp, const char *buf, size_t count, loff_t *f_pos);
void disco_exit(void);
int disco_init(void);

/* Structure that declares the usual file access functions */
struct file_operations disco_fops = {
  .read = disco_read,
  .write = disco_write,
  .open = disco_open,
  .release = disco_release
};

/* Declaration of the init and exit functions */
module_init(disco_init);
module_exit(disco_exit);

#define TRUE 1
#define FALSE 0

/* Global variables of the driver */

int disco_major = 61; /* Major number */

/* Buffer to store data */
#define MAX_SIZE 8192

static char *disco_buffer;
static ssize_t curr_size;

/* Mutex and condition variables */
static KMutex mutex;
static KCondition cond;

static wait_queue_head_t read_queue;
static wait_queue_head_t write_queue;

static int waiting_reader = 0;
static int waiting_writer = 0;

static struct file *reader_filp = NULL;
static struct file *writer_filp = NULL;

int disco_init(void) {
  int rc;

  /* Registering device */
  rc = register_chrdev(disco_major, "disco", &disco_fops);
  if (rc < 0) {
    printk("<1>disco: cannot obtain major number %d\n", disco_major);
    return rc;
  }

  curr_size = 0;
  m_init(&mutex);
  c_init(&cond);

  /* Allocating disco_buffer */
  disco_buffer = kmalloc(MAX_SIZE, GFP_KERNEL);
  if (disco_buffer == NULL) {
    disco_exit();
    return -ENOMEM;
  }
  memset(disco_buffer, 0, MAX_SIZE);

  init_waitqueue_head(&read_queue);
  init_waitqueue_head(&write_queue);

  printk("<1>Inserting disco module\n");
  return 0;
}

void disco_exit(void) {
  /* Freeing the major number */
  unregister_chrdev(disco_major, "disco");

  /* Freeing buffer disco */
  if (disco_buffer) {
    kfree(disco_buffer);
  }

  printk("<1>Removing disco module\n");
}

static int disco_open(struct inode *inode, struct file *filp) {
  int rc = 0;

  m_lock(&mutex);

  if (filp->f_mode & FMODE_WRITE) {
    waiting_writer++;
    if (waiting_reader > 0) {
      /* Pair with the waiting reader */
      writer_filp = filp;
      waiting_writer--;
      waiting_reader--;
      wake_up(&read_queue);
    } else {
      /* Wait for a reader */
      m_unlock(&mutex);
      wait_event(write_queue, reader_filp != NULL);
      m_lock(&mutex);
    }
  } else if (filp->f_mode & FMODE_READ) {
    waiting_reader++;
    if (waiting_writer > 0) {
      /* Pair with the waiting writer */
      reader_filp = filp;
      waiting_reader--;
      waiting_writer--;
      wake_up(&write_queue);
    } else {
      /* Wait for a writer */
      m_unlock(&mutex);
      wait_event(read_queue, writer_filp != NULL);
      m_lock(&mutex);
    }
  }

  m_unlock(&mutex);
  return rc;
}

static int disco_release(struct inode *inode, struct file *filp) {
  m_lock(&mutex);

  if (filp->f_mode & FMODE_WRITE) {
    writer_filp = NULL;
    wake_up(&read_queue);
  } else if (filp->f_mode & FMODE_READ) {
    reader_filp = NULL;
    wake_up(&write_queue);
  }

  m_unlock(&mutex);
  return 0;
}

static ssize_t disco_read(struct file *filp, char *buf, size_t count, loff_t *f_pos) {
  ssize_t rc;

  m_lock(&mutex);

  while (curr_size <= *f_pos && writer_filp) {
    /* If reader is at the end of the buffer but there is a process writing to the buffer, reader waits. */
    if (c_wait(&cond, &mutex)) {
      printk("<1>read interrupted\n");
      rc = -EINTR;
      goto epilog;
    }
  }

  if (count > curr_size - *f_pos) {
    count = curr_size - *f_pos;
  }

  printk("<1>read %d bytes at %d\n", (int)count, (int)*f_pos);

  /* Transferring data to user space */
  if (copy_to_user(buf, disco_buffer + *f_pos, count) != 0) {
    /* the value of buf is an invalid address */
    rc = -EFAULT;
    goto epilog;
  }

  *f_pos += count;
  rc = count;

epilog:
  m_unlock(&mutex);
  return rc;
}

static ssize_t disco_write(struct file *filp, const char *buf, size_t count, loff_t *f_pos) {
  ssize_t rc;
  loff_t last;

  m_lock(&mutex);

  last = *f_pos + count;
  if (last > MAX_SIZE) {
    count -= last - MAX_SIZE;
  }
  printk("<1>write %d bytes at %d\n", (int)count, (int)*f_pos);

  /* Transferring data from user space */
  if (copy_from_user(disco_buffer + *f_pos, buf, count) != 0) {
    /* the value of buf is an invalid address */
    rc = -EFAULT;
    goto epilog;
  }

  *f_pos += count;
  curr_size = *f_pos;
  rc = count;
  c_broadcast(&cond);

epilog:
  m_unlock(&mutex);
  return rc;
}