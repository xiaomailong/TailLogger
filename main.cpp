/*********************************************************************
 Copyright (C), 2015,  Co.Hengjun, Ltd.

 �汾        : 1.0
 ��������    : 2015��5��26��
 ��ʷ�޸ļ�¼: v1.0    ����
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
