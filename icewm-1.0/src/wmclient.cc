/*
 * IceWM
 *
 * Copyright (C) 1997-2001 Marko Macek
 */
#include "config.h"
#include "yfull.h"
#include "wmclient.h"

#include "wmframe.h"
#include "wmmgr.h"
#include "wmapp.h"
#include "sysdep.h"

YFrameClient::YFrameClient(YWindow *parent, Window win):
YWindow(parent, win),
fFrame(NULL),
fProtocols(0), fHaveButtonGrab(false), fBorder(0),
fSizeHints(XAllocSizeHints()), fClassHint(XAllocClassHint()), fHints(NULL), 
fColormap(None), fTransientFor(None),
#ifdef CONFIG_SHAPE
fShaped(false), 
#endif
#ifdef CONFIG_SESSION
fClientLeader(None), fWindowRole(NULL),
#endif
#ifdef CONFIG_MOTIF_HINTS
fMwmHints(NULL),
#endif
#ifdef CONFIG_GNOME_HINTS
fWinHints(0),
#endif
#if CONFIG_KDE_TRAY_WINDOWS
fTrayWindow(NULL), fTrayWindowFor(None),
#endif
fWindowTitle(NULL), fIconTitle(NULL) {
    updateProtocols();
    updateNameHint();
    updateIconNameHint();
    updateSizeHints();
    updateClassHint();
    updateTransient();
    updateWMHints();
#ifdef CONFIG_MOTIF_HINTS
    updateMwmHints();
#endif
#if CONFIG_GNOME_HINTS
    updateWinHints(fWinHints);
#endif
#if CONFIG_KDE_TRAY_WINDOWS
    updateTrayWindowFor();
#endif    
#ifdef CONFIG_WM_SESSION
    updatePid();
#endif
#ifdef CONFIG_SHAPE
    if (shapesSupported) {
        XShapeSelectInput(app->display(), handle(), ShapeNotifyMask);
        updateShape();
    }
#endif

    saveContext();
}

/**
 * Release the frame client.
 */

YFrameClient::~YFrameClient() {
    deleteContext();

    delete[] fWindowTitle;
    delete[] fIconTitle;

    if (fSizeHints) XFree(fSizeHints);

    if (fClassHint) {
        if (fClassHint->res_name) XFree(fClassHint->res_name);
        if (fClassHint->res_class) XFree(fClassHint->res_class);
        XFree(fClassHint);
    }

    if (fHints) XFree(fHints);
    if (fMwmHints) XFree(fMwmHints);
    if (fWindowRole) XFree(fWindowRole);
}

/**
 * Disconnect the internal object and the X Window object.
 */

void YFrameClient::deleteContext() {
    XDeleteContext(app->display(), handle(),
                   trayWindow() ? YWindowManager::trayContext :
                        frame() ? YWindowManager::frameContext :
                                  YWindowManager::clientContext);
}

/**
 * Connect the internal object and the X Window object.
 */

void YFrameClient::saveContext() {
    XSaveContext(app->display(), handle(),
                   trayWindow() ? YWindowManager::trayContext :
                        frame() ? YWindowManager::frameContext :
                                  YWindowManager::clientContext,
                   trayWindow() ? (XPointer)trayWindow() :
                        frame() ? (XPointer)frame() :
                                  (XPointer)this);
}

/**
 * Update the list of window manager protocols supported by the client
 */

void YFrameClient::updateProtocols() {
    Atom *wmp = 0;
    int count;

    fProtocols = fProtocols & wm::DeleteWindow; // always keep WM_DELETE_WINDOW

    if (XGetWMProtocols(app->display(),
                        handle(),
                        &wmp, &count) && wmp)
    {
        for (int i = 0; i < count; i++) {
            if (wmp[i] == atoms.wmDeleteWindow) fProtocols |= wm::DeleteWindow;
            if (wmp[i] == atoms.wmTakeFocus) fProtocols |= wm::TakeFocus;
        }
        XFree(wmp);
    }
}

/**
 * Update the client's size hints
 */

