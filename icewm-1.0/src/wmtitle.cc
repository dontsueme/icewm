/*
 * IceWM
 *
 * Copyright (C) 1997-2001 Marko Macek
 */
#include "config.h"
#include "ylib.h"
#include "ypixbuf.h"
#include "wmtitle.h"

#include "wmframe.h"
#include "wmwinlist.h"
#include "wmapp.h"

#include <string.h>

static YFont *titleFont = 0;
YColor *activeTitleBarBg = 0;
YColor *activeTitleBarFg = 0;
YColor *activeTitleBarSt = 0;

YColor *inactiveTitleBarBg = 0;
YColor *inactiveTitleBarFg = 0;
YColor *inactiveTitleBarSt = 0;

#ifdef CONFIG_LOOK_PIXMAP
YPixmap *titleJ[2] = { 0, 0 }; // Frame <=> Left buttons
YPixmap *titleL[2] = { 0, 0 }; // Left buttons <=> Left pane
YPixmap *titleS[2] = { 0, 0 }; // Left pane
YPixmap *titleP[2] = { 0, 0 }; // Left pane <=> Title
YPixmap *titleT[2] = { 0, 0 }; // Title
YPixmap *titleM[2] = { 0, 0 }; // Title <=> Right pane
YPixmap *titleB[2] = { 0, 0 }; // Right pane
YPixmap *titleR[2] = { 0, 0 }; // Right pane <=> Right buttons
YPixmap *titleQ[2] = { 0, 0 }; // Right buttons <=> Frame
#endif

#ifdef CONFIG_GRADIENTS
YPixbuf *rgbTitleS[2] = { 0, 0 };
YPixbuf *rgbTitleT[2] = { 0, 0 };
YPixbuf *rgbTitleB[2] = { 0, 0 };
#endif

YFrameTitleBar::YFrameTitleBar(YWindow *parent, YFrameWindow *frame):
    YWindow(parent)
{
    if (titleFont == 0)
        titleFont = YFont::font(titleFontName);

    if (activeTitleBarBg == 0)
        activeTitleBarBg = new YColor(clrActiveTitleBar);
    if (activeTitleBarFg == 0)
        activeTitleBarFg = new YColor(clrActiveTitleBarText);
    if (activeTitleBarSt == 0 && *clrActiveTitleBarShadow)
        activeTitleBarSt = new YColor(clrActiveTitleBarShadow);

    if (inactiveTitleBarBg == 0)
        inactiveTitleBarBg = new YColor(clrInactiveTitleBar);
    if (inactiveTitleBarFg == 0)
        inactiveTitleBarFg = new YColor(clrInactiveTitleBarText);
    if (inactiveTitleBarSt == 0 && *clrInactiveTitleBarShadow)
        inactiveTitleBarSt = new YColor(clrInactiveTitleBarShadow);

    movingWindow = 0; fFrame = frame;
    
    toolTip(this->frame()->client()->windowTitle());
}

YFrameTitleBar::~YFrameTitleBar() {
}

void YFrameTitleBar::handleButton(const XButtonEvent &button) {
    if (button.type == ButtonPress) {
        if ((buttonRaiseMask & (1 << (button.button - 1))) &&
           !(button.state & (app->AltMask | ControlMask | app->ButtonMask))) {
            frame()->activate();
            if (raiseOnClickTitleBar)
                frame()->wmRaise();
        }
    }
#ifdef CONFIG_WINLIST
    else if (button.type == ButtonRelease) {
        if (button.button == 1 &&
            IS_BUTTON(BUTTON_MODMASK(button.state), Button1Mask + Button3Mask))
        {
            if (windowList)
                windowList->showFocused(button.x_root, button.y_root);
        }
    }
#endif
    YWindow::handleButton(button);
}

void YFrameTitleBar::handleMotion(const XMotionEvent &motion) {
    YWindow::handleMotion(motion);
}

