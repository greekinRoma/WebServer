#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <signal.h>
#include <assert.h>

#include "./http_conn/headers/http_conn.h"
#include "./thread_pools/headers/pthreadpool.h"
#define MAX_FD 65536 //文件描述符个数
#define MAX_EVENT_NUMBER 10000 //最大监听数量
#define Port_ID 88
extern void addfd(int epollfd,int fd,bool one_shot);
extern void addsig(int sig,void (handler)(int));
extern void removefd(int epollfd,int fd);
int main()
{

    int port =Port_ID;
    addsig(SIGPIPE,SIG_IGN);
    threadpool<http_conn>* pool=NULL;
    try {
        pool = new threadpool<http_conn>();
    } catch( ... ) {
        return 1;
    }
    http_conn* users=new http_conn[MAX_FD];
    int listenfd=socket(PF_INET,SOCK_STREAM,0);

    int ret=0;
    struct sockaddr_in address;
    address.sin_addr.s_addr=INADDR_ANY;
    address.sin_family=AF_INET;
    address.sin_port=htons(port);
    //socket
    int reuse=1;
    setsockopt(listenfd,SOL_SOCKET,SO_REUSEADDR,&reuse,sizeof(reuse));//设置端口复用
    ret=bind(listenfd,(struct sockaddr*) &address,sizeof(address));
    ret=listen(listenfd,5);
    //创建epoll对
    epoll_event events[MAX_EVENT_NUMBER];
    int epollfd=epoll_create(5);
    //添加到epoll对象中
    addfd(epollfd,listenfd,false);
    http_conn::m_epollfd=epollfd;
    while(true)
    {
        int number=epoll_wait(epollfd,events,MAX_EVENT_NUMBER,-1);
        if ((number<0)&&(errno!=EINVAL))//判断是否有事件发生
        {
            printf("epoll failure\n");
            break;
        }
        for (int i=0;i<number;i++)
        {
            int sockfd=events[i].data.fd;
            if (sockfd==listenfd)
            {
                struct sockaddr_in client_addrress;
                socklen_t client_addrlength=sizeof(client_addrress);
                int connfd=accept(listenfd,(struct sockaddr *)&client_addrress,&client_addrlength);
                cout<<"Client: "<<connfd<<" try to log in."<<endl;
                if (connfd<0)
                {
                    printf("errno is %d\n",errno);
                    continue;
                }
                if (http_conn::m_user_count>=MAX_FD)
                {
                    close(connfd);
                    continue;
                }
                users[connfd].init(connfd,client_addrress);
                cout<<"Client: "<<connfd<<" has logged in."<<endl;
            }
            else if (events[i].events&(EPOLLRDHUP|EPOLLHUP|EPOLLERR))
            {
                users[sockfd].close_conn();
            }
            else if (events[i].events&EPOLLIN)
            {
                if (users[sockfd].read())
                {
                    pool->append(users+sockfd);//将任务放到线程池中进行处理
                }
                else
                {
                    users[sockfd].close_conn();
                }
            }
            else if (events[i].events&EPOLLOUT)
            {
                if (!users[sockfd].write())
                {
                    users[sockfd].close_conn();
                }
            }
        }
    }
    close(epollfd);
    close(listenfd);
    delete []users;
    delete pool;
    return 0;
}