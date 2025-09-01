/*
 * Demo5: SysTick系统时基原理深入理解
 * 
 * 学习目标：
 * 1. 理解SysTick作为系统心跳的作用
 * 2. 观察xTaskIncrementTick()如何更新延时计数
 * 3. 理解系统时基与任务延时的关系
 * 4. 学习时间转换宏的使用
 */
#include "FreeRTOS.h"
#include "task.h"


//全局变量
// 系统tick计数器，记录SysTick中断发生次数
volatile uint32_t systicks_count=0;

// 任务延时更新计数器，用于统计延时任务的更新频率
volatile uint32_t task_delay_upload=0;


//任务延时信息结构体
typedef struct{
    char name[20];
    uint32_t delay_start_tick;      // 延时开始时的tick计数
    uint32_t delay_duration_ms;     // 延时持续时间（毫秒）
    uint32_t remaining_ticks;       // 剩余的tick数
    bool is_delaying;               // 是否正在延时状态
}TaskDelayInfo_t;

TaskDelayInfo_t delay_info[3];

/**
 * 自定义的延时函数，带调试信息输出
 * 
 * @param delay_ms: 延时时间（毫秒）
 * @param task_id: 任务ID，用于索引delay_info数组
 * 
 * 功能：
 * 1. 记录延时开始时间和参数
 * 2. 显示延时开始信息
 * 3. 执行实际延时
 * 4. 显示延时结束信息和实际用时
 * 
 * 理解延时机制的关键：
 * 如何记录延时开始时间
 * 如何进行毫秒到tick的转换
 * 如何跟踪延时状态
 * 如何计算实际延时时间
 */
// 自定义的延时函数，带调试信息
void custom_task_delay(uint32_t delay_ms,int task_id){
    delay_info[task_id].delay_start_tick=xTaskGetTickCount();
    delay_info[task_id].delay_duration_ms=delay_ms;
    delay_info[task_id].remaining_ticks=pdMS_TO_TICKS(delay_ms);
    delay_info[task_id].is_delaying=true;

    printf("[%s] 开始延时 %u ms (等于 %u ticks)\n",
           delay_info[task_id].name,
           delay_ms,
           pdMS_TO_TICKS(delay_ms));

    vTaskDelay(pdMS_TO_TICKS(delay_ms));

    delay_info[task_id].is_delaying=false;

    printf("[%s] 延时结束，实际用时 %u ticks\n",
           delay_info[task_id].name,
           xTaskGetTickCount() - delay_info[task_id].delay_start_tick);

}


void PrecisionDelayTask(void *pvParameters){
// 设置任务名称，用于调试输出
    strcpy(delay_info[0].name, "精确延时任务");
    
    // 任务主循环
    while(1)
    {
        printf("\n[精确延时任务] === 开始精确延时演示 ===\n");
        
        // 演示1：100毫秒延时
        printf("[精确延时任务] 演示100ms延时:\n");
        uint32_t start_tick = xTaskGetTickCount(); // 记录开始时间（用于对比）
        custom_task_delay(100, 0);
        
        // 演示2：1秒延时
        printf("[精确延时任务] 演示1秒延时:\n");
        custom_task_delay(1000, 0);
        
        // 演示3：1.5秒延时
        printf("[精确延时任务] 演示1.5秒延时:\n");
        custom_task_delay(1500, 0);
        
        // 演示4：时间转换宏的使用
        printf("[精确延时任务] 时间转换宏演示:\n");
        printf("    pdMS_TO_TICKS(1000) = %u ticks\n", pdMS_TO_TICKS(1000)); // 1秒对应的tick数
        printf("    pdMS_TO_TICKS(100) = %u ticks\n", pdMS_TO_TICKS(100));   // 100ms对应的tick数
        printf("    pdMS_TO_TICKS(1) = %u ticks\n", pdMS_TO_TICKS(1));       // 1ms对应的tick数
        
        printf("[精确延时任务] === 精确延时演示结束 ===\n\n");
        
        // 等待5秒后重复演示循环
        custom_task_delay(5000, 0);
    }
}


