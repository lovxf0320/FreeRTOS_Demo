/*
 * 练习4：时间片关键函数实现分析
 * 目标：深入理解taskSELECT_HIGHEST_PRIORITY_TASK()和相关函数的实现原理
 * 说明：此代码为FreeRTOS时间片调度机制的演示和分析代码，包含调试和监控功能
 */
#include "FreeRTOS.h"
#include "task.h"
#include "list.h"

/* FreeRTOS核心数据结构声明 */
extern List_t pxReadyTasksLists[configMAX_PRIORITIES];  // 就绪任务列表数组，按优先级组织
extern volatile UBaseType_t uxTopReadyPriority;         // 优先级位图，用于快速查找最高优先级
extern TCB_t * volatile pxCurrentTCB;                   // 当前运行任务的控制块指针

/* 调试用全局变量 */
volatile uint32_t g_task_select_count = 0;            // 记录任务选择次数
volatile uint32_t g_priority_reset_count = 0;         // 记录优先级重置次数
volatile uint8_t g_current_priority_trace[1000];      // 记录优先级选择历史
volatile uint32_t g_trace_index = 0;                  // 优先级跟踪数组索引

/*
 * 宏：taskSELECT_HIGHEST_PRIORITY_TASK()
 * 功能：选择最高优先级的就绪任务，是时间片轮转的核心
 * 实现：结合优先级位图和就绪列表选择下一个执行任务
 */
/* 优先级查找优化宏：根据硬件平台选择不同的实现 */
#if(configUSE_PORT_OPTIMISED_TASK_SELECTION==1)

//ARM Cortex-M优化版本：使用CLZ指令快速找到最高优先级
#define portGET_HIGHEST_PRIORITY(uxTopPriority, uxReadyPriorities)  \
    uxTopPriority=(31UL - (uint32_t)__builtin_clz((uxReadyPriorities)))
#else

/* 通用版本：通过逐位检查查找最高优先级 */
#define portGET_HIGHEST_PRIORITY(uxTopPriority, uxReadyPriorities) \
{ \
    UBaseType_t uxBitNumber; \
    for(uxBitNumber = (UBaseType_t)0U; \
        (uxReadyPriorities & portTOP_BIT_OF_BYTE) == 0; \
        uxBitNumber++) \
    { \
        uxReadyPriorities <<= (UBaseType_t)1U; /* 左移检查下一位 */ \
    } \
    uxTopPriority = (configMAX_PRIORITIES - 1UL) - uxBitNumber; /* 计算优先级 */ \
}

#endif /* configUSE_PORT_OPTIMISED_TASK_SELECTION */


/*
 * 函数：vDebugTaskSelect
 * 功能：调试版的任务选择函数，增加了跟踪和统计功能
 * 参数：无
 * 返回：无
 */
void vDebugTaskSelect(void){
    UBaseType_t uxTopPriority;              //存储找到的最高优先级
    TCB_t *pxPreviousTCB=pxCurrentTCB;      //保存当前任务控制块

    //统计任务选择次数
    g_task_select_count++;

    //第一步：使用优先级位图找到最高优先级
    portGET_HIGHEST_PRIORITY(uxTopPriority, uxTopReadyPriority);

    //记录优先级选择历史
    if(g_trace_index<1000){
        g_current_priority_trace[g_trace_index++]=(uint8_t)uxTopPriority;
    }

    /* 第二步：从最高优先级的就绪列表中获取下一个任务 */
    /* 通过listGET_OWNER_OF_NEXT_ENTRY实现时间片轮转 */
    listGetOwnerOfNextEntry(pxCurrentTCB, &(pxReadyTasksLists[uxTopPriority]));

    //检查是否发生任务切换
    if(pxPreviousTCB != pxCurrentTCB){
        /* 任务切换发生，可在此添加额外调试逻辑 */
    }
}


/*
 * 函数：vDebugGetNextEntry
 * 功能：调试版的链表节点获取函数，实现同优先级任务轮转
 * 参数：ppxTCB - 指向任务控制块指针的指针，pxList - 任务列表
 * 返回：无
 */
