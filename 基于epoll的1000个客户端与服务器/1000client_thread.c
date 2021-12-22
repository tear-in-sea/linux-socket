#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h> 
#include <time.h>

#define BUFFER_SIZE 1024
#define CLIENT_NUM 999

int socket_connect()
{
	int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd == -1) {
        printf("socket create %s\n", strerror(errno));
    }
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
	
    addr.sin_family = AF_INET;
    addr.sin_port = htons(9099);
    addr.sin_addr.s_addr = inet_addr("192.168.5.101");
    
	if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
        printf("socket connect %s\n", strerror(errno));
    }
	
	return fd;
}

//线程函数
void *socket_send(void *args)
{
	char buf[64];
	int fd = (int)(*((int*)args));
	time_t rawtime;
    struct tm * timeinfo; 
	
    while(1)
	{
		memset(buf, 0, sizeof(buf));
		memcpy(buf,"1234",4);
		send(fd, buf, strlen(buf), 0);
		sleep(10);
        time ( &rawtime ); 
        timeinfo = localtime ( &rawtime ); 
        printf("send again new start ,time is %s\n",asctime(timeinfo));
	}
}

int main()
{
	int client_fd[CLIENT_NUM];
	int i = 0;

	pthread_t *threads = (pthread_t *)malloc(sizeof(pthread_t)*CLIENT_NUM);
	
	for(i = 0; i < CLIENT_NUM; i++)
	{
		client_fd[i] = socket_connect();
		pthread_create(&threads[i], NULL, socket_send, (void *)&client_fd[i]);  
	}

	//join
	for(i=0;i<CLIENT_NUM;i++)
	{
		pthread_join( threads[i], NULL);
	}
	return 0;
}