void YFrameClient::updateSizeHints() {
    if (fSizeHints) {
        long supplied;

        if (!XGetWMNormalHints(app->display(),
                               handle(),
                               fSizeHints, &supplied))
            fSizeHints->flags = 0;

        if (fSizeHints->flags & PResizeInc) {
            if (fSizeHints->width_inc == 0) fSizeHints->width_inc = 1;
            if (fSizeHints->height_inc == 0) fSizeHints->height_inc = 1;
        } else
            fSizeHints->width_inc = fSizeHints->height_inc = 1;


        if (!(fSizeHints->flags & PBaseSize)) {
            if (fSizeHints->flags & PMinSize) {
                fSizeHints->base_width = fSizeHints->min_width;
                fSizeHints->base_height = fSizeHints->min_height;
            } else
                fSizeHints->base_width = fSizeHints->base_height = 0;
        }
        if (!(fSizeHints->flags & PMinSize)) {
            fSizeHints->min_width = fSizeHints->base_width;
            fSizeHints->min_height = fSizeHints->base_height;
        }
        if (!(fSizeHints->flags & PMaxSize)) {
            fSizeHints->max_width = 32767;
            fSizeHints->max_height = 32767;
        }
        if (fSizeHints->max_width < fSizeHints->min_width)
            fSizeHints->max_width = 32767;
        if (fSizeHints->max_height < fSizeHints->min_height)
            fSizeHints->max_height = 32767;

        if (fSizeHints->min_height <= 0)
            fSizeHints->min_height = 1;
        if (fSizeHints->min_width <= 0)
            fSizeHints->min_width = 1;

        if (!(fSizeHints->flags & PWinGravity)) {
            fSizeHints->win_gravity = NorthWestGravity;
            fSizeHints->flags |= PWinGravity;
        }
    }
}

/**
 * Update the WM_CLASS hint
 */

void YFrameClient::updateClassHint() {
    if (fClassHint) {
        if (fClassHint->res_name) {
            XFree(fClassHint->res_name);
            fClassHint->res_name = 0;
        }
        if (fClassHint->res_class) {
            XFree(fClassHint->res_class);
            fClassHint->res_class = 0;
        }
        XGetClassHint(app->display(), handle(), fClassHint);
    }
}

/**
 * Update the transition state of the client.
 */

void YFrameClient::updateTransient() {
    Window newTransientFor;

    if (XGetTransientForHint(app->display(),
                             handle(),
                             &newTransientFor))
    {
        if (newTransientFor == manager->handle() || /* bug in xfm */
            newTransientFor == desktop->handle() ||
            newTransientFor == handle()             /* bug in fdesign */
            /* !!! TODO: check for recursion */
           )
            newTransientFor = 0;

        if (newTransientFor != fTransientFor) {
            if (fTransientFor && frame())
                frame()->removeAsTransient();

            fTransientFor = newTransientFor;

            if (fTransientFor && frame())
                frame()->addAsTransient();
        }
    }
}

/**
 * Adjust the client to fit into workspace
 */

