###########################################################################
#
#   winui.mak
#
#   winui (MameUI) makefile
#
###########################################################################
#
#   Copyright Aaron Giles
#   All rights reserved.
#
#   Redistribution and use in source and binary forms, with or without
#   modification, are permitted provided that the following conditions are
#   met:
#
#       * Redistributions of source code must retain the above copyright
#         notice, this list of conditions and the following disclaimer.
#       * Redistributions in binary form must reproduce the above copyright
#         notice, this list of conditions and the following disclaimer in
#         the documentation and/or other materials provided with the
#         distribution.
#       * Neither the name 'MAME' nor the names of its contributors may be
#         used to endorse or promote products derived from this software
#         without specific prior written permission.
#
#   THIS SOFTWARE IS PROVIDED BY AARON GILES ''AS IS'' AND ANY EXPRESS OR
#   IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
#   WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
#   DISCLAIMED. IN NO EVENT SHALL AARON GILES BE LIABLE FOR ANY DIRECT,
#   INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
#   (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
#   SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
#   HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
#   STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
#   IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
#   POSSIBILITY OF SUCH DAMAGE.
#
###########################################################################


###########################################################################
#################   BEGIN USER-CONFIGURABLE OPTIONS   #####################
###########################################################################


#-------------------------------------------------
# specify build options; see each option below
# for details
#-------------------------------------------------

# set this to the minimum DirectInput version to support (7 or 8)
# DIRECTINPUT = 8

# uncomment next line to compile OpenGL video renderer
USE_OPENGL = 1

###########################################################################
##################   END USER-CONFIGURABLE OPTIONS   ######################
###########################################################################

# add a define identifying the target osd
DEFS += -DOSD_WINDOWS

#-------------------------------------------------
# append "ui" to the emulator name
#-------------------------------------------------

EMULATOR = $(PREFIX)$(NAME)uifx$(SUFFIX)$(SUFFIX64)$(SUFFIXDEBUG)$(EXE)

#-------------------------------------------------
# object and source roots
#-------------------------------------------------

WINSRC = $(SRC)/osd/windows
WINOBJ = $(OBJ)/osd/windows

OSDSRC = $(SRC)/osd
OSDOBJ = $(OBJ)/osd

OBJDIRS += $(WINOBJ) \
	$(OSDOBJ)/modules/sync \
	$(OSDOBJ)/modules/lib \
	$(OSDOBJ)/modules/midi \
	$(OSDOBJ)/modules/font \
	$(OSDOBJ)/modules/netdev \
	$(OSDOBJ)/modules/debugger/win

# add ui specific src/objs
UISRC = $(SRC)/osd/$(OSD)
UIOBJ = $(OBJ)/osd/$(OSD)

OBJDIRS += $(UIOBJ)

#-------------------------------------------------
# configure the resource compiler
#-------------------------------------------------

RC = @windres --use-temp-file

RCDEFS = -DNDEBUG -D_WIN32_IE=0x0501

# include UISRC directory
RCFLAGS = -O coff -I $(UISRC) -I $(UIOBJ)

#-------------------------------------------------
# due to quirks of using /bin/sh, we need to
# explicitly specify the current path
#-------------------------------------------------

CURPATH = ./



#-------------------------------------------------
# Windows-specific debug objects and flags
#-------------------------------------------------

# define the x64 ABI to be Windows
DEFS += -DX64_WINDOWS_ABI

# disable SDL and QT libraries for MAMEUI
DEFS += -DUSE_QTDEBUG=0
DEFS += -DUSE_SDL=0

# enable UNICODE flags
DEFS += -DUNICODE -D_UNICODE

# add unicode and windows flags to emulator linking only
LDFLAGSEMULATOR += -municode -mwindows

# debug build: enable guard pages on all memory allocations
ifdef DEBUG
DEFS += -DMALLOC_DEBUG
endif



#-------------------------------------------------
# Windows-specific flags and libraries
#-------------------------------------------------

# add our prefix files to the mix, include WINSRC in UI build
CCOMFLAGS += -include $(WINSRC)/winprefix.h

INCPATH += -I$(WINSRC)

# ensure we statically link the gcc runtime lib
LDFLAGS += -static-libgcc

# TODO: needs to use $(CC)
TEST_GCC := $(shell gcc --version)
ifeq ($(findstring 4.4.,$(TEST_GCC)),)
	#if we use new tools
	LDFLAGS += -static-libstdc++
endif