void MultiDelayTask(void *pvParameters){
    // 设置任务名称
    strcpy(delay_info[1].name, "多重延时任务");
    uint32_t cycle = 0; // 周期计数器
    
    // 任务主循环
    while(1)
    {
        cycle++; // 增加周期计数
        printf("\n[多重延时任务] 周期 %u 开始\n", cycle);
        
        // 连续执行5次200ms的短延时
        // 模拟需要间隔执行的操作（如传感器采样、LED闪烁等）
        for(int i = 1; i <= 5; i++)
        {
            printf("[多重延时任务] 第 %d 次短延时 (200ms)\n", i);
            custom_task_delay(200, 1); // 每次延时200ms
        }
        
        printf("[多重延时任务] 周期 %u 结束，长延时 (3秒)\n", cycle);
        // 周期结束后进行较长延时，模拟两个工作周期之间的等待
        custom_task_delay(3000, 1);
    }
}


/**
 * 时基监控任务
 * 
 * 功能：
 * 1. 定期输出系统时基状态信息
 * 2. 监控各任务的延时状态
 * 3. 显示系统配置参数
 * 4. 计算系统运行时间
 */
void TickMonitorTask(void *pvParameters){
    //设置任务名称
    strcpy(delay_info[2].name,"时基监控任务");
    uint32_t last_tick_count=0;     //上次检查时的tick计数

    //任务主循环
    while(1){
        //获取当前系统tick计数
        uint32_t current_tick=xTaskGetTickCount();

        //计算距离上次检查经过的tick数
        uint32_t elapsed_ticks=current_tick-last_tick_count;
        last_tick_count=current_tick;

        // 输出监控报告标题
        printf("\n" "="*60 "\n");
        printf("=== SysTick 系统时基监控报告 ===\n");

        //显示基本时间信息
        printf("当前系统tick计数: %u\n",current_tick);
        printf("距离上次报告经过: %u ticks (%u ms)\n",
                elapsed_ticks,
                elapsed_ticks*portTICK_PERIOD_MS
        );      //将tick转换为毫秒
        printf("系统运行时间：%u秒\n",current_tick/configTICK_RATE_HZ); // 计算总运行时间


        //显示系统时基配置信息
        printf("\n系统时基配置:\n");
        printf("configTICK_RATE_HZ = %u Hz\n", configTICK_RATE_HZ);     // SysTick中断频率
        printf("portTICK_PERIOD_MS = %u ms\n", portTICK_PERIOD_MS);     // 每个tick的时间周期
        printf("每秒产生 %u 次 SysTick 中断\n", configTICK_RATE_HZ);

        //显示所有任务的当前延时状态
        printf("\n当前任务延时状态:\n");

        for(int i=0;i<3;i++){
            //检查任务名称是否已设置
            if(strlen(delay_info[i].name)>0){
                printf("%s:%s\n",
                    delay_info[i].name,
                    delay_info[i].is_delaying?"延时中":"运行中");

                //如果任务正在延时，显示已延时的时间
                if(delay_info[i].is_delaying){
                    uint32_t elapsed=current_tick-delay_info[i].delay_start_tick;
                    printf("已延时: %u ticks (%u ms)\n", 
                           elapsed, elapsed * portTICK_PERIOD_MS);
                }
            }
        }

        printf("=" "="*59 "\n\n");
        
        // 每8秒进行一次监控报告
        custom_task_delay(8000, 2);
    }
}


/**
 * SysTick中断钩子函数（模拟实现）
 * 
 * 说明：
 * - 这个函数在每次SysTick中断时被调用
 * - 在实际系统中，FreeRTOS的xTaskIncrementTick()也在此时被调用
 * - 用于统计系统运行状态
 */
void vApplicationTickHook(void){
    //每次SysTick中断时增加计数器
    systicks_count++;

    //每1000个tick（通常是1秒）统计一次任务延时更新
    if(systicks_count%1000==0){
        task_delay_upload++;
    }
}

/**
 * 延时精度演示任务（一次性执行）
 * 
 * 功能：
 * 1. 测试最小延时精度（1个tick）
 * 2. 测试不同毫秒级延时的精度
 * 3. 分析延时误差
 * 4. 完成后自动删除任务
 */
