/*
 * IceWM
 *
 * Copyright (C) 1997-2001 Marko Macek
 *
 * Window list
 */
#include "config.h"
#include "ykey.h"
#include "wmwinlist.h"
#include "ymenuitem.h"

#include "wmaction.h"
#include "wmclient.h"
#include "wmframe.h"
#include "wmmgr.h"
#include "wmapp.h"
#include "sysdep.h"

#include "intl.h"

#ifdef CONFIG_WINLIST

WindowList *windowList = 0;

WindowListItem::WindowListItem(YClientPeer *peer):
    YListItem(), fPeer(peer) {
}

WindowListItem::~WindowListItem() {
    if (fPeer) {
        fPeer->winListItem(NULL);
        fPeer = NULL;
    }
}

int WindowListItem::offset() {
    int ofs = 0;
    
    for (YClientPeer *peer(peer()); peer->owner(); peer = peer->owner())
        ofs += 20;

    return ofs;
}

const char *WindowListItem::text() {
    return peer()->title();
}

YIcon *WindowListItem::icon() {
    return peer()->icon();
}


WindowListBox::WindowListBox(YScrollView *view, YWindow *aParent):
    YListBox(view, aParent)
{
}

WindowListBox::~WindowListBox() {
}

void WindowListBox::activateItem(YListItem *item) {
    WindowListItem *i = (WindowListItem *)item;
    YClientPeer *f = i->peer();
    if (f) {
        f->activateWindow(true);
        windowList->frame()->wmHide();
    }
}

void WindowListBox::actionPerformed(YAction *action, unsigned int modifiers) {
    if (action == actionTileVertical ||
        action == actionTileHorizontal)
    {
        if (hasSelection()) {
            YListItem *i;
            int count = 0;
            YFrameWindow **w;

            for (i = first(); i; i = i->next())
                if (isSelected(i))
                    count++;

            if (count > 0) {
                w = new YFrameWindow *[count];
                if (w) {
                    int n = 0;

                    for (i = first(); i; i = i->next())
                        if (isSelected(i)) {
                            WindowListItem *item = (WindowListItem *)i;
                            w[n++] = (YFrameWindow *)item->peer();
                        }
                    PRECONDITION(n == count);

                    manager->tileWindows(w, count, action == actionTileVertical);
                    delete w;
                }
            }
        }
    } else if (action == actionCascade ||
               action == actionArrange)
    {
        if (hasSelection()) {
            YFrameWindow *f;
            YListItem *i;
            int count = 0;
            YFrameWindow **w;

            for (f = manager->topLayer(); f; f = f->nextLayer()) {
                i = f->winListItem();
                if (i && isSelected(i))
                    count++;
            }
            if (count > 0) {
                w = new YFrameWindow *[count];
                if (w) {
                    int n = 0;
                    for (f = manager->topLayer(); f; f = f->nextLayer()) {
                        i = f->winListItem();
                        if (i && isSelected(i))
                            w[n++] = f;
                    }

                    if (action == actionCascade) {
                        manager->cascadePlace(w, count);
                    } else if (action == actionArrange) {
                        manager->smartPlace(w, count);
                    }
                    delete w;
                }
            }
        }
    } else {
        if (hasSelection()) {
            YListItem *i;

            for (i = first(); i; i = i->next()) {
                if (isSelected(i)) {
                    WindowListItem *item = (WindowListItem *)i;
#ifndef CONFIG_PDA		    
                    if (action == actionHide)
                        if (item->peer()->isHidden())
                            continue;
#endif			    
                    if (action == actionMinimize)
                        if (item->peer()->isMinimized())
                            continue;
                    item->peer()->actionPerformed(action, modifiers);
                }
            }
        }
    }
}

