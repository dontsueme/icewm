#ifndef __WMCONFIG_H
#define __WMCONFIG_H

extern bool configurationNeeded;

void loadConfiguration(const char *fileName);
void addWorkspace(const char *name, const char *value);
void setLook(const char *name, const char *value);
void freeConfiguration();

#endif
