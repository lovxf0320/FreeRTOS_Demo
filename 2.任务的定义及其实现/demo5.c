/*
 * Demo5: 基础任务管理和优先级
 * 学习要点：
 * 1. 正确使用vTaskDelay()而不是空循环延时
 * 2. 理解任务优先级的作用
 * 3. 任务状态监控
 * 4. 动态创建vs静态创建
 */

#include "FreeRTOS.h"
#include "task.h"
#include <stdio.h>

//任务句柄
TaskHandle_t high_priority_task_handle;
TaskHandle_t medium_priority_task_handle;
TaskHandle_t low_priority_task_handle;
TaskHandle_t monitor_task_handle;

//全局状态变量
volatile uint32_t high_task_counter=0;
volatile uint32_t medium_task_counter=0;
volatile uint32_t low_task_counter=0;

// xTaskGetTickCount 用于获取从操作系统启动以来的系统时钟滴答数（即 Tick 数）。
void high_priority_task(void* pvParameters){
    TickType_t last_wake_time=xTaskGetTickCount();

    for(;;){
        high_task_counter++;

        printf("[HIGH] 高优先级任务运行 #%lu, Tick=%lu\n", 
               high_task_counter, last_wake_time);

        // 精确周期性延时 - 每500ms执行一次
        vTaskDelayUntil(&last_wake_time,pdMS_TO——TICK(500));
    }
}

void medium_priority_task(void* pvParameters){
    for(;;){
        medium_task_counter++;

        printf("[MEDIUM] 中等优先级任务运行 #%lu\n", medium_task_counter);

        // 模拟一些工作 - 占用CPU 100ms
        TickType_t start_time=xTaskGetTickCount();
        while((xTaskGetTickCount()-start_time)<pdMS_TO_TICKS(100)){
            // 模拟CPU密集型工作
        }

        printf("[MEDIUM] 中等优先级任务完成工作\n");
        
        // 普通延时
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void low_priority_task(void* pvParameters){
    for(;;){
        low_task_counter++;

        printf("[LOW] 低优先级任务运行 #%lu (只在其他任务空闲时运行)\n", 
               low_task_counter);

        //长时间延时，让其他任务有机会
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

// 监控任务 - 优先级3，监控系统状态
void monitor_task(void* pvParameters){
    //任务堆栈的大小、任务优先级、计数器等值，或者是 FreeRTOS 内部的一些关键量。
    UBaseType_t high_water_mark;

    for(;;){
        printf("\n====系统监控报告====");
        printf("运行时间: %lu ticks\n",xTaskGetTickCount());
        printf("高优先级任务执行次数：%lu\n",high_task_counter);
        printf("中优先级任务执行次数：%lu\n",medium_task_counter);
        printf("低优先级任务执行次数：%lu\n",low_task_counter);

        //检查各任务的堆栈的使用情况
        if(high_priority_task_handle!=NULL){
            // 这个函数返回指定任务的堆栈剩余空间，以“字”（word）为单位。
            high_water_mark = uxTaskGetStackHighWaterMark(high_priority_task_handle);
            printf("高优先级任务剩余堆栈: %u words\n", high_water_mark);
        }

        //获取系统信息
        printf("空闲任务剩余堆栈: %u words\n", 
               uxTaskGetStackHighWaterMark(NULL));
                // 用来检查系统空闲任务的堆栈剩余空间
        
        printf("当前任务数量: %u\n", uxTaskGetNumberOfTasks());
        printf("==================\n\n");
        
        vTaskDelay(pdMS_TO_TICKS(3000));
    }
}

void demonstrate_task_suspend_resume(void){
    printf("演示人物的挂起和恢复...\n");

    //挂起低优先级
    if(low_priority_task_handle!=NULL){
        vTaskSuspend(low_priority_task_handle);
        printf("低优先级任务以挂起");
    }

    vTaskDelay(pdMS_TO_TICKS(5000));

    //恢复高优先级
    if(low_priority_task_handle!=NULL){
        vTaskResume(low_priority_task_handle);
        printf("低优先级任务以恢复");
    }
}

int main(){
    //少tcb和堆栈数组
    //创建高优先级的任务
    BaseType_t result=xTaskCreate(
        high_priority_task,         // 任务函数
        "HighPriTask",              // 任务名称
        256,                        // 堆栈大小
        NULL,                       // 参数
        4,                          // 优先级
        &high_priority_task_handle  // 任务句柄
    );
    if(result!=pdPASS){
        printf("错误：高优先级任务创建失败!\n");
        return -1;
    }

    result=xTaskCreate(
        medium_priority_task,           // 任务函数
        "MediumPriTask",                // 任务名称
        256,                            // 堆栈大小
        NULL,                           // 参数
        2,                              // 优先级
        &medium_priority_task_handle    // 任务句柄
    );
    if(result!=pdPASS){
        printf("错误：中优先级任务创建失败!\n");
        return -1;
    }

    result=xTaskCreate(
        low_priority_task,          // 任务函数
        "LowPriTask",               // 任务名称
        256,                        // 堆栈大小
        NULL,                       // 参数
        1,                          // 优先级
        &low_priority_task_handle   // 任务句柄
    );
    if(result!=pdPASS){
        printf("错误：低优先级任务创建失败!\n");
        return -1;
    }

    result=xTaskCreate(
        monitor_task,               // 任务函数
        "MonitorTask",              // 任务名称
        512,                        // 堆栈大小
        NULL,                       // 参数
        3,                          // 优先级
        &monitor_task_handle        // 任务句柄
    );
    if(result!=pdPASS){
        printf("错误：低优先级任务创建失败!\n");
        return -1;
    }

    vTaskStartScheduler();

    // 如果调度器启动失败才会执行到这里
    printf("错误：调度器启动失败!\n");
    for(;;);

    return 0;
}

//FreeRTOS钩子函数
void vApplicationStackOverflowHook(TaskHandle_t xTask,char *pcTaskName){
    printf("堆栈溢出检测: 任务 %s\n", pcTaskName);
    for(;;); // 停止执行
}

void vApplicationMallocFailedHook(void){
    printf("内存分配失败!\n");
    for(;;); // 停止执行
}

// 空闲任务钩子 - 在空闲时执行
void vApplicationIdleHook(void){
    // 可以在这里执行低功耗操作
    // 注意：这个函数不能阻塞或调用会阻塞的API
    static uint32_t idle_counter=0;
    idle_counter++;

    // 每100000次循环输出一次（避免输出太频繁）
    if(idle_counter % 100000 == 0) {
        // printf("Idle hook执行 #%lu\n", idle_counter/100000);
    }
}

/*
学习要点总结：

1. vTaskDelay() vs vTaskDelayUntil():
   - vTaskDelay(): 相对延时，从调用时开始计算
   - vTaskDelayUntil(): 绝对延时，用于精确的周期性任务

2. 优先级规则：
   - 数字越大优先级越高
   - 高优先级任务会抢占低优先级任务
   - 相同优先级任务会轮转执行

3. 堆栈监控：
   - uxTaskGetStackHighWaterMark()：查看任务堆栈最大使用量
   - 用于调试和优化堆栈大小

4. 任务状态：
   - Running: 正在运行
   - Ready: 就绪等待
   - Blocked: 阻塞等待
   - Suspended: 挂起

5. 错误处理：
   - 检查任务创建返回值
   - 使用钩子函数处理错误情况
*/