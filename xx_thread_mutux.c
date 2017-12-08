#include "xx_thread_mutux.h"
#include <string.h>
#include <stdlib.h>


//创建锁
xx_thread_mutex xx_mutex_create(void *attr)
{
    xx_thread_mutex mutex = NULL;
#ifdef WIN32
    mutex = CreateMutex(NULL, FALSE, NULL);
    if (GetLastError() == ERROR_ALREADY_EXISTS) // 已经有了一个实例
    {
       return NULL;
    }
#else
    xx_thread_mutex pmutex = (xx_thread_mutex )malloc(sizeof(pthread_mutex_t));
    if(pmutex == NULL) return NULL;
    int ret =pthread_mutex_init(pmutex,NULL);
    if(ret != 0)
    {
        return NULL;
    }
    mutex = pmutex;
#endif
    return mutex;
}

//加锁
int xx_mutex_lock(xx_thread_mutex mutex)
{
    int ret = 0;
#ifdef WIN32
    ret = (int)WaitForSingleObject(mutex, INFINITE);//等待互斥量
#else
    ret = (int)pthread_mutex_lock((pthread_mutex_t *)mutex);
#endif
    return ret;
}

//解锁
int xx_mutex_unlock(xx_thread_mutex mutex)
{
    int ret = 0;
#ifdef WIN32
    ret = (int)(!ReleaseMutex(mutex));
#else
    ret = pthread_mutex_unlock((pthread_mutex_t *)mutex);
#endif
    return ret;
}

//销毁锁
int xx_mutex_destroy(xx_thread_mutex mutex)
{
    int ret = 0;
#ifdef WIN32
    ret = (int)(!CloseHandle(mutex));
#else
    ret = pthread_mutex_destroy((pthread_mutex_t *)mutex);
#endif
    return ret;
}
