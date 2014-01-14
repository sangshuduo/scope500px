# This file is used by Qt Creator as a project file to load all files that are part of
# the project as specified in SOURCES, HEADERS and OTHER_FILES, and to build the final
# executable
#
# You can also use it from the command line:
# If you run `qmake` (without the quotes) from the top of the source tree, it will
# generate a Makefile from this .pro file. From there you can use the usual make targets
# e.g. `make`, `sudo make install`, `sudo make uninstall`, `make clean`, `make distclean`, etc.
# To regenerate the Makefile, simply invoke `qmake` again

TEMPLATE = app
CONFIG += link_pkgconfig
CONFIG -= app_bundle
CONFIG -= qt


###########################################################
# To install dependencies:
#   sudo apt-get install libunity-dev
PKGCONFIG += \
    glib-2.0	\
    unity

# Pkg-config takes care of the includes for the build, so the following variable is not
# strictly needed. We need this simply for Qt Creator to find the
# headers in the editor, as it seems not to use the PGKCONFIG variable.
INCLUDEPATH += \
    /usr/include/glib-2.0 \
    /usr/lib/x86_64-linux-gnu/glib-2.0/include	\
    /usr/include/unity/unity

TARGET = scope500px
DAEMON_PATH = $$[QT_INSTALL_LIBS]/unity-scope-$${TARGET}

DATA_DIR = $$_PRO_FILE_PWD_/data
SCOPE_FILE = $$DATA_DIR/$${TARGET}.scope
SERVICE_FILE = $$DATA_DIR/unity-scope-$${TARGET}.service

SOURCES += \
    scope500px.c \
    scope500px-photo-parser-func.c

HEADERS += \
    config.h \  
    scope500px-photo-parser-func.h

OTHER_FILES += \
    $$SCOPE_FILE \
    $${SERVICE_FILE}.in

# Rule to generate the DBUS .service file, including the architecture-dependent installation path
# generated at build time
servicefilegen.target = servicefilegen
servicefilegen.commands = sed -e \"s,\\(Exec=\\).*\$$,\1$$DAEMON_PATH/$$TARGET,\" $${SERVICE_FILE}.in > $$SERVICE_FILE
QMAKE_EXTRA_TARGETS += servicefilegen
QMAKE_CLEAN += $$SERVICE_FILE
QMAKE_CLEAN += $$TARGET
QMAKE_CLEAN += $$DATA_DIR/*~
PRE_TARGETDEPS += \
    servicefilegen

# Rule to install the scope's binary
target.path = $$DAEMON_PATH

# Rule to install the DBUS .service file
servicefile.path = /usr/share/dbus-1/services
servicefile.files = $$SERVICE_FILE

# Rule to install the .scope file
scopefile.path = /usr/share/unity/scopes/graphics
scopefile.files = $$SCOPE_FILE

INSTALLS += \
    target \
    scopefile \
    servicefile


unix:!macx: LIBS += -ltidy -lcurl

