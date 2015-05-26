/*********************************************************************
 Copyright (C), 2015,  Co.Hengjun, Ltd.

 版本        : 1.0
 创建日期    : 2015年5月26日
 历史修改记录: v1.0    创建
**********************************************************************/
#include "session.h"

int main(int argc, char * argv[])
{
    int ret = 0;

    Session* session = Session.Instance();

    ret = session->Init();
    assert(-1 != ret);

    session->Serve();

    session->Clean();

    return 0;
}
