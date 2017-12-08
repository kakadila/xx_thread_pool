#include "xx_sem.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

//创建信号量
xx_sem xx_sem_create(void *attr, int value)
{
    xx_sem sem = NULL;
#ifdef WIN32
    sem = CreateSemaphore(NULL,value,65535,NULL);
#else
    sem =  (sem_t *)malloc(sizeof(sem_t));
    int ret = sem_init(sem,0,value);
    if(ret <0)
    {
        free(sem);
        sem = NULL;
    }
#endif
    return sem;
}

//发送信号量
int xx_sem_post(xx_sem sem)
{
    int ret = 0;
#ifdef WIN32
    ret = !ReleaseSemaphore(sem,1,NULL);//释放信号量
#else
    ret = sem_post(sem);
#endif
    return ret;
}

//等待信号量
int xx_sem_wait(xx_sem sem,void *timevalue)
{
    int ret = 0;
#ifdef WIN32
    ret = (int)WaitForSingleObject(sem, INFINITE);//等待信号量
#else
    ret = sem_wait(sem);
#endif
    return ret;
}

//销毁信号量
int xx_sem_destroy(xx_sem sem)
{
    if(sem == NULL) return 0;
    int ret = 0;
#ifdef WIN32
    ret = !CloseHandle( sem );
#else
    ret = sem_destroy(sem);
    free(sem);
#endif
    return ret;
}
