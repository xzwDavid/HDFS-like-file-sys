#include<stdio.h>
#include<sys/types.h>
#include<unistd.h>
#include<sys/wait.h>
#include<pthread.h>
#include<stdlib.h>
#include<semaphore.h>

sem_t single1,single2;



int n=1;
void *count1(){
    int i=0;
    for(i=0;i<5000;i++)
    {
        sem_wait(&single1);
        printf("%d\n",n++);
        sem_post(&single2);
        
    }
    return (void*)0;
}
void *count2(){
    int i=0;
    for(i=0;i<5000;i++)
    {
        sem_wait(&single2);
        printf("%d\n",n--);
        sem_post(&single1);
    }
    return (void*)0;
}
int main()
{

    sem_init(&single1,0,1);
    sem_init(&single2,0,0);

    pthread_t tid1,tid2;
    void *ptr = NULL;
    int ret1 = pthread_create(&tid1,NULL,count1,NULL);
    int ret2 = pthread_create(&tid2,NULL,count2,NULL);
    pthread_join(tid1,ptr);
    pthread_join(tid2,ptr);
    if(ret1!=0||ret2!=0){
        printf("error\n");
        exit(1);
    }
    
    return 0;
}