#include "config.h"

#include "yxlib.h"
#include "yproto.h"
#include "aworkspaces.h"
#include "wmtaskbar.h"
#include "prefs.h"
#include "yapp.h"
#include "yevent.h"

#include <stdio.h>
//#include <stdlib.h>
#include <string.h>

/////YTimer *TaskBarApp::fRaiseTimer = 0;
//YTimer *WorkspaceButton::fRaiseTimer = 0;

WorkspaceButton::WorkspaceButton(WorkspacesPane *root, long ws, YWindow *parent): YButton(parent, 0)
{
    fRoot = root;
    fWorkspace = ws;
    //setDND(true);
}

void WorkspaceButton::eventClick(const YButtonEvent &/*up*/) {
}

void WorkspaceButton::handleDNDEnter(int /*nTypes*/, XAtomId * /*types*/) {
#if 0
    if (fRaiseTimer == 0)
        fRaiseTimer = new YTimer(autoRaiseDelay);
    if (fRaiseTimer) {
        fRaiseTimer->setTimerListener(this);
        fRaiseTimer->startTimer();
    }
    repaint();
#endif
}

void WorkspaceButton::handleDNDLeave() {
#if 0
    if (fRaiseTimer && fRaiseTimer->getTimerListener() == this) {
        fRaiseTimer->stopTimer();
        fRaiseTimer->setTimerListener(0);
    }
    repaint();
#endif
}

bool WorkspaceButton::handleTimer(YTimer * /*t*/) {
#if 0
    if (t == fRaiseTimer) {
        fRoot->activateWorkspace(fWorkspace);
    }
#endif
    return false;
}

void WorkspaceButton::actionPerformed(YAction */*action*/, unsigned int modifiers) {
    if (modifiers & ShiftMask) {
#if 0
        fRoot->switchToWorkspace(fWorkspace, true);
#endif
    } else if (modifiers & YEvent::mAlt) {
#if 0
        if (fRoot->getFocus())
            fRoot->getFocus()->wmOccupyWorkspace(fWorkspace);
#endif
    } else {
        fRoot->activateWorkspace(fWorkspace);
        return;
    }
}

WorkspacesPane::WorkspacesPane(YWindow *parent): YWindow(parent) {
    long w;

    fWorkspaceCount = workspaceCount();
    if (fWorkspaceCount > 1)
        fWorkspaceButton = new WorkspaceButton *[fWorkspaceCount];
    else
        fWorkspaceButton = 0;

#if 1
    if (fWorkspaceButton) {
        int ht = 0;
        int leftX = 0;

        for (w = 0; w < fWorkspaceCount; w++) {
            WorkspaceButton *wk = new WorkspaceButton(this, w, this);
            if (wk) {
                wk->__setText(workspaceName(w));
                if ((int)wk->height() > ht) ht = wk->height();
            }
            fWorkspaceButton[w] = wk;
        }
        for (w = 0; w < fWorkspaceCount; w++) {
            YButton *wk = fWorkspaceButton[w];
            //leftX += 2;
            if (wk) {
                wk->setSize(wk->width(), ht);
                wk->setPosition(leftX, 0); // + (ht - ADD - wk->height()) / 2);
                wk->show();
                leftX += wk->width();
            }
        }
        setSize(leftX, ht);
    }
#endif
}

WorkspacesPane::~WorkspacesPane() {
    if (fWorkspaceButton) {
        for (long w = 0; w < fWorkspaceCount; w++)
            delete fWorkspaceButton[w];
        delete [] fWorkspaceButton;
    }
}

WorkspaceButton *WorkspacesPane::workspaceButton(long n) {
    if (!fWorkspaceButton)
        return 0;
    return fWorkspaceButton[n];
}

long WorkspacesPane::workspaceCount() {
    return 4; // !!!
}

const char *WorkspacesPane::workspaceName(long workspace) {
    static char name[16]; // !!! temp
    sprintf(name, " %ld ", workspace);
    return name;
}

extern Time lastEventTime;

void WorkspacesPane::activateWorkspace(long workspace) {
#ifdef GNOME1_HINTS
    XClientMessageEvent xev;

    memset(&xev, 0, sizeof(xev));
    xev.type = ClientMessage;
    xev.window = desktop->handle();
    xev.message_type = _XA_WIN_WORKSPACE;
    xev.format = 32;
    xev.data.l[0] = workspace;
    xev.data.l[1] = lastEventTime; //CurrentTime; //xev.data.l[1] = timeStamp;
    XSendEvent(app->display(), desktop->handle(), False, SubstructureNotifyMask, (XEvent *) &xev);
#endif
}
