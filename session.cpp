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
static const int32_t cycle_buffer_len = (5 * 1024 * 1024);      /*5M*/

static struct sockaddr_un server_address;
static int socket_fd = -1;

Session Session::instance = NULL;

/*
 ��������    : ��ʼ��Session
 ����ֵ      : �ɹ�Ϊ0��ʧ��Ϊ-1
 ����        : ��
 ����        : 2015��5��26�� 12:57:07
*/
int Session::Init()
{
    this->fd = -1;

    if (0 > this->SockInit())
    {
        return -1;
    }
    cycle_buffer = new char[cycle_buffer_len];

    memset(cycle_buffer,0,cycle_buffer_len);

    cycle_buffer_start = -1;
    cycle_buffer_end = 0;

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

    /*��û�м���Ƿ�ɾ���ɹ������ļ������ڵ�ʱ��Ҳ����ν*/
    unlink(ServerSockPath);

    if (bind(socket_fd, (const struct sockaddr *) &server_address, sizeof(server_address)) < 0)
    {
        close(socket_fd);
        perror("server: bind fail");
        return -1;
    }

    /*���ó�ʱ����recvfrom��30s�Զ�����*/
#ifdef _DEBUG
    tv.tv_sec = 1;
#else
    tv.tv_sec = 30;
#endif /* _DEBUG*/
    tv.tv_usec = 0;
    setsockopt(socket_fd, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv, sizeof(struct timeval));

    return 0;
}
/*
 ��������    : ��ʼ����
 ����ֵ      : �ɹ�Ϊ0��ʧ��Ϊ-1
 ����        : ��
 ����        : 2015��5��26�� 15:45:53
*/
int Session::Serve()
{
    struct sockaddr_un client_address;
    int bytes_received = 0;
    static char buf[BUF_SIZE];
    socklen_t address_length = 0;

    while (1)
    {
        bytes_received = recvfrom(socket_fd, buf, BUF_SIZE, 0,
            (struct sockaddr *) &(client_address),
            &address_length);

        if (0 < bytes_received)
        {
            if (0 != strcmp(client_address.sun_path, ClientSockPath))
            {
                printf("ci client sock path wrong:%s\n",client_address.sun_path);
                continue;
            }
        }
        /*
         * ��session��������л��ղ���������
         * ��Ϊrecvfrom��ʱ��᷵�أ����Ա�֤�˵������ж�ʱ���ڴ浱�е����ݺܿ챻ͬ����Ӳ����
         */
        if (CheckExpired())
        {
            Recycle();
        }
        if (0 < bytes_received)
        {
            Write(buf, bytes_received);
        }
    }
    return 0;
}

/*
 ��������    : ���session�Ƿ����
 ����ֵ      : ����Ϊ-1��δ����Ϊ0
 ����        : ��
 ����        : 2015��5��26�� 12:54:35
*/
bool Session::CheckExpired(void)
{
    struct timeval cur_time;

    gettimeofday(&cur_time,NULL);

    /*��ʱ�����1s����Ϊ����*/
    if (cur_time.tv_sec - last_ac_time.tv_sec > 1)
    {
        return true;
    }

    return false;
}

/*
 ��������    : ��Session���л���
 ����ֵ      : �ɹ�Ϊ0��ʧ��Ϊ-1
 ����        : ��
 ����        : 2015��5��26�� 13:03:04
*/
int Session::Recycle()
{
    int len = 0;

    if (-1 != fd)
    {
        Flush();
        close(fd);
    }

    gettimeofday(&start_time,NULL);
    gettimeofday(&last_ac_time,NULL);

    len = snprintf(log_file_name, LOG_FILE_NAME_SIZE, "cilog_%04d%02d%02d_%02d%02d%02d",
                start_time->tm_year + 1900,
                start_time->tm_mon + 1,
                start_time->tm_mday,
                start_time->tm_hour,
                start_time->tm_min,
                start_time->tm_sec);

    if(0 > len)
    {
        printf("���������ļ������ַ������ضϣ������ļ�������\n");
        log_file_name[LOG_FILE_NAME_SIZE - 1] = 0;
    }

    fd = open(log_file_name,O_WRONLY | O_CREAT,S_IRUSR);
    if (0 > fd)
    {
        printf("��%s�ļ�ʧ��",log_file_name);

        return -1;
    }

    memset(cycle_buffer, 0, cycle_buffer_len);

    cycle_buffer_start = -1;
    cycle_buffer_end = 0;

    return 0;
}

int Session::CycleBufferWrite(void* data, int len)
{
    assert(NULL != data);

    int ending_len = 0;

    if (-1 == fd)
    {
        return -1;
    }
    ending_len = cycle_buffer_len - cycle_buffer_end;
    if (ending_len > len)
    {
        memcpy(cycle_buffer + cycle_buffer_end,data,len);
        cycle_buffer_end += len;
    }
    else if (ending_len == len)
    {
        memcpy(cycle_buffer + cycle_buffer_end,data,len);
        cycle_buffer_start = cycle_buffer_end = 0;
    }
    else
    {
        memcpy(cycle_buffer + cycle_buffer_end,data,ending_len);
        memcpy(cycle_buffer,data,len - ending_len);
        cycle_buffer_start = cycle_buffer_end = len - ending_len;
    }
    return 0;
}
/*
 ��������    : ��������
 ����ֵ      : �ɹ�Ϊ0��ʧ��Ϊ-1
 ����        : ��
 ����        : 2015��5��26�� 13:35:40
*/
int Session::Write(void* data,int len)
{
    assert(NULL != data);

    size_t slice_size = 4096;

    /*ȷ��len�ĳ��ȴ��ڻ���������ʱ������*/
    for (size_t i = 0; i < slice_size; i += slice_size)
    {
        CycleBufferWrite((char*)data + i, slice_size);
    }

    return 0;
}
/*
 ��������    : ��session���е����ݱ��浽���ش��̵���
 ����ֵ      : �ɹ�Ϊ0��ʧ��Ϊ-1
 ����        : ��
 ����        : 2015��5��26�� 13:39:34
*/
int Session::Flush()
{
    int ret = 0;

    /*������Ϊ�յ�ʱ�򲻱�������*/
    if (-1 == fd)
    {
        return -1;
    }
    if (-1 == cycle_buffer_start)
    {
        ret = write(fd,cycle_buffer,cycle_buffer_end);
        if (-1 == ret)
        {
            return -1;
        }
    }
    else
    {
        ret = write(fd,cycle_buffer + cycle_buffer_start,cycle_buffer_len - cycle_buffer_start);
        if (-1 == ret)
        {
            return -1;
        }
        ret = write(fd,cycle_buffer,cycle_buffer_end);
        if (-1 == ret)
        {
            return -1;
        }
    }
    return 0;
}
/*
 ��������    : session����Ӧ�������������
 ����ֵ      : �ɹ�Ϊ0��ʧ��Ϊ-1
 ����        : ��
 ����        : 2015��5��26�� 15:54:00
*/
int Session::Clean(void)
{
    unlink(ServerSockPath);
    close(socket_fd);

    delete cycle_buffer;

    return 0;
}
