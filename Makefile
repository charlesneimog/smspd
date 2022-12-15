# library name
lib.name = sms

# python libs 

uname := $(shell uname -s)

ifeq (MINGW,$(findstring MINGW,$(uname)))
  cflags = -I ./libsms/src 
  ldlibs = -L ./libsms/build/ -l sms
  SMS_DYNLIB = ./libsms/build/libsms.dll
  

else ifeq (Linux,$(findstring Linux,$(uname)))
  cflags = -I ./libsms/src 
  ldlibs = -L ./libsms/build/ -l sms
  SMS_DYNLIB = ./libsms/build/libsms.so
  

else ifeq (Darwin,$(findstring Darwin,$(uname)))
  cflags = -I ./libsms/src 
  ldlibs = -L ./libsms/build/ -l sms
  SMS_DYNLIB = ./libsms/build/libsms.dylib

else
  $(error "Unknown system type: $(uname)")
  $(shell exit 1)

endif

# Define SMS lib

# =================================== Sources ===================================

sms.class.sources = src/smspd.c src/smssynth~.c  
# With errors src/smsanal.c src/smsedit.c


# =================================== Data ======================================

datafiles = $(SMS_DYNLIB)

# =================================== Pd Lib Builder =============================

PDLIBBUILDER_DIR=./pd-lib-builder/
include $(PDLIBBUILDER_DIR)/Makefile.pdlibbuilder
