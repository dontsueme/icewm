/*
 * IceWM
 *
 * Copyright (C) 1999-2001 Marko Macek
 *
 * Session management support
 */

#include "config.h"

#ifdef CONFIG_SESSION

#include "yfull.h"
#include "wmframe.h"
#include "wmsession.h"
#include "base.h"
#include "wmapp.h"

#include <stdio.h>
#include <string.h>

#include "intl.h"

SMWindowKey::SMWindowKey(YFrameWindow */*f*/) {
}

SMWindowKey::SMWindowKey(char *id, char *role) {
    clientId = newstr(id);
    windowRole = newstr(role);
    windowClass = 0;
    windowInstance = 0;
}

SMWindowKey::SMWindowKey(char *id, char *klass, char *instance) {
    clientId = newstr(id);
    windowRole = 0;
    windowClass = newstr(klass);
    windowInstance = newstr(instance);
}

SMWindowKey::~SMWindowKey() {
}

SMWindowInfo::SMWindowInfo(YFrameWindow *f): key(f) {
}

SMWindowInfo::SMWindowInfo(char *id, char *role,
                           int ax, int ay, int w, int h,
                           unsigned long astate, int alayer, int aworkspace): key(id, role)
{
    x = ax;
    y = ay;
    width = w;
    height = h;
    state = astate;
    layer = alayer;
    workspace = aworkspace;
}

SMWindowInfo::SMWindowInfo(char *id, char *klass, char *instance,
                           int ax, int ay, int w, int h,
                           unsigned long astate, int alayer, int aworkspace): key(id, klass, instance)
{
    x = ax;
    y = ay;
    width = w;
    height = h;
    state = astate;
    layer = alayer;
    workspace = aworkspace;
}

SMWindowInfo::~SMWindowInfo() {
}

SMWindows::SMWindows() {
    windowCount = 0;
    windows = 0;
}

SMWindows::~SMWindows() {
    clearAllInfo();
}

void SMWindows::clearAllInfo() {
    for (unsigned i(0); i < windowCount; ++i) delete windows[i];

    FREE(windows);
    windows = 0;
    windowCount = 0;
}

void SMWindows::addWindowInfo(SMWindowInfo *info) {
    windows = (SMWindowInfo **)REALLOC(windows, (windowCount + 1) * sizeof(SMWindowInfo *));
    windows[windowCount++] = info;
}

void SMWindows::setWindowInfo(YFrameWindow */*f*/) {
}
                                     
bool SMWindows::getWindowInfo(YFrameWindow */*f*/, SMWindowInfo */*info*/) {
    return false;
}

bool SMWindows::findWindowInfo(YFrameWindow *frame) {
    frame->client()->updateClientLeader();

    Window leader(frame->client()->clientLeader());
    if (leader == None) return false;

    char *cid(frame->client()->clientId(leader));
    if (NULL == cid) return false;

    for (unsigned i(0); i < windowCount; ++i) {
        if (!strcmp(cid, windows[i]->key.clientId)) {
            if (windows[i]->key.windowClass &&
                windows[i]->key.windowInstance) {
                XClassHint *classHint(frame->client()->classHint());
                char *wmName(NULL), *wmClass(NULL);

                if (classHint) {
                    wmClass = classHint->res_class;
                    wmName = classHint->res_name;
                }

                if (!strcmp(wmClass, windows[i]->key.windowClass) &&
                    !strcmp(wmName, windows[i]->key.windowInstance)) {
                    MSG(("got c %s %s %s %d:%d:%d:%d %d %ld %d",
                         cid, wmClass, wmName,
                         windows[i]->x, windows[i]->y,
                         windows[i]->width, windows[i]->height,
                         windows[i]->workspace, windows[i]->state,
                         windows[i]->layer));

                    frame->configureClient(windows[i]->x,
                                           windows[i]->y,
                                           windows[i]->width,
                                           windows[i]->height);

                    frame->layer(windows[i]->layer);
                    frame->workspace(windows[i]->workspace);
                    frame->state(WIN_STATE_ALL, windows[i]->state);

                    XFree(cid);
                    return true;
                }
            }
        }
    }

    XFree(cid);
    return false;
}

bool SMWindows::removeWindowInfo(YFrameWindow */*f*/) {
    return false;
}

SMWindows *sminfo = 0;

static int wr_str(FILE *f, const char *s) {
    if (!s)
        return -1;
    if (fputc('"', f) == EOF)
        return -1;
    while (*s) {
        if (*s >= '0' && *s <= '9' ||
            *s >= 'a' && *s <= 'z' ||
            *s >= 'A' && *s <= 'Z' ||
            *s == '_' || *s == '-' ||
            *s == '.' || *s == ':')
        {
            if (fputc(*s, f) == EOF)
                return -1;
        } else {
            unsigned char c = *s;

            if (fprintf(f, "=%02X", c) != 3)
                return -1;
        }
        s++;
    }
    if (fputc('"', f) == EOF)
        return -1;
    if (fputc(' ', f) == EOF)
        return -1;
    return 0;
}

