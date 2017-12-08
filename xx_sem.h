#ifndef XX_SEM_H
#define XX_SEM_H

#ifdef __cplusplus
extern "C"{
#endif

//条件变量

#ifdef WIN32
    #include <windows.h>
    #include <process.h>
    typedef HANDLE xx_sem;
#else
    #include <pthread.h>
    #include <semaphore.h>
    typedef sem_t *xx_sem;
#endif

//创建信号量
//参数1 预留，参数2 初始信号量值
//失败返回NULL
xx_sem xx_sem_create(void *attr,int value);

//发送信号
//0表示成功
int xx_sem_post(xx_sem sem);

//等待信号量
//第二个参数保留，等待时间无限
//返回0表示成功
int xx_sem_wait(xx_sem sem,void *timevalue);

//销毁信号量
//返回0 表示成功
int xx_sem_destroy(xx_sem sem);

#ifdef __cplusplus
}
#endif

#endif // XX_SEM_H
