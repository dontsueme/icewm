/*
 * IceWM
 *
 * Copyright (C) 1997-2001 Marko Macek
 */
#include "config.h"
#include "yfull.h"
#include "wmmgr.h"

#include "aaddressbar.h"
#include "atasks.h"
#include "aworkspaces.h"
#include "sysdep.h"
#include "wmtaskbar.h"
#include "wmwinlist.h"
#include "objmenu.h"
#include "wmswitch.h"
#include "wmstatus.h"
#include "wmminiicon.h"
#include "wmcontainer.h"
#include "wmframe.h"
#include "wmtraywin.h"
#include "wmdialog.h"
#include "wmsession.h"
#include "wmapp.h"
#include "wmaction.h"
#include "wmprog.h"
#include "prefs.h"

#ifdef CONFIG_GNOME_HINTS
YAction *layerActionSet[WinLayerCount];
#endif // CONFIG_GNOME_HINTS

#ifdef CONFIG_TRAY
YAction *trayOptionActionSet[IcewmTrayOptionCount];
#endif // CONFIG_TRAY

XContext YWindowManager::frameContext(XUniqueContext());
XContext YWindowManager::clientContext(XUniqueContext());
#if CONFIG_KDE_TRAY_WINDOWS
XContext YWindowManager::trayContext(XUniqueContext());
#endif

YWindowManager::YWindowManager(YWindow *parent, Window win):
YDesktop(parent, win),
fShuttingDown(false),
fWorkAreaMoveWindows(false),

fFocusWin(NULL),
fFirstCreated(NULL), fLastCreated(NULL),
fColormapWindow(NULL), fRootProxy(NULL),
fTopWin(NULL),

fActiveWorkspace(WinWorkspaceInvalid),
fLastWorkspace(WinWorkspaceInvalid),
fMinX(0), fMinY(0), fMaxX(width()), fMaxY(height()),

fArrangeCount(0),
fArrangeInfo(NULL) {

    for (icewm::Layer layer(0); layer < WinLayerCount; ++layer) {
        layerActionSet[layer] = new YAction();
        fTop[layer] = fBottom[layer] = NULL;
    }
#ifdef CONFIG_TRAY
    for (icewm::TrayOption option(0); option < IcewmTrayOptionCount; ++option)
        trayOptionActionSet[option] = new YAction();
#endif // CONFIG_TRAY

    style(wsManager);
    pointer(YApplication::leftPointer);
    title(name());

    registerProtocols();

    fTopWin = new YWindow();;
    if (edgeHorzWorkspaceSwitching) {
        fLeftSwitch = new EdgeSwitch(this, -1, false);
        if (fLeftSwitch) {
            fLeftSwitch->geometry(0, 0, 1, height());
            fLeftSwitch->show();
        }
        fRightSwitch = new EdgeSwitch(this, +1, false);
        if (fRightSwitch) {
            fRightSwitch->geometry(width() - 1, 0, 1, height());
            fRightSwitch->show();
        }
    } else {
        fLeftSwitch = fRightSwitch = 0;
    }

    if (edgeVertWorkspaceSwitching) {
        fTopSwitch = new EdgeSwitch(this, -1, true);
        if (fTopSwitch) {
            fTopSwitch->geometry(0, 0, width(), 1);
            fTopSwitch->show();
        }
        fBottomSwitch = new EdgeSwitch(this, +1, true);
        if (fBottomSwitch) {
            fBottomSwitch->geometry(0, height() - 1, width(), 1);
            fBottomSwitch->show();
        }
    } else {
        fTopSwitch = fBottomSwitch = 0;
    }

    YWindow::windowFocus();
}

YWindowManager::~YWindowManager() {
    unregisterProtocols();
}

void YWindowManager::registerProtocols() {
    Atom protocols[] = {
#ifdef CONFIG_GNOME_HINTS
	atoms.winWorkspace,
	atoms.winWorkspaceCount,
	atoms.winWorkspaceNames,
	atoms.winIcons,
	atoms.winWorkarea,
	atoms.winState,
	atoms.winHints,
	atoms.winLayer,
	atoms.winSupportingWmCheck,
	atoms.winClientList,
#endif // CONFIG_GNOME_HINTS

#ifdef CONFIG_WMSPEC_HINTS
        atoms.netSupported,
        atoms.netClientList,
        atoms.netClientListStacking,
        atoms.netNumberOfDesktops,
        atoms.netDesktopGeometry,
        atoms.netDesktopViewport,
        atoms.netCurrentDesktop,
//        atoms.netDesktopName,
        atoms.netActiveWindow,
        atoms.netWorkarea,
        atoms.netSupportingWmCheck,
//        atoms.netCloseWindow,
//        atoms.netwmMoveResize,
//        atoms.netwmName,
//        atoms.netwmIconName,
//        atoms.netwmDesktop,
//        atoms.netwmWindowType,
//        atoms.netwmWindowTypeDesktop,
//        atoms.netwmWindowTypeDock,
//        atoms.netwmWindowTypeToolbar,
//        atoms.netwmWindowTypeMenu,
//        atoms.netwmWindowTypeDialog,
//        atoms.netwmWindowTypeNormal,
//        atoms.netwmState,
//        atoms.netwmStateModal,
//        atoms.netwmStateSticky,
//        atoms.netwmStateMaximizedVert,
//        atoms.netwmStateMaximizedHorz,
//        atoms.netwmStateShaded,
//        atoms.netwmStateSkipTaskbar,
//        atoms.netwmStateSkipPager,
//        atoms.netwmStateFullscreen,
//        atoms.netwmState,
//        atoms.netwmStrut,
//        atoms.netwmIcon,
//        atoms.netwmPid,
//        atoms.netwmHandledIcon,
//        atoms.netwmPing,
#endif // CONFIG_WMSPEC_HINTS

#ifdef CONFIG_KDE_HINTS
        atoms.kwmWinIcon,
#ifdef CONFIG_TRAY
        atoms.kwmDockwindow,
#ifdef CONFIG_WMSPEC_HINTS
        atoms.kdeNetSystemTrayWindows,
//        atoms.kdeNetwmSystemTrayWindowFor,
#endif // CONFIG_TRAY
#endif // CONFIG_WMSPEC_HINTS
#endif // CONFIG_KDE_HINTS

#ifdef CONFIG_TRAY
	atoms.icewmTrayOption,
#endif // CONFIG_TRAY
    };

#ifdef CONFIG_GNOME_HINTS
    XChangeProperty(app->display(), handle(),
                    atoms.winProtocols, XA_ATOM, 32, PropModeReplace,
                    (unsigned char *) protocols, ACOUNT(protocols));
#endif // CONFIG_GNOME_HINTS

#ifdef CONFIG_WMSPEC_HINTS
    XChangeProperty(app->display(), handle(),
                    atoms.netSupported, XA_ATOM, 32, PropModeReplace,
                    (unsigned char *) protocols, ACOUNT(protocols));
#endif // CONFIG_WMSPEC_HINTS

#if CONFIG_GNOME_OR_WMSPEC_HINTS
    YWindow *checkWindow(new YWindow());
    Window xid(checkWindow->handle());
#endif // CONFIG_GNOME_OR_WMSPEC_HINTS

#ifdef CONFIG_GNOME_HINTS
    XChangeProperty(app->display(), checkWindow->handle(),
                    atoms.winSupportingWmCheck, XA_CARDINAL, 32,
                    PropModeReplace, (unsigned char *)&xid, 1);

    XChangeProperty(app->display(), handle(),
                    atoms.winSupportingWmCheck, XA_CARDINAL, 32,
                    PropModeReplace, (unsigned char *)&xid, 1);
#endif // CONFIG_GNOME_HINTS

#ifdef CONFIG_WMSPEC_HINTS
    XChangeProperty(app->display(), checkWindow->handle(),
                    atoms.netSupportingWmCheck, XA_WINDOW, 32,
                    PropModeReplace, (unsigned char *)&xid, 1);

    XChangeProperty(app->display(), handle(),
                    atoms.netSupportingWmCheck, XA_WINDOW, 32,
                    PropModeReplace, (unsigned char *)&xid, 1);
#endif // CONFIG_WMSPEC_HINTS

#ifdef CONFIG_GNOME_HINTS
    yint32 const ac[] = { 1, 1 };
    yint32 const ca[] = { 0, 0 };

    XChangeProperty(app->display(), handle(),
                    atoms.winAreaCount, XA_CARDINAL, 32,
                    PropModeReplace, (unsigned char *)&ac, 2);
    XChangeProperty(app->display(), handle(),
                    atoms.winArea, XA_CARDINAL, 32,
                    PropModeReplace, (unsigned char *)&ca, 2);
#endif // CONFIG_GNOME_HINTS
}

void YWindowManager::unregisterProtocols() {
    XDeleteProperty(app->display(), handle(), atoms.wmProtocols);
    XDeleteProperty(app->display(), handle(), atoms.wmName);

#ifdef CONFIG_GNOME_HINTS
    XDeleteProperty(app->display(), handle(), atoms.winProtocols);
#endif // CONFIG_GNOME_HINTS

#ifdef CONFIG_WMSPEC_HINTS
    XDeleteProperty(app->display(), handle(), atoms.netSupported);
    XDeleteProperty(app->display(), handle(), atoms.netwmName);
#endif // CONFIG_WMSPEC_HINTS

    if (supportSemitransparency) {
        XDeleteProperty(app->display(), handle(), atoms.xrootPixmapId);
        XDeleteProperty(app->display(), handle(), atoms.xrootColorPixel);
    }
}

void YWindowManager::initWorkspaces() {
    icewm::Workspace workspace(0);

#ifdef CONFIG_GNOME_HINTS
    XTextProperty names;

    if (XStringListToTextProperty(workspaceNames, ::workspaceCount, &names)) {
        XSetTextProperty(app->display(), handle(),
                         &names, atoms.winWorkspaceNames);
        XFree(names.value);
    }

    XChangeProperty(app->display(), handle(),
                    atoms.winWorkspaceCount, XA_CARDINAL,
                    32, PropModeReplace, (unsigned char *)&::workspaceCount, 1);
#endif // CONFIG_GNOME_HINTS

#ifdef CONFIG_WMSPEC_HINTS
    XChangeProperty(app->display(), handle(),
                    atoms.netNumberOfDesktops, XA_CARDINAL,
                    32, PropModeReplace, (unsigned char *)&::workspaceCount, 1);

    yint32 geometry[] = { width(), height() };
    XChangeProperty(app->display(), handle(),
                    atoms.netDesktopGeometry, XA_CARDINAL,
                    32, PropModeReplace, (unsigned char *)geometry, 2);

    yint32 viewports[2 * ::workspaceCount];
    memset(viewports, 0, sizeof(viewports));
    XChangeProperty(app->display(), handle(),
                    atoms.netDesktopViewport, XA_CARDINAL,
                    32, PropModeReplace, (unsigned char *)viewports,
                    2 * ::workspaceCount);


    YWindowProperty netCurrentDesktop(handle(),
                                      atoms.netCurrentDesktop, XA_CARDINAL);
    if (Success == netCurrentDesktop && netCurrentDesktop.count()) {
        workspace = netCurrentDesktop.template data<netwm::Desktop>();
        if (workspace >= ::workspaceCount) workspace = 0;
    }
    else
#endif // CONFIG_WMSPEC_HINTS
#ifdef CONFIG_GNOME_HINTS
    {
        YWindowProperty winWorkspace(handle(),
                                     atoms.winWorkspace, XA_CARDINAL);
        if (Success == winWorkspace && winWorkspace.count()) {
            workspace = winWorkspace.template data<long>();
            if (workspace >= ::workspaceCount) workspace = 0;
        }
    }
#endif // CONFIG_GNOME_HINTS

    activateWorkspace(workspace);
}

