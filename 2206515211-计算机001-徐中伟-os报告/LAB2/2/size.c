#include  <stdio.h>
#include  <stdlib.h>
#include  <unistd.h>
#include  <signal.h>
#include  <sys/wait.h>
#include  <sys/types.h>
#include<semaphore.h>
int size=1000;

sem_t sgl;


int main(void) {
    
    
    sem_init(&sgl,1,1);


    int pipefd[2];
    int pid1, pid2;
    char OutPipe[1024], InPipe[1024];  	     // 定义两个字符数组

    // create pipe
    pipe(pipefd);

    // create child 1
    while((pid1 = fork( )) == -1);
    // child 1
    if(pid1 == 0) {
        for(int i=0;i<2000;i++){
            // load data
            sem_wait(sgl);
            if(size==0)
            {
                sem_post(sgl);
                continue;
            }
            
            size--;
            sprintf(OutPipe,"1");
            // lock out pipe
            lockf(pipefd[1], 1, 0);
            // write data
            //printf("Child 1 wiriting !\n");
            write(pipefd[1], OutPipe, 50);
            // wait reading
            sleep(1);
            // free out pipe
            lockf(pipefd[1] ,0 ,0);
            sem_post(sgl);
        }

        exit(0);
    }
    else {
        // create child 2
        while((pid2 = fork()) == -1);          		// 若进程2创建不成功,则空循环
    // child 2
        if(pid2 == 0) {

            for(int i=0;i<2000;i++){
                sem_wait(sgl);    
                if(size==0)
                {
                    sem_post(sgl);
                    continue;
                }
                lockf(pipefd[1], 1, 0);
                
                size--;
                sprintf(OutPipe,"2");
                write(pipefd[1], OutPipe, 50);
                // wait reading
                sleep(1);
                // free out pipe
                lockf(pipefd[1], 0, 0);
                sem_post(sgl);
            }
            exit(0);
        }
    // parent
        else {
            int cnt = 4000;
            while(cnt>0){
            sem_wait(sgl);
            if(size==1000)
            {
                sem_post(sgl);
                continue;
            }
            
            cnt-=1000-size;
            // lock in pipe
            lockf(pipefd[0], 1, 0);
            // read data
            read(pipefd[0], InPipe, 1000-size);
            // free in pipe
            lockf(pipefd[0], 0, 0);
            // print data
            printf("%s\n",InPipe);
            sem_post(sgl);
            }
            sem_destroy(sgl);
            wait(0);
            exit(0);
            
        }
    }

    return 0;
}