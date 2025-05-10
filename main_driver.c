#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/list.h>
#include <linux/slab.h>

#define DEVICE_NAME "mq_driver"
#define CLASS_NAME  "mq_class"

MODULE_LICENSE("GPL");

static int majorNumber;
static struct class *charClass = NULL;
static struct device *charDevice = NULL;

struct list_head list;
int count = 0; //qnt de processos registrados
static int qntPro = 3;
static int qntMsg = 2;
static int tamMsg = 10;

// module_param(qntPro, int, 0);
// module_param(qntMsg, int, 0644);
// module_param(tamMsg, int, 0644);

static int dev_open(struct inode *, struct file *);
static int dev_release(struct inode *, struct file *);
static ssize_t dev_read(struct file *, char *, size_t, loff_t *);
static ssize_t dev_write(struct file *, const char *, size_t, loff_t *);
//========================================//

struct mensagem{
    struct list_head link;
    char *string;
};

struct nodo{
    struct list_head link;
    struct list_head mensagens;
    char *nome;
    int pid;
    int tam;
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

void list_show(void){
	struct nodo *n = NULL;
    if(count <= 0){
        printk(KERN_INFO "Lista vazia\n");
        return;}

	list_for_each_entry(n, &list, link){
		printk(KERN_INFO "PID: %d, Nome: %s\n", n->pid, n->nome);}}

//========================================//

char *strtok(char *buff){
    int i = 0;
    while(buff[i++] != ' ');
    buff[i-1] = '\0';
    return buff+i;}

//========================================//

char estaRegistrado(void){
    struct nodo *n = NULL;
    list_for_each_entry(n, &list, link)
        if(n->pid == task_pid_nr(current))
            return 1;
    return 0;}

//========================================//

char registraProcesso(char *nome){
    struct nodo *n = NULL;
    if(count > qntPro) //se a lista está cheia
        return 0;

    if(!nome && !nome[0]) //se não recebeu um nome e o nome é vazio
        return 0;

    if(estaRegistrado()) //ja está registrado
        return 0;

    count++;
    n = kmalloc((sizeof(struct nodo)), GFP_KERNEL);
    n->pid = task_pid_nr(current);
    n->nome = kmalloc(sizeof(char)*strlen(nome), GFP_KERNEL);
    n->tam = 0;
    strcpy(n->nome, nome);
    INIT_LIST_HEAD(&(n->mensagens));
    list_add(&(n->link), &list);
    list_show();
    return 1;}

//========================================//

char mandaMsg(char *buff){
    struct nodo *n = NULL;
    struct mensagem *m = NULL;
    char *nome = buff;
    if(!estaRegistrado())
        return 0;

    buff = strtok(buff);
    list_for_each_entry(n, &list, link){
        if(strcmp(n->nome, nome) == 0){
            if(n->tam >= tamMsg){
                list_for_each_entry(m, &(n->mensagens), link);
                kfree(m->string);
                list_del(&(m->link));
                kfree(m);
                n->tam--;}

            n->tam++;
            m = kmalloc((sizeof(struct mensagem)), GFP_KERNEL);
            m->string = kmalloc(tamMsg*sizeof(char), GFP_KERNEL);
            strcpy(m->string, buff);
            list_add(&(m->link), &(n->mensagens));
            return 1;}}
    return 0;}

//========================================//

char msgAll(char *buff){
    struct nodo *n = NULL;
    struct mensagem *m = NULL;

    if(!estaRegistrado())
        return 0;

    if(strlen(buff) >= tamMsg)
        return 0;

    if(count <= 0) //se a lista estiver vazia
        return 0;
    
    list_for_each_entry(n, &list, link){
        if(n->pid != task_pid_nr(current)){
            if(n->tam >= tamMsg){
                list_for_each_entry(m, &(n->mensagens), link);
                kfree(m->string);
                list_del(&(m->link));
                kfree(m);
                n->tam--;}
            n->tam++;
            m = kmalloc((sizeof(struct mensagem)), GFP_KERNEL);
            m->string = kmalloc(tamMsg*sizeof(char), GFP_KERNEL);
            strcpy(m->string, buff);
            list_add(&(m->link), &(n->mensagens));}}
    return 1;}

//========================================//

char unReg(void){
    struct nodo *n = NULL;
    struct mensagem *m = NULL;
    if(count <= 0)
        return 0;

    list_for_each_entry(n, &list, link){
        if(n->pid == task_pid_nr(current)){
            if(n->tam > 0){
                list_for_each_entry(m, &(n->mensagens), link){
                    kfree(m->string);
                    list_del(&(m->link));
                    kfree(m);}}
            kfree(n->nome);
            list_del(&(n->link));
            kfree(n);
            count--;
            list_show();
            return 1;}}
    return 0;}

//========================================//

void clearList(void){
    struct nodo *n = NULL;
    struct mensagem *m = NULL;
    if(list_empty(&list))
        return;

    list_for_each_entry(n, &list, link){
        if(n->tam > 0)
            list_for_each_entry(m, &(n->mensagens), link){
                list_del(&(m->link));
                kfree(m);}
        list_del(&(n->link));
        kfree(n->nome);
        kfree(n);}}

//========================================//

static int mq_init(void){
    // if(qntPro == -1 || qntMsg == -1 || tamMsg == -1)
    //     return 1;

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
    printk(KERN_ALERT "Modulo carregado\n");
    printk(KERN_ALERT "%d, %d, %d\n", qntPro, qntMsg, tamMsg);
    return 0;}


//========================================//

static void mq_exit(void){
    clearList();
    device_destroy(charClass, MKDEV(majorNumber, 0));
	class_unregister(charClass);
	class_destroy(charClass);
	unregister_chrdev(majorNumber, DEVICE_NAME);
    printk(KERN_ALERT "Modulo descarregado\n");}

//========================================//

static int dev_open(struct inode *inodep, struct file *filep){
    return 0;}

//========================================//

static ssize_t dev_read(struct file *filep, char *buff, size_t size, loff_t *offset){
    struct nodo *n = NULL;
    struct mensagem *m = NULL;
    if(list_empty(&list))
        return 0;

    list_for_each_entry(n, &list, link){
        if(n->pid == task_pid_nr(current) && n->tam > 0){
            list_for_each_entry(m, &(n->mensagens), link);
            printk(KERN_ALERT "%s\n", m->string);
            list_del(&(m->link));
            kfree(m);
            n->tam--;
            return 1;}}
    return 0;}

//========================================//

static ssize_t dev_write(struct file *filep, const char *buff, size_t size, loff_t *offset){
    char *aux = kmalloc(sizeof(char)*size, GFP_KERNEL);
    char ret = 0;
    strcpy(aux, buff+2);
    if(buff[0] == '1') //registra um processo
        ret = registraProcesso(aux);
        
    if(buff[0] == '2') //envia mensagem para um processo
        ret = mandaMsg(aux);

    if(buff[0] == '3') //mensagens para todos os processos
        ret = msgAll(aux);

    if(buff[0] == '4') //tira um processo da lista
        ret = unReg();

    kfree(aux);
    return ret;}

//========================================//

static int dev_release(struct inode *inodep, struct file *filep){
    return 0;}

module_init(mq_init);
module_exit(mq_exit);