#ifndef HTTP_CONN
#define HTTP_CONN
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <sys/stat.h>
#include <string.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <stdarg.h>
#include <errno.h>
#include <sys/uio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include"./http_type.h"
#include <iostream>
#include <stdarg.h>
#include "./tools.h"
using namespace std;
class http_conn
{
    const char* doc_root = "/root/WebServesr/resources";
    const char* ok_200_title = "OK";
    const char* error_400_title = "Bad Request";
    const char* error_400_form = "Your request has bad syntax or is inherently impossible to satisfy.\n";
    const char* error_403_title = "Forbidden";
    const char* error_403_form = "You do not have permission to get file from this server.\n";
    const char* error_404_title = "Not Found";
    const char* error_404_form = "The requested file was not found on this server.\n";
    const char* error_500_title = "Internal Error";
    const char* error_500_form = "There was an unusual problem serving the requested file.\n";
public:
    static const int FILENAME_LEN=200;
    static const int READ_BUFFER_SIZE=2048;
    static const int WRITE_BUFFER_SIZE=1024;
    static const int MAX_FRAMES=100;
public:
    static int m_epollfd; //所有socket事件被注册到同一个epoll内核事件上，所以设置称静态
    static int m_user_count;//统计用户的数量
public:
    http_conn();
    ~http_conn();
public:
    void init(int sockfd,const sockaddr_in& addr);
    void process();
    void close_conn();
    bool read();
    bool write();
    void unmap();
private:
    void init();
    HTTP_CODE process_read();
    bool process_write(HTTP_CODE ret);
    //解析HTTP请求
    LINE_STATUS parse_line();
    HTTP_CODE parse_request_line(char *text);
    HTTP_CODE parse_headers(char* text);
    HTTP_CODE parse_content(char* text);
    HTTP_CODE do_request();
    char* get_line(){return m_read_buf+m_start_line;}
private:
    int m_sockfd;
    sockaddr_in m_address;
private:
    CHECK_STATE m_check_state;//主机所处的状态
    int m_read_index;
    char m_read_buf[READ_BUFFER_SIZE];
    int m_checked_idx;//当前正在分析的字符在读缓冲区中的位置
    int m_start_line;//当前正在解析的行的起始位置
    int m_content_length;  //HTTP请求消息的总长度

    char m_real_file[FILENAME_LEN];//客户请求的目标文件，其内容等于doc_root + m_url, doc_root是网站根目录
    char* m_url;//客户请求的目标文件的文件名
    struct stat m_file_stat;//目标文件的状态
    char* m_file_address;//映射的起始位置
    bool m_linger; //判断是否要求保持连接
    char* m_host;   // 主机名
    METHOD m_method;// 请求方法
    char* m_version;// HTTP协议版本号，我们仅支持HTTP1.1

    char m_write_buf[WRITE_BUFFER_SIZE];  //写入缓存区的发送的字节数
    int m_write_idx;//写缓冲区中待发送的字节数
    int m_iv_count;                 //m_iv_count表示被写内存块的数量
    int bytes_to_send;              // 将要发送的数据的字节数
    int bytes_have_send;
    struct iovec m_iv[2];   // 我们将采用writev来执行写操作，所以定义下面两个成员，其中m_iv_count表示被写内存块的数量。
    /*
    struct iovec 定义了一个向量元素。
    iov_base 指向一个缓冲区
    iov_len 写入的长度
    */
    //这一组函数被process_write用来回答HTTP应答
    bool add_response(const char* format,...);
    bool add_content(const char* content );
    bool add_content_type();
    bool add_blank_line();
    bool add_linger();
    bool add_content_length(int content_len);
    bool add_status_line( int status, const char* title );
    bool add_headers(int content_len);
private:
    char default_str[200]="index.html";
};
#endif