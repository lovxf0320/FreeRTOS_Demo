/*===========================================
 * 工业数据采集与监控系统
 * 功能：传感器采集、数据处理、通信上报、本地显示
 *==========================================*/
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "event_groups.h"
#include "timers.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

// 任务优先级定义
#define SENSOR_TASK_PRIORITY        3  // 传感器采集 - 高优先级
#define DATA_PROCESS_TASK_PRIORITY  2  // 数据处理 - 中优先级  
#define COMM_TASK_PRIORITY          2  // 通信任务 - 中优先级
#define DISPLAY_TASK_PRIORITY       1  // 显示任务 - 低优先级
#define WATCHDOG_TASK_PRIORITY      4  // 看门狗 - 最高优先级

// 系统配置参数
#define TEMP_ALARM_THRESHOLD    80.0f   // 温度告警阈值
#define HUMIDITY_ALARM_THRESHOLD 90.0f  // 湿度告警阈值
#define HISTORY_SIZE            10      // 历史数据缓存大小

// 按键定义
#define KEY_NEXT    1

// 数据结构定义
typedef struct {
    float temperature;    // 温度 (°C)
    float humidity;      // 湿度 (%)
    float pressure;      // 压力 (kPa)
    uint32_t timestamp;  // 时间戳
    uint8_t status;      // 状态标志
} SensorData_t;

typedef struct {
    SensorData_t data;
    float temp_avg;      // 温度平均值
    float temp_max;      // 温度最大值
    float temp_min;      // 温度最小值
    uint8_t alarm_flags; // 告警标志
} ProcessedData_t;

// 全局变量和队列
QueueHandle_t xSensorDataQueue;     // 原始数据队列
QueueHandle_t xProcessedDataQueue;  // 处理后数据队列
QueueHandle_t xCommQueue;          // 通信队列
SemaphoreHandle_t xDisplayMutex;   // 显示互斥锁
EventGroupHandle_t xSystemEvents;  // 系统事件组

// 全局统计变量
static uint32_t g_taskCounters[5] = {0}; // 任务运行计数器
static uint32_t g_systemRunTime = 0;     // 系统运行时间

// 事件位定义
#define SENSOR_DATA_READY_BIT    (1 << 0)
#define DATA_PROCESSED_BIT       (1 << 1)
#define COMM_SEND_BIT           (1 << 2)
#define ALARM_BIT               (1 << 3)

//===========================================
// 任务1: 传感器数据采集任务
//===========================================




//===========================================
// 系统初始化
//===========================================
void SystemInit(void){
    printf("=== 工业数据采集与监控系统启动 ===\n");

    //创建队列
    xSensorDataQueue=xQueueCreate(10,sizeof(SensorData_t));
    xProcessedDataQueue=xQueueCreate(5,sizeof(ProcessedData_t));
    xCommQueue=xQueueCreate(5,sizeof(ProcessedData_t));

    //检查队列创建是否成功
    if(!xSensorDataQueue || !xProcessedDataQueue || !xCommQueue){
        printf("ERROR: Failed to create queues!\n");
        return;
    }

    //创建信号量
    xDisplayMutex=xSemphoreCreateMutex();
    if(!xDisplayMutex){
        printf("ERROR: Failed to create mutex!\n");
        return;
    }

    //创建事件组
    xSystemEvents=xEventGroupCreate();
    if(!xSystemEvents){
        printf("ERROR: Failed to create event group!\n");
        return;
    }

    //创建任务
    BaseType_t result;
    result = xTaskCreate(SensorTask, "Sensor", 256, NULL, SENSOR_TASK_PRIORITY, NULL);
    if (result != pdPASS) printf("ERROR: Failed to create Sensor task!\n");
    
    result = xTaskCreate(DataProcessTask, "DataProc", 512, NULL, DATA_PROCESS_TASK_PRIORITY, NULL);
    if (result != pdPASS) printf("ERROR: Failed to create DataProcess task!\n");
    
    result = xTaskCreate(CommunicationTask, "Comm", 512, NULL, COMM_TASK_PRIORITY, NULL);
    if (result != pdPASS) printf("ERROR: Failed to create Communication task!\n");
    
    result = xTaskCreate(DisplayTask, "Display", 256, NULL, DISPLAY_TASK_PRIORITY, NULL);
    if (result != pdPASS) printf("ERROR: Failed to create Display task!\n");
    
    result = xTaskCreate(WatchdogTask, "Watchdog", 128, NULL, WATCHDOG_TASK_PRIORITY, NULL);
    if (result != pdPASS) printf("ERROR: Failed to create Watchdog task!\n");
    
    printf("All tasks created successfully!\n");

    //启动调度器
    printf("Starting FreeRTOS scheduler...\n");
    vTaskStartScheduler();

    //如果程序运行到这里，说明调度器启动失败
    printf("ERROR: Scheduler failed to start!\n");
}







//===========================================
// 主函数
//===========================================
int main(void){
    // 硬件初始化（在实际应用中）
    // HAL_Init();
    // SystemClock_Config();
    // GPIO_Init();
    // UART_Init();
    // etc.
    
    // 系统初始化
    SystemInit();

    //程序不应该运行到这里
    while(1){
        vTaskDelay(pdMS_TO_TICKS(1000));
    }

    return 0;
}








