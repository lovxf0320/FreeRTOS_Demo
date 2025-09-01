#include "list.h"

struct xLIST List_Test;
struct xLIST_ITEM List_Item1;
struct xLIST_ITEM List_Item2;
struct xLIST_ITEM List_Item3;

int main(void){

    // Initialize the test list
    vListInitialise(&List_Test);

    // Initialize the list items
    vListInitialiseItem(&List_Item1);
    vListInitialiseItem(&List_Item2);
    vListInitialiseItem(&List_Item3);

    List_Item1.xItemValue=10;
    List_Item2.xItemValue=20;
    List_Item3.xItemValue=30;

    // Add items to the list
    vListInsert(&List_Test, &List_Item1);
    vListInsert(&List_Test, &List_Item2);
    vListInsert(&List_Test, &List_Item3);

    for(;;){

        
    }
}