/*
 * IceWM
 *
 * Copyright (C) 1997-2001 Marko Macek
 */

#include "config.h"
#include "yfull.h"
#include "wmframe.h"

#include "atasks.h"
#include "aaddressbar.h"
#include "wmaction.h"
#include "wmclient.h"
#include "wmcontainer.h"
#include "wmtitle.h"
#include "wmbutton.h"
#include "wmminiicon.h"
#include "wmswitch.h"
#include "wmtaskbar.h"
#include "wmwinlist.h"
#include "wmmgr.h"
#include "wmapp.h"
#include "ypixbuf.h"
#include "sysdep.h"

#include "intl.h"

static YColor *activeBorderBg = 0;
static YColor *inactiveBorderBg = 0;

YTimer *YFrameWindow::fAutoRaiseTimer = 0;
YTimer *YFrameWindow::fDelayFocusTimer = 0;

bool YFrameWindow::isButton(char c) {
    if (strchr(titleButtonsSupported, c) == 0)
        return false;
    if (strchr(titleButtonsRight, c) != 0 || strchr(titleButtonsLeft, c) != 0)
        return true;
    return false;
}

YFrameWindow::YFrameWindow(YWindow *parent, YFrameClient *client):
YWindow(parent) {
    if (activeBorderBg == 0)
        activeBorderBg = new YColor(clrActiveBorder);
    if (inactiveBorderBg == 0)
        inactiveBorderBg = new YColor(clrInactiveBorder);

    fClient = 0;
    fFocused = false;
    fNextFrame = fPrevFrame = NULL;
    fNextCreated = fPrevCreated = NULL;
    fPopupActive = 0;

    normalX = 0;
    normalY = 0;
    normalWidth = 1;
    normalHeight = 1;
    iconX = -1;
    iconY = -1;
    movingWindow = 0;
    sizingWindow = 0;
    indicatorsVisible = 0;
    fFrameFunctions = 0;
    fFrameDecorations = 0;
    fFrameOptions = 0;
    fFrameIcon = 0;
#ifdef CONFIG_TASKBAR
    fTaskButton = 0;
#endif
#ifdef CONFIG_TRAY
    fTrayIcon = 0;
#endif
#ifdef CONFIG_WINLIST
    fWinListItem = 0;
#endif
    fMiniIcon = 0;
    fTransient = 0;
    fNextTransient = 0;
    fOwner = 0;
    fManaged = false;
    fKillMsgBox = 0;

    style(wsOverrideRedirect);
    pointer(YApplication::leftPointer);

    PRECONDITION(client != 0);
    fClient = client;

    fWorkspace = manager->activeWorkspace();
    fLayer = WinLayerNormal;
#ifdef CONFIG_TRAY
    fTrayOption = IcewmTrayIgnore;
#endif
    fWinState = 0;
    fWinOptionMask = ~0;

    createPointerWindows();

    fClientContainer = new YClientContainer(this, this);
    fClientContainer->show();

    fTitleBar = new YFrameTitleBar(this, this);
    fTitleBar->show();

    if (!isButton('m')) /// optimize strchr (flags)
        fMaximizeButton = 0;
    else {
        fMaximizeButton = new YFrameButton(fTitleBar, this, actionMaximize, actionMaximizeVert);
        //fMaximizeButton->winGravity(NorthEastGravity);
        fMaximizeButton->show();
        fMaximizeButton->toolTip(_("Maximize"));
    }

    if (!isButton('i'))
        fMinimizeButton = 0;
    else {
        fMinimizeButton = new YFrameButton(fTitleBar, this,
#ifndef CONFIG_PDA
					   actionMinimize, actionHide);
#else
					   actionMinimize, 0);
#endif
        //fMinimizeButton->winGravity(NorthEastGravity);
        fMinimizeButton->toolTip(_("Minimize"));
        fMinimizeButton->show();
    }

    if (!isButton('x'))
        fCloseButton = 0;
    else {
        fCloseButton = new YFrameButton(fTitleBar, this, actionClose, actionKill);
        //fCloseButton->winGravity(NorthEastGravity);
        fCloseButton->toolTip(_("Close"));
        fCloseButton->show();
    }

#ifndef CONFIG_PDA
    if (!isButton('h'))
#endif
        fHideButton = 0;
#ifndef CONFIG_PDA
    else {
        fHideButton = new YFrameButton(fTitleBar, this, actionHide, actionHide);
        //fHideButton->winGravity(NorthEastGravity);
        fHideButton->toolTip(_("Hide"));
        fHideButton->show();
    }
#endif

    if (!isButton('r'))
        fRollupButton = 0;
    else {
        fRollupButton = new YFrameButton(fTitleBar, this, actionRollup, actionRollup);
        //fRollupButton->winGravity(NorthEastGravity);
        fRollupButton->toolTip(_("Rollup"));
        fRollupButton->show();
    }

    if (!isButton('d'))
        fDepthButton = 0;
    else {
        fDepthButton = new YFrameButton(fTitleBar, this, actionDepth, actionDepth);
        //fDepthButton->winGravity(NorthEastGravity);
        fDepthButton->toolTip(_("Raise/Lower"));
        fDepthButton->show();
    }

    if (!isButton('s'))
        fMenuButton = 0;
    else {
        fMenuButton = new YFrameButton(fTitleBar, this, NULL);
        fMenuButton->show();
        fMenuButton->actionListener(this);
    }

    updateFrameHints();
#ifndef LITE
    updateIcon();
#endif

    manage(client);
    insertFrame();

    defaultOptions();
    if (frameOptions() & foAllWorkspaces) sticky(true);

    addAsTransient();
    addTransients();

    if (!(frameOptions() & foFullKeys))
        grabKeys();
    fClientContainer->grabButtons();

#ifndef LITE
    if (minimizeToDesktop)
        fMiniIcon = new MiniIcon(this, this);
#endif
#ifdef CONFIG_WINLIST
    if (windowList && !(frameOptions() & foIgnoreWinList))
        fWinListItem = windowList->addWindowListApp(this);
#endif
    manager->restackWindows(this);
    if (dontCover())
	manager->updateWorkArea();
#ifdef CONFIG_GUIEVENTS
    wmapp->signalGuiEvent(geWindowOpened);
#endif
    manager->updateClientList();
}

YFrameWindow::~YFrameWindow() {
    fManaged = false;
    if (fKillMsgBox) {
        manager->unmanageClient(fKillMsgBox->handle());
        fKillMsgBox = 0;
    }
#ifdef CONFIG_GUIEVENTS
    wmapp->signalGuiEvent(geWindowClosed);
#endif
    if (fAutoRaiseTimer && fAutoRaiseTimer->timerListener() == this) {
        fAutoRaiseTimer->stop();
        fAutoRaiseTimer->timerListener(NULL);
    }
    if (fDelayFocusTimer && fDelayFocusTimer->timerListener() == this) {
        fDelayFocusTimer->stop();
        fDelayFocusTimer->timerListener(NULL);
    }
    if (movingWindow || sizingWindow)
        endMoveSize();
    if (fPopupActive)
        fPopupActive->cancelPopup();
#ifdef CONFIG_TASKBAR
    if (fTaskButton) {
        if (taskBar && taskBar->taskPane())
            taskBar->taskPane()->remove(this);
        else
            delete fTaskButton;
        fTaskButton = 0;
    }
#endif
#ifdef CONFIG_TRAY
    if (fTrayIcon) {
        if (taskBar && taskBar->trayPane())
            taskBar->trayPane()->remove(this);
        else
            delete fTrayIcon;

        fTrayIcon = NULL;
    }
#endif
#ifdef CONFIG_WINLIST
    if (fWinListItem) {
        if (windowList)
            windowList->removeWindowListApp(fWinListItem);
        delete fWinListItem; fWinListItem = 0;
    }
#endif
    if (fMiniIcon) {
        delete fMiniIcon;
        fMiniIcon = 0;
    }
    // perhaps should be done another way
#ifndef LITE
    if (switchWindow)
        switchWindow->destroyedFrame(this);
#endif
    removeTransients();
    removeAsTransient();
    manager->removeClientFrame(this);
    removeFrame();
    
    if (NULL != fClient) {
        if (!fClient->destroyed())
            XRemoveFromSaveSet(app->display(), client()->handle());

        XDeleteContext(app->display(), client()->handle(),
                       YWindowManager::frameContext);
    }

    if (dontCover())
        manager->updateWorkArea();

    delete fClient; fClient = 0;
    delete fClientContainer; fClientContainer = 0;
    delete fMenuButton; fMenuButton = 0;
    delete fCloseButton; fCloseButton = 0;
    delete fMaximizeButton; fMaximizeButton = 0;
    delete fMinimizeButton; fMinimizeButton = 0;
    delete fHideButton; fHideButton = 0;
    delete fRollupButton; fRollupButton = 0;
    delete fTitleBar; fTitleBar = 0;
    delete fDepthButton; fDepthButton = 0;

    XDestroyWindow(app->display(), topSide);
    XDestroyWindow(app->display(), leftSide);
    XDestroyWindow(app->display(), rightSide);
    XDestroyWindow(app->display(), bottomSide);
    XDestroyWindow(app->display(), topLeftCorner);
    XDestroyWindow(app->display(), topRightCorner);
    XDestroyWindow(app->display(), bottomLeftCorner);
    XDestroyWindow(app->display(), bottomRightCorner);
    manager->updateClientList();
}


