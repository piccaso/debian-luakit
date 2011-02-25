# Get the current version which is either a nearby git tag or a short-hash
# of the current commit.
VERSION   ?= $(shell ./build-utils/getversion.sh)

# Paths
PREFIX     ?= /usr/local
INSTALLDIR ?= $(DESTDIR)$(PREFIX)
MANPREFIX  ?= $(DESTDIR)$(PREFIX)/share/man
DOCDIR     ?= $(DESTDIR)$(PREFIX)/share/luakit/docs

# The lua pkg-config name changes from system to system, try autodetect it.
ifeq ($(LUA_PKG_NAME),)
LUA_PKG_NAME = $(shell pkg-config --exists lua && echo lua)
endif
ifeq ($(LUA_PKG_NAME),)
LUA_PKG_NAME = $(shell pkg-config --exists lua-5.1 && echo lua-5.1)
endif
ifeq ($(LUA_PKG_NAME),)
LUA_PKG_NAME = $(shell pkg-config --exists lua5.1 && echo lua5.1)
endif
ifeq ($(LUA_PKG_NAME),)
LUA_PKG_NAME = $(shell pkg-config --exists lua51 && echo lua51)
endif

ifeq ($(LUA_PKG_NAME),)
$(error Unable to determine lua pkg-config name, specify manually with \
`LUA_PKG_NAME=<name> make`, use `pkg-config --list-all | grep lua` to \
find the correct package name for your system. Please also check that you \
have lua >= 5.1 installed)
endif

# Check if user has sqlite3 libs installed.
ifeq ($(shell pkg-config --exists sqlite3 && echo 1),)
$(error Unable to find sqlite3 libs on your system, do you have sqlite3 \
installed?)
endif

# Check if user has webkit-gtk libs installed.
ifeq ($(shell pkg-config --exists webkit-1.0 && echo 1),)
$(error Unable to find webkit-gtk libs on your system, do you have \
webkit-gtk installed?)
endif

# Generate includes and libs
# liblua5.1-0-dev libgtk2.0-dev libwebkit-dev gperf libsqlite3-dev
PKGS := gtk+-2.0 gthread-2.0 webkit-1.0 $(LUA_PKG_NAME) sqlite3
INCS := $(shell pkg-config --cflags $(PKGS)) -I./
LIBS := $(shell pkg-config --libs $(PKGS))

# Should we load relative config paths first?
ifneq ($(DEVELOPMENT_PATHS),0)
CPPFLAGS += -DDEVELOPMENT_PATHS
endif

# Add flags
CPPFLAGS := -DVERSION=\"$(VERSION)\" $(CPPFLAGS)
CFLAGS   := -std=gnu99 -ggdb -W -Wall -Wextra $(INCS) $(CFLAGS)
LDFLAGS  := $(LIBS) $(LDFLAGS)

# Building on OSX
# TODO: These lines have never been tested
#CFLAGS  += -lgthread-2.0
#LDFLAGS += -pthread

# Building on FreeBSD (or just use gmake)
# TODO: These lines have never been tested
#VERSION != echo `./build-utils/getversion.sh`
#INCS    != echo -I. -I/usr/include `pkg-config --cflags $(PKGS)`
#LIBS    != echo -L/usr/lib `pkg-config --libs $(PKGS)`

# Custom compiler / linker
#CC = clang
