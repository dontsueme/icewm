/*  IceWM - The task and the tray pane.
 *  Copyright (C) 2001 The Authors of IceWM
 *
 *  Release under terms of the GNU Library General Public License
 */

#include "config.h"

#ifdef CONFIG_TASKBAR

#include "ylib.h"
#include "ypixbuf.h"
#include "atasks.h"
#include "wmtaskbar.h"
#include "prefs.h"
#include "yapp.h"
#include "wmmgr.h"
#include "wmframe.h"
#include "wmwinlist.h"

#include <string.h>

extern YColor *taskBarBg;

static YColor *normalFg(NULL);
static YColor *normalBg(NULL);
static YColor *activeFg(NULL);
static YColor *activeBg(NULL);
static YColor *minimizedFg(NULL);
static YColor *minimizedBg(NULL);
static YColor *invisibleFg(NULL);
static YColor *invisibleBg(NULL);
static YFont *normalFont(NULL);
static YFont *activeFont(NULL);

#ifdef CONFIG_GRADIENTS	
static YPixbuf *minimizedGradient(NULL);
static YPixbuf *activeGradient(NULL);
static YPixbuf *normalGradient(NULL);
#endif

YTimer *YFrameProxy::fRaiseTimer(NULL);

/******************************************************************************/
/******************************************************************************/

/**
 * Initialize a new frame proxy object
 */

YFrameProxy::YFrameProxy(YWindow *parent, YClientPeer *peer):
    YWindow(parent),
    fPeer(peer), fState(tsNormal), fShown(false) {
    if (peer) toolTip(peer->title());
    //dnd(true);
}

/**
 * Destruct a frame proxy object
 */

YFrameProxy::~YFrameProxy() {
    if (fRaiseTimer && fRaiseTimer->timerListener() == this) {
        fRaiseTimer->stop();
        fRaiseTimer->timerListener(NULL);
    }
}

/**
 * Handle button events send to the frame proxy.
 *
 * Button1, Button2 pressed: draw pressed
 * Button1 released: - minimize if visible or Ctrl hold
 *                   - limit to current workspace if Shift hold
 *                   - activate otherwise
 * Button2 released: - lower if visible or Ctrl hold
 *                   - show on current workspace if Shift hold
 *                   - activate otherwise
 */

void YFrameProxy::handleButton(const XButtonEvent &button) {
    YWindow::handleButton(button);

    switch (button.type) {
        case ButtonPress:
            if (button.button == 1 || button.button == 2) {
                fState = tsPressed;
                repaint();
            }
            break;

        case ButtonRelease:
            if (peer() && fState == tsPressed) {
                if (button.button == 1) {
                    if (peer()->visibleNow() &&
                        (!peer()->canRaise() || (button.state & ControlMask)))
                        peer()->wmMinimize();
                    else {
                        if (button.state & ShiftMask)
                            peer()->wmOccupyOnlyWorkspace(manager->activeWorkspace());

                        peer()->activateWindow(true);
                    }
                } else if (button.button == 2) {
                    if (peer()->visibleNow() &&
                        (!peer()->canRaise() || (button.state & ControlMask)))
                        peer()->wmLower();
                    else {
                        if (button.state & ShiftMask)
                            peer()->wmOccupyWorkspace(manager->activeWorkspace());

                        peer()->activateWindow(true);
                    }
                }
            }

            fState = tsNormal;
            repaint();
            break;
    }
}

/**
 * Handle mouse crossing events send to the frame proxy.
 *
 * Ensure that the button is drawn pressed only if armed.
 */

void YFrameProxy::handleCrossing(const XCrossingEvent &crossing) {
    if (fState != tsNormal) {
        if (crossing.type == EnterNotify) {
            fState = tsPressed;
            repaint();
        } else if (crossing.type == LeaveNotify) {
            fState = tsArmed;
            repaint();
        }
    }

    YWindow::handleCrossing(crossing);
}

/**
 * Handle button click events send to the frame proxy.
 *
 * Display the context menu if requested.
 */

void YFrameProxy::handleClick(const XButtonEvent &up, int /*count*/) {
msg("%s: this:%p|peer:%p", __PRETTY_FUNCTION__, this,peer());
    if (peer() && up.button == 3)
        peer()->popupSystemMenu(up.x_root, up.y_root, -1, -1,
                                YPopupWindow::pfCanFlipVertical |
                                YPopupWindow::pfCanFlipHorizontal |
                                YPopupWindow::pfPopupMenu);
}

void YFrameProxy::handleDNDEnter() {
    if (NULL == fRaiseTimer)
        fRaiseTimer = new YTimer(autoRaiseDelay);

    if (fRaiseTimer) {
        fRaiseTimer->timerListener(this);
        fRaiseTimer->start();
    }

    fState = tsDndDrop;
    repaint();
}

