#include "config.h"

#include "yfull.h"
#include "yapp.h"
#include "yarray.h"

#if 1
#include <stdio.h>
#include "intl.h"
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#endif

#include "yconfig.h"
#include "yprefs.h"

#define CFGDEF

#include "icewmbg_prefs.h"

char const *ApplicationName = NULL;

class DesktopBackgroundManager: public YApplication {
public:
    DesktopBackgroundManager(int *argc, char ***argv);

    virtual void handleSignal(int sig);

    void addImage(const char *imageFileName);

    void update();
    void sendQuit();
    void sendRestart();
private:
    long getWorkspace();
    void changeBackground(long workspace);
    YPixmap *loadImage(const char *imageFileName);

    bool filterEvent(const XEvent &xev);

private:
    YPixmap *defaultBackground;
    YPixmap *currentBackground;
    YObjectArray<YPixmap> backgroundPixmaps;
    long activeWorkspace;

    Atom _XA_XROOTPMAP_ID;
    Atom _XA_XROOTCOLOR_PIXEL;
    Atom _XA_NET_CURRENT_DESKTOP;

    Atom _XA_ICEWMBG_QUIT;
    Atom _XA_ICEWMBG_RESTART;
};

DesktopBackgroundManager::DesktopBackgroundManager(int *argc, char ***argv):
    YApplication(argc, argv),
    defaultBackground(0),
    currentBackground(0),
    activeWorkspace(-1),
    _XA_XROOTPMAP_ID(None),
    _XA_XROOTCOLOR_PIXEL(None)
{
    desktop->setStyle(YWindow::wsDesktopAware);
    catchSignal(SIGTERM);
    catchSignal(SIGINT);
    catchSignal(SIGQUIT);

    _XA_NET_CURRENT_DESKTOP =
        XInternAtom(app->display(), "_NET_CURRENT_DESKTOP", False);
    _XA_ICEWMBG_QUIT =
        XInternAtom(app->display(), "_ICEWMBG_QUIT", False);
    _XA_ICEWMBG_RESTART =
        XInternAtom(app->display(), "_ICEWMBG_RESTART", False);

#warning "I don't see a reason for this to be conditional...? maybe only as an #ifdef"
#warning "XXX I see it now, the process needs to hold on to the pixmap to make this work :("
    if (supportSemitransparency) {
	_XA_XROOTPMAP_ID = XInternAtom(app->display(), "_XROOTPMAP_ID", False);
	_XA_XROOTCOLOR_PIXEL = XInternAtom(app->display(), "_XROOTCOLOR_PIXEL", False);
    }
}

void DesktopBackgroundManager::handleSignal(int sig) {
    switch (sig) {
    case SIGINT:
    case SIGTERM:
    case SIGQUIT:
        if (supportSemitransparency) {
            if (_XA_XROOTPMAP_ID)
                XDeleteProperty(app->display(), desktop->handle(), _XA_XROOTPMAP_ID);
            if (_XA_XROOTCOLOR_PIXEL)
                XDeleteProperty(app->display(), desktop->handle(), _XA_XROOTCOLOR_PIXEL);
        }

        ///XCloseDisplay(display);
        exit(1);

        break;

    default:
        YApplication::handleSignal(sig);
        break;
    }
}

void DesktopBackgroundManager::addImage(const char *imageFileName) {
    YPixmap *image = loadImage(imageFileName);

    backgroundPixmaps.append(image);
    if (defaultBackground == 0)
        defaultBackground = image;
}

YPixmap *DesktopBackgroundManager::loadImage(const char *imageFileName) {
    if (access(imageFileName, 0) == 0)
        return new YPixmap(imageFileName);
    else
        return 0;
}

void DesktopBackgroundManager::update() {
    long w = getWorkspace();
    if (w != activeWorkspace) {
        activeWorkspace = w;
        changeBackground(activeWorkspace);
    }
}

long DesktopBackgroundManager::getWorkspace() {
    long w;
    Atom r_type;
    int r_format;
    unsigned long nitems, lbytes;
    unsigned char *prop;

    if (XGetWindowProperty(app->display(), desktop->handle(),
                           _XA_NET_CURRENT_DESKTOP,
                           0, 1, False, XA_CARDINAL,
                           &r_type, &r_format,
                           &nitems, &lbytes, &prop) == Success && prop)
    {
        if (r_type == XA_CARDINAL && r_format == 32 && nitems == 1) {
            w = ((long *)prop)[0];
            XFree(prop);
            return w;
        }
        XFree(prop);
    }
    return -1;
}

