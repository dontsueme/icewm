/*
 * IceWM
 *
 * Copyright (C) 1997-2001 Marko Macek
 */
#include "config.h"

#ifndef LITE

#include "ylib.h"
#include "wmminiicon.h"

#include "wmframe.h"
#include "yapp.h"

#include <string.h>

static YFont *minimizedWindowFont = 0;
static YColor *normalMinimizedWindowBg = 0;
static YColor *normalMinimizedWindowFg = 0;
static YColor *activeMinimizedWindowBg = 0;
static YColor *activeMinimizedWindowFg = 0;

MiniIcon::MiniIcon(YWindow *aParent, YFrameWindow *frame): YWindow(aParent) {
    if (minimizedWindowFont == 0)
        minimizedWindowFont = YFont::font(minimizedWindowFontName);
    if (normalMinimizedWindowBg == 0)
        normalMinimizedWindowBg = new YColor(clrNormalMinimizedWindow);
    if (normalMinimizedWindowFg == 0)
        normalMinimizedWindowFg = new YColor(clrNormalMinimizedWindowText);
    if (activeMinimizedWindowBg == 0)
        activeMinimizedWindowBg = new YColor(clrActiveMinimizedWindow);
    if (activeMinimizedWindowFg == 0)
        activeMinimizedWindowFg = new YColor(clrActiveMinimizedWindowText);

    fFrame = frame;
    selected = 0;
    geometry(0, 0, 120, 24);
    
    toolTip(this->frame()->client()->iconTitle());
}

MiniIcon::~MiniIcon() {
}

void MiniIcon::paint(Graphics &g, int /*x*/, int /*y*/, unsigned int /*width*/, unsigned int /*height*/) {
#ifdef CONFIG_TASKBAR
    bool focused = frame()->focused();
    YColor *bg = focused ? activeMinimizedWindowBg : normalMinimizedWindowBg;;
    YColor *fg = focused ? activeMinimizedWindowFg : normalMinimizedWindowFg;;
    int tx = 2;
    int x, y, w, h;

    g.color(bg);
    g.draw3DRect(0, 0, width() - 1, height() - 1, true);
    g.fillRect(1, 1, width() - 2, height() - 2);

    x = tx; y = 2;
    w = width() - 6;
    h = height() - 6;

    if (selected == 2) {
        g.color(bg->darker());
        g.draw3DRect(x, y, w, h, false);
        g.fillRect(x + 1, y + 1, w - 1, h - 1);
    } else {
        g.color(bg->brighter());
        g.drawRect(x + 1, y + 1, w, h);
        g.color(bg->darker());
        g.drawRect(x, y, w, h);
        g.color(bg);
        g.fillRect(x + 2, y + 2, w - 2, h - 2);
    }

    if (frame()->updateClientIcon() &&
        frame()->updateClientIcon()->small()) {
        //int y = (height() - 3 - frame()->updateClientIcon()->small()->height()) / 2;
        g.drawImage(frame()->updateClientIcon()->small(), 2 + tx + 1, 4);
    }

    const char *str = frame()->client()->iconTitle();

    if (strIsEmpty(str))
        str = frame()->client()->windowTitle();

    if (str) {
        g.color(fg);
        YFont *font = minimizedWindowFont;
        if (font) {
            g.font(font);
            int ty = (height() - 1 + font->height()) / 2 - font->descent();
            if (ty < 2) ty = 2;

	    g.drawStringEllipsis(tx + 4 + YIcon::sizeSmall + 2, ty,
				 str, w - 4 - YIcon::sizeSmall - 4);
        }
    }
#endif
}

void MiniIcon::handleButton(const XButtonEvent &button) {
    if (button.type == ButtonPress) {
        if (!(button.state & ControlMask) &&
            (buttonRaiseMask & (1 << (button.button - 1))))
            frame()->wmRaise();
        manager->focus(frame(), false);
        if (button.button == 1) {
            selected = 2;
            repaint();
        }
    } else if (button.type == ButtonRelease) {
        if (button.button == 1) {
            if (selected == 2) {
                if (button.state & app->AltMask) {
                    frame()->wmLower();
                } else {
                    if (!(button.state & ControlMask))
                        frame()->wmRaise();
                    frame()->activate();
                }
            }
            selected = 0;
            repaint();
        }
    }
    YWindow::handleButton(button);
}

void MiniIcon::handleClick(const XButtonEvent &up, int /*count*/) {
    if (up.button == 3) {
        frame()->popupSystemMenu(up.x_root, up.y_root, -1, -1,
                                 YPopupWindow::pfCanFlipVertical |
                                 YPopupWindow::pfCanFlipHorizontal |
                                 YPopupWindow::pfPopupMenu);
    }
}

void MiniIcon::handleCrossing(const XCrossingEvent &crossing) {
    if (selected > 0) {
        if (crossing.type == EnterNotify) {
            selected = 2;
            repaint();
        } else if (crossing.type == LeaveNotify) {
            selected = 1;
            repaint();
        }
    }

    YWindow::handleCrossing(crossing);
}

void MiniIcon::handleDrag(const XButtonEvent &down, const XMotionEvent &motion) {
    if (down.button != 1) {
        int x = motion.x_root - down.x;
        int y = motion.y_root - down.y;

        //x += down.x;
        //y += down.y;

        long l = frame()->layer();
        int mx = manager->minX(l), Mx = manager->maxX(l);
        int my = manager->minY(l), My = manager->maxY(l);

        if (x + int(width()) >= Mx) x = Mx - width();
        if (y + int(height()) >= My) y = My - height();
        if (x < mx) x = mx;
        if (y < my) y = my;

        frame()->position(x, y);
    }
}
#endif
