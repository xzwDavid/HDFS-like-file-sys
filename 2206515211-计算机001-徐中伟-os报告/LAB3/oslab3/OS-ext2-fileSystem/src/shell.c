#pragma warning(disable : 4995)
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include<errno.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<pthread.h>
#include<unistd.h>
#include <ctype.h>
#include <unistd.h>
#include <memory.h>
#include "shell.h"
#include "disksim.h"


#define DEST_PORT 8099
#define DEST_IP "127.0.0.1"
#define MAX_DATA 1024






#define		SECTOR					DWORD
#define		BLOCK_SIZE				1024
#define		SECTOR_SIZE				512
#define		NUMBER_OF_SECTORS		((268435456+1024) / SECTOR_SIZE)

#define COND_MOUNT				0x01
#define COND_UMOUNT				0x02



char* cmdbuf;
char** temp;
int ko=0;

int num=0;
void* cmdfunc(void *args)
{
    //printf("hhhhhh\n");
    FILE *fp;
    int c;

    char **cmd;
    cmd = (char**)malloc(sizeof(char*)*1000);

    fp = fopen((char*)args,"r");
    char res[20];
    int cnt=0;
    int cnt2=0;
    while(1)
    {

        c = fgetc(fp);
        res[cnt++] = c;
        if( feof(fp) )
        {
            break ;
        }
        if(c=='\n')
        {
            cmd[cnt2++]= (char*)malloc(sizeof(cnt+1));
            int hhh=0;
            for(int tt=0;tt<cnt;tt++)  {
                int t = res[tt];
                if((97<=t&&t<=122)||(48<=t&&t<=57)||(65<=t&&t<=90)||res[tt]==' ')
                    cmd[cnt2-1][hhh++] = res[tt];
                //memcpy(cmd,res,cnt+1);
                //printf("%s",cmd[cnt2-1]);
            }
            //printf("%s",cmd[cnt2-1]);
            memset(res,0,20);
            cnt=0;
        }
        //printf("%c", c);
    }

    cmd[cnt2++]= (char*)malloc(sizeof(cnt+1));
    int hhh=0;
    for(int tt=0;tt<cnt;tt++)  {
        int t = res[tt];
        if((97<=t&&t<=122)||(48<=t&&t<=57)||(65<=t&&t<=90)||res[tt]==' ')
            cmd[cnt2-1][hhh++] = res[tt];
    }

    fclose(fp);

    int sockfd;
    struct sockaddr_in dest_addr;

    sockfd=socket(AF_INET,SOCK_STREAM,0);
    if(sockfd==-1){
        printf("socket failed:%d",errno);
    }

    dest_addr.sin_family=AF_INET;
    dest_addr.sin_port=htons(DEST_PORT);
    dest_addr.sin_addr.s_addr=inet_addr(DEST_IP);
    bzero(&(dest_addr.sin_zero),8);



    if(connect(sockfd,(struct sockaddr*)&dest_addr,sizeof(struct sockaddr))==-1){
        printf("connect failed:%d",errno);
    } else{
        for(int j=0;j<cnt2;j++){
            char *b = (char*)malloc(sizeof(char)*68);
            char fuck[] = "{\"id\":1000,\"method\":\"Cmd.Strin\",\"params\":[{\"CmdName\":\"";
            memset(b,0,68);
            memcpy(b,fuck,68);
            // //=cmd[j]+
            char*c = "\"}]}";
            const char *f = cmd[j];
            //printf("%s\n",cmd[j]);
            strcat(b,f);
            strcat(b,c);
            char *data =b;//[]= "{\"id\":1000,\"method\":\"Cmd.Strin\",\"params\":[{\"CmdName\":\"cd\"}]}";
            //printf("%s\n",b);
            send(sockfd,data,strlen(data),0);
            printf("send success\n");
            char buf[MAX_DATA] = {0};
            while(1)
            {
                recv(sockfd,buf,MAX_DATA,0);
                if(strlen(buf) > 0){
                    printf("%s\n",buf);
                    strcat(cmdbuf,buf);
                    strcat(cmdbuf," ");
                    break;
                }
            }
        }
    }

    for(int l=0;l<cnt2;l++)
    {
        temp[ko] =(char*)malloc(sizeof(char)*200);
        temp[ko++] = cmd[l];
    }
    close(sockfd);
    num+=cnt2;
    return (void*)0;
}