// create 8 windows that are used to show the proper pointer
// on frame (for resize)
void YFrameWindow::createPointerWindows() {
    XSetWindowAttributes attributes;
    unsigned int klass = InputOnly;

    attributes.event_mask = 0;

    attributes.cursor = YWMApp::sizeTopPointer.handle();
    topSide = XCreateWindow(app->display(), handle(), 0, 0, 1, 1, 0,
                            CopyFromParent, klass, CopyFromParent,
                            CWCursor | CWEventMask, &attributes);

    attributes.cursor = YWMApp::sizeLeftPointer.handle();
    leftSide = XCreateWindow(app->display(), handle(), 0, 0, 1, 1, 0,
                            CopyFromParent, klass, CopyFromParent,
                            CWCursor | CWEventMask, &attributes);

    attributes.cursor = YWMApp::sizeRightPointer.handle();
    rightSide = XCreateWindow(app->display(), handle(), 0, 0, 1, 1, 0,
                            CopyFromParent, klass, CopyFromParent,
                            CWCursor | CWEventMask, &attributes);

    attributes.cursor = YWMApp::sizeBottomPointer.handle();
    bottomSide = XCreateWindow(app->display(), handle(), 0, 0, 1, 1, 0,
                            CopyFromParent, klass, CopyFromParent,
                            CWCursor | CWEventMask, &attributes);

    attributes.cursor = YWMApp::sizeTopLeftPointer.handle();
    topLeftCorner = XCreateWindow(app->display(), handle(), 0, 0, 1, 1, 0,
                                  CopyFromParent, klass, CopyFromParent,
                                  CWCursor | CWEventMask, &attributes);

    attributes.cursor = YWMApp::sizeTopRightPointer.handle();
    topRightCorner = XCreateWindow(app->display(), handle(), 0, 0, 1, 1, 0,
                                   CopyFromParent, klass, CopyFromParent,
                                   CWCursor | CWEventMask, &attributes);

    attributes.cursor = YWMApp::sizeBottomLeftPointer.handle();
    bottomLeftCorner = XCreateWindow(app->display(), handle(), 0, 0, 1, 1, 0,
                                     CopyFromParent, klass, CopyFromParent,
                                     CWCursor | CWEventMask, &attributes);

    attributes.cursor = YWMApp::sizeBottomRightPointer.handle();
    bottomRightCorner = XCreateWindow(app->display(), handle(), 0, 0, 1, 1, 0,
                                      CopyFromParent, klass, CopyFromParent,
                                      CWCursor | CWEventMask, &attributes);

    XMapSubwindows(app->display(), handle());
    indicatorsVisible = 1;
}

void YFrameWindow::grabKeys() {
    if (app->AltMask) {
        GRAB_WMKEY(gKeyWinRaise);
        GRAB_WMKEY(gKeyWinOccupyAll);
        GRAB_WMKEY(gKeyWinLower);
        GRAB_WMKEY(gKeyWinClose);
        GRAB_WMKEY(gKeyWinRestore);
        GRAB_WMKEY(gKeyWinNext);
        GRAB_WMKEY(gKeyWinPrev);
        GRAB_WMKEY(gKeyWinMove);
        GRAB_WMKEY(gKeyWinSize);
        GRAB_WMKEY(gKeyWinMinimize);
        GRAB_WMKEY(gKeyWinMaximize);
        GRAB_WMKEY(gKeyWinMaximizeVert);
        GRAB_WMKEY(gKeyWinHide);
        GRAB_WMKEY(gKeyWinRollup);
        GRAB_WMKEY(gKeyWinMenu);
    }
}

void YFrameWindow::manage(YFrameClient *client) {
    PRECONDITION(client != 0);
    fClient = client;

    XSetWindowBorderWidth(app->display(),
                          client->handle(),
                          0);

    XAddToSaveSet(app->display(), client->handle());

    client->reparent(fClientContainer, 0, 0);

    client->frame(this);

    sendConfigure();
}

void YFrameWindow::unmanage(bool reparent) {
    PRECONDITION(fClient != 0);

    if (!fClient->destroyed()) {
        int gx, gy;
        client()->gravityOffsets(gx, gy);

        XSetWindowBorderWidth(app->display(),
                              client()->handle(),
                              client()->border());

        int posX, posY, posWidth, posHeight;

        normalGeometry(&posX, &posY, &posWidth, &posHeight);
        if (gx < 0)
            posX -= borderX();
        else if (gx > 0)
            posX += borderX() - 2 * client()->border();
        if (gy < 0)
            posY -= borderY() + titleY();
        else if (gy > 0)
            posY += borderY() - 2 * client()->border();

	if (reparent)
		client()->reparent(manager, posX, posY);

        client()->size(posWidth, posHeight);

        if (phase != phaseRestart)
            client()->wmState(WithdrawnState);

        if (!client()->destroyed())
            XRemoveFromSaveSet(app->display(), client()->handle());
    }

    client()->frame(NULL);
    fClient = NULL;
}

void YFrameWindow::configureClient(const XConfigureRequestEvent &configureRequest) {
    client()->border((configureRequest.value_mask & CWBorderWidth) ?
                      configureRequest.border_width : client()->border());

    int cx((configureRequest.value_mask & CWX) ?
            configureRequest.x : x() + borderX());
    int cy((configureRequest.value_mask & CWY) ?
            configureRequest.y : y() + titleY() + borderY());
    int cwidth((configureRequest.value_mask & CWWidth) ?
                configureRequest.width : client()->width());
    int cheight((configureRequest.value_mask & CWHeight) ?
                 configureRequest.height : client()->height());

    configureClient(cx, cy, cwidth, cheight);

    if (configureRequest.value_mask & CWStackMode) {
        YFrameWindow *sibling = 0;
        XWindowChanges xwc;

        if ((configureRequest.value_mask & CWSibling) &&
            XFindContext(app->display(),
                         configureRequest.above,
                         YWindowManager::clientContext,
                         (XPointer *)&sibling) == 0)
            xwc.sibling = sibling->handle();
        else
            xwc.sibling = configureRequest.above;

        xwc.stack_mode = configureRequest.detail;

        /* !!! implement the rest, and possibly fix these: */

        if (sibling && xwc.sibling != None) { /* ICCCM suggests sibling==None */
            switch (xwc.stack_mode) {
            case Above:
                above(sibling);
                break;
            case Below:
                below(sibling);
                break;
            default:
                return ;
            }
            XConfigureWindow(app->display(),
                             handle(),
                             configureRequest.value_mask & (CWSibling | CWStackMode),
                             &xwc);
        } else if (xwc.sibling == None && manager->top(layer()) != 0) {
            switch (xwc.stack_mode) {
            case Above:
                if (canRaise()) {
                    wmRaise();
                }
#if 1
                if (!(frameOptions() & foNoFocusOnAppRaise) &&
                   (clickFocus || !strongPointerFocus))
                    activate();
#endif
                { /* warning, tcl/tk "fix" here */
                    XEvent xev;

                    memset(&xev, 0, sizeof(xev));
                    xev.xconfigure.type = ConfigureNotify;
                    xev.xconfigure.display = app->display();
                    xev.xconfigure.event = handle();
                    xev.xconfigure.window = handle();
                    xev.xconfigure.x = x();
                    xev.xconfigure.y = y();
                    xev.xconfigure.width = width();
                    xev.xconfigure.height = height();
                    xev.xconfigure.border_width = 0;

                    xev.xconfigure.above = None;
                    xev.xconfigure.override_redirect = False;

                    XSendEvent(app->display(),
                               handle(),
                               False,
                               StructureNotifyMask,
                               &xev);
                }
                break;
            case Below:
                wmLower();
                break;
            default:
                return ;
            }
        } else
            return ;
    }
}

void YFrameWindow::configureClient(int cx, int cy, int cwidth, int cheight) {
    cx -= borderX();
    cy -= borderY() + titleY();
    cwidth += 2 * borderX();
    cheight += 2 * borderY() + titleY();

#if 0
    // !!! should be an option
    if (cx != x() || cy != y() ||
        (unsigned int)cwidth != width() || (unsigned int)cheight != height())
        if (isMaximized()) {
            fWinState &= ~(WinStateMaximizedVert | WinStateMaximizedHoriz);
            if (fMaximizeButton) {
                fMaximizeButton->actions(actionMaximize, actionMaximizeVert);
                fMaximizeButton->toolTip(_("Maximize"));
            }
        }
#endif

    MSG(("setting geometry (%d:%d %dx%d)", cx, cy, cwidth, cheight));

    if (isIconic()) {
        cx += borderX();
        cy += borderY();
        cwidth -= 2 * borderX();
        cheight -= 2 * borderY() + titleY();

        client()->geometry(0, 0, cwidth, cheight);

        int nx = cx;
        int ny = cy;
        int nw = cwidth;
        int nh = cheight;
        XSizeHints *sh = client()->sizeHints();

        bool cxw = true;
        bool cy = true;
        bool ch = true;

        if (isMaximizedHoriz())
            cxw = false;
        if (isMaximizedVert())
            cy = ch = false;
        if (isRollup())
            ch = false;

        if (cxw) {
            normalX = nx;
            normalWidth = sh ? (nw - sh->base_width) / sh->width_inc : nw;
        }
        if (cy)
            normalY = ny;
        if (ch)
            normalHeight = sh ? (nh - sh->base_height) / sh->height_inc : nh;
    } else if (isRollup()) {
        //!!!
    } else {
        geometry(cx, cy, cwidth, cheight);
    }
}

void YFrameWindow::handleClick(const XButtonEvent &up, int /*count*/) {
    if (up.button == 3) {
        popupSystemMenu(up.x_root, up.y_root, -1, -1,
                        YPopupWindow::pfCanFlipVertical |
                        YPopupWindow::pfCanFlipHorizontal |
                        YPopupWindow::pfPopupMenu);
    }
}