void YFrameClient::constrainSize(int &w, int &h, long layer, int flags) {
    if (fSizeHints) {
        int const wMin(fSizeHints->min_width);
        int const hMin(fSizeHints->min_height);
        int const wMax(fSizeHints->max_width);
        int const hMax(fSizeHints->max_height);
        int const wBase(fSizeHints->base_width);
        int const hBase(fSizeHints->base_height);
        int const wInc(fSizeHints->width_inc);
        int const hInc(fSizeHints->height_inc);

        if (fSizeHints->flags & PAspect) { // aspect ratios
            int const xMin(fSizeHints->min_aspect.x);
            int const yMin(fSizeHints->min_aspect.y);
            int const xMax(fSizeHints->max_aspect.x);
            int const yMax(fSizeHints->max_aspect.y);

            // !!! fix handling of KeepX and KeepY together
            if (xMin * h > yMin * w) { // min aspect
                if (flags & csKeepX) {
		    w = clamp(w, wMin, wMax);
                    h = w * yMin / xMin;
		    h = clamp(h, hMin, hMax);
                    w = h * xMin / yMin;
                } else {
		    h = clamp(h, hMin, hMax);
                    w = h * xMin / yMin;
		    w = clamp(w, wMin, wMax);
                    h = w * yMin / xMin;
                }
            }
            if (xMax * h < yMax * w) { // max aspect
                if (flags & csKeepX) {
		    w = clamp(w, wMin, wMax);
                    h = w * yMax / xMax;
		    h = clamp(h, hMin, hMax);
                    w = h * xMax / yMax;
                } else {
		    h = clamp(h, hMin, hMax);
                    w = h * xMax / yMax;
		    w = clamp(w, wMin, wMax);
                    h = w * yMax / xMax;
                }
            }
        }

	h = clamp(h, hMin, hMax);
	w = clamp(w, wMin, wMax);

        if (limitSize) {
            w = min(w, (int)(considerHorizBorder && !frame()->dontCover()
	      ? manager->maxWidth(layer) - 2 * frame()->borderX()
	      : manager->maxWidth(layer)));
            h = min(h, (int)(considerVertBorder && !frame()->dontCover()
	      ? manager->maxHeight(layer) - frame()->titleY()
	      				  - 2 * frame()->borderY()
	      : manager->maxHeight(layer) - frame()->titleY()));
        }

#if 0
        w = wBase + (w - wBase + ((flags & csRound) ? wInc / 2 : 0)) / wInc
								     * wInc;
        h = hBase + (h - hBase + ((flags & csRound) ? hInc / 2 : 0)) / hInc
								     * hInc;
#else
	if (flags & csRound) { w+= wInc / 2; h+= hInc / 2; }

	w-= max(0, w - wBase) % wInc;
	h-= max(0, h - hBase) % hInc;
#endif								     

    }

    if (w <= 0) w = 1;
    if (h <= 0) h = 1;
}

struct _gravity_offset
{
  int x, y;
};

/**
 * Return gravitity information. Needed to reposition window upon (un)map.
 */

void YFrameClient::gravityOffsets(int &xp, int &yp) {
    xp = 0;
    yp = 0;

    if (fSizeHints == 0)
        return ;

    static struct {
        int x, y;
    } gravOfsXY[11] = {
        {  0,  0 },  /* ForgetGravity */
        { -1, -1 },  /* NorthWestGravity */
        {  0, -1 },  /* NorthGravity */
        {  1, -1 },  /* NorthEastGravity */
        { -1,  0 },  /* WestGravity */
        {  0,  0 },  /* CenterGravity */
        {  1,  0 },  /* EastGravity */
        { -1,  1 },  /* SouthWestGravity */
        {  0,  1 },  /* SouthGravity */
        {  1,  1 },  /* SouthEastGravity */
        {  0,  0 },  /* StaticGravity */
    };

    int g = fSizeHints->win_gravity;

    if (!(g < ForgetGravity || g > StaticGravity)) {
        xp = (int)gravOfsXY[g].x;
        yp = (int)gravOfsXY[g].y;
    }
}

void YFrameClient::sendMessage(Atom msg, Time timeStamp) {
    XClientMessageEvent xev;

    memset(&xev, 0, sizeof(xev));
    xev.type = ClientMessage;
    xev.window = handle();
    xev.message_type = atoms.wmProtocols;
    xev.format = 32;
    xev.data.l[0] = msg;
    xev.data.l[1] = timeStamp;
    XSendEvent(app->display(), handle(), False, 0L, (XEvent *) &xev);
}

void YFrameClient::frame(YFrameWindow *frame) {
    if (frame != this->frame()) {
        deleteContext();
        fFrame = frame;
        saveContext();
    }
}

void YFrameClient::trayWindow(YTrayWindow *trayWindow) {
    if (trayWindow != this->trayWindow()) {
        deleteContext();
        fTrayWindow = trayWindow;
        saveContext();
    }
}

wm::State YFrameClient::wmState() {
    YWindowProperty wmState(handle(), atoms.wmState, atoms.wmState, 3);
    return wmState == Success && wmState.count() ?
           wmState.template data<wm::State>() : WithdrawnState;
}

