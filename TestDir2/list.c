#include <stdio.h>
#include "list.h"

static bool firstRun = true;

//List pointer and helper
static List list[LIST_MAX_NUM_HEADS];
static List *freeListHeads; //Stores the head addresses of lists we can use

//Node array and helper
static Node node[LIST_MAX_NUM_NODES];
static List nodeSet; //Holds all nodes that can be used for future list operations
//how do i put them back in here tho wtf

void firstTimeInitialize()
{
    //printf("\nInitializing list for first time run\n");
    firstRun = false;
    //Initializing the nodes
    node[0].prev = NULL;
    node[0].next = &node[1];
    node[0].nodeIndex = 0;

    for (int i = 1; i < LIST_MAX_NUM_NODES; i++)
    {
        node[i].prev = &node[i - 1];
        node[i].next = &node[i + 1];
        node[i].nodeIndex = i;
    }

    //Initializing the list - start at 0 because list has no prev pointer
    for (int i = 0; i < LIST_MAX_NUM_HEADS; i++)
    {
        if (i == LIST_MAX_NUM_HEADS - 1)
        {
            list[LIST_MAX_NUM_HEADS].next = NULL;
        }
        else
        {
            list[i].next = &list[i + 1];
        }
    }

    //Initializing helper arrays
    freeListHeads = &list[0];

    //nodeSet is a list that keeps track of the entire nodeset as a helper function
    nodeSet.start = &node[0];
    nodeSet.current = &node[0];
    nodeSet.end = NULL;
    nodeSet.size = LIST_MAX_NUM_NODES;

    //printf("\nFirst run initialize complete.");
}

List *List_create()
{
    //printf("\nCreating new list.\n");
    if (firstRun)
    {
        firstTimeInitialize();
    }

    //Check if there's remaining space in the list
    if (freeListHeads == NULL)
    {
        //printf("\nThere's no more space remaining in the list.");
        return NULL;
    }

    //Create a new list if it isn't full
    freeListHeads->start = NULL;
    freeListHeads->end = NULL;
    freeListHeads->current = NULL;
    freeListHeads->size = 0;
    freeListHeads->isOOB = true;
    freeListHeads->OOB = LIST_OOB_START;

    List *newList = freeListHeads;
    freeListHeads = freeListHeads->next;
    //printf("List has been created.\n");
    return newList;
}

int List_count(List *pList)
{
    int count = pList->size;
    return count;
}

void *List_first(List *pList)
{
    if (pList->size == 0)
    {
        pList->current = NULL;
        return NULL;
    }
    pList->current = pList->start;
    pList->isOOB = false;
    return pList->current->data;
}

void *List_last(List *pList)
{
    if (pList->size == 0)
    {
        pList->current = NULL;
        return NULL;
    }
    pList->current = pList->end;
    pList->isOOB = false;
    return pList->current->data;
}

void *List_next(List *pList)
{
    if (pList->current == pList->end)
    {
        //printf("\nEnd of list reached.\n");
        pList->current = NULL;
        pList->isOOB = true;
        pList->OOB = LIST_OOB_END;
        return NULL;
    }
    pList->current = pList->current->next;
    return pList->current->data;
}

void *List_prev(List *pList)
{
    if (pList->current == pList->start)
    {
        pList->current = NULL;
        return NULL;
    }
    if (pList->current == pList->start)
    {
        pList->isOOB = true;
        pList->OOB = LIST_OOB_START;
        pList->current = NULL;
        return NULL;
    }
    pList->current = pList->current->prev;
    return pList->current->data;
}

void *List_curr(List *pList)
{
    if (pList->isOOB)
    {
        return NULL;
    }
    return pList->current->data;
}

//Function to check overflow
bool checkOverflow(List *pList)
{
    if ((pList->isOOB && pList->OOB == LIST_OOB_END) || pList->current == pList->end)
    {
        return true;
    }
    return false;
}

//Function to check underflow
bool checkUnderflow(List *pList)
{
    if ((pList->isOOB && pList->OOB == LIST_OOB_START) || pList->current == pList->start)
    {
        return true;
    }
    return false;
}

