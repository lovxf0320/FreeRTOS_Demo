/*
 * Demo5: 任务通知机制
 * 学习要点：
 * 1. 任务通知的基本使用 - 轻量级同步机制
 * 2. 不同的通知模式：覆盖、递增、位设置、无覆盖
 * 3. 任务通知作为二进制信号量使用
 * 4. 任务通知作为计数信号量使用
 * 5. 任务通知作为事件组使用
 * 6. 从中断发送任务通知
 * 7. 性能优势对比
 */

#include "FreeRTOS.h"
#include "task.h"
#include <stdio.h>

// 任务句柄
TaskHandle_t notification_receiver_handle;
TaskHandle_t data_processor_handle;
TaskHandle_t event_handler_handle;
TaskHandle_t counter_task_handle;
TaskHandle_t bit_handler_handle;

// 全局变量
volatile uint32_t interrupt_counter = 0;
volatile uint32_t notification_data = 0;

// 事件位定义（用于任务通知作为事件组）
#define EVENT_DATA_READY    (1 << 0)
#define EVENT_ERROR_OCCUR   (1 << 1) 
#define EVENT_TIMEOUT       (1 << 2)
#define EVENT_USER_INPUT    (1 << 3)

// ==================== 基础任务通知示例（作为二进制信号量使用） ====================

// 通知接收任务
void notification_receiver_task(void *pvParameters){
    uint32_t notification_value;

    for(;;){
        printf("[接收者] 等待任务通知...\n");

        //等待任务通知（作为二进制信号量使用）
        if(ulTaskNotifyTake(pdTRUE,             // 清除通知值
                            portMAX_DELAY)      // 永久等待
            >0){

            printf("[接收者] 收到任务通知！开始处理...\n");
            
            // 模拟数据处理
            vTaskDelay(pdMS_TO_TICKS(1000));
            printf("[接收者] 任务处理完成\n");
        }
    }
}

// 通知发送任务
void notification_sender_task(void *pvParameters){
    static Uint32_t send_count=0;

    for(;;){
        send_count++;
        printf("[发送者] 发送第 %lu 个通知\n", send_count);

        //发送任务通知
        xTaskNotifyGive(notification_receiver_handle);

        vTaskDelay(pdMS_TO_TICKS(3000));
    }
}

// ==================== 数据传递示例 ====================

// 数据处理任务 - 使用通知传递数据
void data_processor_task(void *pvParameters){
    uint32_t received_data;
    BaseType_t result;

    for(;;){
        printf("[数据处理器] 等待数据通知...\n");

        //等待通知并获取数据
        result=xTaskNotifyWait(0x00,            //进入时不清除任何位
                            0xFFFFFFFF,         //退出时清除所有位
                            &received_data,     //接收通知值
                            portMAX_DELAY);     //永久等待

        if(result == pdTRUE){
            printf("[数据处理器] 收到数据: %lu\n", received_data);

            //处理数据
            uint32_t processed_data = received_data * 2;
            printf("[数据处理器] 处理后的数据: %lu\n", processed_data);
            
            // 模拟处理时间
            vTaskDelay(pdMS_TO_TICKS(800));
        }
    }
}

// 数据发送任务
void data_sender_task(void *pvParameters){
    uint32_t data_to_send = 100;

    for(;;){
        printf("[数据发送器] 发送数据: %lu\n", data_to_send);
 
        // 发送数据通知（覆盖模式）
        xTaskNotify(data_processor_handle,        // 目标任务
                   data_to_send,                  // 通知值
                   eSetValueWithOverwrite);       // 覆盖模式

        data_to_send+=10;
        vTaskDelay(pdMS_TO_TICKS(2000));
    }

}


// ==================== 计数信号量模式示例 ====================

// 计数任务 - 任务通知作为计数信号量
void counter_task(void *pvParameters){
    uint32_t count;

    for(;;){
        printf("[计数任务] 等待计数信号...\n");

        //等待计数（作为计数信号量使用）
        count=ulTaskNotifyTake(pdFALSE,         //不清除，只递减
                            portMAX_DELAY);     //永久等待

        printf("[计数任务] 当前计数: %lu\n", count);

        if(count > 0){
            printf("[计数任务] 处理一个项目\n");
            vTaskDelay(pdMS_TO_TICKS(500));
        }
    }
}

// 计数增加任务
void counter_incrementer_task(void *pvParameters){
    static uint32_t increment_round = 0;

    for(;;){
        increment_round++;

        //随机增加1-3个计数
        uint32_t increment = (increment_round % 3) + 1;

        for(uint32_t i = 0; i < increment; i++){
            printf("[计数器] 增加计数 +1\n");
            xTaskNotifyGive(counter_task_handle);
            vTaskDelay(pdMS_TO_TICKS(200));
        }

        vTaskDelay(pdMS_TO_TICKS(2500));
    }
}


// ==================== 位操作模式示例 ====================

