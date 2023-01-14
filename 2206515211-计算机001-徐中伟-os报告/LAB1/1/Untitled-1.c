#include<sys/types.h>
#include<stdio.h>
#include<unistd.h>
#include<sys/wait.h>
#include<sys/syscall.h>

int main()
{
    pid_t pid;
    pid = fork();
    if(pid<0)
    {
        fprintf(stderr,"fork failure");
    }else if(pid == 0){
        pid_t a =syscall(SYS_gettid);
        printf("a = %d\n",a);
    }else{
        printf("this is father process\n");
        printf("the pid of the present is:%d\n",getpid());
    }
    
    return 0;
}