void YFrameWindow::handleCrossing(const XCrossingEvent &crossing) {
    static int old_x = -1, old_y = -1;

    if (crossing.type == EnterNotify &&
        (crossing.mode == NotifyNormal || (strongPointerFocus && crossing.mode == NotifyUngrab)) &&
        crossing.window == handle() &&
        (strongPointerFocus ||
         old_x != crossing.x_root || old_y != crossing.y_root))
    {
        old_x = crossing.x_root;
        old_y = crossing.y_root;

        if (!clickFocus && visible()) {
            if (!delayPointerFocus)
                focus(false);
            else {
                if (fDelayFocusTimer == 0)
                    fDelayFocusTimer = new YTimer(pointerFocusDelay);
                if (fDelayFocusTimer) {
                    fDelayFocusTimer->timerListener(this);
                    fDelayFocusTimer->start();
                }
            }
        }
        if (autoRaise) {
            if (fAutoRaiseTimer == 0) {
                fAutoRaiseTimer = new YTimer(autoRaiseDelay);
            }
            if (fAutoRaiseTimer) {
                fAutoRaiseTimer->timerListener(this);
                fAutoRaiseTimer->start();
            }
        }
    } else if (crossing.type == LeaveNotify &&
               fFocused &&
               focusRootWindow &&
               crossing.window == handle())
    {
        if (crossing.detail != NotifyInferior &&
            crossing.mode == NotifyNormal)
        {
            if (fDelayFocusTimer && fDelayFocusTimer->timerListener() == this) {
                fDelayFocusTimer->stop();
                fDelayFocusTimer->timerListener(NULL);
            }
#if 0 /// !!! focus root
            if (!clickFocus) {
                deactivate();
            }
#endif
            if (autoRaise) {
                if (fAutoRaiseTimer && fAutoRaiseTimer->timerListener() == this) {
                    fAutoRaiseTimer->stop();
                    fAutoRaiseTimer->timerListener(NULL);
                }
            }
        }
    }
}

void YFrameWindow::handleFocus(const XFocusChangeEvent &focus) {
#ifndef LITE
    if (switchWindow && switchWindow->visible())
        return ;
#endif
#if 1
    if (focus.type == FocusIn &&
        focus.mode != NotifyGrab &&
        focus.window == handle() &&
        focus.detail != NotifyInferior &&
        focus.detail != NotifyPointer &&
        focus.detail != NotifyPointerRoot)
        manager->switchFocusTo(this);
#endif
#if 0
    else if (focus.type == FocusOut &&
               focus.mode == NotifyNormal &&
               focus.detail != NotifyInferior &&
               focus.detail != NotifyPointer &&
               focus.detail != NotifyPointerRoot &&
               focus.window == handle())
        manager->switchFocusFrom(this);
#endif

    layoutShape();
}

bool YFrameWindow::handleTimer(YTimer *t) {
    if (t == fAutoRaiseTimer) {
        if (canRaise())
            wmRaise();
    }
    if (t == fDelayFocusTimer)
        focus(false);
    return false;
}

void YFrameWindow::raise() {
    if (this != manager->top(layer())) {
        YWindow::raise();
        above(manager->top(layer()));
    }
}

void YFrameWindow::lower() {
    if (this != manager->bottom(layer())) {
        YWindow::lower();
        above(NULL);
    }
}

void YFrameWindow::removeFrame() {
#ifdef DEBUG
    if (debug_z) dumpZorder("before removing", this);
#endif
    if (prev()) prev()->next(next());
    else manager->top(layer(), next());

    if (next()) next()->prev(prev());
    else manager->bottom(layer(), prev());

    prev(NULL);
    next(NULL);

    if (nextCreated()) nextCreated()->prevCreated(prevCreated());
    else manager->lastCreated(prevCreated());

    if (prevCreated()) prevCreated()->nextCreated(nextCreated());
    else manager->firstCreated(nextCreated());

    prevCreated(NULL);
    nextCreated(NULL);

#ifdef DEBUG
    if (debug_z) dumpZorder("after removing", this);
#endif
}

void YFrameWindow::insertFrame() {
#ifdef DEBUG
    if (debug_z) dumpZorder("before inserting", this);
#endif
    next(manager->top(layer()));
    prev(NULL);

    if (next()) next()->prev(this);
    else manager->bottom(layer(), this);

    manager->top(layer(), this);

    nextCreated(NULL);
    prevCreated(manager->lastCreated());

    if (prevCreated()) prevCreated()->nextCreated(this);
    else manager->firstCreated(this);

    manager->lastCreated(this);
#ifdef DEBUG
    if (debug_z) dumpZorder("after inserting", this);
#endif
}

void YFrameWindow::above(YFrameWindow *aboveFrame) {
#ifdef DEBUG
    if (debug_z) dumpZorder("before setAbove", this, aboveFrame);
#endif
    if (aboveFrame != next() && aboveFrame != this) {
        if (prev()) prev()->next(next());
        else manager->top(layer(), next());

        if (next()) next()->prev(prev());
        else manager->bottom(layer(), prev());

        next(aboveFrame);

        if (next()) {
            prev(next()->prev());
            next()->prev(this);
        } else {
            prev(manager->bottom(layer()));
            manager->bottom(layer(), this);
        }

        if (prev()) prev()->next(this);
        else manager->top(layer(), this);
#ifdef DEBUG
        if (debug_z) dumpZorder("after above", this, aboveFrame);
#endif
    }
}

void YFrameWindow::below(YFrameWindow *belowFrame) {
    if (belowFrame != next()) above(belowFrame->next());
}

YFrameWindow *YFrameWindow::findWindow(int flags) {
    YFrameWindow *p = this;

     if (flags & fwfNext)
         goto next;

     do {
         if ((flags & fwfMinimized) && !p->isMinimized())
             goto next;
         if ((flags & fwfUnminimized) && p->isMinimized())
             goto next;
         if ((flags & fwfVisible) && !p->visible())
             goto next;
         if ((flags & fwfHidden) && !p->isHidden())
             goto next;
         if ((flags & fwfNotHidden) && p->isHidden())
             goto next;
         if ((flags & fwfFocusable) && !p->isFocusable())
             goto next;
         if ((flags & fwfWorkspace) && !p->visibleNow())
             goto next;
         if ((flags & fwfSwitchable) && (p->frameOptions() & foIgnoreQSwitch))
             goto next;
         if (!p->client()->adopted())
             goto next;

         return p;

     next:
         if (flags & fwfBackward)
             p = (flags & fwfLayers) ? p->prevLayer() : p->prev();
         else
             p = (flags & fwfLayers) ? p->nextLayer() : p->next();
         if (p == 0)
             if (!(flags & fwfCycle))
                 return 0;
             else if (flags & fwfBackward)
                 p = (flags & fwfLayers) ? manager->bottomLayer() : manager->bottom(layer());
             else
                 p = (flags & fwfLayers) ? manager->topLayer() : manager->top(layer());
     } while (p != this);

     if (!(flags & fwfSame))
         return 0;
     if ((flags & fwfVisible) && !p->visible())
         return 0;
     if ((flags & fwfFocusable) && !p->isFocusable())
         return 0;
     if ((flags & fwfWorkspace) && !p->visibleNow())
         return 0;
     if (!p->client()->adopted())
         return 0;

     return this;
}

void YFrameWindow::handleConfigure(const XConfigureEvent &/*configure*/) {
}

void YFrameWindow::sendConfigure() {
    XEvent xev;

    memset(&xev, 0, sizeof(xev));
    xev.xconfigure.type = ConfigureNotify;
    xev.xconfigure.display = app->display();
    xev.xconfigure.event = client()->handle();
    xev.xconfigure.window = client()->handle();
    xev.xconfigure.x = x() + borderX();
    xev.xconfigure.y = y() + borderY()
#ifndef TITLEBAR_BOTTOM
        + titleY()
#endif
        ;
    xev.xconfigure.width = client()->width();
    xev.xconfigure.height = client()->height();
    xev.xconfigure.border_width = client()->border();

    xev.xconfigure.above = None;
    xev.xconfigure.override_redirect = False;

#ifdef DEBUG_C
    Status rc =
#endif
        XSendEvent(app->display(),
               client()->handle(),
               False,
               StructureNotifyMask,
               &xev);

#ifdef DEBUG_C
    MSG(("sent %d: x=%d, y=%d, width=%d, height=%d",
         rc,
         x(),
         y(),
         client()->width(),
         client()->height()));
#endif
}

void YFrameWindow::actionPerformed(YAction *action, unsigned int modifiers) {
    if (action == actionRestore) {
        wmRestore();
    } else if (action == actionMinimize) {
        if (canMinimize())
            wmMinimize();
    } else if (action == actionMaximize) {
        if (canMaximize())
            wmMaximize();
    } else if (action == actionMaximizeVert) {
        if (canMaximize())
            wmMaximizeVert();
    } else if (action == actionLower) {
        if (canLower())
            wmLower();
    } else if (action == actionRaise) {
        if (canRaise())
            wmRaise();
    } else if (action == actionDepth) {
        if (Overlaps(true) && canRaise()){
            wmRaise();
            manager->focus(this, true);
        } else if (Overlaps(false) && canLower())
            wmLower();
    } else if (action == actionRollup) {
        if (canRollup())
            wmRollup();
    } else if (action == actionClose) {
        if (canClose())
            wmClose();
    } else if (action == actionKill) {
        wmConfirmKill();
#ifndef CONFIG_PDA
    } else if (action == actionHide) {
        if (canHide())
            wmHide();
#endif
    } else if (action == actionShow) {
        wmShow();
    } else if (action == actionMove) {
        if (canMove())
            wmMove();
    } else if (action == actionSize) {
        if (canSize())
            wmSize();
    } else if (action == actionOccupyAllOrCurrent) {
        wmOccupyAllOrCurrent();
    } else if (action == actionDontCover) {
        wmToggleDontCover();
    } else {
        for (icewm::Layer l(0); l < WinLayerCount; ++l) {
            if (action == layerActionSet[l]) {
                wmSetLayer(l);
                return ;
            }
        }
        for (icewm::Workspace w(0); w < workspaceCount; ++w) {
            if (action == workspaceActionMoveTo[w]) {
                wmMoveToWorkspace(w);
                return ;
            }
        }
#ifdef CONFIG_TRAY
        for (icewm::TrayOption o(0); o < IcewmTrayOptionCount; ++o) {
            if (action == trayOptionActionSet[o]) {
                wmSetTrayOption(o);
                return;
            }
        }
#endif
        wmapp->actionPerformed(action, modifiers);
    }
}

void YFrameWindow::wmSetLayer(icewm::Layer layer) {
    this->layer(layer);
}

#ifdef CONFIG_TRAY
void YFrameWindow::wmSetTrayOption(icewm::TrayOption option) {
    trayOption(option);
}
#endif

void YFrameWindow::wmToggleDontCover() {
    dontCover(!dontCover());
}

