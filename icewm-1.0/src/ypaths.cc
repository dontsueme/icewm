/*
 *  IceWM - Resource paths
 *  Copyright (C) 2001 The Authors of IceWM
 *
 *  Release under terms of the GNU Library General Public License
 *
 *  2001/03/24: Mathias Hasselmann <mathias.hasselmann@gmx.net>
 *  - initial version
 */

#include "config.h"

#include "default.h"
#include "ylib.h"
#include "ypixbuf.h"

#include "base.h"
#include "ypaths.h"

#include "intl.h"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <fcntl.h>
#include <sys/stat.h>

char * YPathElement::joinPath(char const *base, char const *name) const {
    char const *b(base ? base : "");

    if (sub)
        return strJoin(*root, rdir, *sub, "/", b, name, NULL);
    else
        return strJoin(*root, rdir, b, name, NULL);
}

YResourcePaths const & 
YResourcePaths::operator= (YResourcePaths const & other) {
    delete[] fPaths;
    
    if (other.fPaths) {
	unsigned peCount(0);
	for (YPathElement const * pe(other.fPaths); pe->root; ++pe, ++peCount);
	
	fPaths = new YPathElement[peCount];
	memcpy(fPaths, other.fPaths, peCount * sizeof(YPathElement));
    } else
        fPaths = NULL;

    return *this;
}

void YResourcePaths::init(char const * subdir, bool themeOnly) {
    delete[] fPaths;

    static char const * home(::getenv("HOME"));

    static char themeSubdir[PATH_MAX];
    static char const * themeDir(themeSubdir);

    strncpy(themeSubdir, themeName, sizeof(themeSubdir));
    themeSubdir[sizeof(themeSubdir) - 1] = '\0';
	    
    char *dirname(::strrchr(themeSubdir, '/'));
    if (dirname) *dirname = '\0';

    if (themeName && *themeName == '/') {
	MSG(("Searching `%s' resources at absolute location", subdir));
    
	if (themeOnly) {
	    static YPathElement const paths[] = {
	        { &themeDir, "/", NULL },
	        { NULL, NULL, NULL }
	    };

	    fPaths = new YPathElement[ACOUNT(paths)];
	    memcpy(fPaths, paths, sizeof(paths));
	} else {
	    static YPathElement const paths[] = {
	        { &themeDir, "/", NULL },
	        { &home, "/.icewm/", NULL },
	        { &configDir, "/", NULL },
	        { &libDir, "/", NULL },
	        { NULL, NULL, NULL }
	    };
					// To provide consistence behaviour
	    int const themePriority	// with relative paths
		(strncmp(themeDir, configDir, strlen(configDir)) ?
		(strncmp(themeDir, libDir, strlen(libDir)) ? 0 : 2) : 1);

	    MSG(("themePriority: %d", themePriority));

	    fPaths = new YPathElement[ACOUNT(paths)];

	    memcpy(fPaths, paths + 1,
		   themePriority * sizeof(*paths));
	    memcpy(fPaths + themePriority, paths,
		   sizeof(*paths));
	    memcpy(fPaths + themePriority + 1,
	    	   paths + themePriority + 1,
		  (ACOUNT(paths) - themePriority - 1) * sizeof(*paths));
	}
    } else {
	MSG(("Searching `%s' resources at relative locations", subdir));

	if (themeOnly) {
	    static YPathElement const paths[] = {
		{ &home, "/.icewm/themes/", &themeDir },
		{ &configDir, "/themes/", &themeDir },
		{ &libDir, "/themes/", &themeDir },
		{ NULL, NULL, NULL }
	    };

	    fPaths = new YPathElement[ACOUNT(paths)];
	    memcpy(fPaths, paths, sizeof(paths));
	} else {
	    static YPathElement const paths[] = {
		{ &home, "/.icewm/themes/", &themeDir },
		{ &home, "/.icewm/", NULL },
		{ &configDir, "/themes/", &themeDir },
		{ &configDir, "/", NULL },
		{ &libDir, "/themes/", &themeDir },
		{ &libDir, "/", NULL },
		{ NULL, NULL, NULL }
	    };
	    
	    fPaths = new YPathElement[ACOUNT(paths)];
	    memcpy(fPaths, paths, sizeof(paths));
	}
    }
    
    DBG {
	MSG(("Initial search path:"));
	for (YPathElement const *pe(*this); pe->root; pe++) {
	    char *path(pe->joinPath("/icons/"));
	    MSG(("%s", path));
	    delete[] path;
        }
    }

    verifyPaths(subdir);
}
    
