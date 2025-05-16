#include <linux/init.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
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
static int qntPro = 0;
static int qntMsg = 0;
static int tamMsg = 0;

module_param(qntPro, int, 0644);
module_param(qntMsg, int, 0644);
module_param(tamMsg, int, 0644);

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

char *strtok(char *buff){
    int i = 0;
    while(buff[i++] != ' ');
    buff[i-1] = '\0';
    return buff+i;}

//========================================//

char estaRegistrado(char *nome, char flag){
    int i = 0;
    struct nodo *n = NULL;
    list_for_each_entry(n, &list, link){
        if(i++ >= count) break;
        if(strcmp(n->nome, nome) == 0){
            if(flag) printk(KERN_ALERT "[REG] Nome '%s' já está em uso.\n", nome); //printk(KERN_ALERT "Nome já utilizado\n");
            return 1;}
        if(n->pid == task_pid_nr(current)){
            if(flag) printk(KERN_ALERT "[REG] Este processo já está registrado.\n"); //printk(KERN_ALERT "Este processo ja está registrado\n");
            return 1;}}
    return 0;}

//========================================//

char registraProcesso(char *nome){
    struct nodo *n = NULL;
    if(count >= qntPro){ //se a lista está cheia
        printk(KERN_ALERT "[REG] Lista Cheia - Limite de processos atingido (%d).\n", qntPro);
        return 0;}

    if(!nome && !nome[0]){//se não recebeu um nome e o nome é vazio
        printk(KERN_ALERT "[REG] Nome inválido (vazio ou nulo).\n");
        return 0;}

    if(estaRegistrado(nome, 1)) //ja está registrado
        return 0;

    count++;
    n = kmalloc((sizeof(struct nodo)), GFP_KERNEL);
    n->pid = task_pid_nr(current);
    n->nome = kmalloc(sizeof(char)*strlen(nome), GFP_KERNEL);
    n->tam = 0;
    strcpy(n->nome, nome);
    INIT_LIST_HEAD(&n->mensagens);
    list_add(&(n->link), &list);
    printk(KERN_INFO "[REG] Processo '%s' registrado com sucesso (PID=%d).\n", nome, current->pid);
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
    if(flag) printk(KERN_INFO "[READ] Mensagem lida: \"%s\"\n", m->string);  //printk(KERN_INFO "%s\n", m->string);
	list_del(&m->link);
    kfree(m->string);
	kfree(m);
	return 0;}

//========================================//

char mandaMsg(char *buff){
    int i = 0;
    struct nodo *n = NULL;
    char *nome = buff;
    buff = strtok(buff);
    if(!estaRegistrado(nome, 0)){
        printk(KERN_ALERT "[MSG] Processo '%s' não está registrado.\n", nome);
        return 0;}

    if(strlen(buff) >= tamMsg){
        printk(KERN_ALERT "[MSG] Mensagem muito grande. Não enviada.\n");
        return 0;}

    list_for_each_entry(n, &list, link){
        if(i++ >= qntPro) break;
        if(strcmp(n->nome, nome) == 0){
            if(n->tam >= qntMsg){
                printk(KERN_INFO  "[MSG] Lista de mensagens de '%s' cheia. Removendo a mais antiga...\n", nome);
                rmMsg(&n->mensagens, 0);
                n->tam--;}
            addMsg(&n->mensagens, buff);
            n->tam++;
            printk(KERN_INFO "[MSG] Mensagem enviada ao processo '%s'.\n", nome);
            return 1;}}
    return 0;}

//========================================//