typedef struct
{
	char*	name; // ????????????
	int(*handler)(int, char**); // shell_command ????????????
	char	conditions; // ?????????????????????
} COMMAND;
// ???????????????
// extern?????? ???ext2_shell.c?????????
extern void shell_register_filesystem(SHELL_FILESYSTEM*);
extern void printf_by_sel(DISK_OPERATIONS* disk, SHELL_FS_OPERATIONS* fsOprs, const SHELL_ENTRY* parent, SHELL_ENTRY* entry, const char* name, int sel, int num);
void do_shell(void);
void unknown_command(void);
int seperate_string(char* buf, char* ptrs[]);
double get_percentage(unsigned int number, unsigned int total);

int shell_cmd_format(int argc, char* argv[]);
int shell_cmd_exit(int argc, char* argv[]);
int shell_cmd_mount(int argc, char* argv[]);
int shell_cmd_touch(int argc, char* argv[]);
int shell_cmd_cd(int argc, char* argv[]);
int shell_cmd_ls(int argc, char* argv[]);
int shell_cmd_mkdir(int argc, char* argv[]);
int shell_cmd_fill(int argc, char* argv[]);
int shell_cmd_rm(int argc, char* argv[]);
int shell_cmd_rmdir(int argc, char* argv[]);
int shell_cmd_cat(int argc, char* argv[]);


static COMMAND g_commands[] =
{
	{ "cd",		shell_cmd_cd,		COND_MOUNT	},
	{ "mount",	shell_cmd_mount,	COND_UMOUNT	},
	{ "touch",	shell_cmd_touch,	COND_MOUNT	},
	{ "write",	shell_cmd_fill,		COND_MOUNT	},
	{ "ls",		shell_cmd_ls,		COND_MOUNT	},
	{ "format",	shell_cmd_format,	COND_UMOUNT	},
	{ "mkdir",	shell_cmd_mkdir,	COND_MOUNT	},
	{ "rm",		shell_cmd_rm,		COND_MOUNT	},
	{ "rmdir",	shell_cmd_rmdir,	COND_MOUNT	},
	{ "exit",	shell_cmd_exit,		0	},
	{ "cat",	shell_cmd_cat,		COND_MOUNT	},
};

static SHELL_FILESYSTEM		g_fs;
static SHELL_FS_OPERATIONS	g_fsOprs;
static SHELL_ENTRY			g_rootDir;
static SHELL_ENTRY			g_currentDir;
static DISK_OPERATIONS		g_disk;

static SHELL_ENTRY	path[256];
static int pathTop = 0;

int g_commandsCount = sizeof(g_commands) / sizeof(COMMAND);
int g_isMounted;

int main(int argc, char* argv[])
{
	if (disksim_init(NUMBER_OF_SECTORS, SECTOR_SIZE, &g_disk) < 0)
	{
		printf("Disk simulator initialization has been failed\n");
		return -1;
	}else{
		printf("Disk simulator initialization has succeeded !!!\n");
	}
	// ????????????????????????
	shell_register_filesystem(&g_fs);

	do_shell();

	return 0;
}