void DelayPrecisionDemo(void *pvParameters){
    printf("=== 延时精度演示 ===\n");

    // 测试1：最小延时精度测试
    printf("测试最小延时精度...\n");
    
    // 连续测试5次1个tick的延时
    for(int i=0;i<5;i++){
        uint32_t start=xTaskGetTickCount(); //记录开始时间
        vTaskDelay(1);
        uint32_t end=xTaskGetTickCount(); //记录结束时间

        printf("延时1 tick，实际用时: %u ticks (%u ms)\n", 
               end - start, (end - start) * portTICK_PERIOD_MS);
    }

    printf("\n测试毫秒级延时精度...\n");

    // 测试2：不同毫秒级延时的精度测试
    uint32_t test_delays[]={1,5,10,15,50,100};  // 测试的延时时间（毫秒）
    int num_tests=sizeof(test_delays)/sizeof(test_delays[0]);

    for(int i=0;i<num_tests;i++){
        uint32_t delay_ms=test_delays[i];       //当前测试的延时时间
        uint32_t expected_ticks=pdMS_TO_TICKS(delay_ms);    //期望的tick数

        uint32_t start=xTaskGetTickCount();     //记录开始时间
        vTaskDelay(pdMS_TO_TICKS(delay_ms));    //执行延时
        uint32_t end=xTaskGetTickCount();       //记录结束时间

        // 计算误差：实际用时 - 期望用时
        printf("延时 %u ms (期望 %u ticks)，实际用时: %u ticks，误差: %d ticks\n",
               delay_ms, expected_ticks, end - start, (int)(end - start) - (int)expected_ticks);

    }

    // 测试完成，删除任务自己
    printf("=== 延时精度演示完成 ===\n\n");
    vTaskDelete(NULL);
}


int main(void){
    printf("=== FreeRTOS SysTick 系统时基原理演示 ===\n\n");
    
    printf("本演示将展示:\n");
    printf("1. SysTick如何作为系统时间基准\n");
    printf("2. 任务延时如何基于系统tick计算\n");
    printf("3. 不同延时时间的精度表现\n");
    printf("4. 系统时基的配置参数\n\n");
    
    printf("系统配置:\n");
    printf("- SysTick频率: %u Hz\n", configTICK_RATE_HZ);
    printf("- 每个tick周期: %u ms\n", portTICK_PERIOD_MS);
    printf("- 最小延时精度: %u ms\n", portTICK_PERIOD_MS);
    printf("\n");

    //创建演示任务
    xTaskCreate(PrecisionDelayTask,"PrecisionDelay",2000,NULL,2,NULL);
    xTaskCreate(MultiDelayTask,"MultiDelay",2000,NULL,1,NULL);
    xTaskCreate(TickMonitorTask,"TickMonitor",2000,NULL,3,NULL);

    //创建一次性精度测试函数
    xTaskCreate(DelayPrecisionDemo,"DelayPrecision",2000,NULL,4,NULL);

    printf("启动调度器，开始SysTick演示...\n\n");

    // 启动调度器
    vTaskStartScheduler();
    
    printf("调度器启动失败！\n");
    while(1);
}

/*
 * ================== 重要学习要点 ==================
 * 
 * 1. SysTick中断机制：
 *    - SysTick是ARM Cortex-M内核的系统定时器
 *    - 定期产生中断，形成系统"心跳"
 *    - 中断频率由configTICK_RATE_HZ配置（通常1000Hz = 1ms）
 *    - 每次中断调用xTaskIncrementTick()更新系统时间
 * 
 * 2. 时间单位转换：
 *    - Tick：系统最小时间单位，由SysTick中断产生
 *    - pdMS_TO_TICKS(ms)：将毫秒转换为tick数
 *    - portTICK_PERIOD_MS：每个tick对应的毫秒数
 *    - xTaskGetTickCount()：获取当前系统tick计数
 * 
 * 3. 延时函数原理：
 *    - vTaskDelay(n)：延时n个tick
 *    - 任务进入阻塞状态，让出CPU
 *    - 系统在每个SysTick中断中检查延时是否到期
 *    - 到期后将任务从阻塞状态移到就绪状态
 * 
 * 4. 延时精度限制：
 *    - 最小延时单位是1个tick
 *    - 小于1个tick的延时会被向上取整
 *    - 例如：tick周期=10ms时，延时5ms实际会延时10ms
 *    - 延时精度 = portTICK_PERIOD_MS
 * 
 * 5. 系统开销考虑：
 *    - 每个SysTick中断都有处理开销
 *    - 频率越高，精度越好，但开销也越大
 *    - 需要在精度和性能之间找到平衡
 *    - 大量任务同时延时会增加系统负担
 * 
 * 6. 实际应用建议：
 *    - 根据应用需求选择合适的tick频率
 *    - 避免过短的延时（如小于几个tick）
 *    - 使用vTaskDelayUntil()实现周期性任务
 *    - 考虑使用软件定时器处理精确定时需求
 */