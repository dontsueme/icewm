#include "config.h"

#ifdef CONFIG_TASKBAR

#include "ylib.h"
#include "aworkspaces.h"
#include "wmtaskbar.h"
#include "prefs.h"
#include "wmmgr.h"
#include "wmapp.h"
#include "wmframe.h"

#include "intl.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <assert.h>
#include <errno.h>

#include "base.h"

YColor * WorkspaceButton::normalButtonBg(NULL);
YColor * WorkspaceButton::normalButtonFg(NULL);

YColor * WorkspaceButton::activeButtonBg(NULL);
YColor * WorkspaceButton::activeButtonFg(NULL);

YFont * WorkspaceButton::normalButtonFont(NULL);
YFont * WorkspaceButton::activeButtonFont(NULL);

YPixmap *workspacebuttonPixmap(NULL);
YPixmap *workspacebuttonactivePixmap(NULL);

#ifdef CONFIG_GRADIENTS
class YPixbuf *workspacebuttonPixbuf(NULL);
class YPixbuf *workspacebuttonactivePixbuf(NULL);
#endif

WorkspaceButton::WorkspaceButton(long ws, YWindow *parent): YButton(parent, 0)
{
    fWorkspace = ws;
    //dnd(true);
}

void WorkspaceButton::handleClick(const XButtonEvent &up, int count) {
    if (up.button == 3 && count == 1 && IS_BUTTON(up.state, Button3Mask))
        taskBar->contextMenu(up.x_root, up.y_root);
}

void WorkspaceButton::handleDNDEnter() {
    if (fRaiseTimer == 0)
        fRaiseTimer = new YTimer(autoRaiseDelay);
    if (fRaiseTimer) {
        fRaiseTimer->timerListener(this);
        fRaiseTimer->start();
    }
    repaint();
}

void WorkspaceButton::handleDNDLeave() {
    if (fRaiseTimer && fRaiseTimer->timerListener() == this) {
        fRaiseTimer->stop();
        fRaiseTimer->timerListener(NULL);
    }
    repaint();
}

bool WorkspaceButton::handleTimer(YTimer *t) {
    if (t == fRaiseTimer) {
        manager->activateWorkspace(fWorkspace);
    }
    return false;
}

void WorkspaceButton::actionPerformed(YAction */*action*/, unsigned int modifiers) {
    if (modifiers & ShiftMask) {
        manager->switchToWorkspace(fWorkspace, true);
    } else if (modifiers & app->AltMask) {
        if (manager->focus())
            manager->focus()->wmOccupyWorkspace(fWorkspace);
    } else {
        manager->activateWorkspace(fWorkspace);
        return;
    }
}

WorkspacesPane::WorkspacesPane(YWindow *parent): YWindow(parent) {
    if (workspaceCount > 1)
        fWorkspaceButton = new WorkspaceButton *[workspaceCount];
    else
        fWorkspaceButton = 0;

    if (fWorkspaceButton) {
	YResourcePaths paths("", false);

        int ht = 0;
        int leftX = 0;

        for (icewm::Workspace w(0); w < workspaceCount; ++w) {
            WorkspaceButton *wk = new WorkspaceButton(w, this);
            if (wk) {
		YIcon::Image * image
		    (paths.loadImage("workspace/", workspaceNames[w]));

		if (image) wk->image(image);
                else wk->text(workspaceNames[w]);
		
		char * wn(newstr(basename(workspaceNames[w])));
		char * ext(strrchr(wn, '.'));
		if (ext) *ext = '\0';
		
		char * tt(strJoin(_("Workspace: "), wn, NULL));
		delete[] wn;

		wk->toolTip(tt);
		delete[] tt;
		
                if ((int)wk->height() + 1 > ht) ht = wk->height() + 1;
            }
            fWorkspaceButton[w] = wk;
        }

        for (icewm::Workspace w(0); w < workspaceCount; ++w) {
            YButton *wk = fWorkspaceButton[w];
            //leftX += 2;
            if (wk) {
                wk->size(wk->width(), ht);
                wk->position(leftX, 0); // + (ht - ADD - wk->height()) / 2);
                wk->show();
                leftX += wk->width();
            }
        }
        size(leftX, ht);
    }
}

WorkspacesPane::~WorkspacesPane() {
    if (fWorkspaceButton) {
        for (icewm::Workspace w(0); w < workspaceCount; ++w)
            delete fWorkspaceButton[w];
        delete [] fWorkspaceButton;
    }
}

WorkspaceButton *WorkspacesPane::workspaceButton(long n) {
    return (fWorkspaceButton ? fWorkspaceButton[n] : NULL);
}

YFont * WorkspaceButton::font() {
    return isPressed()
    	? *activeWorkspaceFontName
	  ? activeButtonFont
	    ? activeButtonFont
	    : activeButtonFont = YFont::font(activeWorkspaceFontName)
	  : YButton::font()
    	: *normalWorkspaceFontName
	  ? normalButtonFont
	    ? normalButtonFont
	    : normalButtonFont = YFont::font(normalWorkspaceFontName)
	  : YButton::font();
}

YColor * WorkspaceButton::color() {
    return isPressed()
    	? *clrWorkspaceActiveButtonText
	  ? activeButtonFg
	    ? activeButtonFg
	    : activeButtonFg = new YColor(clrWorkspaceActiveButtonText)
	  : YButton::color()
    	: *clrWorkspaceNormalButtonText
	  ? normalButtonFg
	    ? normalButtonFg
	    : normalButtonFg = new YColor(clrWorkspaceNormalButtonText)
	  : YButton::color();
}

YSurface WorkspaceButton::surface() {
    if (activeButtonBg == 0)
        activeButtonBg = new YColor(*clrWorkspaceActiveButton
			? clrWorkspaceActiveButton : clrActiveButton);
    if (normalButtonBg == 0)
        normalButtonBg = new YColor(*clrWorkspaceNormalButton
			? clrWorkspaceNormalButton : clrNormalButton);

#ifdef CONFIG_GRADIENTS    
    return (isPressed() ? YSurface(activeButtonBg, 
				   workspacebuttonactivePixmap, 
				   workspacebuttonactivePixbuf)
    		        : YSurface(normalButtonBg,
		     		   workspacebuttonPixmap, 
				   workspacebuttonPixbuf));
#else		     
    return (isPressed() ? YSurface(activeButtonBg, workspacebuttonactivePixmap)
    		        : YSurface(normalButtonBg, workspacebuttonPixmap));
#endif
}

#endif
