/*
 * Demo4: 事件组机制
 * 学习要点：
 * 1. 事件组的创建和基本操作
 * 2. 等待单个事件和多个事件
 * 3. 等待所有事件 vs 等待任意事件
 * 4. 事件的设置和清除
 * 5. 实际应用：系统初始化同步、多条件触发
 */

#include "FreeRTOS.h"
#include "task.h"
#include "even_groups.h"
#include <stdio.h>

//事件组句柄
EventGroupHandle_t system_events;
EventGroupHandle_t sensor_events;
EventGroupHandle_t network_events;

//系统事件位定义
#define SYSTEM_INIT_COMPLETE    (1<<0)      //系统初始化完成
#define HARDWARE_READY          (1<<1)      //硬件准备就绪
#define CONFIG_LOADED           (1<<2)      //配置加载完成
#define NETWORK_CONNECTED       (1<<3)      //网络连接成功
#define ALL_SYSTEMS_READY       (SYSTEM_INIT_COMPLETE | HARDWARE_READY | CONFIG_LOADED | NETWORK_CONNECTED)

// 传感器事件位定义
#define TEMP_SENSOR_READY      (1 << 0)     //温度传感器就绪
#define HUMIDITY_SENSOR_READY  (1 << 1)     //湿度传感器就绪
#define PRESSURE_SENSOR_READY  (1 << 2)     //压力传感器就绪
#define LIGHT_SENSOR_READY     (1 << 3)     //光照传感器就绪
#define ALL_SENSORS_READY      (TEMP_SENSOR_READY | HUMIDITY_SENSOR_READY | PRESSURE_SENSOR_READY | LIGHT_SENSOR_READY)

// 网络事件位定义
#define WIFI_CONNECTED         (1 << 0)     //WiFi连接
#define MQTT_CONNECTED         (1 << 1)     //MQTT连接
#define HTTP_SERVER_READY      (1 << 2)     //HTTP服务器就绪
#define DATA_SYNC_COMPLETE     (1 << 3)     //数据同步完成


//任务句柄
TaskHandle_t system_init_task_handle;
TaskHandle_t hardware_init_task_handle;
TaskHandle_t config_task_handle;
TaskHandle_t network_task_handle;
TaskHandle_t main_app_task_handle;
TaskHandle_t sensor_monitor_handle;

// ==================== 系统初始化相关任务 ====================
// 系统初始化任务
void system_init_task(void *pvParameters){
    printf("[系统初始化] 开始系统初始化...\n");

    //模拟初始化过程
    vTaskDelay(pdMS_TO_TICKS(1000));
    printf("[系统初始化] 内核初始化完成\n");
    
    vTaskDelay(pdMS_TO_TICKS(500));
    printf("[系统初始化] 内存管理初始化完成\n");
    
    vTaskDelay(pdMS_TO_TICKS(800));
    printf("[系统初始化] 系统初始化全部完成\n");

    //设置系统初始化完成事件
    xEventGroupSetBits(system_events,SYSTEM_INIT_COMPLETE);

    //任务完成。删除自己
    vTaskDelete(NULL);
}

// 硬件初始化任务
void hardware_init_task(void *pvParameters){
    printf("[硬件初始化] 开始硬件初始化...\n");
    
    // 模拟GPIO初始化
    vTaskDelay(pdMS_TO_TICKS(600));
    printf("[硬件初始化] GPIO初始化完成\n");
    
    // 模拟串口初始化
    vTaskDelay(pdMS_TO_TICKS(400));
    printf("[硬件初始化] 串口初始化完成\n");
    
    // 模拟SPI/I2C初始化
    vTaskDelay(pdMS_TO_TICKS(700));
    printf("[硬件初始化] 总线初始化完成\n");
    
    printf("[硬件初始化] 硬件初始化全部完成\n");

    // 设置硬件就绪事件
    xEventGroupSetBits(system_events,HARDWARE_READY);

    vTaskDelete(NULL);
}

// 配置加载任务
void config_task(void *pvParameters){
    printf("[配置管理] 开始加载配置...\n");
    
    // 模拟从Flash读取配置
    vTaskDelay(pdMS_TO_TICKS(1200));
    printf("[配置管理] Flash配置读取完成\n");
    
    // 模拟配置验证
    vTaskDelay(pdMS_TO_TICKS(300));
    printf("[配置管理] 配置验证完成\n");
    
    printf("[配置管理] 配置加载全部完成\n");
    
    // 设置配置加载完成事件
    xEventGroupSetBits(system_events, CONFIG_LOADED);
    
    vTaskDelete(NULL);
}

