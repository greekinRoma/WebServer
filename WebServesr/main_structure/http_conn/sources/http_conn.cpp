#include "../headers/http_conn.h"
int http_conn::m_epollfd=-1;
int http_conn::m_user_count=0;
http_conn::http_conn()
{

}
http_conn::~http_conn()
{

}
bool http_conn::read()
{
    // std::cout<<"hdsofhasoihd"<<endl;
    if (m_read_index>=READ_BUFFER_SIZE) return false;
    int byte_read=0;
    while(true)
    {
        byte_read=recv(m_sockfd,m_read_buf+m_read_index,READ_BUFFER_SIZE-m_read_index,0);
        // printf("%d",byte_read);
        // std::cout<<byte_read<<endl;
        if (byte_read==-1)
        {
            // cout<<(errno==EAGAIN)<<endl;
            if(errno==EAGAIN||errno==EWOULDBLOCK) break;
            return false;
        }
        else if (byte_read==0) return false;
        m_read_index+=byte_read;
    } 
    
    return true;
}
LINE_STATUS http_conn::parse_line() {
    char temp;
    // cout<<"read buf"<<m_read_buf[ m_checked_idx ]<<m_checked_idx<<endl;
    for ( ; m_checked_idx < m_read_index; ++m_checked_idx ) {
        temp = m_read_buf[ m_checked_idx ];
        if ( temp == '\r' ) {
            if ( ( m_checked_idx + 1 ) == m_read_index ) {
                return LINE_OPEN;
            } else if ( m_read_buf[ m_checked_idx + 1 ] == '\n' ) {
                m_read_buf[ m_checked_idx++ ] = '\0';
                m_read_buf[ m_checked_idx++ ] = '\0';
                return LINE_OK;
            }
            return LINE_BAD;
        } else if( temp == '\n' )  {
            if( ( m_checked_idx > 1) && ( m_read_buf[ m_checked_idx - 1 ] == '\r' ) ) {
                m_read_buf[ m_checked_idx-1 ] = '\0';
                m_read_buf[ m_checked_idx++ ] = '\0';
                return LINE_OK;
            }
            return LINE_BAD;
        }
    }
    return LINE_OPEN;
}
/*-----------------------------------------具体解析HTTP请求--------------------*/
HTTP_CODE http_conn::parse_content(char* text)//判断http请求报文是否符合要求
{
    if (m_read_index>=(m_content_length+m_checked_idx))
    {
        text[m_content_length]='\0';//标记是否读到末尾
        return GET_REQUEST;
    }
    return NO_REQUEST;
}
HTTP_CODE http_conn::do_request()//回复
{
    strcpy(m_real_file,doc_root);
    int len=strlen(doc_root);
    if (strlen(m_url)==1) strcpy(m_url,default_str);
    // cout<<strlen(m_url)<<*m_url<<"sdfuasgd"<<endl;
    printf("m_url:%s",m_url);
    strncpy(m_real_file+len,m_url,FILENAME_LEN-len-1);
    //查看m_real_file的状态
    if (stat(m_real_file,&m_file_stat)<0)
    {
        return NO_RESOURCE;
    }
    //判断访问权限
    if (!(m_file_stat.st_mode&S_IROTH)) return FORBIDDEN_REQUEST;//返回无法访问
    if (S_ISDIR(m_file_stat.st_mode)) return BAD_REQUEST;//判断是否为目录
    int fd=open(m_real_file,O_RDONLY);
    printf("real_file:%s",m_real_file);
    //创建内存映射
    m_file_address=(char*) mmap(0,m_file_stat.st_size,PROT_READ,MAP_PRIVATE,fd,0);
    close(fd);
    return FILE_REQUEST;//表示文件获取成功
}
HTTP_CODE http_conn::parse_headers(char *text)
{
    if (text[0]=='\0')
    {
        if (m_content_length!=0)
        {
            m_check_state=CHECK_STATE_CONTENT;
            return NO_REQUEST;
        }
        return GET_REQUEST;
    }else if ( strncasecmp( text, "Connection:", 11 ) == 0 ) {
        // 处理Connection 头部字段  Connection: keep-alive
        text += 11;
        text += strspn( text, " \t" );
        if ( strcasecmp( text, "keep-alive" ) == 0 ) {
            m_linger = true;
        }
    } else if ( strncasecmp( text, "Content-Length:", 15 ) == 0 ) {
        // 处理Content-Length头部字段
        text += 15;
        text += strspn( text, " \t" );
        m_content_length = atol(text);
    } else if ( strncasecmp( text, "Host:", 5 ) == 0 ) {
        // 处理Host头部字段
        text += 5;
        text += strspn( text, " \t" );
        m_host = text;
    } else {
        printf( "oop! unknow header %s\n", text );
    }
    return NO_REQUEST;
}
// 解析HTTP请求行，获得请求方法，目标URL,以及HTTP版本号
HTTP_CODE http_conn::parse_request_line(char* text) {
    // GET /index.html HTTP/1.1
    m_url = strpbrk(text, " \t"); // 判断第二个参数中的字符哪个在text中最先出现
    if (! m_url) { 
        return BAD_REQUEST;
    }
    // GET\0/index.html HTTP/1.1
    *m_url++ = '\0';    // 置位空字符，字符串结束符
    char* method = text;
    if ( strcasecmp(method, "GET") == 0 ) { // 忽略大小写比较
        m_method = GET;
    } else {
        return BAD_REQUEST;
    }
    // /index.html HTTP/1.1
    // 检索字符串 str1 中第一个不在字符串 str2 中出现的字符下标。
    m_version = strpbrk( m_url, " \t" );
    if (!m_version) {
        return BAD_REQUEST;
    }
    *m_version++ = '\0';
    if (strcasecmp( m_version, "HTTP/1.1") != 0 ) {
        return BAD_REQUEST;
    }
    /**
     * http://192.168.110.129:10000/index.html
    */
    if (strncasecmp(m_url, "http://", 7) == 0 ) {   
        m_url += 7;
        // 在参数 str 所指向的字符串中搜索第一次出现字符 c（一个无符号字符）的位置。
        m_url = strchr( m_url, '/' );
    }
    if ( !m_url || m_url[0] != '/' ) {
        return BAD_REQUEST;
    }
    m_check_state = CHECK_STATE_HEADER; // 检查状态变成检查头
    return NO_REQUEST;
}
/*-----------------------------------------解析主程序--------------------*/
HTTP_CODE http_conn::process_read()
{
    LINE_STATUS line_status =LINE_OK;
    HTTP_CODE ret=NO_REQUEST;
    char *text=0;
    // cout<<(m_check_state == CHECK_STATE_CONTENT)<<(line_status == LINE_OK)<<endl;
    // cout<<((line_status = parse_line()) == LINE_OK)<<endl;
    while (((m_check_state == CHECK_STATE_CONTENT) && (line_status == LINE_OK))
                || ((line_status = parse_line()) == LINE_OK)) 
    {
        text=get_line();
        m_start_line=m_checked_idx;
        // cout<<("got 1 http line:%s\n",text)<<endl;
        // printf("got 1 http line:%s\n",text);
        switch ( m_check_state ) {
            case CHECK_STATE_REQUESTLINE: {
                ret = parse_request_line( text );
                if ( ret == BAD_REQUEST ) {
                    return BAD_REQUEST;
                }
                break;
            }
            case CHECK_STATE_HEADER: {
                ret = parse_headers( text );
                if ( ret == BAD_REQUEST ) {
                    return BAD_REQUEST;
                } else if ( ret == GET_REQUEST ) {
                    return do_request();
                }
                break;
            }
            case CHECK_STATE_CONTENT: {
                ret = parse_content( text );
                if ( ret == GET_REQUEST ) {
                    return do_request();
                }
                line_status = LINE_OPEN;
                break;
            }
            default: {
                return INTERNAL_ERROR;
            }
        }
    }
    return NO_REQUEST;
}
/*---------------------------------------------------------------Write--------------------------------------------*/
// 根据服务器处理HTTP请求的结果，决定返回给客户端的内容
bool http_conn::process_write(HTTP_CODE ret) {
    switch (ret)
    {
        case INTERNAL_ERROR:
            add_status_line( 500, error_500_title );
            add_headers( strlen( error_500_form ) );
            if ( ! add_content( error_500_form ) ) {
                return false;
            }
            break;
        case BAD_REQUEST:
            add_status_line( 400, error_400_title );
            add_headers( strlen( error_400_form ) );
            if ( ! add_content( error_400_form ) ) {
                return false;
            }
            break;
        case NO_RESOURCE:
            add_status_line( 404, error_404_title );
            add_headers( strlen( error_404_form ) );
            if ( ! add_content( error_404_form ) ) {
                return false;
            }
            break;
        case FORBIDDEN_REQUEST:
            add_status_line( 403, error_403_title );
            add_headers(strlen( error_403_form));
            if ( ! add_content( error_403_form ) ) {
                return false;
            }
            break;
        case FILE_REQUEST:
            add_status_line(200, ok_200_title );
            add_headers(m_file_stat.st_size);
            m_iv[ 0 ].iov_base = m_write_buf;
            m_iv[ 0 ].iov_len = m_write_idx;
            m_iv[ 1 ].iov_base = m_file_address;
            m_iv[ 1 ].iov_len = m_file_stat.st_size;
            m_iv_count = 2;

            bytes_to_send = m_write_idx + m_file_stat.st_size;
            //要发送的大小
            return true;
        default:
            return false;
    }

    m_iv[ 0 ].iov_base = m_write_buf;
    m_iv[ 0 ].iov_len = m_write_idx;
    m_iv_count = 1;
    bytes_to_send = m_write_idx;
    return true;
}

