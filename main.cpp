/*********************************************************************
 Copyright (C), 2015,  Co.Hengjun, Ltd.

 版本        : 1.0
 创建日期    : 2015年5月26日
 历史修改记录: v1.0    创建
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
    /*该程序为服务进程为了避免打印信息，将其打印信息全部丢弃*/
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