int List_insert_after(List *pList, void *pItem)
{
    if (nodeSet.size == 0)
    {
        //printf("\nThere isn't any space left in the node array");
        return -1;
    }

    Node *newNode = nodeSet.start;
    nodeSet.start->data = pItem;

    if (pList->size == 0)
    {
        nodeSet.start = nodeSet.start->next;
        pList->start = newNode;
        pList->end = newNode;
        pList->current = newNode;
        pList->size++;
        pList->start->next = NULL;
        pList->start->prev = NULL;
        nodeSet.size--;
        return 0;
    }

    if (checkOverflow(pList))
    {
        //printf("\nOverflowed pointer - Adding to the end\n");
        return List_append(pList, pItem);
    }

    else if (checkUnderflow(pList))
    {
        //printf("\nUnderflowed pointer - Adding to the beginning");
        return List_prepend(pList, pItem);
    }

    nodeSet.start = nodeSet.start->next;
    nodeSet.size--;

    //Create a copy of the nodes to easily insert
    Node *copyPrevNode = pList->current;
    Node *copyNextNode = pList->current->next;

    //Connect previous and next nodes to new node
    newNode->next = copyNextNode;
    copyNextNode->prev = newNode;
    newNode->prev = copyPrevNode;
    copyPrevNode->next = newNode;
    pList->current= newNode;
    pList->size++;
    return 0;
}

int List_insert_before(List *pList, void *pItem)
{
    if (nodeSet.size == 0)
    {
        //printf("\nThere isn't any space left in the node array");
        return -1;
    }
    nodeSet.start->data = pItem;
    Node *newNode = nodeSet.start;

    if (pList->size == 0)
    {
        nodeSet.start = nodeSet.start->next;
        pList->start = newNode;
        pList->end = newNode;
        pList->current = newNode;
        pList->size++;
        pList->start->next = NULL;
        pList->start->prev = NULL;
        nodeSet.size--;
        return 0;
    }

    if (checkOverflow(pList))
    {
        //printf("\nOverflowed pointer - Adding to the end\n");
        return List_append(pList, pItem);
    }

    else if (checkUnderflow(pList))
    {
        //printf("\nUnderflowed pointer - Adding to the beginning");
        return List_prepend(pList, pItem);
    }

    nodeSet.start = nodeSet.start->next;
    nodeSet.size--;
    //Create a copy of the nodes to easily insert
    Node *copyPrevNode = pList->current->prev;
    Node *copyNextNode = pList->current;

    //Connect previous and next nodes to new node
    newNode->next = copyNextNode;
    copyNextNode->prev = newNode;
    newNode->prev = copyPrevNode;
    copyPrevNode->next = newNode;
    pList->current= newNode;
    pList->size++;
    return 0;
}

int List_append(List *pList, void *pItem)
{
    if (nodeSet.size == 0)
    {
        //printf("There isn't any space left in the node array");
        return -1;
    }

    Node *newEnd = nodeSet.start;
    nodeSet.start = nodeSet.start->next;
    nodeSet.size--;
    newEnd->data = pItem;
    newEnd->next = NULL;

    if (pList->size == 0)
    {
        nodeSet.start = nodeSet.start->next;
        pList->start = newEnd;
        pList->end = newEnd;
        pList->current = newEnd;
        pList->size++;
        pList->start->next = NULL;
        pList->start->prev = NULL;
        nodeSet.size--;
        return 0;
    }

    Node *oldEnd = pList->end;
    oldEnd->next = newEnd;
    newEnd->prev = oldEnd;
    newEnd->next = NULL;
    pList->end = newEnd;
    pList->size = pList->size + 1;
    pList->current = newEnd;
    pList->isOOB = false;
    return 0;
}

int List_prepend(List *pList, void *pItem)
{
    if (nodeSet.size == 0)
    {
        //printf("There isn't any space left in the node array");
        return -1;
    }

    Node *newStart = nodeSet.start;
    nodeSet.start = nodeSet.start->next;
    nodeSet.size--;
    newStart->data = pItem;
    newStart->next = NULL;

    if (pList->size == 0)
    {
        nodeSet.start = nodeSet.start->next;
        pList->start = newStart;
        pList->end = newStart;
        pList->current = newStart;
        pList->size++;
        pList->start->next = NULL;
        pList->start->prev = NULL;
        nodeSet.size--;
        return 0;
    }

    Node *oldStart = pList->start;
    newStart->next = oldStart;
    oldStart->prev=newStart;
    newStart->prev = NULL;
    pList->start = newStart;
    pList->size = pList->size + 1;
    pList->current = newStart;
    pList->isOOB = false;
    return 0;
}

