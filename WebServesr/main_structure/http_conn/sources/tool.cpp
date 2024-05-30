#include"../headers/tools.h"
// 修改文件描述符，重置socket上的EPOLLONESHOT事件，以确保下一次可读时，EPOLLIN事件能被触发
void modfd(int epollfd, int fd, int ev) {
    epoll_event event;
    event.data.fd = fd;
    event.events = ev | EPOLLET | EPOLLONESHOT | EPOLLRDHUP;
    epoll_ctl( epollfd, EPOLL_CTL_MOD, fd, &event );
}
int setnonblocking(int fd)
{
    int old_option=fcntl(fd,F_GETFL);
    int new_option=old_option|O_NONBLOCK;
    fcntl(fd,F_SETFL,new_option);//设置为非阻塞
    return old_option;
}
void addsig(int sig,void(handler)(int))
{
    struct sigaction sa;
    memset(&sa,'\0',sizeof(sa));
    sa.sa_handler=handler;//忽略信号
    sigfillset(&sa.sa_mask);
    assert(sigaction(sig,&sa,NULL)!=-1);
}
void addfd(int epollfd,int fd,bool one_shot)
{
    /*
    epollfd epoll文件描述符
    fd 监听文件描述符
    one_shoot 表示是否使用边缘触发
    */
    epoll_event event;
    event.data.fd=fd;
    event.events=EPOLLIN|EPOLLRDHUP;//代表监控1.是否有可读数据；2. 是否有写入或者读取
    if(one_shot)
    {
        event.events|=EPOLLONESHOT;
    }
    epoll_ctl(epollfd,EPOLL_CTL_ADD,fd,&event);
    setnonblocking(fd);//设置监听文件为非阻塞
}
void removefd(int epollfd,int fd)
{
    epoll_ctl(epollfd,EPOLL_CTL_DEL,fd,0);
    close(fd);
}