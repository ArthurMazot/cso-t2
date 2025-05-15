Parte do teste do driver
Tu precisa transforamar o que o usuario vai digitar no que tem no "buff"

Usuario digita ->
/reg [nome do processo]


buff = "1 [nome do processo]"
write(fd, buff, strlen(buff)) -> Bota um processo na lista

//=============================//

Usuario digita -> 
/[nome do processo] [mensagem]


buff = "2 [nome do processo] [mensagem]"
write(fd, buff, strlen(buff)) -> Envia uma mensagem para um processo

//=============================//

Usuario digita ->
/all [mensagem]

buff = "3 [mensagem]"
write(fd, buff, strlen(buff)) -> manda mensagem para todos os processos na lista

//=============================//

Usuario digita ->
/unreg [nome do processo]


buff = "4 [nome do processo]"
write(fd, buff, strlen(buff)) -> tira um processo da lista

//=============================//

Pra ler as mensagens não tem no trabalho como vai ser que o usuario vai pedir então acho que um "/read" ta bom

Usuario digita -> 
/read

read(fd, "", strlen("")) -> le uma mensagem

//=============================//
A parte dos alertas ja ta feita então o driver diz se a lista ta cheia por exemplo


Parte dos parametros
Se tu quiser ver como ele fez ta na aula do dia 25/04 - tutorial 2.3
No main_driver.c tem minhas tentativas falhas de os passar parametros, elas tão comentadas
// module_param(qntPro, int, 0644);
// module_param(qntMsg, int, 0644);
// module_param(tamMsg, int, 0644);
Aparentemente eu coloquei tudo que precisava mas não funciona então não sei
se tu conseguir fazer essa bomba funionar tem que mudar essas partes

static int qntPro = 2;
static int qntMsg = 2;
static int tamMsg = 16;
troca todos = pra 0

E descomentar isso la na função mq_init
// if(qntPro <= 0 || qntMsg <= 0 || tamMsg <= 0){
//     printk(KERN_ALERT "Parametros inválidos\n");    
//     return 1;}