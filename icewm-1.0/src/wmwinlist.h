#ifndef __WINLIST_H
#define __WINLIST_H

#include "wmclient.h" // !!! should be ywindow
#include "ylistbox.h"
#include "yscrollview.h"
#include "yaction.h"

#ifdef CONFIG_WINLIST

class WindowListItem;
class WindowListBox;

class WindowListItem: public YListItem {
public:
    WindowListItem(YClientPeer *peer);
    virtual ~WindowListItem();

    virtual int offset();
    
    virtual const char *text();
    virtual YIcon *icon();
    YClientPeer *peer() const { return fPeer; }
private:
    YClientPeer *fPeer;
};

class WindowListBox:
public YListBox,
public YAction::Listener {
public:
    WindowListBox(YScrollView *view, YWindow *aParent);
    virtual ~WindowListBox();

    virtual bool handleKey(const XKeyEvent &key);
    virtual void handleClick(const XButtonEvent &up, int count);
    
    virtual void activateItem(YListItem *item);
    virtual void actionPerformed(YAction *action, unsigned int modifiers);
};

class WindowList: public YFrameClient {
public:
    WindowList(YWindow *aParent);
    virtual ~WindowList();

    void handleFocus(const XFocusChangeEvent &focus);
    virtual void handleClose();

    virtual void configure(const int x, const int y, 
    			   const unsigned width, const unsigned height,
			   const bool resized);
    void relayout();

    WindowListItem *addWindowListApp(YFrameWindow *frame);
    void removeWindowListApp(WindowListItem *item);

    void repaintItem(WindowListItem *item) { fList->repaintItem(item); }
    void showFocused(int x, int y);

    WindowListBox *list() const { return fList; }

private:
    WindowListBox *fList;
    YScrollView *fScroll;
};

extern WindowList *windowList;

#endif

#endif
