/*
 * 练习3：高级时间片应用
 * 目标：理解实际项目中时间片的应用场景和最佳实践
 *
 * 练习要求：
 * 1. 理解生产者-消费者模型中时间片的作用
 * 2. 观察三个工作者任务如何通过时间片公平地处理工作队列中的任务
 * 3. 分析不同优先级任务对系统整体性能的影响
 * 4. 修改工作者任务的数量，观察系统吞吐量的变化
 * 5. 实验taskYIELD()在simulate_work()函数中的作用
 * 
 * 扩展练习：
 * 1. 添加任务亲和性机制（某些任务只能由特定工作者处理）
 * 2. 实现动态优先级调整（根据任务处理时间调整优先级）
 * 3. 添加任务超时机制和错误处理
 * 4. 实现工作者任务的动态创建和销毁
 * 5. 添加系统性能监控和负载均衡功能
 * 
 * 观察重点：
 * - 工作者任务之间的时间片分配是否公平
 * - 队列满时的系统行为
 * - 不同优先级任务的抢占情况
 * - 系统在高负载下的稳定性
 */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "timers.h"

/* 系统配置 */
#define MAX_WORKERS 3
#define WORK_QUEUE_SIZE 10
#define RESULT_QUEUE_SIZE 10

/* 工作项结构体 */
typedef struct {
    uint32_t work_id;         // 工作项的唯一标识
    uint32_t data;            // 要处理的数据
    uint32_t processing_time; // 模拟处理时间（复杂度，1-5）
} WorkItem_t;

/* 结果结构体 */
typedef struct {
    uint32_t work_id;
    uint32_t result;
    uint8_t worker_id;
    uint32_t actual_time;
} ResultItem_t;

/* 全局队列 */
QueueHandle_t xWorkQueue;
QueueHandle_t xResultQueue;

/* 同步信号量 */
SemaphoreHandle_t xPrintMutex;

/* 任务句柄 */
TaskHandle_t xProducerHandle;
TaskHandle_t xWorkerHandles[MAX_WORKERS];
TaskHandle_t xConsumerHandle;
TaskHandle_t xMonitorHandle;

/* 统计信息 */
typedef struct {
    uint32_t tasks_processed;       // 已处理的任务数
    uint32_t total_processing_time; // 总处理时间（ticks）
    uint32_t max_processing_time;   // 最大单次处理时间
    uint32_t min_processing_time;   // 最小单次处理时间
} WorkerStats_t;

volatile WorkerStats_t worker_stats[MAX_WORKERS];
volatile uint32_t g_total_tasks_generated = 0;
volatile uint32_t g_total_tasks_completed = 0;

//模拟工作函数
uint32_t simulate_work(uint32_t data,uint32_t complexity){
    uint32_t result=data;
    TickType_t start_time=xTaskGetTickCount();

    //模拟复杂的过程
    for(uint32_t i=0;i<complexity*100;i++){
        result=(result*7+3)%1000;

        if(i%50==0){
            //偶尔让出CPU，模拟实际应用中的协作式调度
            taskYIELD();
        }
    }

    return result;
}


void vProducerTask(void *pvParameters){
    WorkItem_t work_item;
    TickType_t xLastWakeTime;
    const TickType_t xFrequency=pdMS_TO_TICKS(50);

    xLastWakeTime=xTaskGetTickCount();

    for(;;){
        //准备工作项
        work_item.work_id=g_total_tasks_generated++;
        work_item.data=(work_item.work_id*7+3)%1000;
        work_item.processing_time=(work_item.work_id % 5) + 1;

        if(xQueueSend(xWorkQueue,&work_item,pdMS_TO_TICKS(10))==pdTRUE){
            //成功发送
        }else{
            //队列满，可以记录丢失的任务
        }

        //周期性生成任务
        vTaskDelayUntil(&xLastWakeTime,xFrequency);
    }
}

void vWorkerTask(void *pvParameters){
    uint8_t worker_id =(uint8_t)(uintptr_t)pvParameters;
    WorkItem_t work_item;
    ResultItem_t result_item;
    TickType_t start_time,end_time;

    //初始化统计信息
    worker_stats[worker_id].min_processing_time=0xFFFFFFFF;

    for(;;){
        if(xQueueReceive(xWorkQueue,&work_item,portMAX_DELAY)==pdTRUE){
            start_time=xTaskGetTickCount();

            //处理工作项
            result_item.work_id = work_item.work_id;
            result_item.worker_id = worker_id;
            result_item.result = simulate_work(work_item.data, work_item.processing_time);

            end_time=xTaskGetTickCount();
            result_item.actual_time=end_time - start_time;

            worker_stats[worker_id].tasks_processed++;
            worker_stats[worker_id].total_processing_time += result_item.actual_time;

            if(result_item.actual_time > worker_stats[worker_id].max_processing_time) {
                worker_stats[worker_id].max_processing_time = result_item.actual_time;
            }
            if(result_item.actual_time < worker_stats[worker_id].min_processing_time) {
                worker_stats[worker_id].min_processing_time = result_item.actual_time;
            }

            xQueueSend(xResultQueue,&result_item,portMAX_DELAY);
        }
    }
}