void YFrameProxy::handleDNDLeave() {
    if (fRaiseTimer && fRaiseTimer->timerListener() == this) {
        fRaiseTimer->stop();
        fRaiseTimer->timerListener(NULL);
    }

    fState = tsNormal;
    repaint();
}

bool YFrameProxy::handleTimer(YTimer *timer) {
    if (peer() && timer->timerListener() == this)
        peer()->wmRaise();

    return false;
}

/******************************************************************************/

/**
 * Add a proxy object to the proxy pane.
 */

YFrameProxy *YProxyPane::add(YFrameWindow *frame, bool shown) {
#ifdef CONFIG_WINLIST
    if (frame->client() == windowList) return NULL;
#endif
    if (frame->client() == taskBar) return NULL;

    YFrameProxy *proxy(createProxy(frame));

    if (proxy) {
        add(proxy);
        proxy->shown(shown);
        proxy->show();

        relayout();
    }

    return proxy;
}

/**
 * Remove a proxy object from the proxy pane.
 */

void YProxyPane::remove(YFrameWindow *frame) {
    for (ProxyList::Iterator proxy(proxies); proxy; ++proxy)
        if ((*proxy)->peer() == frame) {
            (*proxy)->hide();
            proxies.destroy(proxy);
            relayout();
            return;
        }
}

/**
 * Count visible proxies.
 */

unsigned YProxyPane::visualCount() const {
    unsigned count(0);

    for (ProxyList::Iterator proxy(proxies); proxy; ++proxy)
        if ((*proxy)->shown()) ++count;

    return count;
}

/**
 * Reposition the proxy objects.
 */

void YProxyPane::relayoutNow() {
    if (fNeedRelayout) {
        fNeedRelayout = false;

        unsigned const count(max(minimalCount(), visualCount()));

        unsigned const w(count ? (width() - 2) / count - 2 : 0);
        unsigned const h(height() - 2 * margin());
        int x(0), y(margin());

        for (ProxyList::Iterator p(proxies); p; ++p) {
            YFrameProxy & proxy(**p);
            if (proxy.shown()) {
                proxy.geometry(x, y, w - spacing(), h);
                proxy.show();
                x += w;
            } else
                proxy.hide();
        }
    }
}

/**
 * Handle button click events send to the proxy pane.
 *
 * Display the context menu if requested.
 */

void YProxyPane::handleClick(const XButtonEvent &up, int count) {
    if (up.button == 3 && count == 1 && IS_BUTTON(up.state, Button3Mask)) {
        taskBar->contextMenu(up.x_root, up.y_root);
    }
}

/******************************************************************************/
/******************************************************************************/

/**
 * Initialize a task pane button.
 */

TaskButton::TaskButton(YWindow *parent, YClientPeer *peer):
YFrameProxy(parent, peer) {
    STATIC_VARIABLE(normalBg,    new YColor(clrNormalTaskBarApp))
    STATIC_VARIABLE(normalFg,    new YColor(clrNormalTaskBarAppText))
    STATIC_VARIABLE(activeBg,    new YColor(clrActiveTaskBarApp))
    STATIC_VARIABLE(activeFg,    new YColor(clrActiveTaskBarAppText))
    STATIC_VARIABLE(minimizedBg, new YColor(clrNormalTaskBarApp))
    STATIC_VARIABLE(minimizedFg, new YColor(clrMinimizedTaskBarAppText))
    STATIC_VARIABLE(invisibleBg, new YColor(clrNormalTaskBarApp))
    STATIC_VARIABLE(invisibleFg, new YColor(clrInvisibleTaskBarAppText))
    STATIC_VARIABLE(normalFont,  YFont::font(normalTaskBarFontName))
    STATIC_VARIABLE(activeFont,  YFont::font(activeTaskBarFontName))
}

/**
 * Paint a task pane button.
 */

