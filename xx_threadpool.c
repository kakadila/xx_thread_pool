#include "xx_threadpool.h"
#include "xx_thread.h"
#include "xx_thread_mutux.h"
#include "xx_sem.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

#define XX_TASK_QUEUE_MAXSIZE 1024 //任务队列最大大小
#define XX_MANAGE_DELTATIME 1 //检查间隔
#define XX_THREAD_CHANG_NUM 5 //线程变化数量

#define TRUE 1
#define FALSE 0
typedef char XX_BOOL;



//任务信息
struct xx_task_info
{
    XX_ThreadFun func;
    void *arg;
};
//任务管理结构体
struct xx_task_manager
{
    xx_thread_mutex mutex; //本结构体锁
    int size;           //队列当前大小
    int capacity;       //队列最大容量
    int head;           //队头
    int tail;           //队尾
    xx_sem sem;         //队列信号量，当队列不为满时可以添加任务
    struct xx_task_info tasks_queue[XX_TASK_QUEUE_MAXSIZE]; //任务队列
};

//线程信息
struct xx_thread_info
{
    xx_thread_handle handle;
    int index;
    void *pool;
    XX_BOOL is_alive;
};

//线程管理结构体
struct xx_thread_manager
{
    xx_thread_mutex mutex; //本结构体锁
    int size;           //栈当前大小
    int max_size;       //栈最大容量
    int min_size;       //栈最小容量
    int work_num;       //正在工作线程个数
    int release_num;  //需要退出的线程数量
    xx_sem sem;         //当有任务时，使用信号量
    struct xx_thread_info *threads_stack; //线程栈
};

//线程池
struct __xx_thread_pool
{
    struct xx_task_manager task_manager; //任务队列管理
    struct xx_thread_manager thread_manager; //线程栈管理

    xx_thread_handle pool_thread; //管理线程池的线程
    XX_BOOL pool_thread_alive; //管理线程池的线程释放创建

    xx_thread_mutex pool_mutex; //线程池锁
    XX_BOOL is_finish; //结束标志
};

//线程栈中每个线程的函数
static void pool_thread_func(void *arg)
{
    struct xx_thread_info *info = (struct xx_thread_info *)arg;
    struct __xx_thread_pool *pool = (struct __xx_thread_pool *)(info->pool);
    XX_BOOL is_finish;
    struct xx_thread_manager *thread_manager = &(pool->thread_manager);
    struct xx_task_manager *task_manager = &(pool->task_manager);
    int ret;
    XX_ThreadFun func;
    void *param;

    while(1)
    {
        //等待信号量
        ret = xx_sem_wait(thread_manager->sem,NULL);
        if(ret != 0) break;

        //检测是否结束
        xx_lock(pool->pool_mutex);
        is_finish = pool->is_finish;
        xx_unlock(pool->pool_mutex);
        if(is_finish) break;


        xx_lock(thread_manager->mutex);
        //线程管理释放
        if(thread_manager->release_num>0)
        {
            --(thread_manager->release_num);
            xx_unlock(thread_manager->mutex);
            break;
        }
        //工作数量加1
        ++(thread_manager->work_num);
        xx_unlock(thread_manager->mutex);

        //从任务队列取任务
        xx_lock(task_manager->mutex);
        func = task_manager->tasks_queue[task_manager->head].func;
        param = task_manager->tasks_queue[task_manager->head].arg;
        task_manager->head = (task_manager->head + 1)% task_manager->capacity;
        --(task_manager->size);
        xx_unlock(task_manager->mutex);

        //发送信号给任务队列
        xx_sem_post(task_manager->sem);

        //工作
        (*func)(param);

        //工作数量-1
        xx_lock(thread_manager->mutex);
        --(thread_manager->work_num);
        xx_unlock(thread_manager->mutex);
    }

    //销毁线程,把栈的最后一个元素移动到当前位置
    xx_lock(thread_manager->mutex);
    int index = info->index;
    --(thread_manager->size);
    struct xx_thread_info *threads_stack = thread_manager->threads_stack;
    threads_stack[index] = threads_stack[thread_manager->size];
    threads_stack[thread_manager->size].is_alive = FALSE;
    xx_unlock(thread_manager->mutex);
}

