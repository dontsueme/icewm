#ifndef __YMENU_H
#define __YMENU_H

#include "yaction.h"
#include "ypopup.h"
#include "ytimer.h"

class YMenuItem;

class YMenu:
public YPopupWindow,
public YTimer::Listener {
public:
    static unsigned const NoItem(~0U);

    YMenu(YWindow *parent = 0);
    virtual ~YMenu();

    virtual void sizePopup(int hspace);
    virtual void activatePopup();
    virtual void deactivatePopup();
    virtual void donePopup(YPopupWindow *popup);

    virtual void paint(Graphics &g, int x, int y, unsigned width, unsigned height);

    virtual bool handleKey(const XKeyEvent &key);
    virtual void handleButton(const XButtonEvent &button);
    virtual void handleMotion(const XMotionEvent &motion);
    virtual bool handleAutoScroll(const XMotionEvent &mouse);

    void trackMotion(const int x_root, const int y_root, const unsigned state);

    YMenuItem *add(YMenuItem *item);
    YMenuItem *addItem(const char *name, int hotCharPos, const char *param, YAction *action);
    YMenuItem *addItem(const char *name, int hotCharPos, YAction *action, YMenu *submenu);
    YMenuItem *addSubmenu(const char *name, int hotCharPos, YMenu *submenu);
    YMenuItem *addSeparator();
    YMenuItem *addLabel(const char *name);
    void removeAll();
    YMenuItem *findAction(const YAction *action);
    YMenuItem *findSubmenu(const YMenu *sub);
    YMenuItem *findName(const char *name, const unsigned first = 0);

    void enableCommand(YAction *action); // 0 == All
    void disableCommand(YAction *action); // 0 == All

    unsigned itemCount() const { return fItemCount; }
    YMenuItem *item(unsigned n) const { return fItems[n]; }

    bool isShared() const { return fShared; }
    void shared(bool shared = true) { fShared = shared; }

    void actionListener(YAction::Listener *actionListener);
    YAction::Listener *actionListener() const { return fActionListener; }

    virtual bool handleTimer(YTimer *timer);

private:
    unsigned itemHeight(unsigned item, int &top, int &bottom, int &pad);
    void itemWidth(unsigned item, int &iw, int &nw, int &pw);
    void offsets(int &left, int &top, int &right, int &bottom);
    void area(int &x, int &y, int &w, int &h);

    void drawBackground(Graphics &g, int x, int y, int w, int h);
    void drawSeparator(Graphics &g, int x, int y, int w);

    void paintItem(Graphics &g, unsigned item, int &l, int &t, int &r,
                                int minY, int maxY, bool paint);
    void paintItems();
    void findItemPos(unsigned item, int &x, int &y);
    unsigned findItem(int x, int y);
    unsigned findActiveItem(unsigned cur, int direction);
    unsigned findHotItem(char key);
    void focusItem(unsigned item, bool submenu, bool byMouse);
    void activateItem(unsigned item, bool byMouse, unsigned modifiers);
    bool isCondCascade(unsigned selectedItem);
    bool onCascadeButton(unsigned selectedItem, int x, int y, bool checkPopup);

    void autoScroll(int deltaX, int deltaY, int mx, int my,
                    const XMotionEvent *motion);
    void finishPopup(YMenuItem *item, YAction *action, unsigned modifiers);


    unsigned fItemCount;
    YMenuItem **fItems;
    unsigned fSelectedItem, fPaintedItem;
    int paramPos;
    int namePos;
    YPopupWindow *fPopup;
    YPopupWindow *fPopupActive;
    bool fShared;
    YAction::Listener *fActionListener;
    int activatedX, activatedY;
    
#ifdef CONFIG_GRADIENTS
    class YPixbuf * fGradient;
#endif

    static YTimer *fMenuTimer;
    static int fTimerX, fTimerY, fTimerSubmenu;
    static unsigned fTimerItem;
    static bool fTimerSlow;
    static int fAutoScrollDeltaX, fAutoScrollDeltaY;
    static int fAutoScrollMouseX, fAutoScrollMouseY;
};

extern YPixmap *menubackPixmap;
extern YPixmap *menuselPixmap;
extern YPixmap *menusepPixmap;

#ifdef CONFIG_GRADIENTS
class YPixbuf;

extern YPixbuf *menubackPixbuf;
extern YPixbuf *menuselPixbuf;
extern YPixbuf *menusepPixbuf;
#endif

#endif