void TaskButton::paint(Graphics &g, int /*x*/, int /*y*/,
                                    unsigned /*width*/, unsigned /*height*/) {
    YColor *bg, *fg;
    YPixmap *bgPix;
#ifdef CONFIG_GRADIENTS	
    YPixbuf *bgGrad;
#endif

    int p(0);

    if (!peer()->visibleNow()) {
        bg = invisibleBg;
        fg = invisibleFg;
        bgPix = taskbackPixmap;
#ifdef CONFIG_GRADIENTS	
	bgGrad = taskbackPixbuf;
#endif
    } else if (peer()->isMinimized()) {
        bg = minimizedBg;
        fg = minimizedFg;
        bgPix = taskbuttonminimizedPixmap;
#ifdef CONFIG_GRADIENTS	
	bgGrad = taskbuttonminimizedPixbuf;
#endif
    } else if (peer()->focused()) {
        bg = activeBg;
        fg = activeFg;
        bgPix = taskbuttonactivePixmap;
#ifdef CONFIG_GRADIENTS	
	bgGrad = taskbuttonactivePixbuf;
#endif
    } else {
        bg = normalBg;
        fg = normalFg;
        bgPix = taskbuttonPixmap;
#ifdef CONFIG_GRADIENTS	
	bgGrad = taskbuttonPixbuf;
#endif
    }

    if (state() == tsDndDrop) {
        p = 2;
        g.color(YColor::black);
        g.drawRect(0, 0, width() - 1, height() - 1);
        g.color(bg);
        g.fillRect(1, 1, width() - 2, height() - 2);
    } else {
        g.color(bg);

        bool const pressed(!peer()->focused() && state() != tsPressed);
        p = pressed && wmLook != lookMetal ? 1 : 2;

        if (wmLook == lookMetal)
            g.drawBorderM(0, 0, width() - 1, height() - 1, pressed);
        else if (wmLook == lookGtk)
            g.drawBorderG(0, 0, width() - 1, height() - 1, pressed);
        else
            g.drawBorderW(0, 0, width() - 1, height() - 1, pressed);

	int const dp(wmLook == lookMetal ? 2 : p);
	unsigned const ds(wmLook == lookMetal ? 4 : 3);

	if (width() > ds && height() > ds) {
#ifdef CONFIG_GRADIENTS
	    if (bgGrad)
                g.drawGradient(*bgGrad, dp, dp, width() - ds, height() - ds);
	    else
#endif
            if (bgPix)
                g.fillPixmap(bgPix, dp, dp, width() - ds, height() - ds);
            else
                g.fillRect(dp, dp, width() - ds, height() - ds);
	}
    }

    YIcon *icon(peer()->icon());

    if (taskBarShowWindowIcons && icon) {
        YIcon::Image *small(icon->small());

        if (small) {
            int const y((height() - 3 - small->height() - 
			((wmLook == lookMetal) ? 1 : 0)) / 2);
            g.drawImage(small, p + 1, p + 1 + y);
        }
    }

    const char *str(peer()->title());
    if(strIsEmpty(str)) str = peer()->iconTitle();

    if (str) {
        YFont * font(peer()->focused() ? activeFont : normalFont);

        if (font) {
	    g.color(fg);
            g.font(font);

	    int const iconSize(taskBarShowWindowIcons ? YIcon::sizeSmall : 0);
	    int const tx(3 + iconSize);
            int const ty(max(2U, (height() + font->height() -
				 (wmLook == lookMetal ? 2 : 1)) / 2 - 
				 font->descent()));
	    int const wm(width() - p - 3 - iconSize - 3);

            g.drawStringEllipsis(p + tx, p + ty, str, wm);
        }
    }
}

/******************************************************************************/

/**
 * Add a frame window to the task pane.
 */

TaskButton *TaskPane::add(YFrameWindow *frame) {
    bool const shown(frame->visibleOn(manager->activeWorkspace()) ||
                     taskBarShowAllWindows);

    return static_cast<TaskButton*>(YProxyPane::add(frame, shown));
}

/**
 * Paint the task pane
 */

void TaskPane::paint(Graphics &g, int /*x*/, int /*y*/,
                                  unsigned /*width*/, unsigned /*height*/) {
    g.color(taskBarBg);
    //g.draw3DRect(0, 0, width() - 1, height() - 1, true);

#ifdef CONFIG_GRADIENTS
    class YPixbuf * gradient(parent()->gradient());

    if (gradient)
        g.copyPixbuf(*gradient, x(), y(), width(), height(), 0, 0);
    else
#endif    
    if (taskbackPixmap)
        g.fillPixmap(taskbackPixmap, 0, 0, width(), height(), x(), y());
    else
        g.fillRect(0, 0, width(), height());
}

/******************************************************************************/
/******************************************************************************/

#ifdef CONFIG_TRAY

/**
 * Initialize a tray icon
 */

TrayIcon::TrayIcon(YWindow *parent, YClientPeer *peer):
YFrameProxy(parent, peer) {
    STATIC_VARIABLE(normalBg,    new YColor(clrNormalTaskBarApp))
    STATIC_VARIABLE(normalFg,    new YColor(clrNormalTaskBarAppText))
    STATIC_VARIABLE(activeBg,    new YColor(clrActiveTaskBarApp))
    STATIC_VARIABLE(activeFg,    new YColor(clrActiveTaskBarAppText))
    STATIC_VARIABLE(minimizedBg, new YColor(clrNormalTaskBarApp))
    STATIC_VARIABLE(minimizedFg, new YColor(clrMinimizedTaskBarAppText))
    STATIC_VARIABLE(invisibleBg, new YColor(clrNormalTaskBarApp))
    STATIC_VARIABLE(invisibleFg, new YColor(clrInvisibleTaskBarAppText))
    STATIC_VARIABLE(normalFont,  YFont::font(normalTaskBarFontName))
    STATIC_VARIABLE(activeFont,  YFont::font(activeTaskBarFontName))
}

