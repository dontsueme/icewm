/*
 * IceWM
 *
 * Copyright (C) 1997-2001 Marko Macek
 */
#include "config.h"

#include "yfull.h"
#include "ykey.h"
#include "ymenu.h"
#include "yaction.h"
#include "ymenuitem.h"

#include "yapp.h"
#include "prefs.h"

#include "ypixbuf.h"

#include <string.h>

YColor *menuBg = 0;
YColor *menuItemFg = 0;
YColor *activeMenuItemBg = 0;
YColor *activeMenuItemFg = 0;
YColor *disabledMenuItemFg = 0;
YColor *disabledMenuItemSt = 0;

YFont *menuFont = 0;

int YMenu::fAutoScrollDeltaX(0);
int YMenu::fAutoScrollDeltaY(0);
int YMenu::fAutoScrollMouseX(-1);
int YMenu::fAutoScrollMouseY(-1);

#ifdef OLDMOTIF
#define CONFIG_RAISE_MENU_ITEM (wmLook == lookMotif)
#else
#define CONFIG_RAISE_MENU_ITEM (false)
#endif

void YMenu::actionListener(YAction::Listener *actionListener) {
    fActionListener = actionListener;
}

void YMenu::finishPopup(YMenuItem *item, YAction *action, unsigned modifiers) {
    YPopupWindow::finishPopup();

    if (item) item->actionPerformed(fActionListener, action, modifiers);
}

YTimer *YMenu::fMenuTimer(NULL);
int YMenu::fTimerX(0), YMenu::fTimerY(0), YMenu::fTimerSubmenu(0);
unsigned YMenu::fTimerItem(0);
bool YMenu::fTimerSlow(false);

YMenu::YMenu(YWindow *parent): 
    YPopupWindow(parent) INIT_GRADIENT(fGradient, NULL) {
    if (menuFont == 0)
        menuFont = YFont::font(menuFontName);
    if (menuBg == 0)
        menuBg = new YColor(clrNormalMenu);
    if (menuItemFg == 0)
        menuItemFg = new YColor(clrNormalMenuItemText);
    if (*clrActiveMenuItem && activeMenuItemBg == 0)
        activeMenuItemBg = new YColor(clrActiveMenuItem);
    if (activeMenuItemFg == 0)
        activeMenuItemFg = new YColor(clrActiveMenuItemText);
    if (disabledMenuItemFg == 0)
        disabledMenuItemFg = new YColor(clrDisabledMenuItemText);
    if (disabledMenuItemSt == 0)
        disabledMenuItemSt = *clrDisabledMenuItemShadow
			   ? new YColor(clrDisabledMenuItemShadow)
			   : menuBg->brighter();

    fItems = 0;
    fItemCount = 0;
    fPaintedItem = fSelectedItem = ~0U;
    fPopup = 0;
    fActionListener = 0;
    fPopupActive = 0;
    fShared = false;
    activatedX = -1;
    activatedY = -1;
}

YMenu::~YMenu() {
    if (fMenuTimer && fMenuTimer->timerListener() == this) {
        fMenuTimer->timerListener(NULL);
        fMenuTimer->stop();
    }
    if (fPopup) {
        fPopup->popdown();
        fPopup = 0;
    }
    
    for (unsigned i(0); i < fItemCount; ++i) delete fItems[i];
    FREE(fItems); fItems = 0;

#ifdef CONFIG_GRADIENTS
    delete fGradient;
#endif    
}

void YMenu::activatePopup() {
    if (popupFlags() & pfButtonDown) fSelectedItem = ~0U;
    else focusItem(findActiveItem(itemCount() - 1, 1), 0, 0);
}

void YMenu::deactivatePopup() {
    if (fPopup) {
        fPopup->popdown();
        fPopup = 0;
    }
}

void YMenu::donePopup(YPopupWindow *popup) {
    PRECONDITION(popup != 0);
    PRECONDITION(fPopup != 0);
    if (fPopup) {
        fPopup->popdown();
        fPopup = NULL;

        if (fSelectedItem != ~0U &&
            item(fSelectedItem)->submenu() == popup)
            paintItems();
    }
}

bool YMenu::isCondCascade(unsigned selItem) {
    return selItem != NoItem && item(selItem)->action() &&
                                item(selItem)->submenu();
}

bool YMenu::onCascadeButton(unsigned selItem, int x, int /*y*/, bool /*checkPopup*/) {
    if (isCondCascade(selItem)) {
        if (fPopup && fPopup == item(selItem)->submenu()) return true;

        unsigned fontHeight(menuFont->height() + 1);
        unsigned h(fontHeight);

        if (item(selItem)->icon() && 
	    item(selItem)->icon()->height() > h)
            h = item(selItem)->icon()->height();

        if (x <= int(width() - h - 4)) return false;
    }

    return true;
}