void vDebugGetNextEntry(TCB_t **ppxTCB,List_t *pxList){
    List_t* const pxConstList=pxList        //保存列表指针

    //步骤1：移动索引到下一个节点，实现轮转
    pxConstList->pxIndex=pxConstList->pxIndex->pxNext;

    //步骤2：检查是否到达链表末尾标记
    if((void *)pxConstList->pxIndex==(void *)&(pxConstList->xListEnd)){
        //跳过末尾标记，指向第一个任务节点
        pxConstList->pxIndex=pxConstList->pxIndex->pxNext;
    }

    //步骤3：获取当前节点对应的任务控制块
    *ppxTCB=(TCB_t *)pxConstList->pxIndex->pvOwner;
}


/*
 * 函数：vDebugResetReadyPriority
 * 功能：调试版的优先级重置函数，检查并更新优先级位图
 * 参数：uxPriority - 要检查的优先级
 * 返回：无
 */
void vDebugResetReadyPriority(UBaseType_t uxPriority){
    g_priority_reset_count++;       //统计优先级的重置次数

    //检查指定优先级的就绪列表是否为空
    if(listCURREBT_LIST_LENGTH(&(pxReadyTaskLists[uxPriority]))==(UBaseType_t)0){
        portRESET_READY_PRIORITY(uxPriority,uxTopReadyPriority);
        /* 调试输出：记录优先级清除（注释掉） */
        /*
        printf("Priority %d cleared from ready bitmap\n", uxPriority);
        */

    }else{
        /* 列表不为空，保留优先级位图 */
        /* 调试输出：记录剩余任务数量（注释掉） */
        /*
        printf("Priority %d still has %d ready tasks\n", 
               uxPriority, listCURRENT_LIST_LENGTH(&(pxReadyTasksLists[uxPriority])));
        */
    }
}


/*
 * 任务信息结构：用于记录演示任务的统计信息
 */
typedef struct {
    uint8_t task_id;                    // 任务ID
    uint32_t execution_count;           // 执行次数
    TickType_t last_execution_time;     // 上次执行时间
    TickType_t total_execution_time;    // 总执行时间
} DemoTaskInfo_t;

static DemoTaskInfo_t demo_task[3];     //存储三个演示任务的信息


/*
 * 函数：vTimesliceDemo
 * 功能：时间片演示任务，模拟任务执行并记录统计信息
 * 参数：pvParameters - 任务ID
 * 返回：无
 */
void vTimesliceDemo(void *pvParameters){
    uint8_t task_id=(uint8_t)(uintptr_t)pvParameters;
    TickType_t start_time,end_time;

    demo_task[task_id].task_id=task_id;
    demo_task[task_id].execution_count=0;
    demo_task[task_id].total_execution_time=0;

    for(;;){
        start_time=xTaskGetTickCount();

        /* 模拟工作负载：执行空操作循环 */
        volatile uint32_t work = 10000;
        while(work--) 
        {
            __NOP(); // 空操作，模拟CPU工作
        }

        end_time=xTaskGetTickCount();

        demo_task[task_id].execution_count++;
        demo_task[task_id].last_execution_time=end_time-start_time;
        demo_task[task_id].total_execution_time+=(end_time-start_time);
    }
}


/*
 * 函数：vDemonstrateListTraversal
 * 功能：演示就绪任务列表的遍历过程
 * 参数：无
 * 返回：无
 */
void vDemonstrateListTraversal(void){
    UBaseType_t priority=2;                     //固定观察优先级2的列表
    List_t *pxList=&pxReadyTaskList[priority];  //获取列表
    ListItem_t *pxIterator;                     //遍历迭代器
    uint32_t task_count=0;                      //任务计数

    printf("\n=== 优先级%d任务列表遍历演示 ===\n",priority);

    //检查列表是否为空
    if(listLIST_IS_EMPTY(pxLIST)==pdTRUE){
        printf("该优先级下没有就绪任务\n");
        return;
    }

    //从列表头部开始遍历
    pxIterator=listGET_HEAD_ENTRY(pxList);

    do{
        TCB_t *pxTCB=(TCB_t *)listGET_LIST_ITEM_OWNER(pxIterator);
        printf("任务%d:%s\n",task_count,pxTCB->pcTaskName);

        pxIterator=listGET_NEXT(pxIterator);
        task_count++;
    }while((pxIterator != &(pxList->xListEnd)) && (task_count < 10));

    /* 打印当前索引和下次调度的任务 */
    printf("当前索引指向的任务: %s\n", 
           ((TCB_t *)pxList->pxIndex->pvOwner)->pcTaskName);
    printf("下次调度将选择的任务: %s\n",
           ((TCB_t *)pxList->pxIndex->pxNext->pvOwner)->pcTaskName);
    printf("===============================\n");
}


