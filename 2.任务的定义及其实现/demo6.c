/*
 * Demo2: 信号量机制详解
 * 学习要点：
 * 1. 二进制信号量 - 任务间同步
 * 2. 计数信号量 - 资源计数管理  
 * 3. 互斥锁 - 保护共享资源，防止优先级反转
 * 4. 实际应用场景演示
 */
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include <stdio.h>
#include <string.h>

// 定义最大资源数
#define MAX_RESOURCES 3

//信号量句柄
SemaphoreHandle_t binary_semaphore;         //二进制信号量
SemaphoreHandle_t counting_semaphore;       //计数信号量
SemaphoreHandle_t mutex_semaphore;          //互斥锁

//任务句柄
TaskHandle_t producer_task_handle;
TaskHandle_t consumer_task_handle;
TaskHandle_t work1_task_handle,work2_task_handle,work3_task_handle;
TaskHandle_t resource_user1_task_handle,resource_user2_task_handle;

//共享资源
volatile uint32_t shared_counter=0;         //二进制信号量使用
char shared_buffer[100];                    //互斥锁使用
volatile uint32_t resource_pool_count=0;    //计数信号量使用


// ==================== 二进制信号量示例 ====================
// 生产者任务 - 产生数据后通知消费者
void producer_task(void* pvParameters){
    uint32_t data=0;

    for(;;){
        data++;
        shared_counter=data;

        printf("[生产者] 生产了数据: %lu\n", data);

        // 发送信号通知消费者
        xSemaphoreGive(binary_semaphore);

        //每两秒产生一次
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

void consumer_task(void* pvParameters){
    uint32_t consumer_data;

    for(;;){
        // 等待生产者信号（永久等待）
        if(xSemaphoreTake(binary_semaphore,portMAX_DELAY)==pdTRUE){
            consumer_data=shared_counter;

            printf("[消费者] 消费了数据: %lu\n", consumer_data);

            //模拟数据处理时间
            vTaskDelay(pdMS_TO_TICKS(500));
        }
    }
}


// ==================== 计数信号量示例 ====================
//工作任务-竞争有限资源

void worker_task(void *pvParameters){
    int worker_id=(int)pvParameters;

    for(;;){
        printf("[工作者%d] 尝试获取资源...\n", worker_id);

        if(xSemaphoreTake(counting_semaphore,pdMS_TO_TICKS(5000))==pdTRUE){
            //成功获取资源
            resource_pool_count++;
            printf("[工作者%d] 获取资源，当前资源数%lu\n", worker_id,resource_pool_count);

            //使用资源（模拟工作）
            vTaskDelay(pdMS_TO_TICKS(3000));

            //释放资源
            resource_pool_count--;
            xSemaphoreGive(counting_semaphore);
            printf("[工作者%d] 释放资源，当前资源数%lu\n", worker_id,resource_pool_count);
        }else{
            printf("[工作者%d] 尝试获取资源失败\n", worker_id);
        }

        //休息一段时间
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}


// ==================== 互斥锁示例 ====================
// 资源使用者任务 - 保护共享资源

void resource_user_task(void *pvParameters){
    int user_id=(int)pvParameters;
    char temp_buffer[50];

    for(;;){
        printf("[用户%d] 尝试获取互斥锁...\n",user_id);

        //获取互斥锁保护共享资源
        if(xSemaphoreTake(mutex_semaphore,pdMS_TO_TICKS(10000))==pdTRUE){
            printf("[用户%d] 获取互斥锁成功，开始操作共享资源\n", user_id);

            //临界区-操作共享资源
            sprintf(temp_buffer,"用户%d写入的数据",user_id);
            strcpy(shared_buffer,temp_buffer);

            printf("[用户%d] 写入: %s\n", user_id, shared_buffer);

            // 模拟长时间操作
            vTaskDelay(pdMS_TO_TICKS(2000));
            
            printf("[用户%d] 读取: %s\n", user_id, shared_buffer);
            printf("[用户%d] 完成操作，释放互斥锁\n", user_id);

            //释放互斥锁
            xSemaphoreGive(mutex_semaphore);
        }else{
            printf("[用户%d] 获取互斥锁超时！\n", user_id);
        }

        vTaskDelay(pdMS_TO_TICKS(3000));
    }
}


int main(){
    printf("信号量机制详解\n");
    //创建二进制信号量
    binary_semaphore=xSemaphoreCreateBinary();
    if(binary_semaphore==NULL){
        printf("二进制信号量创建失败\n");
        return -1;
    }

    //创建计数信号量（最大值为MAX_RESOURCES，初始值为MAX_RESOURCES）
    counting_semaphore=xSemaphoreCreateCounting(MAX_RESOURCES,MAX_RESOURCES);
    if(counting_semaphore==NULL){
        printf("计数信号量创建失败\n");
        return -1;
    }

    //创建互斥锁
    mutex_semaphore=xSemaphoreCreateMutex();
    if(mutex_semaphore==NULL){
        printf("互斥信号量创建失败\n");
        return -1;
    }

    printf("所有信号量创建成功!\n");
    
    // 创建二进制信号量相关任务
    xTaskCreate(producer_task, "Producer", 256, NULL, 3, &producer_task_handle);
    xTaskCreate(consumer_task, "Consumer", 256, NULL, 2, &consumer_task_handle);
    
    // 创建计数信号量相关任务
    xTaskCreate(worker_task, "Worker1", 256, (void*)1, 2, &work1_task_handle);
    xTaskCreate(worker_task, "Worker2", 256, (void*)2, 2, &work2_task_handle);  
    xTaskCreate(worker_task, "Worker3", 256, (void*)3, 2, &work3_task_handle);
    
    // 创建互斥锁相关任务
    xTaskCreate(resource_user_task, "ResUser1", 256, (void*)1, 1, &resource_user1_task_handle);
    xTaskCreate(resource_user_task, "ResUser2", 256, (void*)2, 1, &resource_user2_task_handle);
    
    printf("所有任务创建完成，启动调度器...\n");

    vTaskStartScheduler();

    printf("调度器启动失败！\n");

    return -1;
}

// 应用钩子函数
void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName) {
    printf("堆栈溢出: %s\n", pcTaskName);
    for(;;);
}

void vApplicationMallocFailedHook(void) {
    printf("内存分配失败!\n");
    for(;;);
}

/*
学习要点总结：

1. 二进制信号量：
   - 只有0和1两个状态
   - 用于任务间同步，相当于事件通知
   - Give操作设置为1，Take操作设置为0并可能阻塞
   - 适用场景：中断通知任务、任务间同步

2. 计数信号量：
   - 可以有多个计数值
   - 用于管理有限数量的资源
   - Give操作增加计数，Take操作减少计数
   - 适用场景：资源池管理、缓冲区管理

3. 互斥锁：
   - 特殊的二进制信号量，具有优先级继承特性
   - 同一任务Take后必须由同一任务Give
   - 防止优先级反转问题
   - 适用场景：保护共享资源、临界区保护

4. 重要API：
   - xSemaphoreCreateBinary()：创建二进制信号量
   - xSemaphoreCreateCounting()：创建计数信号量  
   - xSemaphoreCreateMutex()：创建互斥锁
   - xSemaphoreTake()：获取信号量
   - xSemaphoreGive()：释放信号量
   - uxSemaphoreGetCount()：获取计数信号量当前值

5. 注意事项：
   - 在中断中使用FromISR版本的API
   - 互斥锁不能在中断中使用
   - 避免在持有互斥锁时调用可能阻塞的函数
   - 合理设置超时时间避免死锁
*/