void YFrameWindow::wmMove() {
    Window root, child;
    int rx, ry, wx, wy;
    unsigned int mask;
    XQueryPointer(app->display(), desktop->handle(),
                  &root, &child, &rx, &ry, &wx, &wy, &mask);
    if (wx > int(x() + width()))
        wx = x() + width();
    if (wy > int(y() + height()))
        wy = y() + height();
    if (wx < x())
        wx = x();
    if (wy < y())
        wy = y();
    startMoveSize(1, 0,
                  0, 0,
                  wx - x(), wy - y());
}

void YFrameWindow::wmSize() {
    startMoveSize(0, 0,
                  0, 0,
                  0, 0);
}

void YFrameWindow::wmRestore() {
#ifdef CONFIG_GUIEVENTS
    wmapp->signalGuiEvent(geWindowRestore);
#endif
    state(WinStateMaximizedVert | WinStateMaximizedHoriz |
          WinStateMinimized | WinStateHidden | WinStateRollup, 0);
}

void YFrameWindow::wmMinimize() {
#ifdef DEBUG_S
    MSG(("wmMinimize - Frame: %d", visible()));
    MSG(("wmMinimize - Client: %d", client()->visible()));
#endif
    if (isMinimized()) {
#ifdef CONFIG_GUIEVENTS
        wmapp->signalGuiEvent(geWindowRestore);
#endif
        state(WinStateMinimized, 0);
    } else {
        //if (!canMinimize())
        //    return ;
#ifdef CONFIG_GUIEVENTS
        wmapp->signalGuiEvent(geWindowMin);
#endif
        state(WinStateMinimized, WinStateMinimized);
        wmLower();
    }
    if (clickFocus || !strongPointerFocus)
        manager->focusTopWindow();
}

void YFrameWindow::minimizeTransients() {
    for (YFrameWindow *w = transient(); w; w = w->nextTransient()) {
// Since a) YFrameWindow::state is too heavy but b) we want to save memory
	MSG(("> isMinimized: %d\n", w->isMinimized()));
        if (w->isMinimized())
            w->fWinState|= WinStateWasMinimized;
        else
            w->fWinState&= ~WinStateWasMinimized;
	MSG(("> wasMinimized: %d\n", w->wasMinimized()));
        if (!w->isMinimized()) w->wmMinimize();
    }
}

void YFrameWindow::restoreMinimizedTransients() {
    for (YFrameWindow *w = transient(); w; w = w->nextTransient())
        if (w->isMinimized() && !w->wasMinimized())
            w->state(WinStateMinimized, 0);
}

void YFrameWindow::hideTransients() {
    for (YFrameWindow *w = transient(); w; w = w->nextTransient()) {
// See YFrameWindow::minimizeTransients() for reason
	MSG(("> isHidden: %d\n", w->isHidden()));
        if (w->isHidden())
            w->fWinState|= WinStateWasHidden;
        else
            w->fWinState&= ~WinStateWasHidden;
	MSG(("> was visible: %d\n", w->wasHidden());
        if (!w->isHidden()) w->wmHide());
    }
}

void YFrameWindow::restoreHiddenTransients() {
    for (YFrameWindow *w = transient(); w; w = w->nextTransient())
        if (w->isHidden() && !w->wasHidden())
            w->state(WinStateHidden, 0);
}

void YFrameWindow::DoMaximize(long flags) {
    state(WinStateRollup, 0);

    if (isMaximized()) {
#ifdef CONFIG_GUIEVENTS
        wmapp->signalGuiEvent(geWindowRestore);
#endif
        state(WinStateMaximizedVert |
              WinStateMaximizedHoriz |
              WinStateMinimized, 0);
    } else {
        //if (!canMaximize())
        //    return ;

#ifdef CONFIG_GUIEVENTS
        wmapp->signalGuiEvent(geWindowMax);
#endif
        state(WinStateMaximizedVert |
              WinStateMaximizedHoriz |
              WinStateMinimized, flags);
    }
}

void YFrameWindow::wmMaximize() {
    DoMaximize(WinStateMaximizedVert | WinStateMaximizedHoriz);
}

void YFrameWindow::wmMaximizeVert() {
    if (isMaximizedVert()) {
#ifdef CONFIG_GUIEVENTS
        wmapp->signalGuiEvent(geWindowRestore);
#endif
        state(WinStateMaximizedVert, 0);
    } else {
        //if (!canMaximize())
        //    return ;
#ifdef CONFIG_GUIEVENTS
        wmapp->signalGuiEvent(geWindowMax);
#endif
        state(WinStateMaximizedVert, WinStateMaximizedVert);
    }
}

void YFrameWindow::wmMaximizeHorz() {
    if (isMaximizedHoriz()) {
#ifdef CONFIG_GUIEVENTS
        wmapp->signalGuiEvent(geWindowRestore);
#endif
        state(WinStateMaximizedHoriz, 0);
    } else {
        //if (!canMaximize())
        //    return ;
#ifdef CONFIG_GUIEVENTS
        wmapp->signalGuiEvent(geWindowMax);
#endif
        state(WinStateMaximizedHoriz, WinStateMaximizedHoriz);
    }
}

void YFrameWindow::wmRollup() {
    if (isRollup()) {
#ifdef CONFIG_GUIEVENTS
        wmapp->signalGuiEvent(geWindowRestore);
#endif
        state(WinStateRollup, 0);
    } else {
        //if (!canRollup())
        //    return ;
#ifdef CONFIG_GUIEVENTS
        wmapp->signalGuiEvent(geWindowRollup);
#endif
        state(WinStateRollup, WinStateRollup);
    }
}

void YFrameWindow::wmHide() {
    if (isHidden()) {
#ifdef CONFIG_GUIEVENTS
        wmapp->signalGuiEvent(geWindowRestore);
#endif
        state(WinStateHidden, 0);
    } else {
        //if (!canHide())
        //    return ;

#ifdef CONFIG_GUIEVENTS
        wmapp->signalGuiEvent(geWindowHide);
#endif
        state(WinStateHidden, WinStateHidden);
    }
    if (clickFocus || !strongPointerFocus)
        manager->focusTopWindow();
}

void YFrameWindow::wmLower() {
    if (this != manager->bottom(layer())) {
        YFrameWindow *w = this;

#ifdef CONFIG_GUIEVENTS
        wmapp->signalGuiEvent(geWindowLower);
#endif
        while (w) {
            w->doLower();
            w = w->owner();
        }
        manager->restackWindows(this);

        if (clickFocus || !strongPointerFocus)
            manager->focusTopWindow();
    }
}

void YFrameWindow::doLower() {
    above(NULL);
}

void YFrameWindow::wmRaise() {
    doRaise();
    manager->restackWindows(this);
}

void YFrameWindow::doRaise() {
#ifdef DEBUG
    if (debug_z) dumpZorder("wmRaise: ", this);
#endif
    if (this != manager->top(layer())) {
        above(manager->top(layer()));

	for (YFrameWindow * w (transient()); w; w = w->nextTransient())
	    w->doRaise();

#ifdef DEBUG
        if (debug_z) dumpZorder("wmRaise after raise: ", this);
#endif
    }
}

void YFrameWindow::wmClose() {
    if (canClose()) {
#ifdef CONFIG_PDA
        wmHide();
#else
        YServerLock __lock__;
        client()->protocols();

        if (client()->protocols() & wm::DeleteWindow)
            client()->sendMessage(atoms.wmDeleteWindow);
        else
            wmConfirmKill();
#endif
    }
}

void YFrameWindow::wmConfirmKill() {
#ifndef LITE
    if (NULL == fKillMsgBox) {
        YMsgBox *msgbox = new YMsgBox(YMsgBox::mbOK|YMsgBox::mbCancel);
        char *title = strJoin(_("Kill Client: "), this->title(), 0);
        fKillMsgBox = msgbox;

        msgbox->title(title);
        delete title; title = 0;
        msgbox->text(_("WARNING! All unsaved changes will be lost when\n"
                       "this client is killed. Do you wish to proceed?"));
        msgbox->autoSize();
        msgbox->msgBoxListener(this);
        msgbox->showFocused();
    }
#endif
}

void YFrameWindow::wmKill() {
    if (canClose()) {
        MSG(("No WM_DELETE_WINDOW protocol"));
        XKillClient(app->display(), client()->handle());
    }
}

void YFrameWindow::wmPrevWindow() {
    if (next() != this) {
        YFrameWindow *f = findWindow(fwfNext | fwfBackward | fwfVisible | fwfCycle | fwfFocusable | fwfWorkspace | fwfSame);
        if (f) {
            f->wmRaise();
            manager->focus(f, true);
        }
    }
}

void YFrameWindow::wmNextWindow() {
    if (next() != this) {
        wmLower();
        manager->focus(findWindow(fwfNext | fwfVisible | fwfCycle | fwfFocusable | fwfWorkspace | fwfSame), true);
    }
}

void YFrameWindow::wmLastWindow() {
msg("%s:%d: foo", __FILE__, __LINE__);
    if (next() != this) {
        YFrameWindow *f = findWindow(fwfNext | fwfVisible | fwfCycle | fwfFocusable | fwfWorkspace | fwfSame);
        if (f) {
            f->wmRaise();
            manager->focus(f, true);
        }
    }
}

void YFrameWindow::loseWinFocus() {
    MSG(("losing focus %lX", this));

    if (fFocused && fManaged) {
        fFocused = false;

        if (true || !clientMouseActions)
            if (focusOnClickClient || raiseOnClickClient)
                if (fClientContainer)
                    fClientContainer->grabButtons();
        if (isIconic())
            fMiniIcon->repaint();
        else {
            repaint();
            titlebar()->deactivate();
        }
#ifdef CONFIG_TASKBAR
        updateTaskBar();
#endif
    }
}

void YFrameWindow::takeWinFocus() {
    MSG(("taking focus %lX", this));

    if (!fFocused) {
        fFocused = true;

        if (isIconic())
            fMiniIcon->repaint();
        else {
            titlebar()->activate();
            repaint();
        }
#ifdef CONFIG_TASKBAR
        updateTaskBar();
#endif

        if (true || !clientMouseActions)
            if (focusOnClickClient &&
                !(raiseOnClickClient && (this != manager->top(layer()))))
                fClientContainer->releaseButtons();
    }
}

