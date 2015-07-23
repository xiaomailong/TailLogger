#ifndef _session_h__
#define _session_h__

#include <stdio.h>
#include <sys/time.h>
 
class Session
{
public:
    /*
     功能描述    : 检查session是否过期
     返回值      : 过期为true，否则为false
     参数        : 无
     日期        : 2015年5月26日 12:54:35
    */
    bool CheckExpired(void);
    /*
     功能描述    : 初始化Session
     返回值      : 成功为0，失败为-1
     参数        : 无
     日期        : 2015年5月26日 12:57:07
    */
    int Init(void);
    /*
     功能描述    : 对Session进行回收
     返回值      : 成功为0，失败为-1
     参数        : 无
     日期        : 2015年5月26日 13:03:04
    */
    int Recycle(void);
    /*
     功能描述    : 返回单例
     参数        : 无
     日期        : 2015年5月26日 15:38:29
    */
    static Session* Instance(void)
    {
        if (NULL == instance)
        {
            return new Session;
        }
        return instance;
    }
    /*
     功能描述    : 保存数据
     返回值      : 成功为0，失败为-1
     参数        : 无
     日期        : 2015年5月26日 13:35:40
    */
    int Write(void* data,int len);
    /*
     功能描述    : 将session当中的数据保存到本地磁盘当中
     返回值      : 成功为0，失败为-1
     参数        : 无
     日期        : 2015年5月26日 13:39:34
    */
    int Flush(void);
    /*
     功能描述    : 开始服务
     返回值      : 成功为0，失败为-1
     参数        : 无
     日期        : 2015年5月26日 15:45:53
    */
    int Serve(void);
    /*
     功能描述    : session结束应该做的清除工作
     返回值      : 成功为0，失败为-1
     参数        : 无
     日期        : 2015年5月26日 15:54:00
    */
    int Clean(void);
    /*
     功能描述    : 使用生产者，消费者模式生产手动回收标志
     返回值      : 成功为0，失败为-1
     参数        : 无
     日期        : 2015年7月23日 08:55:00
    */
    int ProduceManualRecycle(void);
    /*
     功能描述    : 使用生产者，消费者模式消费回收标志
     返回值      : 是否有手动回收标志
     参数        : 无
     日期        : 2015年7月23日 08:55:00
    */
    bool ConsumeManualRecycle(void);

protected:
    Session()
    {
    }
    int SockInit(void);

    int CycleBufferWrite(void* data,int len);
private:
    struct timeval start_time;
    struct timeval last_ac_time;    /*上一次访问时间*/

    static Session* instance;
    char* cycle_buffer;
    int cycle_buffer_start; /*起始值只用来表示缓冲区是否已被填满，开始循环填充，当数据循环时，头跟尾相等*/
    int cycle_buffer_end;   /*末尾地址*/

    bool b_manual_recycle;
};

#endif /*!_session_h__*/
