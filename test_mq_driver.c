
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

    while (1) {
        printf("\n> ");
        fflush(stdout);
        memset(buff, 0, SIZE_BUFF);
        fgets(buff, SIZE_BUFF, stdin);
        buff[strcspn(buff, "\n")] = 0;  // Remove '\n'

        if (strncmp(buff, "/exit", 5) == 0) {
            break;

        } else if (strncmp(buff, "/read", 5) == 0) {
            read(fd, "", strlen("")); 

        } else if (strncmp(buff, "/reg ", 5) == 0) {
            snprintf(buff, SIZE_BUFF, "1 %s", buff + 5);
            write(fd, buff, strlen(buff));

        } else if (strncmp(buff, "/unreg ", 7) == 0) {
            snprintf(buff, SIZE_BUFF, "4 %s", buff + 7);
            write(fd, buff, strlen(buff)); 

        } else if (strncmp(buff, "/[all] ", 7) == 0) {
            snprintf(buff, SIZE_BUFF, "3 %s", buff + 7);
            write(fd, buff, strlen(buff)); 

        } else if (buff[0] == '/' && buff[1] != '[') {
                snprintf(buff, SIZE_BUFF, "2 %s", buff + 1);
                write(fd, buff, strlen(buff));  
        }
    }

    close(fd);
    return 0;
}
