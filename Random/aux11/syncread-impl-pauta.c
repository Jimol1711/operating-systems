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

/* Declaration of syncread.c functions */
int syncread_open(struct inode *inode, struct file *filp);
int syncread_release(struct inode *inode, struct file *filp);
ssize_t syncread_read(struct file *filp, char *buf, size_t count, loff_t *f_pos);
ssize_t syncread_write(struct file *filp, const char *buf, size_t count, loff_t *f_pos);
void syncread_exit(void);
int syncread_init(void);

/* Structure that declares the usual file */
/* access functions */
struct file_operations syncread_fops = {
  read: syncread_read,
  write: syncread_write,
  open: syncread_open,
  release: syncread_release
};

/* Declaration of the init and exit functions */
module_init(syncread_init);
module_exit(syncread_exit);

/* -------- El driver para lecturas sincronas -------- */


/* Global variables of the driver */
int syncread_major = 61;     /* podría ser cualquier otro, es 61 porque si*/

/* Buffer to store data */
#define MAX_SIZE 8192

static char *syncread_buffer; /*Este es un pseudo archivo, vive en un buffer (var global) del driver, mucho mas rápido que un archivo */
static ssize_t curr_size;  
static int readers;
static int writing;
static int pend_open_write;

/* El mutex y la condicion para syncread */
static KMutex mutex;
static KCondition cond;

int syncread_init(void) {
  int rc;

  /* Registering device */
  rc = register_chrdev(syncread_major, "syncread", &syncread_fops);
  if (rc < 0) {
    printk(
      "<1>syncread: cannot obtain major number %d\n", syncread_major);
    return rc;
  }

  readers= 0;
  writing= FALSE;
  pend_open_write= 0;
  curr_size= 0;
  m_init(&mutex);
  c_init(&cond);

  /* Allocating syncread_buffer */
  syncread_buffer = kmalloc(MAX_SIZE, GFP_KERNEL);
  if (syncread_buffer==NULL) {
    syncread_exit();
    return -ENOMEM;
  }
  memset(syncread_buffer, 0, MAX_SIZE);

  printk("<1>Inserting syncread module\n");
  return 0;
}

void syncread_exit(void) {
  /* Freeing the major number */
  unregister_chrdev(syncread_major, "syncread");

  /* Liberar memoria del buffer */
  if (syncread_buffer) { // 2: puede que antes haya fallado obtener el buffer, si se logro se libera la memoria
    kfree(syncread_buffer); // 1
  }

  printk("<1>Removing syncread module\n");
}

// inode representa el archivo, filp es el file descriptor de "como se esta intentando acceder", 
// "quien es el propietario" y "como se quiere acceder"
int syncread_open(struct inode *inode, struct file *filp) {
  int rc= 0;
  m_lock(&mutex);

  // hay que ver si el usuario quiere escribir o 
  // filp->f_mode muestra como se quiere acceder

  if (filp->f_mode & FMODE_WRITE) {
    int rc;
    printk("<1>open request for write\n");
    /* Se debe esperar hasta que no hayan otros lectores o escritores */
    pend_open_write++;
    while (writing || readers>0) {
      // se espera con c_wait, pero notese que este se puede despertar por 2 razones
      // 1.- Un broadcast (normal), retorna 0
      // 2.- Una interrupcion (como ctrl+C), retorna numero representativo del error

      if (c_wait(&cond, &mutex)) {
        // hubo un error debemos salir y retornar el error correspondiente
        pend_open_write--;
        c_broadcast(&cond);
        rc= -EINTR;
        goto epilog;
      }
    }

    // yo soy el escritor
    writing= TRUE;
    pend_open_write--;
    curr_size= 0;
    c_broadcast(&cond);
    printk("<1>open for write successful\n");
  }
  else if (filp->f_mode & FMODE_READ) {
    /* (dejar al final, post show)
    
    Para evitar la hambruna de los escritores, si nadie esta escribiendo
     * pero hay escritores pendientes (esperan porque readers>0), los
     * nuevos lectores deben esperar hasta que todos los lectores cierren
     * el dispositivo e ingrese un nuevo escritor.
     */
    while (!writing && pend_open_write>0) {
      if (c_wait(&cond, &mutex)) {
        rc= -EINTR;
        goto epilog;
      }
    }

    // se suma un lector
    readers++;
    printk("<1>open for read\n");
  }

epilog:
  m_unlock(&mutex);
  return rc;
}

