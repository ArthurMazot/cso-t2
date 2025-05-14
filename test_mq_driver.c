
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>

#define SIZE_BUFF 256

int main(){
    int fd = open("/dev/mq_driver", 2);
    char buff[SIZE_BUFF];
    if(fd < 0)
        return 1;

    if(write(fd, "1 nomeQualquer\0", strlen("1 nomeQualquer\0"))){
        printf("Adicionado\n");
    }
    else return 0;

    fgets(buff, SIZE_BUFF, stdin);

    if(write(fd, "3 Teste de msg 1\0", strlen("3 Teste de msg 1\0"))){
        printf("Teste mandado\n");
    }

    if(write(fd, "3 Teste de msg 2\0", strlen("3 Teste de msg 2\0"))){
        printf("Teste mandado\n");
    }

    if(write(fd, "3 Teste de msg 3\0", strlen("3 Teste de msg 3\0"))){
        printf("Teste mandado\n");
    }

    if(write(fd, "3 Teste de msg 4\0", strlen("3 Teste de msg 4\0"))){
        printf("Teste mandado\n");
    }

    fgets(buff, SIZE_BUFF, stdin);

    if(read(fd, "", strlen("")));
    if(read(fd, "", strlen("")));
    if(read(fd, "", strlen("")));
    if(read(fd, "", strlen("")));


    write(fd, "4 nomeQualquer\0", strlen("4 nomeQualquer\0"));

    close(fd);
}