// 网络初始化任务
void network_task(void *pvParameters){
    printf("[网络管理] 开始网络初始化...\n");

    // 等待硬件就绪
    printf("[网络管理] 等待硬件就绪...\n");
    xEventGroupWaitBits(system_events,
                        HARDWARE_READY,
                        pdFALSE,            //不清除事件位
                        pdTRUE,             //等待所有位
                        portMAX_DELAY);     //永久等待

    printf("[网络管理] 硬件就绪，开始网络配置\n");

    // 模拟WiFi连接
    vTaskDelay(pdMS_TO_TICKS(2000));
    printf("[网络管理] WiFi连接成功\n");
    xEventGroupSetBits(network_events, WIFI_CONNECTED);

    // 模拟网络协议栈初始化
    vTaskDelay(pdMS_TO_TICKS(800));
    printf("[网络管理] 网络协议栈初始化完成\n");
    
    printf("[网络管理] 网络初始化全部完成\n");
    
    // 设置网络连接完成事件
    xEventGroupSetBits(system_events,NETWORK_CONNECTED);

    vTaskDelete(NULL);
}

// 主应用任务 - 等待所有系统准备就绪
void main_app_task(void *pvParameters){
    EventBits_t event_bits;
    printf("[主应用] 等待所有子系统准备就绪...\n");

    // 等待所有系统事件准备完毕
    // event_bits的值是ALL_SENSORS_READY的值
    event_bits=xEventGroupWaitBits(system_events,
                                ALL_SENSORS_READY,
                                pdFALSE,
                                pdTRUE,
                                pdMS_TO_TICKS(15000));

    if((event_bits & ALL_SYSTEMS_READY)==ALL_SYSTEMS_READY){
        printf("[主应用] ✓ 所有子系统准备就绪，主应用开始运行！\n");

        //主应用逻辑
        for(;;){
            printf("[主应用] 主应用正在运行...\n");

            //检查是否有新事件
            event_bits=xEventGroupGetBits(system_evnets);
            printf("[主应用] 当前系统事件状态: 0x%02X\n", (unsigned int)event_bits);

            vTaskDelay(pdMS_TO_TICKS(5000));
        }
    }else{

        printf("[主应用] ✗ 系统初始化超时！当前状态: 0x%02X\n", (unsigned int)event_bits);

        //分析哪些模块没有就绪
        if(!(event_bits & SYSTEM_INIT_COMPLETE)) {
            printf("[主应用] - 系统初始化未完成\n");
        }
        if(!(event_bits & HARDWARE_READY)) {
            printf("[主应用] - 硬件初始化未完成\n");
        }
        if(!(event_bits & CONFIG_LOADED)) {
            printf("[主应用] - 配置加载未完成\n");
        }
        if(!(event_bits & NETWORK_CONNECTED)) {
            printf("[主应用] - 网络连接未完成\n");
        }
    }
}


// ==================== 传感器相关任务 ====================

//传感器任务
void sensor_task(void *pvParameters){
    int sensor_type=(int)pvParameters;
    EventBits_t sensor_bit;
    const char* sensor_name;
    int init_time;

    switch(sensor_type){
        case 0:
            sensor_bit=TEMP_SENSOR_READY;
            sensor_name="温度传感器";
            init_time=1500;
            break;
        case 1:
            sensor_bit = HUMIDITY_SENSOR_READY;
            sensor_name = "湿度传感器";
            init_time = 1200;
            break;
        case 2:
            sensor_bit = PRESSURE_SENSOR_READY;
            sensor_name = "压力传感器";
            init_time = 2000;
            break;
        case 3:
            sensor_bit = LIGHT_SENSOR_READY;
            sensor_name = "光照传感器";
            init_time = 800;
            break;
        default:
            vTaskDelete(NULL);
            return;
    }

    printf("[%s] 开始初始化...\n", sensor_name);
    
    // 模拟传感器初始化时间
    vTaskDelay(pdMS_TO_TICKS(init_time));
    
    printf("[%s] 初始化完成\n", sensor_name);

    // 设置传感器就绪事件
    xEventGroupSetBits(sensor_events,sensor_bit);

    vTaskDelete(NULL);
}