# add the windows libraries, 4 additional libs at the end for UI
BASELIBS += -luser32 -lgdi32 -ldsound -ldxguid -lwinmm -ladvapi32 -lcomctl32 -lshlwapi -lwsock32
LIBS += -luser32 -lgdi32 -lddraw -ldsound -ldxguid -lwinmm -ladvapi32 -lcomctl32 -lshlwapi -lwsock32 -lkernel32 -lshell32 -lcomdlg32 -luxtheme

ifeq ($(DIRECTINPUT),8)
LIBS += -ldinput8
CCOMFLAGS += -DDIRECTINPUT_VERSION=0x0800
else
LIBS += -ldinput
CCOMFLAGS += -DDIRECTINPUT_VERSION=0x0700
endif


#-------------------------------------------------
# OSD core library
#-------------------------------------------------
# still not sure what to do about main.

OSDCOREOBJS = \
	$(WINOBJ)/strconv.o \
	$(WINOBJ)/windir.o \
	$(WINOBJ)/winfile.o \
	$(OSDOBJ)/modules/sync/sync_windows.o \
	$(WINOBJ)/winutf8.o \
	$(WINOBJ)/winutil.o \
	$(WINOBJ)/winclip.o \
	$(WINOBJ)/winsocket.o \
	$(OSDOBJ)/modules/sync/work_osd.o \
	$(OSDOBJ)/modules/lib/osdlib_win32.o \
	$(OSDOBJ)/modules/osdmodule.o \
	$(WINOBJ)/winptty.o \


#-------------------------------------------------
# OSD Windows library
#-------------------------------------------------

OSDOBJS = \
	$(WINOBJ)/d3d9intf.o \
	$(WINOBJ)/drawd3d.o \
	$(WINOBJ)/d3dhlsl.o \
	$(WINOBJ)/drawdd.o \
	$(WINOBJ)/drawgdi.o \
	$(WINOBJ)/drawbgfx.o \
	$(WINOBJ)/drawnone.o \
	$(WINOBJ)/input.o \
	$(WINOBJ)/output.o \
	$(OSDOBJ)/modules/sound/js_sound.o  \
	$(OSDOBJ)/modules/sound/direct_sound.o  \
	$(OSDOBJ)/modules/sound/sdl_sound.o  \
	$(OSDOBJ)/modules/sound/none.o  \
	$(WINOBJ)/video.o \
	$(WINOBJ)/window.o \
	$(WINOBJ)/winmenu.o \
	$(OSDOBJ)/modules/midi/portmidi.o \
	$(OSDOBJ)/modules/midi/none.o \
	$(OSDOBJ)/modules/lib/osdobj_common.o  \
	$(OSDOBJ)/modules/font/font_sdl.o \
	$(OSDOBJ)/modules/font/font_windows.o \
	$(OSDOBJ)/modules/font/font_osx.o \
	$(OSDOBJ)/modules/font/font_none.o \
	$(OSDOBJ)/modules/netdev/pcap.o \
	$(OSDOBJ)/modules/netdev/taptun.o \
	$(OSDOBJ)/modules/netdev/none.o \

ifdef USE_OPENGL
OSDOBJS += 	$(WINOBJ)/../sdl/drawogl.o $(WINOBJ)/../sdl/gl_shader_tool.o $(WINOBJ)/../sdl/gl_shader_mgr.o
OBJDIRS += $(WINOBJ)/../sdl

DEFS += -DUSE_OPENGL=1
LIBS += -lopengl32

else
DEFS += -DUSE_OPENGL=0
endif

ifndef DONT_USE_NETWORK
DEFS +=	-DSDLMAME_NET_PCAP
endif

CCOMFLAGS += -DDIRECT3D_VERSION=0x0900

# add UI objs
OSDOBJS += \
	$(WINOBJ)/winmainui.o \
	$(UIOBJ)/win_options.o \
	$(UIOBJ)/mui_util.o \
	$(UIOBJ)/directinput.o \
	$(UIOBJ)/dijoystick.o \
	$(UIOBJ)/directdraw.o \
	$(UIOBJ)/directories.o \
	$(UIOBJ)/mui_audit.o \
	$(UIOBJ)/columnedit.o \
	$(UIOBJ)/screenshot.o \
	$(UIOBJ)/treeview.o \
	$(UIOBJ)/splitters.o \
	$(UIOBJ)/bitmask.o \
	$(UIOBJ)/datamap.o \
	$(UIOBJ)/dxdecode.o \
	$(UIOBJ)/picker.o \
	$(UIOBJ)/properties.o \
	$(UIOBJ)/tabview.o \
	$(UIOBJ)/help.o \
	$(UIOBJ)/history.o \
	$(UIOBJ)/dialogs.o \
	$(UIOBJ)/mui_opts.o \
	$(UIOBJ)/layout.o \
	$(UIOBJ)/datafile.o \
	$(UIOBJ)/dirwatch.o \
	$(UIOBJ)/winui.o \
	$(UIOBJ)/helpids.o \