void YFrameWindow::focusOnMap() {
    if (owner() != 0) {
        if (focusOnMapTransient)
            if (owner()->focused() || !focusOnMapTransientActive)
                activate();
    } else {
        if (::focusOnMap)
            activate();
    }
}

void YFrameWindow::wmShow() {
   // recover lost (offscreen) windows !!! (unify with code below)
    if (x() >= int(manager->width()) ||
        y() >= int(manager->height()) ||
        x() <= - int(width()) ||
        y() <= - int(height()))
    {
        int newX = x();
        int newY = y();

        if (x() >= int(manager->width()))
            newX = int(manager->width() - width() + borderX());
        if (y() >= int(manager->height()))
            newY = int(manager->height() - height() + borderY());

        if (newX < int(- borderX()))
            newX = int(- borderX());
        if (newY < int(- borderY()))
            newY = int(- borderY());
        position(newX, newY);
    }

    state(WinStateHidden | WinStateMinimized, 0);
}

void YFrameWindow::focus(bool canWarp) {
    if (!visibleOn(manager->activeWorkspace()))
        manager->activateWorkspace(workspace());
    // recover lost (offscreen) windows !!!
    if (limitPosition &&
        (x() >= int(manager->width()) ||
         y() >= int(manager->height()) ||
         x() <= - int(width()) ||
         y() <= - int(height())))
    {
        int newX = x();
        int newY = y();

        if (x() >= int(manager->width()))
            newX = int(manager->width() - width() + borderX());
        if (y() >= int(manager->height()))
            newY = int(manager->height() - height() + borderY());

        if (newX < int(- borderX()))
            newX = int(- borderX());
        if (newY < int(- borderY()))
            newY = int(- borderY());
        position(newX, newY);
    }

    if (isFocusable())
        manager->focus(this, canWarp);
    if (raiseOnFocus && /* clickFocus && */ phase == phaseRunning)
        wmRaise();
}

void YFrameWindow::activate(bool canWarp) {
    if (fWinState & (WinStateHidden | WinStateMinimized))
        state(WinStateHidden | WinStateMinimized, 0);

#ifdef CONFIG_WM_SESSION
    manager->topLevelProcess(client()->pid());
#endif
    focus(canWarp);
}

void YFrameWindow::paint(Graphics &g, int , int , unsigned int , unsigned int ) {
    YColor *bg;

    if (!(frameDecorations() & (fdResize | fdBorder)))
        return ;

    if (focused())
        bg = activeBorderBg;
    else
        bg = inactiveBorderBg;

    g.color(bg);
    switch (wmLook) {
#if defined(CONFIG_LOOK_WIN95) || defined(CONFIG_LOOK_WARP4) || defined(CONFIG_LOOK_NICE)
#ifdef CONFIG_LOOK_WIN95
    case lookWin95:
#endif
#ifdef CONFIG_LOOK_WARP4
    case lookWarp4:
#endif
#ifdef CONFIG_LOOK_NICE
    case lookNice:
#endif
        g.fillRect(1, 1, width() - 3, height() - 3);
        g.drawBorderW(0, 0, width() - 1, height() - 1, true);
        break;
#endif
#if defined(CONFIG_LOOK_MOTIF) || defined(CONFIG_LOOK_WARP3)
#ifdef CONFIG_LOOK_MOTIF
    case lookMotif:
#endif
#ifdef CONFIG_LOOK_WARP3
    case lookWarp3:
#endif
        g.draw3DRect(0, 0, width() - 1, height() - 1, true);
        g.draw3DRect(borderX() - 1, borderY() - 1,
                     width() - 2 * borderX() + 1, height() - 2 * borderY() + 1,
                     false);

        g.fillRect(1, 1, width() - 2, borderY() - 2);
        g.fillRect(1, 1, borderX() - 2, height() - 2);
        g.fillRect(1, (height() - 1) - (borderY() - 2), width() - 2, borderX() - 2);
        g.fillRect((width() - 1) - (borderX() - 2), 1, borderX() - 2, height() - 2);

#ifdef CONFIG_LOOK_MOTIF
        if (wmLook == lookMotif && canSize()) {
            YColor *b(bg->brighter());
            YColor *d(bg->darker());


            g.color(d);
            g.drawLine(wsCornerX - 1, 0, wsCornerX - 1, height() - 1);
            g.drawLine(width() - wsCornerX - 1, 0, width() - wsCornerX - 1, height() - 1);
            g.drawLine(0, wsCornerY - 1, width(),wsCornerY - 1);
            g.drawLine(0, height() - wsCornerY - 1, width(), height() - wsCornerY - 1);
            g.color(b);
            g.drawLine(wsCornerX, 0, wsCornerX, height() - 1);
            g.drawLine(width() - wsCornerX, 0, width() - wsCornerX, height() - 1);
            g.drawLine(0, wsCornerY, width(), wsCornerY);
            g.drawLine(0, height() - wsCornerY, width(), height() - wsCornerY);
        }
        break;
#endif
#endif
#ifdef CONFIG_LOOK_PIXMAP
    case lookPixmap:
    case lookMetal:
    case lookGtk:
        {
            int n = focused() ? 1 : 0;
            int t = (frameDecorations() & fdResize) ? 0 : 1;

            if ((frameT[t][n] || TEST_GRADIENT(rgbFrameT[t][n])) &&
		(frameL[t][n] || TEST_GRADIENT(rgbFrameL[t][n])) &&
		(frameR[t][n] || TEST_GRADIENT(rgbFrameR[t][n])) &&
		(frameB[t][n] || TEST_GRADIENT(rgbFrameB[t][n])) &&
		frameTL[t][n] && frameTR[t][n] &&
		frameBL[t][n] && frameBR[t][n]) {
		unsigned const xtl(frameTL[t][n]->width());
		unsigned const ytl(frameTL[t][n]->height());
		unsigned const xtr(frameTR[t][n]->width());
		unsigned const ytr(frameTR[t][n]->height());
		unsigned const xbl(frameBL[t][n]->width());
		unsigned const ybl(frameBL[t][n]->height());
		unsigned const xbr(frameBR[t][n]->width());
		unsigned const ybr(frameBR[t][n]->height());

		unsigned const cx(width()/2);
		unsigned const cy(height()/2);

		g.copyPixmap(frameTL[t][n], 0, 0,
			     min(xtl, cx), min(ytl, cy), 0, 0);
		g.copyPixmap(frameTR[t][n], max(0, (int)xtr - (int)cx), 0,
			     min(xtr, cx), min(ytr, cy),
			     width() - min(xtr, cx), 0);
		g.copyPixmap(frameBL[t][n], 0, max(0, (int)ybl - (int)cy),
			     min(xbl, cx), min(ybl, cy),
			     0, height() - min(ybl, cy));
		g.copyPixmap(frameBR[t][n],
			     max(0, (int)xbr - (int)cx), max(0, (int)ybr - (int)cy),
			     min(xbr, cx), min(ybr, cy),
			     width() - min(xbr, cx), height() - min(ybr, cy));

		if (width() > (xtl + xtr))
		    if (frameT[t][n]) g.repHorz(frameT[t][n],
			xtl, 0, width() - xtl - xtr);
#ifdef CONFIG_GRADIENTS
		    else g.drawGradient(*rgbFrameT[t][n],
			xtl, 0, width() - xtl - xtr, borderY());
#endif

		if (height() > (ytl + ybl))
		    if (frameL[t][n]) g.repVert(frameL[t][n],
			0, ytl, height() - ytl - ybl);
#ifdef CONFIG_GRADIENTS
		    else g.drawGradient(*rgbFrameL[t][n],
			0, ytl, borderX(), height() - ytl - ybl);
#endif

		if (height() > (ytr + ybr))
		    if (frameR[t][n]) g.repVert(frameR[t][n],
			width() - borderX(), ytr, height() - ytr - ybr);
#ifdef CONFIG_GRADIENTS
		    else g.drawGradient(*rgbFrameR[t][n],
			width() - borderX(), ytr,
			borderX(), height() - ytr - ybr);
#endif

		if (width() > (xbl + xbr))
		    if (frameB[t][n]) g.repHorz(frameB[t][n],
			xbl, height() - borderY(), width() - xbl - xbr);
#ifdef CONFIG_GRADIENTS
		    else g.drawGradient(*rgbFrameB[t][n],
			xbl, height() - borderY(),
			width() - xbl - xbr, borderY());
#endif

            } else {
                g.fillRect(1, 1, width() - 3, height() - 3);
                g.drawBorderW(0, 0, width() - 1, height() - 1, true);
            }
        }
        break;
#endif
    default:
        break;
    }
}

void YFrameWindow::handlePopDown(YPopupWindow *popup) {
    MSG(("popdown %ld up %ld", popup, fPopupActive));
    if (fPopupActive == popup)
        fPopupActive = 0;
}

void YFrameWindow::popupSystemMenu() {
    if (fPopupActive == 0) {
        if (fMenuButton && fMenuButton->visible() &&
            fTitleBar && fTitleBar->visible())
            fMenuButton->popupMenu();
        else {
            int ax = x() + container()->x();
            int ay = y() + container()->y();
            if (isIconic()) {
                ax = x();
                ay = y() + height();
            }
            popupSystemMenu(ax, ay,
                            -1, -1, //width(), height(),
                            YPopupWindow::pfCanFlipVertical);
        }
    }
}

void YFrameWindow::popupSystemMenu(int x, int y,
                                   int x_delta, int y_delta,
                                   unsigned int flags,
                                   YWindow *forWindow)
{
    if (fPopupActive == 0) {
        updateMenu();
        if (windowMenu()->popup(forWindow, this,
                                x, y, x_delta, y_delta, flags))
            fPopupActive = windowMenu();
    }
}

