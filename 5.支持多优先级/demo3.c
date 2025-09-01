/*
 * Demo 3: 任务切换机制详解
 * 演示任务切换的完整过程和上下文保存/恢复
 * 
 * 目标
 * 演示vTaskSwitchContext()函数的工作原理
 * 
 * 学习要点
 * 任务切换钩子：监控切换过程
 * 优先级抢占：高优先级任务立即执行
 * 切换开销：理解上下文保存/恢复的成本
 * 调度策略：基于优先级的抢占式调度
 */
#include "FreeRTOS.h"
#include "task.h"

// 任务切换信息统计结构体
// 用于记录每次任务切换的详细信息，便于分析调度行为
typedef struct {
    uint32_t switch_count;          // 切换次数：记录这是第几次任务切换
    uint32_t from_priority;         // 切换前任务优先级：被切换出去的任务优先级
    uint32_t to_priority;           // 切换后任务优先级：即将运行的任务优先级
    TickType_t switch_time;         // 切换时间戳：记录切换发生的系统时间
} task_switch_info_t;

//最大记录的切换日志条数
#define MAX_SWITCH_LOG 100
//切换日志数组：环形缓冲区存储任务切换历史
static task_switch_info_t switch_log[MAX_SWITCH_LOG];
//当前日志索引：指向下一个要写入的位置
static uint32_t switch_log_index=0;

#define HIGH_PRIORITY_TASK      5
#define ANALYZER_TASK           4
#define MEDIUM_PRIORITY_TASK    3
#define LOW_PRIORITY_TASK       2

TaskHandle_t High_Priority_Handle;
TaskHandle_t Medium_Priority_Handle;
TaskHandle_t Low_Priority_Handle;
TaskHandle_t Analyzer_Handle;

/**
 * 任务切换钩子函数 - FreeRTOS系统回调
 * 
 * 功能说明：
 * 1. 每当发生任务切换时，FreeRTOS内核自动调用此函数
 * 2. 在这里可以记录切换信息、统计性能数据等
 * 3. 此函数在中断上下文中执行，需要快速返回
 * 
 * 注意事项：
 * - 不能调用任何可能阻塞的FreeRTOS API
 * - 执行时间要尽可能短，避免影响系统性能
 * - 可以访问全局变量，但要注意线程安全
 */
void vApplicationTaskSwitchHook(void){
    //静态变量：保存上一次任务的优先级，用于对比分析
    static UBaseType_t prev_priority=0;
    UBaseType_t curr_priority;

    //检查日志缓冲区是否还有空间
    if(switch_log_index<MAX_SWITCH_LOG){
        //获取当前正在运行任务的优先级
        //NULL参数表示获取当前任务的优先级
        curr_priority=uxTaskPriorityGet(NULL);
        
        //记录本次切换的详细信息
        switch_log[switch_log_index].switch_count=switch_log_index+1;
        switch_log[switch_log_index].from_priority=prev_priority;
        switch_log[switch_log_index].to_priority=curr_priority;
        switch_log[switch_log_index].switch_time=xTaskGetTickCount();

        //更新索引和上一次优先级记录
        switch_log_index++;
        prev_priority=curr_priority;
    }
}

 /**
 * 高频任务 - 模拟需要快速响应的实时任务
 * 
 * 应用场景：
 * - 传感器数据采集
 * - 通信协议处理
 * - 实时控制算法
 * 
 * 特点：
 * - 最高优先级，能抢占其他任务
 * - 使用vTaskDelayUntil确保精确的周期性执行
 * - 处理时间短，频率高
 */
void high_frequency_task(void *pvParameters){
    //记录上次唤醒时间，用于精确周期控制
    TickType_t last_wake_time=xTaskGetTickCount();
    for(;;){
        for(volatile int i = 0; i < 1000; i++);
        vTaskDelayUntil(&last_wake_time,pdMS_TO_TICKS(10));
    }
}


/**
 * 中频任务 - 模拟中等复杂度的数据处理任务
 * 
 * 应用场景：
 * - 数据预处理
 * - 算法计算
 * - 网络通信处理
 * 
 * 特点：
 * - 中等优先级，可被高优先级任务抢占
 * - 处理复杂度适中
 * - 使用vTaskDelay允许其他任务运行
 */
