#ifndef __YLISTBOX_H
#define __YLISTBOX_H

#include "ywindow.h"
#include "yscrollbar.h"
#include "yscrollview.h"
#include "ypopup.h"

class YScrollView;

class YListItem {
public:
    YListItem(): fPrevItem(NULL), fNextItem(NULL), fSelected(false) {}
    virtual ~YListItem() {}

    YListItem *next() const { return fNextItem; }
    YListItem *prev() const { return fPrevItem; }
    void next(YListItem *next) { fNextItem = next; }
    void prev(YListItem *prev) { fPrevItem = prev; }

    bool selected() { return fSelected; }
    void selected(bool selected) { fSelected = selected; }

    virtual int offset() { return 0; }

    virtual const char *text() { return NULL; }
    virtual YIcon *icon() { return NULL; }

private:
    YListItem *fPrevItem, *fNextItem;
    bool fSelected; // !!! remove this from here
};

class YListBox:
public YWindow, 
public YScrollBar::Listener,
public YScrollable {
public:
    YListBox(YScrollView *view, YWindow *parent, bool drawIcons = true);
    virtual ~YListBox();

    int addItem(YListItem *item);
    int addAfter(YListItem *prev, YListItem *item);
    void removeItem(YListItem *item);

    virtual void configure(const int x, const int y,
    			   const unsigned width, const unsigned height,
			   const bool resized);
    virtual bool handleKey(const XKeyEvent &key);
    virtual void handleButton(const XButtonEvent &button);
    virtual void handleClick(const XButtonEvent &up, int count);
    virtual void handleDrag(const XButtonEvent &down, const XMotionEvent &motion);
    virtual void handleMotion(const XMotionEvent &motion);
    virtual bool handleAutoScroll(const XMotionEvent &mouse);

    virtual void paint(Graphics &g, int x, int y, unsigned int width, unsigned int height);
    virtual void scroll(YScrollBar *sb, int delta);
    virtual void move(YScrollBar *sb, int pos);

    virtual bool isFocusTraversable();
    bool hasSelection();
    virtual void activateItem(YListItem *item);

    YListItem *first() const { return fFirst; }
    YListItem *last() const { return fLast; }

    int itemCount();
    YListItem *item(int item);
    int findItemByPoint(int x, int y);
    int findItem(YListItem *item);
    int lineHeight();
    int focusedItem() const { return fFocusedItem; }

    int maxWidth();

    bool isSelected(int item);
    bool isSelected(YListItem *item);

    virtual int contentWidth();
    virtual int contentHeight();
    virtual YWindow *window();

    void focusSelectItem(int no) { focusedItem(no, true, false, false); }

    void repaintItem(YListItem *item);

protected:
    bool isItemSelected(int index);
    void selectItem(int index, bool sel);
    void selectItems(int selStart, int selEnd, bool sel);
    void paintItems(int selStart, int selEnd);
    void clearSelection();
    void focusedItem(int index, bool clear, bool extend, bool virt);

    void applySelection();
    void paintItem(int i);
    void paintItem(Graphics &g, int n);
    void resetScrollBars();
    void freeItems();
    void updateItems();
    void autoScroll(int delta, const XMotionEvent *motion);
    void focusVisible();
    void ensureVisibility(int item);

    YScrollBar *horizontalScrollBar() const { return fVerticalScroll; }
    YScrollBar *verticalScrollBar() const { return fHorizontalScroll; }

private:
    YScrollBar *fVerticalScroll;
    YScrollBar *fHorizontalScroll;
    YScrollView *fView;
    YListItem *fFirst, *fLast;
    int fItemCount;
    YListItem **fItems;

    int fOffsetX;
    int fOffsetY;
    int fMaxWidth;
    int fFocusedItem;
    int fSelectStart, fSelectEnd;
    bool fDragging;
    bool fSelect;
    bool fDrawIcons;

    static int fAutoScrollDelta;

#ifdef CONFIG_GRADIENTS
    class YPixbuf * fGradient;
#endif
};

class YSimpleListBox : public YListBox {
public:
    YSimpleListBox(YScrollView *view, YWindow *parent, bool drawIcons = true):
        YListBox(view, parent, drawIcons) {
    }

    virtual bool handleKey(const XKeyEvent &key);
    virtual void handleButton(const XButtonEvent &button);
    virtual void handleMotion(const XMotionEvent &motion);
    virtual void handleClick(const XButtonEvent &, int) {}
    virtual void handleDrag(const XButtonEvent &, const XMotionEvent &) {}
};

class YListPopup : public YPopupWindow {
protected:
    class ListItem : public YListItem {
    public:
        ListItem(int id, char const *text):
            fId(id), fText(newstr(text)) {}
        virtual ~ListItem() { delete[] fText; }
        virtual const char *text() { return fText; }
        int id() const { return fId; }

    private:
        int fId;
        char *fText;
    };

public:
    YListPopup(YWindow *parent = NULL):
        YPopupWindow(parent),
        fScrollView(new YScrollView(this)),
        fListBox(new YSimpleListBox(fScrollView, fScrollView, false)) {
        fListBox->show();
        fScrollView->view(fListBox);
        fScrollView->show();
    }

    virtual ~YListPopup() {
        delete fListBox;
        delete fScrollView;
    }

    virtual void configure(const int x, const int y,
			   const unsigned w, const unsigned h,
			   const bool resized);
    virtual void paint(Graphics &g, int, int, unsigned, unsigned);

    virtual bool handleKey(const XKeyEvent &key);
    virtual void handleButton(const XButtonEvent &button);
    virtual void handleMotion(const XMotionEvent &motion);

    void add(int id, char const *text) {
        fListBox->addItem(new ListItem(id, text));
    }
    
    ListItem *item(int index) {
        return (ListItem *) fListBox->item(index);
    }
    
    void select(int index) {
        fListBox->focusSelectItem(index);
    }

    int selection() {
        return fListBox->focusedItem();
    }

    int id() {
        ListItem const *item((ListItem *)fListBox->item(selection()));
        return (item ? item->id() : -1);
    }

    void clear();

    unsigned preferredWidth(int x, unsigned width);
    unsigned preferredHeight(int width);

private:
    YScrollView *fScrollView;
    YListBox *fListBox;
};

extern YPixmap * listbackPixmap;

#ifdef CONFIG_GRADIENTS
extern class YPixbuf * listbackPixbuf;
#endif

#endif
