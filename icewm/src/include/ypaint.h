#ifndef __YPAINT_H
#define __YPAINT_H

#include <X11/X.h>

#define ICON_SMALL 16   // small: 16x16 //!!! fix this
#define ICON_LARGE 32   // large: 32x32

class YWindow;

// !!! remove these if possible
#ifndef __YIMP_XLIB__
typedef struct _XDisplay Display;
typedef struct _XGC *GC;
typedef union _XEvent XEvent;
struct XFontStruct;
#ifdef CONFIG_I18N
typedef struct _XOC *XFontSet;
#endif
struct XPoint;
struct XExposeEvent;
struct XGraphicsExposeEvent;
struct XConfigureEvent;
struct XKeyEvent;
struct XButtonEvent;
struct XMotionEvent;
struct XCrossingEvent;
struct XPropertyEvent;
struct XColormapEvent;
struct XFocusChangeEvent;
struct XClientMessageEvent;
struct XMapEvent;
struct XUnmapEvent;
struct XDestroyWindowEvent;
struct XConfigureRequestEvent;
struct XMapRequestEvent;
struct XSelectionEvent;
struct XSelectionClearEvent;
struct XSelectionRequestEvent;
#endif
#ifndef __YIMP_XUTIL__
#ifdef SHAPE
struct XShapeEvent;
struct XTextProperty;
#endif
#endif

class YColorPrefProperty;
class YFontPrefProperty;
class CStr;
class YRect;

class YColor {
public:
    YColor(unsigned short red, unsigned short green, unsigned short blue);
    YColor(const char *clr);

    YColor *darker();
    YColor *brighter();

    static YColor *black;
    static YColor *white;

    unsigned long pixel() {
        if (fPixel == 0xFFFFFFFF) alloc(); return fPixel;
    }

private:
    unsigned long fPixel;
    unsigned short fRed;
    unsigned short fGreen;
    unsigned short fBlue;
    YColor *fDarker; //!!! remove this (needs color caching...)
    YColor *fBrighter; //!!! removethis

    friend class Graphics;

    void alloc();

private: // not-used
    YColor(const YColor &);
    YColor &operator=(const YColor &);
};

class YFont {
public:
    static YFont *getFont(const char *name);
    ~YFont();

    int height() const { return fontAscent + fontDescent; }
    int descent() const { return fontDescent; }
    int ascent() const { return fontAscent; }

    int textWidth(const CStr *str) const;
    int textWidth(const char *str) const;
    int textWidth(const char *str, int len) const;
private:
#ifdef CONFIG_I18N
    XFontSet font_set;
#endif
    XFontStruct *afont;
    int fontAscent, fontDescent;

    YFont(const char *name);

    void alloc();

    friend class Graphics;//!!!fix

private: // not-used
    YFont(const YFont &);
    YFont &operator=(const YFont &);
};

class YPixmap {
public:
    YPixmap(int w, int h, bool mask = false);
    YPixmap(Pixmap pixmap, Pixmap mask, int w, int h, bool owned);
    ~YPixmap();

    Pixmap pixmap() const { return fPixmap; }
    Pixmap mask() const { return fMask; }
    unsigned int width() const { return fWidth; }
    unsigned int height() const { return fHeight; }
private:
    Pixmap fPixmap;
    Pixmap fMask;
    unsigned int fWidth, fHeight;
    bool fOwned;
private: // not-used
    YPixmap(const YPixmap&);
    YPixmap &operator=(const YPixmap &);
};

class YIcon {
public:
    YIcon(const char *fileName);
    YIcon(YPixmap *small, YPixmap *large);
    ~YIcon();

    YPixmap *large();
    YPixmap *small();

    const char *iconName() const { return fPath; }
    YIcon *next() const { return fNext; }

    static YIcon *getIcon(const char *name);
private:
    char *fPath;
    YPixmap *fSmall;
    YPixmap *fLarge;
    YIcon *fNext;
    bool loadedS;
    bool loadedL;

    bool findIcon(char **fullPath, int size);
    YPixmap *loadIcon(int size);

    static class YResourcePath *fIconPaths; //!!! make app local?
    static void initIcons();
    static bool findIcon(char *base, char **fullPath);
    static void freeIcons();

    friend class YApplication;
private: // not-used
    YIcon(const YIcon &);
    YIcon &operator=(const YIcon&);
};

class Graphics {
public:
    Graphics(YWindow *window);
    Graphics(YPixmap *pixmap);
    virtual ~Graphics();

    void copyArea(int x, int y, int width, int height, int dx, int dy);
    void copyPixmap(YPixmap *pixmap, int x, int y, int width, int height, int dx, int dy);
    void drawPoint(int x, int y);
    void drawLine(int x1, int y1, int x2, int y2);
    void drawRect(int x, int y, int width, int height);
    void drawArc(int x, int y, int width, int height, int a1, int a2);
    void drawChars(const char *data, int offset, int len, int x, int y);
    void drawCharUnderline(int x, int y, const char *str, int charPos);
    void drawPixmap(YPixmap *pix, int x, int y);
    void drawClippedPixmap(Pixmap pix, Pixmap clip,
                           int x, int y, int w, int h, int toX, int toY);
    void fillRect(int x, int y, int width, int height);
    void fillPolygon(XPoint *points, int n, int shape, int mode);
    void fillArc(int x, int y, int width, int height, int a1, int a2);
    void setColor(YColor *aColor);
    void setColor(YColorPrefProperty *aColor);
    void setColor(YColorPrefProperty &aColor);
    void setFont(YFont *aFont);
    void setFont(YFontPrefProperty *aFont);
    void setFont(YFontPrefProperty &aFont);
    void setDottedPenStyle(bool dotLine = false); ///!!!hack

    void draw3DRect(int x, int y, int w, int h, bool raised);
    void drawBorderW(int x, int y, int w, int h, bool raised);
    void drawBorderM(int x, int y, int w, int h, bool raised);
    void drawBorderG(int x, int y, int w, int h, bool raised);
    void drawCenteredPixmap(int x, int y, int w, int h, YPixmap *pixmap);
    void drawOutline(int l, int t, int r, int b, int iw, int ih);
    void repHorz(YPixmap *pixmap, int x, int y, int w);
    void repVert(YPixmap *pixmap, int x, int y, int h);
    void fillPixmap(YPixmap *pixmap, int x, int y, int w, int h);

    void drawArrow(int direction, int style, int x, int y, int size);
    void drawCharsEllipsis(const char *data, int len, int x, int y, int maxWidth);

#define DrawText_Vertical (3)
#define DrawText_Horizontal (3 << 2)
#define DrawText_VTop (0)
#define DrawText_VCenter (1)
#define DrawText_VBottom (2)
#define DrawText_HLeft (0 << 2)
#define DrawText_HCenter (1 << 2)
#define DrawText_HRight (2 << 2)

    void drawText(const YRect &rect, const CStr *text, int flags, int underlinePos = -1);

    YColor *getColor() const { return color; }
    YFont *getFont() const { return font; }
    GC handle() const { return gc; }
private:
    Display *display;
    Drawable drawable;
    GC gc;

    YColor *color;
    YFont *font;
private: // not-used
    Graphics(const Graphics&);
    Graphics &operator=(const Graphics &);
};

extern Colormap defaultColormap; //!!!??? cleanup
extern Cursor leftPointer; // !!! make a class for this
extern Cursor rightPointer;


#endif
