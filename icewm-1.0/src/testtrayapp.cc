#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/xpm.h>
#include <X11/extensions/shape.h>
#include <iostream>

static Display * dpy(NULL);

static Window traywin(None), appwin(None);
static Atom XA_WM_PROTOCOLS(None);
static Atom XA_WM_DELETE_WINDOW(None);
static Atom XA_KDE_NET_WM_SYSTEM_TRAY_WINDOW_FOR(None);
static int width, height;

static XImage * gsource(NULL);
static Pixmap pixmap(None), mask(None);

int init(int argc, char ** argv) {
    if ((dpy = XOpenDisplay(NULL)) == NULL) return 1;

    XSetWindowAttributes attr;
    attr.event_mask = StructureNotifyMask|ExposureMask;

    if ((traywin = XCreateWindow(dpy, DefaultRootWindow(dpy),
			         0, 0, width = 16, height = 16, 1,
    			         CopyFromParent, InputOutput, CopyFromParent,
			         CWEventMask, &attr)) == None) return 2;

    char * filename(argc == 1 ? (char *) "../lib/icons/vim_16x16.xpm"
                              : argv[1]);

    if (XpmReadFileToPixmap(dpy, traywin, filename, &pixmap, &mask, NULL))
	return 4;


    XA_WM_PROTOCOLS = XInternAtom(dpy, "WM_PROTOCOLS", False);
    XA_WM_DELETE_WINDOW = XInternAtom(dpy, "WM_DELETE_WINDOW", False);
    XA_KDE_NET_WM_SYSTEM_TRAY_WINDOW_FOR =
        XInternAtom(dpy, "_KDE_NET_WM_SYSTEM_TRAY_WINDOW_FOR", False);

    XClassHint wmclass;
    wmclass.res_name = *argv;
    wmclass.res_class = *argv;

    XChangeProperty(dpy, traywin, XA_KDE_NET_WM_SYSTEM_TRAY_WINDOW_FOR,
                    XA_WINDOW, 32, PropModeReplace,
                    (unsigned char *)&appwin, 1);

    XShapeCombineMask(dpy, traywin, ShapeBounding, 0, 0, mask, ShapeSet);
    XSetWMProtocols(dpy, traywin, &XA_WM_DELETE_WINDOW, 1);
    XSetClassHint(dpy, traywin, &wmclass);
    XSetCommand(dpy, traywin, argv, argc);
    XStoreName(dpy, traywin, "Test: Tray Window");
    
    XMapWindow(dpy, traywin);

    return -1;
}

int main(int argc, char * argv[]) {
    int rc;
    
    for (rc = init(argc, argv); rc == -1; ) {
	XEvent ev;

	XNextEvent(dpy, &ev);

	switch (ev.type) {
	    case ConfigureNotify:
		if (width != ev.xconfigure.width ||
		    height != ev.xconfigure.height) {
		    width = ev.xconfigure.width;
		    height = ev.xconfigure.height;
		}

		break;
		
	    case Expose:
		XCopyArea(dpy, pixmap, ev.xexpose.window,
			  DefaultGC(dpy, DefaultScreen(dpy)),
			  ev.xexpose.x, ev.xexpose.y,
			  ev.xexpose.width, ev.xexpose.height,
			  ev.xexpose.x, ev.xexpose.y);

		break;

	    case ClientMessage:
		if (ev.xclient.message_type == XA_WM_PROTOCOLS &&
		    ev.xclient.data.l[0] == XA_WM_DELETE_WINDOW)
		    rc = 0;

		break;
	}
    }

    if (rc) std::cerr << "Failed with rc=" << rc << std::endl;
    if (dpy) XCloseDisplay(dpy);
    
    return rc;
}
