# gnome-shell
gnome-shell 3.10.4

This is a the Ubuntu gnome-shell v 3.10.4. I use it for porting the npapi plugin used for gnome-shell-integration with the http://extensions.gnome.org web site to the Chrome Pepper API.
The new ppapi-plugin is available in browser-plugin dir.
A second step will be to port for running in nacl.
The first version (ppapi port) is usable by browsing the web site with a chrome launched with:

/opt/google/chrome/chrome --ppapi-out-of-process --register-pepper-plugins="/opt/google/chrome/libgnome-shell-chrome-plugin.so;application/x-gnome-shell-integration" --no-sandbox

This command implies you first install the plugin in /opt/google/chrome dir.

Once this will allow to list and install/uninstall extensions, I manage the nacl porting. It will then be possible to remove the --no-sandbox option.
Then when port finished, I'll package the plugin in a chrome app for deploying the nacl plugin.
