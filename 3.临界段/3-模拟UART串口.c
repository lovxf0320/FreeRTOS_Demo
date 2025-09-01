/*
 * Demo 3: UART中断接收缓冲 - 中断中的临界段使用
 * 
 * 功能描述：
 * - 模拟UART串口数据接收和处理
 * - 中断服务程序中使用临界段保护接收缓冲区
 * - 任务中安全地从缓冲区读取数据进行处理
 * - 演示中断和任务之间的数据同步
 * 
 * 学习要点：
 * - 中断中临界段的正确使用方法
 * - taskENTER_CRITICAL_FROM_ISR()的应用
 * - 中断与任务间的数据共享保护
 * - 缓冲区溢出处理机制
 */
#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

//任务优先级定义
#define UART_PROCESS_TASK_PRIORITY  (taskDLE_PRIORITY+2)
#define DATA_ANALYZE_TASK_PRIORITY  (taskDLE_PRIORITY+2)
#define MONITOR_TASK_PRIORITY       (taskDLE_PRIORITY+2)

//任务栈大小
#define TASK_STACK_SIZE             (configMINIMAL_STACK_SIZE*2)

//UART接收缓冲区配置
#define UART_RX_BUFFER_SIZE         128     //UART缓冲区大小
#define UART_FRAME_MAX_SIZE         32      //最大帧大小

//数据帧类型定义
typedef enum{
    FRAME_TYPE_COMMAND = 0X01,
    FRAME_TYPE_DATA    = 0x02,
    FRAME_TYPE_STATUS  = 0x03,
    FRAME_TYPE_ERROR   = 0x04
}FrameType_t;

//数据帧结构
typedef struct {
    uint8_t frame_header;                   // 帧头 (0xAA)
    uint8_t frame_type;                     // 帧类型
    uint8_t frame_length;                   // 数据长度
    uint8_t frame_data[UART_FRAME_MAX_SIZE];// 数据内容
    uint8_t frame_checksum;                 // 校验和
    uint32_t receive_timestamp;             // 接收时间戳
} DataFrame_t;

//UART接收缓冲区结构
typedef struct {
    uint8_t buffer[UART_RX_BUFFER_SIZE];    // 原始数据缓冲区
    volatile uint16_t write_index;          // 写入索引（中断使用）
    volatile uint16_t read_index;           // 读取索引（任务使用）
    volatile uint16_t data_count;           // 当前数据量
    
    /* 统计信息 */
    volatile uint32_t total_received;       // 总接收字节数
    volatile uint32_t overflow_count;       // 溢出次数
    volatile uint32_t frame_received;       // 接收帧数
    volatile uint32_t frame_errors;         // 帧错误数
    volatile uint32_t last_activity_time;   // 最后活动时间
} UartRxBuffer_t;

//全局UART接收缓冲区
UartRxBuffer_t g_uart_rx_buffer = {0};

//任务句柄
TaskHandle_t xUartProcessTaskHandle = NULL;
TaskHandle_t xDataAnalyzeTaskHandle = NULL;
TaskHandle_t xMonitorTaskHandle = NULL;

//定时器句柄-模拟UART数据接收
TimerHandle_t xUartSimulatorTimer = NULL;


/**
 * @brief 获取UART缓冲区统计信息快照
 * @param stats_snapshot 统计信息存储位置
 */
void get_uart_stats_snapshot(UartRxBuffer_t *stats_snapshot){
    taskENTER_CRITICAL();
    {
        stats_snapshot->data_count          =g_uart_rx_buffer.data_count;
        stats_snapshot->total_received      =g_uart_rx_buffer.total_received;
        stats_snapshot->overflow_count      =g_uart_rx_buffer.overflow_count;
        stats_snapshot->frame_received      =g_uart_rx_buffer.frame_received;
        stats_snapshot->frame_errors        =g_uart_rx_buffer.frame_errors;
        stats_snapshot->last_activity_time  =g_uart_rx_buffer.last_activity_time;
    }
    taskEXIT_CRITICAL();
}


