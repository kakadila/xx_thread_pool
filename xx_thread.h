#ifndef XX_THREAD_H
#define XX_THREAD_H

#ifdef __cplusplus
extern "C"{
#endif

#include "xx_thread_util.h"

#ifdef WIN32
    #include <windows.h>
    #include <process.h>
    typedef HANDLE xx_thread_handle;
#else
    #include <pthread.h>
    typedef pthread_t xx_thread_handle;
#endif


//创建线程
//第三个参数没实现
//返回-1 表示失败
int xx_create_thread(xx_thread_handle *handle,XX_ThreadFun fun,void *arg,void *attr);

//回收线程
//返回0 表示成功，其他值失败
int xx_thread_join(xx_thread_handle handle);


#ifdef __cplusplus
}
#endif
#endif // XX_THREAD_H
