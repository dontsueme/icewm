#ifndef __WMSTATUS_H
#define __WMSTATUS_H

#ifndef LITE

#include "ywindow.h"

class YFrameWindow;

class YWindowManagerStatus: public YWindow {
public:
    YWindowManagerStatus(YWindow *aParent, const char *(*templFunc) ());
    virtual ~YWindowManagerStatus();

    virtual void paint(Graphics &g, int x, int y, unsigned int width, unsigned int height);

    void begin();
    void end() { hide(); }    

    virtual const char* status() = 0;

protected:
    static YColor *statusFg;
    static YColor *statusBg;
    static YFont *statusFont;
};

class MoveSizeStatus: public YWindowManagerStatus {
public:
    MoveSizeStatus(YWindow *aParent);
    virtual ~MoveSizeStatus();

    virtual const char* status();
    
    void begin(YFrameWindow *frame);
    void status(YFrameWindow *frame, int x, int y, int width, int height);
    void status(YFrameWindow *frame);
private:
    static const char* templateFunction();

    int fX, fY, fW, fH;
};

class WorkspaceStatus: public YWindowManagerStatus {
public:
    WorkspaceStatus(YWindow *aParent);
    virtual ~WorkspaceStatus();

    virtual const char* status();
    void begin(long workspace);
    virtual void status(long workspace);
private:
    static const char* templateFunction();
    static const char* status(const char* name);

    long workspace;    
    class YTimer *timer;
};

extern MoveSizeStatus *statusMoveSize;
extern WorkspaceStatus *statusWorkspace;

#endif

#endif