void YFrameTitleBar::handleClick(const XButtonEvent &up, int count) {
    if (count >= 2 && (count % 2 == 0)) {
        if (up.button == (unsigned) titleMaximizeButton &&
            ISMASK(KEY_MODMASK(up.state), 0, ControlMask))
        {
            if (frame()->canMaximize())
                frame()->wmMaximize();
        } else if (up.button == (unsigned) titleMaximizeButton &&
                   ISMASK(KEY_MODMASK(up.state), ShiftMask, ControlMask))
        {
            if (frame()->canMaximize())
                frame()->wmMaximizeVert();
        } else if (up.button == (unsigned) titleMaximizeButton && app->AltMask &&
                   ISMASK(KEY_MODMASK(up.state), app->AltMask + ShiftMask, ControlMask))
        {
            if (frame()->canMaximize())
                frame()->wmMaximizeHorz();
        } else if (up.button == (unsigned) titleRollupButton &&
                 ISMASK(KEY_MODMASK(up.state), 0, ControlMask))
        {
            if (frame()->canRollup())
                frame()->wmRollup();
        } else if (up.button == (unsigned) titleRollupButton &&
                   ISMASK(KEY_MODMASK(up.state), ShiftMask, ControlMask))
        {
            if (frame()->canMaximize())
                frame()->wmMaximizeHorz();
        }
    } else if (count == 1) {
        if (up.button == 3 && (KEY_MODMASK(up.state) & (app->AltMask)) == 0) {
            frame()->popupSystemMenu(up.x_root, up.y_root, -1, -1,
                                        YPopupWindow::pfCanFlipVertical |
                                        YPopupWindow::pfCanFlipHorizontal);
        } else if (up.button == 1) {
            if (KEY_MODMASK(up.state) == app->AltMask) {
                if (frame()->canLower()) frame()->wmLower();
            } else if (lowerOnClickWhenRaised &&
                       (buttonRaiseMask & (1 << (up.button - 1))) &&
                       (up.state & (ControlMask | app->ButtonMask)) == 
                        Button1Mask) {
                static YFrameWindow * raised(NULL);

                if (frame() == raised) {
                        if (raised->canLower()) raised->wmLower();
                        raised = NULL;
                } else {
                        raised = frame();
                }
            }
        }
    }
}

void YFrameTitleBar::handleBeginDrag(const XButtonEvent &down, const XMotionEvent &/*motion*/) {
    if (frame()->canMove()) {
        frame()->startMoveSize(1, 1, 0, 0, down.x + x(), down.y + y());
    }
}

void YFrameTitleBar::activate() {
    repaint();
#ifdef CONFIG_LOOK_WIN95
    if (wmLook == lookWin95 && frame()->menuButton())
        frame()->menuButton()->repaint();
#endif
#ifdef CONFIG_LOOK_PIXMAP
    if (wmLook == lookPixmap || wmLook == lookMetal || wmLook == lookGtk) {
        if (frame()->menuButton()) frame()->menuButton()->repaint();
        if (frame()->closeButton()) frame()->closeButton()->repaint();
        if (frame()->maximizeButton()) frame()->maximizeButton()->repaint();
        if (frame()->minimizeButton()) frame()->minimizeButton()->repaint();
        if (frame()->hideButton()) frame()->hideButton()->repaint();
        if (frame()->rollupButton()) frame()->rollupButton()->repaint();
        if (frame()->depthButton()) frame()->depthButton()->repaint();
    }
#endif
}

void YFrameTitleBar::deactivate() {
    activate(); // for now
}

int YFrameTitleBar::titleLen() {
    const char *title = frame()->client()->windowTitle();
    int tlen = title ? titleFont->textWidth(title) : 0;
    return tlen;
}