void YMenu::focusItem(unsigned nItem, bool submenu, bool byMouse) {
    fSelectedItem = nItem;

    if (fSelectedItem != ~0U) {
        if (x() < 0 || y() < 0 ||
            x() + width() > desktop->width() ||
            y() + height() > desktop->height())
        {
            int ix, iy, ih, t, b, p;

            findItemPos(fSelectedItem, ix, iy);
            ih = itemHeight(fSelectedItem, t, b, p);

            position(x(), clamp(y(), -iy, (int)desktop->height() - ih - iy));
        }
    }

    YMenu *sub = 0;
    if (fSelectedItem != ~0U)
        sub = item(fSelectedItem)->submenu();

    if (sub != fPopup) {
        int repaint = 0;

        if (fPopup) {
            fPopup->popdown();
            fPopup = 0;
            repaint = 1;
        }

        if (submenu && sub && item(fSelectedItem)->isEnabled()) {
            int xp, yp;
            int l, t, r, b;

            offsets(l, t, r, b);
            findItemPos(fSelectedItem, xp, yp);
            sub->actionListener(actionListener());
            sub->popup(this, 0,
                       x() + width() - r, y() + yp - t,
                       width() - r - l, -1,
                       YPopupWindow::pfCanFlipHorizontal |
                       (popupFlags() & YPopupWindow::pfFlipHorizontal) |
                       (byMouse ? (unsigned)YPopupWindow::pfButtonDown : 0U));
            fPopup = sub;
            repaint = 1;
        }
        if (repaint &&
            fSelectedItem != NoItem &&
            fPaintedItem == fSelectedItem &&
            item(fSelectedItem)->action())
            paintItems();
    }
    if (fPaintedItem != fSelectedItem)
        paintItems();
}

unsigned YMenu::findActiveItem(unsigned cur, int direction) {
    PRECONDITION(direction == -1 || direction == 1);

    if (itemCount() == 0) return NoItem;

    if (cur == NoItem)
        cur = direction > 0 ? itemCount() - 1 : 0;

    PRECONDITION((unsigned)cur < itemCount());

    unsigned c(cur);

    do {
        if (c == 0) c = itemCount();
        c += direction;
        if (c >= itemCount()) c = 0;
    } while (c != cur && (!item(c)->action() && !item(c)->submenu()));

    return c;
}

