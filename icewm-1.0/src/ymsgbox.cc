/*
 * IceWM
 *
 * Copyright (C) 1999-2001 Marko Macek
 *
 * MessageBox
 */
#include "config.h"

#ifndef LITE

#include "ylib.h"
#include "ymsgbox.h"

#include "WinMgr.h"
#include "yapp.h"
#include "wmframe.h"
#include "sysdep.h"

#include "intl.h"

YMsgBox::YMsgBox(int buttons, YWindow *owner): YDialog(owner) {
    fListener = 0;
    fButtonOK = 0;
    fButtonCancel = 0;
    fLabel = new YLabel(0, this);
    fLabel->show();

    toplevel(true);

    if (buttons & mbOK) {
        fButtonOK = new YActionButton(this);
        if (fButtonOK) {
            fButtonOK->text(_("OK"));
            fButtonOK->actionListener(this);
            fButtonOK->show();
        }
    }
    if (buttons & mbCancel) {
        fButtonCancel = new YActionButton(this);
        if (fButtonCancel) {
            fButtonCancel->text(_("Cancel"));
            fButtonCancel->actionListener(this);
            fButtonCancel->show();
        }
    }
    autoSize();
    winLayer(WinLayerAboveDock);
    winState(WinStateAllWorkspaces, WinStateAllWorkspaces);
    winHints(WinHintsSkipWindowMenu);
    mwmHints(MWM_FUNC_MOVE | MWM_FUNC_CLOSE,
             MWM_DECOR_BORDER | MWM_DECOR_TITLE | MWM_DECOR_MENU);
}

YMsgBox::~YMsgBox() {
    delete fLabel; fLabel = 0;
    delete fButtonOK; fButtonOK = 0;
    delete fButtonCancel; fButtonCancel = 0;
}

void YMsgBox::autoSize() {
    unsigned lw = fLabel ? fLabel->width() : 0;
    unsigned w = lw + 24, h;

    w = clamp(w, 240U, desktop->width());
    
    h = 12;
    if (fLabel) {
        fLabel->position((w - lw) / 2, h);
        h += fLabel->height();
    }
    h += 18;
    
    unsigned const hh(max(fButtonOK ? fButtonOK->height() : 0,
    			  fButtonCancel ? fButtonCancel->height() : 0));
    unsigned const ww(max(fButtonOK ? fButtonOK->width() : 0,
    			  fButtonCancel ? fButtonCancel->width() : 0) + 3);

    if (fButtonOK) {
        fButtonOK->size(ww, hh);
        fButtonOK->position((w - hh)/2 - fButtonOK->width(), h);
    }
    if (fButtonCancel) {
        fButtonCancel->size(ww, hh);
        fButtonCancel->position((w + hh)/2, h);
    }

    h += fButtonOK ? fButtonOK->height() :
         fButtonCancel ? fButtonCancel->height() : 0;
    h += 12;
    
    size(w, h);
}

void YMsgBox::title(const char *title) {
    windowTitle(title);
    autoSize();
}

void YMsgBox::text(const char *text) {
    if (fLabel)
        fLabel->text(text);
    autoSize();
}

void YMsgBox::pixmap(YPixmap */*pixmap*/) {
}

void YMsgBox::actionPerformed(YAction *action, unsigned int /*modifiers*/) {
    if (fListener) {
        if (action == fButtonOK) {
            fListener->handleMsgBox(this, mbOK);
        } else if (action == fButtonCancel) {
            fListener->handleMsgBox(this, mbCancel);
        }
    }
}

void YMsgBox::handleClose() {
    fListener->handleMsgBox(this, 0);
}

void YMsgBox::handleFocus(const XFocusChangeEvent &/*focus*/) {
}

void YMsgBox::showFocused() {
    if (NULL == frame())
        manager->manageClient(handle(), false);

    if (frame()) {
        frame()->position(desktop->width() / 2 - frame()->width() / 2,
                          desktop->height() / 2 - frame()->height() / 2);
        frame()->activate(true);
	
	switch(msgBoxDefaultAction) {
	    case 0:
		if (fButtonCancel) fButtonCancel->requestFocus();
		break;
	    case 1:
		if (fButtonOK) fButtonOK->requestFocus();
		break;
        }
    }
}
#endif
