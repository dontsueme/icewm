/*
 *  IceWM - Linked lists
 *  Copyright (C) 2001 The Authors of IceWM
 *
 *  Release under terms of the GNU General Public License
 *
 *  2001/10/05: Mathias Hasselmann <mathias.hasselmann@gmx.net>
 *  - initial version
 */

#ifndef __YLISTS_H
#define __YLISTS_H

#include "base.h"
#include <stddef.h>

/*******************************************************************************
 * Iterators
 ******************************************************************************/

template <class IteratorType, class ListType, class ItemType, class ContentType>
class YIterator {
protected:
    typedef YIterator Iterator;
    typedef typename ListType::Item Item;

protected:
    YIterator(ItemType * begin): fPos(begin) {}

public:
    operator bool () const { return valid(); }
    operator ItemType * () const { return fPos; }
    ItemType & operator * () const { return *fPos; }
    ItemType * operator -> () const { return fPos; }

    IteratorType & operator ++ () {
        extend(); return * static_cast<IteratorType *>(this);
    }

    virtual inline bool valid() const = 0;
    virtual inline void extend() = 0;

protected:
    ItemType * fPos;
};

/******************************************************************************/

template <class ListType, class ItemType, class ContentType>
class YForwardIterator:
public YIterator<YForwardIterator<ListType, ItemType, ContentType>,
                 ListType, ItemType, ContentType> {
public:
    YForwardIterator(ListType const & list, ItemType * end = NULL):
        Iterator(list.head()), fEnd(end) {}
    YForwardIterator(ItemType * begin, ItemType * end = NULL):
        Iterator(begin), fEnd(end) {}

    virtual bool valid() const {
        return fPos != fEnd;
    }

    virtual void extend() {
        if (fPos && fPos != fEnd) fPos = fPos->next();
    }

private:
    ItemType * fEnd;
};

template <class ListType, class ItemType, class ContentType>
class YBackwardIterator:
public YIterator<YBackwardIterator<ListType, ItemType, ContentType>,
                 ListType, ItemType, ContentType> {
public:
    YBackwardIterator(ListType const & list, ItemType * end = NULL):
        Iterator(list.tail()), fEnd(end) {}
    YBackwardIterator(ItemType * begin, ItemType * end = NULL):
        Iterator(begin), fEnd(end) {}

    virtual bool valid() const {
        return fPos != fEnd;
    }

    virtual void extend() {
        if (fPos && fPos != fEnd) fPos = fPos->prev();
    }

private:
    ItemType * fEnd;
};

/******************************************************************************/

template <class ListType, class ItemType, class ContentType>
class YForwardRingIterator:
public YIterator<YForwardRingIterator<ListType, ItemType, ContentType>,
                 ListType, ItemType, ContentType> {
public:
    YForwardRingIterator(ListType const & list):
        Iterator(list.head()), fList(list), fBegin(NULL) {}
    YForwardRingIterator(ListType const & list, ItemType * begin):
        Iterator(begin), fList(list), fBegin(NULL) {}
    YForwardRingIterator(ListType const & list, ContentType * begin):
        Iterator(list.find(begin)), fList(list), fBegin(NULL) {}

    virtual bool valid () const {
        return fPos != fBegin;
    }

    virtual void extend() {
        if (fPos && fPos != fBegin) {
            if (NULL == fBegin) fBegin = fPos;
            fPos = fPos->next();
            if (NULL == fPos) fPos = fList.head();
        }
    }

private:
    ListType const & fList;
    ItemType * fBegin;
};

