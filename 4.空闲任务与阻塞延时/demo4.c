/*
 * FreeRTOS 任务切换机制深度演示
 * 展示 vTaskDelay() 如何触发任务切换以及相关API的使用
 */

#include "FreeRTOS.h"
#include "task.h"

//全局变量用于追踪切换
volatile uint32_t context_switch_count=0;
TaskHandle_t task_a_handle,task_b_handle,monitor_handle;


//重写上下文切换钩子函数（如果启动了钩子）
#if(configUSE_YASK_SWOCG_CPINT==1)
void vApplicationTaskSwitchHook(void){
    context_switch_count++;

    //获取切换前后的任务信息
    static TaskHandle_t prev_task=NULL;
    TaskHandle_t curr_task=xTaskGetCurrentHandle();

    if(prev_task!=curr_task){
        printf("[切换] 从任务 %s 切换到 %s (第%u次切换)\n",
            prev_task?pcTaskGetName(prev_task):"NULL",
            pcTaskGetName(curr_task),
            context_switch_count
            );

        prev_task=curr_task;
    }
}

#endif


//任务A:演示vTaskDelay()的切换机制
void TaskA(void *pvParameters){
    TickType_t last_wake_time=xTaskGetTickCount();

    while(1){
        printf("\n[任务A] 开始执行 - 时间:%u\n", xTaskGetTickCount());
        
        // 方式1：相对延时 - 立即进入阻塞状态
        printf("[任务A] 调用 vTaskDelay(500) - 即将阻塞并切换任务\n");
        printf("[任务A] 当前状态: 运行中 → 即将变为阻塞\n");
        vTaskDelay(pdMS_TO_TICKS(500));

        printf("[任务A] vTaskDelay(500) 返回 - 重新获得CPU - 时间:%u\n", xTaskGetTickCount());
        printf("[任务A] 状态变化: 阻塞 → 就绪 → 运行中\n");
        
        // 方式2：绝对延时 - 更精确的周期性执行
        printf("[任务A] 调用 vTaskDelayUntil() - 精确周期性延时\n");
        vTaskDelayUntil(&last_wake_time,pdMS_TO_TICKS(500));

        printf("[任务A] vTaskDelayUntil() 返回\n");
    }
}