void sensor_monitor_task(void *pvParameters){
    EventBits_t sensor_bits;

    for(;;){
        sensor_bits=xEventGroupWaitBits(sensor_events,
                                        ALL_SENSORS_READY,
                                        pdFALSE,        //不清除事件位
                                        pdFALSE,        //等待任意一个位（不是所有）
                                        pdMS_TO_TICKS(3000)
                                    );

        if(sensor_bits!=0){
            printf("[传感器监控] 检测到传感器就绪: 0x%02X\n", (unsigned int)sensor_bits);

            // 检查各个传感器状态
            if(sensor_bits & TEMP_SENSOR_READY) {
                printf("[传感器监控] - 温度传感器已就绪\n");
            }
            if(sensor_bits & HUMIDITY_SENSOR_READY) {
                printf("[传感器监控] - 湿度传感器已就绪\n");
            }
            if(sensor_bits & PRESSURE_SENSOR_READY) {
                printf("[传感器监控] - 压力传感器已就绪\n");
            }
            if(sensor_bits & LIGHT_SENSOR_READY) {
                printf("[传感器监控] - 光照传感器已就绪\n");
            }
            
            // 检查是否所有传感器都就绪
            if((sensor_bits & ALL_SENSORS_READY) == ALL_SENSORS_READY) {
                printf("[传感器监控] ✓ 所有传感器都已就绪！\n");
                break;  // 退出循环
            }

        }else{
            printf("[传感器监控] 等待传感器超时\n");
        }
    }

    // 所有传感器就绪后，开始数据采集
    for(;;) {
        printf("[传感器监控] 采集传感器数据...\n");
        
        // 模拟数据采集
        vTaskDelay(pdMS_TO_TICKS(3000));
    }
}


void network_service_task(void *pvParameters){
    EventBits_t network_bits;

    //等待WiFi连接
    printf("[网络服务] 等待WiFi连接...\n");
    xEventGroupWaitBits(network_events, WIFI_CONNECTED, pdFALSE, pdTRUE, portMAX_DELAY);
    printf("[网络服务] WiFi已连接，启动网络服务\n");

    // 启动MQTT客户端
    vTaskDelay(pdMS_TO_TICKS(1000));
    printf("[网络服务] MQTT客户端启动\n");
    xEventGroupSetBits(network_events, MQTT_CONNECTED);
    
    // 启动HTTP服务器
    vTaskDelay(pdMS_TO_TICKS(800));
    printf("[网络服务] HTTP服务器启动\n");
    xEventGroupSetBits(network_events, HTTP_SERVER_READY);
    
    // 数据同步
    vTaskDelay(pdMS_TO_TICKS(1500));
    printf("[网络服务] 数据同步完成\n");
    xEventGroupSetBits(network_events, DATA_SYNC_COMPLETE);

    //监控网络状态
    for(;;){
        network_bits=xEventGroupGetBits(network_events);
        printf("[网络服务] 网络状态: WiFi:%s MQTT:%s HTTP:%s 同步:%s\n",
                (network_bits&WIFI_CONNECTED)?"✓":"✗",
                (network_bits & MQTT_CONNECTED) ? "✓" : "✗",
                (network_bits & HTTP_SERVER_READY) ? "✓" : "✗",
                (network_bits & DATA_SYNC_COMPLETE) ? "✓" : "✗"
            );

        vTaskDelay(pdMS_TO_TICKS(4000));
    }
}


