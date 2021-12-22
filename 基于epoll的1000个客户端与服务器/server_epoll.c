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
#include <sys/epoll.h>
 
#define MAX_FD_NUM 1000

//设置非阻塞：在读操作时，如果读不到数据，O_NONBLOCK会回传-1，并且设置errno为EAGAIN。
void setnonblock(int fd) {
    int flag = fcntl(fd, F_GETFL, 0);
    if(flag == -1) {
        printf("get fcntl flag %s\n", strerror(errno));
        return;
    }
	
    int ret = fcntl(fd, F_SETFL, flag | O_NONBLOCK);
    if(ret == -1) {
        printf("set fcntl non-blocking %s\n", strerror(errno));
        return;
    }
}

//创建socket并监听
int socket_create(int port) {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if(fd == -1) {
        printf("socket create %s\n", strerror(errno));
        return -1;
    }
    setnonblock(fd);
	
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
	
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
	
    if(bind(fd, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
		printf("socket bind %s\n", strerror(errno));
        return -1;
    }
	
    if (listen(fd, 20) == -1) {
        printf("socket listen %s\n", strerror(errno));
        return -1;
    }
	
    return fd;
}
 
void socket_accept(int fd) {
    int epfd = epoll_create(MAX_FD_NUM);
    int ret = 0;
    if (epfd == -1) {
        printf("epoll create %s\n", strerror(errno));
        return;
    }
	
    struct epoll_event event, events[MAX_FD_NUM];
    memset(&event, 0, sizeof(event));
    
	event.data.fd = fd;
    event.events = EPOLLIN | EPOLLERR | EPOLLHUP;
	
    if (epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &event) == -1) {
        printf("epoll ctl %s\n", strerror(errno));
        return;
    }
	
    int client_fd;
	
    while (1) {
        int num = epoll_wait(epfd, events, MAX_FD_NUM, -1);
        if (num == -1) {
            printf("epoll wait %s\n", strerror(errno));
            break;
        } else {
            for (int i = 0; i<num; i++) {
                if (events[i].data.fd == fd) {
                    struct sockaddr_in client_addr;
                    memset(&client_addr, 0, sizeof(client_addr));
                    int len = sizeof(client_addr);
					
					//accept新连接
                    client_fd = accept(fd, (struct sockaddr *)&client_addr, &len);
                    if (client_fd == -1) {
                        printf("socket accept %s\n", strerror(errno));
                        return;
                    }
					
                    setnonblock(client_fd);
                    event.data.fd = client_fd;
                    event.events = EPOLLIN | EPOLLERR | EPOLLHUP;
                    if (epoll_ctl(epfd, EPOLL_CTL_ADD, client_fd, &event) == -1) {
                        printf("epoll ctl %s\n", strerror(errno));
                        return;
                    }
                    continue;
                } else if (events[i].events & EPOLLERR || events[i].events & EPOLLHUP) {
                    printf("epoll err\n");
                    close(events[i].data.fd);
                    continue;
                } else { //EPOLLIN
                    char buf[64];
                    memset(buf, 0, sizeof(buf));
                    ret = recv(events[i].data.fd, buf, sizeof(buf), 0);
                    if(ret == 0)
                    {
                        printf("###############recv 0#################\n");
                        continue;
                    }
                    printf("recv from is %s, events[i].events & EPOLLIN = %d\n", buf, events[i].events & EPOLLIN);
                    //close(events[i].data.fd); 保持长连接不用关闭socket
                    continue;
                }
            }
        }
    }
}
 
void server(int port) {
    int fd = socket_create(port);
    if (fd == -1) {
        printf("socket create fd failed\n");
        return;
    }
    socket_accept(fd);
}
 
int main(int argc, char *argv[]) {
    int port = atoi(argv[1]);
    server(port);
    return 0;
}
