#include<stdio.h>
#include<unistd.h>
#include<signal.h>
#include<sys/types.h>
#include<sys/wait.h>
#include<stdlib.h>

int wait_flag=1;

void stop(){
    wait_flag = 0;
}


int main(int argc, char** argv){
    pid_t pid1,pid2;
    printf("please choose one:\n");
    int n;
    scanf("%d",&n);
    signal(n,stop);
    while(wait_flag);
    while((pid1 = fork())<0);
    if(pid1>0)//for parent
    {
        while((pid2 = fork())<0);
        if(pid2>0)
        {
            wait_flag = 1;
            
           alarm(3);
            sleep(5);
          kill(pid1,16);
            kill(pid2,17);
            wait(0);
            wait(0);
            printf("kill the parent\n");
            exit(0);
        }else{
            wait_flag =1;
            signal(17,stop);
            printf("Child process 2 is killed by parent\n");
            exit(0);
        }
    }
    else{
        wait_flag = 1;
        signal(16,stop);
        printf("Child process 1 is killed by parent\n");
        exit(0);
    }

}

