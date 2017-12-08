#include "xx_thread.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

struct xx_arg
{
    XX_ThreadFun fun;
    void *arg;
};

#ifdef WIN32
unsigned int __stdcall Win_ThreadFun(PVOID pM)
{
    struct xx_arg *xa = (struct xx_arg *)pM;
    XX_ThreadFun fun = xa->fun;
    void *arg = xa->arg;
    free(xa);
    xa = NULL;
    (*fun)(arg);

    return 0;
}
#else
void *Linux_ThreadFun(void *pM)
{
    struct xx_arg *xa = (struct xx_arg *)pM;
    XX_ThreadFun fun = xa->fun;
    void *arg = xa->arg;
    free(xa);
    xa = NULL;
    (*fun)(arg);
    return NULL;
}
#endif

//创建线程
int xx_create_thread(xx_thread_handle *handle,XX_ThreadFun fun, void *arg, void *attr)
{
    struct xx_arg *xa = (struct xx_arg *)malloc(sizeof(struct xx_arg));
    memset(xa,0,sizeof(struct xx_arg));
    xa->fun = fun;
    xa->arg = arg;
#ifdef WIN32
    *handle = (xx_thread_handle)_beginthreadex(NULL,0,Win_ThreadFun,(void *)xa,0,NULL);
    if((int)handle == 0)
    {
        free(xa);
        xa = NULL;
        return (-1);
    }
#else
    int ret = pthread_create(handle,NULL,Linux_ThreadFun,(void *)xa);
    if(ret != 0 )
    {
        free(xa);
        xa = NULL;
        return (-1);
    }
#endif
    return 0;
}

//回收线程
int xx_thread_join(xx_thread_handle handle)
{
    int ret = 0;
#ifdef WIN32
     ret = (int)WaitForSingleObject(handle,INFINITE);
     if(ret == WAIT_OBJECT_0)
     {
        ret = (int)(!CloseHandle(handle));
     }
#else
    return pthread_join(handle,NULL);
#endif
}
