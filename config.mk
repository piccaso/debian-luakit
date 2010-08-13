# Get the current version which is either a nearby git tag or a short-hash
# of the current commit.
VERSION   ?= $(shell ./build-utils/getversion.sh)

# Paths
PREFIX     ?= /usr/local
INSTALLDIR ?= ${DESTDIR}${PREFIX}
MANPREFIX  ?= ${DESTDIR}${PREFIX}/share/man
DOCDIR     ?= ${DESTDIR}${PREFIX}/share/luakit/docs

# Generate includes and libs
# liblua5.1-0-dev libgtk2.0-dev libwebkit-dev libxdg-basedir-dev gperf
PKGS := gtk+-2.0 gthread-2.0 webkit-1.0 lua5.1
INCS := $(shell pkg-config --cflags ${PKGS}) -I./
LIBS := $(shell pkg-config --libs ${PKGS})

# Add flags
CPPFLAGS := -DVERSION=\"${VERSION}\" ${CPPFLAGS} -DDEVELOPMENT_PATHS
CFLAGS   := -std=c99 -ggdb -W -Wall -Wextra ${INCS} ${CFLAGS}
LDFLAGS  := ${LIBS} ${LDFLAGS}

# Building on OSX
#CFLAGS  += -lgthread-2.0
#LDFLAGS += -pthread

# Building on FreeBSD (or just use gmake)
#VERSION != echo `./build-utils/getversion.sh`
#INCS    != echo -I. -I/usr/include `pkg-config --cflags ${PKGS}`
#LIBS    != echo -L/usr/lib `pkg-config --libs ${PKGS}`

# Custom compiler / linker
#CC = clang
