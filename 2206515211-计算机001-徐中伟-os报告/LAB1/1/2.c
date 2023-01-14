#include<sys/types.h>
#include<stdio.h>
#include<unistd.h>
#include<sys/wait.h>

int m=0;
int main()
{
    int n=0;
    printf("before:n  %d, m   %d\n",&m,&n);
    pid_t pid, pid1;
    pid = fork();
    if(pid<0)
    {
        fprintf(stderr,"fork failure");
    }else if(pid == 0){
        n++;
        m++;
        printf("child of n:%d, %d\n",n, &n);
        printf("child of m:%d, %d\n",m, &m);
    }else{
        n--;
        m++;
        printf("parent:%d, %d\n",n, &n);
        printf("child of m:%d, %d\n",m, &m);
        wait(NULL);
    }
    printf("after:m   %d,n   %d\n",&m,&n);
    return 0;
}