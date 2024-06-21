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
#define TRUE 1
#define FALSE 0

/* Declaration of DRIVERNAME.c functions */
int DRIVERNAME_open(struct inode *inode, struct file *filp);
int DRIVERNAME_release(struct inode *inode, struct file *filp);
ssize_t DRIVERNAME_read(struct file *filp, char *buf, size_t count, loff_t *f_pos);
ssize_t DRIVERNAME_write(struct file *filp, const char *buf, size_t count, loff_t *f_pos);
void DRIVERNAME_exit(void);
int DRIVERNAME_init(void);

/* Structure that declares the usual file */
/* access functions */
struct file_operations DRIVERNAME_fops = {
  read: DRIVERNAME_read,
  write: DRIVERNAME_write,
  open: DRIVERNAME_open,
  release: DRIVERNAME_release
};

/* Declaration of the init and exit functions */
module_init(DRIVERNAME_init);
module_exit(DRIVERNAME_exit);

/* -------- El driver [USO DEL DRIVER] -------- */


/* Global variables of the driver */

int DRIVERNAME_major = 61;     /* podría ser cualquier otro, es 61 porque si*/

/*  --- vars de ejemplo ---
#define MAX_SIZE 8192

static char *DRIVERNAME_buffer;
static ssize_t curr_size;
static int readers;
static int writing;
static int pend_open_write;
    ------------------------
*/

/* El mutex y la condicion para DRIVERNAME */
static KMutex mutex;
static KCondition cond;

int DRIVERNAME_init(void) {
  int rc;

  /* Registrar el dispositivo */
  rc = register_chrdev(DRIVERNAME_major, "DRIVERNAME", &DRIVERNAME_fops);
  if (rc < 0) {
    printk(
      "<1>DRIVERNAME: cannot obtain major number %d\n", DRIVERNAME_major);
    return rc;
  }

  /* Todo lo que debería inicializar */


  return 0;
}

void DRIVERNAME_exit(void) {
  /* Liberar numero major */
  unregister_chrdev(DRIVERNAME_major, "DRIVERNAME");

  /* Liberar memoria de variables del driver */


  printk("<1>Removing DRIVERNAME module\n");
}

int DRIVERNAME_open(struct inode *inode, struct file *filp) {
  int rc= 0;
  m_lock(&mutex);

  if (filp->f_mode & FMODE_WRITE) {
    int rc;
    printk("<1>open request for write\n");
    
    /* Trabajo aca */

    printk("<1>open for write successful\n");
  } else if (filp->f_mode & FMODE_READ) {

    /* Trabajo aca */

    printk("<1>open for read\n");
  }

  m_unlock(&mutex);
  return rc;
}

int DRIVERNAME_release(struct inode *inode, struct file *filp) {
  m_lock(&mutex);
  if (filp->f_mode & FMODE_WRITE) {

    /* Trabajo aca */

    printk("<1>close for write successful\n");
  }
  else if (filp->f_mode & FMODE_READ) {

    /* Trabajo aca */

    printk("<1>close for read (readers remaining=%d)\n", readers);
  }

  m_unlock(&mutex);
  return 0;
}

ssize_t DRIVERNAME_read(struct file *filp, char *buf,
                    size_t count, loff_t *f_pos) {
  ssize_t rc;
  m_lock(&mutex);

  /* Trabajo aca */

  m_unlock(&mutex);
  return rc;
}

ssize_t DRIVERNAME_write( struct file *filp, const char *buf,
                      size_t count, loff_t *f_pos) {
  ssize_t rc;
  loff_t last;

  m_lock(&mutex);

  /* Trabajo aca */

  m_unlock(&mutex);
  return rc;
}

