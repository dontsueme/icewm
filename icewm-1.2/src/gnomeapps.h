#ifndef __GNOMEAPPS_H
#define __GNOMEAPPS_H

#ifdef CONFIG_GNOME_MENUS

#include "objmenu.h"
#include <gnome.h>
#include "obj.h"

class DGnomeDesktopEntry: public DObject {
public:
    DGnomeDesktopEntry(const char * name, YIcon * icon,
    		       GnomeDesktopEntry * dentry);
    ~DGnomeDesktopEntry();

    virtual void open();
private:
    GnomeDesktopEntry * fEntry;
};

class GnomeMenu: public ObjectMenu {
public:
    GnomeMenu(YWindow * parent, const char * path);
    virtual ~GnomeMenu();
    
    virtual void updatePopup();
    
    static void createToplevel(ObjectMenu * menu, const char * path);
    static void createSubmenu(ObjectMenu * menu, const char * path,
                              const char * name, YIcon::Image * icon);

private:
    void populateMenu(ObjectMenu *target);
    void addEntry(const char *name, const int plen, ObjectMenu * target,
                  const int firstItem = 0, const bool firstRun = true);

    char * fPath;
    time_t fModTime;

    static YIcon::Image * folderIcon;
};
#endif

#endif
