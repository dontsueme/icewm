/*
 * IceWM
 *
 * Copyright (C) 1997-2001 Marko Macek
 */
#include "config.h"
#include "yfull.h"
#include "wmframe.h"

#include "wmaction.h"
#include "wmtitle.h"
#include "wmapp.h"
#include "wmclient.h"
#include "wmcontainer.h"
#include "ymenuitem.h"

#ifdef CONFIG_LOOK_PIXMAP
YPixmap *frameTL[2][2] = {{ 0, 0 }, { 0, 0 }};
YPixmap *frameT[2][2] = {{ 0, 0 }, { 0, 0 }};
YPixmap *frameTR[2][2] = {{ 0, 0 }, { 0, 0 }};
YPixmap *frameL[2][2] = {{ 0, 0 }, { 0, 0 }};
YPixmap *frameR[2][2] = {{ 0, 0 }, { 0, 0 }};
YPixmap *frameBL[2][2] = {{ 0, 0 }, { 0, 0 }};
YPixmap *frameB[2][2] = {{ 0, 0 }, { 0, 0 }};
YPixmap *frameBR[2][2] = {{ 0, 0 }, { 0, 0 }};
#endif

#ifdef CONFIG_GRADIENTS
YPixbuf *rgbFrameT[2][2] = {{ 0, 0 }, { 0, 0 }};
YPixbuf *rgbFrameL[2][2] = {{ 0, 0 }, { 0, 0 }};
YPixbuf *rgbFrameR[2][2] = {{ 0, 0 }, { 0, 0 }};
YPixbuf *rgbFrameB[2][2] = {{ 0, 0 }, { 0, 0 }};
#endif

void YFrameWindow::updateMenu() {
    YMenu *windowMenu = this->windowMenu();
    // enable all commands
    windowMenu->actionListener(this);
    windowMenu->enableCommand(0);

    if (isMaximized())
        windowMenu->disableCommand(actionMaximize);
    if (isMinimized())
        windowMenu->disableCommand(actionMinimize);
    if (!(isMaximized() || isMinimized() || isHidden()))
        windowMenu->disableCommand(actionRestore);
    if (isMinimized() || isHidden() || isRollup() || !visibleNow())
        windowMenu->disableCommand(actionSize);
    if (isMinimized() || isHidden() || !visibleNow())
        windowMenu->disableCommand(actionMove);
    if (!canLower())
        windowMenu->disableCommand(actionLower);
    if (!canRaise())
        windowMenu->disableCommand(actionRaise);

    unsigned long func = frameFunctions();

    if (!(func & ffMove))
        windowMenu->disableCommand(actionMove);
    if (!(func & ffResize))
        windowMenu->disableCommand(actionSize);
    if (!(func & ffMinimize))
        windowMenu->disableCommand(actionMinimize);
#ifndef	CONFIG_PDA
    if (!(func & ffHide))
        windowMenu->disableCommand(actionHide);
#endif
    if (!(func & ffRollup) || !titlebar()->visible())
        windowMenu->disableCommand(actionRollup);
    if (!(func & ffMaximize))
        windowMenu->disableCommand(actionMaximize);
    if (!(func & ffClose))
        windowMenu->disableCommand(actionClose);

    YMenuItem *item;

    if ((item = windowMenu->findAction(actionRollup)))
        item->checked(isRollup());
    if ((item = windowMenu->findAction(actionOccupyAllOrCurrent)))
        item->checked(isSticky());
    if ((item = windowMenu->findAction(actionDontCover)))
        item->checked(dontCover());
    if ((item = windowMenu->findSubmenu(moveMenu)))
        item->enabled(!isSticky());

    for (unsigned i(0); i < moveMenu->itemCount(); ++i) {
        item = moveMenu->item(i);
        for (icewm::Workspace w(0); w < workspaceCount; ++w)
            if (item && item->action() == workspaceActionMoveTo[w])
                item->enabled(w != workspace());
    }

    for (unsigned j(0); j < layerMenu->itemCount(); ++j) {
        item = layerMenu->item(j);
        for (icewm::Layer layer(0); layer < WinLayerCount; ++layer)
            if (item && item->action() == layerActionSet[layer]) {
                bool const e(layer == this->layer());
                item->enabled(!e);
                item->checked(e);
            }
    }

#ifdef CONFIG_TRAY
    if (trayMenu)  {
        bool const withTrayMenu(true);
warn("figure out if we have a tray window");

        if ((item = windowMenu->findSubmenu(trayMenu)))
            item->enabled(withTrayMenu);

        if (withTrayMenu)
            for (unsigned o(0); o < trayMenu->itemCount(); ++o) {
                item = trayMenu->item(o);
                for (icewm::TrayOption option(0);
                     option < IcewmTrayOptionCount; ++option)
                    if (item && item->action() == trayOptionActionSet[option]) {
                        bool const e(option == trayOption());
                        item->enabled(!e);
                        item->checked(e);
                    }
            }
    }
#endif    
}

