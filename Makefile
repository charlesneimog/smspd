# library name
lib.name = library

# input source file (class name == source file basename)
class.sources = smsanal.c

# cflags include libsms and sndfile
cflags = -I C:/Users/Neimog/Git/libsms/src -I C:/msys64/mingw64/include/ -lsndfile

# all extra files to be included in binary distribution of the library
datafiles =

# include Makefile.pdlibbuilder from submodule directory 'pd-lib-builder'
PDLIBBUILDER_DIR=./pd-lib-builder/
include $(PDLIBBUILDER_DIR)/Makefile.pdlibbuilder