#if 1
 // should be a separate program to reduce memory waste
static YPixmap * renderBackground(YResourcePaths const & paths,
				  char const * filename, YColor * color) {
    YPixmap *back = NULL;

    if (*filename == '/') {
	if (access(filename, R_OK) == 0)
	    back = new YPixmap(filename);
    } else
	back = paths.loadPixmap(0, filename);

    if (back && centerBackground) {
	YPixmap * cBack = new YPixmap(desktop->width(), desktop->height());
	Graphics g(*cBack, 0, 0);

        g.setColor(color);
        g.fillRect(0, 0, desktop->width(), desktop->height());
        g.drawPixmap(back, (desktop->width() -  back->width()) / 2,
			   (desktop->height() - back->height()) / 2);

        delete back;
        back = cBack;
    }
#warning "TODO: implement scaled background"
    return back;
}
#endif

void DesktopBackgroundManager::changeBackground(long workspace) {
#warning "fixme: add back handling of multiple desktop backgrounds"
#if 0
    YPixmap *pixmap = defaultBackground;

    if (workspace >= 0 && workspace < (long)backgroundPixmaps.getCount() &&
        backgroundPixmaps[workspace])
    {
        pixmap = backgroundPixmaps[workspace];
    }

    if (pixmap != currentBackground) {
        XSetWindowBackgroundPixmap(app->display(), desktop->handle(), pixmap->pixmap());
        XClearWindow(app->display(), desktop->handle());

        if (supportSemitransparency) {
	    if (_XA_XROOTPMAP_ID)
		XChangeProperty(app->display(), desktop->handle(), _XA_XROOTPMAP_ID,
				XA_PIXMAP, 32, PropModeReplace,
				(const unsigned char*) &pixmap, 1);
	    if (_XA_XROOTCOLOR_PIXEL) {
		unsigned long black(BlackPixel(app->display(),
				    DefaultScreen(app->display())));

		XChangeProperty(app->display(), desktop->handle(), _XA_XROOTCOLOR_PIXEL,
				XA_CARDINAL, 32, PropModeReplace,
				(const unsigned char*) &black, 1);
            }
	}
    }
    XFlush(app->display());

#endif
#if 1
    YResourcePaths paths("", true);
    YColor * bColor((DesktopBackgroundColor && DesktopBackgroundColor[0])
                    ? new YColor(DesktopBackgroundColor)
                    : 0);

    if (bColor == 0)
        bColor = YColor::black;

    unsigned long const bPixel(bColor->pixel());
    bool handleBackground(false);
    Pixmap bPixmap(None);

    if (DesktopBackgroundPixmap && DesktopBackgroundPixmap[0]) {
        YPixmap * back(renderBackground(paths, DesktopBackgroundPixmap,
					bColor));

        if (back) {
	    bPixmap = back->pixmap();
            XSetWindowBackgroundPixmap(app->display(), desktop->handle(),
	    			       bPixmap);
	    handleBackground = true;
        }
    } else if (DesktopBackgroundColor && DesktopBackgroundColor[0]) {
        XSetWindowBackgroundPixmap(app->display(), desktop->handle(), 0);
        XSetWindowBackground(app->display(), desktop->handle(), bPixel);
	handleBackground = true;
    }

    if (handleBackground) {
        if (supportSemitransparency &&
            _XA_XROOTPMAP_ID && _XA_XROOTCOLOR_PIXEL) {
            if (DesktopBackgroundPixmap &&
                DesktopTransparencyPixmap &&
                !strcmp (DesktopBackgroundPixmap,
                         DesktopTransparencyPixmap)) {
                delete[] DesktopTransparencyPixmap;
                DesktopTransparencyPixmap = NULL;
            }

	    YColor * tColor(DesktopTransparencyColor &&
	    		    DesktopTransparencyColor[0]
			  ? new YColor(DesktopTransparencyColor)
			  : bColor);

	    YPixmap * root(DesktopTransparencyPixmap &&
	    		   DesktopTransparencyPixmap[0]
			 ? renderBackground(paths, DesktopTransparencyPixmap,
			 		    tColor) : NULL);

	    unsigned long const tPixel(tColor->pixel());
	    Pixmap const tPixmap(root ? root->pixmap() : bPixmap);

	    XChangeProperty(app->display(), desktop->handle(),
			    _XA_XROOTPMAP_ID, XA_PIXMAP, 32,
			    PropModeReplace, (unsigned char const*)&tPixmap, 1);
	    XChangeProperty(app->display(), desktop->handle(),
			    _XA_XROOTCOLOR_PIXEL, XA_CARDINAL, 32,
			    PropModeReplace, (unsigned char const*)&tPixel, 1);
	}

    }
#endif
    XClearWindow(app->display(), desktop->handle());
    XFlush(app->display());
    //    if (backgroundPixmaps.getCount() <= 1)
    if (!supportSemitransparency) {
        exit(0);
    }
}