#ifdef CONFIG_SHAPE
void YFrameWindow::setShape() {
    if (!shapesSupported) return;

    if (client()->shaped()) {
        MSG(("setting shape w=%d, h=%d", width(), height()));
        if (isRollup() || isIconic()) {
            XRectangle full;
            full.x = 0;
            full.y = 0;
            full.width = width();
            full.height = height();
            XShapeCombineRectangles(app->display(), handle(),
                                    ShapeBounding,
                                    0, 0, &full, 1,
                                    ShapeSet, Unsorted);
        } else {
            XRectangle rect[6];
            int nrect = 0;

            if ((frameDecorations() & (fdResize | fdBorder)) == 
                                       fdResize | fdBorder) {
                rect[0].x = 0;
                rect[0].y = 0;
                rect[0].width = width();
                rect[0].height = borderY();

                rect[1] = rect[0];
                rect[1].y = height() - borderY();

                rect[2].x = 0;
                rect[2].y = borderY();
                rect[2].width = borderX();
                rect[2].height = height() - 2 * borderY();

                rect[3] = rect[2];
                rect[3].x = width() - borderX();

                nrect = 4;
            }

            if (titleY() > 0) {
                rect[nrect].x = borderX();
#ifdef TITLEBAR_BOTTOM
                rect[nrect].y = height() - borderY() - titleY();
#else
                rect[nrect].y = borderY();
#endif
                rect[nrect].width  = width() - 2 * borderX();
                rect[nrect].height = titleY();
                nrect++;
            }

            if (nrect !=  0)
                XShapeCombineRectangles(app->display(), handle(),
                                        ShapeBounding,
                                        0, 0, rect, nrect,
                                        ShapeSet, Unsorted);
            XShapeCombineShape(app->display(), handle(),
                               ShapeBounding,
                               borderX(),
                               borderY()
#ifndef TITLEBAR_BOTTOM
                               + titleY()
#endif
                               ,
                               client()->handle(),
                               ShapeBounding, nrect ? ShapeUnion : ShapeSet);
        }
    }
}
#endif