template <class ListType, class ItemType, class ContentType>
class YBackwardRingIterator:
public YIterator<YBackwardRingIterator<ListType, ItemType, ContentType>,
                 ListType, ItemType, ContentType> {
public:
    YBackwardRingIterator(ListType const & list):
        Iterator(list.tail()), fList(list), fBegin(NULL) {}
    YBackwardRingIterator(ListType const & list, ItemType * begin):
        Iterator(begin), fList(list), fBegin(NULL) {}
    YBackwardRingIterator(ListType const & list, ContentType * begin):
        Iterator(list.find(begin)), fList(list), fBegin(NULL) {}

    virtual bool valid () const {
        return fPos != fBegin;
    }

    virtual void extend() {
        if (fPos && fPos != fBegin) {
            if (NULL == fBegin) fBegin = fPos;
            fPos = fPos->prev();
            if (NULL == fPos) fPos = fList.tail();
        }
    }

private:
    ListType const & fList;
    ItemType * fBegin;
};

/******************************************************************************/

template <class IteratorBase, class ListType, class ItemType, class ContentType>
class YCountingIterator:
public IteratorBase {
protected:
    typedef YCountingIterator CountingIterator;

public:
    YCountingIterator(ListType const & list, ItemType * end = NULL):
        IteratorBase(list, end), fCount(0) {}
    YCountingIterator(ItemType * begin, ItemType * end = NULL):
        IteratorBase(begin, end), fCount(0) {}

    unsigned count() const { return fCount; }
    virtual void extend() { IteratorBase::extend(); ++fCount; }

private:
    unsigned fCount;
};

template <class ListType, class ItemType, class ContentType>
class YCountingForwardIterator:
public YCountingIterator <YForwardIterator <ListType, ItemType, ContentType>,
                          ListType, ItemType, ContentType> {
public:
    YCountingForwardIterator(ListType const & list, ItemType * end = NULL):
        CountingIterator(list, end) {}
    YCountingForwardIterator(ItemType * begin, ItemType * end = NULL):
        CountingIterator(begin, end) {}

    YCountingForwardIterator & operator ++ () { extend(); return *this; }
};

template <class ListType, class ItemType, class ContentType>
class YCountingBackwardIterator:
public YCountingIterator <YBackwardIterator <ListType, ItemType, ContentType>,
                          ListType, ItemType, ContentType> {
public:
    YCountingBackwardIterator(ListType const & list, ItemType * end = NULL):
        CountingIterator(list, end) {}
    YCountingBackwardIterator(ItemType * begin, ItemType * end = NULL):
        CountingIterator(begin, end) {}

    YCountingBackwardIterator & operator ++ () { extend(); return *this; }
};

/*******************************************************************************
 * A single linked list
 ******************************************************************************/

template <class ContentType>
class YSingleList {
public:
    class Item {
    public:
        Item(ContentType * data, Item * next = NULL):
            fData(data), fNext(next) {}

    	Item * next() const { return fNext; }
    	ContentType * data() const { return fData; }

        operator ContentType * () const { return data(); }
        ContentType & operator * () const { return *data(); }
        ContentType * operator -> () const { return data(); }

    protected:
        friend class YSingleList;
        void next(Item * next) { fNext = next; }

    private:
        ContentType * fData;
        Item * fNext;
    };

    typedef YForwardIterator<YSingleList, Item, ContentType> Iterator;
    typedef YForwardRingIterator<YSingleList, Item, ContentType> RingIterator;
    typedef YCountingForwardIterator<YSingleList, Item, ContentType> CountingIterator;

    YSingleList(): fHead(NULL), fTail(NULL) {}

    Item * head() const { return fHead; }
    Item * tail() const { return fTail; }