/**
 * @brief 模拟UART接收中断处理函数
 * @param received_byte 接收到的字节
 */
//UART接收缓冲区结构
void UART_RxInterruptHandler(uint8_t received_byte){
    uint32_t interrupt_status;

    /* ========== 在中断中进入临界段（可嵌套版本）========== */
    interrupt_status=taskENTER_CRITICAL_FROM_ISR();
    {
        if(g_uart_rx_buffer.data_count<UART_RX_BUFFER_SIZE){
            //将接收到的字节存入缓冲区
            g_uart_rx_buffer.buffer[g_uart_rx_buffer.write_index] = received_byte;
            g_uart_rx_buffer.write_index = (g_uart_rx_buffer.write_index + 1) % UART_RX_BUFFER_SIZE;
            g_uart_rx_buffer.data_count++;
            
            /* 更新统计信息 */
            g_uart_rx_buffer.total_received++;
            g_uart_rx_buffer.last_activity_time = xTaskGetTickCountFromISR();
        }else{
            //缓冲区溢出
            g_uart_rx_buffer.overflow_count++;
        }
    }
    taskEXIT_CRITICAL_FROM_ISR(interrupt_status);
    /* ========== 退出中断临界段 ========== */

}


/**
 * @brief 数据分析任务
 */
void vDataAnalyzeTask(void *pvParameters){
    UartRxBuffer_t stats;
    TickType_t xLastWakeTime;
    const TickType_t xFrequency = pdMS_TO_TICKS(2000); // 2秒分析一次
    uint32_t last_frame_count = 0;
    
    xLastWakeTime = xTaskGetTickCount();
    
    printf("数据分析任务启动\n");

    while(1){
        vTaskDelayUnitil(&xLastWakeTime,xFrequency);

        get_uart_stats_snapShot(&stats);

        //计算帧接收速率
        uint32_t frame_rate=stats.frame_received-last_frame_count;
        last_frame_count=stats.frame_received;

        printf("数据分析报告:\n");
        printf("接收速率: %lu 帧/2秒\n", frame_rate);
        printf("总接收: %lu 字节, %lu 帧\n", 
               stats.total_received, stats.frame_received);

        
    }
    
}


/**
 * @brief UART数据模拟器定时器回调函数
 */
