#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/xpm.h>
#include <X11/keysym.h>
#include <X11/extensions/shape.h>
#include <stdio.h>
#include <string.h>


#define DEFAULT_FONT "-adobe-helvetica-medium-r-normal-*-*-120-*"
#define DEFAULT_ICON ((char *)"../lib/taskbar/icewm.xpm")

static Display * dpy(NULL);

static Window traywin(None), appwinA(None), appwinB(None);
static Atom XA_WM_PROTOCOLS(None);
static Atom XA_WM_DELETE_WINDOW(None);
static Atom XA_KDE_NET_WM_SYSTEM_TRAY_WINDOW_FOR(None);
static int width, height;

static Pixmap pixmap(None), mask(None);
static XFontStruct * font;

inline void drawString(Window win, GC gc, int x, int y, char const * string) {
    XDrawString(dpy, win, gc, x, y, string, strlen(string));
}

void setTrayWindowFor(Window appwin) {
    XChangeProperty(dpy, traywin, XA_KDE_NET_WM_SYSTEM_TRAY_WINDOW_FOR,
                    XA_WINDOW, 32, PropModeReplace,
                    (unsigned char *)&appwin, 1);
}

int init(int argc, char ** argv) {
    if ((dpy = XOpenDisplay(NULL)) == NULL) return 1;

    XSetWindowAttributes attr;
    attr.event_mask = StructureNotifyMask|ExposureMask;

    attr.event_mask|= KeyReleaseMask;

    if ((appwinA = XCreateWindow(dpy, DefaultRootWindow(dpy),
			         0, 0, 300, 120, 1,
    			         CopyFromParent, InputOutput, CopyFromParent,
			         CWEventMask, &attr)) == None) return 2;
    if ((appwinB = XCreateWindow(dpy, DefaultRootWindow(dpy),
			         0, 0, 300, 120, 1,
    			         CopyFromParent, InputOutput, CopyFromParent,
			         CWEventMask, &attr)) == None) return 2;

    if ((traywin = XCreateWindow(dpy, DefaultRootWindow(dpy),
			         0, 0, width = 48, height = 20, 1,
    			         CopyFromParent, InputOutput, CopyFromParent,
			         CWEventMask, &attr)) == None) return 2;

    char * filename(argc == 1 ? DEFAULT_ICON : argv[1]);

    if (XpmReadFileToPixmap(dpy, traywin, filename, &pixmap, &mask, NULL))
	return 4;

    if (NULL == (font = XLoadQueryFont(dpy, DEFAULT_FONT)) &&
        NULL == (font = XLoadQueryFont(dpy, "fixed")))
        return 5;

    XA_WM_PROTOCOLS = XInternAtom(dpy, "WM_PROTOCOLS", False);
    XA_WM_DELETE_WINDOW = XInternAtom(dpy, "WM_DELETE_WINDOW", False);
    XA_KDE_NET_WM_SYSTEM_TRAY_WINDOW_FOR =
        XInternAtom(dpy, "_KDE_NET_WM_SYSTEM_TRAY_WINDOW_FOR", False);

    XClassHint wmclass;
    wmclass.res_name = *argv;
    wmclass.res_class = *argv;

    setTrayWindowFor(appwinA);

    XShapeCombineMask(dpy, traywin, ShapeBounding, 0, 0, mask, ShapeSet);
    XSetWMProtocols(dpy, traywin, &XA_WM_DELETE_WINDOW, 1);
    XSetClassHint(dpy, traywin, &wmclass);
    XSetCommand(dpy, traywin, argv, argc);
    XStoreName(dpy, traywin, "testtrayapp: Tray Window");


    XSetWMProtocols(dpy, appwinA, &XA_WM_DELETE_WINDOW, 1);
    XSetClassHint(dpy, appwinA, &wmclass);
    XSetCommand(dpy, appwinA, argv, argc);
    XStoreName(dpy, appwinA, "testtrayapp: First main window");
    XMapWindow(dpy, appwinA);

    XSetWMProtocols(dpy, appwinB, &XA_WM_DELETE_WINDOW, 1);
    XSetClassHint(dpy, appwinB, &wmclass);
    XSetCommand(dpy, appwinB, argv, argc);
    XStoreName(dpy, appwinB, "testtrayapp: Second main window");
    XMapWindow(dpy, appwinB);

puts("BUG in IceWM: has to be position independant");
    XMapWindow(dpy, traywin); // BUG!

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
                if (traywin == ev.xexpose.window) {
		    XCopyArea(dpy, pixmap, ev.xexpose.window,
			      DefaultGC(dpy, DefaultScreen(dpy)),
			      ev.xexpose.x, ev.xexpose.y,
			      ev.xexpose.width, ev.xexpose.height,
			      ev.xexpose.x, ev.xexpose.y);
                } else {
                    GC gc(XCreateGC(dpy, ev.xexpose.window, 0, NULL));
                    XSetForeground(dpy, gc, WhitePixel(dpy, DefaultScreen(dpy)));
                    XFillRectangle(dpy, ev.xexpose.window, gc,
                                   ev.xexpose.x, ev.xexpose.y,
			           ev.xexpose.width, ev.xexpose.height);

                    XSetForeground(dpy, gc, BlackPixel(dpy, DefaultScreen(dpy)));
                    XSetFont(dpy, gc, font->fid);
                    
                    int y;

                    drawString(ev.xexpose.window, gc,
                               10, y = 10 + font->ascent,
                               "Keyboard commands:");
                    drawString(ev.xexpose.window, gc,
                               20, y+= font->descent + font->ascent,
                               "0: tray window for None");
                    drawString(ev.xexpose.window, gc,
                               20, y+= font->descent + font->ascent,
                               "1: tray window for first main window");
                    drawString(ev.xexpose.window, gc,
                               20, y+= font->descent + font->ascent,
                               "2: tray window for second main window");
                    drawString(ev.xexpose.window, gc,
                               20, y+= font->descent + font->ascent,
                               "Delete: remove tray property");
                    drawString(ev.xexpose.window, gc,
                               20, y+= font->descent + font->ascent,
                               "m: map tray window");
                    drawString(ev.xexpose.window, gc,
                               20, y+= font->descent + font->ascent,
                               "u: unmap tray window");
                    
                    XFreeGC(dpy, gc);
                }

		break;

	    case KeyRelease:
                switch (XKeycodeToKeysym(dpy, ev.xkey.keycode, 0)) {
                    case XK_0:
                    case XK_KP_0:
                        printf("%lx is tray for None\n", traywin);
                        setTrayWindowFor(None);
                        break;

                    case XK_1:
                    case XK_KP_1:
                        printf("%lx is tray for %lx\n", traywin, appwinA);
                        setTrayWindowFor(appwinA);
                        break;

                    case XK_2:
                    case XK_KP_2:
                        printf("%lx is tray for %lx\n", traywin, appwinB);
                        setTrayWindowFor(appwinB);
                        break;

                    case XK_d:
                    case XK_Delete:
                    case XK_KP_Delete:
                        printf("removing tray property of %lx\n", traywin);
                        XDeleteProperty(dpy, traywin,
                                        XA_KDE_NET_WM_SYSTEM_TRAY_WINDOW_FOR);
                        break;
                        
                    case XK_m:
                        printf("mapping tray window %lx\n", traywin);
                        XMapWindow(dpy, traywin);
                        break;
                        
                    case XK_u:
                        printf("unmapping tray window %lx\n", traywin);
                        XUnmapWindow(dpy, traywin);
                        break;
                }
                break;

	    case ClientMessage:
		if (XA_WM_PROTOCOLS == ev.xclient.message_type &&
		    XA_WM_DELETE_WINDOW == (Atom) ev.xclient.data.l[0])
		    rc = 0;

		break;
	}
    }

    if (rc) fprintf(stderr, "Failed with rc=%d\n", rc);
    if (dpy) XCloseDisplay(dpy);

    return rc;
}