bool DesktopBackgroundManager::filterEvent(const XEvent &xev) {
    if (xev.type == PropertyNotify) {
        if (xev.xproperty.window == desktop->handle() &&
            xev.xproperty.atom == _XA_NET_CURRENT_DESKTOP)
        {
            update();
        }
    } else if (xev.type == ClientMessage) {
        if (xev.xclient.window == desktop->handle() &&
            xev.xproperty.atom == _XA_ICEWMBG_QUIT)
        {
            exit(0);
        }
        if (xev.xclient.window == desktop->handle() &&
            xev.xproperty.atom == _XA_ICEWMBG_RESTART)
        {
            execlp(ICEWMBGEXE, ICEWMBGEXE, 0);
        }
    }

    return YApplication::filterEvent(xev);
}

void DesktopBackgroundManager::sendQuit() {
    XClientMessageEvent xev;

    memset(&xev, 0, sizeof(xev));
    xev.type = ClientMessage;
    xev.window = desktop->handle();
    xev.message_type = _XA_ICEWMBG_QUIT;
    xev.format = 32;
    xev.data.l[0] = getpid();
    XSendEvent(app->display(), desktop->handle(), False, StructureNotifyMask, (XEvent *) &xev);
    XSync(app->display(), False);
}

void DesktopBackgroundManager::sendRestart() {
    XClientMessageEvent xev;

    memset(&xev, 0, sizeof(xev));
    xev.type = ClientMessage;
    xev.window = desktop->handle();
    xev.message_type = _XA_ICEWMBG_RESTART;
    xev.format = 32;
    xev.data.l[0] = getpid();
    XSendEvent(app->display(), desktop->handle(), False, StructureNotifyMask, (XEvent *) &xev);
    XSync(app->display(), False);
}

void printUsage(int rc = 1) {
#if 0
    fputs (_("Usage: icewmbg [OPTION]... pixmap1 [pixmap2]...\n"
	     "Changes desktop background on workspace switches.\n"
	     "The first pixmap is used as a default one.\n\n"
	     "-s, --semitransparency    Enable support for "
				       "semi-transparent terminals\n"),
	     stderr);
#endif
    fputs (_("Usage: icewmbg [ -r | -q ]\n"
             " -r  Restart icewmbg\n"
             " -q  Quit icewmbg\n"
	     "Loads desktop background according to preferences file\n"
	     " DesktopBackgroundCenter  - Display desktop background centered, not tiled\n"
	     " SupportSemitransparency  - Support for semitransparent terminals\n"
	     " DesktopBackgroundColor   - Desktop background color\n"
	     " DesktopBackgroundImage   - Desktop background image\n"
	     " DesktopTransparencyColor - Color to announce for semi-transparent windows\n"
	     " DesktopTransparencyImage - Image to announce for semi-transparent windows\n"),
	   stderr);
    exit(rc);
}

void invalidArgument(const char *appName, const char *arg) {
    fprintf(stderr, _("%s: unrecognized option `%s'\n"
		      "Try `%s --help' for more information.\n"),
		      appName, arg, appName);
    exit(1);
}

DesktopBackgroundManager *bg;

void addBgImage(const char */*name*/, const char *value) {
    bg->addImage(value);
}

