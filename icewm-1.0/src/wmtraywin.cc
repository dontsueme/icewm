/**
 *  IceWM - KDE tray window container
 *  Copyright (C) 2001 The Authors of IceWM
 *
 *  Release under terms of the GNU Library General Public License
 */

#include "config.h"

#if CONFIG_KDE_TRAY_WINDOWS
#include "wmtraywin.h"
#include "wmframe.h"
#include "wmtaskbar.h"
#include "wmapp.h" // !?
#include "aaddressbar.h" // !?

/// !!! monitor property changes in YClientWindow!!!

YTrayWindow::YTrayWindow(YWindow *parent, YFrameWindow *owner,
                         YFrameClient *client):
YFrameProxy(parent, owner),
fClient(client), fOwner(owner) {
msg(__PRETTY_FUNCTION__);
    toolTip(title());
    client->toolTip(title());
    manager->addTrayWindow(this);
}

YTrayWindow::~YTrayWindow() {
msg(__PRETTY_FUNCTION__);

        manager->removeTrayWindow(this);
if (taskBar && taskBar->trayPane())        
        taskBar->trayPane()->remove(this);

    if (NULL != fClient) {
msg("hrm");    
        if (!fClient->destroyed())
            XRemoveFromSaveSet(app->display(), client()->handle());

msg("hrm^2");    
        XDeleteContext(app->display(), client()->handle(),
                       YWindowManager::trayContext);
    }

client()->hide();
client()->reparent(manager, 0, 0);
if (phase != phaseRestart)
    client()->wmState(WithdrawnState);
client()->show();

client()->trayWindow(NULL);
delete fClient;
fClient = NULL;


if (taskBar && taskBar->trayPane()) {
        TrayPane *tp = taskBar->trayPane();
	int const nw(tp->requiredWidth());
        int dw;

        if ((dw = nw - tp->width()))
            taskBar->trayPane()->geometry
		(tp->x() - dw, tp->y(), nw, tp->height());
/*
	if (dw) taskBar->taskPane()->size
	    (taskBar->taskPane()->width() - dw, taskBar->taskPane()->height());

        taskBar->taskPane()->relayout();
*/
//    taskBar->updateProxyPanes();

    if (dw && NULL == taskBar->taskPane() && NULL != taskBar->addressBar())
	taskBar->addressBar()->size
	    (taskBar->addressBar()->width() - dw,
	     taskBar->addressBar()->height());
}             

}

void YTrayWindow::popupSystemMenu() {
    msg("go");
}

void YTrayWindow::popupSystemMenu(int /*x*/, int /*y*/,
                         int /*x_delta*/, int /*y_delta*/,
                         unsigned int /*flags*/,
                         YWindow */*forWindow*/) {
    msg("GO2");
}


#endif // CONFIG_KDE_TRAY_WINDOWS