void YFrameWindow::updateTitle() {
    titlebar()->toolTip(client()->windowTitle());
    titlebar()->repaint();

    layoutShape();

    updateIconTitle();
#ifdef CONFIG_WINLIST
    if (fWinListItem && windowList->visible())
        windowList->repaintItem(fWinListItem);
#endif
#ifdef CONFIG_TASKBAR
    if (fTaskButton)
        fTaskButton->toolTip((const char *)client()->windowTitle());
#endif
#ifdef CONFIG_TRAY
    if (fTrayIcon)
        fTrayIcon->toolTip((const char *)client()->windowTitle());
#endif

    if (fMiniIcon)
        fMiniIcon->toolTip(client()->iconTitle());
}

void YFrameWindow::updateIconTitle() {
#ifdef CONFIG_TASKBAR
    if (fTaskButton) {
        fTaskButton->repaint();
        fTaskButton->toolTip((const char *)client()->windowTitle());
    }
#endif
#ifdef CONFIG_TRAY
    if (fTrayIcon)
        fTrayIcon->toolTip((const char *)client()->windowTitle());
#endif
    if (fMiniIcon)
        fMiniIcon->toolTip(client()->iconTitle());

    if (isIconic())
        fMiniIcon->repaint();
}

void YFrameWindow::wmOccupyAllOrCurrent() {
    if (isSticky()) {
        workspace(manager->activeWorkspace());
        sticky(false);
    } else {
        sticky(true);
    }
#ifdef CONFIG_TASKBAR
    if (taskBar && taskBar->taskPane())
        taskBar->taskPane()->relayout();
#endif
#ifdef CONFIG_TRAY
    if (taskBar && taskBar->trayPane())
        taskBar->trayPane()->relayout();
#endif
}

void YFrameWindow::wmOccupyAll() {
    sticky(!isSticky());
    if (dontCover())
        manager->updateWorkArea();
#ifdef CONFIG_TASKBAR
    if (taskBar && taskBar->taskPane())
        taskBar->taskPane()->relayout();
#endif
#ifdef CONFIG_TRAY
    if (taskBar && taskBar->trayPane())
        taskBar->trayPane()->relayout();
#endif
}

void YFrameWindow::wmOccupyWorkspace(icewm::Workspace workspace) {
    PRECONDITION(workspace < workspaceCount);
    this->workspace(workspace);
}

void YFrameWindow::wmOccupyOnlyWorkspace(icewm::Workspace workspace) {
    PRECONDITION(workspace < workspaceCount);
    this->workspace(workspace);
    sticky(false);
}

void YFrameWindow::wmMoveToWorkspace(icewm::Workspace workspace) {
    wmOccupyOnlyWorkspace(workspace);
}

void YFrameWindow::updateFrameHints() {
#ifdef CONFIG_MOTIF_HINTS
    long decorations(client()->mwmDecorations());
    long functions(client()->mwmFunctions());
    long winHints(client()->winHints());
    MwmHints *mwmHints(client()->mwmHints());

    bool functionsOnly(mwmHints &&
                      (mwmHints->flags & (MWM_HINTS_DECORATIONS |
                                          MWM_HINTS_FUNCTIONS)) ==
                                          MWM_HINTS_FUNCTIONS);

    fFrameFunctions = 0;
    fFrameDecorations = 0;
    fFrameOptions = 0;

    if (decorations & MWM_DECOR_BORDER)   fFrameDecorations |= fdBorder;
    if (decorations & MWM_DECOR_RESIZEH)  fFrameDecorations |= fdResize;
    if (decorations & MWM_DECOR_TITLE)    fFrameDecorations |= fdTitleBar | fdDepth;
    if (decorations & MWM_DECOR_MENU)     fFrameDecorations |= fdSysMenu;
    if (decorations & MWM_DECOR_MAXIMIZE) fFrameDecorations |= fdMaximize;
    if (decorations & MWM_DECOR_MINIMIZE) fFrameDecorations |= fdMinimize | fdHide | fdRollup;

    if (functions & MWM_FUNC_MOVE) {
        fFrameFunctions |= ffMove;
        if (functionsOnly) fFrameDecorations |= fdBorder;
    }
    if (functions & MWM_FUNC_RESIZE)    {
        fFrameFunctions |= ffResize;
        if (functionsOnly) fFrameDecorations |= fdResize | fdBorder;
    }
    if (functions & MWM_FUNC_MAXIMIZE) {
        fFrameFunctions |= ffMaximize;
        if (functionsOnly) fFrameDecorations |= fdMaximize;
    }
    if (functions & MWM_FUNC_MINIMIZE) {
        fFrameFunctions |= ffMinimize | ffHide | ffRollup;
        if (functionsOnly) fFrameDecorations |= fdMinimize | fdHide | fdRollup;
    }
    if (functions & MWM_FUNC_CLOSE) {
        fFrameFunctions |= ffClose;
        fFrameDecorations |= fdClose;
    }
#else
    fFrameFunctions =
        ffMove | ffResize | ffClose |
        ffMinimize | ffMaximize | ffHide | ffRollup;
    fFrameDecorations =
        fdTitleBar | fdSysMenu | fdBorder | fdResize | fdDepth |
        fdClose | fdMinimize | fdMaximize | fdHide | fdRollup;

#endif

    if (winHints & WinHintsSkipFocus)
        fFrameOptions |= foIgnoreQSwitch;
    if (winHints & WinHintsSkipWindowMenu)
        fFrameOptions |= foIgnoreWinList;
    if (winHints & WinHintsSkipTaskBar)
        fFrameOptions |= foIgnoreTaskBar;
    if (winHints & WinHintsDontCover)
        fFrameOptions |= foDontCover;

#ifndef NO_WINDOW_OPTIONS
    WindowOption wo;

    windowOptions(wo, false);

    /*msg("decor: %lX %lX %lX %lX %lX %lX",
            wo.function_mask, wo.functions,
            wo.decor_mask, wo.decorations,
            wo.option_mask, wo.options);*/

    fFrameFunctions &= ~wo.function_mask;
    fFrameFunctions |= wo.functions;
    fFrameDecorations &= ~wo.decor_mask;
    fFrameDecorations |= wo.decors;
    fFrameOptions &= ~(wo.option_mask & fWinOptionMask);
    fFrameOptions |= (wo.options & fWinOptionMask);
#endif
}

#ifndef NO_WINDOW_OPTIONS

void YFrameWindow::windowOptions(WindowOption &opt, bool remove) {
    memset((void *)&opt, 0, sizeof(opt));

    opt.workspace = WinWorkspaceInvalid;
    opt.layer = WinLayerInvalid;
#ifdef CONFIG_TRAY
    opt.tray = IcewmTrayInvalid;
#endif

    if (defOptions) windowOptions(defOptions, opt, false);
    if (hintOptions) windowOptions(hintOptions, opt, remove);
}

void YFrameWindow::windowOptions(WindowOptions *list, WindowOption &opt,
                                 bool remove) {
    XClassHint const *h(client()->classHint());
    WindowOption *wo;

    if (!h) return;

    if (h->res_name && h->res_class) {
        char *both = new char[strlen(h->res_name) + 1 +
                              strlen(h->res_class) + 1];
        if (both) {
            strcpy(both, h->res_name);
            strcat(both, ".");
            strcat(both, h->res_class);
        }
        wo = both ? list->windowOption(both, false, remove) : 0;
        if (wo) WindowOptions::combineOptions(opt, *wo);
        delete[] both;
    }
    if (h->res_class) {
        wo = list->windowOption(h->res_class, false, remove);
        if (wo) WindowOptions::combineOptions(opt, *wo);
    }
    if (h->res_name) {
        wo = list->windowOption(h->res_name, false, remove);
        if (wo) WindowOptions::combineOptions(opt, *wo);
    }
    wo = list->windowOption(0, false, remove);
    if (wo) WindowOptions::combineOptions(opt, *wo);
}
#endif

void YFrameWindow::defaultOptions() {
#ifndef NO_WINDOW_OPTIONS
    WindowOption wo;
    windowOptions(wo, true);

    if (wo.icon) {
#ifndef LITE
        if (fFrameIcon) delete fFrameIcon;
        fFrameIcon = ::getIcon(wo.icon);
#endif
    }
    if (wo.workspace != WinWorkspaceInvalid && wo.workspace < workspaceCount)
        workspace(wo.workspace);
    if (wo.layer != WinLayerInvalid && wo.layer < WinLayerCount)
        layer(wo.layer);
#ifdef CONFIG_TRAY
    if (wo.tray != IcewmTrayInvalid && wo.tray < IcewmTrayOptionCount)
        trayOption(wo.tray);
#endif
#endif
}

#ifndef LITE
YIcon *newClientIcon(int count, int reclen, long * elem) {
    YIcon::Image * small(NULL), * large(NULL), * huge(NULL);
    if (reclen < 2)
        return 0;

    for (int i(0); i < count; i++, elem += reclen) {
        Pixmap pixmap(elem[0]), mask(elem[1]);

        if (pixmap == None) {
            warn("pixmap == None for subicon #%d", i);
            continue;
        }

        Window root;
        int x, y;
        unsigned w(0), h(0), border, depth(0);

        if (reclen >= 6) {
            w = elem[2];
            h = elem[3];
            depth = elem[4];
            root = elem[5];
        } else {
            if (BadDrawable == XGetGeometry(app->display(), pixmap,
                                            &root, &x, &y, &w, &h,
                                            &border, &depth)) {
                warn("BadDrawable for subicon #%d", i);
                continue;
            }
        }

        if (w != h || w == 0 || h == 0) {
            MSG(("Invalid pixmap size for subicon #%d: %dx%d", i, w, h));
            continue;
        }

        if (depth == app->depth()) {
            if (w == YIcon::sizeSmall)
                small = new YIcon::Image(pixmap, mask, w, h);
            else if (w == YIcon::sizeLarge)
                large = new YIcon::Image(pixmap, mask, w, h);
            else if (w == YIcon::sizeHuge)
                huge = new YIcon::Image(pixmap, mask, w, h);
        }
    }

    return (small || large || huge ? new YIcon(small, large, huge) : 0);
}