// 事件处理任务 - 任务通知作为事件组
void event_handler_task(void *pvParameters){
    uint32_t notification_bits;
    BaseType_t result;

    for(;;){
        printf("[事件处理器] 等待事件通知...\n");

        // 等待事件位通知
        result=xTaskNotifyWait(0x00,                    // 进入时不清除
                                0xFFFFFFFF,             // 退出时清除所有位
                                &notification_bits,     // 接收通知值
                                pdMS_TO_TICKS(5000));   // 超时5秒

        if(result==pdTRUE){
            printf("[事件处理器] 收到事件: 0x%08lX\n", notification_bits);

            // 检查各种事件
            if(notification_bits & EVENT_DATA_READY) {
                printf("[事件处理器] - 处理数据就绪事件\n");
            }
            
            if(notification_bits & EVENT_ERROR_OCCUR) {
                printf("[事件处理器] - 处理错误事件\n");
            }
            
            if(notification_bits & EVENT_TIMEOUT) {
                printf("[事件处理器] - 处理超时事件\n");
            }
            
            if(notification_bits & EVENT_USER_INPUT) {
                printf("[事件处理器] - 处理用户输入事件\n");
            }
        }else{
            printf("[事件处理器] 等待事件超时\n");
        }

    }

}

// 事件生成任务
void event_generator_task(void *pvParameters){
    static uint32_t event_cycle = 0;
    uint32_t event_bits;
    
    for(;;) {
        event_cycle++;
        
        // 循环产生不同事件
        switch(event_cycle % 4) {
            case 1:
                event_bits = EVENT_DATA_READY;
                printf("[事件生成器] 生成数据就绪事件\n");
                break;
            case 2:
                event_bits = EVENT_ERROR_OCCUR | EVENT_TIMEOUT;
                printf("[事件生成器] 生成错误和超时事件\n");
                break;
            case 3:
                event_bits = EVENT_USER_INPUT;
                printf("[事件生成器] 生成用户输入事件\n");
                break;
            case 0:
                event_bits = EVENT_DATA_READY | EVENT_USER_INPUT;
                printf("[事件生成器] 生成多个事件\n");
                break;
        }
        
        // 发送事件位通知
        xTaskNotify(event_handler_handle,
                   event_bits,
                   eSetBits);    // 位设置模式
        
        vTaskDelay(pdMS_TO_TICKS(3000));
    }
}


// ==================== 无覆盖模式示例 ====================

// 数据累积任务
void data_accumulator_task(void *pvParameters) {
    uint32_t accumulated_value;
    BaseType_t result;
    
    for(;;) {
        printf("[数据累积器] 等待数据累积通知...\n");
        
        // 等待通知（累积模式）
        result = xTaskNotifyWait(0x00,
                               0xFFFFFFFF,
                               &accumulated_value,
                               portMAX_DELAY);
        
        if(result == pdTRUE) {
            printf("[数据累积器] 累积值: %lu\n", accumulated_value);
            
            // 处理累积的数据
            vTaskDelay(pdMS_TO_TICKS(1500));
        }
    }
}

// 数据累积发送任务
void data_accumulator_sender_task(void *pvParameters) {
    static uint32_t value_to_add = 5;
    
    for(;;) {
        printf("[累积发送器] 累积数据: +%lu\n", value_to_add);
        
        // 发送累积通知（不覆盖，累积值）
        xTaskNotify(data_processor_handle,    // 复用数据处理器任务
                   value_to_add,
                   eIncrement);               // 递增模式
        
        value_to_add += 5;
        vTaskDelay(pdMS_TO_TICKS(800));
    }
}

// ==================== 模拟中断发送通知 ====================

// 模拟中断服务程序
void simulate_interrupt_service_routine(void){
    BaseType_t higher_priority_task_woken=pdFALSE;

    interrupt_counter++;

    // 从中断发送任务通知
    vTaskNotifyGiveFromISR(notification_receiver_handle,    
                        &higher_priority_task_woken);

    // 请求任务切换（如果有更高优先级任务被唤醒）
    portYIELD_FROM_ISR(higher_priority_task_woken);
}

// 中断模拟任务
void interrupt_simulator_task(void *pvParameters){
    for(;;){
        vTaskDelay(pdMS_TO_TICKS(4000));
        
        printf("[中断模拟器] 模拟中断发生...\n");
        simulate_interrupt_service_routine();
    }
}


// ==================== 性能对比演示 ====================

