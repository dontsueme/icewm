#ifndef __YSCROLLBAR_H
#define __YSCROLLBAR_H

#include "ywindow.h"
#include "ytimer.h"
#include "yconfig.h"

#pragma interface

#define SCROLLBAR_SIZE 16 // !!! remove
#define SCROLLBAR_MIN  8

class YScrollBar;

class YScrollBarListener {
public:
    virtual void scroll(YScrollBar *scroll, int delta) = 0;
    virtual void move(YScrollBar *scroll, int pos) = 0;
};

class YScrollBar: public YWindow, public YTimerListener {
public:
    enum Orientation {
        Vertical, Horizontal
    };

    YScrollBar(YWindow *aParent);
    YScrollBar(Orientation anOrientation, YWindow *aParent);
    YScrollBar(Orientation anOrientation,
               int aValue, int aVisibleAmount, int aMin, int aMax,
               YWindow *aParent);
    virtual ~YScrollBar();

    Orientation getOrientation() const { return fOrientation; }
    int getMaximum() const { return fMaximum; }
    int getMinimum() const { return fMinimum; }
    int getVisibleAmount() const { return fVisibleAmount; }
    int getUnitIncrement() const { return fUnitIncrement; }
    int getBlockIncrement() const { return fBlockIncrement; }
    int getValue() const { return fValue; }

    void setOrientation(Orientation anOrientation);
    void setMaximum(int aMaximum);
    void setMinimum(int aMinimum);
    void setVisibleAmount(int aVisibleAmount);
    void setUnitIncrement(int anUnitIncrement);
    void setBlockIncrement(int aBlockIncrement);
    void setValue(int aValue);
    void setValues(int aValue, int aVisibleAmount, int aMin, int aMax);

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
    virtual void handleDNDEnter(int nTypes, Atom *types);
    virtual void handleDNDLeave();
    virtual bool handleDNDPosition(int x, int y, Atom *action);
    void setScrollBarListener(YScrollBarListener *notify) { fListener = notify; }
private:
    enum ScrollOp {
        goUp, goDown, goPageUp, goPageDown, goPosition, goNone
    } fScrollTo;

    void doScroll();
    void drawArrow(Graphics &g, bool forward);
    void getCoord(int &beg, int &end, int &min, int &max, int &nn);
    ScrollOp getOp(int x, int y);

    int fGrabDelta;
    YScrollBarListener *fListener;
    bool fDNDScroll;
    YTimer fScrollTimer;

    static YNumPrefProperty gScrollBarStartDelay;
    static YNumPrefProperty gScrollBarDelay;
    static YColorPrefProperty gScrollBarBg;
    static YColorPrefProperty gScrollBarArrow;
    static YColorPrefProperty gScrollBarSlider;
private: // not-used
    YScrollBar(const YScrollBar &);
    YScrollBar &operator=(const YScrollBar &);
};

#endif