int syncread_release(struct inode *inode, struct file *filp) {
  m_lock(&mutex);

  // tambien se debe revisar que tipo de usuario esta saliendo, escritor o lector

  if (filp->f_mode & FMODE_WRITE) {
    writing= FALSE;
    c_broadcast(&cond);
    printk("<1>close for write successful\n");
  }
  else if (filp->f_mode & FMODE_READ) {
    readers--;
    // ultimo lector llama a broadcast para introducir a escritores que esperan
    if (readers==0)
      c_broadcast(&cond);
    printk("<1>close for read (readers remaining=%d)\n", readers);
  }

  m_unlock(&mutex);
  return 0;
}

/* read:
intenta leer de "filp", lo leido se debe depositar en "buf"
cada lector busca leer "count" bytes desde la posicion "f_pos"

debe retornar cuanta es la informacion leida
*/
ssize_t syncread_read(struct file *filp, char *buf,
                    size_t count, loff_t *f_pos) {
  ssize_t rc; // cantidad de bytes que se lograron leer
  m_lock(&mutex);

  /** Como el lector lee desde la posición f_pos, este debe esperar
   *  si no hay mas que f_pos bytes en el buffer. Considerando que
   *  el escritor siga "contectado"
   *
   * si el lector esta en el final del archivo pero hay un proceso
   * escribiendo todavia en el archivo, el lector espera.
   */
  while (curr_size <= *f_pos && writing) {
    
    if (c_wait(&cond, &mutex)) {
      printk("<1>read interrupted\n");
      rc= -EINTR;
      goto epilog;
    }
  }

  // vamos a leer count bytes si es que hay count bytes para leer
  // si no leeremos curr_size -*f_pos

  if (count > curr_size-*f_pos) {
    count= curr_size-*f_pos;
  }

  printk("<1>read %d bytes at %d\n", (int)count, (int)*f_pos);

  /* Transfiriendo datos hacia el espacio del usuario */
  /* hay que transferir los datos leidos al usuario
  * copy_to_user es la unica funcion segura para hacer esto

    copy_to_user copia en el "buf" del usuario desde "lo que hay", "count" bytes de ello 
  *
    recordar que los arreglos son una direccion al 1er elemento y luego podemos sumar para incrementar

  * puede ser que haya un error con la direccion de buf, en tal caso la funcion retorna != 0 
  *
  * */ 
  if (copy_to_user(buf, syncread_buffer+*f_pos, count)!=0) {
    /* el valor de buf es una direccion invalida */
    rc= -EFAULT;
    goto epilog;
  }

  *f_pos+= count; // movemos "el marca paginas"
  rc= count;

epilog:
  m_unlock(&mutex);
  return rc;
}

ssize_t syncread_write( struct file *filp, const char *buf,
                      size_t count, loff_t *f_pos) {
  ssize_t rc;
  loff_t last;

  m_lock(&mutex);

  // debemos escribir count bytes si se puede
  // NO podemos sobrepasar el limite, por lo que hay que limitar cuanto se puede escribir
  last= *f_pos + count;
  if (last>MAX_SIZE) {
    count -= last-MAX_SIZE;
  }
  printk("<1>write %d bytes at %d\n", (int)count, (int)*f_pos);

  /* Transfiriendo datos desde el espacio del usuario */
  // el equivalente del copy_to_user
  if (copy_from_user(syncread_buffer+*f_pos, buf, count)!=0) {
    /* el valor de buf es una direccion invalida */
    rc= -EFAULT;
    goto epilog;
  }

  *f_pos += count;
  curr_size= *f_pos;
  rc= count;
  c_broadcast(&cond);

epilog:
  m_unlock(&mutex);
  return rc;
}