// 性能测试任务
void performance_test_task(void *pvParameters){
    TickType_t start_time, end_time;
    uint32_t test_iterations = 1000;

    vTaskDelay(pdMS_TO_TICKS(10000));  // 等待系统稳定
    
    printf("\n[性能测试] 开始性能对比测试...\n");
    
    // 测试任务通知性能
    start_time = xTaskGetTickCount();
    for(uint32_t i = 0; i < test_iterations; i++) {
        ulTaskNotifyTake(pdTRUE, 0);  // 非阻塞检查
    }
    end_time = xTaskGetTickCount();

    printf("[性能测试] 任务通知 %lu 次操作耗时: %lu ticks\n", 
           test_iterations, end_time - start_time);
    
    printf("[性能测试] 任务通知的优势:\n");
    printf("- 更快的执行速度（比信号量快约45%%）\n");
    printf("- 更少的内存占用\n");
    printf("- 更少的代码量\n");
    printf("- 不需要创建额外的内核对象\n");

    for(;;) {
        vTaskDelay(pdMS_TO_TICKS(60000));  // 1分钟后重复测试
    }
}

// 监控任务
void notification_monitor_task(void *pvParameters) {
    TaskHandle_t current_task;
    uint32_t notification_value;
    
    for(;;) {
        printf("\n=== 任务通知状态监控 ===\n");
        
        // 检查通知接收者的状态
        current_task = notification_receiver_handle;
        if(current_task != NULL) {
            printf("通知接收者任务状态: %s\n", 
                   eTaskGetState(current_task) == eBlocked ? "阻塞等待" : "运行中");
        }
        
        printf("模拟中断计数: %lu\n", interrupt_counter);
        printf("=======================\n\n");
        
        vTaskDelay(pdMS_TO_TICKS(8000));
    }
}


int main(void) {
    printf("FreeRTOS Demo9: 任务通知机制\n");
    printf("任务通知是FreeRTOS V8.2.0引入的轻量级同步机制\n\n");
    
    // 创建任务
    
    // 基础通知示例
    xTaskCreate(notification_receiver_task, "NotifyRcv", 256, NULL, 3, &notification_receiver_handle);
    xTaskCreate(notification_sender_task, "NotifySend", 256, NULL, 2, NULL);
    
    // 数据传递示例  
    xTaskCreate(data_processor_task, "DataProc", 256, NULL, 3, &data_processor_handle);
    xTaskCreate(data_sender_task, "DataSend", 256, NULL, 2, NULL);
    
    // 计数信号量示例
    xTaskCreate(counter_task, "Counter", 256, NULL, 2, &counter_task_handle);
    xTaskCreate(counter_incrementer_task, "CountInc", 256, NULL, 1, NULL);
    
    // 事件组示例
    xTaskCreate(event_handler_task, "EventHdl", 256, NULL, 3, &event_handler_handle);
    xTaskCreate(event_generator_task, "EventGen", 256, NULL, 2, NULL);
    
    // 累积模式示例
    xTaskCreate(data_accumulator_task, "DataAccum", 256, NULL, 2, NULL);
    xTaskCreate(data_accumulator_sender_task, "AccumSend", 256, NULL, 1, NULL);
    
    // 中断模拟
    xTaskCreate(interrupt_simulator_task, "IntSim", 256, NULL, 1, NULL);
    
    // 性能测试和监控
    xTaskCreate(performance_test_task, "PerfTest", 256, NULL, 1, NULL);
    xTaskCreate(notification_monitor_task, "NotifyMon", 512, NULL, 1, NULL);
    
    printf("所有任务创建完成，启动调度器...\n");
    
    // 启动调度器
    vTaskStartScheduler();
    
    printf("调度器启动失败!\n");
    return -1;
}

 /*
学习要点总结：

1. 任务通知基础概念：
   - 每个任务都有一个32位的通知值
   - 比信号量、队列、事件组更轻量级
   - 只能有一个任务等待通知（一对一通信）
   - 发送方可以是任务或中断

2. 任务通知模式：
   - eNoAction: 只发送通知，不改变通知值
   - eSetBits: 按位设置通知值（作为事件组）
   - eIncrement: 递增通知值（作为计数信号量）
   - eSetValueWithOverwrite: 覆盖通知值
   - eSetValueWithoutOverwrite: 不覆盖已有值

3. 主要API函数：
   - xTaskNotifyGive(): 发送通知（递增模式）
   - ulTaskNotifyTake(): 等待通知（计数模式）
   - xTaskNotify(): 发送通知（指定模式）
   - xTaskNotifyWait(): 等待通知（完整版本）
   - 中断版本：vTaskNotifyGiveFromISR()等

4. 应用场景：
   - 替代二进制信号量：任务间简单同步
   - 替代计数信号量：资源计数管理
   - 替代事件组：轻量级事件位管理
   - 中断到任务通信：中断服务程序通知任务

5. 性能优势：
   - 比信号量快约45%
   - 更少的内存占用
   - 不需要创建额外的内核对象
   - 减少代码复杂度

6. 使用限制：
   - 只能一对一通信（一个任务等待）
   - 只有32位通知值
   - 不能在任务间排队多个通知
   - 不适合需要多个任务等待的场景

7. 注意事项：
   - 合理选择清除/保留通知值
   - 注意通知值的覆盖问题
   - 中断中使用FromISR版本
   - 与其他同步机制的选择权衡
*/