void YFrameClient::wmState(wm::State state) {
    unsigned long arg[] = { state, None };

    //msg("setting frame state to %d", arg[0]);

    if (state == WithdrawnState) {
        if (phase != phaseRestart && phase != phaseShutdown) {
            MSG(("deleting window properties id=%lX", handle()));
            XDeleteProperty(app->display(), handle(), atoms.winWorkspace);
            XDeleteProperty(app->display(), handle(), atoms.winLayer);
#ifdef CONFIG_TRAY
            XDeleteProperty(app->display(), handle(), atoms.icewmTrayOption);
#endif	    
            XDeleteProperty(app->display(), handle(), atoms.winState);
            XDeleteProperty(app->display(), handle(), atoms.wmState);
        }
    } else {
        XChangeProperty(app->display(), handle(),
                        atoms.wmState, atoms.wmState, 32,
                        PropModeReplace, (unsigned char const*)arg, 2);
    }
}

void YFrameClient::handleUnmap(const XUnmapEvent &unmap) {
    MSG(("Unmap: unmapped %d visible %d", unmapped(), visible()));

    if (!unmapped()) {
        MSG(("UnmapWindow"));

        XEvent ev;
        if (XCheckTypedWindowEvent(app->display(), unmap.window,
				   DestroyNotify, &ev)) {
            manager->destroyedClient(unmap.window);
            return; // gets destroyed
        } else if (XCheckTypedWindowEvent(app->display(), unmap.window,
					  ReparentNotify, &ev)) {
            manager->unmanageClient(unmap.window, true, false);
            return; // gets destroyed
        } else {
            manager->unmanageClient(unmap.window, false);
            return; // gets destroyed
        }
    }

    YWindow::handleUnmap(unmap);
}

void YFrameClient::handleProperty(const XPropertyEvent &property) {
    switch (property.atom) {
	case XA_WM_NAME:
            updateNameHint();
	    break;

	case XA_WM_ICON_NAME:
            updateIconNameHint();
            break;

	case XA_WM_CLASS:
	    updateClassHint();
	    if (frame()) frame()->updateFrameHints();
	    break;

	case XA_WM_HINTS:
	    updateWMHints();
#ifndef LITE
            if (frame()) frame()->updateIcon();
#endif            
	    break;

	case XA_WM_NORMAL_HINTS:
	    updateSizeHints();
	    if (frame()) {
		frame()->updateMwmHints();
		frame()->updateNormalSize();
	    }
	    break;

	case XA_WM_TRANSIENT_FOR:
	    updateTransient();
	    break;

	default: // `extern Atom' does not reduce to an integer constant...
	    if (atoms.wmProtocols == property.atom) {
		updateProtocols();
#ifndef LITE
	    } else if (atoms.kwmWinIcon == property.atom ||
		       atoms.winIcons == property.atom) {
		if (frame()) frame()->updateIcon();
#endif

	    } else if (atoms.winHints == property.atom) {
		updateWinHints(fWinHints);

		if (frame()) {
                    frame()->updateFrameHints();
                    manager->updateWorkArea();
#ifdef CONFIG_TASKBAR
                    frame()->updateTaskBar();
#endif
		}
#ifdef CONFIG_MOTIF_HINTS
	    } else if (atoms.mwmHints == property.atom) {
		updateMwmHints();
		if (frame()) frame()->updateMwmHints();
		break;
#endif
#if CONFIG_KDE_TRAY_WINDOWS
	    } else if (atoms.kdeNetwmSystemTrayWindowFor == property.atom) {
msg("updating updateTrayWindowFor");            
                updateTrayWindowFor();
#endif
	    } else
		MSG(("Unknown property changed: %s, window=0x%lX",
		     XGetAtomName(app->display(), property.atom), handle()));

	    break;
    }
}

void YFrameClient::handleColormap(const XColormapEvent &event) {
    colormap(event.colormap); //(colormap.state == ColormapInstalled && colormap.c_new == True)
//                ? colormap.colormap
//                : None);
}


void YFrameClient::handleDestroyWindow(const XDestroyWindowEvent &destroyWindow) {
    //msg("DESTROY: %lX", destroyWindow.window);
    YWindow::handleDestroyWindow(destroyWindow);

    if (destroyed())
        manager->destroyedClient(destroyWindow.window);
}