/*
 * 函数：vDemonstratePriorityBitmap
 * 功能：显示优先级位图的状态
 * 参数：无
 * 返回：无
 */
void vDemonstratePriorityBitmap(void){
    printf("\n=== 优先级位图状态 ===\n");
    printf("当前位图值: 0x%08X\n", (unsigned int)uxTopReadyPriority);

    for(int i=configMAX_PRIORITIES-1;i>=0;i--){
        if(uxTopReadyPriority&(1UL<<i)){
            printf("优先级%d: 有就绪任务 (%d个)\n", 
                   i, listCURRENT_LIST_LENGTH(&pxReadyTasksLists[i]));
        }
    }
    printf("==================\n");
}


/*
 * 函数：vSystemMonitor
 * 功能：系统监控任务，周期性输出系统状态
 * 参数：pvParameters - 未使用
 * 返回：无
 */
void vSystemMonitor(void *pvParameters){
    TickType_t xLastWakeTime;
    const TickType_t xFrenquency=pdMS_TO_TICKS(5000);

    xLastWakeTime=xTaskGetTickCount();

    for(;;){
        vTaskDelayUntil(xLastWakeTime,xFrenquency);

        printf("\n=== 系统监控报告 ===\n");
        printf("任务选择次数: %d\n", g_task_select_count);
        printf("优先级重置次数: %d\n", g_priority_reset_count);

        //输出每个演示任务的执行统计
        for(int i = 0; i < 3; i++)
        {
            if(demo_tasks[i].execution_count > 0)
            {
                printf("演示任务%d: 执行%d次, 平均时间%d ticks\n",
                       i, demo_tasks[i].execution_count,
                       demo_tasks[i].total_execution_time / demo_tasks[i].execution_count);
            }
        }
        /* 显示就绪列表和优先级位图状态 */
        vDemonstrateListTraversal();
        vDemonstratePriorityBitmap();
        
        printf("==================\n");
    }
}


/*
 * 函数：main
 * 功能：程序入口，初始化任务并启动调度器
 * 参数：无
 * 返回：无（永不返回）
 */
int main(void){
    /* 创建三个同优先级（优先级2）的演示任务 */
    for(int i = 0; i < 3; i++)
    {
        char task_name[16];
        snprintf(task_name, sizeof(task_name), "Demo%d", i); // 生成任务名称
        xTaskCreate(vTimesliceDemo, task_name, configMINIMAL_STACK_SIZE,
                    (void*)(uintptr_t)i, 2, NULL); // 创建任务
    }
    
    /* 创建系统监控任务（优先级3，高于演示任务） */
    xTaskCreate(vSystemMonitor, "Monitor", configMINIMAL_STACK_SIZE,
                NULL, 3, NULL);
    
    /* 启动FreeRTOS调度器 */
    vTaskStartScheduler();
    
    /* 调度器启动后永不返回 */
    for(;;);
}


/*
 * 核心概念总结：
 * 
 * 1. 时间片轮转的实现原理：
 *    - 每个优先级维护一个双向链表存储同优先级的任务
 *    - pxIndex指针在链表中循环移动，指向下一个要执行的任务
 *    - 每次任务切换时，pxIndex自动指向下一个任务，实现轮转
 * 
 * 2. 关键数据结构：
 *    - pxReadyTasksLists[]: 各优先级的就绪任务链表数组
 *    - uxTopReadyPriority: 优先级位图，快速定位最高优先级
 *    - pxIndex: 每个链表的遍历指针，实现轮转的关键
 * 
 * 3. 时间片切换时机：
 *    - SysTick中断中检查是否需要任务切换
 *    - 同优先级下有多个就绪任务时触发切换
 *    - 每次切换时间固定为1个tick
 * 
 * 4. 公平性保证：
 *    - 链表的循环特性保证每个任务都有机会执行
 *    - pxIndex的移动保证任务按顺序轮转执行
 *    - 没有饥饿现象，每个任务都能分到时间片
 * 
 * 实验建议：
 * 1. 单步调试观察pxIndex指针的移动
 * 2. 监控uxTopReadyPriority位图的变化
 * 3. 分析任务执行的时间分布是否均匀
 * 4. 测试不同数量同优先级任务的调度效果
 */