/* define if have old kstat (on solaris only?) */
#undef HAVE_OLD_KSTAT

/* the locale dependent property describing the codeset (langinfo.h) */
#undef HAVE_CODESET
#undef HAVE__NL_CTYPE_CODESET_NAME
#undef HAVE__NL_MESSAGES_CODESET

/* define how to query the current locale's codeset */
#undef CONFIG_NL_CODESETS

/* define when iconv is imported from GNU libiconv */
#undef CONFIG_LIBICONV

/* preferred Unicode set */
#undef CONFIG_UNICODE_SET

/* support for KDE tray windows */
#define CONFIG_KDE_TRAY_WINDOWS defined(CONFIG_TRAY) && \
                                defined(CONFIG_KDE_HINTS)

/* support for GNOME or net wm-spec tray windows */
#define CONFIG_GNOME_OR_WMSPEC_HINTS defined(CONFIG_GNOME_HINTS) && \
                                     defined(CONFIG_WMSPEC_HINTS)
