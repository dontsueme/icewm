#ifndef __YSCROLLVIEW_H
#define __YSCROLLVIEW_H

#include "ywindow.h"
#include "yscrollbar.h"

class YScrollable {
public:
    virtual int contentWidth() = 0;
    virtual int contentHeight() = 0;

    virtual YWindow *window() = 0; // !!! hack ?
};

class YScrollView: public YWindow {
public:
    YScrollView(YWindow *aParent);
    virtual ~YScrollView() {}

    void view(YScrollable *l);

    YScrollBar &verticalScrollBar() { return fScrollVert; }
    YScrollBar &horizontalScrollBar() { return fScrollHoriz; }
    YScrollable *scrollable() { return fScrollable; }
    
    void layout();
    virtual void configure(const int x, const int y, 
    			   const unsigned width, const unsigned height,
			   const bool resized);
    virtual void paint(Graphics &g, int x, int y,
                       unsigned int width, unsigned int height);

protected:
    void gap(int &dx, int &dy);

private:
    YScrollBar fScrollVert;
    YScrollBar fScrollHoriz;
    YScrollable *fScrollable;
};

#endif