    bool empty() const { return NULL == fHead; }
    bool filled() const { return NULL != fHead; }

/**
 * Inserts a pointer to a Item object at the tail of a single linked list.
 */ 
    void append(Item * item) {
        PRECONDITION((bool) fHead == (bool) fTail);
        PRECONDITION(NULL == fTail || NULL == fTail->next());
        PRECONDITION(item != NULL);

        if (NULL != fTail) fTail->next(item);
        else fHead = fTail = item;

        while (NULL != fTail->next()) { fTail = fTail->next(); }
    }

/**
 * Inserts a pointer to a ContentType object at the tail of a single linked list.
 * A new item object is allocated to achieve this.
 */ 
    void append(ContentType * data) {
        append(new Item(data));
    }

/**
 * Inserts a pointer to a Item object at the head of a single linked list.
 */ 
    void prepend(Item * item) {
        PRECONDITION((bool) fHead == (bool) fTail);
        PRECONDITION(NULL == fTail || NULL == fTail->next());
        PRECONDITION(item != NULL);

        item->next(fHead);

        if (NULL != fTail) fHead = item;
        else fHead = fTail = item;
    }

/**
 * Inserts a pointer to a ContentType object at the head of a single linked list.
 * A new item object is allocated to achieve this.
 */ 
    void prepend(ContentType * data) {
        prepend(new Item(data));
    }

/**
 * Removes an item from a single linked list.
 */ 
    void remove(Item * item) {
        PRECONDITION((bool) fHead == (bool) fTail);
        PRECONDITION(NULL == fTail || NULL == fTail->next());
        PRECONDITION(item != NULL);

        if (item != fHead) {
            Item * predecessor(fHead), * next;
            while ((next = predecessor->next()) != item) predecessor = next;
            predecessor->next(item->next());
            if (item == fTail) fTail = predecessor;
        } else {
            fHead = item->next();
            if (item == fTail) fTail = NULL;
        }
    }

/**
 * Removes a ContentType object from a single linked list.
 * The associated item is destroyed.
 */ 
    void remove(ContentType * data) {
        PRECONDITION(NULL != fHead);
        PRECONDITION(NULL != fTail && NULL == fTail->next());
        PRECONDITION(data != NULL);

        Item * item(fHead);        

        if (data != *fHead) {
            Item * predecessor(fHead);
            while (*(item = predecessor->next()) != data) predecessor = item;
            predecessor->next(item->next());
            if (item == fTail) fTail = predecessor;
        } else {
            fHead = (item = fHead)->next();
            if (item == fTail) fTail = NULL;
        }

        delete item;
    }

/**
 * Removes an item from a single linked list and destroys it.
 */ 
    void destroy(Item * item) {
        remove(item);
        delete item;
    }

/**
 * Removes a ContentType object from a single linked list and destroys it.
 * The associated item is destroyed.
 */ 
    void destroy(ContentType * data) {
        remove(data);
        delete data;
    }

/**
 * Number of items in this list.
 */ 
    unsigned count() const {
        return count(head());
    }

/**
 * Number of items behind this item.
 */ 
    unsigned count(Item * begin) const {
        CountingIterator i(begin); while (i) ++i; return i.count();
    }

/**
 * Finds an item in this list.
 */ 
    template <class ReferenceType>
    Item * find(ReferenceType const & reference) const  {
        Iterator i(head()); while (i && *i != reference) ++i; return i;
    }

private:
    Item * fHead, * fTail;
};

/*******************************************************************************
 * A double linked list
 ******************************************************************************/

template <class ContentType>
class YDoubleList {
public:
    class Item {
    public:
        Item(ContentType * data, Item * prev = NULL, Item * next = NULL):
            fData(data), fPrev(prev), fNext(next) {}

    	Item * prev() const { return fPrev; }
    	Item * next() const { return fNext; }
    	ContentType * data() const { return fData; }

        operator ContentType * () const { return data(); }
        ContentType & operator * () const { return *data(); }
        ContentType * operator -> () const { return data(); }

    protected:
        friend class YDoubleList;
        void prev(Item * prev) { fPrev = prev; }
        void next(Item * next) { fNext = next; }

    private:
        ContentType * fData;
        Item * fPrev, * fNext;
    };

    typedef YForwardIterator<YDoubleList, Item, ContentType> Iterator;
    typedef YForwardIterator<YDoubleList, Item, ContentType> ForwardIterator;
    typedef YBackwardIterator<YDoubleList, Item, ContentType> BackwardIterator;