void YFrameTitleBar::paint(Graphics &g, int , int , unsigned int , unsigned int ) {
    if (frame()->client() == NULL) return;

    YColor *bg = frame()->focused() ? activeTitleBarBg : inactiveTitleBarBg;
    YColor *fg = frame()->focused() ? activeTitleBarFg : inactiveTitleBarFg;
    YColor *st = frame()->focused() ? activeTitleBarSt : inactiveTitleBarSt;

    int onLeft(0);
    int onRight(width());

    if (titleButtonsLeft)
        for (const char *bc = titleButtonsLeft; *bc; bc++) {
            YWindow const *b(frame()->button(*bc));
            if (b) onLeft = max(onLeft, (int)(b->x() + b->width()));
        }

    if (titleButtonsRight)
        for (const char *bc = titleButtonsRight; *bc; bc++) {
            YWindow const *b(frame()->button(*bc));
            if (b) onRight = min(onRight, b->x());
        }
    
    g.font(titleFont);

    char const *title(frame()->title());
    int const yPos((height() - titleFont->height()) / 2 + 
    		   titleFont->ascent() + titleBarVertOffset);
    int tlen(title ? titleFont->textWidth(title) : 0);

    int stringOffset(onLeft + (onRight - onLeft - tlen)
    			    * (int) titleBarJustify / 100);
    g.color(bg);
    switch (wmLook) {
#ifdef CONFIG_LOOK_WIN95
    case lookWin95:
#endif
#ifdef CONFIG_LOOK_NICE
    case lookNice:
#endif
        g.fillRect(0, 0, width(), height());
        break;
#ifdef CONFIG_LOOK_WARP3
    case lookWarp3:
        {
#ifdef TITLEBAR_BOTTOM
            int y = 1;
            int y2 = 0;
#else
            int y = 0;
            int y2 = height() - 1;
#endif

            g.fillRect(0, y, width(), height() - 1);
            g.color(frame()->focused() ? fg->darker() : bg->darker());
            g.drawLine(0, y2, width(), y2);
        }
        break;
#endif
#ifdef CONFIG_LOOK_WARP4
    case lookWarp4:
	if (titleBarJustify == 0)
	    stringOffset++;
	else if (titleBarJustify == 100)
	    stringOffset--;

        if (frame()->focused()) {
            g.fillRect(1, 1, width() - 2, height() - 2);
            g.color(inactiveTitleBarBg);
            g.draw3DRect(onLeft, 0, onRight - 1, height() - 1, false);
        } else {
            g.fillRect(0, 0, width(), height());
        }
        break;
#endif
#ifdef CONFIG_LOOK_MOTIF
    case lookMotif:
	if (titleBarJustify == 0)
	    stringOffset++;
	else if (titleBarJustify == 100)
	    stringOffset--;

        g.fillRect(1, 1, width() - 2, height() - 2);
        g.draw3DRect(onLeft, 0, onRight - 1 - onLeft, height() - 1, true);
        break;
#endif
#ifdef CONFIG_LOOK_PIXMAP
    case lookPixmap:
    case lookMetal:
    case lookGtk: {
	int const pi(frame()->focused() ? 1 : 0);

// !!! we really need a fallback mechanism for small windows
	if (titleL[pi]) {
	    g.drawPixmap(titleL[pi], onLeft, 0);
	    onLeft+= titleL[pi]->width();
	}
	
	if (titleR[pi]) {
	    onRight-= titleR[pi]->width();
	    g.drawPixmap(titleR[pi], onRight, 0);
	}

	int lLeft(onLeft + (titleP[pi] ? (int)titleP[pi]->width() : 0)),
	    lRight(onRight - (titleM[pi] ? (int)titleM[pi]->width() : 0));

	tlen = clamp(lRight - lLeft, 0, tlen);
	stringOffset = lLeft + (lRight - lLeft - tlen)
			      * (int) titleBarJustify / 100;

	lLeft = stringOffset;
	lRight = stringOffset + tlen;

	if (lLeft < lRight) {
#ifdef CONFIG_GRADIENTS	
	    if (rgbTitleT[pi]) {
		int const gx(titleBarJoinLeft ? lLeft - onLeft : 0);
	        int const gw((titleBarJoinRight ? onRight : lRight) -
			     (titleBarJoinLeft ? onLeft : lLeft));
		g.drawGradient(*rgbTitleT[pi], lLeft, 0,
			       lRight - lLeft, height(), gx, 0, gw, height());
	    } else 
#endif	    
	    if (titleT[pi])
		g.repHorz(titleT[pi], lLeft, 0, lRight - lLeft);
	    else
		g.fillRect(lLeft, 0, lRight - lLeft, height());
	}

	if (titleP[pi]) {
	    lLeft-= titleP[pi]->width();
	    g.drawPixmap(titleP[pi], lLeft, 0);
	}
	if (titleM[pi]) {
	    g.drawPixmap(titleM[pi], lRight, 0);
	    lRight+= titleM[pi]->width();
	}
	
	if (onLeft < lLeft) {
#ifdef CONFIG_GRADIENTS	
	    if (rgbTitleS[pi]) {
	        int const gw((titleBarJoinLeft ? titleBarJoinRight ? 
			      onRight : lRight : lLeft) - onLeft);
		g.drawGradient(*rgbTitleS[pi], onLeft, 0,
			       lLeft - onLeft, height(), 0, 0, gw, height());
	    } else
#endif	    
	    if (titleS[pi])
		g.repHorz(titleS[pi], onLeft, 0, lLeft - onLeft);
	    else
		g.fillRect(onLeft, 0, lLeft - onLeft, height());
	}
	if (lRight < onRight) {
#ifdef CONFIG_GRADIENTS	
	    if (rgbTitleB[pi]) {
		int const gx(titleBarJoinRight ? titleBarJoinLeft ?
		     lRight - onLeft: lRight - lLeft : 0);
	        int const gw(titleBarJoinRight ? titleBarJoinLeft ?
		    onRight - onLeft : onRight - lLeft : onRight - lRight);

		g.drawGradient(*rgbTitleB[pi], lRight, 0,
			       onRight - lRight, height(), gx, 0, gw, height());
	    } else
#endif	    
	    if (titleB[pi])
		g.repHorz(titleB[pi], lRight, 0, onRight - lRight);
	    else
		g.fillRect(lRight, 0, onRight - lRight, height());
	}

	if (titleJ[pi])
	    g.drawPixmap(titleJ[pi], 0, 0);
	if (titleQ[pi])
	    g.drawPixmap(titleQ[pi], width() - titleQ[pi]->width(), 0);

        break;
    }
#endif
    default:
        break;
    }

    if (title && tlen) {
	stringOffset+= titleBarHorzOffset;

	if (st) {
	    g.color(st);
	    g.drawStringEllipsis(stringOffset + 1, yPos + 1, title, tlen);
	}

	g.color(fg);
        g.drawStringEllipsis(stringOffset, yPos, title, tlen);
    }
}