void medium_frequency_task(void *pvParameters){
    for(;;){
        for(volatile int i = 0; i < 5000; i++);
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}


/**
 * 低频任务 - 模拟后台处理任务
 * 
 * 应用场景：
 * - 日志记录
 * - 数据备份
 * - 系统维护
 * - 文件操作
 * 
 * 特点：
 * - 最低优先级，只在其他任务空闲时运行
 * - 处理时间长，但对实时性要求不高
 * - 不影响系统的实时响应性能
 */
void low_frequency_task(void *pvParameters){
    for(;;){
        for(volatile int i = 0; i < 20000; i++);
        vTaskDelay(pdMS_TO_TICKS(200));
    }
}


/**
 * 上下文切换分析任务 - 系统性能监控
 * 
 * 功能：
 * 1. 统计任务切换频率
 * 2. 分析系统调度行为
 * 3. 为性能优化提供数据支持
 * 
 * 使用方法：
 * - 在调试器中设置断点观察变量值
 * - 通过串口输出统计信息
 * - 记录到日志文件进行离线分析
 */
void context_switch_analyzer(void *pvParameters){
    //静态变量：记录上次统计时的切换次数
    static uint32_t last_switch_count=0;
    uint32_t current_switch_count;      //当前切换次数
    uint32_t switches_per_second;       //每秒切换次数

    for(;;){
        //计算本统计周期内的切换次数
        current_switch_count=switch_log_index;
        switches_per_second=current_switch_count-last_switch_count;
        last_switch_count=current_switch_count;

        vTaskDelay(pdMS_TO_TICKS(1000));
    }

}


int main(void){
    xTaskCreate(
        context_switch_analyzer,
        "Analyer",
        256,
        NULL,
        ANALYZER_TASK,
        &Analyzer_Handle
    );

    xTaskCreate(
        high_frequency_task,
        "HighFreq",
        256,
        NULL,
        HIGH_PRIORITY_TASK,
        &High_Priority_Handle
    );

    xTaskCreate(
        medium_frequency_task,
        "MediumFreq",
        256,
        NULL,
        MEDIUM_PRIORITY_TASK,
        &Medium_Priority_Handle
    );

    xTaskCreate(
        low_frequency_task,
        "LowFreq",
        256,
        NULL,
        LOW_PRIORITY_TASK,
        &Low_Priority_Handle
    );

    vTaskStartScheduler();

    for(;;);
}


/**
 * 内存分配失败钩子函数（可选）
 * 
 * 功能说明：
 * - 当堆内存分配失败时，FreeRTOS会调用此函数
 * - 可以在这里实现错误处理和诊断
 * - 帮助调试内存不足问题
 */
void vApplicationMallocFailedHook(void){
    // 内存分配失败时的处理
    // 实际应用中可以：
    // 1. 记录错误日志
    // 2. 发送错误报告
    // 3. 尝试释放不必要的内存
    // 4. 重启系统
    
    // 简单处理：无限循环，便于调试
    for(;;);
}

/**
 * 栈溢出检测钩子函数（可选）
 * 
 * 功能说明：
 * - 当检测到任务栈溢出时，FreeRTOS会调用此函数
 * - 有助于发现栈大小配置不当的问题
 * - 提高系统稳定性
 * 
 * 参数说明：
 * @param xTask: 发生栈溢出的任务句柄
 * @param pcTaskName: 发生栈溢出的任务名称
 */
void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName){
    //栈溢出时的处理
    //pcTaskName指向发生溢出的任务名称
    
    //实际应用中可以：
    //1.输出错误信息和任务名称
    //2.记录调用栈信息
    //3.增加相应任务的栈大小
    //4.分析任务的内存使用模式
    
    //简单处理：无限循环，便于调试
    for(;;);
}

/*
// 空闲任务的静态内存分配
// FreeRTOS要求为空闲任务提供静态内存（当使用静态内存分配时）
static StackType_t idle_task_stack[configMINIMAL_STACK_SIZE];
static TCB_t idle_task_tcb;


空闲任务内存分配回调函数

功能说明：
- FreeRTOS在创建空闲任务时会调用此函数
- 必须提供空闲任务的TCB和栈内存
- 空闲任务负责清理已删除任务的资源

参数说明：
@param ppxIdleTaskTCBBuffer: 返回空闲任务TCB的指针
@param ppxIdleTaskStackBuffer: 返回空闲任务栈的指针  
@param pulIdleTaskStackSize: 返回空闲任务栈大小

void vApplicationGetIdleTaskMemory(TCB_t **ppxIdleTaskTCBBuffer,
                                    StackType **ppxIdleTaskStackBuffer,
                                    uint32_t *pulIdleTaskStackSize){
    *ppxIdleTaskTCBBuffer=&idle_task_tcb;               //任务控制块
    *ppxIdleTaskStackBuffer=idle_task_stack;            //栈内存
    *pulIdleTaskStackSize=configMINIMAL_STACK_SIZE;     //栈大小
}

*/