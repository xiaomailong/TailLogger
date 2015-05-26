#include "session.h"

#include <sys/socket.h>
#include <sys/un.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>

#define LOG_FILE_NAME_SIZE 100

static char log_file_name[LOG_FILE_NAME_SIZE] = {0};

#define BUF_SIZE 4096

static struct sockaddr_un server_address;
static int socket_fd = -1;

/*
 功能描述    : 检查session是否过期
 返回值      : 过期为-1，未过期为0
 参数        : 无
 日期        : 2015年5月26日 12:54:35
*/
int Session::CheckExpire(void)
{
    struct timeval cur_time;

    assert(NULL != session);

    gettimeofday(&cur_time,NULL);

    /*若时间相差1s则认为过期*/
    if (cur_time.tv_sec - session->last_ac_time.tv_sec > 1)
    {
        return -1;
    }

    return 0;
}

/*
 功能描述    : 初始化Session
 返回值      : 成功为0，失败为-1
 参数        : 无
 日期        : 2015年5月26日 12:57:07
*/
int Session::Init()
{
    int len = 0;
    this->fd = -1;

    if (0 > this->SockInit())
    {
        return -1;
    }

    gettimeofday(&session->start_time,NULL);
    gettimeofday(&session->last_ac_time,NULL);

    len = snprintf(log_file_name, LOG_FILE_NAME_SIZE, "cilog_%04d%02d%02d_%02d%02d%02d",
                session->start_time->tm_year + 1900,
                session->start_time->tm_mon + 1,
                session->start_time->tm_mday,
                session->start_time->tm_hour,
                session->start_time->tm_min,
                session->start_time->tm_sec);

    if(0 > len)
    {
        printf("生成配置文件名称字符串被截断，请检测文件名长度\n");
        log_file_name[LOG_FILE_NAME_SIZE - 1] = 0;
    }

    session->fd = open(log_file_name,O_WRONLY | O_CREAT,S_IRUSR);
    if (0 > session->fd)
    {
        printf("打开%s文件失败",log_file_name);

        return -1;
    }

    return 0;
}
/*
 功能描述    : 对Session进行回收
 返回值      : 成功为0，失败为-1
 参数        : 无
 日期        : 2015年5月26日 13:03:04
*/
int Session::Recycle()
{
    assert(NULL != session);

    /*在session过期的情况下应该对session进行回收*/
    if (0 > session->fd)
    {
        close(session->fd);
    }

    memset(&session->start_time, 0, sizeof(session->start_time));
    memset(&session->last_ac_time, 0, sizeof(session->last_ac_time));

    return 0;
}
/*
 功能描述    : 保存数据
 返回值      : 成功为0，失败为-1
 参数        : 无
 日期        : 2015年5月26日 13:35:40
*/
int Session::Write(void* data,int len)
{
    assert(NULL != data);

    return 0;
}
/*
 功能描述    : 将session当中的数据保存到本地磁盘当中
 返回值      : 成功为0，失败为-1
 参数        : 无
 日期        : 2015年5月26日 13:39:34
*/
int Session::Flush()
{
    return 0;
}
/*
 功能描述    : 开始服务
 返回值      : 成功为0，失败为-1
 参数        : 无
 日期        : 2015年5月26日 15:45:53
*/
int Session::Serve()
{
    struct sockaddr_un client_address;
    int bytes_received = 0;
    static char* buf[BUF_SIZE];
    socklen_t address_length = 0;

    while (1)
    {
        bytes_received = recvfrom(socket_fd, buf, BUF_SIZE, 0,
            (struct sockaddr *) &(client_address),
            &address_length);

        if (0 != strcmp(client_address.sun_path, ClientSockPath))
        {
            printf("ci client sock path wrong:%s\n",client_address.sun_path);
        }
        /*若session过期则进行回收并重新利用*/
        if (CheckExpire())
        {
            Recycle();
            Init();
        }
        Write(buf, bytes_received);
    }
    return 0;
}
/*
 功能描述    : session结束应该做的清除工作
 返回值      : 成功为0，失败为-1
 参数        : 无
 日期        : 2015年5月26日 15:54:00
*/
int Session::Clean(void)
{
    unlink(ServerSockPath);
    close(socket_fd);

    return 0;
}

int Session::SockInit(void)
{
    struct timeval tv;
    socklen_t address_length = sizeof(struct sockaddr_un);

    if ((socket_fd = socket(AF_UNIX, SOCK_DGRAM, 0)) < 0)
    {
        perror("server: socket init fail");
        return -1;
    }

    memset(&server_address, 0, sizeof(server_address));
    server_address.sun_family = AF_UNIX;
    strncpy(server_address.sun_path, ServerSockPath, sizeof(server_address.sun_path) - 1);

    /*并没有检查是否删除成功，当文件不存在的时候也无所谓*/
    unlink(ServerSockPath);

    if (bind(socket_fd, (const struct sockaddr *) &server_address, sizeof(server_address)) < 0)
    {
        close(socket_fd);
        perror("server: bind fail");
        return -1;
    }

    /*设置超时，当recvfrom在30s自动返回*/
    tv.tv_sec = 30;
    tv.tv_usec = 0;
    setsockopt(socket_fd, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv, sizeof(struct timeval));

    return 0;
}