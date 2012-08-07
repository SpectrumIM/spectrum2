# Hey Emacs, this is a -*- makefile -*-
# The twitcurl library.. a Makefile for OpenWRT
# Makefile-fu by John Boiles
# 28 September 2009

LIBNAME = twitcurl
SRC = $(LIBNAME).cpp
STAGING_DIR = 
INCLUDE_DIR = $(STAGING_DIR)/usr/include
LINCLUDE_DIR = $(STAGING_DIR)/usr/local/include
LIBRARY_DIR = $(STAGING_DIR)/usr/lib
LLIBRARY_DIR = $(STAGING_DIR)/usr/local/lib
LDFLAGS += -Wl,-rpath-link=$(STAGING_DIR)/usr/lib
CC = g++
REMOVE = rm -f
COPY = cp
REMOTEIP = 192.168.1.30

# Compiler flag to set the C Standard level.
# c89   - "ANSI" C
# gnu89 - c89 plus GCC extensions
# c99   - ISO C99 standard (not yet fully implemented)
# gnu99 - c99 plus GCC extensions
# CSTANDARD = -std=gnu99

# Place -D or -U options here
CDEFS =

# Place -I options here
CINCS = 

CFLAGS =$(CDEFS) $(CINCS) $(CSTANDARD)

all: target

target: $(SRC) $(LIBNAME).h
	$(CC) -Wall -fPIC -c -I$(INCLUDE_DIR) $(SRC) oauthlib.cpp urlencode.cpp base64.cpp HMAC_SHA1.cpp SHA1.cpp
	$(CC) -shared -Wl,-soname,lib$(LIBNAME).so.1 $(LDFLAGS) -o lib$(LIBNAME).so.1.0 *.o -L$(LIBRARY_DIR) -lcurl

#clean project.
clean:
	$(REMOVE) $(LIBNAME)*.so.1.0
	$(REMOVE) $(LIBNAME).o
	$(REMOVE) $(LIBRARY_DIR)/lib$(LIBNAME).so*
	
# Install library to local openwrt library directory
install: all
	$(COPY) lib$(LIBNAME).so.1.0 $(LIBRARY_DIR)
	$(COPY) lib$(LIBNAME).so.1.0 $(LLIBRARY_DIR)
	$(COPY) $(LIBNAME).h $(INCLUDE_DIR)/
	$(COPY) $(LIBNAME).h $(LINCLUDE_DIR)/
	$(COPY) oauthlib.h $(INCLUDE_DIR)/
	$(COPY) oauthlib.h $(LINCLUDE_DIR)/
	ln -sf $(LIBRARY_DIR)/lib$(LIBNAME).so.1.0 $(LIBRARY_DIR)/lib$(LIBNAME).so
	ln -sf $(LIBRARY_DIR)/lib$(LIBNAME).so.1.0 $(LIBRARY_DIR)/lib$(LIBNAME).so.1
	ln -sf $(LLIBRARY_DIR)/lib$(LIBNAME).so.1.0 $(LLIBRARY_DIR)/lib$(LIBNAME).so
	ln -sf $(LLIBRARY_DIR)/lib$(LIBNAME).so.1.0 $(LLIBRARY_DIR)/lib$(LIBNAME).so.1