void YWindowManager::grabKeys() {
#ifdef CONFIG_ADDRESSBAR
    if (taskBar && taskBar->addressBar())
        GRAB_WMKEY(gKeySysAddressBar);
#endif // CONFIG_ADDRESSBAR
    if (runDlgCommand && runDlgCommand[0])
        GRAB_WMKEY(gKeySysRun);
    if (quickSwitch) {
        GRAB_WMKEY(gKeySysSwitchNext);
        GRAB_WMKEY(gKeySysSwitchLast);
    }
    GRAB_WMKEY(gKeySysWinNext);
    GRAB_WMKEY(gKeySysWinPrev);
    GRAB_WMKEY(gKeySysDialog);

    GRAB_WMKEY(gKeySysWorkspacePrev);
    GRAB_WMKEY(gKeySysWorkspaceNext);
    GRAB_WMKEY(gKeySysWorkspaceLast);

    GRAB_WMKEY(gKeySysWorkspacePrevTakeWin);
    GRAB_WMKEY(gKeySysWorkspaceNextTakeWin);
    GRAB_WMKEY(gKeySysWorkspaceLastTakeWin);

    GRAB_WMKEY(gKeySysWinMenu);
    GRAB_WMKEY(gKeySysMenu);
    GRAB_WMKEY(gKeySysWindowList);

    GRAB_WMKEY(gKeySysWorkspace1);
    GRAB_WMKEY(gKeySysWorkspace2);
    GRAB_WMKEY(gKeySysWorkspace3);
    GRAB_WMKEY(gKeySysWorkspace4);
    GRAB_WMKEY(gKeySysWorkspace5);
    GRAB_WMKEY(gKeySysWorkspace6);
    GRAB_WMKEY(gKeySysWorkspace7);
    GRAB_WMKEY(gKeySysWorkspace8);
    GRAB_WMKEY(gKeySysWorkspace9);
    GRAB_WMKEY(gKeySysWorkspace10);
    GRAB_WMKEY(gKeySysWorkspace11);
    GRAB_WMKEY(gKeySysWorkspace12);

    GRAB_WMKEY(gKeySysWorkspace1TakeWin);
    GRAB_WMKEY(gKeySysWorkspace2TakeWin);
    GRAB_WMKEY(gKeySysWorkspace3TakeWin);
    GRAB_WMKEY(gKeySysWorkspace4TakeWin);
    GRAB_WMKEY(gKeySysWorkspace5TakeWin);
    GRAB_WMKEY(gKeySysWorkspace6TakeWin);
    GRAB_WMKEY(gKeySysWorkspace7TakeWin);
    GRAB_WMKEY(gKeySysWorkspace8TakeWin);
    GRAB_WMKEY(gKeySysWorkspace9TakeWin);
    GRAB_WMKEY(gKeySysWorkspace10TakeWin);
    GRAB_WMKEY(gKeySysWorkspace11TakeWin);
    GRAB_WMKEY(gKeySysWorkspace12TakeWin);

    GRAB_WMKEY(gKeySysTileVertical);
    GRAB_WMKEY(gKeySysTileHorizontal);
    GRAB_WMKEY(gKeySysCascade);
    GRAB_WMKEY(gKeySysArrange);
    GRAB_WMKEY(gKeySysUndoArrange);
    GRAB_WMKEY(gKeySysArrangeIcons);
    GRAB_WMKEY(gKeySysMinimizeAll);
    GRAB_WMKEY(gKeySysHideAll);

    {
        KProgram *k = keyProgs;
        while (k) {
            grabVKey(k->key(), k->modifiers());
            k = k->next();
        }
    }
    if (app->WinMask) {
        //fix -- allow apps to use remaining key combos (except single press)
        if (app->Win_L) grabKey(app->Win_L, 0);
        if (app->Win_R) grabKey(app->Win_R, 0);
    }

    if (useMouseWheel) {
        grabButton(4, ControlMask | app->AltMask);
        grabButton(5, ControlMask | app->AltMask);
        if (app->WinMask) {
            grabButton(4, app->WinMask);
            grabButton(5, app->WinMask);
        }
    }
}

void YWindowManager::setupRootProxy() {
#if CONFIG_GNOME_HINTS
    if (grabRootWindow) {
        fRootProxy = new YProxyWindow(0);

        if (fRootProxy) {
            fRootProxy->style(wsOverrideRedirect);
            XID rid = fRootProxy->handle();

            XChangeProperty(app->display(), handle(),
                            atoms.winDesktopButtonProxy, XA_CARDINAL, 32,
                            PropModeReplace, (unsigned char *)&rid, 1);
            XChangeProperty(app->display(), fRootProxy->handle(),
                            atoms.winDesktopButtonProxy, XA_CARDINAL, 32,
                            PropModeReplace, (unsigned char *)&rid, 1);
        }
    }
#endif // CONFIG_GNOME_HINTS
}

bool YWindowManager::handleKey(const XKeyEvent &key) {
    YFrameWindow *frame = focus();

    if (key.type == KeyPress) {
        KeySym k = XKeycodeToKeysym(app->display(), key.keycode, 0);
        unsigned int m = KEY_MODMASK(key.state);
        unsigned int vm = VMod(m);

        MSG(("down key: %d, mod: %d", k, m));

#ifndef LITE
        if (quickSwitch && switchWindow) {
            if (IS_WMKEY(k, vm, gKeySysSwitchNext)) {
                switchWindow->begin(1, key.state);
            } else if (IS_WMKEY(k, vm, gKeySysSwitchLast)) {
                switchWindow->begin(0, key.state);
            }
        }
#endif // LITE
        if (IS_WMKEY(k, vm, gKeySysWinNext)) {
            if (frame) frame->wmNextWindow();
        } else if (IS_WMKEY(k, vm, gKeySysWinPrev)) {
            if (frame) frame->wmPrevWindow();
        } else if (IS_WMKEY(k, vm, gKeySysWinMenu)) {
            if (frame) frame->popupSystemMenu();
#ifndef LITE
        } else if (IS_WMKEY(k, vm, gKeySysDialog)) {
            if (ctrlAltDelete) ctrlAltDelete->activate();
#endif // LITE
        } else if (IS_WMKEY(k, vm, gKeySysMenu)) {
            popupStartMenu();
#ifdef CONFIG_WINLIST
        } else if (IS_WMKEY(k, vm, gKeySysWindowList)) {
            if (windowList) windowList->showFocused(-1, -1);
#endif // CONFIG_WINLIST
        } else if (IS_WMKEY(k, vm, gKeySysWorkspacePrev)) {
            XUngrabKeyboard(app->display(), CurrentTime);
            switchToPrevWorkspace(false);
        } else if (IS_WMKEY(k, vm, gKeySysWorkspaceNext)) {
            XUngrabKeyboard(app->display(), CurrentTime);
            switchToNextWorkspace(false);
        } else if (IS_WMKEY(k, vm, gKeySysWorkspaceLast)) {
            XUngrabKeyboard(app->display(), CurrentTime);
            switchToLastWorkspace(false);
        } else if (IS_WMKEY(k, vm, gKeySysWorkspacePrevTakeWin)) {
            switchToPrevWorkspace(true);
        } else if (IS_WMKEY(k, vm, gKeySysWorkspaceNextTakeWin)) {
            switchToNextWorkspace(true);
        } else if (IS_WMKEY(k, vm, gKeySysWorkspaceLastTakeWin)) {
            switchToLastWorkspace(true);
        } else if (IS_WMKEY(k, vm, gKeySysWorkspace1)) {
            switchToWorkspace(0, false);
        } else if (IS_WMKEY(k, vm, gKeySysWorkspace2)) {
            switchToWorkspace(1, false);
        } else if (IS_WMKEY(k, vm, gKeySysWorkspace3)) {
            switchToWorkspace(2, false);
        } else if (IS_WMKEY(k, vm, gKeySysWorkspace4)) {
            switchToWorkspace(3, false);
        } else if (IS_WMKEY(k, vm, gKeySysWorkspace5)) {
            switchToWorkspace(4, false);
        } else if (IS_WMKEY(k, vm, gKeySysWorkspace6)) {
            switchToWorkspace(5, false);
        } else if (IS_WMKEY(k, vm, gKeySysWorkspace7)) {
            switchToWorkspace(6, false);
        } else if (IS_WMKEY(k, vm, gKeySysWorkspace8)) {
            switchToWorkspace(7, false);
        } else if (IS_WMKEY(k, vm, gKeySysWorkspace9)) {
            switchToWorkspace(8, false);
        } else if (IS_WMKEY(k, vm, gKeySysWorkspace10)) {
            switchToWorkspace(9, false);
        } else if (IS_WMKEY(k, vm, gKeySysWorkspace11)) {
            switchToWorkspace(10, false);
        } else if (IS_WMKEY(k, vm, gKeySysWorkspace12)) {
            switchToWorkspace(11, false);
        } else if (IS_WMKEY(k, vm, gKeySysWorkspace1TakeWin)) {
            switchToWorkspace(0, true);
        } else if (IS_WMKEY(k, vm, gKeySysWorkspace2TakeWin)) {
            switchToWorkspace(1, true);
        } else if (IS_WMKEY(k, vm, gKeySysWorkspace3TakeWin)) {
            switchToWorkspace(2, true);
        } else if (IS_WMKEY(k, vm, gKeySysWorkspace4TakeWin)) {
            switchToWorkspace(3, true);
        } else if (IS_WMKEY(k, vm, gKeySysWorkspace5TakeWin)) {
            switchToWorkspace(4, true);
        } else if (IS_WMKEY(k, vm, gKeySysWorkspace6TakeWin)) {
            switchToWorkspace(5, true);
        } else if (IS_WMKEY(k, vm, gKeySysWorkspace7TakeWin)) {
            switchToWorkspace(6, true);
        } else if (IS_WMKEY(k, vm, gKeySysWorkspace8TakeWin)) {
            switchToWorkspace(7, true);
        } else if (IS_WMKEY(k, vm, gKeySysWorkspace9TakeWin)) {
            switchToWorkspace(8, true);
        } else if (IS_WMKEY(k, vm, gKeySysWorkspace10TakeWin)) {
            switchToWorkspace(9, true);
        } else if (IS_WMKEY(k, vm, gKeySysWorkspace11TakeWin)) {
            switchToWorkspace(10, true);
        } else if (IS_WMKEY(k, vm, gKeySysWorkspace12TakeWin)) {
            switchToWorkspace(11, true);
	} else if(IS_WMKEY(k, vm, gKeySysTileVertical)) {
	    wmapp->actionPerformed(actionTileVertical, 0);
	} else if(IS_WMKEY(k, vm, gKeySysTileHorizontal)) {
	    wmapp->actionPerformed(actionTileHorizontal, 0);
	} else if(IS_WMKEY(k, vm, gKeySysCascade)) {
	    wmapp->actionPerformed(actionCascade, 0);
	} else if(IS_WMKEY(k, vm, gKeySysArrange)) {
	    wmapp->actionPerformed(actionArrange, 0);
	} else if(IS_WMKEY(k, vm, gKeySysUndoArrange)) {
	    wmapp->actionPerformed(actionUndoArrange, 0);
	} else if(IS_WMKEY(k, vm, gKeySysArrangeIcons)) {
	    wmapp->actionPerformed(actionArrangeIcons, 0);
	} else if(IS_WMKEY(k, vm, gKeySysMinimizeAll)) {
	    wmapp->actionPerformed(actionMinimizeAll, 0);
	} else if(IS_WMKEY(k, vm, gKeySysHideAll)) {
	    wmapp->actionPerformed(actionHideAll, 0);
#ifdef CONFIG_ADDRESSBAR
        } else if (IS_WMKEY(k, vm, gKeySysAddressBar)) {
            if (taskBar) {
                taskBar->popOut();
                if (taskBar->addressBar())
                    taskBar->addressBar()->windowFocus();
            }
#endif // CONFIG_ADDRESSBAR
        } else if (IS_WMKEY(k, vm, gKeySysRun)) {
            if (runDlgCommand && runDlgCommand[0])
                app->runCommand(runDlgCommand);
        } else {
            KProgram *p = keyProgs;
            while (p) {
                //msg("%X=%X %X=%X", k, p->key(), vm, p->modifiers());
                if (p->isKey(k, vm))
                    p->open();
                p = p->next();
            }
        }

        if (app->WinMask) {
            if (k == app->Win_L || k == app->Win_R) {
                /// !!! needs sync grab
                XAllowEvents(app->display(), ReplayKeyboard, CurrentTime);
            } else if (m & app->WinMask) {
                /// !!! needs sync grab
                XAllowEvents(app->display(), ReplayKeyboard, CurrentTime);
            }
        }
    } else if (key.type == KeyRelease) {
#ifdef DEBUG
        KeySym k = XKeycodeToKeysym(app->display(), key.keycode, 0);
        unsigned int m = KEY_MODMASK(key.state);

        MSG(("up key: %d, mod: %d", k, m));
#endif // DEBUG
    }
    return true;
}

