/*
 * IceWM
 *
 * Copyright (C) 1997-2001 Marko Macek
 */
#include "config.h"

#ifndef LITE

#include "yscrollview.h"

#include "ylistbox.h"
#include "yscrollbar.h"

#include "yapp.h"
#include "prefs.h"

extern YColor *scrollBarBg;


YScrollView::YScrollView(YWindow *aParent):
YWindow(aParent),
fScrollVert(YScrollBar::Vertical, this),
fScrollHoriz(YScrollBar::Horizontal, this),
fScrollable(NULL) {
    fScrollVert.show();
    fScrollHoriz.show();
}

void YScrollView::view(YScrollable *scrollable) {
    fScrollable = scrollable;
}

void YScrollView::gap(int &dx, int &dy) {
    unsigned const cw(fScrollable->contentWidth());
    unsigned const ch(fScrollable->contentHeight());

    dx = dy = 0;

    if (width() < cw) {
        dy = scrollBarHeight;
        if (height() - dy < ch) dx = scrollBarWidth;
    } else if (height() < ch) {
        dx = scrollBarWidth;
        if (width() - dx < cw) dy = scrollBarHeight;
    }
}

void YScrollView::layout() {
    if (fScrollable) {
        int dx, dy;
        gap(dx, dy);

        int const w(width()), h(height());
        int const sw(max(0, w - dx));
        int const sh(max(0, h - dy));

        fScrollVert.geometry(w - dx, 0, dx, sh);
        fScrollHoriz.geometry(0, h - dy, sw, dy);

        dx = min(dx, w);
        dy = min(dy, h);

        fScrollable->window()->geometry(0, 0, w - dx, h - dy);
    }
}

void YScrollView::configure(const int x, const int y, 
			    const unsigned width, const unsigned height, 
			    const bool resized) {
    YWindow::configure(x, y, width, height, resized);
    if (resized) layout();
}

void YScrollView::paint(Graphics &g, int x, int y, unsigned w, unsigned h) {
    int dx, dy;
    
    gap(dx, dy);
    
    g.color(scrollBarBg);
    if (dx && dy) g.fillRect(width() - dx, height() - dy, dx, dy);

    YWindow::paint(g, x, y, w, h);
}

#endif
