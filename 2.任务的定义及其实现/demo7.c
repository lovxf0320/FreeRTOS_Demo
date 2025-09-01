/*
 * Demo3: 队列通信机制
 * 学习要点：
 * 1. 基本队列操作 - 发送和接收数据
 * 2. 队列集合 - 同时监听多个队列
 * 3. 从中断发送到队列
 * 4. 不同数据类型的队列使用
 * 5. 队列的阻塞和非阻塞操作
 */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

//队列句柄
QueueHandle_t data_queue;       // 基础数据队列
QueueHandle_t message_queue;    // 消息队列
QueueHandle_t sensor_queue;     // 传感器数据队列
QueueHandle_t command_queue;    // 命令队列

//任务句柄
TaskHandle_t sender_task_handle;
TaskHandle_t receiver_task_handle;
TaskHandle_t sensor_task_handle;
TaskHandle_t display_task_handle;
TaskHandle_t command_processor_handle;

//数据结构定义
typedef struct{
    uint32_t id;
    float value;
    char unit[10];
    uint32_t timestamp;
}sensor_data_t;

typedef struct{
    char text[50];      // 修复：原来是test，应该是text
    uint8_t priority;
    uint32_t sender_id;
}message_t;

typedef struct{
    uint8_t cmd_type;
    uint32_t param1;
    uint32_t param2;
    char description[30];
}command_t;

//命令类型定义
#define CMD_LED_ON  1
#define CMD_LED_OFF 2
#define CMD_RESET   3
#define CMD_STATUS  4


