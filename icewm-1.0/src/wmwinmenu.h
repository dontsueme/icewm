#ifndef __WMWINMENU_H
#define __WMWINMENU_H

class WindowListMenu: public YMenu {
public:
    WindowListMenu(YWindow *parent = NULL): YMenu(parent) {}
    YMenu *populate(YMenu *menu, icewm::Workspace workspace);
    virtual void updatePopup();
};

#endif