void YFrameWindow::updateIcon() {
    int count;
    long *elem;
    Pixmap *pixmap;
    Atom type;

    YIcon *oldFrameIcon(fFrameIcon);

    if (client()->updateWinIcons(type, count, elem)) {
        fFrameIcon = type == atoms.winIcons
                   ? newClientIcon(elem[0], elem[1], elem + 2)
                   : newClientIcon(count/2, 2, elem); // compatibility
        XFree(elem);
    } else if (client()->updateKwmIcon(count, pixmap) && count == 2) {
        XWMHints const *wmHints(client()->hints());

        if (wmHints && (wmHints->flags & IconPixmapHint)) {
            long pix[] = {
                pixmap[0], pixmap[1],
                wmHints->icon_pixmap,
                wmHints->flags & IconMaskHint ? wmHints->icon_mask : None
            };

            fFrameIcon = newClientIcon(2, 2, pix);
        } else {
            long pix[] = { pixmap[0], pixmap[1] };
            fFrameIcon = newClientIcon(1, 2, pix);
        }

        XFree(pixmap);
    } else {
        XWMHints const *wmHints(client()->hints());

        if (wmHints && (wmHints->flags & IconPixmapHint)) {
            long pix[] = {
                wmHints->icon_pixmap,
                wmHints->flags & IconMaskHint ? wmHints->icon_mask : None
            };

            fFrameIcon = newClientIcon(1, 2, pix);
        }
    }

    if (fFrameIcon && !(fFrameIcon->small() || fFrameIcon->large())) {
	delete fFrameIcon;
	fFrameIcon = NULL;
    }

    if (NULL == fFrameIcon) fFrameIcon = oldFrameIcon;
    else if (oldFrameIcon != fFrameIcon) delete oldFrameIcon;

// !!! BAH, we need an internal signaling framework
    if (menuButton()) menuButton()->repaint();
    if (miniIcon()) miniIcon()->repaint();
    if (fTrayIcon) fTrayIcon->repaint();
    if (fTaskButton) fTaskButton->repaint();
    if (windowList && fWinListItem)
    	windowList->list()->repaintItem(fWinListItem);

}
#endif

YFrameWindow *YFrameWindow::nextLayer() {
    if (fNextFrame) return fNextFrame;

    for (icewm::Layer l(layer()); l-- > 0; )
        if (manager->top(l)) return manager->top(l);

    return 0;
}

YFrameWindow *YFrameWindow::prevLayer() {
    if (fPrevFrame) return fPrevFrame;

    for (icewm::Layer l(layer() + 1); l < WinLayerCount; ++l)
        if (manager->bottom(l)) return manager->bottom(l);

    return 0;
}

YMenu *YFrameWindow::windowMenu() {
    //if (frameOptions() & foFullKeys)
    //    return windowMenuNoKeys;
    //else
    return ::windowMenu;
}

void YFrameWindow::addAsTransient() {
    Window groupLeader(client()->ownerWindow());

    if (groupLeader) {
        fOwner = manager->findFrame(groupLeader);

        if (fOwner) {
            MSG(("transient for 0x%lX: 0x%lX", groupLeader, fOwner));
	    PRECONDITION(fOwner->transient() != this);

            fNextTransient = fOwner->transient();
            fOwner->transient(this);
	}
    }
}

void YFrameWindow::removeAsTransient() {
    if (fOwner) {
        MSG(("removeAsTransient"));

        for (YFrameWindow * curr(fOwner->transient()), * prev(NULL);
	     curr; prev = curr, curr = curr->nextTransient()) {
	    if (curr == this) {
                if (prev) prev->nextTransient(nextTransient());
                else fOwner->transient(nextTransient());
		break;
            }
        }

        fOwner = NULL;
        fNextTransient = NULL;
    }
}

void YFrameWindow::addTransients() {
    for (YFrameWindow * w(manager->bottomLayer()); w; w = w->prevLayer())
        if (w->owner() == 0) w->addAsTransient();
}

void YFrameWindow::removeTransients() {
    if (transient()) {
        MSG(("removeTransients"));
        YFrameWindow *w = transient(), *n;

        while (w) {
            n = w->nextTransient();
            w->nextTransient(0);
            w->owner(0);
            w = n;
        }
        fTransient = 0;
    }
}

bool YFrameWindow::isModal() {
    if (!client())
        return false;

    MwmHints *mwmHints = client()->mwmHints();
    if (mwmHints && (mwmHints->flags & MWM_HINTS_INPUT_MODE))
        if (mwmHints->input_mode != MWM_INPUT_MODELESS)
            return true;

    if (hasModal())
        return true;

    return false;
}

bool YFrameWindow::hasModal() {
    YFrameWindow *w = transient();
    while (w) {
        if (w->isModal())
            return true;
        w = w->nextTransient();
    }
    /* !!! missing code for app modal dialogs */
    return false;
}

bool YFrameWindow::isFocusable() {
    if (hasModal())
        return false;

    if (!client())
        return false;

    XWMHints *hints = client()->hints();

    if (!hints)
        return true;
    if (frameOptions() & foIgnoreNoFocusHint)
        return true;
    if (!(hints->flags & InputHint))
        return true;
    if (hints->input)
        return true;
#if 1
    if (client()->protocols() & wm::TakeFocus)
        return true;
#endif
    return false;
}

void YFrameWindow::workspace(icewm::Workspace workspace) {
    if (workspace < workspaceCount &&
        workspace != fWorkspace) {
        client()->winWorkspace(fWorkspace = workspace);
        updateState();
        if (clickFocus || !strongPointerFocus)
            manager->focusTopWindow();
#ifdef CONFIG_TASKBAR
        updateTaskBar();
#endif
    }
}

void YFrameWindow::layer(icewm::Layer layer) {
    if (layer < WinLayerCount && layer != fLayer) {
        icewm::Layer const oldLayer(fLayer);

        removeFrame();
        fLayer = layer;
        insertFrame();
        client()->winLayer(fLayer);
        manager->restackWindows(this);

        if (limitByDockLayer &&
	   (this->layer() == WinLayerDock || oldLayer == WinLayerDock))
            manager->updateWorkArea();
    }
}

#ifdef CONFIG_TRAY
void YFrameWindow::trayOption(icewm::TrayOption option) {
    if (option < IcewmTrayOptionCount && option != fTrayOption) {
        client()->icewmTrayOption(fTrayOption = option);
#ifdef CONFIG_TASKBAR
        updateTaskBar();
#endif
    }
}
#endif

void YFrameWindow::updateState() {
    if (!isManaged())
        return ;

    client()->winState(WIN_STATE_ALL, fWinState);

    wm::State newState(NormalState);
    bool showFrame(true);
    bool showClient(true);

    // some code is probably against the ICCCM.
    // some applications misbehave either way.
    // (some hide windows on iconize, this is bad when switching workspaces
    // or rolling up the window).

    if (isHidden()) {
        showFrame = false;
        showClient = false;
        newState = IconicState;
    } else if (!visibleNow()) {
        showFrame = false;
        showClient = false;
        newState = isMinimized() || isIconic() ? IconicState : NormalState;
    } else if (isMinimized()) {
        showFrame = minimizeToDesktop;
        showClient = false;
        newState = IconicState;
    } else if (isRollup()) {
        showFrame = true;
        showClient = false;
        newState = NormalState; // ?
    } else {
        showFrame = true;
        showClient = true;
    }

    MSG(("updateState: winState=%lX, frame=%d, client=%d",
         fWinState, showFrame, showClient));

    client()->wmState(newState);

    if (showClient) {
        client()->show();
        fClientContainer->show();
    }

    if (showFrame) show(); else hide();

    if (!showClient) {
        fClientContainer->hide();
        client()->hide();
    }
}

void YFrameWindow::normalGeometry(int *x, int *y, int *w, int *h) {
    XSizeHints *sh = client()->sizeHints();
    bool cxw = true;
    bool cy = true;
    bool ch = true;

    *x = this->x() + borderX();
    *y = this->y() + borderY() + titleY();
    *w = client()->width();
    *h = client()->height();

    if (isIconic())
        cxw = cy = ch = false;
    else {
        if (isMaximizedHoriz())
            cxw = false;
        if (isMaximizedVert())
            cy = ch = false;
        if (isRollup())
            ch = false;
    }
    if (!cxw) {
        if (x) *x = normalX;
        if (w) *w = sh ? normalWidth * sh->width_inc + sh->base_width : normalWidth;
    }
    if (!cy)
        if (y) *y = normalY + titleY();
    if (!ch)
        if (h) *h = sh ? normalHeight * sh->height_inc + sh->base_height : normalHeight;
}

void YFrameWindow::updateNormalSize() {
    if (isIconic()) {
        iconX = this->x();
        iconY = this->y();
    } else {
        int nx = this->x() + borderX();
        int ny = this->y() + borderY();
        int nw = client()->width();
        int nh = client()->height();
        XSizeHints *sh = client()->sizeHints();
        bool cxw = true;
        bool cy = true;
        bool ch = true;

        if (isMaximizedHoriz())
            cxw = false;
        if (isMaximizedVert())
            cy = ch = false;
        if (isRollup())
            ch = false;

        if (cxw) {
            normalX = nx;
            normalWidth = sh ? (nw - sh->base_width) / sh->width_inc : nw;
        }
        if (cy)
            normalY = ny;
        if (ch)
            normalHeight = sh ? (nh - sh->base_height) / sh->height_inc : nh;
    }

    MSG(("updateNormalSize: (%d:%d %dx%d) icon (%d:%d)",
    	 normalX, normalY, normalWidth, normalHeight, iconX, iconY));
}

