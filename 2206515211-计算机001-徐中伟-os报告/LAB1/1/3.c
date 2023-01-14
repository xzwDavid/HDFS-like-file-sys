#include<sys/types.h>
#include<stdio.h>
#include<unistd.h>
#include<sys/wait.h>


int main()
{
    pid_t pid;
    pid = fork();
    if(pid<0)
    {
        fprintf(stderr,"fork failure");
    }else if(pid == 0){
        char *arg[] = {NULL};
        execvp("./4",arg);
        printf("error exec\n");
    }else{
        printf("this is father process\n");
        printf("the pid of the present is:%d\n",getpid());
    }
    
    return 0;
}