.DEFAULT_GOAL := all

BASE_LIBNAME := rvdriver

variant :=

ifdef DEBUG
	echo "building debug"
	CFLAGS += -g -DDEBUG
	variant := $(variant).debug
else
	CFLAGS += -O3 -DNDEBUG
endif

UNAME := $(shell uname)

top_build_dir := build

ARNOLD_VER4 = $(ARNOLD_MAJOR_VERSION).$(ARNOLD_MINOR_VERSION)
build_dir     := ${top_build_dir}/${ARNOLD_VER4}/${OS_ARCH}${variant}

RELEASE = $(REPO_PATH)/arnold/shaders/published/$(ARNOLD_VER4)/`uname`-`uname -m`

INCLUDE = 	-I$(ARNOLD)/include 		\
			-I.

CFLAGS += -c -Wall -Wsign-compare -fvisibility = hidden -fPIC $(INCLUDE)

CC = /usr/bin/g++
CC = g++

LINK = /usr/bin/g++
LINK = g++

LIBS = -lai

LIBDIR = -L$(ARNOLD)/bin

LDFLAGS = -shared --whole-archive -Bsymbolic  $(LIBDIR)

ifeq ($(UNAME), Linux)
	CFLAGS += -D_LINUX
	LDFLAGS += -export-dynamic
	LIBEXT = so
endif

SOURCES := rvdriver.cpp

SOURCE_FILENAMES := $(notdir $(SOURCES))
OBJS := $(SOURCE_FILENAMES:%.cpp=${build_dir}/%.o)

LIBNAME = ${BASE_LIBNAME}.${LIBEXT}

FINAL_LIB := ${build_dir}/${LIBNAME}

BUILD_INFO_FILE := ${top_build_dir}/qtcreator

# RECIPES
.PHONY : all debug clean ${BUILD_INFO_FILE} 

debug:
	${MAKE} DEBUG=1

all: ${FINAL_LIB}
	# guarantees the last-built library is always in a certain place,
	# so that in qtcreator/eclipse we can always point to a single place 
	# in our run/debug configuration, regardless of whether we're in 
	# release or debug config
	cp -f ${FINAL_LIB} $(top_build_dir)/$(LIBNAME)

debug:
	${MAKE} DEBUG=1


${build_dir}/_buildDirMarker :
	mkdir -p ${build_dir}
	touch $@

# This is just here so that we can state myCppFile.h as a dependency for every
# myCppFile.cpp, even if myCppFile.h doesn't exist
%.h :
	echo "Dummy rule for header files - $@"

${build_dir}/%.o : %.cpp %.h ${build_dir}/_buildDirMarker
	$(CC) -o $@ $(CFLAGS) $<

${FINAL_LIB}: $(OBJS)
	$(LINK) -o $@ $(OBJS) $(LDFLAGS) $(LIBS)
	(if [ -z "${DEBUG}" ] ; \
	then \
		echo "Copying to $(RELEASE)" ; \
		mkdir -p $(RELEASE) ; \
		cp -f $@ $(RELEASE) ; \
	fi)

clean:
	rm -rf $(top_build_dir)