void event_monitor_task(void *pvParameters){
        EventBits_t system_bits, sensor_bits, network_bits;
    
    for(;;) {
        // 获取所有事件组的当前状态
        system_bits = xEventGroupGetBits(system_events);
        sensor_bits = xEventGroupGetBits(sensor_events);
        network_bits = xEventGroupGetBits(network_events);
        
        printf("\n=== 事件状态监控 ===\n");
        printf("系统事件: 0x%02X (初始化:%s 硬件:%s 配置:%s 网络:%s)\n",
               (unsigned int)system_bits,
               (system_bits & SYSTEM_INIT_COMPLETE) ? "✓" : "✗",
               (system_bits & HARDWARE_READY) ? "✓" : "✗",
               (system_bits & CONFIG_LOADED) ? "✓" : "✗",
               (system_bits & NETWORK_CONNECTED) ? "✓" : "✗");
        
        printf("传感器事件: 0x%02X (温度:%s 湿度:%s 压力:%s 光照:%s)\n",
               (unsigned int)sensor_bits,
               (sensor_bits & TEMP_SENSOR_READY) ? "✓" : "✗",
               (sensor_bits & HUMIDITY_SENSOR_READY) ? "✓" : "✗",
               (sensor_bits & PRESSURE_SENSOR_READY) ? "✓" : "✗",
               (sensor_bits & LIGHT_SENSOR_READY) ? "✓" : "✗");
        
        printf("网络事件: 0x%02X\n", (unsigned int)network_bits);
        printf("==================\n\n");
        
        vTaskDelay(pdMS_TO_TICKS(6000));
}


int main(void){
    printf("FreeRTOS Demo4: 事件组机制\n");

    //创建事件组
    system_events=xEventGroupCreate();
    if(system_events==NULL){
        printf("系统事件组创建失败!\n");
        return -1;
    }

    sensor_events=xEventGroupCreate();
    if(sensor_events==NULL){
        printf("传感器事件组创建失败!\n");
        return -1;
    }

    network_events=xEventGroupCreate();
    if(network_events==NULL){
        printf("网络事件组创建失败!\n");
        return -1;
    }

    printf("所有事件组创建成功\n");

    // 创建系统初始化相关任务
    xTaskCreate(system_init_task, "SysInit", 256, NULL, 3, &system_init_task_handle);
    xTaskCreate(hardware_init_task, "HwInit", 256, NULL, 3, &hardware_init_task_handle);
    xTaskCreate(config_task, "Config", 256, NULL, 2, &config_task_handle);
    xTaskCreate(network_task, "Network", 256, NULL, 2, &network_task_handle);
    
    // 创建主应用任务
    xTaskCreate(main_app_task, "MainApp", 512, NULL, 4, &main_app_task_handle);
    
    // 创建传感器任务
    xTaskCreate(sensor_task, "TempSensor", 256, (void*)0, 2, NULL);
    xTaskCreate(sensor_task, "HumiSensor", 256, (void*)1, 2, NULL);
    xTaskCreate(sensor_task, "PresSensor", 256, (void*)2, 2, NULL);
    xTaskCreate(sensor_task, "LightSensor", 256, (void*)3, 2, NULL);
    
    xTaskCreate(sensor_monitor_task, "SensorMon", 512, NULL, 3, &sensor_monitor_handle);
    
    // 创建网络服务任务
    xTaskCreate(network_service_task, "NetService", 256, NULL, 2, NULL);
    
    // 创建监控任务
    xTaskCreate(event_monitor_task, "EventMon", 512, NULL, 1, NULL);
    
    printf("所有任务创建完成，启动调度器...\n");

    vTaskStartSchedule();

    return -1;
}


//应用钩子函数
void vApplicationMallocFailedHook(TaskHandle_t xTask, char *pcTaskName){
    printf("堆栈溢出: %s\n", pcTaskName);
    for(;;);
}

void vApplicationMallocFailedHook(void) {
    printf("内存分配失败!\n");
    for(;;);
}


 /*
学习要点总结：

1. 事件组基础概念：
   - 24位事件位（bit 0-23可用）
   - 每个位代表一个事件状态
   - 多个任务可以等待同一个事件组
   - 一个任务可以等待多个事件位

2. 事件组操作API：
   - xEventGroupCreate()：创建事件组
   - xEventGroupSetBits()：设置事件位
   - xEventGroupClearBits()：清除事件位  
   - xEventGroupWaitBits()：等待事件位
   - xEventGroupGetBits()：获取当前事件状态

3. 等待模式：
   - 等待任意一个事件：xWaitForAnyBits = pdFALSE
   - 等待所有事件：xWaitForAllBits = pdTRUE
   - 等待后清除事件位：xClearOnExit = pdTRUE
   - 等待后保留事件位：xClearOnExit = pdFALSE

4. 典型应用场景：
   - 系统初始化同步
   - 多条件触发
   - 状态机事件管理
   - 多模块协调工作

5. 与其他同步机制对比：
   - vs 信号量：事件组可以同时处理多个条件
   - vs 队列：事件组只传递状态，不传递数据
   - vs 任务通知：事件组可以被多个任务等待

6. 注意事项：
   - 事件位数量有限（24位）
   - 事件位的设置是原子操作
   - 合理使用清除标志避免丢失事件
   - 注意超时处理机制
*/