//线程池管理线程函数
static void pool_manager_func(void *arg)
{
    struct __xx_thread_pool *pool = (struct __xx_thread_pool *)arg;
    struct xx_thread_manager *thread_manager = &(pool->thread_manager);
    XX_BOOL is_finish;
    int work_num,thread_num,i,ret,release_num;
    while(1)
    {
#ifdef WIN32
        Sleep(XX_MANAGE_DELTATIME * 1000);
#else
        sleep(XX_MANAGE_DELTATIME);
#endif
        //检测是否结束
        xx_lock(pool->pool_mutex);
        is_finish = pool->is_finish;
        xx_unlock(pool->pool_mutex);
        if(is_finish) break;

        //获取线程管理信息
        xx_lock(thread_manager->mutex);
        work_num = thread_manager->work_num;
        thread_num = thread_manager->size;
        xx_unlock(thread_manager->mutex);

       //printf("work_num = %d,thread_num = %d\n",work_num,thread_num);

        //根据特定算法改变线程数量
        if(thread_num * 0.8 < work_num)
        {
            //增加线程数量
            xx_lock(thread_manager->mutex);
            thread_manager->size += XX_THREAD_CHANG_NUM;
            if(thread_manager->size > thread_manager->max_size)
                thread_manager->size = thread_manager->max_size;
            //创建任务线程
            for(i = thread_num;i<thread_manager->size;++i)
            {
                thread_manager->threads_stack[i].index = i;
                thread_manager->threads_stack[i].pool = (void *)pool;
                ret = xx_create_thread(&thread_manager->threads_stack[i].handle,pool_thread_func,(void *)&thread_manager->threads_stack[i],NULL);
                if(ret != 0)
                {
                    break;
                }
                thread_manager->threads_stack[i].is_alive = TRUE;

            }
            if(i < thread_manager->size)
            {
                xx_unlock(thread_manager->mutex);
                break; //退出管理线程
            }
            xx_unlock(thread_manager->mutex);
        }else if(thread_num * 0.5 > work_num)
        {
            //释放一定数量线程
             release_num = XX_THREAD_CHANG_NUM;
             xx_lock(thread_manager->mutex);
             if(thread_manager->min_size > thread_manager->size - release_num)
             {
                release_num = abs(thread_manager->min_size -  thread_manager->size) ;
             }
             thread_manager->release_num = release_num;
             xx_unlock(thread_manager->mutex);

             //发送信号释放线程
            for(i = 0;i<release_num;++i)
            {
                xx_sem_post(thread_manager->sem);
            }

        }

    }
}



//创建线程池
//参1 ：最小线程数量，参2：最大线程数量
//失败返回NULL
xx_thread_pool xx_threadpool_create(int min_threadnum,int max_threadnum)
{
    struct __xx_thread_pool *pool = NULL;
    pool = (struct __xx_thread_pool *)malloc(sizeof(struct __xx_thread_pool));
    if(pool == NULL)
    {
        return NULL;
    }
    memset(pool,0,sizeof(struct __xx_thread_pool));

    pool->is_finish = FALSE;
    pool->pool_mutex = xx_mutex_create(NULL);
    if(pool->pool_mutex == NULL)
    {
        free(pool);
        return NULL;
    }

    struct xx_task_manager *task_manager = NULL;
    struct xx_thread_manager *thread_manager = NULL;
    int i,ret;
    do{
        //初始化任务管理
        task_manager = &pool->task_manager;
        task_manager->size = 0;
        task_manager->capacity = XX_TASK_QUEUE_MAXSIZE;
        task_manager->head = 0;
        task_manager->tail = 0;
        task_manager->sem = xx_sem_create(NULL,XX_TASK_QUEUE_MAXSIZE);
        if(task_manager->sem ==NULL)
        {
            break;
        }
        task_manager->mutex = xx_mutex_create(NULL);
        if(task_manager->mutex == NULL)
        {
            break;
        }

        //初始化线程栈管理
        thread_manager = &pool->thread_manager;
        thread_manager->size = min_threadnum;
        thread_manager->min_size = min_threadnum;
        thread_manager->max_size = max_threadnum;
        thread_manager->release_num = 0;
        thread_manager->work_num = 0;
        thread_manager->threads_stack = (struct xx_thread_info *)malloc(sizeof(struct xx_thread_info) *max_threadnum );
        if(thread_manager->threads_stack == NULL)
        {
            break;
        }
        memset(thread_manager->threads_stack,0,sizeof(struct xx_thread_info) *min_threadnum );
        thread_manager->sem = xx_sem_create(NULL,0);
        if(thread_manager->sem ==NULL)
        {
            break;
        }
        thread_manager->mutex = xx_mutex_create(NULL);
        if(thread_manager->mutex == NULL)
        {
            break;
        }
        //创建任务线程
        for(i = 0;i<min_threadnum;++i)
        {
            thread_manager->threads_stack[i].index = i;
            thread_manager->threads_stack[i].pool = (void *)pool;
            ret = xx_create_thread(&thread_manager->threads_stack[i].handle,pool_thread_func,(void *)&thread_manager->threads_stack[i],NULL);
            if(ret != 0)
            {
                break;
            }
            thread_manager->threads_stack[i].is_alive = TRUE;

        }
        if(i < min_threadnum) break;

        //创建管理线程
        ret = xx_create_thread(&pool->pool_thread,pool_manager_func,(void *)pool,NULL);
        if(ret != 0)
        {
            break;
        }
        pool->pool_thread_alive = TRUE;

        return (void *)pool;
    }while(0);

    xx_threadpool_del((void *)pool);

    return NULL;
}

