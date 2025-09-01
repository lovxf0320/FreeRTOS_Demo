/*
 * Demo 2: 优先级位图表机制演示
 * 演示FreeRTOS内部如何使用位图快速查找最高优先级
 * 
 * 学习要点
 * 位图表原理：每个位对应一个优先级
 * CLZ指令应用：快速计算最高优先级
 * 位操作：置位和清位操作
 * 性能优化：O(1)时间复杂度查找最高优先级
 */
#include "FreeRTOS.h"
#include "task.h"

//模拟FreeRTOS内部的优先级位图表
static volatile UBaseType_t demo_ready_priorities=0;

//任务优先级定义（使用不连续的优先级来演示位图）
#define TASK_A_PRIORITY     7       // 位7
#define TASK_B_PRIORITY     3       // 位3
#define TASK_C_PRIORITY     1       // 位1
#define TASK_D_PRIORITY     0       // 位0

//任务状态
volatile char task_a_state='S';     //S=Suspended, R=Ready, B=Blocked
volatile char task_b_state='S';     
volatile char task_c_state='S';     
volatile char task_d_state='R';     //空闲任务始终就绪

//任务句柄
TaskHandle_t task_handles[4];


//模拟taskRECORD_READY_PRIORITY宏 - 标记任务就绪
#define DEMO_RECORD_READY_PRIORITY(priority)    \
    do{
        demo_ready_priorities |= (1UL<<(priority));
    }while(0)

//模拟taskRESET_READY_PRIORITY宏 - 清除任务就绪状态
#define taskRESET_READY_PRIORITY(priority)  \
    do{
        demo_ready_priorities &= ~(1UL << (priority));  \
    }while(0)

//模拟portGET_HIGHEST_PRIORITY宏（使用CLZ指令） - 快速查找最高优先级
#define portGET_HIGHEST_PRIORITY(priority)  \
    do{
        (top_priority) = (31UL - (uint32_t)__builtin_clz((ready_priorities))); \
    }while(0)


// 演示任务A - 周期性运行
void task_a(void *pvParameters){
    for(;;)
    {
        task_a_state = 'R';
        
        // 执行任务逻辑
        for(volatile int i = 0; i < 10000; i++);
        
        task_a_state = 'B';
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

// 演示任务B
void task_b(void *pvParameters) {
    for(;;)
    {
        task_b_state = 'R';
        
        // 执行任务逻辑
        for(volatile int i = 0; i < 8000; i++);
        
        task_b_state = 'B';
        vTaskDelay(pdMS_TO_TICKS(300));
    }
}

// 演示任务C
void task_c(void *pvParameters){
    for(;;)
    {
        task_c_state = 'R';
        
        // 执行任务逻辑  
        for(volatile int i = 0; i < 6000; i++);
        
        task_c_state = 'B';
        vTaskDelay(pdMS_TO_TICKS(200));
    }
}

void priority_monitor_task(void *pvParameters){
    UBaseType_t highest_priority;

    for(;;){
        //模拟系统就绪状态更新
        demo_ready_priorities=0;

        //根据任务状态更新位图
        if(task_a_stste=='R'){
            DEMO_RECORD_READY_PRIORITY(TASK_A_PRIORITY);
        }

        if(task_a_stste=='R'){
            DEMO_RECORD_READY_PRIORITY(TASK_B_PRIORITY);
        }

        if(task_a_stste=='R'){
            DEMO_RECORD_READY_PRIORITY(TASK_C_PRIORITY);
        }

        DEMO_RECORD_READY_PRIORITY(TASK_D_PRIORITY);    //空闲任务始终就绪


        //查找最高优先级
        if(demo_ready_priorities!=0){
             DEMO_GET_HIGHEST_PRIORITY(highest_priority, demo_ready_priorities);
            
            // 在调试器中观察这些值
            // demo_ready_priorities的二进制表示
            // highest_priority的值
        }

        vTaskDelay(pdMS_TO_TICKS(50));
    }
}


int main(void){
    xTaskCreate(
        priority_monitor_task,
        "Monitor",
        128,
        NULL,
        8,
        &task_handles[0]
    );

    xTaskCreate(
        task_a,
        "TaskA",
        128,
        NULL,
        TASK_A_PRIORITY,
        &task_handles[1]
    );

    xTaskCreate(
        task_b,
        "TaskB",
        128,
        NULL,
        TASK_B_PRIORITY,
        &task_handles[2]
    );

    xTaskCreate(
        task_c,
        "TaskC",
        128,
        NULL,
        TASK_C_PRIORITY,
        &task_handles[3]
    );

    vTaskStartScheduler();

    for(;;);
}