#ifdef CONFIG_SHAPED_DECORATION
void YFrameTitleBar::renderShape(Pixmap shape) {
#ifdef CONFIG_LOOK_PIXMAP
    if (wmLook == lookPixmap || wmLook == lookMetal || wmLook == lookGtk) {
	Graphics g(shape);
    
	int onLeft(0);
	int onRight(width());

	if (titleButtonsLeft)
	    for (const char *bc = titleButtonsLeft; *bc; bc++) {
		YFrameButton const *b(frame()->button(*bc));
		if (b) {
		    onLeft = max(onLeft, (int)(b->x() + b->width()));

		    YPixmap const *pixmap(b->image(0));
		    if (pixmap) g.copyDrawable(pixmap->mask(), 0, 0,
			b->width(), b->height(), x() + b->x(), y() + b->y());
		}
	    }

	if (titleButtonsRight)
            for (const char *bc = titleButtonsRight; *bc; bc++) {
		YFrameButton const *b(frame()->button(*bc));
		if (b) {
		    onRight = min(onRight, b->x());

		    YPixmap const *pixmap(b->image(0));
		    if (pixmap) g.copyDrawable(pixmap->mask(), 0, 0,
			b->width(), b->height(), x() + b->x(), y() + b->y());
		}
            }
	    
	onLeft+= x();
	onRight+= x();
    
	char const *title(frame()->title());
	int tlen(title ? titleFont->textWidth(title) : 0);
	int stringOffset(onLeft + (onRight - onLeft - tlen)
    			        * (int) titleBarJustify / 100);

	int const pi(frame()->focused() ? 1 : 0);

	if (titleL[pi]) {
	    g.drawMask(titleL[pi], onLeft, y());
	    onLeft+= titleL[pi]->width();
	}
	
	if (titleR[pi]) {
	    onRight-= titleR[pi]->width();
	    g.drawMask(titleR[pi], onRight, y());
	}

	int lLeft(onLeft + (titleP[pi] ? (int)titleP[pi]->width() : 0)),
	    lRight(onRight - (titleM[pi] ? (int)titleM[pi]->width() : 0));

	tlen = clamp(lRight - lLeft, 0, tlen);
	stringOffset = lLeft + (lRight - lLeft - tlen)
			      * (int) titleBarJustify / 100;

	lLeft = stringOffset;
	lRight = stringOffset + tlen;

	if (lLeft < lRight && titleT[pi])
	    g.repHorz(titleT[pi]->mask(),
		      titleT[pi]->width(), titleT[pi]->height(),
	    	      lLeft, y(), lRight - lLeft);

	if (titleP[pi]) {
	    lLeft-= titleP[pi]->width();
	    g.drawMask(titleP[pi], lLeft, y());
	}
	if (titleM[pi]) {
	    g.drawMask(titleM[pi], lRight, y());
	    lRight+= titleM[pi]->width();
	}
	
	if (onLeft < lLeft && titleS[pi])
	    g.repHorz(titleS[pi]->mask(),
		      titleS[pi]->width(), titleS[pi]->height(),
	    	      onLeft, y(), lLeft - onLeft);

	if (lRight < onRight && titleB[pi])
	    g.repHorz(titleB[pi]->mask(), 
	    	      titleB[pi]->width(), titleB[pi]->height(),
		      lRight, y(), onRight - lRight);

	if (titleJ[pi])
	    g.drawMask(titleJ[pi], x(), y());
	if (titleQ[pi])
	    g.drawMask(titleQ[pi], x() + width() - titleQ[pi]->width(), y());
    }
#endif
}
#endif

