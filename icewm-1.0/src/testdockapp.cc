#include <X11/Xlib.h>
#include <X11/xpm.h>
#include <X11/extensions/shape.h>
#include <stdio.h>

static Display * dpy(NULL);

static Window dockapp(None);
static Atom XA_WM_PROTOCOLS(None);
static Atom XA_WM_DELETE_WINDOW(None);
static int width, height;

static Pixmap pixmap(None), mask(None);

int init(int argc, char ** argv) {
    if ((dpy = XOpenDisplay(NULL)) == NULL) return 1;

    XSetWindowAttributes attr;
    attr.event_mask = StructureNotifyMask|ExposureMask;

    if ((dockapp = XCreateWindow(dpy, DefaultRootWindow(dpy),
			         0, 0, width = 48, height = 48, 1,
    			         CopyFromParent, InputOutput, CopyFromParent,
			         CWEventMask, &attr)) == None) return 2;

    char * filename(argc == 1 ? (char *) "../lib/icons/vim_48x48.xpm"
                              : argv[1]);

    if (XpmReadFileToPixmap(dpy, dockapp, filename, &pixmap, &mask, NULL))
	return 4;


    XA_WM_PROTOCOLS = XInternAtom(dpy, "WM_PROTOCOLS", False);
    XA_WM_DELETE_WINDOW = XInternAtom(dpy, "WM_DELETE_WINDOW", False);

    XClassHint wmclass;
    wmclass.res_name = *argv;
    wmclass.res_class = *argv;

    XWMHints wmhints;
    wmhints.initial_state = WithdrawnState;
    wmhints.icon_window = dockapp;
    wmhints.window_group = dockapp;
    wmhints.flags = StateHint | IconWindowHint/* | WindowGroupHint*/;

    XShapeCombineMask(dpy, dockapp, ShapeBounding, 0, 0, mask, ShapeSet);
    XSetWMProtocols(dpy, dockapp, &XA_WM_DELETE_WINDOW, 1);
    XSetClassHint(dpy, dockapp, &wmclass);
    XSetWMHints(dpy, dockapp, &wmhints);
    XSetCommand(dpy, dockapp, argv, argc);
    XStoreName(dpy, dockapp, "Test: Dockapp");
    
    XMapWindow(dpy, dockapp);

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
