/*
 * 练习1：基础时间片实验
 * 目标：理解同优先级任务的时间片轮转机制
 */

#include "FreeRTOS.h"
#include "task.h"

/* 全局标志变量，用于逻辑分析仪观察 */
volatile uint8_t task1_flag = 0;
volatile uint8_t task2_flag = 0;
volatile uint8_t task3_flag = 0;

/* 任务句柄 */
TaskHandle_t xTask1Handle;
TaskHandle_t xTask2Handle; 
TaskHandle_t xTask3Handle;

/* 任务栈大小 */
#define TASK_STACK_SIZE 128

/* 任务控制块和栈内存（静态分配）*/
StackType_t xTask1Stack[TASK_STACK_SIZE];
StackType_t xTask2Stack[TASK_STACK_SIZE];
StackType_t xTask3Stack[TASK_STACK_SIZE];

TCB_t xTask1TCB;
TCB_t xTask2TCB;
TCB_t xTask3TCB;

/* 软件延时函数 */
void vSoftwareDelay(uint32_t count){
    volatile uint32_t i;
    for(i = 0; i < count; i++)
    {
        __NOP(); // 空操作，防止编译器优化
    }
}

void vTask1(void *pvParameters){
    for(;;){
        task1_flag=1;
        vSoftwareDelay(500);
        task1_flag=0;
        vTaskDelay(pdMS_TO_TICKS(5));
    }
}

void vTask2(void *pvParameters){
    for(;;){
        task2_flag=1;
        vSoftwareDelay(500);
        task2_flag=0;
        vTaskDelay(pdMS_TO_TICKS(5));
    }
}

void vTask3(void *pvParameters){
    for(;;){
        task3_flag=1;
        vTaskDelay(pdMS_TO_TICKS(10));
        task3_flag=0;
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}


int main(void){
    xTask1Handle=xTaskCreateStatic(
        vTask1,
        "Task1",
        TASK_STACK_SIZE,
        NULL,
        2,
        xTask1Stack,
        &xTask1TCB
    );

    xTask2Handle=xTaskCreateStatic(
        vTask2,
        "Task2",
        TASK_STACK_SIZE,
        NULL,
        2,
        xTask2Stack,
        &xTask2TCB
    );

    xTask3Handle=xTaskCreateStatic(
        vTask3,
        "Task3",
        TASK_STACK_SIZE,
        NULL,
        3,
        xTask3Stack,
        &xTask3TCB
    );

    vTaskStartScheuler();

    for(;;);
}


/* 空闲任务内存分配（FreeRTOS要求提供）*/
StackType_t xIdleTaskStack[TASK_STACK_SIZE];
TCB_t IdleTaskTCB;

void vApplicationGetIdleTaskMemory(TCB_T **ppxIdleTaskTCBBuffer,
                                    StackType_t **ppxIdleTaskStackBuffer,
                                    uint32_t *pulIdleTaskStackSize){
    
    *ppxIdleTaskTCBBuffer=&IdleTaskTCB;
    *ppxIdleTaskStackBuffer=xIdleTaskStack;
    *pulIdleTaskStackSize=TASK_STACK_SIZE;
}

 /*
 * 练习要求：
 * 1. 使用逻辑分析仪或示波器观察task1_flag、task2_flag、task3_flag的波形
 * 2. 验证任务1和任务2是否在1个tick时间内轮流执行
 * 3. 观察任务3的执行周期是否为2个tick
 * 4. 分析为什么任务1和任务2能够轮流执行
 */