#ifdef CONFIG_SHAPE
void YFrameClient::handleShapeNotify(const XShapeEvent &shape) {
    if (shapesSupported) {
        MSG(("shape event: %d %d %d:%d=%dx%d time=%ld",
             shape.shaped, shape.kind,
             shape.x, shape.y, shape.width, shape.height, shape.time));
        if (shape.kind == ShapeBounding) {
            bool const newShaped(shape.shaped);
            if (newShaped) fShaped = newShaped;
            if (frame()) frame()->setShape();
            fShaped = newShaped;
        }
    }
}
#endif

void YFrameClient::windowTitle(const char *title) {
    delete[] fWindowTitle; fWindowTitle = newstr(title);
    if (frame()) frame()->updateTitle();
}

void YFrameClient::iconTitle(const char *title) {
    delete[] fIconTitle; fIconTitle = newstr(title);
    if (frame()) frame()->updateIconTitle();
}

#ifdef CONFIG_I18N
void YFrameClient::windowTitle(const XTextProperty & title) {
    if (NULL == title.value || title.encoding == XA_STRING)
        windowTitle((const char *)title.value);
    else {
        int count;
        char ** strings(NULL);

        if (XmbTextPropertyToTextList(app->display(), &title,
                                      &strings, &count) >= 0 &&
            count > 0 && strings[0])
            windowTitle((const char *)strings[0]);
        else
            windowTitle((const char *)title.value);

        if (strings) XFreeStringList(strings);
    }
}

void YFrameClient::iconTitle(const XTextProperty & title) {
    if (NULL == title.value || title.encoding == XA_STRING)
        iconTitle((const char *)title.value);
    else {
        int count;
        char ** strings(NULL);

        if (XmbTextPropertyToTextList(app->display(), &title,
                                      &strings, &count) >= 0 &&
            count > 0 && strings[0])
            iconTitle((const char *)strings[0]);
        else
            iconTitle((const char *)title.value);

        if (strings) XFreeStringList(strings);
    }
}
#endif

void YFrameClient::colormap(Colormap cmap) {
    fColormap = cmap;
    if (frame() && manager->colormapWindow() == frame())
        manager->installColormap(cmap);
}

#ifdef CONFIG_SHAPE
void YFrameClient::updateShape() {
    fShaped = false;

    if (shapesSupported) {
        int xws, yws, xbs, ybs;
        unsigned wws, hws, wbs, hbs;
        Bool boundingShaped, clipShaped;

        XShapeQueryExtents(app->display(), handle(),
                           &boundingShaped, &xws, &yws, &wws, &hws,
                           &clipShaped, &xbs, &ybs, &wbs, &hbs);
        fShaped = boundingShaped;
  }
}
#endif

void YFrameClient::handleClientMessage(const XClientMessageEvent &message) {
    if (message.message_type == atoms.wmChangeState) {
        YFrameWindow *frame = manager->findFrame(message.window);

        if (message.data.l[0] == IconicState) {
            if (frame && !(frame->isMinimized() || frame->isRollup()))
                frame->wmMinimize();
        } else if (message.data.l[0] == NormalState) {
            if (frame)
                frame->state(WinStateHidden |
                             WinStateRollup |
                             WinStateMinimized, 0);
        } // !!! handle WithdrawnState if needed
#ifdef CONFIG_GNOME_HINTS
    } else if (message.message_type == atoms.winWorkspace) {
        if (frame()) frame()->workspace(message.data.l[0]);
        else winWorkspace(message.data.l[0]);
    } else if (message.message_type == atoms.winLayer) {
        if (frame()) frame()->layer(message.data.l[0]);
        else winLayer(message.data.l[0]);
    } else if (message.message_type == atoms.winState) {
        if (frame()) frame()->state(message.data.l[0], message.data.l[1]);
        else winState(message.data.l[0], message.data.l[1]);
#endif        
#ifdef CONFIG_TRAY	    
    } else if (message.message_type == atoms.icewmTrayOption) {
        if (frame()) frame()->trayOption(message.data.l[0]);
        else icewmTrayOption(message.data.l[0]);
#endif	    
    } else
        YWindow::handleClientMessage(message);
}

void YFrameClient::updateNameHint() {
#ifdef CONFIG_I18N
    XTextProperty name;
    if (XGetWMName(app->display(), handle(), &name))
#else    
    char * name;
    if (XFetchName(app->display(), handle(), &name))
#endif
        windowTitle(name);
    else
        windowTitle(NULL);
}

