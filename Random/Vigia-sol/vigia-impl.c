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
#include <asm/uaccess.h> /* copy_from/to_user */

#include "kmutex.h"

MODULE_LICENSE("Dual BSD/GPL");

/* Declaration of vigia-impl.c functions */
static int vigia_open(struct inode *inode, struct file *filp);
static int vigia_release(struct inode *inode, struct file *filp);
static ssize_t vigia_read(struct file *filp, char *buf, size_t count, loff_t *f_pos);
static ssize_t vigia_write(struct file *filp, const char *buf, size_t count, loff_t *f_pos);

void vigia_exit(void);
int vigia_init(void);

/* Structure that declares the usual file */
/* access functions */
struct file_operations vigia_fops = {
  read: vigia_read,
  write: vigia_write,
  open: vigia_open,
  release: vigia_release
};

/* Declaration of the init and exit functions */
module_init(vigia_init);
module_exit(vigia_exit);

/*** El driver para lecturas sincronas *************************************/

#define TRUE 1
#define FALSE 0

/* Global variables of the driver */

int vigia_major = 61;     /* Major number */

/* Buffer to store data */
#define MAX_SIZE 80
#define NBUF 10

static char *vigia_buffer;
static int in, out, size;

static int nvig;

/* El mutex y la condicion para vigia */
static KMutex mutex;
static KCondition cond;

int vigia_init(void) {
  /* Registering device */
  int rc = register_chrdev(vigia_major, "vigia", &vigia_fops);
  if (rc < 0) {
    printk(
      "<1>vigia: cannot obtain major number %d\n", vigia_major);
    return rc;
  }

  in= out= size= 0;
  m_init(&mutex);
  c_init(&cond);

  nvig= 0;

  /* Allocating vigia_buffer */
  vigia_buffer = kmalloc(MAX_SIZE, GFP_KERNEL);
  if (vigia_buffer==NULL) {
    vigia_exit();
    return -ENOMEM;
  }

  printk("<1>Inserting vigia module\n");
  return 0;
}

void vigia_exit(void) {
  /* Freeing the major number */
  unregister_chrdev(vigia_major, "vigia");

  /* Freeing buffer vigia */
  if (vigia_buffer!=NULL) {
    kfree(vigia_buffer);
  }

  printk("<1>Removing vigia module\n");
}

static int vigia_open(struct inode *inode, struct file *filp) {
  char *mode=   filp->f_mode & FMODE_WRITE ? "write" :
                filp->f_mode & FMODE_READ ? "read" :
                "unknown";
  printk("<1>open %p for %s\n", filp, mode);
  return 0;
}

static int vigia_release(struct inode *inode, struct file *filp) {
  printk("<1>release %p\n", filp);
  return 0;
}

static ssize_t vigia_read(struct file *filp, char *buf,
                    size_t ucount, loff_t *f_pos) {
  ssize_t count= ucount;

  printk("<1>read %p %ld\n", filp, count);
  m_lock(&mutex);

  while (size==0) {
    /* si no hay nada en el buffer, el lector espera */
    if (c_wait(&cond, &mutex)) {
      printk("<1>read interrupted\n");
      count= -EINTR;
      goto epilog;
    }
  }

  if (count > size) {
    count= size;
  }

  /* Transfiriendo datos hacia el espacio del usuario */
  for (int k= 0; k<count; k++) {
    if (copy_to_user(buf+k, vigia_buffer+out, 1)!=0) {
      /* el valor de buf es una direccion invalida */
      count= -EFAULT;
      goto epilog;
    }
    printk("<1>read byte %c (%d) from %d\n",
            vigia_buffer[out], vigia_buffer[out], out);
    out= (out+1)%MAX_SIZE;
    size--;
  }

epilog:
  c_broadcast(&cond);
  m_unlock(&mutex);
  printk("<1>read %p returns %ld\n", filp, count);
  return count;
}

static int put(char *buf) {
  int k;
  int len= strlen(buf);
  for (k= 0; k<len; k++) {
    while (size==MAX_SIZE) {
      /* si el buffer esta lleno, el escritor espera */
      if (c_wait(&cond, &mutex)) {
        printk("<1>write interrupted %d\n", EINTR);
        return -EINTR;
      }
    }

    memcpy(vigia_buffer+in, buf+k, 1);
    printk("<1>write byte %c (%d) at %d\n",
           vigia_buffer[in], vigia_buffer[in], in);
    in= (in+1)%MAX_SIZE;
    size++;
    c_broadcast(&cond);
  }
  return 0;
}

static ssize_t vigia_write( struct file *filp, const char *buf,
                      size_t ucount, loff_t *f_pos) {
  ssize_t count= ucount;
  int my_serial;
  char name[MAX_SIZE+1];

  printk("<1>write %p %ld\n", filp, count);
  m_lock(&mutex);
  my_serial= nvig;

  if (count>MAX_SIZE) count= MAX_SIZE;

  if (copy_from_user(name, buf, count)!=0) {
    /* el valor de buf es una direccion invalida */
    count= -EFAULT;
    goto epilog;
  }

  name[count]= 0;
  printk("<1>entra %d: %s\n", my_serial, name);

  put("entra: "); put(name);

  nvig++;

  c_broadcast(&cond);

  while (nvig<my_serial+3) {
    if (c_wait(&cond, &mutex)!=0) {
      count= -EINTR;
      goto epilog;
    }
  }

  printk("<1>sale %d: %s %d\n", my_serial, name, nvig);
  put("sale: "); put(name);

epilog:
  m_unlock(&mutex);
  printk("<1>read %p returns %ld\n", filp, count);
  return count;
}