bool http_conn::add_headers(int content_len) {
    add_content_length(content_len);
    add_content_type();
    add_linger();
    add_blank_line();
    return true;
}
bool http_conn::add_content_length(int content_len) {
    return add_response( "Content-Length: %d\r\n", content_len );
}

bool http_conn::add_linger()
{
    return add_response( "Connection: %s\r\n", ( m_linger == true ) ? "keep-alive" : "close" );
}

bool http_conn::add_status_line( int status, const char* title ) {
    return add_response( "%s %d %s\r\n", "HTTP/1.1", status, title );
}
bool http_conn::add_blank_line()
{
    return add_response( "%s", "\r\n" );
}

bool http_conn::add_content( const char* content )
{
    return add_response( "%s", content );
}

bool http_conn::add_content_type() {
    return add_response("Content-Type:%s\r\n", "text/html");
}
bool http_conn::add_response(const char* format,...)
{
    if(m_write_idx>=WRITE_BUFFER_SIZE) return false;
    va_list arg_list;
    va_start(arg_list,format);
    int len = vsnprintf( m_write_buf + m_write_idx, WRITE_BUFFER_SIZE - 1 - m_write_idx, format, arg_list );
    if( len >= ( WRITE_BUFFER_SIZE - 1 - m_write_idx ) ) {
        return false;
    }
    m_write_idx += len;
    va_end( arg_list );
    return true;
}
/*---------------------------------------------------------------Control--------------------------------------------*/
void http_conn::process()
{
    // cout<<"we are processing"<<endl;
    HTTP_CODE read_ret=process_read();
    if (read_ret==NO_REQUEST)
    {
        //设置文件可以去读数据
        modfd(m_epollfd,m_sockfd,EPOLLIN);
        return;
    }
    bool write_ret=process_write(read_ret);
    if ( !write_ret ) {
        close_conn();
    }
    modfd( m_epollfd, m_sockfd, EPOLLOUT);
}
void http_conn::init(int sockfd,const sockaddr_in& addr)
{
    m_sockfd=sockfd;
    m_address=addr;
    //端口复用
    int reuse=1;
    setsockopt(m_sockfd,SOL_SOCKET,SO_REUSEADDR,&reuse,sizeof(reuse));
    addfd(m_epollfd,sockfd,true);
    m_user_count++;
    init();
}
void http_conn::init()
{
    m_linger=false;//默认不保持连接
    m_check_state=CHECK_STATE_REQUESTLINE;
    m_read_index=0;
    m_start_line=0;
    m_checked_idx=0;
    m_host=0;
    m_version=0;
    m_content_length=0;
    m_url=0;
    m_method=GET;
    m_write_idx=0;
    bytes_to_send=0;
    bzero(m_real_file,FILENAME_LEN);
    bzero(m_read_buf,READ_BUFFER_SIZE);
    bzero(m_write_buf,WRITE_BUFFER_SIZE);
}
void http_conn::close_conn()
{
    if (m_sockfd!=-1)
    {
        removefd(m_epollfd,m_sockfd);
        m_sockfd=-1;
        m_user_count--;
    }
}

