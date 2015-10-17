CXXFLAGS += -O3 # optimize heavily
#CXXFLAGS += -g  # include debug information
CXXFLAGS += -std=c++11 -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64 -DOSMIUM_WITH_SPARSEHASH=1

OS:=$(shell uname -s)
ifeq ($(OS),Darwin)
	CXXFLAGS += -stdlib=libc++
	LDFLAGS += -stdlib=libc++
endif

CXXFLAGS_GEOS     := $(shell geos-config --cflags)
CXXFLAGS_OGR      := $(shell gdal-config --cflags)
CXXFLAGS_WARNINGS := -Wall -Wextra -pedantic -Wredundant-decls -Wdisabled-optimization -Wctor-dtor-privacy -Wnon-virtual-dtor -Woverloaded-virtual -Wsign-promo -Wold-style-cast -Wuninitialized

ifeq ($(CXX),clang++) # compiler is clang++
	this_compiler_file=last_use_of_clang.tmp
	other_compiler_file=last_use_of_gcc.tmp

	# add more warnings
	CXXFLAGS_WARNINGS += -Weverything # g++ doesn't know -Weverything to enable _all_ warnings, so we add it for clang++ exclusively

	# disable warnings
	CXXFLAGS_WARNINGS += -Wno-c++98-compat -Wno-c++98-compat-pedantic

else # other compiler (g++)
	this_compiler_file=last_use_of_gcc.tmp
	other_compiler_file=last_use_of_clang.tmp

	# add more warnings
	CXXFLAGS_WARNINGS += -Wcast-align -Wcast-qual -Wformat=2 -Winit-self -Wlogical-op -Wmissing-declarations -Wmissing-include-dirs -Wnoexcept -Wshadow -Wsign-conversion -Wstrict-null-sentinel -Wstrict-overflow=5 -Wswitch-default -Wundef -Wabi -Wconversion -Winline -Winvalid-pch -Wmissing-format-attribute -Wpadded -Wstack-protector -Wswitch-enum -Wunsafe-loop-optimizations -Wzero-as-null-pointer-constant -Wuseless-cast # taken from https://stackoverflow.com/questions/5088460/flags-to-enable-thorough-and-verbose-g-warnings

	# warnings that one might want to ignore
	#CXXFLAGS_WARNINGS += -Weffc++

	# disable warnings
	CXXFLAGS_WARNINGS += -Wno-long-long
endif

LIB_EXPAT := -lexpat
LIB_PBF   := -pthread -lz -lprotobuf-lite -losmpbf
# statically link protobuf-lite when libprotobuf-lite.so.8 (or similar) isn't available on the target system; explanation: http://stackoverflow.com/a/938947
#LIB_PBF   := -pthread -lz -Wl,-Bstatic -lprotobuf-lite -Wl,-Bdynamic -losmpbf
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

PROGRAMS := osmi_addresses

.PHONY: all clean

all: $(PROGRAMS)

osmi_addresses: main.cpp Makefile *.hpp $(other_compiler_file)
	$(CXX) $(CXXFLAGS) $(CXXFLAGS_WARNINGS) $(CXXFLAGS_OGR) -o $@ $< $(LDFLAGS) $(LIB_IO) $(LIB_GEOS) $(LIB_OGR) $(LIB_PRGOPT) $(LIB_BOOST_FS) $(LIB_BOOST_SYS) ; touch $(this_compiler_file)

last_use_of_clang.tmp:
	touch last_use_of_clang.tmp

last_use_of_gcc.tmp:
	touch last_use_of_gcc.tmp
	
clean:
	rm -f *.o core $(PROGRAMS)
