#include<stdio.h>
#include<sys/types.h>
#include<unistd.h>
int main()
{
    pid_t pid;
    pid = getpid();
    printf("this is a test for exec\n");
    printf("this is a child pid:%d\n",getpid());
}