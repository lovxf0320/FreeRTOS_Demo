// Demo 3: 任务优先级和调度演示

// 重点：展示不同优先级任务的调度行为
// 特色：4个不同优先级的任务，观察抢占式调度效果
// 学习要点：优先级设置、任务抢占、调度策略理解

#include "FreeRTOS.h"
#include "task.h"
#include <string.h>


//任务句柄
TaskHandle_t HightPriority_Handle;
TaskHandle_t MediumPriority_Handle;
TaskHandle_t LowPriority_Handle;
TaskHandle_t IdleMonitor_Handle;


//任务堆栈大小
#define HIGH_PRIORITY_STACK_SIZE 256
#define MEDIUM_PRIORITY_STACK_SIZE 256
#define LOW_PRIORITY_STACK_SIZE 256
#define IDLE_MONITOR_STACK_SIZE 256


//任务堆栈
StackType_t HighPriorityStack[HIGH_PRIORITY_STACK_SIZE];
StackType_t MediumPriorityStack[MEDIUM_PRIORITY_STACK_SIZE];
StackType_t LowPriorityStack[LOW_PRIORITY_STACK_SIZE];
StackType_t IdleMonitorStack[IDLE_MONITOR_STACK_SIZE];


//任务控制块
StaticTask_t HighPriorityTCB;
StaticTask_t MediumPriorityTCB;
StaticTask_t LowPriorityTCB;
StaticTask_t IdleMonitorTCB;


//任务运行计数器
volatile uint32_thigh_priority_counter=0;
volatile uint32_tmedium_priority_counter=0;
volatile uint32_tlow_priority_counter=0;
volatile uint32_tidle_monitor_counter=0;


//任务状态标志
volatile uint8_t high_priority_running = 0;
volatile uint8_t medium_priority_running = 0;
volatile uint8_t low_priority_running = 0;


//延时函数
void delay_ms(uint32_t ms){
    // for(uint32_t i=0;i<ms*1000;i++);
    vTaskDelay(pdMS_TO_TICKS(ms));
}


// 高优先级任务 - 短时间运行，频繁执行
void HighPriority_Task(void* pvParameters){
    for(;;){
        high_priority_running=1;
        high_priority_counter++;

        // 模拟紧急任务处理
        delay(50);  // 短时间运行

        high_priority_running=0;

        // 较长时间的延时，让其他任务有机会运行
        delay_ms(400);

        taskYIELD();
    }
}

// 中优先级任务 - 中等时间运行
void MediumPriority_Task(void* pvParameters){
    for(;;){
        medium_priority_running=1;
        medium_priority_counter++;

        // 模拟一般任务处理
        delay(200); // 中等时间运行

        medium_priority_running=0;

        // 中等时间的延时
        delay_ms(300);

        taskYIELD();
    }
}

// 低优先级任务 - 长时间运行，处理后台任务
void LowPriority_Task(void* pvParameters){
    for(;;){
        low_priority_running=1;
        low_priority_counter++;

        // 模拟后台任务处理（如数据整理、垃圾回收等）
        delay(800); // 长时间运行

        low_priority_running=0;

        // 短时间延时，但由于优先级低，经常被高优先级任务抢占
        delay_ms(100);

        taskYIELD();
    }
}

void IdleMonitor_Task(void* pvParameters){
    for(;;){
        idle_monitor_counter++;

        // 计算各任务的运行比例（简化版）
        uint32_t total_count=high_priority_counter+medium_priority_counter+low_priority_counter;
    
        // 在实际应用中，这些信息可以通过调试接口或显示器输出
        volatile uin32_t high_percentage=total_count>0?(high_priority_counter*100)/total_count:0;
        volatile uin32_t medium_percentage=total_count>0?(medium_priority_counter*100)/total_count:0;
        volatile uin32_t low_percentage=total_count>0?(low_priority_counter*100)/total_count:0;
    
        // 防止编译器优化
        // 这些语句告诉编译器这些变量是"被使用"的
        // 防止编译器因为变量没有被后续代码使用而优化掉计算过程
        // 在调试时，可以通过调试器查看这些变量的值
        (void)high_percentage;
        (void)medium_percentage;
        (void)low_percentage;
        
        delay_ms(2000);
        taskYIELD();
    }   
}

int main(){

    // 创建高优先级任务
    HightPriority_Handle = xTaskCreateStatic(
        HighPriority_Task,              // 任务函数
        "HighPriority",                 // 任务名称
        HIGH_PRIORITY_STACK_SIZE,       // 任务堆栈大小
        NULL,                           // 任务参数← 这里就是 pvParameters
        1,                              // 优先级
        HighPriorityStack,              // 任务堆栈
        &HighPriorityTCB                // 任务控制块
    );
    
    // 创建中优先级任务
    MediumPriority_Handle = xTaskCreateStatic(
        MediumPriority_Task,            // 任务函数
        "MediumPriority",               // 任务名称
        MEDIUM_PRIORITY_STACK_SIZE,     // 任务堆栈大小
        NULL,                           // 任务参数
        2,                              // 优先级
        MediumPriorityStack,            // 任务堆栈
        &MediumPriorityTCB              // 任务控制块
    );
    
    // 创建低优先级任务
    LowPriority_Handle = xTaskCreateStatic(
        LowPriority_Task,               // 任务函数
        "LowPriority",                  // 任务名称
        LOW_PRIORITY_STACK_SIZE,        // 任务堆栈大小
        NULL,                           // 任务参数
        3,                              // 优先级
        LowPriorityStack,               // 任务堆栈
        &LowPriorityTCB                 // 任务控制块
    );
    
    // 创建空闲监控任务
    IdleMonitor_Handle = xTaskCreateStatic(
        IdleMonitor_Task,               // 任务函数
        "IdleMonitor",                  // 任务名称
        IDLE_MONITOR_STACK_SIZE,        // 任务堆栈大小
        NULL,                           // 任务参数
        4,                              // 优先级
        IdleMonitorStack,               // 任务堆栈
        &IdleMonitorTCB                 // 任务控制块
    );

    // 启动调度器
    vTaskStartScheduler();

    for(;;);

    return 0;
}