// ==================== 基础队列示例 ====================
// 数据发送任务
void sender_task(void* pvParameters){
    uint32_t send_data = 0;
    BaseType_t result;

    for(;;){
        result = xQueueSend(data_queue, &send_data, pdMS_TO_TICKS(1000));
        if(result == pdPASS){
            printf("[发送者] 数据发送成功: %lu\n", send_data);
        }else{
            printf("[发送者] 数据发送失败\n");
        }

        send_data++;
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

//数据接收任务
void receiver_task(void* pvParameters){
    uint32_t receive_data = 0;
    BaseType_t result;

    for(;;){
        result = xQueueReceive(data_queue, &receive_data, pdMS_TO_TICKS(1000));  // 修复：xQueueRceive -> xQueueReceive
        if(result == pdPASS){
            printf("[接收者] 数据接收成功: %lu\n", receive_data);  // 修复：打印信息应该是"接收者"
        }else{
            printf("[接收者] 数据接收失败\n");
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

// ==================== 传感器数据队列示例 ====================
// 模拟传感器任务
void sensor_task(void* pvParameters){
    sensor_data_t sensor_data;
    static uint32_t sensor_id = 1;

    for(;;){
        sensor_data.id = sensor_id;
        sensor_data.value = 20.0f + (rand() % 100) / 10.0f;
        strcpy(sensor_data.unit, "°C");  // 修复：添加sensor_data.前缀
        sensor_data.timestamp = xTaskGetTickCount();

        printf("[传感器%lu] 采集温度: %.1f%s\n", 
           sensor_data.id, sensor_data.value, sensor_data.unit);

        if(xQueueSend(sensor_queue, &sensor_data, pdMS_TO_TICKS(1000)) != pdPASS){
            printf("[传感器%lu] 数据发送失败，队列满!\n", sensor_data.id);
        }

        sensor_id = (sensor_id % 3) + 1;

        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

void display_task(void* pvParameters){
    sensor_data_t sensor_data;
    message_t message;
    BaseType_t result;

    for(;;){
        //首先检查是否有传感器数据（非阻塞）
        result = xQueueReceive(sensor_queue, &sensor_data, 0);  // 修复：xQueueRecive -> xQueueReceive
        if(result == pdPASS) {
            printf("[显示器] 传感器数据 - ID:%lu, 值:%.1f%s, 时间:%lu\n",
                sensor_data.id, sensor_data.value, 
                sensor_data.unit, sensor_data.timestamp);
        }

        result = xQueueReceive(message_queue, &message, 0);
        if(result == pdPASS) {
            printf("[显示器] 消息 - 优先级:%d, 发送者:%lu, 内容:%s\n",
                   message.priority, message.sender_id, message.text);
        }

        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

// ==================== 命令处理队列示例 ====================
// 命令处理任务
void command_processor_task(void *pvParameters){
    command_t cmd;  // 修复：amd -> cmd

    for(;;){
        //等待命令
        if(xQueueReceive(command_queue, &cmd, portMAX_DELAY) == pdPASS){  // 修复：porMAX_DELAY -> portMAX_DELAY
            printf("[命令处理器] 收到命令: %s\n", cmd.description);

            switch(cmd.cmd_type){
                case CMD_LED_ON:
                    printf("[命令处理器] 执行: 打开LED %lu\n", cmd.param1);
                    break;

                case CMD_LED_OFF:
                    printf("[命令处理器] 执行: 关闭LED %lu\n", cmd.param1);
                    break;
                
                case CMD_RESET:
                    printf("[命令处理器] 执行: 系统重置\n");
                    break;

                case CMD_STATUS:
                    printf("[命令处理器] 执行: 查询状态\n");
                    //发送状态信息
                    message_t status_msg = {  // 修复：语法错误
                        .priority = 1,
                        .sender_id = 99,
                    };
                    strcpy(status_msg.text, "系统运行正常");
                    xQueueSend(message_queue, &status_msg, 0);
                    break;

                default:
                    printf("[命令处理器] 未知命令类型: %d\n", cmd.cmd_type);
                    break;
            }

            // 模拟命令执行时间
            vTaskDelay(pdMS_TO_TICKS(500));
        }
    }  
}

// 命令发送任务（模拟用户输入）
void command_sender_task(void *pvParameters){
    command_t cmd;
    static uint8_t cmd_sequence = 0;

    for(;;){
        cmd_sequence++;

        // 循环发送不同命令
        switch(cmd_sequence % 4){  // 修复：seitch -> switch, smd_sequence -> cmd_sequence
            case 1:
                cmd.cmd_type = CMD_LED_ON;
                cmd.param1 = 1;
                strcpy(cmd.description, "打开LED灯");
                break;
            
            case 2:
                cmd.cmd_type = CMD_STATUS;
                strcpy(cmd.description, "查询系统状态");
                break;

            case 3:
                cmd.cmd_type = CMD_LED_OFF;
                cmd.param1 = 1;
                strcpy(cmd.description, "关闭LED灯");
                break;

            case 0:
                cmd.cmd_type = CMD_RESET;
                strcpy(cmd.description, "系统重置命令");
                break;
        }

        printf("[命令发送器] 发送命令: %s\n", cmd.description);
    
        if(xQueueSend(command_queue, &cmd, pdMS_TO_TICKS(1000)) != pdPASS) {
            printf("[命令发送器] 命令发送失败!\n");
        }
        
        vTaskDelay(pdMS_TO_TICKS(4000));
    }
}

// 队列监控任务
void queue_monitor_task(void *pvParameters){
    for(;;){
        printf("\n=== 队列状态监控 ===\n");
        // uxQueueMessagesWaiting() - 获取队列中当前等待的消息数量
        printf("数据队列: %d/%d\n", uxQueueMessagesWaiting(data_queue), 5);
        printf("消息队列: %d/%d\n", uxQueueMessagesWaiting(message_queue), 10);
        printf("传感器队列: %d/%d\n", uxQueueMessagesWaiting(sensor_queue), 8);
        printf("命令队列: %d/%d\n", uxQueueMessagesWaiting(command_queue), 5);
        printf("==================\n\n");
        
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}

// 模拟中断服务程序 - 从中断发送紧急消息
void simulate_interrupt_send_message(void){
    message_t urgent_msg = {
        .priority = 0,        //最高优先级
        .sender_id = 0,       //中断ID
    };

    strcpy(urgent_msg.text, "紧急中断消息!");

    BaseType_t higher_priority_task_woken = pdFALSE;

    // 从中断发送消息（使用FromISR版本）
    // 中断中的队列发送（绝不阻塞），队列满了直接返回失败
    xQueueSendFromISR(
        message_queue,                      // 目标队列
        &urgent_msg,                        // 要发送的数据指针
        &higher_priority_task_woken);       // 输出参数
    
    // 如果有更高优先级任务被唤醒，请求任务切换
    portYIELD_FROM_ISR(higher_priority_task_woken);
}

int main(void){  // 修复：void main -> int main
    printf("FreeRTOS Demo: 队列通信机制\n");

    //创建各种队列
    //uxQueueLength：队列能容纳的最大元素数量
    //uxItemSize：每个元素的大小（字节）
    data_queue = xQueueCreate(5, sizeof(uint32_t));
    if(data_queue == NULL){
        printf("数据队列创建失败!\n");
        return -1;
    }

    message_queue = xQueueCreate(10, sizeof(message_t));
    if(message_queue == NULL){
        printf("消息队列创建失败!\n");  // 修复：错误信息
        return -1;
    }

    sensor_queue = xQueueCreate(8, sizeof(sensor_data_t));
    if(sensor_queue == NULL){
        printf("传感器队列创建失败!\n");  // 修复：错误信息
        return -1;
    }

    command_queue = xQueueCreate(5, sizeof(command_t));
    if(command_queue == NULL){
        printf("命令队列创建失败!\n");  // 修复：错误信息
        return -1;
    }

    printf("所有队列创建成功!\n");

    // 创建任务
    xTaskCreate(sender_task, "Sender", 256, NULL, 2, &sender_task_handle);
    xTaskCreate(receiver_task, "Receiver", 256, NULL, 2, &receiver_task_handle);
    
    xTaskCreate(sensor_task, "Sensor", 256, NULL, 3, &sensor_task_handle);
    xTaskCreate(display_task, "Display", 512, NULL, 3, &display_task_handle);
    
    xTaskCreate(command_processor_task, "CmdProc", 512, NULL, 4, &command_processor_handle);
    xTaskCreate(command_sender_task, "CmdSender", 256, NULL, 1, NULL);
    
    xTaskCreate(queue_monitor_task, "QueueMon", 512, NULL, 1, NULL);

    printf("所有任务创建完成，启动调度器...\n");

    //启动调度器
    vTaskStartScheduler();  // 修复：vTaskStartSchedular -> vTaskStartScheduler

    printf("调度器启动失败!\n");
    return 0;
}

/*
学习要点总结：

1. 队列基础概念：
   - FIFO（先进先出）数据结构
   - 线程安全的数据传递机制
   - 可以传递任意类型和大小的数据

2. 队列操作API：
   - xQueueCreate()：创建队列
   - xQueueSend()：发送数据到队列尾部
   - xQueueSendToFront()：发送数据到队列头部
   - xQueueReceive()：从队列接收数据
   - xQueuePeek()：查看队列数据但不移除

3. 队列状态查询：
   - uxQueueMessagesWaiting()：查看队列中消息数量
   - uxQueueSpacesAvailable()：查看队列剩余空间
   - xQueueIsQueueFullFromISR()：检查队列是否满

4. 阻塞与超时：
   - 发送方：队列满时可以阻塞等待空间
   - 接收方：队列空时可以阻塞等待数据
   - 使用超时参数控制等待时间

5. 中断中的队列操作：
   - 使用FromISR版本的API
   - 注意higher_priority_task_woken参数的使用
   - 及时调用portYIELD_FROM_ISR()

6. 应用场景：
   - 任务间数据传递
   - 中断到任务的数据传递
   - 缓冲区管理
   - 命令队列
   - 日志系统

7. 注意事项：
   - 队列创建时指定元素大小和数量
   - 避免在队列满时强制发送导致阻塞
   - 合理设置队列大小避免内存浪费
   - 注意数据的生命周期和拷贝语义
*/