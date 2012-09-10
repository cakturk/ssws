#############################################################################
# Makefile for building: ssws (simple stupid web server)
#############################################################################
CC            = gcc
DEFINES       =
CFLAGS        = -pipe -g -Wall -Wp,-D_FORTIFY_SOURCE=2 -mtune=generic \
		-Wall -Wno-unused-function $(DEFINES)
INCPATH       =
LINK          = gcc
LFLAGS        =
LIBS          =
AR            = ar cqs
TAR           = tar -cf
COMPRESS      = gzip -9f
COPY          = cp -f
SED           = sed
COPY_FILE     = $(COPY)
COPY_DIR      = $(COPY) -r
STRIP         = strip --strip-unneeded
INSTALL_FILE  = install -m 644 -p
INSTALL_DIR   = $(COPY_DIR)
INSTALL_PROGRAM = install -m 755 -p
DEL_FILE      = rm -f
SYMLINK       = ln -f -s
DEL_DIR       = rmdir
MOVE          = mv -f
CHK_DIR_EXISTS= test -d
MKDIR         = mkdir -p
OBJECTS       = main.o http_header_parser.o ssws_core.o
EXE           = ssws

first: all

####### Implicit rules
.SUFFIXES: .o .c

.c.o:
	$(CC) -c $(CFLAGS) $(INCPATH) -o "$@" "$<"


all: Makefile $(EXE)

$(EXE):  $(OBJECTS)
	$(LINK) $(LFLAGS) -o $(EXE) $(OBJECTS) $(LIBS)

release: $(EXE)
	$(STRIP) $(EXE)

clean:
	-$(DEL_FILE) $(OBJECTS)
	-$(DEL_FILE) *~ core.*
