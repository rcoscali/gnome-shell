
mozillalibdir = $(MOZILLA_PLUGIN_DIR)

mozillalib_LTLIBRARIES = libgnome-shell-browser-plugin.la

libgnome_shell_browser_plugin_la_LDFLAGS = -module -avoid-version -no-undefined

libgnome_shell_browser_plugin_la_LIBADD = 	\
	$(MOZILLA_PLUGIN_LIBS)

libgnome_shell_browser_plugin_la_SOURCES = 	\
	npapi-plugin.c \
	npapi/npapi.h \
	npapi/npfunctions.h \
	npapi/npruntime.h \
	npapi/nptypes.h

libgnome_shell_browser_plugin_la_CFLAGS = 	\
	$(MOZILLA_PLUGIN_CFLAGS)		\
	-DG_LOG_DOMAIN=\"GnomeShellMozillaPlugin\"
