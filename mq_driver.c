#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/list.h>
#include <linux/slab.h>
#include "list_driver.h"

#define DEVICE_NAME "mq_driver"
#define CLASS_NAME  "mq_class"

static int majorNumber;
static struct class *charClass = NULL;
static struct device *charDevice = NULL;

struct list_head list;
int qntPro;
int qntMsg;
int tamMsg;

//========================================//

struct nodo{
    struct list_head link;
    char **mensagens;
    int inicio;
    int fim;
};

//========================================//

static struct file_operations fops =
{
	.open = dev_open,
	.read = dev_read,
	.write = dev_write,
	.release = dev_release,
};

//========================================//

static int mq_init(int qntP, int qntM, int tamM){
    majorNumber = register_chrdev(0, DEVICE_NAME, &fops);
    if(majorNumber < 0) return majorNumber;

    charClass = class_create(THIS_MODULE, CLASS_NAME);
    if(IS_ERR(charClass)){
        unregister_chrdev(majorNumber, DEVICE_NAME);
        return PTR_ERR(charClass);}

    charDevice = device_create(charClass, NULL, MKDEV(majorNumber, 0), NULL, DEVICE_NAME);
    if(IS_ERR(charDevice)) {
		class_destroy(charClass);
		unregister_chrdev(majorNumber, DEVICE_NAME);
		return PTR_ERR(charDevice);}

    qntPro = qntP;
    qntMsg = qntM;
    tamMsg = tamM;

    INIT_LIST_HEAD(&list);
    for(int i = 0; i < qntPro; i++){ //aloca tudo que precisa (da pra fazer melhor), mas esse jeito deve funcionar
        struct nodo *newNodo = kmalloc((sizeof(struct nodo)), GFP_KERNEL);
        newNodo->mensagens = kmalloc((sizeof(char*)), GFP_KERNEL);
        for(int j = 0; j < qntMsg; j++)
            newNodo->mensagens[j] = kmalloc((sizeof(char)), GFP_KERNEL);
        list_add_tail(&(NewNodo->link), &list);}}


//========================================//

static void mq_exit(void){
    struct nodo *n = NULL;
    list_for_each_entry(n, &list, link){
        for(int i = 0; i < qntMsg; i++)
            kfree(n->mensagens[i]);
        kfree(n->mensagens);
        kfree(n);}

    device_destroy(charClass, MKDEV(majorNumber, 0));
	class_unregister(charClass);
	class_destroy(charClass);
	unregister_chrdev(majorNumber, DEVICE_NAME);}

//========================================//

module_init(mq_init);
module_exit(mq_exit);