//任务B：展示不同优先级的抢占
void TaskB(void *pvParameters){
    while(1) {
        printf("\n[任务B] 获得CPU，开始执行 - 时间:%u\n", xTaskGetTickCount());
        
        // 模拟一些工作
        for(int i = 0; i < 3; i++) {
            printf("[任务B] 工作中... %d/3\n", i+1);
            
            // 短暂延时，但仍会触发任务切换
            vTaskDelay(pdMS_TO_TICKS(100));
        }
        
        printf("[任务B] 工作完成，进入长时间延时\n");
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}


//监控任务：显示任务状态和切换信息
void MonitorTask(void *pvParameters){
    while(1){
        vTaskDelay(pdMS_TO_TICKS(3000));    // 每3秒监控一次

        printf("\n" "="*60 "\n");
        printf("=== 任务切换监控报告 ===\n");
        printf("系统时间: %u ticks\n", xTaskGetTickCount());
        printf("总切换次数: %u\n", context_switch_count);

        //查询各任务状态
        TaskHandle_t handles[]={task_a_handle,task_b_handle,monitor_handle};
        char* name[]={"任务A","任务B","监控任务"};

        for(int i=0;i<3;i++){
            eTaskState state=eTaskGetState(handle[i]);
            const char* state_str;

            switch(state) {
                case eRunning:   state_str = "运行中"; break;
                case eReady:     state_str = "就绪"; break;
                case eBlocked:   state_str = "阻塞"; break;
                case eSuspended: state_str = "挂起"; break;
                case eDeleted:   state_str = "已删除"; break;
                default:         state_str = "未知"; break;
            }

            printf("%s: %s\n", names[i], state_str);
        }
        //显示当前运行的任务
        TaskHandle_t current=xTaskGetCurrentTaskHandle();
        printf("当前运行任务: %s\n", pcTaskGetName(current));
        
        printf("="*60 "\n");
    }
}


//演示调度器暂停的任务
void SchedulerDemo(void *pvParameters){
    vTaskDelay(pdMS_TO_TICKS(10000)); // 等待10秒
    
    while(1) {
        printf("\n[调度器] 演示调度器暂停和恢复\n");
        printf("[调度器] 暂停调度器 - 所有任务切换停止\n");
        
        vTaskSuspendAll(); // 暂停调度器
        
        // 在暂停期间，即使调用vTaskDelay也不会切换任务
        printf("[调度器] 调度器已暂停，进行原子操作...\n");
        for(volatile int i = 0; i < 1000000; i++); // 模拟原子操作
        printf("[调度器] 原子操作完成\n");
        
        xTaskResumeAll(); // 恢复调度器
        printf("[调度器] 调度器已恢复，任务切换重新启用\n");
        
        vTaskDelay(pdMS_TO_TICKS(15000)); // 15秒后再次演示
    }
}


//演示强制任务切换的任务
void ForceYieldDemo(void *pvParameters){
    vTaskDelay(pdMS_TO_TICKS(5000));

    while(1){
        printf("\n[强制切换] 演示 taskYIELD() 的效果\n");
        printf("[强制切换] 调用 taskYIELD() 主动让出CPU\n");

        //强制让出CPU，即使没有延时
        taskYIELD();        // 立即切换到其他同优先级或更高优先级任务

        printf("[强制切换] taskYIELD() 返回，重新获得CPU\n");

        vTaskDelay(pdMS_TO_TICKS(8000));    //8秒后再次演示
    }
}



int main(void){
    printf("=== FreeRTOS 任务切换机制深度演示 ===\n\n");
    
    printf("重要概念说明:\n");
    printf("1. vTaskDelay() 不是普通延时，会立即阻塞当前任务并切换\n");
    printf("2. 任务切换在 Tick 中断、API 调用、强制切换时发生\n");
    printf("3. 高优先级任务总是会抢占低优先级任务\n");
    printf("4. 观察任务状态变化：运行→阻塞→就绪→运行\n\n");


    //创建任务（不同的优先级）
    xTaskCreate(TaskA, "TaskA", 1000, NULL, 2, &task_a_handle);
    xTaskCreate(TaskB, "TaskB", 1000, NULL, 1, &task_b_handle);
    xTaskCreate(MonitorTask, "Monitor", 1000, NULL, 3, &monitor_handle);
    
    // 可选的高级演示任务
    xTaskCreate(ForceYieldDemo, "YieldDemo", 1000, NULL, 2, NULL);
    xTaskCreate(SchedulerDemo, "SchedDemo", 1000, NULL, 4, NULL);
    
    printf("启动调度器...\n");
    printf("观察要点：每次vTaskDelay()调用都会触发任务切换！\n\n");
    
    // 启动调度器
    vTaskStartScheduler();
    
    printf("调度器启动失败！\n");
    while(1);
}


/*
 * 关键API总结：
 * 
 * 任务切换相关：
 * - vTaskDelay()           : 相对延时，立即阻塞并切换
 * - vTaskDelayUntil()      : 绝对延时，精确周期执行
 * - taskYIELD()            : 强制让出CPU
 * - portYIELD_FROM_ISR()   : 中断中触发切换
 * 
 * 调度器控制：
 * - vTaskSuspendAll()      : 暂停调度器
 * - xTaskResumeAll()       : 恢复调度器
 * 
 * 状态查询：
 * - eTaskGetState()        : 获取任务状态
 * - xTaskGetCurrentTaskHandle() : 获取当前任务句柄
 * - xTaskGetTickCount()    : 获取系统时间
 * 
 * 预期现象：
 * 1. 每次调用vTaskDelay()都会看到任务立即切换
 * 2. 高优先级任务会抢占低优先级任务
 * 3. 监控任务显示实时的任务状态变化
 * 4. 可以观察到精确的切换时机和次数
 */