int main(int argc, char **argv) {
    ApplicationName = basename(*argv);

#if 0
    {
        int n;
        int gotOpts = 0;

        for (n = 1; n < argc; ++n) if (argv[n][0] == '-')
            if (argv[n][1] == 's' ||
                strcmp(argv[n] + 1, "-semitransparency") == 0 &&
                !supportSemitransparency)
            {
                supportSemitransparency = true;
                gotOpts++;
            } else if (argv[n][1] == 'h' ||
                     strcmp(argv[n] + 1, "-help") == 0)
                printUsage(0);
            else
                invalidArgument("icewmbg", argv[n]);

        if (argc < 1 + gotOpts + 1)
            printUsage();
    }
#endif

    bg = new DesktopBackgroundManager(&argc, &argv);

    if (argc > 1) {
        if ( strcmp(argv[1], "-r") == 0) {
            bg->sendRestart();
            return 0;
        } else if (strcmp(argv[1], "-q") == 0) {
            bg->sendQuit();
            return 0;
        } else
            printUsage();
    }

#ifndef NO_CONFIGURE
    {
        cfoption theme_prefs[] = {
            OSV("Theme", &themeName, "Theme name"),
            OK0()
        };

        app->loadConfig(theme_prefs, "preferences");
        app->loadConfig(theme_prefs, "theme");
    }
    YApplication::loadConfig(icewmbg_prefs, "preferences");
    if (themeName != 0) {
        MSG(("themeName=%s", themeName));

        char *theme = 0;
#warning "!!! hack to fix current theme selector"
        if (themeName[0] == '/')
            theme = newstr(themeName);
        else
            theme = strJoin("themes/", themeName, NULL);
#warning "FIXME: do not allow all settings to be set by themes"
        YApplication::loadConfig(icewmbg_prefs, theme);
        delete [] theme;
    }
    YApplication::loadConfig(icewmbg_prefs, "prefoverride");
#endif

#if 0
    {
        char *configFile = 0;

        if (configFile == 0)
            configFile = app->findConfigFile("preferences");
        if (configFile)
            loadConfig(icewmbg_prefs, configFile);
        delete configFile; configFile = 0;

        if (themeName) {
	    if (*themeName == '/')
                loadConfig(icewmbg_prefs, themeName);
	    else {
		char *theme(strJoin("themes/", themeName, NULL));
		char *themePath(app->findConfigFile(theme));

                if (themePath)
                    loadConfig(icewmbg_prefs, themePath);

		delete[] themePath;
		delete[] theme;
            }
        }
    }
#endif

    ///XSelectInput(app->display(), desktop->handle(), PropertyChangeMask);
    bg->update();

    return bg->mainLoop();
}


///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////

#if 0
Pixmap loadPixmap(const char *filename) {
    Pixmap pixmap = 0;
#ifdef CONFIG_IMLIB
    if (!hImlib) hImlib = Imlib_init(display);

    ImlibImage *im = Imlib_load_image(hImlib, (char *)filename);
    if (im) {
        Imlib_render(hImlib, im, im->rgb_width, im->rgb_height);
        pixmap = (Pixmap)Imlib_move_image(hImlib, im);
        Imlib_destroy_image(hImlib, im);
    } else {
        fprintf(stderr, _("Loading image %s failed"), filename);
        fputs("\n", stderr);
    }
#else
    XpmAttributes xpmAttributes;
    xpmAttributes.colormap  = defaultColormap;
    xpmAttributes.closeness = 65535;
    xpmAttributes.valuemask = XpmSize|XpmReturnPixels|XpmColormap|XpmCloseness;

    Pixmap mask;
    int const rc(XpmReadFileToPixmap(display, root, (char *)filename,
				     &pixmap, &mask, &xpmAttributes));

    if (rc != XpmSuccess)
        warn(_("Loading of pixmap \"%s\" failed: %s"),
	       filename, XpmGetErrorString(rc));
    else
	if (mask != None) XFreePixmap(display, mask);
#endif
    return pixmap;
}
#endif

#if 0
#ifdef CONFIG_IMLIB
#include <Imlib.h>

static ImlibData *hImlib = 0;
#else
#include <X11/xpm.h>
#endif
#endif

#if 0
#include <assert.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/time.h>
#include <fcntl.h>
#include <stdarg.h>
#include <X11/Xproto.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xresource.h>
#include <X11/cursorfont.h>
#include <X11/keysym.h>
///#include <signal.h>

#include "base.h"
#include "WinMgr.h"
#endif

///#warning duplicates lots of prefs
///#include "default.h"
///#include "wmconfig.h"


