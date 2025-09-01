// 题目4：链表末尾插入
// 任务描述：创建一个链表，
// 使用末尾插入方式添加几个相同优先级的任务项，理解FIFO（先进先出）的特性。

// 链表结构：FreeRTOS使用双向环形链表，xListEnd作为哨兵节点
// FIFO特性：vListInsertEnd()确保后插入的项目排在前面插入项目的后面
// 相同优先级处理：当任务优先级相同时，按插入顺序排列


#include <stdio.h>
#include <stdlib.h>
#include "list.h"
#include <string.h>

// FreeRTOS链表相关定义（简化版）
typedef struct xLIST_ITEM{
    TickType_t xItemValue; // 列表项的值（模拟优先级）
    struct xLIST_ITEM *pxNext; // 指向下一个项
    struct xLIST_ITEM *pxPrevious;
    void *pvOwner;
    struct xLIST *pxContainer;
}ListItem_t

typedef struct xLIST{
    UBaseType_t uxNumberOfItems;
    ListItem_t *pxIndex;       // 用于遍历的指针
    // pxIndex是FreeRTOS调度器用来实现时间片轮转（Round Robin）的关键指针。
    // 它记录了当前正在运行的任务在链表中的位置。
    ListItem_t xListEnd;        //列表末尾的标志项
}List_t;


// 基本类型定义
typedef unsigned long UBaseType_t;
typedef unsigned long TickType_t;


// 宏定义
#define listGET_HEAD_ENTRY(pxList)          (((pxList)->xListEnd).pxNext)
#define listGET_NEXT(pxListItem)            (((pxListItem)->pxNext))
#define listGET_LIST_ITEM_VALUE(pxListItem) (((pxListItem)->xItemValue))
#define listGET_LIST_ITEM_OWNER(pxListItem) ((pxListItem)->pvOwner)


// 任务结构体（简化版）
typedef struct{
    char taskName[20];
    int taskID;
    int priority;
}Task_t;


// 函数声明
//vListInitialise\vListInitialiseItem\vListInsertEnd这几个是RTOS链表API的函数
//printList和createTask是我们自定义的函数
void vListInitialise(List_t* const pxList);
void vListInitialiseItem(ListItem_t* const pxItem);
void vListInsertEnd(List_t const* pxList,ListItem_t const* pxNewListItem);
void printList(List_t* const pxList);
Task_t* createTask(const char* taskName, int taskID, int priority);


// 初始化链表
void vListInitialise(List_t* const pxList){
    //列表的哨兵节点指向自己
    pxList->xListEnd.pxNext=&(pxList->xListEnd);
    pxList->xListEnd.pxPrevious=&(pxList->xListEnd);

    //哨兵节点的值设为最大，让其保持在最后
    pxList->xListEnd.xItemValue=(TickType_t)0xFFFFFFFF;

    //列表内的列表项数量为0
    pxList->uxNumberOfItems=0;

    //初始化索引指针要指向最后一个节点
    pxList->pxIndex=&(pxList->xListEnd);
}


// 初始化列表项
void vListInitialiseItem(ListItem_t* const pxItem){
    pxItem->xItemValue=0;
    pxItem->pxNext=NULL;
    pxItem->pxPrevious=NULL;
    pxItem->pxContainer=NULL;
    pxItem->pvOwner=NULL;
}


// 在列表末尾插入项目
void vListInsertEnd(List_t const* pxList,ListItem_t const* pxNewListItem){
    ListItem_t const* pxIndex = pxList->pxIndex;

    pxNewListItem->pxNext=pxIndex;
    pxNewListItem->pxPrevious=pxIndex->pxPrevious;
    pxIndex->pxPrevious->pxNext=pxNewListItem;
    pxIndex->pxPrevious=pxNewListItem;

    pxNewListItem->pxContainer=pxList;

    (pxList->uxNumberOfItems)++
}


// 创建任务
Task_t* createTask(const char* taskName, int taskID, int priority){
    Task_t* task=(Task_t*)malloc(sizeof(Task_t));

    if(task){
        strncpy(task->taskName,taskName,sizeof(task->taskName)-1);
        task->taskName[sizeof(task->taskName)-1]='\0';
        task->taskID=taskID;
        task->priority=priority;
    }

    return task;
}


// 打印链表内容
void printList(List_t* const pxList){
    printf("=== 链表内容 (共%lu项) ===\n", pxList->uxNumberOfItems);

    if(pxList->uxNumberOfItems==0){
        printf("链表为空\n");
        return;
    }

    ListItem_t *pxIterator=listGET_HEAD_ENTRY(&pxList);
    int index=1;

    while(pxIterator!=&(pxList->xListEnd)){
        Task_t *task=(Task_t*)listGET_LIST_ITEM_OWNER(pxIterator)
        if(task){
            printf("%d. 任务: %s, ID: %d, 优先级: %d\n",
                    index,task->taskName,task->taskID,task->priority);
            
        }

        pxIterator=listGET_NEXT(pxIterator);
    }
    printf("\n");
}


