// 定时器回调 → 模拟外部设备（如另一个MCU、传感器等）通过UART发送数据
// 中断函数 UART_ReceiveISR → 模拟本设备的UART接收中断，当硬件收到数据时触发
// 处理任务 vUartProcessTask → 模拟应用程序处理接收到的数据
#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include <stdio.h>
#include <stdint.h>


#define BUFFER_SIZE 64
#define UART_TASK_PRIORITY    (taskIDLE_PRIORITY + 2)
#define PROCESS_TASK_PRIORITY (taskIDLE_PRIORITY + 1)


typedef struct{
    uint8_t buffer[BUFFER_SIZE];        //接收缓冲区
    volatile int write_pos;             //写入位置（中断使用）
    volatile int read_pos;              //读取位置（任务使用）
    volatile int data_count;            //当前数据量
}UartBuffer_t;


UartBuffer_t g_uart_buffer={0};


TaskHandle_t xProcessTaskHandle = NULL;
TimerHandle_t xUartSimTimer = NULL;


//模拟本设备的UART接收中断，当硬件收到数据时触发
void UART_ReceiveISR(uint8_t received_byte){
    uint32_t interrupt_status;

    interrupt_status=taskENTER_CRITICAL_FROM_ISR();
    {
        //检查缓冲区是否有空间
        if(g_uart_buffer.data_count<BUFFER_SIZE){
            //存入缓冲区
            g_uart_buffer.buffer[g_uart_buffer.write_pos]=received_byte;
            g_uart_buffer.write_pos=(g_uart_buffer.write_pos+1)%BUFFER_SIZE;
            g_uart_buffer.data_count++;

            printf("ISR收到: 0x%02X ('%c'), 缓冲区: %d字节\n", 
                   received_byte, 
                   (received_byte >= 32 && received_byte <= 126) ? received_byte : '?',
                   g_uart_buffer.data_count);
        }else{
            printf("缓冲区满，数据丢失！\n");
        }
    }

    //退出中断临界段
    taskEXIT_CRITICAL_FROM_ISR();

    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    //通知处理任务有新数据（从中断安全版本）
    vTaskNotifyGiveFromISR(xProcessTaskHandle, &xHigherPriorityTaskWoken);

    //如果有更高优先级任务被唤醒，执行任务切换
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}


int uart_read_byte(uint8_t *data){
    int result=0;

    taskENTER_CRITICAL();
    {
        if(g_uart_buffer.data_count>0){
            *data=g_uart_buffer.buffer[g_uart_buffer.read_pos];
            g_uart_buffer.read_pos = (g_uart_buffer.read_pos + 1) % BUFFER_SIZE;
            g_uart_buffer.data_count--;
            result = 1;  // 成功
        }
    }
    taskEXIT_CRITICAL();

    return result;
}


//模拟应用程序处理接收到的数据
void vUartProcessTask(void *pvParameters){
    uint8_t received_byte;

    printf("UART处理任务启动\n");

    while(1){
        //等待中断通知（有新数据时被唤醒）
        ulTaskNotifyTake(pdTrue,portMAX_DELAY);

        printf("处理接收数据:");

        //读取并处理所有可用数据
        while(uart_read_byte(&received_byte)){
            printf("0x%02X('%c')",
                received_byte,
                (received_byte >= 32 && received_byte <= 126) ? received_byte : '?');
        }

        printf("\n");
    }
}


//模拟外部设备（如另一个MCU、传感器等）通过UART发送数据
void vUartSimulatorCallback(TimerHandle_t xTimer){
    //模拟接收的测试数据
    TickType_t xListWaskTime=xTaskGetTickCount();

    static char test_message[]="Hello FreeRTOS UART!";
    static int msg_index=0;

    if(test_message[msg_index]!='\0'){
        //模拟中断接收一个字节
        UART_ReceiveISR(test_message[msg_index]);
        msg_index++;
    }else{
        //消息发送完毕，重新开始
        msg_index=0;
        //暂停一下再发送下一轮
        vTaskDelayUntil(&xListWakeTime,pdMS_TO_TICKS(100));
    }
}


void create_uart_demo_tasks(void){
    BaseType_t xResult;

    xResult=xTaskCreate(
        vUartProcessTask,          // 任务函数
        "UartProcess",             // 任务名
        configMINIMAL_STACK_SIZE,  // 栈大小
        NULL,                      // 参数
        PROCESS_TASK_PRIORITY,     // 优先级
        &xProcessTaskHandle        // 任务句柄
    );
    if(xResult!=paPASS){
        printf("❌ UART处理任务创建失败\n");
        return;
    }

    xUartSimTimer=xTimerCreate(
        "UartSim",
        pdMA_TO_TICKS(200),
        pdTRUE,
        NULL,
        vUartSimulatorCallback
    );
    if(xUartSimTimer!=NULL){
        xTimerStart(xUartSimTimer,0);
        printf("UART模拟器启动成功\n");
    }else{
        printf("UART模拟器未启动成功\n");
    }

}


int main(void){
    printf("=== FreeRTOS简化版UART接收Demo ===\n\n");
    printf("核心功能:\n");
    printf("模拟UART中断接收\n");
    printf("临界段保护共享缓冲区\n");
    printf("任务通知机制\n");
    printf("任务间数据传递\n\n");

    //创建所有任务和定时器
    create_uart_demo_tasks();

    vTaskStartScheduler();

    while(1);

    return 0;
}


/*
=== 关键FreeRTOS概念说明 ===

1. 临界段保护:
   - taskENTER_CRITICAL_FROM_ISR() : 中断中使用
   - taskENTER_CRITICAL()          : 任务中使用

2. 任务通知:
   - vTaskNotifyGiveFromISR()      : 中断中通知任务
   - ulTaskNotifyTake()            : 任务中等待通知

3. 任务调度:
   - portYIELD_FROM_ISR()          : 中断中触发任务切换
   - vTaskDelay()                  : 任务延时

4. 软件定时器:
   - xTimerCreate()                : 创建定时器
   - xTimerStart()                 : 启动定时器

预期输出示例:
=== FreeRTOS简化版UART接收Demo ===
UART处理任务启动
UART模拟器启动成功

ISR收到: 0x48 ('H'), 缓冲区: 1字节
ISR收到: 0x65 ('e'), 缓冲区: 2字节  
ISR收到: 0x6C ('l'), 缓冲区: 3字节
处理接收数据: 0x48('H') 0x65('e') 0x6C('l') 
ISR收到: 0x6C ('l'), 缓冲区: 1字节
ISR收到: 0x6F ('o'), 缓冲区: 2字节
处理接收数据: 0x6C('l') 0x6F('o') 
...
*/