char msgAll(char *buff){
    int i = 0;
    struct nodo *n = NULL;
    if(count <= 0){
        printk(KERN_ALERT "[MSG-ALL] Lista de processos vazia. Nenhum destinatário.\n");
        return 0;}

    if(strlen(buff) >= tamMsg){
        printk(KERN_ALERT "[MSG-ALL] Mensagem muito grande. Não enviada.\n");
        return 0;}
    
    list_for_each_entry(n, &list, link){
        if(i++ >= qntPro) break;
        if(n->tam >= qntMsg){
            printk(KERN_INFO "[MSG-ALL] Lista de mensagens cheia para '%s'. Removendo a mais antiga... \n", n->nome);
            rmMsg(&n->mensagens, 0);
            n->tam--;}
        addMsg(&n->mensagens, buff);
        n->tam++;}
    printk(KERN_INFO "[MSG-ALL] Mensagem enviada para todos os processos.\n");
    return 1;}

//========================================//

char unReg(char *nome){
    int i = 0;
    struct nodo *n = NULL;
    if(count <= 0){
        printk(KERN_ALERT "[UNREG] Lista de processos vazia.\n");
        return 0;}

    list_for_each_entry(n, &list, link){
        if(i++ >= count) break;
        if(strcmp(n->nome, nome) == 0){
            if(n->pid == task_pid_nr(current)){
                while(n->tam--) rmMsg(&n->mensagens, 0);
                kfree(n->nome);
                list_del(&(n->link));
                kfree(n);
                count--;
                printk(KERN_INFO "[UNREG] Processo '%s' removido com sucesso.\n", nome);
                return 1;}
            printk(KERN_ALERT "[UNREG] Este processo não registrou %s. Processo não removido.\n", nome);
            return 0;}}
    printk(KERN_ALERT "[UNREG] Processo '%s' não registrado.\n", nome);
    return 0;}

//========================================//

void clearList(void){
    struct nodo *n = NULL;
    if(list_empty(&list))
        return;

    list_for_each_entry(n, &list, link){
        if(count <= 0) break;
        while(n->tam--) rmMsg(&n->mensagens, 0);
        kfree(n->nome);
        list_del(&(n->link));
        kfree(n);
        count--;}
    }

//========================================//

static int mq_init(void){
    if(qntPro <= 0 || qntMsg <= 0 || tamMsg <= 0){
        printk(KERN_ALERT "[INIT] Parâmetros inválidos.");  
        return 1;}

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
    printk(KERN_INFO "\n=====================\n");
    printk(KERN_INFO "[MQ] Módulo carregado com sucesso!\n");
    printk(KERN_INFO "[MQ] Limites => Processos: %d | Mensagens: %d | Tam/msg: %d\n", qntPro, qntMsg, tamMsg);
    printk(KERN_INFO "=====================\n");
    return 0;}


//========================================//

static void mq_exit(void){
    clearList();
    device_destroy(charClass, MKDEV(majorNumber, 0));
	class_unregister(charClass);
	class_destroy(charClass);
	unregister_chrdev(majorNumber, DEVICE_NAME);
    printk(KERN_INFO "\n=====================\n");
    printk(KERN_INFO "[MQ] Módulo descarregado. Recursos liberados.\n");
    printk(KERN_INFO "=====================\n");
}

//========================================//

static int dev_open(struct inode *inodep, struct file *filep){
    return 0;}

//========================================//

static ssize_t dev_read(struct file *filep, char *buff, size_t size, loff_t *offset){
    int i = 0;
    struct nodo *n = NULL;
    if(count <= 0){
        printk(KERN_ALERT "[READ] Lista de processos vazia.\n");
        return 0;}

    list_for_each_entry(n, &list, link){
        if(i++ >= count) break;
        if(n->pid == task_pid_nr(current)){
            if(n->tam <= 0){
                printk(KERN_ALERT "[READ] Lista de mensagens vazia.\n");
                return 0;}
            rmMsg(&n->mensagens, 1);
            n->tam--;
            return 1;}}

    printk(KERN_ALERT "[READ] Processo não registrado.\n");
    return 0;}

//========================================//

static ssize_t dev_write(struct file *filep, const char *buff, size_t size, loff_t *offset){
    char aux[256];
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

    return ret;}

//========================================//

static int dev_release(struct inode *inodep, struct file *filep){
    return 0;}

module_init(mq_init);
module_exit(mq_exit);