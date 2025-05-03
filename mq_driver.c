#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/list.h>
#include <linux/slab.h>

#define DEVICE_NAME "mq_driver"
#define CLASS_NAME  "mq_class"

static int majorNumber;
static struct class *charClass = NULL;
static struct device *charDevice = NULL;

struct list_head list;
int qntPro;
int qntMsg;
int tamMsg;

static int	dev_open(struct inode *, struct file *);
static int	dev_release(struct inode *, struct file *);
static ssize_t	dev_read(struct file *, char *, size_t, loff_t *);
static ssize_t	dev_write(struct file *, const char *, size_t, loff_t *);
//========================================//

struct nodo{
    struct list_head link;
    char *nome;
    int pid;
    char **mensagens;
    int count;
    int escrita;
    int leitura;
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

void inicializaList(int qntP, int qntM, int tamM){
    qntPro = qntP;
    qntMsg = qntM;
    tamMsg = tamM;
    while(qntP--){ //aloca tudo que precisa (da pra fazer melhor), mas esse jeito deve funcionar
        struct nodo *newNodo = kmalloc((sizeof(struct nodo)), GFP_KERNEL);
        newNodo->mensagens = kmalloc((sizeof(char*)), GFP_KERNEL);
        while(qntM){
            newNodo->mensagens[qntM-1] = kmalloc((sizeof(char)), GFP_KERNEL);
            qntM--;}
        newNodo->pid = -1;
        list_add_tail(&(newNodo->link), &list);}}

void mandaMsg(struct nodo *n, const char *buff){
    strcpy(n->mensagens[n->escrita], buff);
    n->escrita = (n->escrita + 1)%qntMsg;
    n->count++;
    if(n->count == qntMsg+1){
        n->count = qntMsg;
        n->leitura = (n->leitura+1)%qntMsg;}}

//========================================//

static int mq_init(void){
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

    INIT_LIST_HEAD(&list);
    return 0;}


//========================================//

static void mq_exit(void){
    struct nodo *n = NULL;
    int i;
    list_for_each_entry(n, &list, link){
        i = 0;
        while(i < qntMsg)
            kfree(n->mensagens[i++]);
        kfree(n->mensagens);
        kfree(n);}

    device_destroy(charClass, MKDEV(majorNumber, 0));
	class_unregister(charClass);
	class_destroy(charClass);
	unregister_chrdev(majorNumber, DEVICE_NAME);}

//========================================//

static int dev_open(struct inode *inodep, struct file *filep){
    return 0;
}

static ssize_t dev_read(struct file *filep, char *buffer, size_t len, loff_t *offset){
    return 0;
}

static ssize_t dev_write(struct file *filep, const char *buff, size_t num, loff_t *offset){
    if(list_empty(&list))
        inicializaList(20, 8, 128);
        
    if(num == 1){ //registra um processo
        struct nodo *n = NULL;
        list_for_each_entry(n, &list, link){
            if(n->pid < 0){
                n->pid = 1; //não sei o que botar, mas os pids tem que ser unicos (mudar essa linha)
                n->count = 0;
                n->escrita = 0;
                n->leitura = 0;
                strcpy(n->nome, buff);
                return 1;}}
        return -1;}

    if(num == 2){ //envia mensagem para um processo
        struct nodo *n = NULL;
        list_for_each_entry(n, &list, link){
            if(strcmp(n->nome, buff) == 0){
                mandaMsg(n, buff);
                return 2;}}
        return -2;}

    if(num == 3){ //mensagens para todos os processos
        struct nodo *n = NULL;
        list_for_each_entry(n, &list, link){
            if(n->pid >= 0)
                mandaMsg(n, buff);}
        return 3;}

    if(num == 4){ //tira um processo da lista
        struct nodo *n = NULL;
        list_for_each_entry(n, &list, link){
            if(strcmp(n->nome, buff) == 0){
                n->pid = -1;
                return 4;}}
        return -4;}
    return 0;
}

static int dev_release(struct inode *inodep, struct file *filep){
    return 0;
}

module_init(mq_init);
module_exit(mq_exit);