void do_shell(void)
{
	char buf[1000];
	char command[100];
	char* argv[100];
	int argc;
	int i, j = 0;

	printf("%s File system shell (designed by zhongwei)\n", g_fs.name);


    char ** a;
    a = (char**)malloc(sizeof (char*));
    a[0] = "format";
    g_commands[5].handler(1,a);
    a[0] = "mount";
    g_commands[1].handler(1,a);

    a = (char**)malloc(sizeof(char*)*2);
    a[0] = "xzw";
    a[1] = "xzw";
    g_commands[6].handler(2,a);
    g_commands[6].handler(2,a);
//**********************************rpc part********************************************
printf("hhhhh\n");


    int rpc=0;
    printf("please input whether to test the rpc:");
    scanf("%d",&rpc);
    if(rpc)
    {
        temp = (char**)malloc(sizeof(char*)*100);
        memset(temp,0,2000);
        cmdbuf= (char*)malloc(sizeof(char)*5000);
        memset(cmdbuf,0,5000);
        pthread_t tid;
        pthread_t tid1;
        pthread_t tid2;
        printf("main--pid=%d,tid=%lu\n",getpid(),pthread_self());
        char p[]= "cmd.txt";
        int ret=pthread_create(&tid,NULL,cmdfunc,(void*)p);

        char p1[]= "cmd1.txt";
        pthread_create(&tid1,NULL,cmdfunc,(void*)p1);

        char p2[]= "cmd2.txt";
        pthread_create(&tid2,NULL,cmdfunc,(void*)p2);

        //sleep();
        pthread_join(tid,NULL);
        pthread_join(tid1,NULL);
        pthread_join(tid2,NULL);
        printf("cmdbuf:%s",temp);




    }


    printf("\n%d\n",num);
    for(int nn=0;nn<num;nn++){
        printf("\n%s\n",temp[nn]);

        argc = seperate_string(temp[nn], argv); // ????????????
        if (argc == 0)
            continue;
        for (i = 0; i < g_commandsCount; i++)
        {
            if (strcmp(g_commands[i].name, argv[0]) == 0)
            {
                if (check_conditions(g_commands[i].conditions) == 0)
                    g_commands[i].handler(argc, argv);
                break;
            }
        }

    }

//**********************************rpc part********************************************
	while (-1)
	{
		printf("OS-zhongwei: [/%s]$ ", g_currentDir.name);
		fgets(buf, 1000, stdin); // ??????????????????command????????????
	
		argc = seperate_string(buf, argv); // ????????????
		if (argc == 0)
			continue;
		// ????????????
		for (i = 0; i < g_commandsCount; i++)
		{
			if (strcmp(g_commands[i].name, argv[0]) == 0)
			{
				if (check_conditions(g_commands[i].conditions) == 0)
					g_commands[i].handler(argc, argv);
				break;
			}
		}
		
		if (i == g_commandsCount)
			unknown_command();
	}
}
/******************************************************************************/
/*																			  */
/*							SHELL_COMMAND_LIST 						    	  */
/*																			  */
/******************************************************************************/
int shell_cmd_cd(int argc, char* argv[])
{
	SHELL_ENTRY	newEntry;
	int			result, i;

	path[0] = g_rootDir;

	if (argc > 2)
	{
		printf("usage : %s [directory]\n", argv[0]);
		return 0;
	}

	if (argc == 1)
		pathTop = 0;
	else
	{
		if (strcmp(argv[1], ".") == 0)
			return 0;
		else if (strcmp(argv[1], "..") == 0 && pathTop > 0)
			pathTop--;
		else
		{
			result = g_fsOprs.lookup(&g_disk, &g_fsOprs, &g_currentDir, &newEntry, argv[1]);

			if (result)
			{
				printf("directory not found\n");
				return -1;
			}
			else if (!newEntry.isDirectory)
			{
				printf("%s is not a directory\n", argv[1]);
				return -1;
			}
			path[++pathTop] = newEntry;
		}
	}

	g_currentDir = path[pathTop];

	return 0;
}

int shell_cmd_exit(int argc, char* argv[])
{
	disksim_uninit(&g_disk);
	_exit(0);

	return 0;
}

int shell_cmd_mount(int argc, char* argv[])
{
	int result;
	
	if (g_fs.mount == NULL)
	{
		printf("The mount functions is NULL\n");
		return 0;
	}
	
	result = g_fs.mount(&g_disk, &g_fsOprs, &g_rootDir);
	g_currentDir = g_rootDir;
	
	if (result < 0)
	{
		printf("%s file system mounting has been failed\n", g_fs.name);
		return -1;
	}
	else
	{
		printf("%s file system has been mounted successfully\n", g_fs.name);
		g_isMounted = 1;
	}

	return 0;
}