void YFrameClient::updateIconNameHint() {
#ifdef CONFIG_I18N
    XTextProperty name;
    if (XGetWMIconName(app->display(), handle(), &name))
#else    
    char * name;
    if (XGetIconName(app->display(), handle(), &name))
#endif
        iconTitle(name);
    else
        iconTitle(NULL);
}

void YFrameClient::updateWMHints() {
    if (fHints) XFree(fHints);
    fHints = XGetWMHints(app->display(), handle());
}

#ifdef CONFIG_MOTIF_HINTS
void YFrameClient::updateMwmHints() {
    if (fMwmHints) {
        XFree(fMwmHints);
        fMwmHints = NULL;
    }

    YWindowProperty mwmHints(handle(), atoms.mwmHints, atoms.mwmHints, 20);
    if (mwmHints == Success && mwmHints.count() >= PROP_MWM_HINTS_ELEMENTS)
        fMwmHints = mwmHints.template release<MwmHints>();
}

/**
 * Sets the complete Motif Window Manager hint.
 */

void YFrameClient::mwmHints(const MwmHints &mwm) {
    if (fMwmHints) XFree(fMwmHints);

    fMwmHints = (MwmHints *)malloc(sizeof(MwmHints));
    if (fMwmHints) *fMwmHints = mwm;

    XChangeProperty(app->display(), handle(),
                    atoms.mwmHints, atoms.mwmHints, 32,
                    PropModeReplace, (unsigned char const*)&mwm,
                    sizeof(mwm)/sizeof(long)); ///!!! ???
}

/**
 * Sets the most common Motif Window Manager hints.
 */

void YFrameClient::mwmHints(unsigned long functions,
                            unsigned long decorations) {
    MwmHints mwm;
    
    mwm.flags = MWM_HINTS_FUNCTIONS | MWM_HINTS_DECORATIONS;
    mwm.functions = functions;
    mwm.decorations = decorations;

    mwmHints(mwm);
}

unsigned long YFrameClient::mwmFunctions() {
    unsigned long functions = ~0U;

    if (fMwmHints && (fMwmHints->flags & MWM_HINTS_FUNCTIONS)) {
        functions = fMwmHints->functions & MWM_FUNC_ALL
                  ? ~fMwmHints->functions
                  : fMwmHints->functions;
    } else {
        XSizeHints *sh = sizeHints();

        if (sh) {
            bool minmax = false;
            if (sh->min_width == sh->max_width &&
                sh->min_height == sh->max_height)
            {
                functions &= ~MWM_FUNC_RESIZE;
                minmax = true;
            }
            if ((minmax && !(sh->flags & PResizeInc)) ||
                (sh->width_inc == 0 && sh->height_inc == 0))
                functions &= ~MWM_FUNC_MAXIMIZE;
        }
    }

    functions &= (MWM_FUNC_RESIZE | MWM_FUNC_MOVE |
                  MWM_FUNC_MINIMIZE | MWM_FUNC_MAXIMIZE |
                  MWM_FUNC_CLOSE);

    return functions;
}

unsigned long YFrameClient::mwmDecorations() {
    unsigned long decorations = ~0U;
    unsigned long functions(mwmFunctions());

    if (fMwmHints && (fMwmHints->flags & MWM_HINTS_DECORATIONS)) {
        decorations = fMwmHints->decorations & MWM_DECOR_ALL
                    ? ~fMwmHints->decorations
                    : fMwmHints->decorations;
    } else {
        XSizeHints *sh = sizeHints();

        if (sh) {
            bool minmax = false;
            if (sh->min_width == sh->max_width &&
                sh->min_height == sh->max_height)
            {
                decorations &= ~MWM_DECOR_RESIZEH;
                minmax = true;
            }
            if ((minmax && !(sh->flags & PResizeInc)) ||
                (sh->width_inc == 0 && sh->height_inc == 0))
                decorations &= ~MWM_DECOR_MAXIMIZE;
        }
    }
    decorations &= (MWM_DECOR_BORDER | MWM_DECOR_RESIZEH |
                    MWM_DECOR_TITLE | MWM_DECOR_MENU |
                    MWM_DECOR_MINIMIZE | MWM_DECOR_MAXIMIZE);

    /// !!! add disabled buttons
    decorations &=
        ~(/*(functions & MWM_FUNC_RESIZE ? 0 : MWM_DECOR_RESIZEH) |*/
          (functions & MWM_FUNC_MINIMIZE ? 0 : MWM_DECOR_MINIMIZE) |
          (functions & MWM_FUNC_MAXIMIZE ? 0 : MWM_DECOR_MAXIMIZE));

    return decorations;
}

