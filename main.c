/*********************************************************************
 Copyright (C), 2015,  Co.Hengjun, Ltd.

 版本        : 1.0
 创建日期    : 2015年5月26日
 历史修改记录: v1.0    创建
**********************************************************************/
#include <sys/socket.h>
#include <sys/un.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>

const char* ci_server_sock_path = "/tmp/CI_UNIX_SOCK_SRV";
const char* ci_client_sock_path = "/tmp/CI_UNIX_SOCK_CLIENT";
static struct sockaddr_un server_address;
static int socket_fd = -1;

static int sock_init(void)
{
    socklen_t address_length = sizeof(struct sockaddr_un);

    if ((socket_fd = socket(AF_UNIX, SOCK_DGRAM, 0)) < 0)
    {
        perror("server: socket init fail");
        return -1;
    }

    memset(&server_address, 0, sizeof(server_address));
    server_address.sun_family = AF_UNIX;
    strncpy(server_address.sun_path, ci_server_sock_path,sizeof(server_address.sun_path) - 1);

    /*并没有检查是否删除成功，当文件不存在的时候也无所谓*/
    unlink(ci_server_sock_path);

    if (bind(socket_fd, (const struct sockaddr *) &server_address, sizeof(server_address)) < 0)
    {
        close(socket_fd);
        perror("server: bind fail");
        return -1;
    }

    return 0;
}

static int sock_clean(void)
{
    unlink(ci_server_sock_path);
    close(socket_fd);

    return 0;
}

static int server_run()
{
    struct sockaddr_un client_address;
    int bytes_received = 0;
    int integer_buffer = 0;
    socklen_t address_length = 0;

    while (1)
    {
        bytes_received = recvfrom(socket_fd, (char *)&integer_buffer, sizeof(int), 0,
            (struct sockaddr *) &(client_address),
            &address_length);

        if (0 != strcmp(client_address.sun_path,ci_client_sock_path))
        {
            printf("ci client sock path wrong:%s\n",client_address.sun_path);
        }
    }
    return 0;
}

int main(int argc, char * argv[])
{
    int ret = 0;

    ret = sock_init();
    assert(-1 != ret);

    ret = server_run();
    assert(-1 != ret);

    ret = sock_clean();
    assert(-1 != ret);

    return 0;
}
