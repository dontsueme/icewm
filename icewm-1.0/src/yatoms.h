/*
 *  IceWM - Definition of all X11 atoms supported
 *
 *  Copyright (C) 2001 The Authors of IceWM
 *
 *  Released under terms of the GNU Library General Public License
 */

#ifndef __YATOM_H
#define __YATOM_H

#include <X11/X.h>

struct YAtomInfo {
    Atom * atom;
    char const * name;
};

struct YAtoms {
    void init();

/******************************************************************************
 * ICCCM (Inter-Client Communication Conventions Manual)
 * ftp://ftp.x.org/pub/R6.4/xc/doc/hardcopy/ICCCM/icccm.PS.gz
 * file:///usr/src/xc/doc/hardcopy/ICCCM/icccm.PS.gz
 ******************************************************************************/

    Atom clipboard,                     // ICCCM 2.0: 2.6.1.3
         targets,                       // ICCCM 2.0: 2.6.2
         wmProtocols,                   // ICCCM 2.0: 4.1.2.7
         wmDeleteWindow,                // ICCCM 2.0: 4.1.2.7, 4.1.7
         wmTakeFocus,                   // ICCCM 2.0: 4.1.2.7, 4.2.8.1
         wmColormapWindows,             // ICCCM 2.0: 4.1.2.8, 4.1.8
         wmName,                        // ICCCM 2.0:
         wmState,                       // ICCCM 2.0: 4.1.3.1
         wmChangeState,                 // ICCCM 2.0: 4.1.4
         smClientId,                    // ICCCM 2.0: 5.1
         wmClientLeader;                // ICCCM 2.0: 5.1

/******************************************************************************
 * Xdnd (Drag-and-Drop Protocol for the X Window System)
 * http://www.newplanetsoftware.com/xdnd/
 ******************************************************************************/

#ifdef CONFIG_XDND_HINTS
    Atom xdndAware,
         xdndEnter,
         xdndLeave,
         xdndPosition,
         xdndStatus,
         xdndDrop,
         xdndFinished,
         xdndSelection,
         xdndTypelist;
#endif // CONFIG_XDND_HINTS

/******************************************************************************
 * Motif related atoms
 ******************************************************************************/

#ifdef CONFIG_MOTIF_HINTS
    Atom mwmHints;                     // ???
#endif // CONFIG_MOTIF_HINTS

/******************************************************************************
 * GNOME (GNU Network Object Model Environment)
 * http://developer.gnome.org/doc/standards/wm/book1.html
 ******************************************************************************/

#ifdef CONFIG_GNOME_HINTS
    Atom winProtocols,
         winSupportingWmCheck,
         winIcons,
         winWorkspace,
         winWorkspaceCount,
         winWorkspaceNames,
         winWorkspaces,
         winWorkspacesAdd,
         winWorkspacesRemove,
         winLayer,
         winHints,
         winState,
         winWorkarea,
         winClientList,
         winDesktopButtonProxy,
         winArea,
         winAreaCount;
#endif // CONFIG_GNOME_HINTS

/******************************************************************************
 * KDE (K Desktop Environment)
 ******************************************************************************/

#ifdef CONFIG_KDE_HINTS
    Atom kwmWinIcon;
#ifdef CONFIG_TRAY
    Atom kwmDockwindow;
#ifdef CONFIG_WMSPEC_HINTS
    Atom kdeNetSystemTrayWindows,
         kdeNetwmSystemTrayWindowFor;
#endif // CONFIG_TRAY
#endif // CONFIG_WMSPEC_HINTS
#endif // CONFIG_KDE_HINTS

/******************************************************************************
 * wm-spec (Window Manager Specification aka NetWM)
 * http://www.freedesktop.org/standards/wm-spec.html
 ******************************************************************************/

#ifdef CONFIG_WMSPEC_HINTS
    Atom netSupported,
         netClientList,
         netClientListStacking,
         netNumberOfDesktops,
         netDesktopGeometry,
         netDesktopViewport,
         netCurrentDesktop,
         netCurrentDesktopNames,
         netActiveWindow,
         netWorkarea,
         netSupportingWmCheck,

         netCloseWindow,
         netwmMoveResize,

         netwmName,
         netwmIconName,
         netwmDesktop,
         netwmWindowType,
         netwmWindowTypeDesktop,
         netwmWindowTypeDock,
         netwmWindowTypeToolbar,
         netwmWindowTypeMenu,
         netwmWindowTypeDialog,
         netwmWindowTypeNormal,
         netwmState,
         netwmStateModal,
         netwmStateSticky,
         netwmStateMaximizedVert,
         netwmStateMaximizedHorz,
         netwmStateShaded,
         netwmStateSkipTaskbar,
         netwmStateSkipPager,
         netwmStateFullscreen,

         netwmStrut,
         netwmIconGeometry,
         netwmIcon,
         netwmPid,
         netwmPing,
         netwmHandledIcons;
#endif // CONFIG_WMSPEC_HINTS

/******************************************************************************
 * Semitransparency
 ******************************************************************************/

    Atom xrootPixmapId,
         xrootColorPixel;

/******************************************************************************
 * IceWM specific
 * http://www.icewm.org/
 ******************************************************************************/

#ifndef NO_WINDOW_OPTIONS
    Atom icewmWinOpt;                   // store winoptions
#endif
#ifdef CONFIG_TRAY
    Atom icewmTrayOpt;                  // store tray settings
#endif
#ifdef CONFIG_GUIEVENTS
    Atom icewmGuiEvent;                 // signal gui events
#endif
#ifdef CONFIG_WM_SESSION
    Atom icewmPid;                      // process id
#endif
    Atom icewmFontPath;                 // additions to the font path
};

extern YAtoms atoms;

#endif