void vUartSimulatorTimerCallback(TimerHandle_t xTimer){
    static uint8_t test_data[]={
        // 命令帧
        0xAA, 0x01, 0x04, 0x10, 0x20, 0x30, 0x40, 0x85,
        // 数据帧
        0xAA, 0x02, 0x06, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x02,
        // 状态帧
        0xAA, 0x03, 0x01, 0xFF, 0xFD,
        // 错误帧（故意错误的校验和）
        0xAA, 0x04, 0x02, 0xEE, 0xFF, 0x00
    };

    static int data_index=0;
    static int send_count=0;

    //模拟逐字节接收
    if(data_index<sizeof(test_data)){
        UART_RxInterruptHandler(test_data[data_index]);
        data_index++;
    }else{
        data_index=0;
        send_count++;

        //每发送10轮后暂停一下，模拟通信间隔
        if(send_count % 10 == 0) {
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }

}


/**
 * @brief 创建UART中断demo任务
 */
void create_uart_interrupt_demo_tasks(void){
    BaseType_t xReturn;
    
    /* 创建UART处理任务 */
    xReturn = xTaskCreate(vUartProcessTask,
                         "UartProcess",
                         TASK_STACK_SIZE,
                         NULL,
                         UART_PROCESS_TASK_PRIORITY,
                         &xUartProcessTaskHandle);
    if(xReturn != pdPASS) {
        printf("UART处理任务创建失败!\n");
        return;
    }
    
    /* 创建数据分析任务 */
    xReturn = xTaskCreate(vDataAnalyzeTask,
                         "DataAnalyze",
                         TASK_STACK_SIZE,
                         NULL,
                         DATA_ANALYZE_TASK_PRIORITY,
                         &xDataAnalyzeTaskHandle);
    if(xReturn != pdPASS) {
        printf("数据分析任务创建失败!\n");
        return;
    }
    
    /* 创建监控任务 */
    xReturn = xTaskCreate(vMonitorTask,
                         "Monitor",
                         TASK_STACK_SIZE,
                         NULL,
                         MONITOR_TASK_PRIORITY,
                         &xMonitorTaskHandle);
    if(xReturn != pdPASS) {
        printf("监控任务创建失败!\n");
        return;
    }
    
    /* 创建UART数据模拟器定时器 */
    xUartSimulatorTimer = xTimerCreate("UartSim",
                                      pdMS_TO_TICKS(50), // 50ms周期
                                      pdTRUE,            // 自动重载
                                      NULL,              // 定时器ID
                                      vUartSimulatorTimerCallback);
    if(xUartSimulatorTimer != NULL) {
        // 启动定时器，0表示立即启动（不等待）
        xTimerStart(xUartSimulatorTimer, 0);
    } else {
        printf("UART模拟器定时器创建失败!\n");
        return;
    }
    
    printf("UART中断Demo启动成功!\n");
    printf("观察中断中临界段如何保护UART接收缓冲区\n\n");
}


/**
 * @brief 获取缓冲区当前数据量（安全版本）
 * @return 当前数据量
 */
uint16_t uart_get_available_data(void){
    uint16_t count;

    taskENTER_CRITICAL();
    {
        count = g_uart_rx_buffer.data_count;
    }
    taskEXIT_CRITICAL();

    return count;
}


/* ============================================================================
 * 缓冲区操作函数
 * ============================================================================ */
/**
 * @brief 从UART接收缓冲区读取单个字节（任务安全）
 * @param data 读取的数据存储位置
 * @return 0:成功读取, -1:无数据可读
 * 
 * @note 该函数在任务中调用，使用taskENTER_CRITICAL()保护
 */
int uart_read_byte(uint8_t *data){
    int result=-1;

    if(!data){
        return -1;
    }

    /* ========== 任务中进入临界段 ========== */
    // taskENTER_CRITICAL()用于任务中，会禁用中断
    taskENTER_CRITICAL();
    {
        if(g_uart_rx_buffer.data_count>0){
            // 从环形缓冲区读取数据
            *data=g_uart_rx_buffer.buffer[g_uart_rx_buffer.read_index];

            // 更新读指针（环形）
            g_uart_rx_buffer.read_index=(g_uart_rx_buffer.read_index+1)% UART_RX_BUFFER_SIZE;
            g_uart_rx_buffer.data_count--;      // 减少数据计数
            result=0;                           // 标记成功
        }
    }
    taskEXIT_CRITICAL();
    /* ========== 退出任务临界段 ========== */

    return result;
}


/**
 * @brief 解析数据帧（状态机实现）
 * @param frame 解析出的数据帧存储位置
 * @return 0:成功解析出完整帧, -1:失败或无完整帧
 * 
 * @note 实现要点：
 *       1. 使用状态机解析帧格式
 *       2. 处理帧头同步
 *       3. 验证帧长度合法性
 *       4. 校验和验证
 *       5. 更新统计信息
 */
int parse_data_frame(DataFrame_t *frame){
    uint8_t byte;
    uint8_t frame_buffer[64];   // 临时缓冲区，存储正在解析的帧
    int buffer_index=0;         // 缓冲区索引
    int frame_state=0;          // 帧解析状态：0=寻找帧头, 1=读取帧数据

    //状态机解析数据帧
    while(uart_read_byte(&byte)==0 && buffer_index<sizeof(frame_buffer)){
        frame_buffer[buffer_index++]=byte;

        if(frame_state==0){
            //寻找帧头 0xAA
            if(byte==0xAA){
                frame_state=1;
                frame->frame_header = byte;
                frame->receive_timestamp = xTaskGetTickCount();
            }else{
                buffer_index=0;     //重新开始
            }
        }else{
            /* 读取帧数据 */
            if(buffer_index>=4){
                // 至少有帧头+类型+长度+1字节数据
                frame->frame_type=frame_buffer[1];
                frame->frame_length=frame_buffer[2];

                /* 检查帧长度合法性 */
                if(frame->frame_length > UART_FRAME_MAX_SIZE) {
                    printf("帧长度错误: %d\n", frame->frame_length);
                    
                    taskENTER_CRITICAL();
                    g_uart_rx_buffer.frame_errors++;
                    taskEXIT_CRITICAL();
                    
                    return -1;
                }

                /* 检查是否接收完整帧 */
                int expected_frame_size = 3 + frame->frame_length + 1; // 头+类型+长度+数据+校验
                if(buffer_index>=expected_frame_size){
                    /* 复制数据部分 */
                    memcpy(frame->frame_data, &frame_buffer[3], frame->frame_length);
                    frame->frame_checksum = frame_buffer[expected_frame_size - 1];

                    //校验数据帧
                    uint8_t calculated_checksum=
                }

            }
        }
    }

}


/* ============================================================================
 * FreeRTOS任务实现
 * ============================================================================ */
/**
 * @brief UART数据处理任务
 * @param pvParameters 任务参数（未使用）
 * 
 * @note 任务功能：
 *       1. 周期性尝试解析数据帧
 *       2. 根据帧类型进行不同处理
 *       3. 监控缓冲区状态
 */
void vUartProcessTask(void *pvParameters){
    DataFrame_t frame;                                // 解析出的数据帧
    TickType_t xLastWakeTime;                         // 上次唤醒时间
    const TickType_t xFrequency = pdMS_TO_TICKS(100); // 100ms检查一次

    xLastWakeTime=xTaskGetTickCount();

    printf("UART处理任务启动\n");

    while(1){
        //尝试解析数据帧
        if(parse_data_frame(&frame)==0){
            printf("接收到数据帧: 类型=0x%02X, 长度=%d, 时间戳=%lu\n",
                   frame.frame_type, frame.frame_length, frame.receive_timestamp);
                
            //根据帧类型进行处理
            switch(frame.frame_type){
                case FRAME_TYPE_COMMAND:
                    printf("命令帧:");
                    for(int i=0;i<frame.frame_length;i++){
                        printf("%02X ", frame.frame_data[i]);
                    }
                    printf("\n");
                    break;

                case FRAME_TYPE_DATA:
                    printf("数据帧:");
                    for(int i = 0; i < frame.frame_length; i++){
                        printf("%02X",frame.frame_data[i]);
                    }
                    printf("\n");
                    break;

                case FRAME_TYPE_STATUS:
                    printf("状态帧: 状态码=0x%02X\n", frame.frame_data[0]);
                    break;
                    
                case FRAME_TYPE_ERROR:
                    printf("错误帧: 错误码=0x%02X\n", frame.frame_data[0]);
                    break;
                    
                default:
                    printf("未知帧类型: 0x%02X\n", frame.frame_type);
                    break;
            }

        }

        //检查是否有数据等待处理
        uint16_t available=uart_get_available_data();
        if(available > 0) {
            printf("缓冲区中还有 %d 字节待处理\n", available);
        }
        
        vTaskDelayUntil(&xLastWakeTime, xFrequency);
    }
}


int main(void){
    /* 输出程序信息和配置 */
    printf("=== FreeRTOS 临界段保护 Demo 3: UART中断接收 ===\n\n");
    printf("配置信息:\n");
    printf("  UART缓冲区大小: %d 字节\n", UART_RX_BUFFER_SIZE);
    printf("  最大帧大小: %d 字节\n", UART_FRAME_MAX_SIZE);
    printf("  模拟数据发送周期: 50ms\n");
    printf("\n");
    
    /* 创建demo任务 */
    create_uart_interrupt_demo_tasks();
    
    /* 启动FreeRTOS调度器 */
    // 注意：一旦调用此函数，系统控制权交给FreeRTOS
    // main函数的后续代码正常情况下不会执行
    vTaskStartScheduler();
    
    /* 如果调度器启动失败才会执行到这里 */
    // 可能的原因：内存不足、配置错误等
    for(;;);
    
    return 0;
}