/**
 * Paint a tray icon
 */

void TrayIcon::paint(Graphics &g, int /*x*/, int /*y*/,
                     	          unsigned /*width*/, unsigned /*height*/) {
    int sx(parent() ? x() + parent()->x() : x());
    int sy(parent() ? y() + parent()->y() : y());

#ifdef CONFIG_GRADIENTS	
    unsigned sw((parent() && parent()->parent() ? 
    		 parent()->parent() : this)->width());
    unsigned sh((parent() && parent()->parent() ? 
    		 parent()->parent() : this)->height());
#endif

    YColor *bg, *fg;
    YPixmap *bgPix;
#ifdef CONFIG_GRADIENTS	
    YPixbuf *bgGrad;
#endif

    if (!peer()->visibleNow()) {
        bg = invisibleBg;
        fg = invisibleFg;
        bgPix = taskbackPixmap;
#ifdef CONFIG_GRADIENTS	
	bgGrad = gradient();
#endif
    } else if (peer()->isMinimized()) {
        bg = minimizedBg;
        fg = minimizedFg;
        bgPix = taskbuttonminimizedPixmap;
#ifdef CONFIG_GRADIENTS	
	if (minimizedGradient == NULL && taskbuttonminimizedPixbuf)
	    minimizedGradient = new YPixbuf(*taskbuttonminimizedPixbuf, sw, sh);
	bgGrad = minimizedGradient;
#endif
    } else if (peer()->focused()) {
        bg = activeBg;
        fg = activeFg;
        bgPix = taskbuttonactivePixmap;
#ifdef CONFIG_GRADIENTS	
	if (activeGradient == NULL && taskbuttonactivePixbuf)
	    activeGradient = new YPixbuf(*taskbuttonactivePixbuf, sw, sh);
	bgGrad = activeGradient;
#endif
    } else {
        bg = normalBg;
        fg = normalFg;
        bgPix = taskbuttonPixmap;
#ifdef CONFIG_GRADIENTS	
	if (normalGradient == NULL && taskbuttonPixbuf)
	    normalGradient = new YPixbuf(*taskbuttonPixbuf, sw, sh);
	bgGrad = normalGradient;
#endif
    }

    if (state() == tsDndDrop) {
        g.color(YColor::black);
        g.drawRect(0, 0, width() - 1, height() - 1);
        g.color(bg);
        g.fillRect(1, 1, width() - 2, height() - 2);
    } else {
	if (width() > 0 && height() > 0) {
#ifdef CONFIG_GRADIENTS
	    if (bgGrad)
                g.copyPixbuf(*bgGrad, sx, sy, width(), height(), 0, 0);
	    else
#endif
            if (bgPix)
                g.fillPixmap(bgPix, 0, 0, width(), height(), sx, sy);
            else {
		g.color(bg);
                g.fillRect(0, 0, width(), height());
	    }
	}
    }

    YIcon *icon(peer()->icon());

    if (icon) {
        YIcon::Image *small(icon->small());
        if (small) g.drawImage(small, 2, 2);
    }
}

/******************************************************************************/

/**
 * Add a frame window to the tray pane.
 */

TrayIcon *TrayPane::add(YFrameWindow *frame) {
    bool const shown(frame->visibleOn(manager->activeWorkspace()) ||
                     trayShowAllWindows);

    return static_cast<TrayIcon*>(YProxyPane::add(frame, shown));
}

/**
 * Calculate the required with for this tray pane
 */

unsigned TrayPane::requiredWidth() {
    unsigned const count(visualCount());
    return (count ? 4 + count * (height() - 4) : 1);
}

/**
 * Paint the tray pane
 */

void TrayPane::paint(Graphics &g, int /*x*/, int /*y*/,
                                  unsigned /*width*/, unsigned /*height*/) {
    int const w(width());
    int const h(height());

    g.color(taskBarBg);

#ifdef CONFIG_GRADIENTS
    YPixbuf * gradient(parent() ? parent()->gradient() : NULL);

    if (gradient)
        g.copyPixbuf(*gradient, x(), y(), w, h, 0, 0);
    else 
#endif    
    if (taskbackPixmap)
        g.fillPixmap(taskbackPixmap, 0, 0, w, h, x(), y());
    else
        g.fillRect(0, 0, w, h);
    
    if (trayDrawBevel && w > 1)
	if (wmLook == lookMetal)
	    g.draw3DRect(1, 1, w - 2, h - 2, false);
	else
	    g.draw3DRect(0, 0, w - 1, h - 1, false);
}

#endif // CONFIG_TRAY
#endif // CONFIG_TASKBAR