#endif /* CONFIG_MOTIF_HINTS */

bool YFrameClient::updateKwmIcon(int &count, Pixmap *&pixmap) {
    YWindowProperty kwmIcon(handle(), atoms.kwmWinIcon, atoms.kwmWinIcon, 2);
    if (kwmIcon == Success &&  kwmIcon.format() == 32 &&
        kwmIcon.count() == 2 && kwmIcon.type() == atoms.kwmWinIcon) {
        count = kwmIcon.count();
        pixmap = kwmIcon.template release<Pixmap>();
        updateWMHints();
        return true;
    }

    return false;
}

#if CONFIG_GNOME_HINTS
bool YFrameClient::updateWinIcons(Atom &type, int &count, long *&elem) {
    YWindowProperty winIcons(handle(), atoms.winIcons, AnyPropertyType, 4096);
    if (winIcons == Success &&  winIcons.format() == 32 &&
        winIcons.count() > 0 && (winIcons.type() == atoms.winIcons ||
                                 winIcons.type() == XA_PIXMAP)) {
        type = winIcons.type();
        count = winIcons.count();
        elem = winIcons.template release<long>();
        return true;
    }

    return false;
}

void YFrameClient::winWorkspace(gnome::Workspace workspace) {
    XChangeProperty(app->display(), handle(),
                    atoms.winWorkspace, XA_CARDINAL, 32,
                    PropModeReplace, (unsigned const char*)&workspace, 1);
}

void YFrameClient::winLayer(gnome::Layer layer) {
    XChangeProperty(app->display(), handle(),
                    atoms.winLayer, XA_CARDINAL, 32,
                    PropModeReplace, (unsigned char const*)&layer, 1);
}

void YFrameClient::winState(gnome::State mask, gnome::State state) {
    gnome::State stateHint[] = { state, mask };
    MSG(("set state=%lX mask=%lX", state, mask));

    XChangeProperty(app->display(), handle(),
                    atoms.winState, XA_CARDINAL, 32,
                    PropModeReplace, (unsigned char const*)&stateHint, 2);
}

void YFrameClient::winHints(gnome::Hints hints) {
    fWinHints = hints;

    XChangeProperty(app->display(), handle(),
                    atoms.winHints, XA_CARDINAL, 32,
                    PropModeReplace,(unsigned char const*)&hints, 1);
}

bool YFrameClient::updateWinWorkspace(gnome::Workspace &workspace) {
    YWindowProperty winWorkspace(handle(), atoms.winWorkspace, XA_CARDINAL);
    if (winWorkspace.valid(XA_CARDINAL, 32)) {
        gnome::Workspace newWorkspace;
        winWorkspace.copy(newWorkspace);

        if (workspaceCount > newWorkspace) {
            workspace = newWorkspace;
            return true;
        }
    }

    return false;
}

bool YFrameClient::updateWinLayer(gnome::Layer &layer) {
    YWindowProperty winLayer(handle(), atoms.winLayer, XA_CARDINAL);
    if (winLayer.valid(XA_CARDINAL, 32)) {
        icewm::Layer newLayer;
        winLayer.copy(newLayer);

        if (WinLayerCount > newLayer) {
            layer = newLayer;
            return true;
        }
    }

    return false;
}

bool YFrameClient::updateWinState(gnome::State &mask, gnome::State &state) {
    YWindowProperty winState(handle(), atoms.winState, XA_CARDINAL, 2);
    
    if (winState == Success && winState.count() >= 1) {
        MSG(("got state"));

        if (winState.type() == XA_CARDINAL && winState.format() == 32) {
            state = winState.template data<gnome::State>(0);
            mask = winState.count() >= 2
                 ? winState.template data<gnome::State>(1)
                 : WIN_STATE_ALL;

            return true;
        }

        MSG(("bad state"));
    }

    return false;
}