void xConsumerTask(void *pvParameters){
    ResultItem_t result_item;

    for(;;){
        if(xQueueReceive(xResultQueue,&result_item,portMAX_DELAY)==pdTURE){
            g_total_tasks_completed++;

            /* 处理结果（这里只是简单计数）*/
            /* 在实际应用中，这里可能会将结果写入文件、发送到网络等 */
            
            /* 模拟结果处理时间 */
            vTaskDelay(pdMS_TO_TICKS(5));
        }
    }
}

void xMonitorTask(void *pvParameters){
    TickType_t xLastWakeTime;
    const TickType_t xFrenquency=pdMS_TO_TICKS(1000);

    xLastWakeTime=xTaskGetTickCount();

    for(;;){
        vTaskDelayUntile(xLastWakeTime,xFrenquency);

        if(xSemaphoreTask(xPrintMutex,pdMS_TO_TICKS(100)==pdTURE)){
            printf("\n=== 系统状态报告 ===\n");
            printf("任务生成: %d, 任务完成: %d\n", 
                   g_total_tasks_generated, g_total_tasks_completed);

            printf("工作队列: %d/%d, 结果队列: %d/%d\n",
                    uxQueueMessagesWaiting(xWorkQueue),
                    WORK_QUEUE_SIZE,
                    uxQueueMessagesWaiting(xResultQueue),
                    RESULT_QUEUE_SIZE);

            for(int i = 0; i < MAX_WORKERS; i++) {
                printf("工作者%d: 处理%d个任务, 平均时间%d ticks\n",
                       i, worker_stats[i].tasks_processed,
                       worker_stats[i].tasks_processed > 0 ? 
                       worker_stats[i].total_processing_time / worker_stats[i].tasks_processed : 0);
            }

            xSemaphoreGive(xPrintMutex);
        }
    }
}


//定时器回调函数 - 系统负载调整
void vLoadAdjustTimerCallback(TimerHandle_t xTimer){
    static uint8_t load_level=1;

    //动态调整系统负载（这里只是示例）
    load_level=(load_level%3)+1;

    //可以根据load_level调整生产者的生成频率
    //实际应用中可能根据系统资源使用情况动态调整
}


int main(void){
    //创建队列
    xWorkQueue=xQueueCreate(WORK_QUEUE_SIZE,sizeof(WorkItem_t));
    xResultQueue=xQueueCreate(RESULT_QUEUE_SIZE,sizeof(ResultItem_t));

    //创建互斥锁
    xPrintMutex=xSemaphoreCreateMutex();

    //创建生产者任务
    xTaskCreate(vProducerTask,"Producer",configMINIMAL_STACK_SIZE,NULL,3,&xProducerHandle);

    //创建多个同优先级的工作者任务（时间片轮转）
    for(i=0;i<MAX_WORKERS;i++){
        char task_name[16];
        snprintf(task_name,sizeof(task_name),"Worker%d",i);
        xTaskCreate(vWorkerTask,"task_name",configMINIMAL_STACK_SIZE,(void*)(uintptr_t)i,2,&xWorkerHandles[i]);
    }

    //创建消费者任务
    xTaskCreate(xConsumerTask,"Consumer",configMINIMAL_STACK_SIZE,NULL,1,&xConsumerHandle);

    //创建监控任务
    xTaskCreate(xMonitorTask,"Monitor",configMINIMAL_STACK_SIZE,NULL,4,&xMonitorHandle);

    //创建定时器
    TimerHandle_t xLoadAdjustTimer = xTimerCreate(
        "LoadAdjust",              // 定时器名称
        pdMS_TO_TICKS(5000),       // 定时器周期（5秒）
        pdTRUE,                    // 自动重载
        NULL,                      // 定时器ID
        vLoadAdjustTimerCallback   // 回调函数
    );

    //启动调度器
    vTaskStartScheduler();

    for(;;);
}

void vApplicationMallocFailedHook(void){
    //内存分配失败
    for(;;);
}

void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName){
    //栈溢出
    for(;;);
}
