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
#if 1
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
  printk(KERN_INFO "disco_open: called\n");

  if (filp->f_mode & FMODE_WRITE) {
    if (reader) {
      writer = filp;
      printk(KERN_INFO "disco_open: matched writer with reader\n");
      c_signal(&cond); /* Signal the reader */
    } else {
      writer = filp;
      printk(KERN_INFO "disco_open: writer waiting for reader\n");
      while (!reader) {
        if (c_wait(&cond, &mutex)) {
          rc = -EINTR;
          writer = NULL;
          printk(KERN_INFO "disco_open: writer interrupted\n");
          goto epilog;
        }
      }
      printk(KERN_INFO "disco_open: writer matched with reader\n");
      c_signal(&cond); /* Signal the reader */
    }
  } else if (filp->f_mode & FMODE_READ) {
    if (writer) {
      reader = filp;
      printk(KERN_INFO "disco_open: matched reader with writer\n");
      c_signal(&cond); /* Signal the writer */
    } else {
      reader = filp;
      printk(KERN_INFO "disco_open: reader waiting for writer\n");
      while (!writer) {
        if (c_wait(&cond, &mutex)) {
          rc = -EINTR;
          reader = NULL;
          printk(KERN_INFO "disco_open: reader interrupted\n");
          goto epilog;
        }
      }
      printk(KERN_INFO "disco_open: reader matched with writer\n");
      c_signal(&cond); /* Signal the writer */
    }
  }

epilog:
  m_unlock(&mutex);
  printk(KERN_INFO "disco_open: open successful %p\n", filp);
  return rc;
}

/* Release the device */
int disco_release(struct inode *inode, struct file *filp) {
  m_lock(&mutex);
  printk(KERN_INFO "disco_release: called\n");

  if (filp == writer) {
    writer = NULL;
    printk(KERN_INFO "disco_release: writer released\n");
    c_broadcast(&cond); /* Notify all waiting readers */
  } else if (filp == reader) {
    reader = NULL;
    printk(KERN_INFO "disco_release: reader released\n");
    c_broadcast(&cond); /* Notify all waiting writers */
  }

  m_unlock(&mutex);
  printk(KERN_INFO "disco_release: release successful %p\n", filp);
  return 0;
}

/* Read from the device */
ssize_t disco_read(struct file *filp, char *buf, size_t count, loff_t *f_pos) {
  ssize_t rc;

  m_lock(&mutex);
  printk(KERN_INFO "disco_read: called, buffer_size=%ld\n", buffer_size);

  while (buffer_size == 0 && writer) {
    printk(KERN_INFO "disco_read: waiting for data\n");
    if (c_wait(&cond, &mutex)) {
      rc = -EINTR;
      printk(KERN_INFO "disco_read: interrupted while waiting for data\n");
      goto epilog;
    }
  }

  if (count > buffer_size) {
    count = buffer_size;
  }

  if (copy_to_user(buf, disco_buffer, count) != 0) {
    rc = -EFAULT;
    printk(KERN_ALERT "disco_read: copy_to_user failed\n");
    goto epilog;
  }

  buffer_size = 0;
  rc = count;
  printk(KERN_INFO "disco_read: read %zu bytes\n", count);
  c_signal(&cond); /* Notify the writer */

epilog:
  m_unlock(&mutex);
  return rc;
}

/* Write to the device */
ssize_t disco_write(struct file *filp, const char *buf, size_t count, loff_t *f_pos) {
  ssize_t rc;

  m_lock(&mutex);
  printk(KERN_INFO "disco_write: called, buffer_size=%ld\n", buffer_size);

  while (buffer_size > 0 && reader) {
    printk(KERN_INFO "disco_write: waiting for buffer space\n");
    if (c_wait(&cond, &mutex)) {
      rc = -EINTR;
      printk(KERN_INFO "disco_write: interrupted while waiting for buffer space\n");
      goto epilog;
    }
  }

  if (count > MAX_SIZE) {
    count = MAX_SIZE;
  }

  if (copy_from_user(disco_buffer, buf, count) != 0) {
    rc = -EFAULT;
    printk(KERN_ALERT "disco_write: copy_from_user failed\n");
    goto epilog;
  }

  buffer_size = count;
  rc = count;
  printk(KERN_INFO "disco_write: wrote %zu bytes\n", count);
  c_signal(&cond); /* Notify the reader */

epilog:
  m_unlock(&mutex);
  return rc;
}
#endif
#if 0
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
    read = disco_read,
    write = disco_write,
    open = disco_open,
    release = disco_release
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
#define TRUE 1
#define FALSE 0

