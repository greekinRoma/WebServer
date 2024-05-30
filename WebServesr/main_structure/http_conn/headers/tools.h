#ifndef HTTP_TOOLS_H
#define HTTP_TOOLS_H
#include <signal.h>
#include<sys/epoll.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <assert.h>
// 修改文件描述符，重置socket上的EPOLLONESHOT事件，以确保下一次可读时，EPOLLIN事件能被触发
void modfd(int epollfd, int fd, int ev);
int setnonblocking(int fd);
void addsig(int sig,void(handler)(int));
void addfd(int epollfd,int fd,bool one_shot);
void removefd(int epollfd,int fd);
#endif