void YWindowManager::handleButton(const XButtonEvent &button) {
    if (fRootProxy && button.window == handle() &&
        !(useRootButtons & (1 << (button.button - 1))) &&
       !((button.state & (ControlMask + app->AltMask)) == ControlMask + app->AltMask))
    {
        if (button.send_event == False) {
            XUngrabPointer(app->display(), CurrentTime);
            XSendEvent(app->display(),
                       fRootProxy->handle(),
                       False,
                       SubstructureNotifyMask,
                       (XEvent *) &button);
        }
        return ;
    }
    YFrameWindow *frame = 0;
    if (useMouseWheel && ((frame = focus()) != 0) && button.type == ButtonPress &&
        ((KEY_MODMASK(button.state) == app->WinMask && app->WinMask) ||
         (KEY_MODMASK(button.state) == ControlMask + app->AltMask && app->AltMask)))
    {
        if (button.button == 4)
            frame->wmNextWindow();
        else if (button.button == 5)
            frame->wmPrevWindow();
    }
    if (button.type == ButtonPress) do {
#ifndef NO_CONFIGURE_MENUS
        if (button.button + 10 == (unsigned) rootMenuButton) {
            if (rootMenu)
                rootMenu->popup(0, 0, button.x, button.y, -1, -1,
                                YPopupWindow::pfCanFlipVertical |
                                YPopupWindow::pfCanFlipHorizontal |
                                YPopupWindow::pfPopupMenu |
                                YPopupWindow::pfButtonDown);
            break;
        }
#endif
#ifdef CONFIG_WINMENU
        if (button.button + 10 == (unsigned) rootWinMenuButton) {
            popupWindowListMenu(button.x, button.y);
            break;
        }
#endif
#if 0
#ifdef CONFIG_WINLIST
        if (button.button + 10 == rootWinListButton) {
            if (windowList)
                windowList->showFocused(button.x_root, button.y_root);
            break;
        }
#endif
#endif
    } while (0);
    YWindow::handleButton(button);
}

void YWindowManager::handleClick(const XButtonEvent &up, int count) {
    if (count == 1) do {
#ifndef NO_CONFIGURE_MENUS
        if (up.button == (unsigned) rootMenuButton) {
            if (rootMenu)
                rootMenu->popup(0, 0, up.x, up.y, -1, -1,
                                YPopupWindow::pfCanFlipVertical |
                                YPopupWindow::pfCanFlipHorizontal |
                                YPopupWindow::pfPopupMenu);
            break;
        }
#endif
#ifdef CONFIG_WINMENU
        if (up.button == (unsigned) rootWinMenuButton) {
            popupWindowListMenu(up.x, up.y);
            break;
        }
#endif
#ifdef CONFIG_WINLIST
        if (up.button == (unsigned) rootWinListButton) {
            if (windowList)
                windowList->showFocused(up.x_root, up.y_root);
            break;
        }
#endif
    } while (0);
}

void YWindowManager::handleConfigureRequest(const XConfigureRequestEvent &configureRequest) {
    YFrameWindow *frame = findFrame(configureRequest.window);

    if (frame) {
        MSG(("root configure request -- frame"));
        frame->configureClient(configureRequest);
    } else {
        MSG(("root configure request -- client"));

        XWindowChanges xwc;
        xwc.x = configureRequest.x;
        xwc.y = configureRequest.y;
        xwc.width = configureRequest.width;
        xwc.height = configureRequest.height;
        xwc.border_width = configureRequest.border_width;
        xwc.stack_mode = configureRequest.detail;
        xwc.sibling = configureRequest.above;
        XConfigureWindow(app->display(), configureRequest.window,
                         configureRequest.value_mask, &xwc);
#if 0
        {
            Window rootw;
            unsigned int bw, depth;
            int fX, fY;
            unsigned int fWidth, fHeight;

            XGetGeometry(app->display(), configureRequest.window, &rootw,
                         &fX, &fY, &fWidth, &fHeight, &bw, &depth);
            /*fX = attributes.x;
             fY = attributes.y;
             fWidth = attributes.width;
             fHeight = attributes.height;*/

            MSG(("changed geometry (%d:%d %dx%d)", fX, fY, fWidth, fHeight));
        }
#endif
    }
}

void YWindowManager::handleMapRequest(const XMapRequestEvent &mapRequest) {
    mapClient(mapRequest.window);
}

void YWindowManager::handleUnmap(const XUnmapEvent &unmap) {
    if (unmap.send_event)
        unmanageClient(unmap.window, false);
}

void YWindowManager::handleDestroyWindow(const XDestroyWindowEvent &destroyWindow) {
    if (destroyWindow.window == handle())
        YWindow::handleDestroyWindow(destroyWindow);
}

void YWindowManager::handleClientMessage(const XClientMessageEvent &message) {
#ifdef CONFIG_WMSPEC_HINTS
    if (message.message_type == atoms.netCurrentDesktop)
        winWorkspace(message.data.l[0]);
#endif
#ifdef CONFIG_GNOME_HINTS
    if (message.message_type == atoms.winWorkspace)
        winWorkspace(message.data.l[0]);
#endif
}

Window YWindowManager::findWindow(char const * resource) {
    char *wmInstance(0), *wmClass(0);

    char const * dot(resource ? strchr(resource, '.') : 0);

    if (dot) {
	wmInstance = (dot != resource ? newstr(resource, dot - resource) : 0);
	wmClass = newstr(dot + 1);
    } else if (resource)
	wmInstance = newstr(resource);

    Window win = findWindow(desktop->handle(), wmInstance, wmClass);

    delete[] wmClass;
    delete[] wmInstance;

    return win;
}

Window YWindowManager::findWindow(Window root, char const * wmInstance,
				  char const * wmClass) {
    Window firstMatch = None;
    Window parent, *clients;
    unsigned nClients;

    XQueryTree(app->display(), root, &root, &parent, &clients, &nClients);

    if (clients) {
	unsigned n;

	for (n = 0; !firstMatch && n < nClients; ++n) {
	    XClassHint wmclass;

	    if (XGetClassHint(app->display(), clients[n], &wmclass)) {
		if ((wmInstance == NULL ||
		    strcmp(wmInstance, wmclass.res_name) == 0) &&
		    (wmClass == NULL ||
		    strcmp(wmClass, wmclass.res_class) == 0))
		    firstMatch = clients[n];

		XFree(wmclass.res_name);
		XFree(wmclass.res_class);
	    }

	    if (!firstMatch)
		firstMatch = findWindow(clients[n], wmInstance, wmClass);
	}

	XFree(clients);
    }

    return firstMatch;
}

YFrameWindow *YWindowManager::findFrame(Window win) {
    YFrameWindow *frame;
    return (Success == XFindContext(app->display(), win, frameContext,
            (XPointer*)&frame) ? frame : NULL);
}

YFrameClient *YWindowManager::findClient(Window win) {
    YFrameClient *client;
    return (Success == XFindContext(app->display(), win, clientContext,
            (XPointer*)&client) ? client : NULL);
}

#if CONFIG_KDE_TRAY_WINDOWS
YTrayWindow *YWindowManager::findTrayWindow(Window win) {
    YTrayWindow *tray;
    return (Success == XFindContext(app->display(), win, trayContext,
            (XPointer*)&tray) ? tray : NULL);
}
#endif

