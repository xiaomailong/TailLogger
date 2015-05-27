/*********************************************************************
 Copyright (C), 2015,  Co.Hengjun, Ltd.

 �汾        : 1.0
 ��������    : 2015��5��26��
 ��ʷ�޸ļ�¼: v1.0    ����
**********************************************************************/
#include "session.h"
#include <fcntl.h>
#include <unistd.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>

const char* log_path = "/root/ci_log/";

int main(int argc, char * argv[])
{
    int ret = 0;
#ifndef _DEBUG
    int fd = 0;
    fd = open("/dev/null",O_RDWR);
    if (0 > fd)
    {
        perror("open /dev/null fail");
    }
    /*�ó���Ϊ�������Ϊ�˱����ӡ��Ϣ�������ӡ��Ϣȫ������*/
    dup2(fd,1);
    dup2(fd,2);
#endif /* _DEBUG*/

    mkdir(log_path, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);

    Session* session = Session::Instance();

    ret = session->Init();
    assert(-1 != ret);

    session->Serve();

    session->Clean();

    return 0;
}