void YResourcePaths::verifyPaths(char const *base) {
    unsigned j(0), i(0);

    for (; fPaths[i].root; i++) {
        char *path(fPaths[i].joinPath(base));

        if (access(path, R_OK) == 0)
            fPaths[j++] = fPaths[i];

        delete path;
    }
    fPaths[j] = fPaths[i];
}

YPixmap * YResourcePaths::loadPixmap(const char *base, const char *name) const {
    YPixmap * pixmap(NULL);

    for (YPathElement * pe(fPaths); pe->root && pixmap == NULL; pe++) {
        char * path(pe->joinPath(base, name));

        if (isreg(path) && (pixmap = new YPixmap(path)) == NULL)
	    die(1, _("Out of memory for pixel map %s"), path);

        delete[] path;
    }
#ifdef DEBUG
    if (debug)
        warn(_("Could not find pixel map %s"), name);
#endif

    return pixmap;
}

#ifdef CONFIG_ANTIALIASING
YPixbuf * YResourcePaths::loadPixbuf(char const * base, char const * name,
				     bool const fullAlpha) const {
    YPixbuf * pixbuf(NULL);

    for (YPathElement * pe(fPaths); pe->root && pixbuf == NULL; pe++) {
        char * path(pe->joinPath(base, name));

        if (isreg(path) && (pixbuf = new YPixbuf(path, fullAlpha)) == NULL)
	    die(1, _("Out of memory for RGB pixel buffer %s"), path);

        delete[] path;
    }
#ifdef DEBUG
    if (debug)
        warn(_("Could not find RGB pixel buffer %s"), name);
#endif

    return pixbuf;
}
#endif

/**
 * Returns the name of the user's icewm directory.
 *
 * The directory's name is built by expanding "$HOME/.icewm/".
 * Side effect: The private directory is created if it doesn't exist.
 */
char const *YResourcePaths::privateDirectory(void) {
    static char *privateDirectory(NULL);
    
    if (NULL == privateDirectory)
        privateDirectory = strJoin(getenv("HOME"), "/.icewm/", NULL);

    mkdir(privateDirectory, 0755);
    return privateDirectory;
}

/**
 * Returns a filename in the user's private icewm directory.
 *
 * The filename is built by expanding "$HOME/.icewm/$basename$suffix".
 * Side effect: The private directory is created if it doesn't exist.
 */
char *YResourcePaths::privateFilename(char const *basename,
                                      char const *suffix) {
    return strJoin(privateDirectory(), basename, suffix, NULL);
}

#ifdef CONFIG_GNOME_MENUS

/**
 * Returns the name of KDE's toplevel directory.
 *
 * When $KDEDIR is set it's value is returned, the value passed
 * to the configure script's --with-kdedir switch otherwise.
 */
char const *YResourcePaths::kdeDirectory(void) {
    char const *kdedir(getenv("KDEDIR"));
    return (kdedir ? kdedir : KDEDIR);
}

/**
 * Returns a filename in the KDE directory.
 *
 * In general the filename is built by expanding "$KDEDIR/$basename$suffix".
 */
char *YResourcePaths::kdeFilename(char const *basename,
                                  char const *suffix) {
    return strJoin(kdeDirectory(), basename, suffix, NULL);
}

/**
 * Returns the path to the KDE menu.
 *
 * The value of the KDEMenuDir options and if not set
 * "$KDEDIR/share/applnk" is returned.
 */
char *YResourcePaths::kdeMenuPath(void) {
    return '\0' == *kdeMenuDir ? kdeFilename("/share/applnk")
                               : strdup(kdeMenuDir);
}

#endif
