/*
 * 练习2：时间片机制分析
 * 目标：深入理解listGET_OWNER_OF_NEXT_ENTRY()函数的工作原理
 * 用于遍历 FreeRTOS 的就绪任务列表（pxReadyTasksLists），以获取下一个就绪任务的 TCB（任务控制块）
 *
 * 练习要求：
 * 1. 观察三个同优先级任务的时间片轮转情况
 * 2. 分析监控任务对时间片轮转的影响
 * 3. 修改任务的工作负载，观察对时间片分配的影响
 * 4. 思考：为什么vVariableLoadTask中的taskYIELD()会影响任务执行顺序？
 * 5. 实验：注释掉taskYIELD()，观察任务执行的变化
 * 
 * 扩展练习：
 * 1. 添加串口输出，实时显示任务执行统计信息
 * 2. 实现一个简单的任务执行时间测量功能
 * 3. 分析在不同系统负载下的时间片分配公平性
 */

#include <FreeRTOS.h>
#include <task.h>
#include <Queue.h>

//用于调试的全局变量
volatile uint32_t g_task_switch_count = 0;
volatile uint32_t g_task1_exec_count = 0;
volatile uint32_t g_task2_exec_count = 0;
volatile uint32_t g_task3_exec_count = 0;

//任务执行时间统计（模拟）
volatile uint32_t g_task1_exec_time=0;
volatile uint32_t g_task2_exec_time=0;

//任务句柄
TaskHandle_t xTaskHandle[4];

//任务参数结构体
typedef struct{
    uint8_t task_id;
    uint32_t work_load;
    volatile uint32_t *exec_counter;
    volatile uint8_t *flag;
}TaskParam_t;

//任务标志
volatile uint8_t task_flags[4]={0};

TaskParam_t task_params[4];

/* 模拟时间测量函数 */
uint32_t get_tick_count(void)
{
    return xTaskGetTickCount();
}


void vSamePriorityTask(void *pvParameters){
    TaskParam_t *param=(TaskParam_t *)pvParameters;
    uint32_t start_time,end_time;
    uint32_t local=0;

    for(;;){
        start_time=get_tick_count();
        (*param->flag)=1;

        /* 模拟工作负载 */
        volatile uint32_t work = param->work_load;
        while(work--) {
            __NOP();
        }

        end_time=get_tick_count();
        (*param->flag)=0;

        /* 统计执行次数 */
        (*(param->exec_counter))++;
        local_counter++;
        
        /* 每执行100次打印一次信息（如果有串口的话）*/
        if(local_counter % 100 == 0) {
            // printf("Task%d executed %d times\n", param->task_id, local_counter);
        }
        
        /* 注意：这里故意不调用vTaskDelay()，形成时间片轮转 */
    }

}

void vVariableLoadTask(void *pvParameters){
    uint32_t base_load=(uint32_t)pvParameters;
    uint32_t current_load;
    uint32_t cycle_count=0;

    for(;;){
        //动态调整工作负载
        current_load=base_load+(cycle_count%3)*500;

        task_flags[0]=1;

        //执行可变工作负载
        volatile uint32_t work=current_load;
        while(work--){
            __NOP();
        }

        task_flags[0]=0;

        cycle_count++;
        g_task1_exec_count++;

        if(cycle_count%50==0){
            taskYELD();
        }
    }
}

void vMonitorTask(void *pvParameters){
    TickType_t xLastWakeTime;
    const TickType_t xFrequency = pdMS_TO_TICKS(100); // 100ms周期
    
    xLastWakeTime = xTaskGetTickCount();

    for(;;)
    {
        /* 周期性唤醒 */
        vTaskDelayUntil(&xLastWakeTime, xFrequency);
        
        /* 设置监控标志 */
        task_flags[3] = 1;
        
        /* 这里可以添加系统状态监控代码 */
        g_task_switch_count++;
        
        /* 模拟监控工作 */
        vTaskDelay(pdMS_TO_TICKS(10));
        
        task_flags[3] = 0;
    }
}


int main(void){
    //初始化任务参数
    task_params[0].task_id = 1;
    task_params[0].work_load = 1500;
    task_params[0].exec_counter = &g_task1_exec_count;
    task_params[0].flag = &task_flags[0];
    
    task_params[1].task_id = 2;
    task_params[1].work_load = 1200;
    task_params[1].exec_counter = &g_task2_exec_count;
    task_params[1].flag = &task_flags[1];


    /* 创建同优先级任务1 */
    xTaskCreate(
        vSamePriorityTask,         // 任务函数
        "SameTask1",               // 任务名称
        configMINIMAL_STACK_SIZE,  // 栈大小
        &task_params[0],           // 任务参数
        2,                         // 优先级
        &xTaskHandle[0]            // 任务句柄
    );
    
    /* 创建同优先级任务2 */
    xTaskCreate(
        vSamePriorityTask,         // 任务函数
        "SameTask2",               // 任务名称
        configMINIMAL_STACK_SIZE,  // 栈大小
        &task_params[1],           // 任务参数
        2,                         // 优先级（与任务1相同）
        &xTaskHandle[1]            // 任务句柄
    );

    /* 创建可变负载任务 */
    xTaskCreate(
        vVariableLoadTask,         // 任务函数
        "VarLoadTask",             // 任务名称
        configMINIMAL_STACK_SIZE,  // 栈大小
        (void*)1000,               // 基础工作负载
        2,                         // 优先级（与其他任务相同）
        &xTaskHandle[2]            // 任务句柄
    );
    
    /* 创建高优先级监控任务 */
    xTaskCreate(
        vMonitorTask,              // 任务函数
        "Monitor",                 // 任务名称
        configMINIMAL_STACK_SIZE,  // 栈大小
        NULL,                      // 任务参数
        3,                         // 高优先级
        &xTaskHandle[3]            // 任务句柄
    );

    /* 启动调度器 */
    vTaskStartScheduler();
    
    /* 程序不应该运行到这里 */
    for(;;);
}


//栈溢出钩子函数
void vApplicationStackOverflowHook(TaskHandle_t xTask,char *pcTaskName){
    //栈溢出处理
    for(;;);
}

