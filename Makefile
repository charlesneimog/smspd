lib.name = sms

uname := $(shell uname -s)

ifeq (MINGW,$(findstring MINGW,$(uname)))
	cflags = -I ./libsms/src -fPIC
	ldlibs = -L ./libsms/build/ -l sms -Wl,--allow-multiple-definition
	SMS_DYNLIB = ./libsms/build/libsms.dll

else ifeq (Linux,$(findstring Linux,$(uname)))
  	cflags = -I ./libsms/src -fPIC
  	ldlibs = -L ./libsms/build/ -l sms -Wl,--allow-multiple-definition
  	SMS_DYNLIB = ./libsms/build/libsms.so 
  
else ifeq (Darwin,$(findstring Darwin,$(uname)))
  	cflags = -I ./libsms/src -fPIC
  	ldlibs = -L ./libsms/build/ -l sms -Wl,--allow-multiple-definition
  	SMS_DYNLIB = ./libsms/build/libsms.dylib

else
  $(error "Unknown system type: $(uname)")
  $(shell exit 1)

endif

# Add the new target to the default target
.PHONY: all libsms
all: libsms

# Add a new target to build libsms
libsms:

	cd ./libsms && mkdir -p build && cd build && cmake .. && cmake --build . --config Release 
	cp $(SMS_DYNLIB) .





# =================================== Sources ===================================

sms.class.sources =  src/smspd.c src/smsanal.c src/smssynth~.c src/smsedit.c 
# With errors src/smsanal.c src/smsedit.c
 
# =================================== Data ======================================

datafiles = $(SMS_DYNLIB) ./help/sms-help.pd ./help/basic.pd ./help/smsanal-help.pd ./help/smsedit-help.pd ./help/smssynth~-help.pd COPYING README.md

# =================================== Pd Lib Builder =============================

PDLIBBUILDER_DIR=./pd-lib-builder/
include $(PDLIBBUILDER_DIR)/Makefile.pdlibbuilder