int shell_cmd_umount(int argc, char* argv[])
{
	g_isMounted = 0;

	if (g_fs.umount == NULL)
		return 0;

	g_fs.umount(&g_disk, &g_fsOprs);
	return 0;
}

int shell_cmd_touch(int argc, char* argv[])
{
	SHELL_ENTRY	entry;
	int			result;
	if (argc < 2)
	{
		printf("usage : touch [files...]\n");
		return -2;
	}
	result = g_fsOprs.fileOprs->create(&g_disk, &g_fsOprs, &g_currentDir, argv[1], &entry);
	
	if (result)
	{
		printf("create failed\n");
		return -1;
	}

	return 0;
}

int shell_cmd_fill(int argc, char* argv[])
{
	SHELL_ENTRY	entry;
	char*		buffer;
	char*		tmp;
	int			size;
	int			result;

	if( argc != 3 ) 
	{
		printf( "usage : fill [file] [size]\n" );
		return 0;
	}
    char tmpbuf[100];
    memset(tmpbuf,0,100);
    int eh=0;
    if(*argv[2] == '0')
    {
        char ee;

        printf("please input the content\n");
        while(ee !='\n'){ee=getchar();tmpbuf[eh++]=ee;}
    }
	sscanf( argv[2], "%d", &size ); 
	

	result = g_fsOprs.fileOprs->create( &g_disk, &g_fsOprs, &g_currentDir, argv[1], &entry ); 
	
	if( result && result !=-3)
	{
		printf( "create failed\n" );
		return -1;
	}
	// ?????????????????????
	buffer = ( char* )malloc( size + 13 ); 
	memset(buffer,0,sizeof(buffer));
	tmp = buffer;
	if(*argv[2]=='0'){
        printf("%d\n%s\n",eh,tmpbuf);
        memcpy(tmp,tmpbuf,eh+1);
        tmp+=eh+1;
        g_fsOprs.fileOprs->write( &g_disk, &g_fsOprs, &g_currentDir, &entry, 0, eh+1, tmpbuf );

        //free( tmpbuf );
    }else{
        while( tmp < buffer + size )
        {
            memcpy( tmp, "hello,world!\n", 13 );
            tmp += 13;
        }
        g_fsOprs.fileOprs->write( &g_disk, &g_fsOprs, &g_currentDir, &entry, 0, size, buffer );

        free( buffer );
    }
	/*
	for(int j=0;j<size+13;j++){
		printf("%c",buffer[j]);
	}
	*/


	return 0;
}

int shell_cmd_rm(int argc, char* argv[])
{
	int i;

	if (argc < 2)
	{
		printf("usage : rm [files...]\n");
		return 0;
	}
	// ????????????????????????files
	for (i = 1; i < argc; i++)
	{
		if (g_fsOprs.fileOprs->remove(&g_disk, &g_fsOprs, &g_currentDir, argv[i]))
			printf("cannot remove file\n");
	}

	return 0;
}

int shell_cmd_format(int argc, char* argv[])
{
	int		result;
	unsigned int k = 0;
	char*	param = NULL;

	if (argc >= 2)
		param = argv[1];

	result = g_fs.format(&g_disk);

	if (result < 0)
	{
		printf("%s formatting is failed\n", g_fs.name);
		return -1;
	}

	printf("disk has been formatted successfully\n");
	return 0;
}




int shell_cmd_mkdir(int argc, char* argv[])
{
	SHELL_ENTRY	entry;
	int result;

	if (argc != 2)
	{
		printf("usage : %s [name]\n", argv[0]);
		return 0;
	}

	result = g_fsOprs.mkdir(&g_disk, &g_fsOprs, &g_currentDir, argv[1], &entry);

	if (result)
	{
		printf("cannot create directory\n");
		return -1;
	}

	return 0;
}