/* Mutex y condicion para disco */
KMutex mutex;
KCondition cond;
int waiting_reader = FALSE;
int waiting_writer = FALSE;

int disco_init(void) {
    int rc;

    /* Registering device */
    rc = register_chrdev(disco_major, "disco", &disco_fops);
    if (rc < 0) {
        printk(KERN_ALERT "disco: cannot obtain major number %d\n", disco_major);
        return rc;
    }

    printk(KERN_INFO "Inserting disco module\n");
    return 0;
}

void disco_exit(void) {
    /* Freeing the major number */
    unregister_chrdev(disco_major, "disco");

    printk(KERN_INFO "Removing disco module\n");
}

int disco_open(struct inode *inode, struct file *filp) {

    int rc = 0;
    m_lock(&mutex);

    if (filp->f_mode & FMODE_WRITE) {
      if (waiting_reader) {
        // Pair with the waiting reader
      } else {
        // Wait for a reader
      }
    } else if (filp->f_mode & FMODE_READ) {
      if (waiting_writer) {
        // Pair with the waiting writer
      } else {
        // Wait for a writer
      }
    }

epilog:
  m_unlock(&mutex);
  return rc;
}

int disco_release(struct inode *inode, struct file *filp) {
    struct disco_pipe *pipe = filp->private_data;

    m_lock(&pipe->mutex);

    if (filp->f_mode & FMODE_WRITE) {
        pipe->closed = 1;
        c_broadcast(&pipe->cond);
        printk(KERN_INFO "disco_release: writer closed\n");
    } else if (filp->f_mode & FMODE_READ) {
        if (pipe->writer) {
            pipe->writer = NULL;
        }
        printk(KERN_INFO "disco_release: reader closed\n");
    }

    if (!pipe->writer && !pipe->reader) {
        kfree(pipe->buffer);
        kfree(pipe);
        printk(KERN_INFO "disco_release: pipe freed\n");
    }

    m_unlock(&pipe->mutex);
    printk(KERN_INFO "disco_release: release successful %p\n", filp);
    return 0;
}

ssize_t disco_read(struct file *filp, char *buf, size_t count, loff_t *f_pos) {
  int count = ucount;
  struct disco_pipe pipe = filp->private_data;

  printk("<1>read %p %d\n", filp, count);
  m_lock(&mutex);

  while (pipe->size==0) {
    /* si no hay nada en el pipe, el lector espera */
    if (c_wait(&cond, &mutex)) {
      printk("<1>read interrupted\n");
      count= -EINTR;
      goto epilog;
    }
  }

  if (count > pipe->size) {
    count= pipe->size;
  }

  /* Transfiriendo datos hacia el espacio del usuario */
  for (int k= 0; k<count; k++) {
    if (copy_to_user(buf+k, pipe->buffer+out, 1)!=0) {
      /* el valor de buf es una direccion invalida */
      count= -EFAULT;
      goto epilog;
    }
    printk("<1>read byte %c (%d) from %d\n",
            pipe->buffer[pipe->out], pipe->buffer[pipe->out], pipe->out);
    pipe->out= (pipe->out+1)%MAX_SIZE;
    pipe->size--;
  }

epilog:
  c_broadcast(&cond);
  m_unlock(&mutex);
  return count;
}

ssize_t disco_write(struct file *filp, const char *buf, size_t count, loff_t *f_pos) {
  int count= ucount;
  struct disco_pipe pipe = filp->private_data;

  printk("<1>write %p %d\n", filp, count);
  m_lock(&mutex);

  for (int k= 0; k<count; k++) {
    while (pipe->size==MAX_SIZE) {
      /* si el buffer esta lleno, el escritor espera */
      if (c_wait(&cond, &mutex)) {
        printk("<1>write interrupted\n");
        count= -EINTR;
        goto epilog;
      }
    }

    if (copy_from_user(pipe->buffer+in, buf+k, 1)!=0) {
      /* el valor de buf es una direccion invalida */
      count= -EFAULT;
      goto epilog;
    }
    printk("<1>write byte %c (%d) at %d\n",
           pipe->buffer[pipe->in], pipe->buffer[pipe->in], pipe->in);
    pipe->in= (pipe->in+1)%MAX_SIZE;
    pipe->size++;
    c_broadcast(&cond);
  }

epilog:
  m_unlock(&mutex);
  return count;
}
#endif