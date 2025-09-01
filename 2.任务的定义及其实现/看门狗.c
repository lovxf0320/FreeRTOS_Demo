#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include <stdio.h>
#include <stdbool.h>

#define MAX_WATCHDOG_TASK 10            //最大监控任务数
#define WATCHDOG_TIMEOUT_MS 1000        //看门狗超时时间（ms）
#define WATCHDOG_CHECK_PERIOD_MSQ 100   //定时器检查周期


//看门狗任务信息
typedef struct{
    TaskHandle_t task_handle;   //任务句柄
    uint32_t last_feed_time;    //上次喂狗时间
    bool is_active;             //任务是否活跃
}WatchdogTask_t;


//全局变量
static WatchdogTask_t watchdog_tasks[MAX_WATCHDOG_TASK];
static uint32_t task_count=0;
static TimerHandle_t watchdog_timer=NULL;


// 喂狗函数
void Watchdog_Feed(void){
    TaskHandle_t current_task=xTaskGetCurrentTaskHandle();
    for(int32_t i=0;i<task_count;i++){
        if(watchdog_tasks[i].task_handle==current_task&&watchdog_tasks[i].is_active){
            watchdog_tasks[i].last_feed_time = xTaskGetTickCount();
            break;
        }
    }
}


// 注册任务到看门狗
// 将自身句柄添加到watchdog_tasks数组，记录初始喂狗时间，标记为活跃
bool Watchdog_RegisterTask(TaskHandle_t task_handle){
    if(task_count<MAX_WATCHDOG_TASK){
        watchdog_tasks[task_count].task_handle = task_handle;
        watchdog_tasks[task_count].last_feed_time = xTaskGetTickCount();
        watchdog_tasks[task_count].is_active = true;
        task_count++;
        return true;
    }
    return false;
}


//定时器回调函数：检查任务状态
void Watchdog_TimerCallback(TimerHandle_t xTimer){
    uint32_t current_time=xTaskGetTickCount();

    for(uint32_t i=0;i<task_count;i++){
        if(watchdog_tasks[i].is_active && 
            (current_time-wastchdog_task[i].last_feed_time)>pdMS_TO_TICKS(WATCHDOG_CHECK_PERIOD_MSQ)){
            //任务超时，触发恢复动作
            printf("Task %s timeout detected!\n",
                pcTaskGetName(watchdog_task[i].task_handle));

            //示例：标记任务为非活跃（实际可重启任务或系统复位）
            watchdog_tasks[i].is_active=false;
        }
    }
}


//初始化看门狗
void Watchdog_Init(void){
    //清空任务列表
    for(uint32_t i=0;i<MAX_WATCHDOG_TASK;i++){
        watchdog_tasks[i].is_active=false;
    }

    //创建循环定时器，每100ms检查一次
    watchdog_timer=xTimerCreate(
        "WatchdogTimer",
        pdMS_TO_TICKS(MAX_WATCHDOG_TASK),
        pdTRUE,
        NULL,
        Watchdog_TimerCallback
    );
    if(watchdog_timer!=NULL){
        xTimerStart(watchdog_timer,0);
    }else{
        printf("Failed to create watchdog timer!\n");
        while (1);
    }
}


void Example_Task(void *pvParameters){
    // 注册任务
    Watchdog_RegisterTask(xTaskGetCurrentTaskHandle());

    while (1) {
        // 模拟正常任务
        Watchdog_Feed();
        printf("Task %s running...\n", pcTaskGetName(NULL));
        vTaskDelay(pdMS_TO_TICKS(500)); // 每500ms喂狗
        // 模拟异常：注释掉Watchdog_Feed()可触发超时
    }
}


int main(void){
    Watchdog_Init();

    // 创建示例任务
    xTaskCreate(Example_Task, "ExampleTask", 256, NULL, tskIDLE_PRIORITY + 1, NULL);
    
    // 启动FreeRTOS调度器
    vTaskStartScheduler();

    while(1);
}



