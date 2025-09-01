/*
 * Demo 1: 基础优先级概念演示
 * 演示3个不同优先级的任务如何被调度
 */
#include "FreeRTOS.h"
#include "task.h"

//全局标志变量，用于观察任务执行
volatile uint32_t task_high_counter=0;        //高优先级任务计数器
volatile uint32_t task_medium_counter=0;      //中优先级任务计数器
volatile uint32_t task_low_counter=0;         //低优先级任务计数器


//任务优先级定义
#define TASK_HIGH_PRIORITY      3
#define TASK_MEDIUM_PRIORITY    2
#define TASK_LOW_PRIORITY       1


//任务栈大小
#define TASK_STACK_SIZE         128

TaskHandle_t high_task_handle;
TaskHandle_t medium_task_handle;
TaskHandle_t low_task_handle;


void high_priority_task(void *pvParameters){
    while(1){
        task_high_counter++;

        //模拟快速处理
        for(volatile int i = 0; i < 1000; i++);

        vTaskDelay(pdMS_TO_TICKS(500));
    }
}


void high_priority_task(void *pvParameters){
    while(1){
        task_medium_counter++;

        //模拟快速处理
        for(volatile int i = 0; i < 1000; i++);

        vTaskDelay(pdMS_TO_TICKS(500));
    }
}


void high_priority_task(void *pvParameters){
    while(1){
        task_low_counter++;

        //模拟快速处理
        for(volatile int i = 0; i < 1000; i++);

        vTaskDelay(pdMS_TO_TICKS(500));
    }
}


int main(void){
    //创建高优先级任务
    xTaskCreate(
        high_priority_task,
        "high_priority_task",
        TASK_STACK_SIZE,
        NULL,
        TASK_HIGH_PRIORITY,
        &high_task_handle
    );

    //创建中优先级任务
    xTaskCreate(
        medium_priority_task,
        "medium_priority_task",
        TASK_STACK_SIZE,
        NULL,
        TASK_MEDIUM_PRIORITY,
        &medium_task_handle
    );

    //创建低优先级任务
    xTaskCreate(
        low_priority_task,
        "low_priority_task",
        TASK_STACK_SIZE,
        NULL,
        TASK_LOW_PRIORITY,
        &low_task_handle
    );

    vTaskStartScheduler();

    for(;;);
}