/*
 * IceWM
 *
 * Copyright (C) 1997-2001 Marko Macek
 *
 * Dialogs
 */
#include "config.h"

#ifndef LITE
#include "ylib.h"
#include "wmabout.h"

#include "prefs.h"
#include "wmapp.h"
#include "wmframe.h"
#include "sysdep.h"
#include "WinMgr.h"

#include "intl.h"

AboutDlg *aboutDlg(NULL);

AboutDlg::AboutDlg():
YDialog() {
    char const * version(YWindowManager::name());
    char * copyright(strJoin("Copyright ", _("(C)"), " 1997-2001 Marko Macek\n",
                             "Copyright ", _("(C)"), " 2001 Mathias Hasselmann",
                             NULL));

    fProgTitle = new YLabel(version, this);
    fCopyright = new YLabel(copyright, this);
    delete[] copyright;

    fThemeNameS = new YLabel(_("Theme:"), this);
    fThemeDescriptionS = new YLabel(_("Theme Description:"), this);
    fThemeAuthorS = new YLabel(_("Theme Author:"), this);
    fThemeName = new YLabel(themeName, this);
    fThemeDescription = new YLabel(themeDescription, this);
    fThemeAuthor = new YLabel(themeAuthor, this);
    autoSize();
    fProgTitle->show();
    fCopyright->show();
    fThemeNameS->show();
    fThemeName->show();
    fThemeDescriptionS->show();
    fThemeDescription->show();
    fThemeAuthorS->show();
    fThemeAuthor->show();

    windowTitle(_("icewm - About"));
    //iconTitle("icewm - About");
    winLayer(WinLayerAboveDock);
    winState(WinStateAllWorkspaces, WinStateAllWorkspaces);
    winHints(WinHintsSkipWindowMenu);
    mwmHints(MWM_FUNC_MOVE | MWM_FUNC_CLOSE,
             MWM_DECOR_BORDER | MWM_DECOR_TITLE | MWM_DECOR_MENU);
}

#define RX(w) (int((w)->x() + (w)->width()))
#define XMAX(x,nx) ((nx) > (x) ? (nx) : (x))

void AboutDlg::autoSize() {
    int dx = 20, dx1 = 20;
    int dy = 20;
    int W = 0, H;
    int cy;

    fProgTitle->position(dx, dy); dy += fProgTitle->height();
    W = XMAX(W, RX(fProgTitle));
    dy += 4;
    fCopyright->position(dx, dy); dy += fCopyright->height();
    W = XMAX(W, RX(fCopyright));
    dy += 20;

    fThemeNameS->position(dx, dy);
    fThemeDescriptionS->position(dx, dy);
    fThemeAuthorS->position(dx, dy);
    
    dx = XMAX(dx, RX(fThemeNameS));
    dx = XMAX(dx, RX(fThemeDescriptionS));
    dx = XMAX(dx, RX(fThemeAuthorS));
    dx += 20;

    fThemeNameS->position(dx1, dy);
    cy = fThemeNameS->height();
    W = XMAX(W, RX(fThemeName));
    fThemeName->position(dx, dy);
    cy = XMAX(cy, int(fThemeName->height()));
    W = XMAX(W, RX(fThemeName));
    dy += cy;
    dy += 4;
    
    fThemeDescriptionS->position(dx1, dy);
    cy = fThemeDescriptionS->height();
    W = XMAX(W, RX(fThemeDescriptionS));
    fThemeDescription->position(dx, dy);
    cy = XMAX(cy, int(fThemeDescription->height()));
    W = XMAX(W, RX(fThemeDescription));

    dy += cy;
    dy += 4;
    
    fThemeAuthorS->position(dx1, dy);
    cy = fThemeAuthorS->height();
    W = XMAX(W, RX(fThemeAuthorS));
    fThemeAuthor->position(dx, dy);
    cy = XMAX(cy, int(fThemeAuthor->height()));
    W = XMAX(W, RX(fThemeAuthor));
    dy += cy;

    H = dy + 20;
    
    W += 20;
    size(W, H);
}

void AboutDlg::showFocused() {
    if (NULL == frame())
        manager->manageClient(handle(), false);

    if (frame()) {
        frame()->position(desktop->width() / 2 - frame()->width() / 2,
                             desktop->height() / 2 - frame()->height() / 2);
        frame()->activate(true);
    }
}

void AboutDlg::handleClose() {
    if (!frame()->isHidden()) frame()->wmHide();
}
#endif
