/*
 * Demo1: FreeRTOS 空闲任务基础概念演示
 * 
 * 学习目标：
 * 1. 理解空闲任务的作用
 * 2. 理解阻塞延时和软件延时的区别
 * 3. 观察任务切换过程
 */
#include "FreeRTOS.h"
#include "task.h"

//全局标志，用于观察任务运行状态
volatile int task1_counter = 0;
volatile int task2_counter = 0;
volatile int idle_counter = 0;

// 任务句柄
TaskHandle_t Task1_Handle;
TaskHandle_t Task2_Handle;

//任务1：每2秒运行一次
void Task1_Function(void *pvParameters){
    while(1){
        task1_counter++;
        printf("Task1 running, counter: %d\n",task1_counter);

        //阻塞延时2秒，期间CPU可以运行其他任务
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}


//任务2：每3秒运行一次
void Task2_Function(void *pvParameters){
    while(1){
        task2_counter++;
        printf("Task2 running, counter: %d\n", task2_counter);
        
        // 阻塞延时3秒，期间CPU可以运行其他任务
        vTaskDelay(pdMS_TO_TICKS(3000));
    }
}


// 空闲任务钩子函数（当系统进入空闲任务时会调用）
void vApplicationIdleHook(void){
    idle_counter++;
    // 注意：空闲任务中不能使用会阻塞的函数！
    // printf("Idle task running...\n");  // 不要在这里用printf
    
    // 在实际应用中，这里可以：
    // 1. 让CPU进入低功耗模式
    // 2. 执行一些后台清理工作
    // 3. 喂看门狗
}


int main(void){
    printf("=== FreeRTOS 空闲任务概念演示 ===\n");
    printf("观察：当Task1和Task2都在延时时，系统会运行空闲任务\n\n");

    //创建任务1
    xTaskCreate(
        Task1_Function,
        "Task1",
        1000,
        2,
        &Task1_Handle
    );

    //创建任务2
    xTaskCreate(
        Task2_Funtion,
        "Task2",
        1000,
        1,
        &Task2_Handle
    );

    //启动调度器
    printf("启动调度器...\n");
    vTaskStartScheduler();

    //不应该运行到这里
    printf("调度器启动失败！\n");
    while(1);
}

/*
 * 预期现象：
 * 1. Task1每2秒打印一次
 * 2. Task2每3秒打印一次  
 * 3. 当两个任务都在延时时，idle_counter会快速增长
 * 4. 高优先级的Task1会抢占低优先级的Task2
 */