bool WindowListBox::handleKey(const XKeyEvent &key) {
    if (key.type == KeyPress) {
        KeySym k = XKeycodeToKeysym(app->display(), key.keycode, 0);
        int m = KEY_MODMASK(key.state);

        switch (k) {
        case XK_Escape:
            windowList->frame()->wmHide();
            return true;
        case XK_F10:
        case XK_Menu:
            if (k != XK_F10 || m == ShiftMask) {
                if (hasSelection()) {
                    moveMenu->enableCommand(0);
                    windowListPopup->popup(0, 0,
                                           key.x_root, key.y_root, -1, -1,
                                           YPopupWindow::pfCanFlipVertical |
                                           YPopupWindow::pfCanFlipHorizontal |
                                           YPopupWindow::pfPopupMenu);
                } else {
                    windowListAllPopup->popup(0, 0, key.x_root, key.y_root, -1, -1,
                                              YPopupWindow::pfCanFlipVertical |
                                              YPopupWindow::pfCanFlipHorizontal |
                                              YPopupWindow::pfPopupMenu);
                }
            }
            break;
        case XK_Delete:
            {
                actionPerformed(actionClose, key.state);
            }
            break;
        }
    }
    return YListBox::handleKey(key);
}

void WindowListBox::handleClick(const XButtonEvent &up, int count) {
    if (up.button == 3 && count == 1 && IS_BUTTON(up.state, Button3Mask)) {
        int no = findItemByPoint(up.x, up.y);

        if (no != -1) {
            YListItem *i = item(no);
            if (!isSelected(i)) {
                focusSelectItem(no);
            } else {
                //fFocusedItem = -1;
            }
            moveMenu->enableCommand(0);
            windowListPopup->popup(0, 0,
                                   up.x_root, up.y_root, -1, -1,
                                   YPopupWindow::pfCanFlipVertical |
                                   YPopupWindow::pfCanFlipHorizontal |
                                   YPopupWindow::pfPopupMenu);
        } else {
            windowListAllPopup->popup(0, 0, up.x_root, up.y_root, -1, -1,
                                      YPopupWindow::pfCanFlipVertical |
                                      YPopupWindow::pfCanFlipHorizontal |
                                      YPopupWindow::pfPopupMenu);

        }
        return ;
    }
    YListBox::handleClick(up, count);
}