void YFrameWindow::layoutShape() {
#ifdef CONFIG_SHAPED_DECORATION
    if (shapesSupported && (frameDecorations() & fdBorder))
	if(isIconic())
	    XShapeCombineMask(app->display(), handle(),
		              ShapeBounding, 0, 0, None, ShapeSet);
	else {
	    int const a(focused() ? 1 : 0);
	    int const t((frameDecorations() & fdResize) ? 0 : 1);

	    Pixmap shape(YPixmap::createMask(width(), height()));
	    Graphics g(shape);

	    g.color(YColor::white);
	    g.fillRect(0, 0, width(), height());

	    const int xTL(frameTL[t][a] ? frameTL[t][a]->width() : 0),
    		      xTR(width() - 
		         (frameTR[t][a] ? frameTR[t][a]->width() : 0)),
    		      xBL(frameBL[t][a] ? frameBL[t][a]->width() : 0),
		      xBR(width() - 
		         (frameBR[t][a] ? frameBR[t][a]->width() : 0));
	    const int yTL(frameTL[t][a] ? frameTL[t][a]->height() : 0),
    		      yBL(height() -
		         (frameTR[t][a] ? frameTR[t][a]->height() : 0)),
    		      yTR(frameBL[t][a] ? frameBL[t][a]->height() : 0),
    		      yBR(height() -
		         (frameBR[t][a] ? frameBR[t][a]->height() : 0));

	    if (frameTL[t][a]) {
		g.copyDrawable(frameTL[t][a]->mask(), 0, 0,
			       frameTL[t][a]->width(), frameTL[t][a]->height(),
			       0, 0);
		if (protectClientWindow)
		    g.fillRect(borderX(), borderY(),
			       frameTL[t][a]->width() - borderX(),
			       frameTL[t][a]->height() - borderY());
	    }
	    if (frameTR[t][a]) {
		g.copyDrawable(frameTR[t][a]->mask(), 0, 0,
			       frameTR[t][a]->width(), frameTR[t][a]->height(),
			       xTR, 0);
		if (protectClientWindow)
		    g.fillRect(xTR, borderY(),
			       frameTR[t][a]->width() - borderX(),
			       frameTR[t][a]->height() - borderY());
	    }
	    if (frameBL[t][a]) {
		g.copyDrawable(frameBL[t][a]->mask(), 0, 0,
			       frameBL[t][a]->width(), frameBL[t][a]->height(),
			       0, yBL);
		if (protectClientWindow)
		    g.fillRect(borderX(), yBL,
			       frameBL[t][a]->width() - borderX(),
			       frameBL[t][a]->height() - borderY());
	    }
	    if (frameBR[t][a]) {
		g.copyDrawable(frameBR[t][a]->mask(), 0, 0,
			       frameBR[t][a]->width(), frameBL[t][a]->height(),
			       xBR, yBR);
		if (protectClientWindow)
		    g.fillRect(xBR, yBR,
			       frameBR[t][a]->width() - borderX(),
			       frameBR[t][a]->width() - borderY());
	    }
	    
	    if (frameT[t][a])
		g.repHorz(frameT[t][a]->mask(), 
			  frameT[t][a]->width(), frameT[t][a]->height(),
			  xTL, 0, xTR - xTL);
	    if (frameB[t][a])
		g.repHorz(frameB[t][a]->mask(), 
			  frameB[t][a]->width(), frameB[t][a]->height(),
			  xBL, height() - frameB[t][a]->height(), xBR - xBL);
	    if (frameL[t][a])
		g.repVert(frameL[t][a]->mask(), 
			  frameL[t][a]->width(), frameL[t][a]->height(),
			  0, yTL, yBL - yTL);
	    if (frameR[t][a])
		g.repVert(frameR[t][a]->mask(), 
			  frameR[t][a]->width(), frameR[t][a]->height(),
			  width() - frameR[t][a]->width(), yTR, yBR - yTR);

	    if (titlebar() && titleY())
		titlebar()->renderShape(shape);
	    XShapeCombineMask(app->display(), handle(),
			      ShapeBounding, 0, 0, shape, ShapeSet);
	    XFreePixmap(app->display(), shape);
	}
#endif
}

void YFrameWindow::configure(const int x, const int y, 
			     const unsigned width, const unsigned height, 
			     const bool resized) {
    //int oldX = this->x();
    //int oldY= this->y();

    MSG(("configure %d %d %d %d", x, y, width, height));

#ifdef CONFIG_SHAPE
    unsigned int oldWidth = container()->width();
    unsigned int oldHeight = container()->height();
    int oldcx = container()->x();
    int oldcy = container()->y();
#endif

    YWindow::configure(x, y, width, height, resized);

    layoutTitleBar();
    layoutButtons();
    layoutResizeIndicators();
    if (resized) layoutShape();
    layoutClient();

    // ??? !!!
    //if (x != oldX || y != oldY)
    sendConfigure();

#ifdef CONFIG_SHAPE
    int cx = container()->x();
    int cy = container()->y();
    if (oldWidth != container()->width() ||
    	oldHeight != container()->height() ||
        cx != oldcx || cy != oldcy)
        setShape();
#endif

    if (dontCover())
	manager->updateWorkArea();
}

void YFrameWindow::layoutTitleBar() {
    if (titleY() == 0) {
        titlebar()->hide();
    } else {
        titlebar()->show();

        int title_width = width() - 2 * borderX();
        titlebar()->geometry(borderX(),
                             borderY()
#ifdef TITLEBAR_BOTTOM
                           + height() - titleY() - 2 * borderY()
#endif
                           , (title_width > 0) ? title_width : 1, titleY());
    }
}

YFrameButton *YFrameWindow::button(char c) {
    unsigned long const decorations(frameDecorations());

    switch (c) {
        case 's': return (decorations & fdSysMenu)  ? fMenuButton     : NULL;
        case 'x': return (decorations & fdClose)    ? fCloseButton    : NULL;
        case 'm': return (decorations & fdMaximize) ? fMaximizeButton : NULL;
        case 'i': return (decorations & fdMinimize) ? fMinimizeButton : NULL;
        case 'h': return (decorations & fdHide)     ? fHideButton     : NULL;
        case 'r': return (decorations & fdRollup)   ? fRollupButton   : NULL;
        case 'd': return (decorations & fdDepth)    ? fDepthButton    : NULL;
        default:  return NULL;
    }
}

