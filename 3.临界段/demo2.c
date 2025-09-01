/*
 * Demo 2: 环形缓冲区 - 生产者消费者模式
 * 
 * 功能描述：
 * - 实现线程安全的环形缓冲区
 * - 多个生产者任务向缓冲区写入数据
 * - 多个消费者任务从缓冲区读取数据
 * - 使用临界段保护读写指针和计数器
 * 
 * 学习要点：
 * - 环形缓冲区的实现原理
 * - 生产者-消费者模式的同步
 * - 临界段保护共享数据结构
 * - 缓冲区满/空的处理机制
 */

#include "FreeRTOS.h"
#include "task.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* 任务优先级定义 */
#define PRODUCER_TASK_PRIORITY  (tskIDLE_PRIORITY + 2)
#define CONSUMER_TASK_PRIORITY  (tskIDLE_PRIORITY + 2)
#define MONITOR_TASK_PRIORITY   (tskIDLE_PRIORITY + 3)

/* 任务栈大小 */
#define TASK_STACK_SIZE         (configMINIMAL_STACK_SIZE * 2)

/* 环形缓冲区配置 */
#define RING_BUFFER_SIZE        16
#define PRODUCER_COUNT          2   // 生产者数量
#define CONSUMER_COUNT          2   // 消费者数量

/* 数据包结构 */
typedef struct {
    uint32_t sequence_number;   // 序列号
    uint32_t producer_id;       // 生产者ID
    uint32_t timestamp;         // 时间戳
    uint8_t data[8];            // 数据内容
} DataPacket_t;

/* 环形缓冲区结构 */
typedef struct {
    DataPacket_t buffer[RING_BUFFER_SIZE];  // 数据缓冲区
    volatile uint16_t head;                 // 写指针（生产者使用）
    volatile uint16_t tail;                 // 读指针（消费者使用）
    volatile uint16_t count;                // 当前数据包数量
    
    /* 统计信息 */
    volatile uint32_t total_written;        // 总写入次数
    volatile uint32_t total_read;           // 总读取次数
    volatile uint32_t write_failures;       // 写入失败次数（缓冲区满）
    volatile uint32_t read_failures;        // 读取失败次数（缓冲区空）
    volatile uint32_t max_usage;            // 最大使用量
} RingBuffer_t;

/* 全局环形缓冲区 */
RingBuffer_t g_ring_buffer = {0};

/* 任务句柄 */
TaskHandle_t xProducerTaskHandles[PRODUCER_COUNT];
TaskHandle_t xConsumerTaskHandles[CONSUMER_COUNT];
TaskHandle_t xMonitorTaskHandle = NULL;

/**
 * @brief 向环形缓冲区写入数据包
 * @param packet 要写入的数据包
 * @return 0:成功, -1:缓冲区满
 */
int ring_buffer_write(const DataPacket_t *packet){
    int result=0;

    if(!packet){
        return -1;
    }

    /* ========== 进入临界段 - 保护缓冲区状态 ========== */
    taskENTER_CRITICAL();
    {
        if(g_ring_buffer.count < RING_BUFFER_SIZE) {
            /* 复制数据包到缓冲区 */
            g_ring_buffer.buffer[g_ring_buffer.head] = *packet;
            
            /* 更新写指针（循环） */
            g_ring_buffer.head = (g_ring_buffer.head + 1) % RING_BUFFER_SIZE;
            
            /* 更新数据包计数 */
            g_ring_buffer.count++;
            
            /* 更新统计信息 */
            g_ring_buffer.total_written++;
            if(g_ring_buffer.count > g_ring_buffer.max_usage) {
                g_ring_buffer.max_usage = g_ring_buffer.count;
            }
            
            result = 0;
        } else {
            /* 缓冲区满 */
            g_ring_buffer.write_failures++;
            result = -1;
        }
    }
    taskEXIT_CRITICAL();
}


/**
 * @brief 从环形缓冲区读取数据包
 * @param packet 读取的数据包存储位置
 * @return 0:成功, -1:缓冲区空
 */
int ring_buffer_read(DataPacket_t *packet){
    int result=0;

    if(!packet){
        return -1;
    }

    taskENTER_CRITICAL();
    {
        if(g_ring_buffer.count>0){
           //从缓冲区复制数据包
           *packet=g_ring_buffer.buffer[g_ring_buffer.tail];
           
           //更新读指针（循环）
           g_ring_buffer.tail=(g_ring_buffer.tail+1)%RING_BUFFER_SIZE;

           //更新数据包计数
            g_ring_buffer.count--;
            
            // 更新统计信息
            g_ring_buffer.total_read++;
            
            result = 0;
        }else{
            //缓冲区空
            g_ring_buffer.read_failures++;
            result = -1;
        }
    }
    taskEXIT_CRITICAL();

    return result;
}


/**
 * @brief 生产者任务
 * @param pvParameters 任务参数（生产者ID）
 */
