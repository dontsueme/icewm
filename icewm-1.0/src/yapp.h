#ifndef __YAPP_H
#define __YAPP_H

#include <signal.h>

#include "yatoms.h"
#include "ywindow.h"
#include "ycursor.h"
#include "ypaths.h"

class YClipboard;

class YApplication {
public:
    YApplication(int *argc, char ***argv, const char *displayName = 0);
    virtual ~YApplication();

    int mainLoop();
    void exitLoop(int exitCode);
    void exit(int exitCode);
    
    Display * display() const { return fDisplay; }
    int screen() { return DefaultScreen (display()); }
    Visual * visual() { return DefaultVisual(display(), screen()); }
    Colormap colormap() { return DefaultColormap(display(), screen()); }
    unsigned depth() { return DefaultDepth(display(), screen()); }
    char const * executable() { return fExecutable; }
    
    Atom internAtom(char const *name, bool queryOnly = false);
    void internAtoms(YAtomInfo * info, unsigned const count,
                     bool queryOnly = false);

    bool hasColormap();
    bool hasGNOME();

    void sync(bool discard = false) { XSync(display(), discard); }

    void saveEventTime(XEvent &xev);
    Time eventTime() const { return fLastEventTime; }

    int grabEvents(YWindow *win, Cursor ptr, unsigned int eventMask,
                   bool grabMouse = true, bool grabKeyboard = true,
                   bool grabTree = false);
    int releaseEvents();
    void handleGrabEvent(YWindow *win, XEvent &xev);

    void replayEvent();

    void captureGrabEvents(YWindow *win);
    void releaseGrabEvents(YWindow *win);

    void dispatchEvent(YWindow *win, XEvent &e);
    virtual void afterWindowEvent(XEvent &xev);

    YPopupWindow *popup() const { return fPopup; }
    bool popup(YWindow *forWindow, YPopupWindow *popup);
    void popdown(YPopupWindow *popdown);

    YWindow *grabWindow() const { return fGrabWindow; }

    virtual void handleSignal(int sig);
    virtual void handleIdle();

    void catchSignal(int sig);
    void resetSignals();
    //void unblockSignal(int sig);

    void initModifiers();
    bool isModifier(KeyCode keycode) const;

    void alert();

    void runProgram(const char *str, const char *const *args);
    void runCommand(const char *prog);

    static char * findConfigFile(const char *name);
    
#ifdef CONFIG_SESSION
    bool haveSessionManager();
    virtual void smSaveYourself(bool shutdown, bool fast);
    virtual void smSaveYourselfPhase2();
    virtual void smSaveComplete();
    virtual void smShutdownCancelled();
    virtual void smDie();
    void smSaveDone();
    void smRequestShutdown();
    void smCancelShutdown();
#endif

    void clipboardText(char *data, int len);

    static YCursor leftPointer;
    static YCursor rightPointer;
    static YCursor movePointer;

#ifndef LITE
    static YResourcePaths iconPaths;
#endif

    unsigned int AltMask;
    unsigned int MetaMask;
    unsigned int NumLockMask;
    unsigned int ScrollLockMask;
    unsigned int SuperMask;
    unsigned int HyperMask;
    unsigned int ModeSwitchMask;

    unsigned int WinMask;
    unsigned int Win_L;
    unsigned int Win_R;

    unsigned int KeyMask;
    unsigned int ButtonMask;
    unsigned int ButtonKeyMask;
    
    static char const * Name;

private:
    Display *fDisplay;
    char const * fExecutable;
    XModifierKeymap *fModifierMap;

    Time fLastEventTime;
    bool fReplayEvent;

    bool fGrabTree;
    bool fGrabMouse;
    YWindow *fXGrabWindow;
    YWindow *fGrabWindow;
    YPopupWindow *fPopup;

    YClipboard *fClip;

    int fLoopLevel;
    int fExitCode;
    bool fLoopContinue;
    bool fAppContinue;
};

extern YApplication *app;

class YServerLock {
public:
    YServerLock() { XGrabServer(app->display()); }
    virtual ~YServerLock() { XUngrabServer(app->display()); }
};

class YSynchronServerLock :
public YServerLock {
public:
    YSynchronServerLock(bool discard = false) { app->sync(discard); }
};

#endif