void YFrameWindow::positionButton(YFrameButton *b, int &xPos, bool onRight) {
    /// !!! clean this up
    if (b == fMenuButton) {
	const unsigned bw(NULL == b->image(0) ? titleY() : b->image(0)->width());
        if (onRight) xPos -= bw;
        b->geometry(xPos, 0, bw, titleY());
        if (!onRight) xPos += bw;
    } else if (wmLook == lookPixmap || wmLook == lookMetal || wmLook == lookGtk) {
	const unsigned bw(b->image(0) ? b->image(0)->width() : titleY());

        if (onRight) xPos -= bw;
        b->geometry(xPos, 0, bw, titleY());
        if (!onRight) xPos += bw;
    } else if (wmLook == lookWin95) {
        if (onRight) xPos -= titleY();
        b->geometry(xPos, 2, titleY(), titleY() - 3);
        if (!onRight) xPos += titleY();
    } else {
        if (onRight) xPos -= titleY();
        b->geometry(xPos, 0, titleY(), titleY());
        if (!onRight) xPos += titleY();
    }
}

void YFrameWindow::layoutButtons() {
    if (titleY() == 0)
        return ;

    unsigned long decorations = frameDecorations();

    if (fMinimizeButton)
        if (decorations & fdMinimize)
            fMinimizeButton->show();
        else
            fMinimizeButton->hide();

    if (fMaximizeButton)
        if (decorations & fdMaximize)
            fMaximizeButton->show();
        else
            fMaximizeButton->hide();

    if (fRollupButton)
        if (decorations & fdRollup)
            fRollupButton->show();
        else
            fRollupButton->hide();

    if (fHideButton)
        if (decorations & fdHide)
            fHideButton->show();
        else
            fHideButton->hide();

    if (fCloseButton)
        if ((decorations & fdClose))
            fCloseButton->show();
        else
            fCloseButton->hide();

    if (fMenuButton)
        if (decorations & fdSysMenu)
            fMenuButton->show();
        else
            fMenuButton->hide();

    if (fDepthButton)
        if (decorations & fdDepth)
            fDepthButton->show();
        else
            fDepthButton->hide();

#ifdef CONFIG_LOOK_PIXMAP
	const int pi(focused() ? 1 : 0);
#endif

    if (titleButtonsLeft) {
#ifdef CONFIG_LOOK_PIXMAP
        int xPos(titleJ[pi] ? titleJ[pi]->width() : 0);
#else	
        int xPos(0);
#endif

        for (const char *bc = titleButtonsLeft; *bc; bc++) {
            YFrameButton *b = 0;

            switch (*bc) {
            case ' ':
                xPos++;
                b = 0;
                break;
            default:
                b = button(*bc);
                break;
            }

            if (b)
                positionButton(b, xPos, false);
        }
    }

    if (titleButtonsRight) {
#ifdef CONFIG_LOOK_PIXMAP
        int xPos(width() - 2 * borderX() - 
		(titleQ[pi] ? titleQ[pi]->width() : 0));
#else
        int xPos(width() - 2 * borderX());
#endif

        for (const char *bc = titleButtonsRight; *bc; bc++) {
            YFrameButton *b = 0;

            switch (*bc) {
            case ' ':
                xPos--;
                b = 0;
                break;
            default:
                b = button(*bc);
                break;
            }

            if (b)
                positionButton(b, xPos, true);
        }
    }
}