// 写HTTP响应
bool http_conn::write()
{
    int temp=0;
    int m_iv_to_send=m_iv_count;
    int m_iv_have_send=0;
    if (bytes_to_send==0||m_iv_count==0)
    {
        unmap();
        init();
        modfd(m_epollfd,m_sockfd,EPOLLIN);
        return false;
    }
    while(true)
    {
        temp=writev(m_sockfd,m_iv,m_iv_count);
        if (temp<=-1)
        {

            if (errno==EAGAIN)
            {
                modfd(m_epollfd,m_sockfd,EPOLLOUT);
                return true;
            }
            unmap();
            return false;
        }
        bytes_have_send+=temp;
        bytes_to_send-=temp;
        if (bytes_have_send>=m_iv[0].iov_len)
        {
            m_iv[0].iov_base=0;
            m_iv[0].iov_len=0;
            m_iv[1].iov_base=m_file_address+(bytes_have_send-m_write_idx);
            m_iv[1].iov_len=bytes_to_send;
        }
        else
        {
            m_iv[0].iov_base=m_write_buf+(bytes_have_send-m_write_idx);
            m_iv[0].iov_len=m_iv[0].iov_len-temp;
        }
        if (bytes_to_send<=0)
        {
            unmap();
            modfd(m_epollfd,m_sockfd,EPOLLIN);
            if (m_linger)
            {
                return true;
            }
            else
            {
                return false;
            }
        }
    }
}
// 对内存映射区执行munmap操作
void http_conn::unmap () 
{
    if(m_file_address)
    {
        munmap(m_file_address,m_file_stat.st_size);
        //解除文本映射
        m_file_address=0;
        //初始化文件描述符
    }
}