#ifndef _session_h__
#define _session_h__

#include <time.h>
 
class Session
{
public:
    /*
     ��������    : ���session�Ƿ����
     ����ֵ      : ����Ϊtrue������Ϊfalse
     ����        : ��
     ����        : 2015��5��26�� 12:54:35
    */
    bool CheckExpire(void);
    /*
     ��������    : ��ʼ��Session
     ����ֵ      : �ɹ�Ϊ0��ʧ��Ϊ-1
     ����        : ��
     ����        : 2015��5��26�� 12:57:07
    */
    int Init(void);
    /*
     ��������    : ��Session���л���
     ����ֵ      : �ɹ�Ϊ0��ʧ��Ϊ-1
     ����        : ��
     ����        : 2015��5��26�� 13:03:04
    */
    int Recycle(void);
    /*
     ��������    : ���ص���
     ����        : ��
     ����        : 2015��5��26�� 15:38:29
    */
    static Session* Instance(void)
    {
        if (NULL == instance)
        {
            return new Session;
        }
        return instance
    }
    /*
     ��������    : ��������
     ����ֵ      : �ɹ�Ϊ0��ʧ��Ϊ-1
     ����        : ��
     ����        : 2015��5��26�� 13:35:40
    */
    int Write(void* data,int len);
    /*
     ��������    : ��session���е����ݱ��浽���ش��̵���
     ����ֵ      : �ɹ�Ϊ0��ʧ��Ϊ-1
     ����        : ��
     ����        : 2015��5��26�� 13:39:34
    */
    int Flush(void);
    /*
     ��������    : ��ʼ����
     ����ֵ      : �ɹ�Ϊ0��ʧ��Ϊ-1
     ����        : ��
     ����        : 2015��5��26�� 15:45:53
    */
    int Serve(void);
    /*
     ��������    : session����Ӧ�������������
     ����ֵ      : �ɹ�Ϊ0��ʧ��Ϊ-1
     ����        : ��
     ����        : 2015��5��26�� 15:54:00
    */
    int Clean(void);

    const char* ServerSockPath = "/tmp/CI_UNIX_SOCK_SRV";
    const char* ClientSockPath = "/tmp/CI_UNIX_SOCK_CLIENT";

protected:
    Session()
    {
    }
    int SockInit(void);

private:
    struct timeval start_time;
    struct timeval last_ac_time;    /*��һ�η���ʱ��*/

    int fd;

    static Session* instance;
};

Session Session::instance = NULL;

#endif /*!_session_h__*/