void vProducerTask(void *pvParameters){
    uint32_t producer_id = (uint32_t)pvParameters;
    uint32_t sequence_number = 0;
    DataPacket_t packet;
    TickType_t xLastWakeTime;
    const TickType_t xFrequency = pdMS_TO_TICKS(300 + producer_id * 100); // 不同生产速度
    
    /* 初始化上次唤醒时间 */
    xLastWakeTime = xTaskGetTickCount();
    
    printf("生产者%lu启动 (生产周期:%lu ms)\n", 
           producer_id, (300 + producer_id * 100));
    
    while(1) {
        /* 准备数据包 */
        packet.sequence_number = ++sequence_number;
        packet.producer_id = producer_id;
        packet.timestamp = xTaskGetTickCount();
        
        /* 填充一些模拟数据 */
        for(int i = 0; i < 8; i++) {
            packet.data[i] = (uint8_t)((producer_id * 100) + (sequence_number % 100));
        }
        
        /* 尝试写入缓冲区 */
        if(ring_buffer_write(&packet) == 0) {
            printf("生产者%lu: 写入数据包#%lu (时间戳:%lu)\n",
                   producer_id, sequence_number, packet.timestamp);
        } else {
            printf("生产者%lu: 缓冲区满，数据包#%lu写入失败\n",
                   producer_id, sequence_number);
        }
        
        vTaskDelayUntil(&xLastWakeTime, xFrequency);
    }
}


void vConsumerTask(void *pvParameters){
    uint32_t consumer_id = (uint32_t)pvParameters;
    DataPacket_t packet;
    TickType_t xLastWakeTime;
    const TickType_t xFrequeny=pdMS_TO_TICKS(400 + consumer_id * 150);

    //初始化上次唤醒时间
    xLastWakeTime=xTaskTickCount();

    printf("消费者%lu启动 (消费周期:%lu ms)\n", 
           consumer_id, (400 + consumer_id * 150));

    while(1){
        vTaskDelayUntil(&xLastWakeTime,xFrequeny);

        //尝试从缓冲区读取数据包
        if(ring_buffer_read(&packet)==0){
            TickType_t processing_delay=xTaskGetCount()-packet.timestamp;

            printf("消费者%lu: 读取数据包#%lu (来自生产者%lu, 延迟:%lu ticks)\n",
                    consumer_id, packet.sequence_number, 
                    packet.producer_id, processing_delay);

            //模拟数据处理时间
            vTaskDelay(pdMS_TO_TICKS(50));
            
            printf("消费者%lu: 处理完成数据包#%lu\n",
                   consumer_id, packet.sequence_number);
        }else{
            printf("消费者%lu: 缓冲区空，无数据可读\n", consumer_id);
        }
    }
}


/**
 * @brief 创建环形缓冲区demo任务
 */
void create_ring_buffer_demo_tasks(void){
    BaseType_t xReturn;
    char task_name[16];

    //创建生产者任务
    for(int i=0;i<PRODUCER_COUNT;i++){
        sprintf(task_name, "Process%d", i+1);

        xReturn=xTaskCreate(vProducerTask,
                            task_name,
                            TASK_STACK_SIZE,
                            (void*)(i+1),       //传递生产者ID
                            PRODUCER_TASK_PRIORITY,
                            &xProducerTaskHandles[i]);

        if(xReturn!=pdPASS){
            printf("生产者任务%d创建失败!\n", i + 1);
            return;
        }
    }

    //创建消费者任务
    for(int i=0;i<CONSUMER_COUNT;i++){
        sprintf(task_name, "Consumer%d", i+1);

        xReturn=xTaskCreate(vProducerTask,
                            task_name,
                            TASK_STACK_SIZE,
                            (void*)(i + 1),  // 传递消费者ID
                            CONSUMER_TASK_PRIORITY,
                            &xConsumerTaskHandles[i]);

        if(xReturn!=pdPASS){
            printf("生产者任务%d创建失败!\n", i + 1);
            return;
        }
    }

    //创建监控任务
    xReturn=xTaskCreate(vMonitorTask,
                        "monitor",
                        TASK_STACK_SIZE,
                        NULL,
                        MONITOR_TASK_PRIORITY,
                        &xMonitorTaskHandle
                        );

    if(xReturn!=pdPASS){
        printf("监控任务创建失败!\n");
        return;
    }

    printf("环形缓冲区Demo启动成功!\n");
    printf("观察生产者-消费者模式下临界段如何保护共享缓冲区\n\n");
}


/**
 * @brief 主函数
 */
int main(void)
{
    printf("=== FreeRTOS 临界段保护 Demo 2: 环形缓冲区 ===\n\n");
    printf("配置信息:\n");
    printf("缓冲区大小: %d\n", RING_BUFFER_SIZE);
    printf("生产者数量: %d\n", PRODUCER_COUNT);
    printf("消费者数量: %d\n", CONSUMER_COUNT);
    printf("\n");
    
    /* 创建demo任务 */
    create_ring_buffer_demo_tasks();
    
    /* 启动调度器 */
    vTaskStartScheduler();
    
    /* 正常情况下不会执行到这里 */
    for(;;);
    
    return 0;
}