WindowList::WindowList(YWindow *parent):
YFrameClient(parent) {
    fScroll = new YScrollView(this);
    fList = new WindowListBox(fScroll, fScroll);
    fScroll->view(fList);
    fList->show();
    fScroll->show();

    YMenu *closeSubmenu = new YMenu();
    assert(closeSubmenu != 0);

    closeSubmenu->addItem(_("_Close"), -2, _("Delete"), actionClose);
    closeSubmenu->addSeparator();
    closeSubmenu->addItem(_("_Kill Client"), -2, 0, actionKill);
#if 0
    closeSubmenu->addItem(_("_Terminate Process"), -2, 0, actionTermProcess)->enabled(false);
    closeSubmenu->addItem(_("Kill _Process"), -2, 0, actionKillProcess)->enabled(false);
#endif

    windowListPopup = new YMenu();
    windowListPopup->actionListener(fList);
    windowListPopup->addItem(_("_Show"), -2, 0, actionShow);
#ifndef CONFIG_PDA		    
    windowListPopup->addItem(_("_Hide"), -2, 0, actionHide);
#endif
    windowListPopup->addItem(_("_Minimize"), -2, 0, actionMinimize);
    windowListPopup->addSubmenu(_("Move _To"), -2, moveMenu);
    windowListPopup->addSeparator();
    windowListPopup->addItem(_("Tile _Vertically"), -2, KEY_NAME(gKeySysTileVertical), actionTileVertical);
    windowListPopup->addItem(_("T_ile Horizontally"), -2, KEY_NAME(gKeySysTileHorizontal), actionTileHorizontal);
    windowListPopup->addItem(_("Ca_scade"), -2, KEY_NAME(gKeySysCascade), actionCascade);
    windowListPopup->addItem(_("_Arrange"), -2, KEY_NAME(gKeySysArrange), actionArrange);
    windowListPopup->addSeparator();
    windowListPopup->addItem(_("_Minimize All"), -2, KEY_NAME(gKeySysMinimizeAll), actionMinimizeAll);
    windowListPopup->addItem(_("_Hide All"), -2, KEY_NAME(gKeySysHideAll), actionHideAll);
    windowListPopup->addItem(_("_Undo"), -2, KEY_NAME(gKeySysUndoArrange), actionUndoArrange);
    windowListPopup->addSeparator();
    windowListPopup->addItem(_("_Close"), -2, actionClose, closeSubmenu);

    windowListAllPopup = new YMenu();
    windowListAllPopup->actionListener(wmapp);
    windowListAllPopup->addItem(_("Tile _Vertically"), -2, KEY_NAME(gKeySysTileVertical), actionTileVertical);
    windowListAllPopup->addItem(_("T_ile Horizontally"), -2, KEY_NAME(gKeySysTileHorizontal), actionTileHorizontal);
    windowListAllPopup->addItem(_("Ca_scade"), -2, KEY_NAME(gKeySysCascade), actionCascade);
    windowListAllPopup->addItem(_("_Arrange"), -2, KEY_NAME(gKeySysArrange), actionArrange);
    windowListAllPopup->addItem(_("_Minimize All"), -2, KEY_NAME(gKeySysMinimizeAll), actionMinimizeAll);
    windowListAllPopup->addItem(_("_Hide All"), -2, KEY_NAME(gKeySysHideAll), actionHideAll);
    windowListAllPopup->addItem(_("_Undo"), -2, KEY_NAME(gKeySysUndoArrange), actionUndoArrange);

    int w = desktop->width();
    int h = desktop->height();

    geometry(w / 3, h / 3, w / 3, h / 3);

    windowList = this;
    windowTitle(_("Window list"));
    iconTitle(_("Window list"));
    winState(WinStateAllWorkspaces, WinStateAllWorkspaces);
    winWorkspace(0);
    winLayer(WinLayerAboveDock);
}

WindowList::~WindowList() {
    delete fList;
    delete fScroll;
}

void WindowList::handleFocus(const XFocusChangeEvent &focus) {
    if (focus.type == FocusIn) {
        fList->windowFocus();
    } else if (focus.type == FocusOut) {
    }
}

void WindowList::relayout() {
    fList->repaint();
}

WindowListItem *WindowList::addWindowListApp(YFrameWindow *frame) {
    if (!frame->client()->adopted())
        return 0;
    WindowListItem *item = new WindowListItem(frame);
    if (item) {
        if (frame->owner() &&
            frame->owner()->winListItem())
        {
            fList->addAfter(frame->owner()->winListItem(), item);
        } else {
            fList->addItem(item);
        }
    }
    return item;
}

void WindowList::removeWindowListApp(WindowListItem *item) {
    if (item) {
        fList->removeItem(item);
        delete item;
    }
}

void WindowList::configure(const int x, const int y, 
			   const unsigned width, const unsigned height, 
			   const bool resized) {
    YFrameClient::configure(x, y, width, height, resized);
    if (resized) fScroll->geometry(0, 0, width, height);
}

void WindowList::handleClose() {
    if (!frame()->isHidden()) frame()->wmHide();
}

void WindowList::showFocused(int x, int y) {
    YFrameWindow *f = manager->focus();

    if (f != frame())
        fList->focusSelectItem(f ? fList->findItem(f->winListItem()) : 0);

    if (NULL == frame())
        manager->manageClient(handle(), false);

    if (frame()) {
        if (x != -1 && y != -1) {
            int px(x - frame()->width() / 2);
            int py(y - frame()->height() / 2);

            px = clamp(px, 0, (int)desktop->width() - (int)frame()->width());
            py = clamp(py, 0, (int)desktop->height() - (int)frame()->height());

            frame()->position(px, py);
        }

        frame()->layer(WinLayerAboveDock);
        frame()->state(WinStateAllWorkspaces, WinStateAllWorkspaces);
        frame()->activate(true);
    }
}
#endif
