#ifndef __WMBUTTON_H
#define __WMBUTTON_H

#include "yactionbutton.h"
#include "ymenu.h"

class YFrameWindow;

class YFrameButton: public YButton {
public:
    YFrameButton(YWindow *parent, YFrameWindow *frame, YAction *action, YAction *action2 = 0);
    virtual ~YFrameButton();

    virtual void paint(Graphics &g, int x, int y, unsigned int width, unsigned int height);
    virtual void paintFocus(Graphics &g, int x, int y, unsigned int w, unsigned int h);
    virtual void handleButton(const XButtonEvent &button);
    virtual void handleClick(const XButtonEvent &up, int count);
    virtual void handleBeginDrag(const XButtonEvent &down, const XMotionEvent &motion);

    virtual void actionPerformed(YAction *action, unsigned int modifiers);
    void actions(YAction *action, YAction *action2 = NULL);
    virtual void updatePopup();

    YPixmap *image(int pn) const;
    YFrameWindow *frame() const { return fFrame; };
private:
    YFrameWindow *fFrame;
    YAction *fAction;
    YAction *fAction2;
};

extern YPixmap *closePixmap[2];
extern YPixmap *minimizePixmap[2];
extern YPixmap *maximizePixmap[2];
extern YPixmap *restorePixmap[2];
extern YPixmap *hidePixmap[2];
extern YPixmap *rollupPixmap[2];
extern YPixmap *rolldownPixmap[2];
extern YPixmap *depthPixmap[2];

#endif
