/**
 *  IceWM - KDE tray window container
 *  Copyright (C) 2001 The Authors of IceWM
 *
 *  Release under terms of the GNU Library General Public License
 */

#ifndef __WMTRAYWIN_H
#define __WMTRAYWIN_H


#if CONFIG_KDE_TRAY_WINDOWS

class YTrayWindow:
public YFrameProxy {
public:
    YTrayWindow(YWindow *parent, YFrameWindow *owner, YFrameClient *client);
    virtual ~YTrayWindow();

    YFrameClient *client() const { return fClient; }
    virtual YFrameWindow *owner() const { return fOwner; }
#ifdef CONFIG_WINLIST
    virtual void winListItem(WindowListItem *) {}
#endif
#ifndef LITE
    virtual YIcon *icon() const { return NULL; }
#endif
    virtual const char *title() const { return client()->windowTitle(); }
    virtual const char *iconTitle() const { return client()->iconTitle(); }
    virtual void activateWindow(bool /* raise */) {}
    virtual void actionPerformed(YAction */*action*/, unsigned /*modifiers*/) {}
    virtual bool isHidden() const { return false; }
    virtual bool isMinimized() const { return false; }
    virtual bool focused() const { return false; }
    virtual bool visibleNow() const { return true; }
    virtual bool canRaise() { return false; }
    virtual void wmRaise() {}
    virtual void wmLower() {}
    virtual void wmMinimize() {}
    virtual void wmOccupyWorkspace(long /* workspace */ ) {}
    virtual void wmOccupyOnlyWorkspace(long /* workspace */) {}
    virtual void popupSystemMenu();
    virtual void popupSystemMenu(int /*x*/, int /*y*/,
                         int /*x_delta*/, int /*y_delta*/,
                         unsigned int /*flags*/,
                         YWindow */*forWindow*/ = 0);

private:
    YFrameClient *fClient;
    YFrameWindow *fOwner;
};


#endif // CONFIG_KDE_TRAY_WINDOWS

#endif // __WMTRAYWIN_H
