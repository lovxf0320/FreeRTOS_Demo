/*
 * Demo6: 软件定时器
 * 学习要点：
 * 1. 一次性定时器和周期性定时器
 * 2. 定时器的创建、启动、停止、重置
 * 3. 定时器回调函数的使用
 * 4. 动态改变定时器周期
 * 5. 定时器状态查询和管理
 * 6. 实际应用场景：超时监控、周期性任务、LED闪烁等
 */

#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include <stdio.h>

// ==================== 定时器句柄声明 ====================
// 定时器句柄用于操作和管理定时器
// 定时器句柄（就像定时器的"遥控器"）
TimeHandle_t led_timer;         //LED闪烁定时器
TimeHandle_t oneshot_timer;     //一次性定时器
TimeHandle_t timeout_timer;     //超时监控定时器
TimeHandle_t periodic_timer;    //周期性数据采集定时器
TimeHandle_t dynamic_timer;     //动态周期定时器


// ==================== 任务句柄声明 ====================
// 任务句柄用于任务间通信和管理
TaskHandle_t timer_control_task_handle;
TaskHandle_t data_sender_task_handle;


// ==================== 全局变量 ====================
// 使用volatile确保变量在中断和任务间的可见性
volatile uint32_t led_state=0;          //LED当前状态（开/关）
volatile uint32_t periodic_counter=0;   //周期性采集计数器
volatile uint32_t timeout_flag=0;       //超时标志：0=正常，1=超时
volatile uint32_t oneshot_executed=0;   //一次性定时器执行次数统计


// ==================== 定时器ID定义 ====================
// 用于在回调函数中区分不同的定时器
#define LED_TIMER_ID        1  // LED定时器标识
#define ONESHOT_TIMER_ID    2  // 一次性定时器标识
#define TIMEOUT_TIMER_ID    3  // 超时定时器标识
#define PERIODIC_TIMER_ID   4  // 周期性定时器标识
#define DYNAMIC_TIMER_ID    5  // 动态定时器标识


// ==================== 定时器回调函数 ====================
/**
 * LED闪烁定时器回调函数
 * 功能：每500ms切换一次LED状态
 * 参数：xTimer - 触发回调的定时器句柄
 */
void led_timer_callback(TimerHandle_t xTimer){

    //获取定时器ID，用于确认是哪个定时器触发了回调
    uint32_t timer_id=(uint32_t)pvTimerGetTimerID(xTimer);  //用于获取定时器的 ID

    //确定是LED定时器
    if(timer_id==LED_TIMER_ID){
        //切换LED状态（0变1，1变0）
        led_state=!led_state;
        printf("[LED定时器] LED状态: %s\n", led_state ? "ON" : "OFF");
    }
}


/**
 * 一次性定时器回调函数
 * 功能：执行延时任务，只触发一次
 * 参数：xTimer - 触发回调的定时器句柄
 */
void oneshot_timer_callback(TimerHandle_t xTimer){

    uint32_t timer_id=(uint32_t)pvTimerGetTimerID(xTimer);

    //确定一次性定时器
    if(timer_id==ONESHOT_TIMER_ID){
        oneshot_executed++;
        printf("[一次性定时器] 定时器触发！执行次数: %lu\n", oneshot_executed);
        printf("[一次性定时器] 5秒延时任务完成\n");
    }
}


/**
 * 周期性数据采集定时器回调函数
 * 功能：每2秒执行一次数据采集
 * 参数：xTimer - 触发回调的定时器句柄
 */
void periodic_data_callback(TimerHandle_t xTimer){

    uint32_t timer_id=(uint32_t)pvTimerGetTimerID(xTimer);

    //确定是周期性定时器
    if(timer_id==PERIODIC_TIMER_ID){
        periodic_counter++;     //采集次数计数

        //模拟传感器数据采集（实际项目中会读取真实传感器）
        float temperature=25.0f+(periodic_counter%10);      //温度：25-34°C
        float humidity=50.0f+(periodic_counter%20);         //湿度：50-69%

        printf("[数据采集]第%lu次采集 - 温度:%.1f°C 湿度:%.1f%%\n", 
            periodic_counter, temperature, humidity);

        if(periodic_counter%10==0){
            printf("[数据采集]生成第%lu份数据报告\n", periodic_counter / 10);
        }
    }

}


