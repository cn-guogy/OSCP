#include "list.h"

//----------------------------------------------------------------------
// ListElement::ListElement
//  初始化一个列表元素，以便可以将其添加到列表中的某个位置。
//
//	"itemPtr" 是要放在列表中的项目。它可以是指向任何事物的指针。
//	"sortKey" 是项目的优先级（如果有的话）。
//----------------------------------------------------------------------

ListElement::ListElement(void *itemPtr, int sortKey)
{
    item = itemPtr;
    key = sortKey;
    next = NULL; // 假设我们将其放在列表的末尾
}

//----------------------------------------------------------------------
// List::List
//  初始化一个列表，开始时为空。
//  现在可以向列表中添加元素。
//----------------------------------------------------------------------

List::List()
{
    first = last = NULL;
}

//----------------------------------------------------------------------
// List::~List
//  为释放列表做准备。如果列表仍然包含任何
//  ListElements，则释放它们。然而，请注意，我们不*会*
//  释放列表上的“项目”——此模块分配
//  和释放 ListElements 以跟踪每个项目，
//  但给定的项目可能在多个列表中，因此我们不能
//  在这里释放它们。
//----------------------------------------------------------------------

List::~List()
{
    while (Remove() != NULL)
        ; // 删除所有列表元素
}

//----------------------------------------------------------------------
// List::Append
//      将“项目”附加到列表的末尾。
//
//	分配一个 ListElement 来跟踪该项目。
//      如果列表为空，则这将是唯一的元素。
//	否则，将其放在末尾。
//
//	"item" 是要放在列表中的东西，它可以是指向
//		任何事物的指针。
//----------------------------------------------------------------------

void List::Append(void *item)
{
    ListElement *element = new ListElement(item, 0);

    if (IsEmpty())
    { // 列表为空
        first = element;
        last = element;
    }
    else
    { // 否则将其放在最后
        last->next = element;
        last = element;
    }
}

//----------------------------------------------------------------------
// List::Prepend
//      将“项目”放在列表的前面。
//
//	分配一个 ListElement 来跟踪该项目。
//      如果列表为空，则这将是唯一的元素。
//	否则，将其放在开头。
//
//	"item" 是要放在列表中的东西，它可以是指向
//		任何事物的指针。
//----------------------------------------------------------------------

void List::Prepend(void *item)
{
    ListElement *element = new ListElement(item, 0);

    if (IsEmpty())
    { // 列表为空
        first = element;
        last = element;
    }
    else
    { // 否则将其放在第一个之前
        element->next = first;
        first = element;
    }
}

//----------------------------------------------------------------------
// List::Remove
//      从列表的前面移除第一个“项目”。
//
// 返回：
//	指向已移除项目的指针，如果列表中没有任何内容则返回 NULL。
//----------------------------------------------------------------------

void *
List::Remove()
{
    return SortedRemove(NULL); // 与 SortedRemove 相同，但忽略键
}

//----------------------------------------------------------------------
// List::Mapcar
//	对列表中的每个项目应用一个函数，通过逐个遍历
//	列表，一个元素一个元素地进行。
//
//	与 LISP 不同，这个 mapcar 不返回任何东西！
//
//	"func" 是要应用于列表中每个元素的过程。
//----------------------------------------------------------------------

void List::Mapcar(VoidFunctionPtr func)
{
    for (ListElement *ptr = first; ptr != NULL; ptr = ptr->next)
    {
        DEBUG('l', "在 mapcar 中，即将调用 %x(%x)\n", func, ptr->item);
        (*func)((_int)ptr->item);
    }
}

//----------------------------------------------------------------------
// List::IsEmpty
//      如果列表为空（没有项目），则返回 TRUE。
//----------------------------------------------------------------------

bool List::IsEmpty()
{
    if (first == NULL)
        return TRUE;
    else
        return FALSE;
}

//----------------------------------------------------------------------
// List::SortedInsert
//      将“项目”插入到列表中，以便列表元素按“sortKey”
//	递增顺序排序。
//
//	分配一个 ListElement 来跟踪该项目。
//      如果列表为空，则这将是唯一的元素。
//	否则，逐个遍历列表，
//	找到新项目应该放置的位置。
//
//	"item" 是要放在列表中的东西，它可以是指向
//		任何事物的指针。
//	"sortKey" 是项目的优先级。
//----------------------------------------------------------------------

void List::SortedInsert(void *item, int sortKey)
{
    ListElement *element = new ListElement(item, sortKey);
    ListElement *ptr; // 用于跟踪

    if (IsEmpty())
    { // 如果列表为空，放置
        first = element;
        last = element;
    }
    else if (sortKey < first->key)
    {
        // 项目放在列表的前面
        element->next = first;
        first = element;
    }
    else
    { // 查找列表中第一个大于项目的元素
        for (ptr = first; ptr->next != NULL; ptr = ptr->next)
        {
            if (sortKey < ptr->next->key)
            {
                element->next = ptr->next;
                ptr->next = element;
                return;
            }
        }
        last->next = element; // 项目放在列表的末尾
        last = element;
    }
}

//----------------------------------------------------------------------
// List::SortedRemove
//      从排序列表的前面移除第一个“项目”。
//
// 返回：
//	指向已移除项目的指针，如果列表中没有任何内容则返回 NULL。
//	将 *keyPtr 设置为已移除项目的优先级值
//	（例如，这在 interrupt.cc 中是需要的）。
//
//	"keyPtr" 是指向存储已移除项目的优先级的
//		位置的指针。
//----------------------------------------------------------------------

void *
List::SortedRemove(int *keyPtr)
{
    ListElement *element = first;
    void *thing;

    if (IsEmpty())
        return NULL;

    thing = first->item;
    if (first == last)
    { // 列表只有一个项目，现在没有了
        first = NULL;
        last = NULL;
    }
    else
    {
        first = element->next;
    }
    if (keyPtr != NULL)
        *keyPtr = element->key;
    delete element;
    return thing;
}