//演示调度器轮询过程
void demostrateIndexUsage(LIst_t* const pxList){
    printf("=== pxIndex轮询机制演示 ===\n");
    printf("当前pxIndex指向: %s\n",
            (pxList->pxIndex)==&(pxList->xListEnd)?"列表末尾":"列表中的某一项");
    
    // 模拟调度器的轮询过程
    printf("\n模拟时间片轮转调度过程：\n");
    for (int i = 0; i < 6; i++) {
        // 移动到下一个任务
        pxList->pxIndex = pxList->pxIndex->pxNext;
        
        if (pxList->pxIndex == &(pxList->xListEnd)) {
            printf("第%d次轮询: 跳过列表末尾标记，继续到下一个\n", i + 1);
            pxList->pxIndex = pxList->pxIndex->pxNext;
        }
        
        Task_t* currentTask = (Task_t*)pxList->pxIndex->pvOwner;
        printf("第%d次轮询: 当前运行任务 -> %s\n", i + 1, currentTask->taskName);
    }
    printf("\n");
}

int main(){
    printf("=== FreeRTOS链表末尾插入示例 ===\n\n");

    //1.创建并初始化链表
    List_t taskList;
    vListInitialise(&taskList);
    printf("1. 初始化链表完成\n");
    printf("pxIndex初始化指向: %s\n", 
           (taskList.pxIndex == &(taskList.xListEnd)) ? "列表末尾标记" : "某个任务");
    printf("   原因: 确保第一次调度时能正确找到第一个任务\n\n");

    //2. 创建相同优先级的任务
    Task_t* task1=createTask("LED_Task",1,1);
    Task_t* task2=createTask("UART_Task",2,1);
    Task_t* task3=createTask("SPI_Task",3,1);
    Task_t* task4=createTask("TINER_Task",4,1);

    //3. 创建并初始化列表项
    ListItem_t ListItem1,ListItem2,ListItem3,ListItem4;
    vListInitialiseItem(&ListItem1);
    vListInitialiseItem(&ListItem2);
    vListInitialiseItem(&ListItem3);
    vListInitialiseItem(&ListItem4);

    //4. 设置列表项的值和所有值
    ListItem1.xItemValue=task1->priority;
    ListItem1.pvOwner=task1;

    ListItem2.xItemValue=task2->priority;
    ListItem2.pvOwner=task2;

    ListItem3.xItemValue=task3->priority;
    ListItem3.pvOwner=task3;

    ListItem4.xItemValue=task4->priority;
    ListItem4.pvOwner=task4;

    //5. 末尾插入
    printf("插入任务1：%s\n",task1->taskName);
    vListInsertEnd(&taskList,&ListItem1);
    printList(&taskList);

    printf("插入任务2：%s\n",task2->taskName);
    vListInsertEnd(&taskList,&ListItem2);
    printList(&taskList);

    printf("插入任务3：%s\n",task3->taskName);
    vListInsertEnd(&taskList,&ListItem3);
    printList(&taskList);

    printf("插入任务3：%s\n",task4->taskName);
    vListInsertEnd(&taskList,&ListItem3);
    printList(&taskList);

    //6. 验证FIFO特性
    printf("   插入顺序：LED_Task -> UART_Task -> SPI_Task -> Timer_Task\n");
    printf("   链表顺序：");

    ListItem_t* pxIterator=listGET_HEAD_ENTRY(&taskList);
    while(pxIterator!=&(taskList.xListEND)){
        Task_t* task=(Task_t*)listGET_LIST_ITEM_OWNER(pxIterator);
        if(task){
            printf("%s",task->taskName);
            if(listGET_NEXT(pxIterator)!=&(taskList.xListEND)){
                printf("——>");
            }
        }
        pxIterator=listGET_NEXT(pxIterator);
    }
    printf("\n");

    //7. 展示环形结构
    ListItem_t* head = listGET_HEAD_ENTRY(&taskList);
    if (head != &(taskList.xListEnd)) {
        Task_t* firstTask = (Task_t*)listGET_LIST_ITEM_OWNER(head);
        printf("   第一个任务：%s\n", firstTask->taskName);
        
        // 找到最后一个任务
        ListItem_t* last = taskList.xListEnd.pxPrevious;
        if (last != &(taskList.xListEnd)) {
            Task_t* lastTask = (Task_t*)listGET_LIST_ITEM_OWNER(last);
            printf("   最后一个任务：%s\n", lastTask->taskName);
            printf("   最后一个任务的下一个指向：%s\n", 
                   (last->pxNext == &(taskList.xListEnd)) ? "列表末尾标记" : "其他");
            printf("   列表末尾标记的下一个指向：%s\n",
                   (taskList.xListEnd.pxNext == head) ? "第一个任务" : "其他");
        }
    }

    //8. 演示轮询机制
    demostrateIndexUsage(&taskList);

    //释放内存
    free(task1);
    free(task2);
    free(task3);
    free(task4);

    return 0;
}