void YFrameWindow::layoutResizeIndicators() {
    if (((frameDecorations() & (fdResize | fdBorder)) == (fdResize | fdBorder)) &&
        !isRollup() && !isMinimized()) {
        if (!indicatorsVisible) {
            indicatorsVisible = 1;

            XMapWindow(app->display(), topSide);
            XMapWindow(app->display(), leftSide);
            XMapWindow(app->display(), rightSide);
            XMapWindow(app->display(), bottomSide);

            XMapWindow(app->display(), topLeftCorner);
            XMapWindow(app->display(), topRightCorner);
            XMapWindow(app->display(), bottomLeftCorner);
            XMapWindow(app->display(), bottomRightCorner);
        }
    } else {
        if (indicatorsVisible) {
            indicatorsVisible = 0;

            XUnmapWindow(app->display(), topSide);
            XUnmapWindow(app->display(), leftSide);
            XUnmapWindow(app->display(), rightSide);
            XUnmapWindow(app->display(), bottomSide);

            XUnmapWindow(app->display(), topLeftCorner);
            XUnmapWindow(app->display(), topRightCorner);
            XUnmapWindow(app->display(), bottomLeftCorner);
            XUnmapWindow(app->display(), bottomRightCorner);
        }
    }
    if (!indicatorsVisible)
        return;

    XMoveResizeWindow(app->display(), topSide, 0, 0, width(), borderY());
    XMoveResizeWindow(app->display(), leftSide, 0, 0, borderX(), height());
    XMoveResizeWindow(app->display(), rightSide, width() - borderX(), 0, borderY(), height());
    XMoveResizeWindow(app->display(), bottomSide, 0, height() - borderY(), width(), borderY());

    XMoveResizeWindow(app->display(), topLeftCorner, 0, 0, wsCornerX, wsCornerY);
    XMoveResizeWindow(app->display(), topRightCorner, width() - wsCornerX, 0, wsCornerX, wsCornerY);
    XMoveResizeWindow(app->display(), bottomLeftCorner, 0, height() - wsCornerY, wsCornerX, wsCornerY);
    XMoveResizeWindow(app->display(), bottomRightCorner, width() - wsCornerX, height() - wsCornerY, wsCornerX, wsCornerY);
}

void YFrameWindow::layoutClient() {
    if (!isRollup() && !isIconic()) {
        //int x = this->x() + borderX();
        //int y = this->y() + borderY();
        int w = this->width() - 2 * borderX();
        int h = this->height() - 2 * borderY() - titleY();

        fClientContainer->geometry(borderX(), borderY()
#ifndef TITLEBAR_BOTTOM
                                 + titleY()
#endif
                                 , w, h);
        fClient->geometry(0, 0, w, h);
    }
}

bool YFrameWindow::canClose() {
    return frameFunctions() & ffClose;
}

bool YFrameWindow::canMaximize() {
    return frameFunctions() & ffMaximize;
}

bool YFrameWindow::canMinimize() {
    return frameFunctions() & ffMinimize;
}

bool YFrameWindow::canRollup() {
    return (frameFunctions() & ffRollup) && titleY() > 0;
}

bool YFrameWindow::canHide() {
    return frameFunctions() & ffHide;
}

bool YFrameWindow::canLower() {
    return this != manager->bottom(layer());
}

bool YFrameWindow::canRaise() {
    for (YFrameWindow *w = prev(); w; w = w->prev()) {
        if (w->visibleNow()) {
            YFrameWindow *o = w;
            while (o) {
                o = o->owner();
                if (o == this)
                    break;
                else if (o == 0)
                    return true;
            }
        }
    }
    return false;
}

bool YFrameWindow::Overlaps(bool above) {
    YFrameWindow *f;
    int w1x2 , w1y2 , w2x2 , w2y2;
    long curWorkspace = manager->activeWorkspace();
    bool B,C,D,E,F,H;

    if (above)
        f = prev();
    else
        f = next();

    while (f){
        if (!f->isMinimized() && !f->isHidden() && f->visibleOn(curWorkspace)) {
            w2x2 = f->x() + (int)f->width() - 1;
            w2y2 = f->y() + (int)f->height() - 1;
            w1x2 = x() + (int)width() - 1;
            w1y2 = y() + (int)height() - 1;
            B = w2x2 >= x();
            C = y() >= f->y();
            E = w1x2 >= f->x();
            F = w2x2 >= w1x2;
            H = w2y2 >= w1y2;
            if (w1y2 >= f->y()) {
                if (F) {
                    if (E && H) {
                        return true;
                    }
                } else {
                    if (B && !C) {
                        return true;
                    }
                }
            }
            D = w2y2 >= y();
            if (x() >= f->x()){
                if (C) {
                    if (B && D) {
                        return true;
                    }
                } else {
                    if (F && !H) {
                        return true;
                    }
                }
            } else {
                if (H) {
                    if (C && !F) {
                        return true;
                    }
                } else {
                    if (E && D) {
                        return true;
                    }
                }
            }
        }

        if (above)
            f = f->prev();
        else
            f = f->next();
    }
    return false;
}
