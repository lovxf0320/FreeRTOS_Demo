// 题目3：链表项的所有者设置 任务描述：创建一个链表和几个模拟的任务控制块，
// 将链表项与任务控制块关联，实现通过链表项找到对应任务的功能。
// 可能用到的API： 
// vListInitialise()
// vListInitialiseItem()
// vListInsert()
// listSET_LIST_ITEM_OWNER()
// listGET_LIST_ITEM_OWNER()
// listSET_LIST_ITEM_VALUE()

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "list.h"

typedef struct{
    char taskName[20];
    int taskID;
    int currentpriority;
    ListItem_t stateListItem;
    ListItem_t eventListItem;
}TaskControlBlock_t;


TaskControlBlock_t* create_Task(char* task, int id, int priority){
    TaskControlBlock_t *newTask = (TaskControlBlock_t *)malloc(sizeof(TaskControlBlock_t));

    strcpy(newTask->taskName, task);
    newTask->taskID = id;
    newTask->currentpriority = priority;

    // 初始化链表项
    vListInitialiseItem(&newTask->stateListItem);
    vListInitialiseItem(&newTask->eventListItem);

    //设置链表项许哦属的任务为当前任务
    listSET_LIST_ITEM_OWNER(&newTask->stateListItem, newTask);
    listSET_LIST_ITEM_OWNER(&newTask->eventListItem, newTask);

    // 设置链表项的值为当前优先级
    listSET_LIST_ITEM_VALUE(&newTask->eventListItem, priority)

    return newTask;
}


int main(void){
    printf("=== FreeRTOS 链表API使用示例 ===\n\n");

    //1. 创建并初始化链表
    struct xList ReadyList;
    vListInitialise(&ReadyList);

    //2. 创建任务控制块
    TaskControlBlock_t* task1=create_Task("Task1",1,3);
    TaskControlBlock_t* task2=create_Task("Task2",2,1);
    TaskControlBlock_t* task3=create_Task("Task3",3,2);

    //3. 将任务控制块的链表项插入到链表中
    vListInsert(&ReadyList, &task1->stateListItem);
    vListInsert(&ReadyList, &task2->stateListItem);
    vListInsert(&ReadyList, &task3->stateListItem);

    //4. 遍历链表，输出任务信息
    ListItem_t *pxIterator = listGET_HEAD_ENTRY(&ReadyList);
    for(int i=0 ;i<ReadyList->uxNumberOfItems ;i++){
        TaskControlBlock_t *pxTCB = listGET_LIST_ITEM_OWNER(pxIterator);
        printf("任务名称: %s, 任务ID: %d, 当前优先级: %d\n", 
               pxTCB->taskName, 
               pxTCB->taskID, 
               listGET_LIST_ITEM_VALUE(pxIterator));
        pxIterator = listGET_NEXT(pxIterator);
    }

    //5. 清理内存
    free(task1);
    free(task2);
    free(task3);

    return 0;

}







