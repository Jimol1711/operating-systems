#if 0
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
  read = disco_read,
  write = disco_write,
  open = disco_open,
  release = disco_release
};

/* Declaration of the init and exit functions */
module_init(disco_init);
module_exit(disco_exit);

/* Global variables of the driver */

int disco_major = 61; /* Major number */

/* Buffer to store data */
#define MAX_SIZE 8192

typedef struct pipe {
  char *buffer;
  int in, out, size;
  KMutex mutex;
  KCondition cond;
  int closed;
};

static int waiting_reader;
static int waiting_writer;

/* Mutex and condition variables */
static KMutex mutex;
static KCondition cond;

int disco_init(void) {
  int rc;

  /* Registering device */
  rc = register_chrdev(disco_major, "disco", &disco_fops);
  if (rc < 0) {
    printk("<1>disco: cannot obtain major number %d\n", disco_major);
    return rc;
  }

  waiting_reader = 0;
  waiting_writer = 0;
  m_init(&mutex);
  c_init(&cond);

  printk("<1>Inserting disco module\n");
  return 0;
}

void disco_exit(void) {
  /* Freeing the major number */
  unregister_chrdev(disco_major, "disco");
  printk("<1>Removing disco module\n");
}

static int disco_open(struct inode *inode, struct file *filp) {
  int rc = 0;

  m_lock(&mutex);

  if (filp->f_mode & FMODE_WRITE) {
    // modo escritura
    if (waiting_reader) {
      /* Pair with the waiting reader */
      waiting_reader = 0;
      struct pipe *p = kmalloc(sizeof(struct pipe), GFP_KERNEL);
      p->buffer = kmalloc(MAX_SIZE, GFP_KERNEL);
      p->in = p->out = p->size = 0;
      c_broadcast(&cond);
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
#endif

// ESTE FUNCIONA A MEDIAS
#if 0
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
int disco_major = 61;  /* Major number */

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
#endif

#if 1 // ULTIMO INTENTO CHATGPT
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
static int disco_open(struct inode *inode, struct file *filp);
static int disco_release(struct inode *inode, struct file *filp);
static ssize_t disco_read(struct file *filp, char *buf, size_t count, loff_t *f_pos);
static ssize_t disco_write(struct file *filp, const char *buf, size_t count, loff_t *f_pos);

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

#define TRUE 1
#define FALSE 0

/* Global variables of the driver */
int disco_major = 61; /* Major number */
#define MAX_SIZE 8192

/* Buffer and synchronization primitives */
static char *disco_buffer;
static ssize_t curr_size;
static int writer_waiting, reader_waiting;

/* Mutex and condition variables */
static KMutex mutex;
static KCondition cond_reader, cond_writer;

/* Module initialization */
int disco_init(void) {
    int result;

    result = register_chrdev(disco_major, "disco", &disco_fops);
    if (result < 0) {
        printk("<1>disco: cannot obtain major number %d\n", disco_major);
        return result;
    }

    disco_buffer = kmalloc(MAX_SIZE, GFP_KERNEL);
    if (!disco_buffer) {
        unregister_chrdev(disco_major, "disco");
        return -ENOMEM;
    }
    memset(disco_buffer, 0, MAX_SIZE);

    curr_size = 0;
    writer_waiting = reader_waiting = FALSE;

    m_init(&mutex);
    c_init(&cond_reader);
    c_init(&cond_writer);

    printk("<1>Inserting disco module\n");
    return 0;
}

/* Module cleanup */
void disco_exit(void) {
    unregister_chrdev(disco_major, "disco");
    if (disco_buffer) {
        kfree(disco_buffer);
    }
    printk("<1>Removing disco module\n");
}

/* Open function */
static int disco_open(struct inode *inode, struct file *filp) {
    m_lock(&mutex);

    if (filp->f_mode & FMODE_WRITE) {
        while (writer_waiting || reader_waiting) {
            if (c_wait(&cond_writer, &mutex)) {
                m_unlock(&mutex);
                return -EINTR;
            }
        }
        writer_waiting = TRUE;
    } else if (filp->f_mode & FMODE_READ) {
        while (reader_waiting || writer_waiting) {
            if (c_wait(&cond_reader, &mutex)) {
                m_unlock(&mutex);
                return -EINTR;
            }
        }
        reader_waiting = TRUE;
    }

    m_unlock(&mutex);
    return 0;
}

/* Release function */
static int disco_release(struct inode *inode, struct file *filp) {
    m_lock(&mutex);

    if (filp->f_mode & FMODE_WRITE) {
        writer_waiting = FALSE;
        c_signal(&cond_reader);
    } else if (filp->f_mode & FMODE_READ) {
        reader_waiting = FALSE;
        c_signal(&cond_writer);
    }

    m_unlock(&mutex);
    return 0;
}

/* Read function */
static ssize_t disco_read(struct file *filp, char *buf, size_t count, loff_t *f_pos) {
    ssize_t ret = 0;

    m_lock(&mutex);

    while (curr_size == 0 && writer_waiting) {
        if (c_wait(&cond_reader, &mutex)) {
            m_unlock(&mutex);
            return -EINTR;
        }
    }

    if (count > curr_size) {
        count = curr_size;
    }

    if (copy_to_user(buf, disco_buffer, count)) {
        ret = -EFAULT;
        goto out;
    }

    curr_size -= count;
    memmove(disco_buffer, disco_buffer + count, curr_size);
    ret = count;

out:
    c_signal(&cond_writer);
    m_unlock(&mutex);
    return ret;
}

/* Write function */
static ssize_t disco_write(struct file *filp, const char *buf, size_t count, loff_t *f_pos) {
    ssize_t ret = 0;

    m_lock(&mutex);

    while (curr_size == MAX_SIZE && reader_waiting) {
        if (c_wait(&cond_writer, &mutex)) {
            m_unlock(&mutex);
            return -EINTR;
        }
    }

    if (count > MAX_SIZE - curr_size) {
        count = MAX_SIZE - curr_size;
    }

    if (copy_from_user(disco_buffer + curr_size, buf, count)) {
        ret = -EFAULT;
        goto out;
    }

    curr_size += count;
    ret = count;

out:
    c_signal(&cond_reader);
    m_unlock(&mutex);
    return ret;
}

#endif