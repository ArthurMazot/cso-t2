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
static int qntPro = 2;
static int qntMsg = 2;
static int tamMsg = 16;

// module_param(qntPro, int, 0644);
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

void msg_show(struct list_head *mesagens){
    struct mensagem *m = NULL;
    list_for_each_entry(m, mesagens, link){
        printk(KERN_INFO "Mensagem: %s", m->string);
    }
}

//========================================//

void list_show(void){
	struct nodo *n = NULL;
    if(count <= 0){
        printk(KERN_INFO "Lista vazia\n");
        return;}

	list_for_each_entry(n, &list, link){
		printk(KERN_INFO "PID: %d, Nome: %s\n", n->pid, n->nome);
        printk(KERN_INFO "Tam: %d", n->tam);
        msg_show(&n->mensagens);
        printk(KERN_INFO "\n\n");}}

//========================================//

char *strtok(char *buff){
    int i = 0;
    while(buff[i++] != ' ');
    buff[i-1] = '\0';
    return buff+i;}

//========================================//

char estaRegistrado(char *nome){
    struct nodo *n = NULL;
    list_for_each_entry(n, &list, link){
        if(strcmp(n->nome, nome) == 0){
            printk(KERN_ALERT "Nome já utilizado\n");
            return 1;}
        if(n->pid == task_pid_nr(current)){
            printk(KERN_ALERT "Este processo ja está registrado\n");
            return 1;}}
    return 0;}

//========================================//

char registraProcesso(char *nome){
    struct nodo *n = NULL;
    if(count >= qntPro){ //se a lista está cheia
        printk(KERN_ALERT "A lista está cheia\n");
        return 0;}

    if(!nome && !nome[0]){//se não recebeu um nome e o nome é vazio
        printk(KERN_ALERT "Nome inválido\n");
        return 0;}

    if(estaRegistrado(nome)) //ja está registrado
        return 0;

    count++;
    n = kmalloc((sizeof(struct nodo)), GFP_KERNEL);
    n->pid = task_pid_nr(current);
    n->nome = kmalloc(sizeof(char)*strlen(nome), GFP_KERNEL);
    n->tam = 0;
    strcpy(n->nome, nome);
    INIT_LIST_HEAD(&n->mensagens);
    list_add(&(n->link), &list);
    return 1;}

//========================================//

int addMsg(struct list_head *mesagens, char *buff){
	struct mensagem *m = kmalloc((sizeof(struct mensagem)), GFP_KERNEL);
    m->string = kmalloc((strlen(buff)*sizeof(char)), GFP_KERNEL);
	strcpy(m->string, buff);
	list_add_tail(&(m->link), mesagens);
	return 0;}

//========================================//

int rmMsg(struct list_head *mesagens, char flag){
	struct mensagem *m = list_first_entry(mesagens, struct mensagem, link);
    if(flag) printk(KERN_INFO "%s\n", m->string);
	list_del(&m->link);
    kfree(m->string);
	kfree(m);
	return 0;}

//========================================//

char mandaMsg(char *buff){
    struct nodo *n = NULL;
    char *nome = buff;
    buff = strtok(buff);
    if(!estaRegistrado(nome)){
        printk(KERN_ALERT "Este processo não está registrado\n");
        return 0;}

    if(strlen(buff) >= tamMsg){
        printk(KERN_ALERT "Mensagem muito grande\n");
        return 0;}

    list_for_each_entry(n, &list, link){
        if(strcmp(n->nome, nome) == 0){
            if(n->tam >= qntMsg){
                printk(KERN_INFO "Lista de mensagens cheia, a mais antiga será removida\n");
                rmMsg(&n->mensagens, 0);
                n->tam--;}
            addMsg(&n->mensagens, buff);
            n->tam++;
            return 1;}}
    return 0;}

//========================================//

char msgAll(char *buff){
    struct nodo *n = NULL;
    if(count <= 0){
        printk(KERN_ALERT "A Lista de processos está vazia\n");
        return 0;}

    if(strlen(buff) >= tamMsg){
        printk(KERN_ALERT "Mensagem muito grande\n");
        return 0;}
    
    list_for_each_entry(n, &list, link){
        if(n->tam >= qntMsg){
            printk(KERN_INFO "Lista de mensagens cheia, a mais antiga será removida\n");
            rmMsg(&n->mensagens, 0);
            n->tam--;}
        addMsg(&n->mensagens, buff);
        n->tam++;}
    return 1;}

//========================================//

char unReg(char *nome){
    struct nodo *n = NULL;
    if(count <= 0){
        printk(KERN_ALERT "Lista de processos vazia\n");
        return 0;}

    list_for_each_entry(n, &list, link){
        if(strcmp(n->nome, nome) == 0){
            if(n->pid == task_pid_nr(current)){
                while(n->tam--) rmMsg(&n->mensagens, 0);
                kfree(n->nome);
                list_del(&(n->link));
                kfree(n);
                count--;
                return 1;}
            printk(KERN_ALERT "Este processo não registrou %s, processo não removido\n", nome);
            return 0;}}
    printk(KERN_ALERT "O processo %s, não está registrado\n", nome);
    return 0;}

//========================================//

void clearList(void){
    struct nodo *n = NULL;
    if(list_empty(&list))
        return;

    list_for_each_entry(n, &list, link){
        while(n->tam--) rmMsg(&n->mensagens, 0);
        list_del(&(n->link));
        kfree(n->nome);
        kfree(n);
        count--;}}

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
    printk(KERN_INFO "Modulo carregado\n");
    printk(KERN_INFO "%d, %d, %d\n", qntPro, qntMsg, tamMsg);
    return 0;}


//========================================//

static void mq_exit(void){
    clearList();
    device_destroy(charClass, MKDEV(majorNumber, 0));
	class_unregister(charClass);
	class_destroy(charClass);
	unregister_chrdev(majorNumber, DEVICE_NAME);
    printk(KERN_INFO "Modulo descarregado\n");}

//========================================//

static int dev_open(struct inode *inodep, struct file *filep){
    return 0;}

//========================================//

static ssize_t dev_read(struct file *filep, char *buff, size_t size, loff_t *offset){
    struct nodo *n = NULL;
    if(count <= 0){
        printk(KERN_ALERT "Lista de processos vazia\n");
        return 0;}

    list_for_each_entry(n, &list, link){
        if(n->pid == task_pid_nr(current)){
            if(n->tam <= 0){
                printk(KERN_ALERT "Lista de mensagens vazia\n");
                return 0;}
            rmMsg(&n->mensagens, 1);
            n->tam--;
            return 1;}}

    printk(KERN_ALERT "O processo não está registrado\n");
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
        ret = unReg(aux);

    kfree(aux);
    return ret;}

//========================================//

static int dev_release(struct inode *inodep, struct file *filep){
    return 0;}

module_init(mq_init);
module_exit(mq_exit);