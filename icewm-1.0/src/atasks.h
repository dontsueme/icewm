/*  IceWM - The task and the tray pane.
 *  Copyright (C) 2001 The Authors of IceWM
 *
 *  Release under terms of the GNU Library General Public License
 */

#ifndef __ATASKS_H
#define __ATASKS_H

#ifdef CONFIG_TASKBAR

#include "ywindow.h"
#include "ytimer.h"
#include "wmframe.h"

/******************************************************************************/
/******************************************************************************/

/**
 * Common code for objects implementing quick access to toplevel windows.
 */

class YFrameProxy:
public YWindow,
public YTimer::Listener {
public:
    enum State {
        tsNormal,
        tsArmed,
        tsPressed,
        tsDndDrop
    };

    YFrameProxy(YWindow *parent, YClientPeer *peer);
    virtual ~YFrameProxy();

    virtual bool isFocusTraversable() { return true; }

    virtual void handleButton(const XButtonEvent &button);
    virtual void handleClick(const XButtonEvent &up, int count);
    virtual void handleCrossing(const XCrossingEvent &crossing);
    virtual void handleDNDEnter(void);
    virtual void handleDNDLeave(void);
    virtual bool handleTimer(YTimer *t);

    YClientPeer *peer() const { return fPeer; }

    void state(State state) { fState = state; }
    bool state() const { return fState; }
    
    void shown(bool shown) { fShown = shown; }
    bool shown() const { return fShown; }

private:
    YClientPeer *fPeer;
    State fState;
    bool fShown;

    static YTimer *fRaiseTimer;
};

/**
 * Base class for frame proxy containers
 */

class YProxyPane:
public YWindow {
public:
    YProxyPane(YWindow *parent): YWindow(parent), fNeedRelayout(true) {}
    virtual ~YProxyPane() {}

    void add(YFrameProxy *proxy) { proxies.append(proxy); }
    void remove(YFrameProxy *proxy) { proxies.remove(proxy); }

    YFrameProxy *add(YFrameWindow *frame, bool shown = true);
    void remove(YFrameWindow *frame);

    unsigned visualCount() const;

    void relayout() { fNeedRelayout = true; }
    void relayoutNow();

    virtual void handleClick(const XButtonEvent &up, int count);

protected:
    virtual YFrameProxy *createProxy(YFrameWindow *frame) = 0;
    virtual unsigned minimalCount(void) = 0;
    virtual unsigned margin(void) = 0;
    virtual unsigned spacing(void) = 0;

    typedef YDoubleList<YFrameProxy> ProxyList;
    ProxyList proxies;

    bool fNeedRelayout;
};

/******************************************************************************/
/******************************************************************************/

/**
 * Task pane buttons representing their frame by text and an optional icon.
 */

class TaskButton:
public YFrameProxy {
public:
    TaskButton(YWindow *parent, YClientPeer *peer);
    virtual void paint(Graphics &g, int x, int y, unsigned int width, unsigned int height);
};

/**
 * The task pane displays buttons for each window on the current workspace.
 */

class TaskPane:
public YProxyPane {
public:
    TaskPane(YWindow *parent): YProxyPane(parent) {}

    TaskButton *add(YFrameWindow *frame);
    virtual void paint(Graphics &g, int x, int y, unsigned w, unsigned h);

protected:
    virtual YFrameProxy *createProxy(YFrameWindow *frame) {
        return new TaskButton(this, frame);
    }

    virtual unsigned minimalCount(void) { return 3; }
    virtual unsigned margin(void) { return 0; }
    virtual unsigned spacing(void) { return 2; }
};

/******************************************************************************/
/******************************************************************************/

#ifdef CONFIG_TRAY

/**
 * Little icons representing frames on the current workspace.
 * This kind if tray icons is put into the icon tray by IceWM's tray option.
 */

class TrayIcon:
public YFrameProxy {
public:
    TrayIcon(YWindow *parent, YClientPeer *peer);
    virtual void paint(Graphics &g, int x, int y, unsigned w, unsigned h);
};


/**
 * The icon tray displays icons for selected window in the taskbar.
 */

class TrayPane:
public YProxyPane {
public:
    TrayPane(YWindow *parent): YProxyPane(parent) {}

    TrayIcon *add(YFrameWindow *frame);
/*
    TrayWindow *add(YTrayWindow *frame);
*/    

    unsigned requiredWidth();
    virtual void paint(Graphics &g, int x, int y, unsigned w, unsigned h);

protected:
    virtual YFrameProxy *createProxy(YFrameWindow *frame) {
        return new TrayIcon(this, frame);
    }

    virtual unsigned minimalCount(void) { return 0; }
    virtual unsigned margin(void) { return 2; }
    virtual unsigned spacing(void) { return 0; }
};

#endif // CONFIG_TRAY
#endif // CONFIG_TASKBAR
#endif // __ATASKS_H
