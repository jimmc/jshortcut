TOPDIR = ../..
MAKEINFODIR = $(TOPDIR)/makeinfo

include $(MAKEINFODIR)/javadefs.mak

ifeq ($(OSTYPE),windows)
    DEFAULT_TARGET = dll
else
    DEFAULT_TARGET = notwin
endif

BASENAME      = jshortcut

OBJS          = jshortcut.obj

INCLUDES      = /I $(WINDOWS_JDK)\include /I $(WINDOWS_JDK)\include\win32

LIBS          = advapi32.lib shell32.lib \
		ole32.lib uuid.lib libcmt.lib kernel32.lib

default:	$(DEFAULT_TARGET)

notwin:
	@echo "You must build the DLL on Windows"

dll:		..\..\$(BASENAME).dll

..\..\$(BASENAME).dll:	$(OBJS)
	link /nologo /incremental:no /fixed:no /nod /dll \
		/release /machine:ix86 \
		/out:..\..\$(BASENAME).dll /def:$(BASENAME).def \
		$(OBJS) $(LIBS)

clean:;		rm -f *.obj *.dll *.exp *.lib