void YFrameWindow::updateLayout() {
    if (isIconic()) {
        if (iconX == -1 && iconY == -1)
            manager->iconPosition(this, &iconX, &iconY);

	geometry(iconX, iconY, fMiniIcon->width(), fMiniIcon->height());
    } else {
	XSizeHints *sh(client()->sizeHints());

	int nx(normalX);
	int ny(normalY);

	int nw(sh ? normalWidth * sh->width_inc + sh->base_width
		  : normalWidth);
	int nh(sh ? normalHeight * sh->height_inc + sh->base_height
		  : normalHeight);

	int const maxWidth(manager->maxWidth(this));
	int const maxHeight(manager->maxHeight(this) - titleY());

        if (isMaximizedHoriz()) nw = maxWidth;
        if (isMaximizedVert()) nh = maxHeight;
/*
	if (!dontCover()) {
	    nx = min(nx, manager->maxX(layer()) - nw);
	    nx = max(nx, manager->minX(layer()));
	    nw = min(nw, manager->maxX(layer()) -
                 (considerHorizBorder ? nx + 2 * (int) borderX() : nx));

	    ny = min(ny, manager->maxY(layer()) - (int)titleY() - nh);
	    ny = max(ny, manager->minY(layer()));
	    nh = min(nh, manager->maxY(layer()) - (int)titleY() -
                 (considerVertBorder ? ny + 2 * (int) borderY() : ny));
	}
*/
        client()->constrainSize(nw, nh, layer());

	if (!isMaximizedHoriz()) {
            nx-= borderX();
            nw+= 2 * borderX();
        } else {
	    nx = manager->minX(this);

            if (!considerHorizBorder) nw+= 2 * borderX();
            if (centerMaximizedWindows && !(sh && (sh->flags & PMaxSize)))
                nx+= (maxWidth - nw) / 2;
            else if (!considerHorizBorder)
                nx-= borderX();
        }

	if (!isMaximizedVert()) {
	    ny-= borderY();
            nh+= 2 * borderY();
        } else {
	    ny = manager->minY(this);

	    if (!considerVertBorder) nh+= 2 * borderY();
            if (centerMaximizedWindows && !(sh && (sh->flags & PMaxSize)))
                ny+= (maxHeight - nh) / 2;
            else if (!considerVertBorder)
	        ny-= borderY();
        }

        if (isRollup()) nh = 2 * borderY();

	geometry(nx, ny, nw, nh + titleY());
    }
}

void YFrameWindow::state(gnome::State mask, gnome::State state) {
    updateNormalSize(); // !!! fix this to move below (or remove totally)

    gnome::State fOldState(fWinState);
    gnome::State fNewState((fWinState & ~mask) | (state & mask));

    // !!! this should work
    //if (fNewState == fOldState)
    //    return ;

    // !!! move here

    fWinState = fNewState;

    if ((fOldState ^ fNewState) & WinStateAllWorkspaces) {
        MSG(("WinStateAllWorkspaces: %d", isSticky()));
#ifdef CONFIG_TASKBAR
        updateTaskBar();
#endif
    }
    MSG(("state: oldState: %lX, newState: %lX, mask: %lX, state: %lX",
         fOldState, fNewState, mask, state));
    //msg("normal1: (%d:%d %dx%d)", normalX, normalY, normalWidth, normalHeight);
    if ((fOldState ^ fNewState) & (WinStateMaximizedVert |
                                   WinStateMaximizedHoriz))
    {
        MSG(("WinStateMaximized: %d", isMaximized()));

        if (fMaximizeButton)
            if (isMaximized()) {
                fMaximizeButton->actions(actionRestore, actionRestore);
                fMaximizeButton->toolTip(_("Restore"));
            } else {
                fMaximizeButton->actions(actionMaximize, actionMaximizeVert);
                fMaximizeButton->toolTip(_("Maximize"));
            }
    }
    if ((fOldState ^ fNewState) & WinStateMinimized) {
        MSG(("WinStateMinimized: %d", isMaximized()));
        if (fNewState & WinStateMinimized)
            minimizeTransients();
        else if (owner() && owner()->isMinimized())
            owner()->state(WinStateMinimized, 0);

        if (minimizeToDesktop && fMiniIcon) {
            if (isIconic()) {
                fMiniIcon->raise();
                fMiniIcon->show();
            } else {
                fMiniIcon->hide();
                iconX = x();
                iconY = y();
            }
        }
#ifdef CONFIG_TASKBAR
        updateTaskBar();
#endif
        if (clickFocus || !strongPointerFocus)
            manager->focusTopWindow();
    }
    if ((fOldState ^ fNewState) & WinStateRollup) {
        MSG(("WinStateRollup: %d", isRollup()));
        if (fRollupButton) {
            if (isRollup()) {
                fRollupButton->toolTip(_("Rolldown"));
            } else {
                fRollupButton->toolTip(_("Rollup"));
            }
            fRollupButton->repaint();
        }
    }
    if ((fOldState ^ fNewState) & WinStateHidden) {
        MSG(("WinStateHidden: %d", isHidden()));

        if (fNewState & WinStateHidden)
            hideTransients();
        else if (owner() && owner()->isHidden())
            owner()->state(WinStateHidden, 0);

#ifdef CONFIG_TASKBAR
        updateTaskBar();
#endif
    }
#if 0 //!!!
    if ((fOldState ^ fNewState) & WinStateDockHorizontal) {
        if (dontCover())
            manager->updateWorkArea();
    }
#endif

    updateState();
    updateLayout();

#ifdef CONFIG_SHAPE
    if ((fOldState ^ fNewState) & (WinStateRollup | WinStateMinimized))
        setShape();
#endif
    if ((fOldState ^ fNewState) & WinStateMinimized) {
        if (!(fNewState & WinStateMinimized))
            restoreMinimizedTransients();
    }
    if ((fOldState ^ fNewState) & WinStateHidden) {
        if (!(fNewState & WinStateHidden))
            restoreHiddenTransients();
    }
    if ((clickFocus || !strongPointerFocus) &&
        this == manager->focus() &&
        ((fOldState ^ fNewState) & WinStateRollup)) {
        manager->focus(this);
    }
}

void YFrameWindow::sticky(bool sticky) {
    state(WinStateAllWorkspaces, sticky ? WinStateAllWorkspaces : 0);

    if (dontCover())
	manager->updateWorkArea();
}

void YFrameWindow::dontCover(bool dontCover) {
    long winHints = client()->winHints();
    fWinOptionMask&= ~foDontCover;

    if (dontCover) {
	fFrameOptions|= foDontCover;
	winHints|= WinHintsDontCover;
    } else {
	fFrameOptions&= ~foDontCover;
	winHints&= ~WinHintsDontCover;
    }

    client()->winHints(winHints);
    manager->updateWorkArea();
}

void YFrameWindow::updateMwmHints() {
    int bx = borderX();
    int by = borderY();
    int ty = titleY(), tt;

    updateFrameHints();

    int gx, gy;
    client()->gravityOffsets(gx, gy);

#ifdef TITLEBAR_BOTTOM
    if (gy == -1)
#else
    if (gy == 1)
#endif
        tt = titleY() - ty;
    else
        tt = 0;

    if (!isRollup() && !isIconic()) /// !!! check (emacs hates this)
        configureClient(x() + bx - borderX() + borderX(),
                y() + by - borderY() + tt + borderY() + titleY(),
                client()->width(), client()->height());
}

#ifndef LITE
YIcon *YFrameWindow::updateClientIcon() const {
#warning i am not updateClientIcon
    for(YFrameWindow const *f(this); f != NULL; f = f->owner())
        if (f->clientIcon()) return f->clientIcon();

    return defaultAppIcon;
}
#endif

void YFrameWindow::updateProperties() {
    client()->winWorkspace(fWorkspace);
    client()->winLayer(fLayer);
#ifdef CONFIG_TRAY
    client()->icewmTrayOption(fTrayOption);
#endif
    client()->winState(WIN_STATE_ALL, fWinState);
}

#ifdef CONFIG_TASKBAR
void YFrameWindow::updateTaskBar() {
#ifdef CONFIG_TRAY
    bool needTrayIcon(false);
    int dw(0);

    if (taskBar && fManaged && taskBar->trayPane()) {
        if (!isHidden() &&
            !(frameOptions() & foIgnoreTaskBar) &&
	    (trayOption() != IcewmTrayIgnore))
            if (trayShowAllWindows || visibleOn(manager->activeWorkspace()))
                needTrayIcon = true;

        if (needTrayIcon && NULL == fTrayIcon)
            fTrayIcon = taskBar->trayPane()->add(this);

        if (fTrayIcon) {
            fTrayIcon->shown(needTrayIcon);
            if (fTrayIcon->shown()) ///!!! optimize
                fTrayIcon->repaint();
        }
        /// !!! optimize

        TrayPane *tp = taskBar->trayPane();
	int const nw(tp->requiredWidth());

        if ((dw = nw - tp->width()))
            taskBar->trayPane()->geometry(tp->x() - dw, tp->y(), nw, tp->height());

        taskBar->trayPane()->relayout();
    }
#endif

    bool needTaskButton(false);

    if (taskBar && fManaged && taskBar->taskPane()) {
#ifndef CONFIG_TRAY
        if (!(isHidden() || (frameOptions() & foIgnoreTaskBar))
#else
        if (!(isHidden() || (frameOptions() & foIgnoreTaskBar)) &&
            (trayOption() == IcewmTrayIgnore ||
            (trayOption() == IcewmTrayMinimized && !isMinimized())))
#endif
            if (taskBarShowAllWindows || visibleOn(manager->activeWorkspace()))
                needTaskButton = true;

        if (needTaskButton && fTaskButton == 0)
            fTaskButton = taskBar->taskPane()->add(this);

        if (fTaskButton) {
            fTaskButton->shown(needTaskButton);
            if (fTaskButton->shown()) ///!!! optimize
                fTaskButton->repaint();
        }
        /// !!! optimize

#ifdef CONFIG_TRAY
	if (dw) taskBar->taskPane()->size
	    (taskBar->taskPane()->width() - dw, taskBar->taskPane()->height());
#endif
        taskBar->taskPane()->relayout();
    }

    if (dw && NULL == taskBar->taskPane() && NULL != taskBar->addressBar())
	taskBar->addressBar()->size
	    (taskBar->addressBar()->width() - dw,
	     taskBar->addressBar()->height());
}
#endif

void YFrameWindow::handleMsgBox(YMsgBox *msgbox, int operation) {
    //msg("msgbox operation %d", operation);
    if (msgbox == fKillMsgBox && fKillMsgBox) {
        if (fKillMsgBox) {
            manager->unmanageClient(fKillMsgBox->handle());
            fKillMsgBox = 0;
            if (clickFocus || !strongPointerFocus)
                manager->focusTopWindow();
        }
        if (operation == YMsgBox::mbOK)
            wmKill();
    }
}
