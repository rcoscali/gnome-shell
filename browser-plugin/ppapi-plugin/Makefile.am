
chromelibdir = $(CHROME_PLUGIN_DIR)

chromelib_LTLIBRARIES = libgnome-shell-chrome-plugin.la

libgnome_shell_chrome_plugin_la_LDFLAGS = \
	$(CHROME_PLUGIN_LDFLAGS)
	-module -avoid-version -no-undefined

libgnome_shell_chrome_plugin_la_LIBADD = 	\
	$(CHROME_PLUGIN_LIBS) \
	$(MOZILLA_PLUGIN_LIBS)

libgnome_shell_chrome_plugin_la_SOURCES = 	\
	ppapi-plugin.c

libgnome_shell_chrome_plugin_la_CFLAGS = 	\
	$(MOZILLA_PLUGIN_CFLAGS)		\
	$(CHROME_PLUGIN_CFLAGS)		\
	-DG_LOG_DOMAIN=\"GnomeShellChromePlugin\"
