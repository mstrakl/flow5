# Automated tests, built and run with ./docker/dev.sh test after the main build.
# Not part of flow5.pro: the GUI tests link the app's object files, so the app must be built first.

TEMPLATE = subdirs

SUBDIRS = lib gui
