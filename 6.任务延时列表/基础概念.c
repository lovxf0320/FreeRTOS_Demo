// FreeRTOS 延时机制练习代码
#include <stdint.h>
#include <stdio.h>

// 任务结构体
typedef struct Task {
    uint32_t xWakeTime;        // 任务唤醒的绝对时间
    struct Task* pxNext;       // 指向列表中下一个任务的指针（简化的单链表）
    char* pcName;              // 任务名称，用于调试
    int bReady;                // 1 表示任务就绪，0 表示延时中
} Task_t;


// 简化的列表结构体（单链表）
typedef struct {
    Task_t* pxHead;            // 列表头部指针
} List_t;


//全局变量
volatile uint32_t xTickCount=0;             //当前系统tick计数
volatile uint32_t xNextTaskUnblockTime=0;   //下一次任务唤醒时间
List_t pxDelayedTaskList={NULL};            //正常延时列表
List_t pxOverflowDelayedTaskList={NULL};    //溢出延时列表
Task_t* pxCurrentTask=NULL;                 //当前任务指针


//就绪列表（用于存放已唤醒的任务）
List_t pxReadyList={NULL};


//将任务插入列表，按 xWakeTime 升序排列（最早唤醒的任务在头部）
void vListInsert(List_t* pxList, Task_t* pxTask){
    Task_t* pxCurrent=pxList->pxHead;
    Task_t* pxPrev=NULL;

    if(pxCurrent==NULL||pxTask->xWakeTime < pxCurrent->xWakeTime){
        pxTask->pxNext=pxCurrent;
        pxList->pxHead=pxTask;
    }else{
        while(pxCurrent!=NULL&&pxCurrent->xWakeTime<=pxTask->xWakeTime){
            pxCurrent=pxCurrent->pxNext;
            pxPrev=pxCurrent;
        }
        pxTask->pxNext=pxTask;
        pxPrev->pxNext=pxTask;
    }
}


// 从列表中移除指定任务
void vListRemove(List_t* pxList, Task_t* pxTask){
    Task_t* pxCurrent=pxList->pxHead;
    Task_t* pxPrev=NULL;

    while(pxCurrent!=NULL&&pxCurrent!=pxTask){
        pxPrev=pxCurrent;
        pxCurrent=pxCurrent->pxNext;
    }

    if(pxCurrent==pxTask){
        if(pxPrev==NULL){
            pxList->pxHead=pxCurrent->pxNext;
        }else{
            pxPrev->pxNext=pxCurrent->pxNext;
        }
        pxTask->pxNext=NULL;
    }
}


// 使当前任务延时指定tick数
void vTaskDelay(uint32_t xTicksToDelay){
    pxCurrentTask->bReady=0;    //标记任务为延时状态
    uint32_t xTimeToWake=xTickCount+xTicksToDelay;

    //检查是否发生溢出并插入到相应列表
    pxCurrentTask->xWakeTime=xTimeToWake;
    if(xTimeToWake<xTickCount){
        //发生溢出，放入溢出延时列表
        vListInsert(&pxOverflowDelayedTaskList, pxCurrentTask);
    }else{
        // 正常情况，放入正常延时列表
        vListInsert(&pxDelayedTaskList, pxCurrentTask);
    }

    //更新 xNextTaskUnblockTime
    if(xNextTaskUnblockTime==0||xTimeToWake<xNextTaskUnblockTime){
        xNextTaskUnblockTime=xTimeToWake;
    }

    vTaskSwitchContext();
}


// 检查并唤醒延时任务
void vTaskCheckDelayedTasks(void){
    Task_t* pxTask;

    //检查正常延时列表中 xWakeTime <= xTickCount 的任务
    while(pxDelayedTaskList.pxHead&&pxDelayedTaskList.pxHead->xWakeTime<=xTickCount){
        pxTask=pxDelayedTaskList.pxHead;
        vListRemove(&pxDelayedTaskList,pxTask);
        pxTask->bReady=1;
        vListInsert(&pxReadyList,pxTask);
    }

    //更新xNextTaskUnblockTime
    if(pxDelayedTaskList.pxHead){
        //正常延时列表不为空，取头部任务的唤醒时间
        xNextTaskUnblockTime=pxDelayedTaskList.pxHead->xWakeTime;

    }else if(pxOverflowDelayedTaskList.pxHead){
        //正常列表为空但溢出列表不为空，取溢出列表头部任务的唤醒时间
        xNextTaskUnblockTime=pxOverflowDelayedTaskList.pxHead->xWakeTime;

    }else{
        //两个列表都为空，设置为最大值
        xNextTaskUnblockTime=UINT32_MAX;
    }
}


// 简化的任务切换（选择第一个就绪任务）
void vTaskSwitchContext(void){
    if(pxReadyList.pxHead!=NULL){
        pxCurrentTask=pxReadyList.pxHead;
        vListRemove(&pxReadyList,pxCurrentTask);
    }else{
        pxCurrentTask=NULL;
    }
}


// SysTick 中断处理函数
// SysTick_Handler 是对 xTaskIncrementTick() 的简化模拟：
void SysTick_Handler(void){
    xTickCount++;

    //处理 xTickCount 溢出（当其变为 0 时）
    if(xTickCount==0){
        //交换正常延时列表和溢出延时列表
        List_t* pxTemp=pxDelayedTaskList.pxHead;
        pxDelayedTaskList.pxHead=pxOverflowDelayedTaskList.pxHead;
        pxOverflowDelayedTaskList.pxHead=pxTemp;

        //更新 xNextTaskUnblockTime
        if(pxDelayedTaskList.pxHead){
            xNextTaskUnblockTime=pxDelayedTaskList.pxHead->xWakeTime;
        }else if(pxOverflowDelayedTaskList.pxHead){
            xNextTaskUnblockTime=pxOverflowDelayedTaskList.pxHead->xWakeTime;
        }else{
            xNextTaskUnblockTime=UINT32_MAX;
        }
    }

    if(xTickCount>=xNextTaskUnblockTime){
        vTaskCheckDelayedTasks();
    }
}


// 打印任务状态，用于调试
void vPrintTaskStatus(void){
    printf("当前 Tick: %u, 下次唤醒: %u\n", xTickCount, xNextTaskUnblockTime);
    printf("延时列表: ");

    Task_t* pxTask=pxDelayedTaskList.pxHead;
    while(pxTask){
        printf("%s(%u)->",pxTask->pcName,pxTask->xWakeTime);
        pxTask=pxTask->pxNext;
    }

    printf("NULL\n溢出延时列表: ");
    pxTask=pxOverflowDelayedTaskList.pxHead;
    while(pxTask){
        printf("%s(%u)->",pxTask->pcName,pxTask->xWakeTime);
        pxTask=pxTask->pxNext;
    }

    printf("NULL\n就绪列表: ");
    pxTask=pxReadyList.pxHead;
    while(pxTask){
        printf("%s -> ", pxTask->pcName);
        pxTask=pxTask->pxNext;
    }

    printf("NULL\n当前任务: %s\n\n", pxCurrentTask ? pxCurrentTask->pcName : "无");
}