/**
 * 动态周期定时器回调函数
 * 功能：演示如何动态改变定时器周期
 * 参数：xTimer - 触发回调的定时器句柄
 */
void dynamic_timer_callback(TimerHandle_t xTimer){
    uint32_t timer_id=(uinter32_t)pvTimerGetTimerID(xTimer);
    static uint32_t dynamic_counter=0;          // 静态计数器，保持状态

    //确认是动态定时器
    if(timer_id==DYNAMIC_TIMER_ID){
        dynamic_counter++;          //执行次数计数
        printf("[动态定时器] 执行第%lu次，当前周期:%lums\n", 
               dynamic_counter, pdTICKS_TO_MS(xTimerGetPeriod(xTimer)));

        //每执行5次就改变一次定时器周期
        if(dynamic_counter%5==0){
            TickType_t new_period;

            //根据执行次数在三个周期间循环：1秒、2秒、3秒
            switch((dynamic_counter/5)%3){
                case 0:
                    new_period=pdMS_TO_TICKS(1000);     //1秒
                    break;
                case 1:
                    new_period = pdMS_TO_TICKS(2000);  // 2秒
                    break;
                case 2:
                    new_period = pdMS_TO_TICKS(2000);  // 3秒
                    break;
            }
            printf("[动态定时器] 改变周期为%lums\n", pdTICKS_TO_MS(new_period));
            
            // 动态改变定时器周期（不需要停止再启动）
            xTimerChangePeriod(xTimer, new_period, 0);

        }
    }
}


// ==================== 任务函数 ====================

/**
 * 定时器控制任务
 * 功能：按步骤控制各个定时器的启动、停止、重置
 * 参数：pvParameters - 任务参数（未使用）
 */
void timer_control_task(void *pvParameters){
    uint32_t control_step=0;        //控制步骤计数器

    //任务主函数
    for(;;){
        control_step++;     //步骤递增

        printf("\n[定时器控制] ===== 控制步骤 %lu =====\n", control_step);

        //根据不同步骤执行不同的控制操作
        switch(sontrol_step){
            case 1:
               printf("[定时器控制] 启动LED闪烁定时器\n"); 
               xTimerStart(led_timer,0);
               break;

            case 2:
                printf("[定时器控制] 启动一次性定时器（5秒后触发）\n");
                xTimerStart(oneshot_timer,0);
                break;
            
            case 3:
                printf("[定时器控制] 启动周期性数据采集定时器\n");
                xTimerStart(periodic_timer, 0);  // 启动数据采集定时器
                break;
                
            case 4:
                printf("[定时器控制] 启动动态周期定时器\n");
                xTimerStart(dynamic_timer, 0);  // 启动动态定时器
                break;
                
            case 5:
                printf("[定时器控制] 启动超时监控定时器\n");
                xTimerStart(timeout_timer, 0);  // 启动超时监控定时器
                break;

            case 8:
                printf("[定时器控制] 停止LED定时器\n");
                xTimerStop(led_timer, 0);  // 停止LED定时器
                led_state = 0;  // 手动关闭LED
                break;
                
            case 10:
                printf("[定时器控制] 重新启动LED定时器\n");
                xTimerStart(led_timer, 0);  // 重新启动LED定时器
                break;

            case 12:
                printf("[定时器控制] 重置一次性定时器\n");
                xTimerReset(oneshot_timer, 0);  // 重置定时器，重新计时
                break;
                
            case 15:
                printf("[定时器控制] 停止超时定时器\n");
                xTimerStop(timeout_timer, 0);  // 停止超时定时器
                timeout_flag = 0;  // 清除超时标志
                break;
                
            case 20:
                printf("[定时器控制] 重启控制循环\n");
                control_step = 0;  // 重置步骤计数，重新开始循环
                break;
        }

        // 每3秒执行一次控制步骤
        vTaskDelay(pdMS_TO_TICKS(3000));
    }
}


