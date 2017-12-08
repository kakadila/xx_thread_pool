# xx_thread_pool
a threadpool can be used in both windows and linux

## use these fun ,you can use this threadpool
<br>  
>>>  this fun create a new threadpool   
### xx_thread_pool xx_threadpool_create(int min_threadnum,int max_threadnum);   

<br>   
>>>add a new task into threadpool   
### int xx_threadpool_add(xx_thread_pool pool,XX_ThreadFun func,void *arg);  
<br>   
>>>stop and delete the threadpool   
### int xx_threadpool_del(xx_thread_pool pool);   