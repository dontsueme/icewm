/*
 * IceWM
 *
 * Copyright (C) 1997-2001 Marko Macek
 */
#include "config.h"
#include "ytooltip.h"

#ifdef CONFIG_TOOLTIP
#include "base.h"
#include "prefs.h"

#include <string.h>

YColor *YToolTip::toolTipBg = 0;
YColor *YToolTip::toolTipFg = 0;

YFont *YToolTip::toolTipFont = 0;
YTimer *YToolTip::fToolTipVisibleTimer = 0;

YToolTip::YToolTip(YWindow *aParent): YWindow(aParent) {
    if (toolTipBg == 0)
        toolTipBg = new YColor(clrToolTip);
    if (toolTipFg == 0)
        toolTipFg = new YColor(clrToolTipText);
    if (toolTipFont == 0)
        toolTipFont = YFont::font(toolTipFontName);

    fText = 0;
    style(wsOverrideRedirect);
}

YToolTip::~YToolTip() {
    delete fText; fText = 0;
    if (fToolTipVisibleTimer) {
        if (fToolTipVisibleTimer->timerListener() == this) {
            fToolTipVisibleTimer->timerListener(NULL);
            fToolTipVisibleTimer->stop();
        }
    }
}

void YToolTip::paint(Graphics &g, int /*x*/, int /*y*/, unsigned int /*width*/, unsigned int /*height*/) {
    g.color(toolTipBg);
    g.fillRect(0, 0, width(), height());
    g.color(YColor::black);
    g.drawRect(0, 0, width() - 1, height() - 1);
    if (fText) {
        int y = toolTipFont->ascent() + 2;
        g.font(toolTipFont);
        g.color(toolTipFg);
	g.drawStringMultiline(3, y, fText);
    }
}

void YToolTip::text(const char *tip) {
    delete fText; fText = 0;
    if (tip) {
        fText = newstr(tip);
        if (fText) {
	    YDimension const size(toolTipFont->multilineAlloc(fText));
            this->size(size.w + 6, size.h + 7);
        } else {
            size(20, 20);
        }

        //!!! merge with below code in locate
        int x = this->x();
        int y = this->y();
        if (x + width() >= desktop->width())
            x = desktop->width() - width();
        if (y + height() >= desktop->height())
            y = desktop->height() - height();
        if (y < 0)
            y = 0;
        if (x < 0)
            x = 0;
        position(x, y);
    }
    repaint();
}

bool YToolTip::handleTimer(YTimer *t) {
    if (t == fToolTipVisibleTimer && fToolTipVisibleTimer)
        hide();
    else
        display();
    return false;
}

void YToolTip::display() {
    raise();
    show();

    if (!fToolTipVisibleTimer && ToolTipTime > 0)
        fToolTipVisibleTimer = new YTimer(ToolTipTime);

    if (fToolTipVisibleTimer) {
        fToolTipVisibleTimer->timerListener(this);
        fToolTipVisibleTimer->start();
    }
}

void YToolTip::locate(YWindow *w, const XCrossingEvent &/*crossing*/) {
    int x, y;

    x = w->width() / 2;
    y = w->height();
    w->mapToGlobal(x, y);
    x -= width() / 2;
    if (x + width() >= desktop->width())
        x = desktop->width() - width();
    if (y + height() >= desktop->height())
        y -= height() + w->height();
    if (y < 0)
        y = 0;
    if (x < 0)
        x = 0;
    position(x, y);
}
#endif