bool YFrameClient::updateWinHints(gnome::Hints &hints) {
    YWindowProperty winHints(handle(), atoms.winHints, XA_CARDINAL);
    
    if (winHints == Success) {
        MSG(("got hints"));

        if (winHints.valid(XA_CARDINAL, 32)) {
            winHints.copy(hints);
            return true;
        }

        MSG(("bad hints"));
    }

    return false;
}
#endif /* CONFIG_GNOME_HINTS */

#ifdef CONFIG_TRAY
void YFrameClient::icewmTrayOption(icewm::TrayOption option) {
    XChangeProperty(app->display(), handle(),
                    atoms.icewmTrayOption, XA_CARDINAL, 32,
                    PropModeReplace, (unsigned char const*)&option, 1);
}

bool YFrameClient::updateIcewmTrayOption(icewm::TrayOption &option) {
    YWindowProperty icewmTrayOption(handle(), atoms.icewmTrayOption, XA_CARDINAL);
    
    if (icewmTrayOption.valid(XA_CARDINAL, 32)) {
        icewm::TrayOption newOption;
        icewmTrayOption.copy(newOption);

        if (IcewmTrayOptionCount > newOption) {
            option = newOption;
            return true;
        }
    }

    return false;
}
#endif /* CONFIG_TRAY */

#ifdef CONFIG_WM_SESSION
void YFrameClient::updatePid() {
    YWindowProperty icewmPid(handle(), atoms.icewmPid, XA_CARDINAL);
    if (icewmPid.valid(XA_CARDINAL, 32))
        icewmPid.copy(fPid);
    else    
        warn(_("Window %p has no _ICEWM_PID property. "
	       "Export the LD_PRELOAD variable to preload the preice library."),
	       handle());
}
#endif

void YFrameClient::updateClientLeader() {
    YWindowProperty wmClientLeader(handle(), atoms.wmClientLeader, XA_WINDOW);

    if (wmClientLeader.valid(XA_WINDOW, 32))
        wmClientLeader.copy(fClientLeader);
}

void YFrameClient::updateWindowRole() {
}

char *YFrameClient::clientId(Window leader) { /// !!! fix
    YWindowProperty smClientId(leader, atoms.smClientId, XA_STRING, 256);
    
    return smClientId == Success && smClientId.format() == 8 &&
           smClientId.count() > 0 && smClientId.type() == XA_STRING
        ? smClientId.template release<char>() : NULL;
}

#if CONFIG_KDE_TRAY_WINDOWS

/**
 * Update KDE's tray hint. Return true if successfull.
 *
 * KDE tray windows carry the KWM_DOCKWINDOW or the
 * _KDE_NET_WM_SYSTEM_TRAY_WINDOW_FOR property.
 */

bool YFrameClient::updateTrayWindowFor(void) {
    fTrayWindowFor = None;

#ifdef CONFIG_WMSPEC_HINTS
    YWindowProperty kdeTrayWindow(handle(), atoms.kdeNetwmSystemTrayWindowFor);
    if (kdeTrayWindow.valid(XA_WINDOW, 32)) {
        kdeTrayWindow.copy(fTrayWindowFor);
        if (fTrayWindowFor == None) fTrayWindowFor = desktop->handle();
        return true;
    }
#endif /* CONFIG_WMSPEC_HINTS */
    YWindowProperty kwmDockWindow(handle(), atoms.kwmDockwindow);
    if (kwmDockWindow.valid(atoms.kwmDockwindow, 32)) {
        fTrayWindowFor = desktop->handle();
        return true;
    }

    return false;
}
#endif

/**
 * Return true if this client is supposed to be a DockApp.
 *
 * DockApp have WithdrawnState as initial state and have a pointer to
 * an icon window in their WMHints property.
 */

#ifdef CONFIG_DOCK
bool YFrameClient::isDockApp() const {
    return (NULL != fHints && (StateHint | IconWindowHint) ==
           (fHints->flags & (StateHint | IconWindowHint)) &&
           WithdrawnState == fHints->initial_state);
}
#endif