/**
 * 数据发送任务
 * 功能：模拟数据发送过程，用于测试超时定时器功能
 * 参数：pvParameters - 任务参数（未使用）
 */
void data_sender_task(void *pvParameters){
    uint32_t send_counter=0;        //发送计数器

    //等待15秒让超时定时器启动
    vTaskDelay(pdMS_TO_TICKS(15000));

    //任务主循环
    for(;;){
        send_counter++;     //发送次数计数

        printf("[数据发送器] 开始发送数据包 #%lu\n", send_counter);

        //每次发送开始时重置超时定时器
        if(xTimerIsTimerActive(timeout_timer)){     // 检查定时器是否激活
            xTimerReset(timeout_timer,0);           // 重置定时器重新计时
            timeout_flag=0;                         // 清除超时标志
        }

        //模拟不同的数据发送时间（2-9秒，模拟网络延迟等情况）
        uint32_t send_time=2000 + (send_counter % 8) * 1000;
        printf("[数据发送器] 预计发送时间: %lums\n", send_time);

        //模拟数据发送过程（实际中可能是网络传输、文件写入等）
        vTaskDelay(pdMS_TO_TICKS(send_time));

        //发送完成后检查是否发生了超时
        if(timeout_flag){
            printf("[数据发送器]数据发送超时，取消发送\n");
            timeout_flag = 0;  // 清除超时标志
        }else{
            printf("[数据发送器]数据发送完成\n");
            
            // 发送成功，停止超时定时器
            xTimerStop(timeout_timer, 0);
        }

        // 发送间隔3秒
        vTaskDelay(pdMS_TO_TICKS(3000));

    }
}


/**
 * 定时器状态监控任务
 * 功能：定期监控并显示所有定时器的状态信息
 * 参数：pvParameters - 任务参数（未使用）
 */
void timer_monitor_task(void *pvParameters){
    //任务主循环
    for(;;){
        printf("\n=== 定时器状态监控 ===\n");

        //检查并显示各定时器的运行状态
        printf("LED定时器: %s\n", 
               xTimerIsTimerActive(led_timer) ? "运行中" : "已停止");
        
        printf("一次性定时器: %s\n", 
               xTimerIsTimerActive(oneshot_timer) ? "运行中" : "已停止");
        
        printf("超时定时器: %s\n", 
               xTimerIsTimerActive(timeout_timer) ? "运行中" : "已停止");
        
        printf("数据采集定时器: %s\n", 
               xTimerIsTimerActive(periodic_timer) ? "运行中" : "已停止");

        //显示动态定时器状态和当前周期
        printf("动态定时器: %s (当前周期:%lums)\n", 
               xTimerIsTimerActive(dynamic_timer) ? "运行中" : "已停止",
               xTimerIsTimerActive(dynamic_timer) ? 
               pdTICKS_TO_MS(xTimerGetPeriod(dynamic_timer)) : 0);

        // 显示系统统计信息
        printf("\n--- 统计信息 ---\n");
        printf("LED当前状态: %s\n", led_state ? "ON" : "OFF");
        printf("周期性采集次数: %lu\n", periodic_counter);
        printf("一次性定时器执行次数: %lu\n", oneshot_executed);
        printf("超时标志: %s\n", timeout_flag ? "已触发" : "正常");
        printf("===================\n\n");
        
        // 每10秒更新一次监控信息
        vTaskDelay(pdMS_TO_TICKS(10000));
    }
}


/**
 * 定时器命令处理任务
 * 功能：演示定时器的高级操作和管理
 * 参数：pvParameters - 任务参数（未使用）
 */
