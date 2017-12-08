#ifndef XX_THREAD_MUTUX_H
#define XX_THREAD_MUTUX_H

#ifdef __cplusplus
extern "C"{
#endif

#ifdef WIN32
    #include <windows.h>
    #include <process.h>
    typedef HANDLE xx_thread_mutex;
#else
    #include <pthread.h>
    typedef pthread_mutex_t *xx_thread_mutex;
#endif

#define xx_lock(x) if(x){xx_mutex_lock(x);}
#define xx_unlock(x) if(x){xx_mutex_unlock(x);}

//创建互斥锁
//参数预留
//失败返回NULL
xx_thread_mutex xx_mutex_create(void *attr);

//锁
//返回值0 成功
int xx_mutex_lock(xx_thread_mutex mutex);

//解锁
//返回值0表示成功
int xx_mutex_unlock(xx_thread_mutex mutex);

//销毁锁
//0表示成功
int xx_mutex_destroy(xx_thread_mutex mutex);

#ifdef __cplusplus
}
#endif
#endif // XX_THREAD_MUTUX_H