int shell_cmd_rmdir(int argc, char* argv[])
{
	int result;

	if (argc != 2)
	{
		printf("usage : %s [name]\n", argv[0]);
		return 0;
	}

	result = g_fsOprs.rmdir(&g_disk, &g_fsOprs, &g_currentDir, argv[1]);

	if (result)
	{
		printf("cannot remove directory\n");
		return -1;
	}

	return 0;
}

int shell_cmd_mkdirst(int argc, char* argv[])
{
	SHELL_ENTRY	entry;
	int		result, i, count;
	char	buf[10];

	if (argc != 2)
	{
		printf("usage : %s [count]\n", argv[0]);
		return 0;
	}

	sscanf(argv[1], "%d", &count);
	for (i = 0; i < count; i++)
	{
		printf(buf, "%d", i);
		result = g_fsOprs.mkdir(&g_disk, &g_fsOprs, &g_currentDir, buf, &entry);

		if (result)
		{
			printf("cannot create directory\n");
			return -1;
		}
	}

	return 0;
}

int shell_cmd_cat(int argc, char* argv[])
{
	SHELL_ENTRY	entry;
	char		buf[1024];
	int			result;
	unsigned long	offset = 0;

	if (argc != 2)
	{
		printf("usage : %s [file name]\n", argv[0]);
		return 0;
	}

	result = g_fsOprs.lookup(&g_disk, &g_fsOprs, &g_currentDir, &entry, argv[1]);
	if (result)
	{
		printf("%s lookup failed\n", argv[1]);
		return -1;
	}
	// ?????????????????????

	while ((result == (g_fsOprs.fileOprs->read(&g_disk, &g_fsOprs, &g_currentDir, &entry, offset, 1024, buf))) > 0)
	{
		printf("%s", buf);
		offset += 1024;
		memset(buf, 0, sizeof(buf));
		
	}
	printf("\n");
    return 1;
}

int shell_cmd_ls(int argc, char* argv[])
{
	SHELL_ENTRY_LIST		list;
	SHELL_ENTRY_LIST_ITEM*	current;

	if (argc > 2)
	{
		printf("usage : %s [path]\n", argv[0]);
		return 0;
	}
	init_entry_list(&list);
   // printf("\nhhh\n");
	if (g_fsOprs.read_dir(&g_disk, &g_fsOprs, &g_currentDir, &list))
	{
     //   printf("\nhhh2\n");
		printf("Failed to read_dir\n");
		return -1;
	}
    //printf("\nhhh\n");
	current = list.first;

	printf("[File names] [D] [File sizes]\n");
	while (current)
	{
		printf("%-12s  %1d  %12d\n",
			current->entry.name, current->entry.isDirectory, current->entry.size);
		current = current->next;
	}
	printf("\n");

	release_entry_list(&list);
	return 0;
}

/******************************************************************************/
/*																			  */
/*							UTILITY FUNCTIONS 								  */
/*																			  */
/******************************************************************************/

int check_conditions(int conditions)
{
	if (conditions & COND_MOUNT && !g_isMounted)
	{
		printf("file system is not mounted\n");
		return -1;
	}

	if (conditions & COND_UMOUNT && g_isMounted)
	{
		printf("file system is already mounted\n");
		return -1;
	}

	return 0;
}

void unknown_command(void)
{
	int i;

	printf(" * ");
	for (i = 0; i < g_commandsCount; i++)
	{
		if (i < g_commandsCount - 1)
			printf("%s, ", g_commands[i].name);
		else
			printf("%s", g_commands[i].name);
	}
	printf("\n");
}

int seperate_string(char* buf, char* ptrs[])
{
	char prev = 0;
	int count = 0;

	while (*buf)
	{
		if (isspace(*buf))
			*buf = 0;
		else if (prev == 0)
			ptrs[count++] = buf;

		prev = *buf++;
	}

	return count;
}

double get_percentage(unsigned int number, unsigned int total)
{
	return ((double)number) / total * 100.;
}