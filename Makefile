# Top level Makefile for Egoboo 2.x

#!!!! do not specify the prefix in this file
#!!!! if you want to specify a prefix, do it on the command line
#For instance: "make PREFIX=$HOME/.local"

#---------------------
# some project definitions

PROJ_NAME	:= egoboo
PROJ_VERSION	:= 2.x

#---------------------
# the target names

EGO_DIR           := game
EGO_TARGET        := $(PROJ_NAME)-$(PROJ_VERSION)

EGOLIB_DIR        := egolib
EGOLIB_TARGET     := libegolib.a

IDLIB_DIR         := idlib
IDLIB_TARGET      := libidlib.a

CARTMAN_DIR       := cartman
CARTMAN_TARGET    := cartman

EGOTOOL_DIR       := utilities/convertpaletted
EGOTOOL_TARGET    := egotool

INSTALL_DIR       := data

#---------------------
# the SDL configuration

SDL_CONF  := sdl2-config
TMPFLAGS  := $(shell ${SDL_CONF} --cflags)
SDLCONF_L := $(shell ${SDL_CONF} --libs)

EXTERNAL_LUA := $(shell pwd)/external/lua-5.2.3

# set LUA_LDFLAGS to not use the lua in external/
#ifeq ($(LUA_LDFLAGS),)
#LUA_CFLAGS := -I${EXTERNAL_LUA}/src
#LUA_LDFLAGS := ${EXTERNAL_LUA}/src/liblua.a
#USE_EXTERNAL_LUA := 1
#endif

#---------------------
# the compiler options
TMPFLAGS += -std=c++14 $(LUA_CFLAGS)

# set paths if PREFIX is defined
ifneq ($(PREFIX),)
	ifeq ($(PREFIX),/usr)
		BINARY_PATH ?= /usr/bin
		CONFIG_PATH ?= /etc/egoboo
		DATA_PATH ?= /usr/share/egoboo
	else
		BINARY_PATH ?= $(PREFIX)/bin
		CONFIG_PATH ?= $(PREFIX)/etc/egoboo
		DATA_PATH ?= $(PREFIX)/share/egoboo
	endif
endif

ifneq ($(CONFIG_PATH),)
	ifneq ($(DATA_PATH),)
		TMPFLAGS += -DFS_HAS_PATHS -DFS_CONFIG_PATH=\"$(CONFIG_PATH)\" -DFS_DATA_PATH=\"$(DATA_PATH)\"
	endif
endif

ifeq ($(BINARY_PATH),)
	CANNOT_INSTALL := 1
else ifeq ($(CONFIG_PATH),)
	CANNOT_INSTALL := 1
else ifeq ($(DATA_PATH),)
	CANNOT_INSTALL := 1
else
	CANNOT_INSTALL := 0
endif

EGO_CXXFLAGS = $(TMPFLAGS)
EGO_LDFLAGS  = -pthread $(LUA_LDFLAGS) ${SDLCONF_L} -lSDL2_ttf -lSDL2_mixer -lSDL2_image -lphysfs -lGL

export CONFIG_PATH DATA_PATH EGO_CXXFLAGS EGO_LDFLAGS IDLIB_TARGET EGOLIB_TARGET EGO_TARGET CARTMAN_TARGET EGOTOOL_TARGET

#------------------------------------
# definitions of the target projects

.PHONY: all clean idlib egolib egoboo cartman install doxygen external_lua test egotool

all: idlib egolib egoboo cartman egotool

idlib: external_lua
	${MAKE} -C $(IDLIB_DIR)

egolib: idlib
	${MAKE} -C $(EGOLIB_DIR)

egoboo: egolib
	${MAKE} -C $(EGO_DIR)

cartman: egolib
	${MAKE} -C $(CARTMAN_DIR)

egotool: egolib
	${MAKE} -C $(EGOTOOL_DIR)

test: all
	${MAKE} -C ${IDLIB_DIR} test
	${MAKE} -C ${EGOLIB_DIR} test

external_lua:
ifeq ($(USE_EXTERNAL_LUA), 1)
	${MAKE} -C $(EXTERNAL_LUA)/src liblua.a SYSCFLAGS="-DLUA_USE_POSIX"
endif

clean:
	${MAKE} -C $(IDLIB_DIR) clean
	${MAKE} -C $(EGOLIB_DIR) clean
	${MAKE} -C $(EGO_DIR) clean
	${MAKE} -C $(CARTMAN_DIR) clean
	${MAKE} -C $(EGOTOOL_DIR) clean
ifeq ($(USE_EXTERNAL_LUA), 1)
	${MAKE} -C $(EXTERNAL_LUA) clean
endif

doxygen:
	doxygen

install: egoboo
ifeq ($(CANNOT_INSTALL),1)
	$(error cannot install; binary, data, and config paths are must be set\
	(either via PREFIX or BINARY_PATH, CONFIG_PATH, and DATA_PATH))
else
	mkdir -p $(BINARY_PATH)
	install -m 755 $(EGO_DIR)/$(EGO_TARGET) $(BINARY_PATH)
	
	${MAKE} -C $(INSTALL_DIR) install
endif

uninstall:
ifeq ($(CANNOT_INSTALL),1)
	$(error cannot uninstall; binary, data, and config paths are must be set\
	(either via PREFIX or BINARY_PATH, CONFIG_PATH, and DATA_PATH))
else
	rm $(BINARY_PATH)/$(EGO_TARGET)
	${MAKE} -C $(INSTALL_DIR) uninstall
endif