//添加任务
//参数1：函数，参数2:函数参数
//返回0表示成功
int xx_threadpool_add(xx_thread_pool p,XX_ThreadFun func,void *arg)
{
    if(p == NULL)
    {
        return -1;
    }
    struct __xx_thread_pool *pool = (struct __xx_thread_pool *)p;
    int ret;
    //检验线程池是否退出
    xx_lock(pool->pool_mutex);
    if(pool->is_finish) return -1;
    xx_unlock(pool->pool_mutex);

    struct xx_task_manager *task_manager = &pool->task_manager;
    struct xx_thread_manager *thread_manager = &pool->thread_manager;

    //等待任务队列有位置
    ret = xx_sem_wait(task_manager->sem,NULL);
    //添加任务
    xx_lock(task_manager->mutex);
    struct xx_task_info *info = &task_manager->tasks_queue[task_manager->tail];
    info->func = func;
    info->arg = arg;
    task_manager->tail = (task_manager->tail + 1)% task_manager->capacity;
    xx_unlock(task_manager->mutex);

    //给线程栈发送有任务信号
    xx_sem_post(thread_manager->sem);

    return 0;
}


//线程池销毁
//返回0表示成功
int xx_threadpool_del(xx_thread_pool p)
{
    if(p == NULL) return 0;
    struct __xx_thread_pool *pool = (struct __xx_thread_pool *)p;
    struct xx_task_manager *task_manager = &pool->task_manager;
    struct xx_thread_manager *thread_manager = &pool->thread_manager;
    int i,ret;
    int size;

    //等待线程结束
    xx_lock(pool->pool_mutex);
    pool->is_finish = TRUE;
    xx_unlock(pool->pool_mutex);


    //释放管理线程
    if(pool->pool_thread_alive)
    {
        xx_thread_join(pool->pool_thread);
        pool->pool_thread_alive = FALSE;
    }

    //释放任务线程
    if(thread_manager->threads_stack != NULL)
    {
        xx_lock(thread_manager->mutex);
        size = thread_manager->size;
        xx_unlock(thread_manager->mutex);
        for(i = 0;i<size;++i)
        {
            if(thread_manager->threads_stack[i].is_alive)
                xx_sem_post(thread_manager->sem);
            else
                break;
        }
        for(i = 0;i<size;++i)
        {
            if(thread_manager->threads_stack[i].is_alive)
            {
                xx_thread_join(thread_manager->threads_stack[i].handle);
            }else
            {
                break;
            }
        }
    }



    //释放
    if(task_manager->sem != NULL)
    {
        xx_sem_destroy(task_manager->sem);
    }

    if(task_manager->mutex != NULL)
    {
        xx_mutex_destroy(task_manager->mutex);
    }

    if(thread_manager->sem != NULL)
    {
        xx_sem_destroy(thread_manager->sem);
    }

    if(thread_manager->mutex != NULL)
    {
        xx_mutex_destroy(thread_manager->mutex);
    }
    if(thread_manager->threads_stack != NULL)
    {
        free(thread_manager->threads_stack);
        thread_manager->threads_stack = NULL;
    }

    if(pool->pool_mutex != NULL)
    {
        xx_mutex_destroy(pool->pool_mutex);
    }
    if(pool != NULL)
    {
        free(pool);
        pool = NULL;
    }

    return 0;
}

int xx_threadpool_num(xx_thread_pool *p)
{
    if(p == NULL)
    {
        return -1;
    }
    struct __xx_thread_pool *pool = (struct __xx_thread_pool *)p;
    int thread_num = -1;
    xx_lock(pool->thread_manager.mutex);
    thread_num = pool->thread_manager.size;
    xx_unlock(pool->thread_manager.mutex);
    return thread_num;
}