    typedef YForwardRingIterator<YDoubleList, Item, ContentType> RingIterator;
    typedef YForwardRingIterator<YDoubleList, Item, ContentType> ForwardRingIterator;
    typedef YBackwardRingIterator<YDoubleList, Item, ContentType> BackwardRingIterator;

    typedef YCountingForwardIterator<YDoubleList, Item, ContentType> CountingIterator;
    typedef YCountingForwardIterator<YDoubleList, Item, ContentType> CountingForwardIterator;
    typedef YCountingBackwardIterator<YDoubleList, Item, ContentType> CountingBackwardIterator;

    YDoubleList(): fHead(NULL), fTail(NULL) {}

    Item * head() const { return fHead; }
    Item * tail() const { return fTail; }

    bool empty() const { return NULL == fHead; }
    bool filled() const { return NULL != fHead; }

/**
 * Inserts a pointer to a Item object at the tail of a double linked list.
 */ 
    void append(Item * item) {
        PRECONDITION((bool) fHead == (bool) fTail);
        PRECONDITION(NULL == fHead || NULL == fHead->prev());
        PRECONDITION(NULL == fTail || NULL == fTail->next());
        PRECONDITION(item != NULL);

        item->prev(fTail);

        if (NULL != fTail) fTail->next(item);
        else fHead = fTail = item;

        while (NULL != fTail->next()) { fTail = fTail->next(); }
    }

/**
 * Inserts a pointer to a ContentType object at the tail of a double linked list.
 * A new item object is allocated to achieve this.
 */ 
    void append(ContentType * data) {
        append(new Item(data));
    }

/**
 * Inserts a pointer to a Item object at the head of a douule double list.
 */ 
    void prepend(Item * item) {
        PRECONDITION((bool) fHead == (bool) fTail);
        PRECONDITION(NULL == fHead || NULL == fHead->prev());
        PRECONDITION(NULL == fTail || NULL == fTail->next());
        PRECONDITION(item != NULL);

        item->next(fHead);
        if (NULL != fHead) fHead->prev(item);

        if (NULL != fTail) fHead = item;
        else fHead = fTail = item;
    }

/**
 * Inserts a pointer to a ContentType object at the head of a double linked list.
 * A new item object is allocated to achieve this.
 */ 
    void prepend(ContentType * data) {
        prepend(new Item(data));
    }

/**
 * Removes an item from a double double list.
 */ 
    void remove(Item * item) {
        PRECONDITION((bool) fHead == (bool) fTail);
        PRECONDITION(NULL == fTail || NULL == fTail->next());
        PRECONDITION(item != NULL);

        if (item->next()) item->next()->prev(item->prev());
        else fTail = item->prev();

        if (item->prev()) item->prev()->next(item->next());
        else fHead = item->next();

        item->prev(NULL);
        item->next(NULL);
    }

/**
 * Removes an item from a double double list.
 */ 
    void remove(ContentType * data) {
        remove(find(data));
    }

/**
 * Removes an item from a double linked list and destroys it.
 */ 
    void destroy(Item * item) {
        remove(item);
        delete item;
    }

/**
 * Removes a ContentType object from a double linked list and destroys it.
 * The associated item is destroyed.
 */ 
    void destroy(ContentType * data) {
        remove(data);
        delete data;
    }

/**
 * Number of items in this list.
 */ 
    unsigned count() const { return count(head()); }

/**
 * Number of items behind this item.
 */ 
    unsigned count(ContentType * begin) const {
        CountingForwardIterator i(begin); while (i) ++i; return i.count();
    }

/**
 * Position of this item in the list.
 */ 
    unsigned position(ContentType * item) const {
        CountingBackwardIterator i(item); while (i) ++i; return i.count();
    }

/**
 * Finds an item in this list.
 */ 
    template <class ReferenceType>
    Item * find(ReferenceType const & reference) const  {
        Iterator i(head()); while (i && *i != reference) ++i; return i;
    }

private:
    Item * fHead, * fTail;
};

#endif
