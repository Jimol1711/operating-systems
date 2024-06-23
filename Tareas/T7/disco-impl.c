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

/* Structures for maintaining state */
static char *disco_buffer;
static ssize_t buffer_size;

/* Synchronization variables */
static KMutex mutex;
static KCondition cond;

/* File pointers for pairing processes */
static struct file *writer = NULL;
static struct file *reader = NULL;

/* Initialize the module */
int disco_init(void) {
  int rc;

  /* Registering device */
  rc = register_chrdev(disco_major, "disco", &disco_fops);
  if (rc < 0) {
    printk("<1>disco: cannot obtain major number %d\n", disco_major);
    return rc;
  }

  m_init(&mutex);
  c_init(&cond);

  /* Allocating disco_buffer */
  disco_buffer = kmalloc(MAX_SIZE, GFP_KERNEL);
  if (!disco_buffer) {
    disco_exit();
    return -ENOMEM;
  }
  memset(disco_buffer, 0, MAX_SIZE);

  buffer_size = 0;

  printk("<1>Inserting disco module\n");
  return 0;
}

/* Clean up the module */
void disco_exit(void) {
  /* Freeing the major number */
  unregister_chrdev(disco_major, "disco");

  /* Freeing buffer */
  if (disco_buffer) {
    kfree(disco_buffer);
  }

  printk("<1>Removing disco module\n");
}

/* Open the device */
int disco_open(struct inode *inode, struct file *filp) {
  int rc = 0;

  m_lock(&mutex);

  if (filp->f_mode & FMODE_WRITE) {
    if (reader) {
      writer = filp;
      c_signal(&cond); /* Signal the reader */
    } else {
      writer = filp;
      while (!reader) {
        if (c_wait(&cond, &mutex)) {
          rc = -EINTR;
          writer = NULL;
          goto epilog;
        }
      }
      c_signal(&cond); /* Signal the reader */
    }
  } else if (filp->f_mode & FMODE_READ) {
    if (writer) {
      reader = filp;
      c_signal(&cond); /* Signal the writer */
    } else {
      reader = filp;
      while (!writer) {
        if (c_wait(&cond, &mutex)) {
          rc = -EINTR;
          reader = NULL;
          goto epilog;
        }
      }
      c_signal(&cond); /* Signal the writer */
    }
  }

epilog:
  m_unlock(&mutex);
  return rc;
}

/* Release the device */
int disco_release(struct inode *inode, struct file *filp) {
  m_lock(&mutex);

  if (filp == writer) {
    writer = NULL;
    c_broadcast(&cond); /* Notify all waiting readers */
  } else if (filp == reader) {
    reader = NULL;
    c_broadcast(&cond); /* Notify all waiting writers */
  }

  m_unlock(&mutex);
  return 0;
}

/* Read from the device */
ssize_t disco_read(struct file *filp, char *buf, size_t count, loff_t *f_pos) {
  ssize_t rc;

  m_lock(&mutex);

  while (buffer_size == 0 && writer) {
    if (c_wait(&cond, &mutex)) {
      rc = -EINTR;
      goto epilog;
    }
  }

  if (count > buffer_size) {
    count = buffer_size;
  }

  if (copy_to_user(buf, disco_buffer, count) != 0) {
    rc = -EFAULT;
    goto epilog;
  }

  buffer_size = 0;
  rc = count;
  c_signal(&cond); /* Notify the writer */

epilog:
  m_unlock(&mutex);
  return rc;
}

/* Write to the device */
ssize_t disco_write(struct file *filp, const char *buf, size_t count, loff_t *f_pos) {
  ssize_t rc;

  m_lock(&mutex);

  while (buffer_size > 0 && reader) {
    if (c_wait(&cond, &mutex)) {
      rc = -EINTR;
      goto epilog;
    }
  }

  if (count > MAX_SIZE) {
    count = MAX_SIZE;
  }

  if (copy_from_user(disco_buffer, buf, count) != 0) {
    rc = -EFAULT;
    goto epilog;
  }

  buffer_size = count;
  rc = count;
  c_signal(&cond); /* Notify the reader */

epilog:
  m_unlock(&mutex);
  return rc;
}