#ifndef LITE
void YWindowManager::focus(YFrameWindow *f, bool canWarp) {
#else
void YWindowManager::focus(YFrameWindow *f, bool /*canWarp*/) {
#endif
    YFrameClient *c = f ? f->client() : 0;
    Window w = desktop->handle();

    MSG(("SET FOCUS f=%lX", f));

    if (f == 0) {
        YFrameWindow *ff = focus();
        if (ff) switchFocusFrom(ff);
    }

    bool input = true;
    XWMHints *hints = c ? c->hints() : 0;

    if (hints && (hints->flags & InputHint) && !hints->input)
        input = false;

    if (f && f->visible()) {
        if (c && c->visible() && !(f->isRollup() || f->isIconic()))
            w = c->handle();
        else
            w = f->handle();

        if (input)
            switchFocusTo(f);
    }
#if 0
    if (w == desktop->handle()) {
        msg("%lX Focus 0x%lX desktop", app->eventTime(), w);
    } else if (f && w == f->handle()) {
        msg("%lX Focus 0x%lX frame %s", app->eventTime(), w, f->title());
    } else if (f && c && w == c->handle()) {
        msg("%lX Focus 0x%lX client %s", app->eventTime(), w, f->title());
    } else {
        msg("%lX Focus 0x%lX",
            app->eventTime(), w);
    }
#endif

    if (input) {
        XSetInputFocus(app->display(), w, RevertToNone, CurrentTime);
    }

    if (c && w == c->handle() && c->protocols() & wm::TakeFocus) {
        c->sendMessage(atoms.wmTakeFocus);
    }

    if (!pointerColormap)
        colormapWindow(f);

#ifndef LITE
    /// !!! /* warp pointer sucks */
    if (f && canWarp && !clickFocus && warpPointer && phase == phaseRunning)
        XWarpPointer(app->display(), None, handle(), 0, 0, 0, 0,
                     f->x() + f->borderX(), f->y() + f->borderY() + f->titleY());

#endif

#ifdef CONFIG_WMSPEC_HINTS
    Window focus(fFocusWin && fFocusWin->client() ?
                 fFocusWin->client()->handle() : None);

    XChangeProperty(app->display(), handle(),
                    atoms.netActiveWindow, XA_WINDOW, 32,
                    PropModeReplace, (unsigned char *)&focus, 1);
#endif


    MSG(("SET FOCUS END"));
}

void YWindowManager::loseFocus(YFrameWindow *window) {
    PRECONDITION(window != 0);
#ifdef DEBUG
    if (debug_z) dumpZorder("losing focus: ", window);
#endif
    YFrameWindow *w = window->findWindow(YFrameWindow::fwfNext |
                                         YFrameWindow::fwfVisible |
                                         YFrameWindow::fwfCycle |
                                         YFrameWindow::fwfFocusable |
                                         YFrameWindow::fwfLayers |
                                         YFrameWindow::fwfWorkspace);

    PRECONDITION(w != window);
    focus(w, false);
}

void YWindowManager::loseFocus(YFrameWindow *window,
                               YFrameWindow *next,
                               YFrameWindow *prev)
{
    PRECONDITION(window != 0);
#ifdef DEBUG
    if (debug_z) dumpZorder("close: losing focus: ", window);
#endif

    YFrameWindow *w = 0;

    if (next)
        w = next->findWindow(YFrameWindow::fwfVisible |
                             YFrameWindow::fwfCycle |
                             YFrameWindow::fwfFocusable |
                             YFrameWindow::fwfLayers |
                             YFrameWindow::fwfWorkspace);
    else if (prev)
        w = prev->findWindow(YFrameWindow::fwfNext |
                             YFrameWindow::fwfVisible |
                             YFrameWindow::fwfCycle |
                             YFrameWindow::fwfFocusable |
                             YFrameWindow::fwfLayers |
                             YFrameWindow::fwfWorkspace);

    if (w == window)
        w = 0;
    //msg("loseFocus to %s", w ? w->title() : "<none>");
    focus(w, false);
}

void YWindowManager::activate(YFrameWindow *window, bool canWarp) {
    if (window) {
        window->wmRaise();
        window->activate(canWarp);
    }
}

void YWindowManager::top(icewm::Layer layer, YFrameWindow *top) {
    if (true || !clientMouseActions) // some programs are buggy
        if (fTop[layer]) {
            if (raiseOnClickClient)
                fTop[layer]->container()->grabButtons();
        }
    fTop[layer] = top;
    if (true || !clientMouseActions) // some programs are buggy
        if (fTop[layer]) {
            if (raiseOnClickClient &&
                !(focusOnClickClient && !fTop[layer]->focused()))
                fTop[layer]->container()->releaseButtons();
        }
}

void YWindowManager::updateClientList() {
#if CONFIG_GNOME_OR_WMSPEC_HINTS
    int count(0);

    for (YFrameWindow *f(bottomLayer()); f; f = f->prevLayer())
        if (f->client() && f->client()->adopted())
            ++count;

    XID *ids(new XID[count]);

    if (NULL != ids) {
        XID *id(ids);

        for (YFrameWindow *f(bottomLayer()); f; f = f->prevLayer())
            if (f->client() && f->client()->adopted())
                *id++ = f->client()->handle();

        PRECONDITION((id - ids) == count);
    }

#ifdef CONFIG_GNOME_HINTS
    XChangeProperty(app->display(), desktop->handle(),
                    atoms.winClientList, XA_CARDINAL, 32,
                    PropModeReplace, (unsigned char *)ids, count);
#endif

#ifdef CONFIG_WMSPEC_HINTS
    XChangeProperty(app->display(), desktop->handle(),
                    atoms.netClientListStacking, XA_WINDOW, 32,
                    PropModeReplace, (unsigned char *)ids, count);

    if (NULL != ids) {
        XID *id(ids);

        for (YFrameWindow *f(firstCreated()); f; f = f->nextCreated())
            if (f->client() && f->client()->adopted())
                *id++ = f->client()->handle();

        PRECONDITION((id - ids) == count);
    }

    XChangeProperty(app->display(), desktop->handle(),
                    atoms.netClientList, XA_WINDOW, 32,
                    PropModeReplace, (unsigned char *)ids, count);
#endif

    delete[] ids;
#endif

    checkLogout();
}

#if CONFIG_KDE_TRAY_WINDOWS

void YWindowManager::addTrayWindow(YTrayWindow *window) {
msg(__PRETTY_FUNCTION__);
    fTrayWindows.append(window);
    updateTrayList();
}

void YWindowManager::removeTrayWindow(YTrayWindow *window) {
msg(__PRETTY_FUNCTION__);
    fTrayWindows.remove(window);
    updateTrayList();
}

void YWindowManager::updateTrayList() {
    unsigned const count(fTrayWindows.count());
    
    XID *ids(new XID[count]);
#warning hrm
/*
    for (TrayWindowList::CountingIterator
         window(fTrayWindows.head()); window; ++window)
        ids[window.count()] = (*window)->client()->handle();
*/
    XChangeProperty(app->display(), desktop->handle(),
                    atoms.kdeNetSystemTrayWindows, XA_WINDOW, 32,
                    PropModeReplace, (unsigned char *)ids, count);

    delete[] ids;
}

#endif // CONFIG_KDE_TRAY_WINDOWS

void YWindowManager::installColormap(Colormap cmap) {
    if (app->hasColormap()) {
        //MSG(("installing colormap 0x%lX", cmap));
        if (app->grabWindow() == 0) {
            if (cmap == None) {
                XInstallColormap(app->display(), app->colormap());
            } else {
                XInstallColormap(app->display(), cmap);
            }
        }
    }
}

void YWindowManager::colormapWindow(YFrameWindow *frame) {
    if (fColormapWindow != frame) {
        fColormapWindow = frame;

        installColormap(colormapWindow() ?
                        colormapWindow()->client()->colormap() : None);
    }
}

void YWindowManager::manageClients() {
    unsigned int clientCount;
    Window winRoot, winParent, *winClients;

    {
        YSynchronServerLock __lock__;

        XQueryTree(app->display(), handle(),
                   &winRoot, &winParent, &winClients, &clientCount);

        if (winClients) {
            for (unsigned i(0); i < clientCount; ++i)
                if (NULL == findClient(winClients[i]))
                    manageClient(winClients[i]);

            XFree(winClients);
        }
    }

    updateWorkArea();
    phase = phaseRunning;
    if (clickFocus || !strongPointerFocus) focusTopWindow();
}

void YWindowManager::unmanageClients() {
    focus(NULL);

    YServerLock __lock__;

#if CONFIG_KDE_TRAY_WINDOWS
#warning hrm
/*
    for (TrayWindowList::Iterator ti(fTrayWindows.head()); ti; ) {
        YTrayWindow * traywin(*ti); ++ti;
msg("%p", traywin->client()->handle()); 
delete traywin;       
//            unmanageClient(traywin.client()->handle(), true);
    }
*/
#endif    

    for (unsigned layer(0); layer < WinLayerCount; ++layer)
        while (bottom(layer))
            unmanageClient(bottom(layer)->client()->handle(), true);
}

int intersection(int s1, int e1, int s2, int e2) {
    int s, e;

    if (s1 > e2)
        return 0;
    if (s2 > e1)
        return 0;

    /* start */
    if (s2 > s1)
        s = s2;
    else
        s = s1;

    /* end */
    if (e1 < e2)
        e = e1;
    else
        e = e2;
    if (e > s)
        return e - s;
    else
        return 0;
}

int addco(int *v, int &n, int c) {
    int l, r, m;

    /* find */
    l = 0;
    r = n;
    while (l < r) {
        m = (l + r) / 2;
        if (v[m] == c)
            return 0;
        else if (v[m] > c)
            r = m;
        else
            l = m + 1;
    }
    /* insert if not found */
    memmove(v + l + 1, v + l, (n - l) * sizeof(int));
    v[l] = c;
    n++;
    return 1;
}

int YWindowManager::calcCoverage(bool down, YFrameWindow *frame, int x, int y, int w, int h) {
    int cover = 0;
    int factor = down ? 2 : 1; // try harder not to cover top windows

    for (YFrameWindow * f = frame; f ; f = (down ? f->next() : f->prev())) {
        if (f == frame || f->isMinimized() || f->isHidden() || !f->isManaged())
            continue;

        if (!f->isSticky() && f->workspace() != frame->workspace())
            continue;

        cover +=
            intersection(f->x(), f->x() + f->width(), x, x + w) *
            intersection(f->y(), f->y() + f->height(), y, y + h) * factor;

        if (factor > 1)
            factor /= 2;
    }
    //msg("coverage %d %d %d %d = %d", x, y, w, h, cover);
    return cover;
}

void YWindowManager::tryCover(bool down, YFrameWindow *frame, int x, int y, int w, int h,
                              int &px, int &py, int &cover)
{
    int ncover;


    if (x < fMinX)
        return ;
    if (y < fMinY)
        return ;
    if (x + w > fMaxX)
        return ;
    if (y + h > fMaxY)
        return ;

    ncover = calcCoverage(down, frame, x, y, w, h);
    if (ncover < cover) {
        //msg("min: %d %d %d", ncover, x, y);
        px = x;
        py = y;
        cover = ncover;
    }
}

bool YWindowManager::smartPlace(bool down, YFrameWindow *frame, int &x, int &y, int w, int h) {
    x = fMinX;
    y = fMinY;
    int cover, px, py;
    int *xcoord, *ycoord;
    int xcount, ycount;
    int n = 0;
    YFrameWindow *f = 0;

    for (f = frame; f; f = (down ? f->next() : f->prev()))
        n++;
    n = (n + 1) * 2;
    xcoord = new int[n];
    if (xcoord == 0)
        return false;
    ycoord = new int[n];
    if (ycoord == 0)
        return false;

    xcount = ycount = 0;
    addco(xcoord, xcount, fMinX);
    addco(ycoord, ycount, fMinY);
    for (f = frame; f; f = (down ? f->next() : f->prev())) {
        if (f == frame || f->isMinimized() || f->isHidden() || !f->isManaged() || f->isMaximized())
            continue;

        if (!f->isSticky() && f->workspace() != frame->workspace())
            continue;

        addco(xcoord, xcount, f->x());
        addco(xcoord, xcount, f->x() + f->width());
        addco(ycoord, ycount, f->y());
        addco(ycoord, ycount, f->y() + f->height());
    }
    addco(xcoord, xcount, fMaxX);
    addco(ycoord, ycount, fMaxY);
    assert(xcount <= n);
    assert(ycount <= n);

    int xn = 0, yn = 0;
    px = x; py = y;
    cover = calcCoverage(down, frame, x, y, w, h);
    while (1) {
        x = xcoord[xn];
        y = ycoord[yn];

        tryCover(down, frame, x - w, y - h, w, h, px, py, cover);
        tryCover(down, frame, x - w, y    , w, h, px, py, cover);
        tryCover(down, frame, x    , y - h, w, h, px, py, cover);
        tryCover(down, frame, x    , y    , w, h, px, py, cover);

        if (cover == 0)
            break;

        xn++;;
        if (xn >= xcount) {
            xn = 0;
            yn++;
            if (yn >= ycount)
                break;
        }
    }
    x = px;
    y = py;
    delete [] xcoord;
    delete [] ycoord;
    return true;
}

void YWindowManager::smartPlace(YFrameWindow **w, int count) {
    saveArrange(w, count);

    if (count == 0)
        return ;

    for (int i = 0; i < count; i++) {
        YFrameWindow *f = w[i];
        int x = f->x();
        int y = f->y();
        if (smartPlace(false, f, x, y, f->width(), f->height()))
            f->position(x, y);
    }
}

void YWindowManager::cascadePlace(YFrameWindow *frame, int &lastX, int &lastY, int &x, int &y, int w, int h) {
    /// !!! make auto placement cleaner and (optionally) smarter
    if (lastX < minX(frame)) lastX = minX(frame);
    if (lastY < minY(frame)) lastY = minY(frame);

    x = lastX;
    y = lastY;

    lastX += wsTitleBar;
    lastY += wsTitleBar;
    if (int(y + h) >= maxY(frame)) {
        y = minY(frame);
        lastY = wsTitleBar;
    }
    if (int(x + w) >= maxX(frame)) {
        x = minX(frame);
        lastX = wsTitleBar;
    }
}

void YWindowManager::cascadePlace(YFrameWindow **w, int count) {
    saveArrange(w, count);

    if (count == 0)
        return ;

    int lx = fMinX;
    int ly = fMinY;
    for (int i = count; i > 0; i--) {
        YFrameWindow *f = w[i - 1];
        int x;
        int y;

        cascadePlace(f, lx, ly, x, y, f->width(), f->height());
        f->position(x, y);
    }
}

void YWindowManager::windows(YFrameWindow **w, int count, YAction *action) {
    saveArrange(w, count);

    if (count == 0)
        return ;

    for (int i = count; i > 0; i--) {
        YFrameWindow *f = w[i - 1];
        if (action == actionHideAll) {
            if (!f->isHidden())
                f->wmHide();
        } else if (action == actionMinimizeAll) {
            if (!f->isMinimized())
                f->wmMinimize();
        }
    }
}

void YWindowManager::newPosition(YFrameWindow *frame, int &x, int &y, int w, int h) {
    if (centerTransientsOnOwner && frame->owner() != 0) {
        x = frame->owner()->x() + frame->owner()->width() / 2 - w / 2;
        y = frame->owner()->y() + frame->owner()->width() / 2 - h / 2;
    } else if (smartPlacement) {
        smartPlace(true, frame, x, y, w, h);
    } else {

        static int lastX = 0;
        static int lastY = 0;

        cascadePlace(frame, lastX, lastY, x, y, w, h);
    }
#if 0  // !!!
    if (frame->dontCover()) {
        if (frame->state() & WinStateDockHorizontal) y = 0;
        else x = 0;
    }
#endif
}

void YWindowManager::placeWindow(YFrameWindow *frame, int x, int y,
				 bool newClient, bool &
#ifdef CONFIG_SESSION
canActivate
#endif
) {
    YFrameClient *client = frame->client();

    int posWidth = client->width() + 2 * frame->borderX();
    int posHeight = client->height() + 2 * frame->borderY() + frame->titleY();

#ifdef CONFIG_SESSION
    if (app->haveSessionManager() && findWindowInfo(frame)) {
        if (frame->workspace() != activeWorkspace()) canActivate = false;
        return;
    } else
#endif

#ifndef NO_WINDOW_OPTIONS
    if (newClient) {
        WindowOption wo;
        frame->windowOptions(wo, true);

        //msg("positioning %d %d %d %d %d", wo.gx, wo.gy, wo.gw, wo.gh, wo.gflags);
        if (wo.gh != 0 && wo.gw != 0) {
            if (wo.gflags & (WidthValue | HeightValue))
                frame->size(wo.gw, wo.gh);
        }
        if (wo.gflags & (XValue | YValue)) {
            frame->position(wo.gx, wo.gy);
            return ;
        }
    }
#endif

    if (newClient && client->adopted() && client->sizeHints() &&
        (!(client->sizeHints()->flags & (USPosition | PPosition)) ||
         ((client->sizeHints()->flags & PPosition) &&
          frame->frameOptions() & YFrameWindow::foIgnorePosition))) {
        newPosition(frame, x, y, posWidth, posHeight);
        newClient = false;
    }

    int posX = x;
    int posY = y;

    if (1) {
        int gx, gy;

        client->gravityOffsets(gx, gy);

        if (gx > 0)
            posX -= 2 * frame->borderX() - 1 - client->border();
        if (gy > 0)
            posY -= 2 * frame->borderY() +
                        frame->titleY() - 1 - client->border();
    }

    MSG(("mapping geometry (%d:%d %dx%d)", posX, posY, posWidth, posHeight));
    frame->geometry(posX, posY, posWidth, posHeight);
}

YFrameWindow *YWindowManager::manageClient(Window win, bool mapClient) {
    MSG(("managing window 0x%lX", win));
    PRECONDITION(NULL == findFrame(win));

#if 1
    YServerLock __lock__;
#else
    YSynchronServerLock __lock__;
    {
        XEvent xev;
        if (XCheckTypedWindowEvent(app->display(), win, DestroyNotify, &xev))
            return NULL;
    }
#endif

    YFrameClient *client(findClient(win));

    if (NULL == client) {
        XWindowAttributes attributes;

        if (!XGetWindowAttributes(app->display(), win, &attributes))
            return NULL;

        if (attributes.override_redirect)
            return NULL;

        // !!! is this correct ???
        if (!mapClient && attributes.map_state == IsUnmapped)
            return NULL;

        if (NULL == (client = new YFrameClient(NULL, win)))
            return NULL;

        client->border(attributes.border_width);
        client->colormap(attributes.colormap);
    }

#ifdef CONFIG_WM_SESSION
    topLevelProcess(client->pid());
#endif

#if CONFIG_KDE_TRAY_WINDOWS
msg("tray window for: %p", client->trayWindowFor());
    if (None != client->trayWindowFor()) {
        manageTrayWindow(client);
        return NULL;
    }
#endif

#ifdef CONFIG_DOCK
    if (client->isDockApp()) {
        manageDockApp(client);
        return NULL;
    }
#endif

    return manageFrame(client, mapClient);
}

YFrameWindow *YWindowManager::manageFrame(YFrameClient *client,
                                          bool mapClient) {
    int const cx(client->x()), cy(client->y());

    MSG(("initial geometry (%d:%d %dx%d)",
         cx, cy, client->width(), client->height()));

    YFrameWindow *frame(new YFrameWindow(0, client));

    if (NULL == frame) {
        delete client;
        return NULL;
    }

    if (client->visible() && phase == phaseStartup)
        mapClient = true;

    bool canActivate(true);
    placeWindow(frame, cx, cy, phase != phaseStartup, canActivate);

#ifdef CONFIG_SHAPE
    frame->setShape();
#endif

    MSG(("Map - Frame: %d", frame->visible()));
    MSG(("Map - Client: %d", client->visible()));

    gnome::Layer layer;
    if (client->updateWinLayer(layer)) frame->layer(layer);

    gnome::State stateMask, state;
    if (client->updateWinState(stateMask, state)) {
        frame->state(stateMask, state);
    } else {
        wm::State state(client->wmState());

        if (WithdrawnState == state) {
            XWMHints const *wmHints(frame->client()->hints());
            state = wmHints && (wmHints->flags & StateHint)
                  ? wmHints->initial_state : NormalState;
        }

        MSG(("FRAME state = %d", state));

        switch (state) {
            case IconicState:
                frame->state(WinStateMinimized, WinStateMinimized);
                break;

            case NormalState:
            case WithdrawnState:
                break;
        }
    }

    gnome::Workspace workspace(0);
    if (frame->client()->updateWinWorkspace(workspace))
        frame->workspace(workspace);

#ifdef CONFIG_TRAY
    icewm::TrayOption trayoption(0);
    if (frame->client()->updateIcewmTrayOption(trayoption))
        frame->trayOption(trayoption);
#endif

    if ((limitSize || limitPosition) &&
        (phase != phaseStartup) &&
	!frame->dontCover()) {
        int posX(frame->x() + frame->borderX()),
	    posY(frame->y() + frame->borderY()),
	    posWidth(frame->width() - 2 * frame->borderX()),
	    posHeight(frame->height() - 2 * frame->borderY());

        if (limitSize) {
            posWidth = min(posWidth, maxWidth(frame));
            posHeight = min(posHeight, maxHeight(frame));

            posHeight -= frame->titleY();
            frame->client()->constrainSize(posWidth, posHeight,
	    				   frame->layer(), 0);
            posHeight += frame->titleY();
        }

        if (limitPosition &&
            !(client->sizeHints() &&
	     (client->sizeHints()->flags & USPosition))) {
            posX = clamp(posX, minX(frame),
	    		       maxX(frame) - posWidth);
            posY = clamp(posY, minY(frame),
	    		       maxY(frame) - posHeight);
        }

        posX -= frame->borderX();
        posY -= frame->borderY();
        posWidth += 2 * frame->borderX();
        posHeight += 2 * frame->borderY();
        frame->geometry(posX, posY, posWidth, posHeight);
    }

    if (!mapClient) {
        /// !!! fix (new internal state)
        frame->state(WinStateHidden, WinStateHidden);
    }
    frame->managed();

    bool const canManualPlace
        (canActivate && manualPlacement && phase == phaseRunning &&
#ifdef CONFIG_WINLIST
         client != windowList &&
#endif
         NULL == frame->owner() &&
        (NULL == client->sizeHints() ||
               !(client->sizeHints()->flags & (USPosition | PPosition))));

    if (mapClient && (!frame->state() || frame->isRollup()) &&
        canManualPlace && !opaqueMove)
        frame->manualPlace();

    frame->updateState();
    frame->updateProperties();
#ifdef CONFIG_TASKBAR
    frame->updateTaskBar();
#endif
    frame->updateNormalSize();
    frame->updateLayout();

    if (frame->dontCover()) updateWorkArea();

    if (mapClient && (!frame->state() || frame->isRollup())) {
        if (phase == phaseRunning && canActivate)
            frame->focusOnMap();
        if (canManualPlace && opaqueMove)
            frame->wmMove();
    }

    return frame;
}

#if CONFIG_KDE_TRAY_WINDOWS

void YWindowManager::manageTrayWindow(YFrameClient *client) {
#if 0
msg(__PRETTY_FUNCTION__);

msg("win:%p", client->trayWindowFor());
    YFrameWindow *frame(findFrame(client->trayWindowFor()));
msg("mw:%p", frame);
if (frame) { msg("tell the frame that it has a tray window..."); }

    YTrayWindow *trayWindow =
        new YTrayWindow(taskBar->trayPane(), frame, client);
msg("client:%p trayWindow:%p %p", client, trayWindow, (YClientPeer*) trayWindow);
    taskBar->trayPane()->add(trayWindow);
    trayWindow->shown(true);
    client->reparent(trayWindow, 0, 0);
    client->trayWindow(trayWindow);
    client->show();
    taskBar->trayPane()->relayout();

        TrayPane *tp = taskBar->trayPane();
	int const nw(tp->requiredWidth());
        int dw;

        if ((dw = nw - tp->width()))
            taskBar->trayPane()->geometry
		(tp->x() - dw, tp->y(), nw, tp->height());
/*
	if (dw) taskBar->taskPane()->size
	    (taskBar->taskPane()->width() - dw, taskBar->taskPane()->height());

        taskBar->taskPane()->relayout();
*/
//    taskBar->updateProxyPanes();

    if (dw && NULL == taskBar->taskPane() && NULL != taskBar->addressBar())
	taskBar->addressBar()->size
	    (taskBar->addressBar()->width() - dw,
	     taskBar->addressBar()->height());
#endif
    warn("Sorry, Tray windows are not supported yet (window 0x%x)",
         client->handle());
}
#endif

#ifdef CONFIG_DOCK
void YWindowManager::manageDockApp(YFrameClient *client) {
    warn("Sorry, DockApps are not supported yet (window 0x%x)",
         client->handle());
}
#endif

YFrameWindow *YWindowManager::mapClient(Window win) {
    YFrameWindow *frame = findFrame(win);

    MSG(("mapping window 0x%lX", win));
    if (frame == 0)
        return manageClient(win, true);
    else {
        frame->state(WinStateMinimized | WinStateHidden, 0);
        if (clickFocus || !strongPointerFocus)
            frame->activate(true);/// !!! is this ok
    }

    return frame;
}

void YWindowManager::unmanageClient(Window win, bool mapClient,
				    bool reparent) {
    YFrameWindow *frame = findFrame(win);

    MSG(("unmanaging window 0x%lX", win));

    if (frame) {
        YFrameClient *client = frame->client();

        // !!! cleanup
        client->hide();

        frame->hide();
        frame->unmanage(reparent);
        delete frame;

        if (mapClient)
            client->show();
        delete client;
    } else {
        MSG(("unmanage: unknown window: 0x%lX", win));
    }
}

void YWindowManager::destroyedClient(Window win) {
    YWindow *window;

    if (NULL != (window = findTrayWindow(win))) delete window;
    else if (NULL != (window = findFrame(win))) delete window;
    else MSG(("destroyed: unknown window: 0x%lX", win));
}

void YWindowManager::focusTopWindow() {
    if (phase != phaseRunning)
        return ;
    if (!focusTop(topLayer(WinLayerNormal)))
        focusTop(topLayer());
}

bool YWindowManager::focusTop(YFrameWindow *f) {
    if (!f)
        return false;

    f = f->findWindow(YFrameWindow::fwfVisible |
                      YFrameWindow::fwfFocusable |
                      YFrameWindow::fwfWorkspace |
                      YFrameWindow::fwfSame |
                      YFrameWindow::fwfLayers |
                      YFrameWindow::fwfCycle);
    //msg("found focus %lX", f);
    if (!f) {
        focus(NULL);
        return false;
    }
    focus(f);
    return true;
}

YFrameWindow *YWindowManager::topLayer(icewm::Layer layer) {
    for (icewm::Layer l(layer + 1); l-- > 0; )
        if (fTop[l]) return fTop[l];

    return NULL;
}

YFrameWindow *YWindowManager::bottomLayer(icewm::Layer layer) {
    for (icewm::Layer l(layer); l < WinLayerCount; ++l)
        if (fBottom[l]) return fBottom[l];

    return NULL;
}

void YWindowManager::restackWindows(YFrameWindow *win) {
    unsigned count(0);
    YFrameWindow *f;
    YPopupWindow *p;

    for (f = win; f; f = f->prev())
        //if (f->visibleNow())
            count++;

    for (icewm::Layer ll(win->layer() + 1); ll < WinLayerCount; ++ll) {
        f = bottom(ll);
        for (; f; f = f->prev())
            //if (f->visibleNow())
                count++;
    }

#ifndef LITE
    if (statusMoveSize && statusMoveSize->visible())
        count++;
#endif

    p = app->popup();
    while (p) {
        count++;
        p = p->prevPopup();
    }
#ifndef LITE
    if (ctrlAltDelete && ctrlAltDelete->visible())
        count++;
#endif

    if (fLeftSwitch && fLeftSwitch->visible())
        count++;

    if (fRightSwitch && fRightSwitch->visible())
        count++;

    if (fTopSwitch && fTopSwitch->visible())
        count++;

    if (fBottomSwitch && fBottomSwitch->visible())
        count++;

    if (count == 0)
        return ;

    count++;

    Window *w = new Window[count];
    if (w == 0)
        return ;

    unsigned i(0);

    w[i++] = fTopWin->handle();

    p = app->popup();
    while (p) {
        w[i++] = p->handle();
        p = p->prevPopup();
    }

    if (fLeftSwitch && fLeftSwitch->visible())
        w[i++] = fLeftSwitch->handle();

    if (fRightSwitch && fRightSwitch->visible())
        w[i++] = fRightSwitch->handle();

    if (fTopSwitch && fTopSwitch->visible())
        w[i++] = fTopSwitch->handle();

    if (fBottomSwitch && fBottomSwitch->visible())
        w[i++] = fBottomSwitch->handle();

#ifndef LITE
    if (ctrlAltDelete && ctrlAltDelete->visible())
        w[i++] = ctrlAltDelete->handle();
#endif

#ifndef LITE
    if (statusMoveSize->visible())
        w[i++] = statusMoveSize->handle();
#endif

    for (icewm::Layer ll(WinLayerCount - 1); ll > win->layer(); --ll) {
        for (f = top(ll); f; f = f->next())
            //if (f->visibleNow())
                w[i++] = f->handle();
    }
    for (f = top(win->layer()); f; f = f->next()) {
        //if (f->visibleNow())
            w[i++] = f->handle();
        if (f == win)
            break;
    }

    if (count > 0) {
#if 0
        /* remove this code if ok !!! must determine correct top window */
#if 1
        XRaiseWindow(app->display(), w[0]);
#else
        if (win->next()) {
            XWindowChanges xwc;

            xwc.sibling = win->next()->handle();
            xwc.stack_mode = Above;
            XConfigureWindow(app->display(), w[0], CWSibling | CWStackMode, &xwc);
        }
#endif
        if (count > 1)
#endif
        XRestackWindows(app->display(), w, count);
    }
    if (i != count) {
        MSG(("i=%d, count=%d", i, count));
    }

    PRECONDITION(i == count);
    updateClientList();

    delete w;
}

int YWindowManager::minX(icewm::Layer layer) const {
    return layer < WinLayerDock ? fMinX : 0;
}

int YWindowManager::minY(icewm::Layer layer) const {
    return layer < WinLayerDock ? fMinY : 0;
}

int YWindowManager::maxX(icewm::Layer layer) const {
    return layer < WinLayerDock ? fMaxX : width();
}

int YWindowManager::maxY(icewm::Layer layer) const {
    return layer < WinLayerDock ? fMaxY : height();
}

int YWindowManager::minX(YFrameWindow const *frame) const {
    return minX(!frame->dontCover() ? frame->layer() : WinLayerInvalid);
}

int YWindowManager::minY(YFrameWindow const *frame) const {
    return minY(!frame->dontCover() ? frame->layer() : WinLayerInvalid);
}

int YWindowManager::maxX(YFrameWindow const *frame) const {
    return maxX(!frame->dontCover() ? frame->layer() : WinLayerInvalid);
}

int YWindowManager::maxY(YFrameWindow const *frame) const {
    return maxY(!frame->dontCover() ? frame->layer() : WinLayerInvalid);
}

void YWindowManager::updateWorkArea() {
    int nMinX(0),
	nMinY(0),
    	nMaxX(width()),
	nMaxY(height()),
	midX((nMinX + nMaxX) / 2),
	midY((nMinY + nMaxY) / 2);

    YFrameWindow * w;

    if (limitByDockLayer)	// -------- find the first dontCover window ---
	w = top(WinLayerDock);
    else
	for (w = topLayer();
	     w && !(w->frameOptions() & YFrameWindow::foDontCover);
	     w = w->nextLayer());

    while(w) {
        // !!! FIX: WORKAREA windows must currently be sticky
        if (!(w->isHidden() ||
              w->isRollup() ||
              w->isIconic() ||
              w->isMinimized() ||
              !w->visibleNow() ||
              !w->isSticky())) {
        // hack
	    int wMinX(nMinX), wMinY(nMinY), wMaxX(nMaxX), wMaxY(nMaxY);
	    bool const isHoriz(w->width() > w->height());

	    if (!isHoriz /*!!!&& !(w->state() & WinStateDockHorizontal)*/) {
		if (w->x() + int(w->width()) < midX)
		    wMinX = w->x() + w->width();
		else if (w->x() > midX)
		    wMaxX = w->x();
	    } else {
		if (w->y() + int(w->height()) < midY)
		    wMinY = w->y() + w->height();
		else if (w->y() > midY)
		    wMaxY = w->y();
	    }

	    nMinX = max(nMinX, wMinX);
	    nMinY = max(nMinY, wMinY);
	    nMaxX = min(nMaxX, wMaxX);
	    nMaxY = min(nMaxY, wMaxY);
	}

	if (limitByDockLayer)	// --------- find the next dontCover window ---
	    w = w->next();
	else
	    do w = w->nextLayer();
	    while (w && !(w->frameOptions() & YFrameWindow::foDontCover));
    }

    if (fMinX != nMinX || fMinY != nMinY || // -- store the new workarea ---
        fMaxX != nMaxX || fMaxY != nMaxY) {
        int const deltaX(nMinX - fMinX);
        int const deltaY(nMinY - fMinY);

        fMinX = nMinX; fMinY = nMinY;
        fMaxX = nMaxX; fMaxY = nMaxY;

        if (fWorkAreaMoveWindows)
            relocateWindows(deltaX, deltaY);

        resizeWindows();
        announceWorkArea();
    }
}

void YWindowManager::announceWorkArea() {
#if CONFIG_GNOME_OR_WMSPEC_HINTS

    yint32 workarea[] = {
        minX(WinLayerNormal), minY(WinLayerNormal),
        maxX(WinLayerNormal), maxY(WinLayerNormal)
    };

#ifdef CONFIG_GNOME_HINTS
    XChangeProperty(app->display(), handle(),
                    atoms.winWorkarea, XA_CARDINAL, 32,
                    PropModeReplace, (unsigned char *)workarea, 4);
#endif

#ifdef CONFIG_WMSPEC_HINTS
    workarea[2] = workarea[2] - workarea[0];
    workarea[3] = workarea[3] - workarea[1];

    XChangeProperty(app->display(), handle(),
                    atoms.netWorkarea, XA_CARDINAL, 32,
                    PropModeReplace, (unsigned char *)workarea, 4);
#endif
#endif
}

void YWindowManager::relocateWindows(int dx, int dy) {
    for (YFrameWindow * f(topLayer(WinLayerDock - 1)); f; f = f->nextLayer())
	if (!f->dontCover()) f->position(f->x() + dx, f->y() + dy);
}

void YWindowManager::resizeWindows() {
    for (YFrameWindow * f(topLayer(WinLayerDock - 1)); f; f = f->nextLayer())
	if (!f->dontCover()) {
	    if (f->isMaximized() || f->canSize())
		f->updateLayout();
#if 0
            if (f->isMaximized())
		f->updateLayout();
#endif
#if 0
	    if (isMaximizedFully())
		f->geometry(fMinX, fMinY, fMaxX - fMinX, fMaxY - fMinY);
	    else if (f->isMaximizedVert())
		f->geometry(f->x(), fMinY, f->width(), fMaxY - fMinY);
	    else if (f->isMaximizedHoriz())
		f->geometry(fMinX, f->y(), fMaxX - fMinX, f->height());
#endif
	}
}

void YWindowManager::activateWorkspace(icewm::Workspace workspace) {
    if (workspace != fActiveWorkspace) {
#ifdef CONFIG_TASKBAR
        if (taskBar && taskBar->workspacesPane() &&
	    fActiveWorkspace != WinWorkspaceInvalid) {
            if (taskBar->workspacesPane()->workspaceButton(fActiveWorkspace))
                taskBar->workspacesPane()->workspaceButton(fActiveWorkspace)->pressed(0);
        }
#endif
        fLastWorkspace = fActiveWorkspace;
        fActiveWorkspace = workspace;
#ifdef CONFIG_TASKBAR
        if (taskBar && taskBar->workspacesPane() &&
            taskBar->workspacesPane()->workspaceButton(fActiveWorkspace))
            taskBar->workspacesPane()->workspaceButton(fActiveWorkspace)->pressed(1);
#endif

        icewm::Workspace ws(fActiveWorkspace);
#ifdef CONFIG_GNOME_HINTS
        XChangeProperty(app->display(), handle(),
                        atoms.winWorkspace, XA_CARDINAL, 32,
                        PropModeReplace, (unsigned char *)&ws, 1);
#endif
#ifdef CONFIG_WMSPEC_HINTS
        XChangeProperty(app->display(), handle(),
                        atoms.netCurrentDesktop, XA_CARDINAL, 32,
                        PropModeReplace, (unsigned char *)&ws, 1);
#endif
        updateWorkArea();

        YFrameWindow *w;

        for (w = bottomLayer(); w; w = w->prevLayer())
            if (!w->visibleNow()) {
                w->updateState();
#ifdef CONFIG_TASKBAR
                w->updateTaskBar();
#endif
            }

        for (w = topLayer(); w; w = w->nextLayer())
            if (w->visibleNow()) {
                w->updateState();
#ifdef CONFIG_TASKBAR
                w->updateTaskBar();
#endif
            }

        if ((clickFocus || !strongPointerFocus)
            /* && (focus() == 0 || !focus()->visibleNow() || !focus()->isFocusable())*/)
        {
            focusTopWindow();
        } else {
            if (strongPointerFocus) {
                XSetInputFocus(app->display(), PointerRoot, RevertToNone, CurrentTime);

            }
        }
        resetColormap(true);

#ifdef CONFIG_TASKBAR
        if (taskBar && taskBar->taskPane())
            taskBar->taskPane()->relayout();
#endif
#ifdef CONFIG_TRAY
        if (taskBar && taskBar->trayPane())
            taskBar->trayPane()->relayout();
#endif
#ifndef LITE
        if (workspaceSwitchStatus && (!showTaskBar || !taskBarShowWorkspaces))
            statusWorkspace->begin(workspace);
#endif
#ifdef CONFIG_GUIEVENTS
        wmapp->signalGuiEvent(geWorkspaceChange);
#endif
    }
}

void YWindowManager::winWorkspace(icewm::Workspace workspace) {
    if (workspace < workspaceCount()) activateWorkspace(workspace);
    else MSG(("invalid workspace switch %ld", workspace));
}

void YWindowManager::wmCloseSession() { // ----------------- shutdow started ---
    for (YFrameWindow * f(topLayer()); f; f = f->nextLayer())
        if (f->client()->adopted()) // not to ourselves?
            f->wmClose();
}

void YWindowManager::iconPosition(YFrameWindow *frame, int *iconX, int *iconY) {
    static int x = 0, y = 0;
    MiniIcon *iw = frame->miniIcon();

    x = max(x, minX(WinLayerDesktop));
    y = max(y, minY(WinLayerDesktop));

    *iconX = x;
    *iconY = y;

    y += iw->height();
    if (y >= maxY(WinLayerDesktop)) {
        x += iw->width();
        y = minX(WinLayerDesktop);
        if (x >= maxX(WinLayerDesktop)) {
            x = 0;
            y = 0;
        }
    }
}

void YWindowManager::resetColormap(bool active) {
    if (active) {
        if (colormapWindow() && colormapWindow()->client())
            installColormap(colormapWindow()->client()->colormap());
    } else {
        installColormap(None);
    }
}

void YWindowManager::handleProperty(const XPropertyEvent &property) {
#ifndef NO_WINDOW_OPTIONS
    if (property.atom == atoms.icewmWinOpt) {
        YWindowProperty icewmWinOpt(handle(), atoms.icewmWinOpt,
                                              atoms.icewmWinOpt, 8192, 0, true);
        if (icewmWinOpt == Success && icewmWinOpt.count()) {
            char *p(icewmWinOpt.template ptr<char>());
            char *e(p + icewmWinOpt.count());

            while (p < e) {
                char *clsin;
                char *option;
                char *arg;

                clsin = p;
                while (p < e && *p) p++;
                if (p == e) break;
                p++;

                option = p;
                while (p < e && *p) p++;
                if (p == e) break;
                p++;

                arg = p;
                while (p < e && *p) p++;
                if (p == e) break;
                p++;

                if (p != e) break;

                hintOptions->winOption(clsin, option, arg);
            }
        }
    }
#endif
}

void YWindowManager::checkLogout() {
    if (fShuttingDown && !haveClients()) {
        if (rebootOrShutdown == 1 && rebootCommand && rebootCommand[0]) {
            msg("reboot... (%s)", rebootCommand);
            system(rebootCommand);
        } else if (rebootOrShutdown == 2 && shutdownCommand && shutdownCommand[0]) {
            msg("shutdown ... (%s)", shutdownCommand);
            system(shutdownCommand);
        }
        app->exit(0);
    }
}

void YWindowManager::removeClientFrame(YFrameWindow *frame) {
    YFrameWindow *p = frame->prev(), *n = frame->next();

    if (fArrangeInfo) {
        for (int i = 0; i < fArrangeCount; i++)
            if (fArrangeInfo[i].frame == frame)
                fArrangeInfo[i].frame = 0;
    }
    if (frame == focus())
        loseFocus(frame, n, p);
    if (frame == focus())
        focus(0);
    if (colormapWindow() == frame)
        colormapWindow(focus());
    if (frame->dontCover())
	updateWorkArea();
}

void YWindowManager::switchFocusTo(YFrameWindow *frame) {
    if (frame != fFocusWin) {
        if (fFocusWin) fFocusWin->loseWinFocus();
        fFocusWin = frame;
        if (fFocusWin) fFocusWin->takeWinFocus();
    }
}

void YWindowManager::switchFocusFrom(YFrameWindow *frame) {
    if (frame == fFocusWin) {
        if (fFocusWin) fFocusWin->loseWinFocus();
        fFocusWin = NULL;
    }
}

#ifdef CONFIG_WINMENU
void YWindowManager::popupWindowListMenu(int x, int y) {
    windowListMenu->popup(0, 0, x, y, -1, -1,
                          YPopupWindow::pfCanFlipVertical |
                          YPopupWindow::pfCanFlipHorizontal |
                          YPopupWindow::pfPopupMenu);
}
#endif

void YWindowManager::popupStartMenu() { // !! fix
#ifdef CONFIG_TASKBAR
    if (showTaskBar && taskBar && taskBarShowStartMenu)
        taskBar->popupStartMenu();
    else
#endif
    {
#ifndef NO_CONFIGURE_MENUS
        rootMenu->popup(0, 0, 0, 0, -1, -1,
                        YPopupWindow::pfCanFlipVertical |
                        YPopupWindow::pfCanFlipHorizontal |
                        YPopupWindow::pfPopupMenu);
#endif
    }
}

#ifdef CONFIG_WINMENU
void YWindowManager::popupWindowListMenu() {
#ifdef CONFIG_TASKBAR
    if (showTaskBar && taskBar && taskBarShowWindowListMenu)
        taskBar->popupWindowListMenu();
    else
#endif
        popupWindowListMenu(0, 0);
}
#endif

void YWindowManager::switchToWorkspace(icewm::Workspace workspace,
                                       bool takeCurrent) {
    if (workspace < workspaceCount()) {
        YFrameWindow *frame(focus());

        if (takeCurrent && frame && !frame->isSticky()) {
            frame->wmOccupyAll();
            frame->wmRaise();
            activateWorkspace(workspace);
            frame->wmOccupyOnlyWorkspace(workspace);
        } else {
            activateWorkspace(workspace);
        }
#ifdef CONFIG_TASKBAR
        if (taskBar) taskBar->popOut();
#endif
    }
}

void YWindowManager::switchToPrevWorkspace(bool takeCurrent) {
    switchToWorkspace((activeWorkspace() + workspaceCount() - 1) %
                       workspaceCount(), takeCurrent);
}

void YWindowManager::switchToNextWorkspace(bool takeCurrent) {
    switchToWorkspace((activeWorkspace() + 1) % workspaceCount(), takeCurrent);
}

void YWindowManager::switchToLastWorkspace(bool takeCurrent) {
    switchToWorkspace(lastWorkspace(), takeCurrent);
}

void YWindowManager::tilePlace(YFrameWindow *w, int tx, int ty, int tw, int th) {
    w->state(WinStateMinimized |
             WinStateRollup |
             WinStateMaximizedVert |
             WinStateMaximizedHoriz |
             WinStateHidden, 0);

    tw -= 2 * w->borderX();
    th -= 2 * w->borderY() + w->titleY();
    w->client()->constrainSize(tw, th, WinLayerNormal, 0);
    tw += 2 * w->borderX();
    th += 2 * w->borderY() + w->titleY();
    w->geometry(tx, ty, tw, th);
}

void YWindowManager::tileWindows(YFrameWindow **w, int count, bool vertical) {
    saveArrange(w, count);

    if (count == 0)
        return ;

    int curWin = 0;
    int cols = 1;

    while (cols * cols <= count)
        cols++;
    cols--;

    int areaX, areaY, areaW, areaH;

    if (vertical) { // swap meaning of rows/cols
        areaY = minX(WinLayerNormal);
        areaX = minY(WinLayerNormal);
        areaH = maxX(WinLayerNormal) - minX(WinLayerNormal);
        areaW = maxY(WinLayerNormal) - minY(WinLayerNormal);
    } else {
        areaX = minX(WinLayerNormal);
        areaY = minY(WinLayerNormal);
        areaW = maxX(WinLayerNormal) - minX(WinLayerNormal);
        areaH = maxY(WinLayerNormal) - minY(WinLayerNormal);
    }

    int normalRows = count / cols;
    int normalWidth = areaW / cols;
    int windowX = areaX;

    for (int col = 0; col < cols; col++) {
        int rows = normalRows;
        int windowWidth = normalWidth;
        int windowY = areaY;

        if (col >= (cols * (1 + normalRows) - count))
            rows++;
        if (col >= (cols * (1 + normalWidth) - areaW))
            windowWidth++;

        int normalHeight = areaH / rows;

        for (int row = 0; row < rows; row++) {
            int windowHeight = normalHeight;

            if (row >= (rows * (1 + normalHeight) - areaH))
                windowHeight++;

            if (vertical) // swap meaning of rows/cols
                tilePlace(w[curWin++],
                          windowY, windowX, windowHeight, windowWidth);
            else
                tilePlace(w[curWin++],
                          windowX, windowY, windowWidth, windowHeight);

            windowY += windowHeight;
        }
        windowX += windowWidth;
    }
}

void YWindowManager::windowsToArrange(YFrameWindow ***win, int *count) {
    YFrameWindow *w = topLayer(WinLayerNormal);

    *count = 0;
    while (w) {
        if (w->owner() == 0 && // not transient ?
            w->visibleOn(activeWorkspace()) && // visible
            !w->isSticky() && // not on all workspaces
            !w->isRollup() &&
            !w->isMinimized() &&
            !w->isHidden())
        {
            ++*count;
        }
        w = w->next();
    }
    *win = new YFrameWindow *[*count];
    int n = 0;
    w = topLayer(WinLayerNormal);
    if (*win) {
        while (w) {
            if (w->owner() == 0 && // not transient ?
                w->visibleOn(activeWorkspace()) && // visible
                !w->isSticky() && // not on all workspaces
                !w->isRollup() &&
                !w->isMinimized() &&
                !w->isHidden())
            {
                (*win)[n] = w;
                n++;
            }

            w = w->next();
        }
    }
    PRECONDITION(n == *count);
}

void YWindowManager::saveArrange(YFrameWindow **w, int count) {
    delete [] fArrangeInfo;
    fArrangeCount = count;
    fArrangeInfo = new WindowPosState[count];
    if (fArrangeInfo) {
        for (int i = 0; i < count; i++) {
            fArrangeInfo[i].x = w[i]->x();
            fArrangeInfo[i].y = w[i]->y();
            fArrangeInfo[i].w = w[i]->width();
            fArrangeInfo[i].h = w[i]->height();
            fArrangeInfo[i].state = w[i]->state();
            fArrangeInfo[i].frame = w[i];
        }
    }
}
void YWindowManager::undoArrange() {
    if (fArrangeInfo) {
        for (int i = 0; i < fArrangeCount; i++) {
            YFrameWindow *f = fArrangeInfo[i].frame;
            if (f) {
                f->state(WIN_STATE_ALL, fArrangeInfo[i].state);
                f->geometry(fArrangeInfo[i].x, fArrangeInfo[i].y,
                            fArrangeInfo[i].w, fArrangeInfo[i].h);
            }
        }
        delete [] fArrangeInfo; fArrangeInfo = 0;
        fArrangeCount = 0;
        if (clickFocus || !strongPointerFocus)
            focusTopWindow();
    }
}

bool YWindowManager::haveClients() {
    for (YFrameWindow * f(topLayer()); f ; f = f->nextLayer())
        if (f->canClose() && f->client()->adopted())
            return true;

    return false;
}

void YWindowManager::exitAfterLastClient(bool shuttingDown) {
    fShuttingDown = shuttingDown;
    checkLogout();
}

/******************************************************************************/

#ifdef CONFIG_WM_SESSION
void YWindowManager::topLevelProcess(pid_t p) {
    if (p != getpid() && p != PID_MAX) {
	msg("moving process %d to the top", p);
	fProcessList.push(p);
    }
}

void YWindowManager::removeLRUProcess() {
    pid_t const lru(fProcessList[0]);
    msg("Kernel sent a cleanup request. Closing process %d.", lru);

    Window leader(None);

    for (YFrameWindow * f(topLayer()); f; f = f->nextLayer())
	if (f->client()->pid() == lru &&
	   (None != (leader = f->client()->clientLeader()) ||
	    None != (leader = f->client()->handle())))
	    break;

    if (leader != None) { // the group leader doesn't have to be mapped
	msg("Sending WM_DELETE_WINDOW to %p", leader);
	YFrameClient::sendMessage(leader, atoms.wmDeleteWindow, CurrentTime);
    }

/* !!! TODO:	- windows which do not support WM_DELETE_WINDOW
		- unmapping -> removing from process list
		- leader == None --> loop over all processes?
		- s/msg/MSG/
		- apps launched from icewm ignore the PRELOAD library
*/
}
#endif /* CONFIG_WM_SESSION */


char const * YWindowManager::name() {
    return "IceWM "VERSION"-"RELEASE" ("HOSTOS"/"HOSTCPU")";
}

/******************************************************************************
 * Proxy window for desktop (icon) managers
 ******************************************************************************/

YProxyWindow::YProxyWindow(YWindow *parent): YWindow(parent) {
    style(wsOverrideRedirect);
}

YProxyWindow::~YProxyWindow() {
}

void YProxyWindow::handleButton(const XButtonEvent &/*button*/) {
}


/******************************************************************************
 * Workspace switching when edges are touched
 ******************************************************************************/

YTimer *EdgeSwitch::fEdgeSwitchTimer(NULL);

EdgeSwitch::EdgeSwitch(YWindowManager *manager, int delta, bool vertical):
YWindow(manager),
fManager(manager),
fCursor(delta < 0 ? vertical ? YWMApp::scrollUpPointer
                             : YWMApp::scrollLeftPointer
                  : vertical ? YWMApp::scrollDownPointer
                             : YWMApp::scrollRightPointer),
fDelta(delta) {
    style(wsOverrideRedirect | wsInputOnly);
    pointer(YApplication::leftPointer);
}

EdgeSwitch::~EdgeSwitch() {
    if (fEdgeSwitchTimer && fEdgeSwitchTimer->timerListener() == this) {
        fEdgeSwitchTimer->stop();
        fEdgeSwitchTimer->timerListener(NULL);
        delete fEdgeSwitchTimer;
        fEdgeSwitchTimer = NULL;
    }
}

void EdgeSwitch::handleCrossing(const XCrossingEvent &crossing) {
    if (crossing.type == EnterNotify && crossing.mode == NotifyNormal) {
        if (!fEdgeSwitchTimer)
            fEdgeSwitchTimer = new YTimer(edgeSwitchDelay);
        if (fEdgeSwitchTimer) {
            fEdgeSwitchTimer->timerListener(this);
            fEdgeSwitchTimer->start();
            pointer(fCursor);
        }
    } else if (crossing.type == LeaveNotify && crossing.mode == NotifyNormal) {
        if (fEdgeSwitchTimer && fEdgeSwitchTimer->timerListener() == this) {
            fEdgeSwitchTimer->stop();
            fEdgeSwitchTimer->timerListener(NULL);
            pointer(YApplication::leftPointer);
        }
    }
}

bool EdgeSwitch::handleTimer(YTimer *t) {
    if (t != fEdgeSwitchTimer)
        return false;

    if (fDelta == -1)
        fManager->switchToPrevWorkspace(false);
    else
        fManager->switchToNextWorkspace(false);

    if (edgeContWorkspaceSwitching) {
        return true;
    } else {
        pointer(YApplication::leftPointer);
        return false;
    }
}
