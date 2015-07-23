#include "session.h"

#include <sys/socket.h>
#include <sys/un.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <time.h>
#include <fcntl.h>
#include <sys/stat.h>

#define LOG_FILE_NAME_SIZE 100
#define BUF_SIZE 4096

static char log_file_name[LOG_FILE_NAME_SIZE] = {0};

static const int32_t cycle_buffer_len = (1 * 1024 * 1024);      /*1M*/

static struct sockaddr_un server_address;
static int socket_fd = -1;

Session* Session::instance = NULL;
static const char* ServerSockPath = "/tmp/CI_UNIX_SOCK_SRV";
static const char* ClientSockPath = "/tmp/CI_UNIX_SOCK_CLIENT";

extern const char* log_path;

/*
 ��������    : ��ʼ��Session
 ����ֵ      : �ɹ�Ϊ0��ʧ��Ϊ-1
 ����        : ��
 ����        : 2015��5��26�� 12:57:07
*/
int Session::Init()
{
    if (0 > this->SockInit())
    {
        return -1;
    }
    cycle_buffer = new char[cycle_buffer_len];

    memset(cycle_buffer,0,cycle_buffer_len);

    cycle_buffer_start = -1;
    cycle_buffer_end = 0;
    b_manual_recycle = 0;

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
        memset(&client_address,0,sizeof(client_address));
        bytes_received = recvfrom(socket_fd, buf, BUF_SIZE, 0,
            (struct sockaddr *) &client_address,
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
        if (CheckExpired() || ConsumeManualRecycle())
        {
            Recycle();
        }
        if (0 < bytes_received)
        {
            Write(buf, bytes_received);
            gettimeofday(&last_ac_time,NULL);
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
    if (cur_time.tv_sec - last_ac_time.tv_sec > 5)
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
    struct tm* tm = NULL;
    time_t cur_time;
    struct stat st;

    Flush();

    gettimeofday(&start_time,NULL);
    gettimeofday(&last_ac_time,NULL);

    cur_time = time(NULL);
    tm = localtime(&cur_time);

    assert(NULL != log_path);

    memset(log_file_name,0,LOG_FILE_NAME_SIZE);

    len = snprintf(log_file_name, LOG_FILE_NAME_SIZE, "%s/log_%04d%02d%02d_%02d%02d%02d",
            log_path,
            tm->tm_year + 1900,
            tm->tm_mon + 1,
            tm->tm_mday,
            tm->tm_hour,
            tm->tm_min,
            tm->tm_sec);

    if(0 > len)
    {
        printf("���������ļ������ַ������ضϣ������ļ�������\n");
        log_file_name[LOG_FILE_NAME_SIZE - 1] = 0;
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
    if(0 >= len)
    {
        return -1;
    }

    assert(cycle_buffer_len >= 4096);
    assert(cycle_buffer_len > cycle_buffer_end);

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
    size_t slice_count = len / slice_size;
    size_t i = 0;

    /*ȷ��len�ĳ��ȴ��ڻ���������ʱ������*/
    for (i = 0; i < slice_count; i += 1)
    {
        CycleBufferWrite((char*)data + i * slice_size, slice_size);
    }

    CycleBufferWrite((char*)data + i * slice_size, len - i * slice_size);

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
    int fd = 0;

    /*���浱��������*/
    if (-1 == cycle_buffer_start && 0 == cycle_buffer_end)
    {
        return -1;
    }
    printf("Flush:cycle_buffer_start:%d\t cycle_buffer_end:%d\n",cycle_buffer_start,cycle_buffer_end);

    fd = open(log_file_name, O_WRONLY | O_CREAT, S_IRUSR);
    if (0 > fd)
    {
        printf("��%s�ļ�ʧ��", log_file_name);

        return -1;
    }

    if (-1 == cycle_buffer_start)
    {
        ret = write(fd,cycle_buffer,cycle_buffer_end);
        if (-1 == ret)
        {
            goto fail;
        }
    }
    else
    {
        ret = write(fd,cycle_buffer + cycle_buffer_end,cycle_buffer_len - cycle_buffer_end);
        if (-1 == ret)
        {
            goto fail;
        }
        ret = write(fd,cycle_buffer,cycle_buffer_end);
        if (-1 == ret)
        {
            goto fail;
        }
    }
    close(fd);
    return 0;

fail:
    close(fd);
    return -1;
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
/*
 ��������    : ʹ�������ߣ�������ģʽ�����ֶ����ձ�־
 ����ֵ      : �ɹ�Ϊ0��ʧ��Ϊ-1
 ����        : ��
 ����        : 2015��7��23�� 08:55:00
*/
int Session::ProduceManualRecycle(void)
{
    b_manual_recycle = 1;

    return 0;
}
/*
 ��������    : ʹ�������ߣ�������ģʽ���ѻ��ձ�־
 ����ֵ      : �Ƿ����ֶ����ձ�־
 ����        : ��
 ����        : 2015��7��23�� 08:55:00
*/
int Session::ConsumeManualRecycle(void)
{
    int temp = b_manual_recycle;

    b_manual_recycle = 0;

    return temp;
}