static int rd_str(char *s, char *d) {
    char c;
    bool old = true;

    c = *s++;
    while (c == ' ')
        c = *s++;
    if (c == '"') {
        old = false;
        c = *s++;
    }

    while (c != EOF) {
        if (c == '"' && !old) {
            c = *s++;
            break;
        }
        if (c == ' ' && old)
            break;
        if (!old && c == '=') {
            int i = ' ';

            sscanf(s, "%02X", &i);
            s += 2;
            c = i & 0xFF;
        }
        *d++ = c;
        c = *s++;
    }
    *d = 0;
    return 0;
}

void loadWindowInfo() {
    sminfo = new SMWindows();

    FILE *fp;
    char line[1024];
    char cid[1024];
    char role[1024];
    char klass[1024];
    char instance[1024];
    int x, y, w, h;
    long workspace, layer;
    unsigned long state;
    SMWindowInfo *info;

    char *name = getsesfile();

    if (name == 0)
        return ;

    fp = fopen(name, "r");
    if (fp == NULL)
        return ;

    while (fgets(line, sizeof(line), fp) != 0) {
        if (line[0] == 'c') {
            if (sscanf(line, "c %s %s %s %d:%d:%d:%d %ld %lu %ld",
                       cid, klass, instance, &x, &y, &w, &h,
                       &workspace, &state, &layer) == 10)
            {
                MSG(("%s %s %s %d:%d:%d:%d %ld %lu %ld",
                    cid, klass, instance, x, y, w, h,
                     workspace, state, layer));
                rd_str(cid, cid);
                rd_str(klass, klass);
                rd_str(instance, instance);
                info = new SMWindowInfo(cid, klass, instance,
                                        x, y, w, h, state, layer, workspace);
                sminfo->addWindowInfo(info);
            } else {
                msg(_("Session Manager: Unknown line %s"), line);
            }
        } else if (line[0] == 'r') {
            if (sscanf(line, "r %s %s %d:%d:%d:%d %ld %lu %ld",
                       cid, role, &x, &y, &w, &h,
                       &workspace, &state, &layer) == 9)
            {
                MSG(("%s %s %s %d:%d:%d:%d %ld %lu %ld\n",
                     cid, klass, instance, x, y, w, h,
                     workspace, state, layer));
                rd_str(cid, cid);
                rd_str(role, role);
                info = new SMWindowInfo(cid, role,
                                        x, y, w, h, state, layer, workspace);
                sminfo->addWindowInfo(info);
            } else {
                msg(_("Session Manager: Unknown line %s"), line);
            }
        } else if (line[0] == 'w') {
            int ws = 0;

            if (sscanf(line, "w %d", &ws) == 1) {        
                if (ws >= 0 && ws < manager->workspaceCount())
                    manager->activateWorkspace(ws);
            }
        } else {
            msg(_("Session Manager: Unknown line %s"), line);
        }
    }
    fclose(fp);
}

bool findWindowInfo(YFrameWindow *f) {
    if (sminfo && sminfo->findWindowInfo(f)) {
        return true;
    }
    return false;
}

void YWMApp::smSaveYourself(bool shutdown, bool fast) {
    YApplication::smSaveYourself(shutdown, fast);
}

void YWMApp::smShutdownCancelled() {
    //!!!manager->exitAfterLastClient(false);
    YApplication::smShutdownCancelled();
}

void YWMApp::smDie() {
    exit(0);
    //!!!manager->exitAfterLastClient(true);
}

void YWMApp::smSaveYourselfPhase2() {
    char *name;
    FILE *fp;;
    
    if (NULL != (name = getsesfile()) &&
        NULL != (fp = fopen(name, "w+"))) {
        for (YFrameWindow *frame(manager->topLayer());
             NULL != frame; frame = frame->nextLayer()) {
            frame->client()->updateClientLeader();
            Window leader(frame->client()->clientLeader());

            //msg("window=%s", frame->client()->windowTitle());
            if (leader != None) {
                //msg("leader=%lX", leader);
                char *cid(frame->client()->clientId(leader));

                if (cid) {
                    frame->client()->updateWindowRole();
                    const char *role(frame->client()->windowRole());

                    if (role) {
                        fprintf(fp, "r ");
                        //%s %s ", cid, role);
                        wr_str(fp, cid);
                        wr_str(fp, role);
                    } else {
                        frame->client()->updateClassHint();
                        XClassHint *classHint(frame->client()->classHint());
                        char *wmName(NULL), *wmClass(NULL);

                        if (classHint) {
                            wmClass = classHint->res_class;
                            wmName = classHint->res_name;
                        }

                        if (wmClass && wmName) {
                            //msg("k=%s, i=%s", klass, instance);
                            fprintf(fp, "c ");
                            //%s %s %s ", cid, klass, instance);
                            wr_str(fp, cid);
                            wr_str(fp, wmClass);
                            wr_str(fp, wmName);
                        } else {
                            XFree(cid);
                            continue;
                        }
                    }

                    XFree(cid);
                    fprintf(fp, "%d:%d:%d:%d %ld %lu %ld\n",
                            frame->x(), frame->y(),
                            frame->client()->width(), frame->client()->height(),
                            frame->workspace(), frame->state(), frame->layer());
                }
            }
        }

        fprintf(fp, "w %lu\n", manager->activeWorkspace());
        fclose(fp);
    }

    YApplication::smSaveYourselfPhase2();
}

#endif /* CONFIG_SESSION */