void *List_remove(List *pList)
{
    if (pList->isOOB == true)
    {
        //printf("Underflow/Overflow encountered. Cancelling remove.");
        return NULL;
    }

    //Copy this since they will be unaccessible after list operations
    Node *removedNode = pList->current;
    if (pList->size == 1)
    {
        pList->start = NULL;
        pList->end = NULL;
        pList->current = NULL;
        pList->size = 0;
        pList->isOOB = false;
    }
    else
    {
        //If it's the first element in the list
        if (pList->current == pList->start)
        {
            Node *nextNode = pList->current->next;
            pList->start->next = NULL;
            nextNode->prev = NULL;
            pList->size--;
            pList->start = nextNode;
            pList->current = nextNode;
        }
            //If it's the last element in the list
        else if (pList->current == pList->end)
        {
            Node *prevNode = pList->current->prev;
            pList->end->prev = NULL;
            prevNode->next = NULL;
            pList->size--;
            pList->end = prevNode;
            pList->current = prevNode;
        }
        else
        {
            Node *prevNode = pList->current->prev;
            Node *nextNode = pList->current->next;
            prevNode->next = nextNode;
            nextNode->prev = prevNode;
            pList->current = nextNode;
            pList->size--;
        }
    }

    void *deletedData = removedNode->data;
    int removedNodeIndex = removedNode->nodeIndex;
    node[removedNodeIndex].data = NULL;

    //Put it in the beginning of nodeset so it'll be picked up for the next created node
    nodeSet.start->prev = &node[removedNodeIndex];
    node[removedNodeIndex].next = nodeSet.start;
    nodeSet.start = &node[removedNodeIndex];
    nodeSet.start->prev = NULL;
    nodeSet.size++;
    return deletedData;
}

void *List_trim(List *pList)
{
    if (pList->size == 0)
    {
        return NULL;
    }
    else
    {
        Node *removedNode = pList->end;
        if (pList->size == 1)
        {
            pList->start = NULL;
            pList->end = NULL;
            pList->current = NULL;
            pList->size--;
        }
        else
        {
            Node *prevLast = pList->end;
            Node *newLast = pList->current;
            newLast->next=NULL;
            prevLast->prev = NULL;
            prevLast->next = NULL;
            pList->end = newLast;
            pList->size--;
        }

        void *deletedData = removedNode->data;
        int removedNodeIndex = removedNode->nodeIndex;
        node[removedNodeIndex].data = NULL;

        //Put it back in the list of free nodes
        nodeSet.start->prev = &node[removedNodeIndex];
        node[removedNodeIndex].next = nodeSet.start;
        nodeSet.start = &node[removedNodeIndex];
        nodeSet.start->prev = NULL;
        nodeSet.size++;
        return deletedData;
    }
}

void List_concat(List *pList1, List *pList2)
{
    if (pList1->size == 0)
    {
        pList1->start = pList2->start;
        pList1->end = pList2->end;
        pList1->size = pList2->size;
        pList2->start = NULL;
        pList2->end = NULL;
    }
    else if (pList2->size == 0)
    {
        pList2->start = NULL;
        pList2->end = NULL;
        pList2->current = NULL;
        //Just gotta make sure pList2 is safe to remove
    }
    else
    {
        pList1->end->next = pList2->start;
        pList2->start->prev = pList1->end;
        pList1->end = pList2->end;
        pList2->start = NULL;
        pList2->end = NULL;
        pList2->current = NULL;
        pList1->size = pList1->size + pList2->size;
        pList2->size=0;
        pList2->next = freeListHeads;
    }
    pList2->next = freeListHeads;
    freeListHeads = pList2;
}

void List_free(List *pList, FREE_FN pItemFreeFn){
    //Set current to start of list
    List_first(pList);

    //Remove all elements in list
    if(pList->size!=0){
        while(pList->current->next != NULL){
            (*pItemFreeFn)(pList->current);
            List_remove(pList);
        }
        List_last(pList);
        List_remove(pList);
    }

    //Similar to releasing a node, release the list and put it back in circulation
    pList->start=NULL;
    pList->end=NULL;
    pList->current=NULL;
    pList->isOOB=false;
    pList->size=0;

    List* tempList = pList;
    tempList->next=freeListHeads;
    freeListHeads=tempList;
}

void* List_search(List *pList, COMPARATOR_FN pComparator, void *pComparisonArg)
{
    if(checkUnderflow(pList)||checkOverflow(pList)){
        pList->current=pList->start;
    }
    if(pList->size==0){
        return NULL;
    }
    while(pList->current!=pList->end){
        void* tempData = pList->current->data;
        if((pComparator)(tempData, pComparisonArg) == 1){
            printf("\n\tItem has been found!");
            return pList->current;
        }
        List_next(pList);
    }

    //Checking the last node
    List_last(pList);
    if((pComparator)(pList->current->data, pComparisonArg) == 1){
        return pList->current;
    }
    pList->isOOB=true;
    pList->OOB=LIST_OOB_END;
    printf("\n\tItem has NOT been found!");
    return NULL;
}