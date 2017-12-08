#ifndef XX_THREADPOOL_H
#define XX_THREADPOOL_H
#ifdef __cplusplus
extern "C"{
#endif

#include "xx_thread_util.h"

typedef void *xx_thread_pool; //线程池

//创建线程池
//参1 ：最小线程数量，参2：最大线程数量
//失败返回NULL
xx_thread_pool xx_threadpool_create(int min_threadnum,int max_threadnum);

//添加任务
//参数1：函数，参数2:函数参数
//返回0表示成功
int xx_threadpool_add(xx_thread_pool pool,XX_ThreadFun func,void *arg);


//线程池销毁
//返回0表示成功
int xx_threadpool_del(xx_thread_pool pool);

//返回线程池中线程数量
//返回-1失败
int xx_threadpool_num(xx_thread_pool *pool);
#ifdef __cplusplus
}
#endif
#endif // XX_THREADPOOL_H
