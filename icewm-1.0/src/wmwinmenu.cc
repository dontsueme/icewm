/*
 * IceWM
 *
 * Copyright (C) 1997-2001 Marko Macek
 */
#include "config.h"

#ifdef CONFIG_WINMENU

#include "wmaction.h"
#include "ylib.h"
#include "ymenu.h"
#include "ymenuitem.h"
#include "yaction.h"
#include "sysdep.h"
#include "prefs.h"

#include "wmmgr.h"
#include "wmframe.h"
#include "wmwinmenu.h"

#include "intl.h"

class ActivateWindowMenuItem: public YMenuItem, public YAction {
public:
    ActivateWindowMenuItem(YFrameWindow *frame): 
    YMenuItem(frame->title(), -1, 0, this, 0),
    fFrame(frame) {
        if (fFrame->updateClientIcon())
            icon(fFrame->updateClientIcon()->small());
    }

    virtual void actionPerformed(YAction::Listener * /*listener*/,
                                 YAction * /*action*/, unsigned int modifiers) {
        YFrameWindow *f = manager->topLayer();

        while (f) {
            if ((void *)f == fFrame) {
                if (modifiers & ShiftMask)
                    f->wmOccupyOnlyWorkspace(manager->activeWorkspace());
                manager->activate(f, true);
                return ;
            }
            f = f->nextLayer();
        }
    }
private:
    YFrameWindow *fFrame;
};


YMenu *WindowListMenu::populate(YMenu *menu, icewm::Workspace workspace) {
    bool needSeparator = false;

    /// !!! fix performance (smarter update, on change only)
    if (workspace == manager->activeWorkspace())
        for (icewm::Layer layer(0); layer < WinLayerCount; ++layer)
            if (manager->top(layer)) {
                bool needLayerSeparator(layer > 0);

                for (unsigned level(0); level < 4; ++level) {
                    bool needLevelSeparator(level > 0);

                    for (YFrameWindow *frame = manager->top(layer);
                         frame; frame = frame->next()) {
                        unsigned const windowLevel(frame->isHidden() ? 3 :
                                                   frame->isMinimized() ? 2 :
                                                   frame->isRollup() ? 1 : 0);

                        if (frame->client()->adopted() &&
                            frame->visibleOn(workspace) &&
                           !frame->ignoreWinList() &&
                            level == windowLevel) {

                            if ((needLevelSeparator ||
                                 needLayerSeparator) && needSeparator) {
                                menu->addSeparator();
                                needLevelSeparator =
                                needLayerSeparator = false;
                            }

                            menu->add(new ActivateWindowMenuItem(frame));
                            needSeparator = true;
                        }
                    }
            }
        }

    return menu;
}

void WindowListMenu::updatePopup() {
    removeAll();

    populate(this, manager->activeWorkspace()); // !!! fix

    bool first(true);
    for (icewm::Workspace workspace(0);
         workspace < manager->workspaceCount(); ++workspace) 
        if (workspace != manager->activeWorkspace()) {
            if (first) {
                addSeparator();
                first = false;
            }

            YMenu *submenu(populate(new YMenu(), workspace));
            if (submenu->itemCount()) {
                char label[64 + 32];
                sprintf(label, _("%lu. Workspace %-.32s"),
                        workspace + 1, manager->workspaceName(workspace));

                addItem(label, workspace < 10 ? 0 : -1,
                        workspaceActionActivate[workspace], submenu);
            } else
                delete submenu;
    }

#ifdef CONFIG_WINLIST
    addSeparator();
    addItem(_("_Window list"), -2, KEY_NAME(gKeySysWindowList), actionWindowList);
#endif
}

#endif
