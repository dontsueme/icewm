#ifndef __YCLIENT_H
#define __YCLIENT_H

#include "ywindow.h"
#include "ymenu.h"
#include "MwmUtil.h"

class YFrameWindow;
class YTrayWindow;
class WindowListItem;

typedef int FrameState;

#ifndef __YIMP_UTIL__
//!!! remove these if possible
typedef struct XWMHints;
typedef struct XSizeHints;
typedef struct XClassHint;
#endif

class YClientPeer {
public:
    virtual ~YClientPeer() {}

#ifdef CONFIG_WINLIST
    virtual void winListItem(WindowListItem *i) = 0;
#endif
    virtual YFrameWindow *owner() const = 0;
#ifndef LITE
    virtual YIcon *icon() const = 0;
#endif
    virtual const char *title() const = 0;
    virtual const char *iconTitle() const = 0;
    virtual void activateWindow(bool raise) = 0;
    virtual bool isHidden() const = 0;
    virtual bool isMinimized() const = 0;
    virtual void actionPerformed(YAction *action, unsigned int modifiers) = 0;
    virtual bool focused() const = 0;
    virtual bool visibleNow() const = 0;
    virtual bool canRaise() = 0;
    virtual void wmRaise() = 0;
    virtual void wmLower() = 0;
    virtual void wmMinimize() = 0;
    virtual void wmOccupyWorkspace(long workspace) = 0;
    virtual void wmOccupyOnlyWorkspace(long workspace) = 0;
    virtual void popupSystemMenu() = 0;
    virtual void popupSystemMenu(int x, int y,
                         int x_delta, int y_delta,
                         unsigned int flags,
                         YWindow *forWindow = 0) = 0;
};

class YFrameClient: public YWindow  {
public:
    YFrameClient(YWindow *parent = NULL, Window win = None);
    virtual ~YFrameClient();

    virtual void handleProperty(const XPropertyEvent &property);
    virtual void handleColormap(const XColormapEvent &colormap);
    virtual void handleUnmap(const XUnmapEvent &unmap);
    virtual void handleDestroyWindow(const XDestroyWindowEvent &destroyWindow);
    virtual void handleClientMessage(const XClientMessageEvent &message);
#ifdef CONFIG_SHAPE
    virtual void handleShapeNotify(const XShapeEvent &shape);
#endif

    unsigned border() const { return fBorder; }
    void border(unsigned border) { fBorder = border; }
    void frame(YFrameWindow *frame);
    YFrameWindow *frame() const { return fFrame; };
    void trayWindow(YTrayWindow *trayWindow);
    YTrayWindow *trayWindow() const { return fTrayWindow; };

    enum {
        wpDeleteWindow = 1 << 0,
        wpTakeFocus    = 1 << 1
    } WindowProtocols;

    void sendMessage(Atom msg, Time timeStamp = CurrentTime);

    enum {
        csKeepX      = 1 << 0,
        csKeepY      = 1 << 1,
        csRound      = 1 << 2,
        csDockWindow = 1 << 4
    };
    
    void constrainSize(int &w, int &h, long layer, int flags = 0);
				 
    void gravityOffsets(int &xp, int &yp);

    FrameState frameState();
    void frameState(FrameState state);

    Colormap colormap() const { return fColormap; }
    void colormap(Colormap cmap);

    void updateProtocols();
    unsigned long protocols() const { return fProtocols; }

    void updateWMHints();
    XWMHints *hints() const { return fHints; }

    void updateSizeHints();
    XSizeHints *sizeHints() const { return fSizeHints; }

    void updateClassHint();
    XClassHint *classHint() const { return fClassHint; }

    void updateTransient();
    Window ownerWindow() const { return fTransientFor; }

    void updateNameHint();
    void updateIconNameHint();
    void windowTitle(const char *title);
    void iconTitle(const char *title);
#ifdef CONFIG_I18N
    void windowTitle(const XTextProperty & title);
    void iconTitle(const XTextProperty & title);
#endif
    const char *windowTitle() { return fWindowTitle; }
    const char *iconTitle() { return fIconTitle; }

#if CONFIG_GNOME_HINTS
    bool updateWinIcons(Atom &type, int &count, long *&elem);
    bool updateWinWorkspace(long & workspace);
    bool updateWinLayer(long & layer);
    bool updateWinState(long & mask, long & state);
    bool updateWinHints(long & hints);

    void winWorkspace(long workspace);
    void winLayer(long layer);
    void winState(long mask, long state);
    void winHints(long hints);
    long winHints(void) const { return fWinHints; }
#endif

#ifdef CONFIG_TRAY	    
    bool updateIcewmTrayHint(long & trayopt);
    void icewmTrayHint(long trayopt);
#endif

#ifdef CONFIG_MOTIF_HINTS
    void updateMwmHints();
    MwmHints *mwmHints() const { return fMwmHints; }
    void mwmHints(unsigned long functions, unsigned long decoration);
    void mwmHints(const MwmHints &mwm);
    unsigned long mwmFunctions();
    unsigned long mwmDecorations();
#endif

#ifdef CONFIG_KDE_HINTS
    bool updateKwmIcon(int &count, Pixmap *&pixmap);
#endif

#if CONFIG_KDE_TRAY_WINDOWS
    bool updateTrayWindowFor(void);
    Window trayWindowFor(void) const { return fTrayWindowFor; }
#endif

#ifdef CONFIG_SHAPE
    void updateShape();
    bool shaped() const { return fShaped; }
#endif

#ifdef CONFIG_WM_SESSION
    void updatePid();
    pid_t pid() const { return fPid; }
#endif

#ifdef CONFIG_SESSION
    void updateClientLeader();
    void updateWindowRole();

    Window clientLeader() const { return fClientLeader; }
    const char *windowRole() const { return fWindowRole; }

    char *clientId(Window leader);
#endif

#ifdef CONFIG_DOCK
    bool isDockApp() const;
#endif

protected:
    void deleteContext();
    void saveContext();

private:
    YFrameWindow *fFrame;

    int fProtocols;
    bool fHaveButtonGrab;
    unsigned fBorder;
    XSizeHints *fSizeHints;
    XClassHint *fClassHint;
    XWMHints *fHints;
    Colormap fColormap;
    Window fTransientFor;

#ifdef CONFIG_SHAPE
    bool fShaped;
#endif
#ifdef CONFIG_SESSION
    Window fClientLeader;
    char *fWindowRole;
#endif
#ifdef CONFIG_MOTIF_HINTS
    MwmHints *fMwmHints;
#endif
#ifdef CONFIG_GNOME_HINTS
    long fWinHints;
#endif    
#if CONFIG_KDE_TRAY_WINDOWS
    YTrayWindow *fTrayWindow;
    Window fTrayWindowFor;
#endif    
#ifdef CONFIG_WM_SESSION
    pid_t fPid;
#endif

    char *fWindowTitle;
    char *fIconTitle;
};

#endif
