CXXFLAGS += -O3 # optimize heavily
#CXXFLAGS += -g  # include debug information
CXXFLAGS += -std=c++11 -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64 -DOSMIUM_WITH_SPARSEHASH=1

OS:=$(shell uname -s)
ifeq ($(OS),Darwin)
	CXXFLAGS += -stdlib=libc++
	LDFLAGS += -stdlib=libc++
endif

ifeq ($(CXX),clang++)
	this_compiler_file=last_use_of_clang.tmp
	other_compiler_file=last_use_of_gcc.tmp
else
	this_compiler_file=last_use_of_gcc.tmp
	other_compiler_file=last_use_of_clang.tmp
endif

CXXFLAGS_GEOS     := $(shell geos-config --cflags)
CXXFLAGS_OGR      := $(shell gdal-config --cflags)
CXXFLAGS_WARNINGS := -Wall -Wextra -pedantic -Wredundant-decls -Wdisabled-optimization -Wctor-dtor-privacy -Wnon-virtual-dtor -Woverloaded-virtual -Wsign-promo -Wold-style-cast

LIB_EXPAT := -lexpat
LIB_PBF   := -pthread -lz -lprotobuf-lite -losmpbf
LIB_GZIP  := -lz
LIB_BZIP2 := -lbz2

LIB_IO     := $(LIB_EXPAT) $(LIB_PBF) $(LIB_GZIP) $(LIB_BZIP2)

LIB_OGR   := $(shell gdal-config --libs)
LIB_GEOS  := $(shell geos-config --libs)
LIB_SHAPE  := -lshp $(LIB_GEOS)
LIB_PRGOPT := -lboost_program_options

LIB_BOOST_FS  := -lboost_filesystem
LIB_BOOST_SYS := -lboost_system

# this links libboost_program_options statically, because for some reason
# it always wants the exact version which might not be available on a
# different host
#LIB_PRGOPT := -Wl,-Bstatic -lboost_program_options -Wl,-Bdynamic

PROGRAMS := osmi

.PHONY: all clean

all: $(PROGRAMS)

osmi: main.cpp Makefile *.hpp $(other_compiler_file)
	$(CXX) $(CXXFLAGS) $(CXXFLAGS_WARNINGS) $(CXXFLAGS_OGR) -o $@ $< $(LDFLAGS) $(LIB_IO) $(LIB_GEOS) $(LIB_OGR) $(LIB_PRGOPT) $(LIB_BOOST_FS) $(LIB_BOOST_SYS) ; touch $(this_compiler_file)

last_use_of_clang.tmp:
	touch last_use_of_clang.tmp
last_use_of_gcc.tmp:
	touch last_use_of_gcc.tmp
	
clean:
	rm -f *.o core $(PROGRAMS)
