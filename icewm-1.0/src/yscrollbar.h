#ifndef __YSCROLLBAR_H
#define __YSCROLLBAR_H

#include "ywindow.h"
#include "ytimer.h"

#define SCROLLBAR_MIN  8

class YScrollBar:
public YWindow,
public YTimer::Listener {
public:
    class Listener {
    public:
        virtual void scroll(YScrollBar *scroll, int delta) = 0;
        virtual void move(YScrollBar *scroll, int pos) = 0;
    };

    enum Orientation {
        Vertical, Horizontal
    };

    YScrollBar(YWindow *aParent);
    YScrollBar(Orientation anOrientation, YWindow *aParent);
    YScrollBar(Orientation anOrientation,
               int aValue, int aVisibleAmount, int aMin, int aMax,
               YWindow *aParent);
    virtual ~YScrollBar();

    Orientation orientation() const { return fOrientation; }
    int maximum() const { return fMaximum; }
    int minimum() const { return fMinimum; }
    int visibleAmount() const { return fVisibleAmount; }
    int unitIncrement() const { return fUnitIncrement; }
    int blockIncrement() const { return fBlockIncrement; }
    int value() const { return fValue; }

    void orientation(Orientation anOrientation);
    void maximum(int aMaximum);
    void minimum(int aMinimum);
    void visibleAmount(int aVisibleAmount);
    void unitIncrement(int anUnitIncrement);
    void blockIncrement(int aBlockIncrement);
    void value(int aValue);
    void values(int aValue, int aVisibleAmount, int aMin, int aMax);

    bool handleScrollKeys(const XKeyEvent &key);
    bool handleScrollMouse(const XButtonEvent &button);

private:
    Orientation fOrientation;
    int fMaximum;
    int fMinimum;
    int fValue;
    int fVisibleAmount;
    int fUnitIncrement;
    int fBlockIncrement;
public:
    void scroll(int delta);
    void move(int pos);

    virtual void paint(Graphics &g, int x, int y, unsigned int width, unsigned int height);
    virtual void handleButton(const XButtonEvent &button);
    virtual void handleMotion(const XMotionEvent &motion);
    virtual bool handleTimer(YTimer *timer);
    virtual void handleDNDEnter();
    virtual void handleDNDLeave();
    virtual void handleDNDPosition(int x, int y);
    void scrollBarListener(Listener *notify) { fListener = notify; }
private:
    enum ScrollOp {
        goUp, goDown, goPageUp, goPageDown, goPosition, goNone
    } fScrollTo;

    void doScroll();
    void coord(int &beg, int &end, int &min, int &max, int &nn);
    ScrollOp op(int x, int y);

    int fGrabDelta;
    Listener *fListener;
    bool fDNDScroll;
    static YTimer *fScrollTimer;
};

#endif