# extra dependencies
$(WINOBJ)/drawdd.o :    $(SRC)/emu/rendersw.inc
$(WINOBJ)/drawgdi.o :   $(SRC)/emu/rendersw.inc

# add debug-specific files
OSDOBJS += \
	$(OSDOBJ)/modules/debugger/debugwin.o \
	$(OSDOBJ)/modules/debugger/win/consolewininfo.o \
	$(OSDOBJ)/modules/debugger/win/debugbaseinfo.o \
	$(OSDOBJ)/modules/debugger/win/debugviewinfo.o \
	$(OSDOBJ)/modules/debugger/win/debugwininfo.o \
	$(OSDOBJ)/modules/debugger/win/disasmbasewininfo.o \
	$(OSDOBJ)/modules/debugger/win/disasmviewinfo.o \
	$(OSDOBJ)/modules/debugger/win/disasmwininfo.o \
	$(OSDOBJ)/modules/debugger/win/editwininfo.o \
	$(OSDOBJ)/modules/debugger/win/logwininfo.o \
	$(OSDOBJ)/modules/debugger/win/memoryviewinfo.o \
	$(OSDOBJ)/modules/debugger/win/memorywininfo.o \
	$(OSDOBJ)/modules/debugger/win/pointswininfo.o \
	$(OSDOBJ)/modules/debugger/win/uimetrics.o \
	$(OSDOBJ)/modules/debugger/debugint.o \
	$(OSDOBJ)/modules/debugger/debugqt.o \
	$(OSDOBJ)/modules/debugger/none.o

# add our UI resources
RESFILE += $(UIOBJ)/mameui.res

BGFX_LIB = $(OBJ)/libbgfx.a
INCPATH += -I$(3RDPARTY)/bgfx/include -I$(3RDPARTY)/bx/include

$(WINOBJ)/winmainui.o : $(WINSRC)/winmain.c
	@echo Compiling $<...
	$(CC) $(CDEFS) -Dmain=utf8_main $(CFLAGS) -c $< -o $@


#-------------------------------------------------
# WinPCap
#-------------------------------------------------
INCPATH += -I$(3RDPARTY)/winpcap/Include

#-------------------------------------------------
# rules for building the libaries
#-------------------------------------------------

$(LIBOCORE): $(OSDCOREOBJS)

$(LIBOSD): $(OSDOBJS)

# The : is important! It prevents the dependency above from including mui_main.o in its target!
LIBOSD := $(UIOBJ)/mui_main.o $(LIBOSD)

#-------------------------------------------------
# rules for creating helpids.c
#-------------------------------------------------

$(UISRC)/helpids.c : $(UIOBJ)/mkhelp$(EXE) $(UISRC)/resource.h $(UISRC)/resource.hm $(UISRC)/mameui.rc
	@"$(UIOBJ)/mkhelp$(EXE)" $(UISRC)/mameui.rc >$@

# rule to build the generator
$(UIOBJ)/mkhelp$(EXE): $(UIOBJ)/mkhelp.o $(LIBOCORE)
	@echo Linking $@...
	$(LD) $(LDFLAGS) $(OSDBGLDFLAGS) $^ $(LIBS) -o $@

#-------------------------------------------------
# generic rule for the resource compiler for UI
#-------------------------------------------------

$(RESFILE): $(UISRC)/mameui.rc $(UIOBJ)/mamevers.rc
	@echo Compiling mameui resources $<...
	$(RC) $(RCDEFS) $(RCFLAGS) -o $@ -i $<



#-------------------------------------------------
# rules for resource file
#-------------------------------------------------

$(UIOBJ)/mamevers.rc: $(SRC)/build/verinfo.py $(SRC)/version.c
	@echo Emitting $@...
	$(PYTHON) $(SRC)/build/verinfo.py -b mame -o $@ $(SRC)/version.c


#####  End winui.mak ##############################################
