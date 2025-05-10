
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

    fgets(buff, SIZE_BUFF, stdin);

    if(write(fd, "3 Alo!!!\0", strlen("3 Alo!!!\0"))){
        printf("Mandando Alo!!!\n");
    }

    fgets(buff, SIZE_BUFF, stdin);

    if(write(fd, "4 \0", strlen("4 \0"))){
        printf("Removido\n");
    }
    close(fd);
}