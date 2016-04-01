#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

int main()
{
        pid_t pid;

        pid = fork();
        if(pid == -1) {
                printf("fork erro\n");
                exit(0);
        }

        if(pid == 0) {
                system("python udp.py");
        }
        else {
                system("raspivid -t 0 -w 720 -h 480 -hf -ih -fps 60 -o - | nc -k -l 2222");
        }
        return 0;

}