void timer_command_task(void *pvParameters){
    uint32_t command_cycle = 0;  // 命令循环计数器

    // 等待25秒让系统稳定运行
    vTaskDelay(pdMS_TO_TICKS(25000));

    //任务主循环
    for(;;){
        command_cycle++;

        printf("\n[定时器命令] 执行命令 #%lu\n", command_cycle);

        //根据循环次数执行不同的命令（6个命令循环）
        switch(command_cycle%6){
            case 1:
                printf("[定时器命令] 暂停所有周期性定时器\n");
                xTinmeStop(led_timer,0);
                xTimerStop(periodic_timer, 0); // 停止数据采集定时器
                xTimerStop(dynamic_timer, 0);  // 停止动态定时器
                break;
            
            case 2:
                printf("[定时器命令] 恢复LED定时器\n");
                xTimerStart(led_timer, 0);  // 重新启动LED定时器
                break;

             case 3:
                printf("[定时器命令] 恢复数据采集定时器\n");
                xTimerStart(periodic_timer, 0);  // 重新启动数据采集定时器
                break;
                
            case 4:
                printf("[定时器命令] 恢复动态定时器\n");
                xTimerStart(dynamic_timer, 0);  // 重新启动动态定时器
                break;
                
            case 5:
                printf("[定时器命令] 手动触发一次性定时器\n");
                xTimerReset(oneshot_timer, 0);  // 重置一次性定时器，重新计时
                break;
                
            case 0:
                printf("[定时器命令] 获取定时器信息\n");
                // 如果LED定时器正在运行，显示剩余时间
                if(xTimerIsTimerActive(led_timer)) {
                    printf("LED定时器剩余时间: %lu ticks\n", 
                           xTimerGetExpiryTime(led_timer) - xTaskGetTickCount());
                }
                break;
        }

        // 每8秒执行一次命令
        vTaskDelay(pdMS_TO_TICKS(8000));
    }
}


/**
 * 主函数
 * 功能：系统初始化，创建定时器和任务，启动调度器
 */
int main(void){
    printf("软件定时器在定时器服务任务中执行回调函数\n\n");

    // ==================== 创建定时器 ====================
    
    /*
     * 创建LED闪烁定时器
     * 参数说明：
     * - "LEDTimer": 定时器名称（用于调试）
     * - pdMS_TO_TICKS(500): 定时器周期500毫秒
     * - pdTRUE: 自动重载，定时器到期后自动重启（周期性）
     * - (void *)LED_TIMER_ID: 定时器ID，用于在回调中识别
     * - led_timer_callback: 定时器到期时调用的回调函数
     */
    led_timer=xTimerCreate("LEDTimer",
                        pdMA_TO_TICKS(500),
                        pdTRUE,
                        (void*)LED_TIMES_ID,
                        led_timer_callback);

    if(led_timer==NULL){
        printf("LED定时器创建失败!\n");
        return -1;
    }

    /*
     * 创建一次性定时器
     * pdFALSE: 不自动重载，只触发一次
     */
    oneshot_timer=xTimerCreate("OneshotTimer", 
                                pdMS_TO_TICKS(5000),        // 5秒延时
                                pdFALSE,                    // 一次性
                                (void *)ONESHOT_TIMER_ID,
                                oneshot_timer_callback);

    if(oneshot_timer == NULL) {
        printf("一次性定时器创建失败!\n");
        return -1;
    }

    /*
     * 创建超时监控定时器
     * 用于监控操作超时，7秒后触发
     */
    timeout_timer=xTimerCreate("TimeoutTimer",
                                pdMS_TO_TICKS(7000),        // 7秒超时
                                pdFALSE,                    // 一次性
                                (void *)TIMEOUT_TIMER_ID,
                                timeout_timer_callback);

    if(timeout_timer == NULL) {
        printf("超时定时器创建失败!\n");
        return -1;
    }

    /*
     * 创建周期性数据采集定时器
     * 每2秒触发一次数据采集
     */
    periodic_timer = xTimerCreate("PeriodicTimer",
                                 pdMS_TO_TICKS(2000),       // 2秒周期
                                 pdTRUE,                    // 周期性
                                 (void *)PERIODIC_TIMER_ID,
                                 periodic_data_callback);
    
    if(periodic_timer == NULL) {
        printf("周期性定时器创建失败!\n");
        return -1;
    }

    /*
     * 创建动态周期定时器
     * 初始周期1秒，后续会动态改变
     */
    dynamic_timer=xTimerCreate("DynamicTimer",
                                pdMS_TO_TICKS(1000),
                                pdTRUE,
                                (void*)DYNAMIC_TIMER_ID,
                                dynamic_timer_callback);
    if(dynamic_timer==NULL){
        printf("动态定时器创建失败!\n");
        return -1;
    }

    printf("所有定时器创建成功!\n");
    // ==================== 创建任务 ====================
    
    /*
     * 创建任务函数说明：
     * xTaskCreate(任务函数, 任务名称, 堆栈大小, 参数, 优先级, 任务句柄)
     * 优先级数字越大优先级越高
     */
    
    // 创建定时器控制任务（优先级3，较高）
    xTaskCreate(timer_control_task, "TimerCtrl", 512, NULL, 3, &timer_control_task_handle);
    
    // 创建数据发送任务（优先级2，中等）
    xTaskCreate(data_sender_task, "DataSender", 256, NULL, 2, &data_sender_task_handle);
    
    // 创建定时器监控任务（优先级1，较低）
    xTaskCreate(timer_monitor_task, "TimerMon", 512, NULL, 1, NULL);
    
    // 创建定时器命令处理任务（优先级1，较低）
    xTaskCreate(timer_command_task, "TimerCmd", 256, NULL, 1, NULL);
    
    printf("所有任务创建完成，启动调度器...\n");
    
    // 启动FreeRTOS调度器，开始多任务调度
    // 注意：调度器启动后，main函数不会继续执行
    vTaskStartScheduler();
    
    // 如果执行到这里，说明调度器启动失败（通常是内存不足）
    printf("调度器启动失败!\n");
    return -1;

}