#ifdef DEBUG
void YMenu::activateItem(unsigned nItem, bool byMouse, unsigned modifiers) {
#else
void YMenu::activateItem(unsigned /*no*/, bool byMouse, unsigned modifiers) {
#endif
    PRECONDITION(fSelectedItem == nItem && fSelectedItem != NoItem);

    if (item(fSelectedItem)->isEnabled()) {
        if (item(fSelectedItem)->action() == 0 &&
            item(fSelectedItem)->submenu() != 0)
            focusItem(fSelectedItem, 1, byMouse);
        else if (item(fSelectedItem)->action())
            finishPopup(item(fSelectedItem),
	    		item(fSelectedItem)->action(), modifiers);
    } else {
        //app->alert();
    }
}

unsigned YMenu::findHotItem(char key) {
    unsigned count = 0;

    for (unsigned i(0); i < itemCount(); ++i) {
        int hot = item(i)->hotChar();

        if (hot != -1 && TOUPPER(char(hot)) == key) ++count;
    }
    if (count == 0) return 0;

    unsigned cur(fSelectedItem);
    if (cur == NoItem) cur = itemCount() - 1;

    unsigned c(cur);

    do {
        ++c;

        if (c >= itemCount()) c = 0;

        if (item(c)->action() || item(c)->submenu()) {
            int hot = item(c)->hotChar();

            if (hot != -1 && TOUPPER(char(hot)) == key) {
                focusItem(c, 0, 0);
                break;
            }
        }
    } while (c != cur);

    return count;
}

bool YMenu::handleKey(const XKeyEvent &key) {
    KeySym k = XKeycodeToKeysym(app->display(), key.keycode, 0);
    int m = KEY_MODMASK(key.state);

    if (key.type == KeyPress) {
        if ((m & ~ShiftMask) == 0) {
            if (k == XK_Escape) {
                cancelPopup();
            } else if (k == XK_Left || k == XK_KP_Left) {
                if (prevPopup())
                    cancelPopup();
            } else if (itemCount() > 0) {
                if (k == XK_Up || k == XK_KP_Up)
                    focusItem(findActiveItem(fSelectedItem, -1), 0, 0);
                else if (k == XK_Down || k == XK_KP_Down)
                    focusItem(findActiveItem(fSelectedItem, 1), 0, 0);
                else if (k == XK_Home || k == XK_KP_Home)
                    focusItem(findActiveItem(itemCount() - 1, 1), 0, 0);
                else if (k == XK_End || k == XK_KP_End)
                    focusItem(findActiveItem(0, -1), 0, 0);
                else if (k == XK_Right || k == XK_KP_Right)
                    focusItem(fSelectedItem, 1, 0);
                else if (k == XK_Return || k == XK_KP_Enter) {
                    if (fSelectedItem != NoItem &&
                        (item(fSelectedItem)->action() != 0 ||
                         item(fSelectedItem)->submenu() != 0)) {
                        activateItem(fSelectedItem, 0, key.state);
                        return true;
                    }
                } else if ((k < 256) && ((m & ~ShiftMask) == 0)) {
                    if (findHotItem(TOUPPER(char(k))) == 1) {
                        if (!(m & ShiftMask))
                            activateItem(fSelectedItem, 0, key.state);
                    }
                    return true;
                }
            }
        }
    }
    return YPopupWindow::handleKey(key);
}

void YMenu::handleButton(const XButtonEvent &button) {
    if (button.button == Button4) {
	position(x(), clamp(y() - (int)(button.state & ShiftMask ? 
					menuFont->height() * 5/2 :
					menuFont->height()),
			       button.y_root - (int)height() + 1,
			       button.y_root));
        if (menuMouseTracking)
	    trackMotion(clamp(button.x_root, x() + 2, x() + (int)width() - 3),
			      button.y_root, button.state);
    } else if (button.button == Button5) {
	position(x(), clamp(y() + (int)(button.state & ShiftMask ? 
					menuFont->height() * 5/2 :
					menuFont->height()),
			       button.y_root - (int)height() + 1,
			       button.y_root));
        if (menuMouseTracking)
	    trackMotion(clamp(button.x_root, x() + 2, x() + (int)width() - 3),
			      button.y_root, button.state);
    } else if (button.button) {
        int const selItem(findItem(button.x_root - x(), button.y_root - y()));
        bool const nocascade(onCascadeButton(selItem,
					     button.x_root - x(),
                                             button.y_root - y(), true) ||
                             (button.state & ControlMask));

        if (button.type == ButtonRelease &&
	    fPopupActive == fPopup && fPopup != NULL &&
	    nocascade) {
            fPopup->popdown();
            fPopupActive = fPopup = 0;
            focusItem(selItem, 0, 1);
            if (nocascade) paintItems();
            return;
        } else if (button.type == ButtonPress)
            fPopupActive = fPopup;

        focusItem(selItem, nocascade, 1);

        if (fSelectedItem != NoItem &&
            button.type == ButtonRelease &&
            (item(fSelectedItem)->submenu() != NULL ||
             item(fSelectedItem)->action() != NULL)
            &&
            (item(fSelectedItem)->action() == NULL ||
             item(fSelectedItem)->submenu() == NULL ||
	     !nocascade)) { // ??? !!! ??? WTF
            activatedX = button.x_root;
            activatedY = button.y_root;
            activateItem(fSelectedItem, 1, button.state);
            return;
        }

        if (button.type == ButtonRelease &&
            (fSelectedItem == NoItem ||
            (!item(fSelectedItem)->action() &&
	     !item(fSelectedItem)->submenu())))
            focusItem(findActiveItem(itemCount() - 1, 1), 0, 0);
    }
    YPopupWindow::handleButton(button);
}

void YMenu::handleMotion(const XMotionEvent &motion) {
    bool isButton =
        (motion.state & (Button1Mask |
                         Button2Mask |
                         Button3Mask |
                         Button4Mask |
                         Button5Mask));

    if (menuMouseTracking || isButton)
	trackMotion(motion.x_root, motion.y_root, motion.state);

    if (menuFont != NULL) { // ================ autoscrolling of large menus ===
        int const fh(menuFont->height());

        int const sx(motion.x_root < fh ? +fh :
		     motion.x_root >= (int)(desktop->width() - fh - 1) ? -fh :
		     0),
        	  sy(motion.y_root < fh ? +fh :
		     motion.y_root >= (int)(desktop->height() - fh - 1) ? -fh :
		     0);

	autoScroll(sx, sy, motion.x_root, motion.y_root, &motion);
    }

    YPopupWindow::handleMotion(motion); // ========== default implementation ===
}

void YMenu::trackMotion(const int x_root, const int y_root,
			const unsigned state) {
    unsigned selItem(findItem(x_root - x(), y_root - y()));

    if (fMenuTimer && fMenuTimer->timerListener() == this) {
        //msg("sel=%d timer=%d listener=%p =? this=%p", selItem, fTimerItem, fMenuTimer->timerListener(), this);
        if (selItem != fTimerItem || fTimerSlow) {
            fTimerItem = NoItem;
            if (fMenuTimer) fMenuTimer->stop();
        }
    }

    if (selItem != NoItem || app->popup() == this) {
        const bool submenu(state & ControlMask || 
			   onCascadeButton(selItem,
					   x_root - x(), y_root - y(), false));
        //if (selItem != NoItem)
        bool canFast = true;

        if (fPopup && activatedX != -1 && SubmenuActivateDelay != 0) {
            int dx = 0;
            int dy = y_root - activatedY;
            int ty = fPopup->y() - activatedY;
            int by = fPopup->y() + fPopup->height() - activatedY;
            int px;

            if (fPopup->x() < activatedX)
                px = activatedX - (fPopup->x() + fPopup->width());
            else
                px = fPopup->x() - activatedX;

            if (fPopup->x() > x_root)
                dx = x_root - activatedX;
            else
                dx = activatedX - x_root;

            dy = dy * px;

            if (dy >= ty * dx * 2 && dy <= by * dx * 2)
                canFast = false;
        }

        if (canFast) {
            YPopupWindow *p = fPopup;

            if (MenuActivateDelay != 0 && selItem != NoItem) {
                if (fMenuTimer == 0)
                    fMenuTimer = new YTimer();
                if (fMenuTimer) {
                    fMenuTimer->interval(MenuActivateDelay);
                    fMenuTimer->timerListener(this);
                    if (!fMenuTimer->running())
                        fMenuTimer->start();
                }
                fTimerItem = selItem;
                fTimerX = x_root;
                fTimerY = y_root;
                fTimerSubmenu = submenu;
                fTimerSlow = false;
            } else {
                focusItem(selItem, submenu, 1);
                if (fPopup && p != fPopup) {
                    activatedX = x_root;
                    activatedY = y_root;
                }
            }
        } else {
            //focusItem(selItem, 0, 1);
            fTimerItem = selItem;
            fTimerX = x_root;
            fTimerY = y_root;
            fTimerSubmenu = submenu;
            fTimerSlow = true;
            if (fMenuTimer == 0)
                fMenuTimer = new YTimer();
            if (fMenuTimer) {
                fMenuTimer->interval(SubmenuActivateDelay);
                fMenuTimer->timerListener(this);
                if (!fMenuTimer->running())
                    fMenuTimer->start();
            }
        }
    }
}

bool YMenu::handleTimer(YTimer */*timer*/) {
    activatedX = fTimerX;
    activatedY = fTimerY;
    focusItem(fTimerItem, fTimerSubmenu, 1);
    return false;
}

bool YMenu::handleAutoScroll(const XMotionEvent & /*mouse*/) {
    int px = x();
    int py = y();

    if (fAutoScrollDeltaX != 0) {
        if (fAutoScrollDeltaX < 0) {
            if (px + width() > desktop->width())
                px += fAutoScrollDeltaX;
        } else {
            if (px < 0)
                px += fAutoScrollDeltaX;
        }
    }
    if (fAutoScrollDeltaY != 0) {
        if (fAutoScrollDeltaY < 0) {
            if (py + height() > desktop->height())
                py += fAutoScrollDeltaY;
        } else {
            if (py < 0)
                py += fAutoScrollDeltaY;
        }
    }
    position(px, py);
    {
        int mx = fAutoScrollMouseX - x();
        int my = fAutoScrollMouseY - y();

        int selItem = findItem(mx, my);
        focusItem(selItem, 0, 1);
    }
    return true;
}

void YMenu::autoScroll(int deltaX, int deltaY, int mx, int my, const XMotionEvent *motion) {
    fAutoScrollDeltaX = deltaX;
    fAutoScrollDeltaY = deltaY;
    fAutoScrollMouseX = mx;
    fAutoScrollMouseY = my;
    beginAutoScroll((deltaX != 0 || deltaY != 0), motion);
}

YMenuItem *YMenu::addItem(const char *name, int hotCharPos, const char *param, YAction *action) {
    return add(new YMenuItem(name, hotCharPos, param, action, 0));
}

YMenuItem *YMenu::addItem(const char *name, int hotCharPos, YAction *action, YMenu *submenu) {
    return add(new YMenuItem(name, hotCharPos, 0, action, submenu));
}

YMenuItem * YMenu::addSubmenu(const char *name, int hotCharPos, YMenu *submenu) {
    return add(new YMenuItem(name, hotCharPos, 0, 0, submenu));
}

YMenuItem * YMenu::addSeparator() {
    return add(new YMenuItem());
}

YMenuItem * YMenu::addLabel(const char *name) {
    return add(new YMenuItem(name));
}

void YMenu::removeAll() {
    if (fPopup) {
        fPopup->popdown();
        fPopup = 0;
    }
    for (unsigned i(0); i < itemCount(); ++i) delete fItems[i];
    FREE(fItems); fItems = 0;
    fItemCount = 0;
    fPaintedItem = fSelectedItem = NoItem;
    fPopup = 0;
}

YMenuItem * YMenu::add(YMenuItem *item) {
    if (item) {
        YMenuItem **newItems = (YMenuItem **)REALLOC(fItems, sizeof(YMenuItem *) * ((fItemCount) + 1));
        if (newItems) {
            fItems = newItems;
            fItems[fItemCount++] = item;
            return item;
        } else {
            delete item;
            return 0;
        }
    }
    return item;
}

YMenuItem *YMenu::findAction(const YAction *action) {
    for (unsigned i(0); i < itemCount(); ++i)
        if (action == item(i)->action()) return item(i);

    return NULL;
}

YMenuItem *YMenu::findSubmenu(const YMenu *sub) {
    for (unsigned i(0); i < itemCount(); ++i)
        if (sub == item(i)->submenu()) return item(i);

    return NULL;
}

YMenuItem *YMenu::findName(const char *name, const unsigned first) {
    if (name != NULL)
        for (unsigned i(first); i < itemCount(); ++i) {
            const char *iname = item(i)->name();
            if (iname && !strcmp(name, iname))
                return item(i);
        }

    return NULL;
}

void YMenu::enableCommand(YAction *action) {
    for (unsigned i(0); i < itemCount(); ++i)
        if (action == 0 || action == item(i)->action())
            item(i)->enabled();
}

void YMenu::disableCommand(YAction *action) {
    for (unsigned i(0); i < itemCount(); ++i)
        if (action == 0 || action == item(i)->action())
            item(i)->enabled(false);
}

unsigned YMenu::itemHeight(unsigned item, int &top, int &bottom, int &pad) {
    top = bottom = pad = 0;
    if (item >= itemCount()) return 0;

    YMenuItem *menuItem(this->item(item));
    if (menuItem->name() || menuItem->submenu()) {
        unsigned ih(menuFont->height() + 1);

        if (menuItem->icon() &&
            menuItem->icon()->height() > ih)
            ih = menuItem->icon()->height();

        if (wmLook == lookWarp4 || wmLook == lookWin95) {
            top = bottom = 0;
            pad = 1;
        } else if (wmLook == lookMetal) {
            top = bottom = 1;
            pad = 1;
        } else if (wmLook == lookMotif) {
            top = bottom = 2;
            pad = 0; //1
        } else if (wmLook == lookGtk) {
            top = bottom = 2;
            pad = 0; //1
        } else {
            top = 1;
            bottom = 2;
            pad = 0;//1;
        }

        return top + pad + ih + pad + bottom;
    } else {
        pad = 1;
        return wmLook == lookMetal ? 3 : 4;
    }
}

void YMenu::itemWidth(unsigned i, int &iw, int &nw, int &pw) {
    iw = nw = pw = 0;

    if (item(i)->name() != NULL || item(i)->submenu() != NULL) {
        YIcon::Image const *icon(item(i)->icon());
        if (icon) iw = icon->height();

        const char *name(item(i)->name());
        if (name) nw = menuFont->textWidth(name);

        const char *param(item(i)->param());
        if (param) pw = menuFont->textWidth(param);
    }
}

void YMenu::offsets(int &left, int &top, int &right, int &bottom) {
    if (wmLook == lookMetal) {
        left = 1;
        right = 1;
        top = 2;
        bottom = 2;
    } else {
        left = 2;
        top = 2;
        right = 3;
        bottom = 3;
    }
}

void YMenu::area(int &x, int &y, int &w, int &h) {
    offsets(x, y, w, h);
    w = width() - 1 - x - w;
    h = height() - 1 - y - h;
}

void YMenu::findItemPos(unsigned itemNo, int &x, int &y) {
    x = -1;
    y = -1;

    if (itemNo < itemCount()) {
        int w, h;

    	area(x, y, w, h);

        for (unsigned i(0); i < itemNo; ++i) {
            int ih, top, bottom, pad;
            ih = itemHeight(i, top, bottom, pad);
            y += ih;
        }
    }
}

unsigned YMenu::findItem(int mx, int my) {
    int x, y, w, h;

    area(x, y, w, h);
    for (unsigned i(0); i < itemCount(); ++i) {
        int top, bottom, pad;

        h = itemHeight(i, top, bottom, pad);
        if (my >= y && my < y + h && mx > 0 && mx < int(width()) - 1)
            return i;

        y += h;
    }

    return NoItem;
}

void YMenu::sizePopup(int hspace) {
    unsigned width, height;
    int maxName(0);
    int maxParam(0);
    int maxIcon(16);
    int l, t, r, b;
    int padx(1);
    int left(1);

    offsets(l, t, r, b);

    width = l;
    height = t;

    for (unsigned i(0); i < itemCount(); ++i) {
        int ih, top, bottom, pad;

        ih = itemHeight(i, top, bottom, pad);
        height += ih;

        if (pad > padx)
            padx = pad;
        if (top > left)
            left = top;

        int iw, nw, pw;

        itemWidth(i, iw, nw, pw);

        if (item(i)->submenu())
            pw += 2 + ih;

        if (iw > maxIcon)
            maxIcon = iw;
        if (nw > maxName)
            maxName = nw;
        if (pw > maxParam)
            maxParam = pw;
    }

    maxName = min(maxName, (int)(MenuMaximalWidth ? MenuMaximalWidth
                                 : desktop->width() * 2/3));

    hspace -= 4 + r + maxIcon + l + left + padx + 2 + maxParam + 6 + 2;
    hspace = max(hspace, (int) desktop->width() / 3);

    // !!! not correct, to be fixed
    if (maxName > hspace)
        maxName = hspace;

    namePos = l + left + padx + maxIcon + 2;
    paramPos = namePos + 2 + maxName + 6;
    width = paramPos + maxParam + 4 + r;
    height += b;

#ifdef CONFIG_GRADIENTS
    if (menubackPixbuf && !(fGradient &&
    			    fGradient->width() == width &&
			    fGradient->height() == height)) {
	delete fGradient;
	fGradient = new YPixbuf(*menubackPixbuf, width, height);
    }
#endif

    size(width, height);
}

void YMenu::paintItems() {
    Graphics &g = graphics();
    int l, t, r, b;
    offsets(l, t, r, b);

    for (unsigned i(0); i < itemCount(); ++i)
        paintItem(g, i, l, t, r, 0, height(), i == fSelectedItem ||
                                              i == fPaintedItem);

    fPaintedItem = fSelectedItem;
}

void YMenu::drawBackground(Graphics &g, int x, int y, int w, int h) {
#ifdef CONFIG_GRADIENTS
    if (fGradient)
	g.copyPixbuf(*fGradient, x, y, w, h, x, y);
    else 
#endif    
    if (menubackPixmap)
	g.fillPixmap(menubackPixmap, x, y, w, h);
    else
	g.fillRect(x, y, w, h);
}

void YMenu::drawSeparator(Graphics &g, int x, int y, int w) {
    g.color(menuBg);
    
#ifdef CONFIG_GRADIENTS
    if (menusepPixbuf) {
    	drawBackground(g, x, y, w, 2 - menusepPixmap->height()/2);

	g.drawGradient(*menusepPixbuf,
		       x, y + 2 - menusepPixmap->height()/2,
		       w, menusepPixmap->height());

	drawBackground(g, x, y + 2 + (menusepPixmap->height()+1)/2,
		       w, 2 - (menusepPixmap->height()+1)/2);
    } else
#endif    
    if (menusepPixmap) {
    	drawBackground(g, x, y, w, 2 - menusepPixmap->height()/2);

	g.fillPixmap(menusepPixmap,
		     x, y + 2 - menusepPixmap->height()/2,
		     w, menusepPixmap->height());

	drawBackground(g, x, y + 2 + (menusepPixmap->height()+1)/2,
		       w, 2 - (menusepPixmap->height()+1)/2);
    } else if (wmLook == lookMetal) {
	drawBackground(g, x, y + 0, w, 1);

	if (activeMenuItemBg)
            g.color(activeMenuItemBg);

        g.drawLine(x, y + 1, w, y + 1);;
        g.color(menuBg->brighter());
        g.drawLine(x, y + 2, w, y + 2);;
        g.drawLine(x, y, x, y + 2);
    } else {
	drawBackground(g, x, y + 0, w, 1);

        g.color(menuBg->darker());
        g.drawLine(x, y + 1, w, y + 1);
        g.color(menuBg->brighter());
        g.drawLine(x, y + 2, w, y + 2);
        g.color(menuBg);

	drawBackground(g, x, y + 3, w, 1);
    }
}

void YMenu::paintItem(Graphics &g, unsigned i, int &l, int &t, int &r,
                                   int minY, int maxY, bool paint) {
    int const fontHeight(menuFont->height() + 1);
    int const fontBaseLine(menuFont->ascent());

    YMenuItem *mitem = item(i);
    const char *name = mitem->name();
    const char *param = mitem->param();
    

    if (mitem->name() == 0 && mitem->submenu() == 0) {
        if (t + 4 >= minY && t <= maxY)
            if (paint) drawSeparator(g, 1, t, width() - 2);

        t += (wmLook == lookMetal) ? 3 : 4;
    } else {
        bool const active(i == fSelectedItem && 
		         (mitem->action () || mitem->submenu()));

        int eh, top, bottom, pad, ih;
        eh = itemHeight(i, top, bottom, pad);
        ih = eh - top - bottom - pad - pad;

        if (t + eh >= minY && t <= maxY) {

	int const cascadePos(width() - r - 2 - ih - pad);

        g.color(active && activeMenuItemBg ? activeMenuItemBg : menuBg);

        if (paint) {
            if (active) {
#ifdef CONFIG_GRADIENTS
		if (menuselPixbuf)
		    g.drawGradient(*menuselPixbuf, l, t, width() - r - l, eh);
		else 
#endif
		if (menuselPixmap)
		    g.fillPixmap(menuselPixmap, l, t, width() - r - l, eh);
		else if (activeMenuItemBg) {
		    g.color(activeMenuItemBg);
                    g.fillRect(l, t, width() - r - l, eh);
		} else {
		    g.color(menuBg);
		    drawBackground(g, l, t, width() - r - l, eh);
		}
            } else {
		g.color(menuBg);
		drawBackground(g, l, t, width() - r - l, eh);
	    }

            if (wmLook == lookMetal && i != fSelectedItem) {
                g.color(menuBg->brighter());
                g.drawLine(1, t, 1, t + eh - 1);
                g.color(menuBg);
            }

            if (wmLook != lookWin95 && wmLook != lookWarp4 && active) {
                if (wmLook == lookGtk) {
                    g.color(activeMenuItemBg);
                    g.drawBorderW(l, t, width() - r - l - 1, eh - 1, true);
                } else if (wmLook == lookMetal) {
                    g.color((activeMenuItemBg ? activeMenuItemBg
		    				 : menuBg)->darker());
                    g.drawLine(l, t, width() - r - l, t);
                    g.color((activeMenuItemBg ? activeMenuItemBg
		    				 : menuBg)->brighter());
                    g.drawLine(l, t + eh - 1, width() - r - l, t + eh - 1);
                } else {
                    bool const raised(CONFIG_RAISE_MENU_ITEM);

                    g.color(activeMenuItemBg && raised ? activeMenuItemBg
                                                          : menuBg);
                    g.draw3DRect(l, t, width() - r - l - 1, eh - 1, raised);
                    
                    if (wmLook == lookMotif)
                        g.draw3DRect(l + 1, t + 1, width() - r - l - 3, eh - 3,
                                     raised);
                }
            }

            YColor *fg(mitem->isEnabled() ? active ? activeMenuItemFg
						   : menuItemFg
					  : disabledMenuItemFg);
            g.color(fg);
            g.font(menuFont);

            int delta = (active) ? 1 : 0;
            if (wmLook == lookMotif || wmLook == lookGtk ||
                wmLook == lookWarp4 || wmLook == lookWin95 ||
                wmLook == lookMetal)
                delta = 0;
            int baseLine = t + top + pad + (ih - fontHeight) / 2 + fontBaseLine + delta;
                //1 + 1 + t + (eh - fontHeight) / 2 + fontBaseLine + delta;

            if (mitem->isChecked()) {
                XPoint points[4];

                points[0].x = l + 3 + (16 - 10) / 2;
                points[1].x = 5;
                points[2].x = 5;
                points[3].x = -5;
                points[0].y = t + top + pad + ih / 2;
                points[1].y = -5;
                points[2].y = 5;
                points[3].y = 5;

                g.fillPolygon(points, 4, Convex, CoordModePrevious);
            } else if (mitem->icon()) {
		g.drawImage(mitem->icon(),
		    l + 1 + delta, t + delta + top + pad +
		    (eh - top - pad * 2 - bottom - 
		    mitem->icon()->height()) / 2);
            }

            if (name) {
		int const maxWidth((param ? paramPos - delta :
				    mitem->submenu() != NULL ? cascadePos :
				    width()) - namePos);

                if (!mitem->isEnabled()) {
                    g.color(disabledMenuItemSt);
		    g.drawStringEllipsis(1 + delta + namePos, 1 + baseLine, 
					 name, maxWidth);

                    if (mitem->hotCharPos() != -1)
                        g.drawCharUnderline(1 + delta +  namePos, 1 + baseLine,
                                            name, mitem->hotCharPos());
                }
                g.color(fg);
                g.drawStringEllipsis(delta + namePos, baseLine, name, maxWidth);

                if (mitem->hotCharPos() != -1)
                    g.drawCharUnderline(delta + namePos, baseLine,
                                        name, mitem->hotCharPos());
            }

            if (param) {
                if (!mitem->isEnabled()) {
                    g.color(disabledMenuItemSt);
                    g.drawChars(param, 0, strlen(param),
                                paramPos + delta + 1,
                                baseLine + 1);
                }
                g.color(fg);
                g.drawChars(param, 0, strlen(param),
                            paramPos + delta,
                            baseLine);
            }
            else if (mitem->submenu() != 0) {
//                int active = ((mitem->action() == 0 && i == fSelectedItem) ||
//                              fPopup == mitem->submenu()) ? 1 : 0;
                if (mitem->action()) {
                    if (0) {
			drawBackground(g, width() - r - 1 -ih - pad, t + top + pad, ih, ih);
                        g.drawBorderW(width() - r - 1 - ih - pad, t + top + pad,
                                      ih - 1, ih - 1, !active);
                    } else {
                        YColor *bgColor(activeMenuItemBg && active
                            ? activeMenuItemBg : menuBg);

                        g.color(bgColor->darker());
                        g.drawLine(cascadePos, t + top + pad,
                                   cascadePos, t + top + pad + ih);
                        g.color(bgColor->brighter());
                        g.drawLine(cascadePos + 1, t + top + pad,
                                   cascadePos + 1, t + top + pad + ih);

                    }
                    delta = delta ? active ? 1 : 0 : 0;
                }

                if (wmLook == lookGtk || wmLook == lookMotif) {
                    int const asize(9);
                    int const ax(delta + width() - r - 1 - asize * 3 / 2);
                    int const ay(delta + t + top + pad + (ih - asize) / 2);

                    g.color(activeMenuItemBg && active ? activeMenuItemBg
                                                          : menuBg);
                    g.drawArrow(Right, ax, ay, asize, active);
                } else {
                    int asize(9);
                    int const ax(width() - r - 1 - asize);
                    int const ay(delta + t + top + pad + (ih - asize) / 2);

                    g.color(fg);
		    
		    if (wmLook == lookWarp3) {
			wmLook = lookNice;
			g.drawArrow(Right, ax, ay, asize);
			wmLook = lookWarp3;
		    } else
			g.drawArrow(Right, ax, ay, asize);
                }

            }
        }
        }
        t += eh;
    }
}

void YMenu::paint(Graphics &g, int /*_x*/, int _y, unsigned /*_width*/, unsigned _height) {
    if (wmLook == lookMetal) {
        g.color(activeMenuItemBg ? activeMenuItemBg : menuBg);
        g.drawLine(0, 0, width() - 1, 0);
        g.drawLine(0, 0, 0, height() - 1);
        g.drawLine(width() - 1, 0, width() - 1, height() - 1);
        g.drawLine(0, height() - 1, width() - 1, height() - 1);
        g.color(menuBg->brighter());
        g.drawLine(1, 1, width() - 2, 1);
        g.color(menuBg);
        g.drawLine(1, height() - 2, width() - 2, height() - 2);
    } else {
        g.color(menuBg);
        g.drawBorderW(0, 0, width() - 1, height() - 1, true);
        g.drawLine(1, 1, width() - 3, 1);
        g.drawLine(1, 1, 1, height() - 3);
        g.drawLine(1, height() - 3, width() - 3, height() - 3);
        g.drawLine(width() - 3, 1, width() - 3, height() - 3);
    }

    int l, t, r, b;
    offsets(l, t, r, b);

    for (unsigned i(0); i < itemCount(); ++i)
        paintItem(g, i, l, t, r, _y, _y + _height, 1);

    fPaintedItem = fSelectedItem;
}