// ==================== 应用钩子函数 ====================

/**
 * 堆栈溢出钩子函数
 * 当任务堆栈溢出时被调用（需要在FreeRTOSConfig.h中启用）
 */
void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName) {
    printf("堆栈溢出: %s\n", pcTaskName);
    for(;;);  // 停止系统运行
}

/**
 * 内存分配失败钩子函数
 * 当动态内存分配失败时被调用
 */
void vApplicationMallocFailedHook(void) {
    printf("内存分配失败!\n");
    for(;;);  // 停止系统运行
}


/*
学习要点总结：

1. 软件定时器基础概念：
   - 在定时器服务任务中执行回调函数
   - 不占用硬件定时器资源
   - 精度取决于系统tick频率
   - 回调函数在定时器任务上下文中执行

2. 定时器类型：
   - 一次性定时器：pdFALSE（不自动重载）
   - 周期性定时器：pdTRUE（自动重载）
   - 可以动态改变定时器类型

3. 主要API函数：
   - xTimerCreate()：创建定时器
   - xTimerStart()：启动定时器
   - xTimerStop()：停止定时器
   - xTimerReset()：重置定时器
   - xTimerChangePeriod()：改变定时器周期
   - xTimerDelete()：删除定时器

4. 定时器状态查询：
   - xTimerIsTimerActive()：检查定时器是否激活
   - xTimerGetPeriod()：获取定时器周期
   - xTimerGetExpiryTime()：获取到期时间
   - pvTimerGetTimerID()：获取定时器ID

5. 回调函数注意事项：
   - 回调函数应该快速执行
   - 不要在回调中调用会阻塞的API
   - 可以使用定时器ID区分不同定时器
   - 可以在回调中启动/停止其他定时器

6. 应用场景：
   - LED闪烁控制
   - 超时监控
   - 周期性数据采集
   - 延时执行任务
   - 看门狗功能
   - 系统状态监控

7. 配置要求：
   - FreeRTOSConfig.h中设置configUSE_TIMERS为1
   - 配置configTIMER_TASK_PRIORITY定时器任务优先级
   - 配置configTIMER_QUEUE_LENGTH定时器命令队列长度
   - 配置configTIMER_TASK_STACK_DEPTH定时器任务堆栈大小

8. 性能考虑：
   - 定时器数量影响内存使用
   - 回调函数执行时间影响定时器精度
   - 定时器任务优先级要合理设置
   - 避免在回调中执行耗时操作
*/