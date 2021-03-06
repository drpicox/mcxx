##########################################################################
#  (C) Copyright 2006-2013 Barcelona Supercomputing Center               #
#                          Centro Nacional de Supercomputacion           #
#                                                                        #
#  This file is part of Mercurium C/C++ source-to-source compiler.       #
#                                                                        #
#  See AUTHORS file in the top level directory for information           #
#  regarding developers and contributors.                                #
#                                                                        #
#  This library is free software; you can redistribute it and/or         #
#  modify it under the terms of the GNU Lesser General Public            #
#  License as published by the Free Software Foundation; either          #
#  version 3 of the License, or (at your option) any later version.      #
#                                                                        #
#  Mercurium C/C++ source-to-source compiler is distributed in the hope  #
#  that it will be useful, but WITHOUT ANY WARRANTY; without even the    #
#  implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR       #
#  PURPOSE.  See the GNU Lesser General Public License for more          #
#  details.                                                              #
#                                                                        #
#  You should have received a copy of the GNU Lesser General Public      #
#  License along with Mercurium C/C++ source-to-source compiler; if      #
#  not, write to the Free Software Foundation, Inc., 675 Mass Ave,       #
#  Cambridge, MA 02139, USA.                                             #
##########################################################################

noinst_PROGRAMS = tpp

CC_FOR_BUILD=@CC_FOR_BUILD@

if SUPPORTED_SILENT_RULES
CCBUILD_verbose = $(CCBUILD_verbose_$(V))
CCBUILD_verbose_ = $(CCBUILD_verbose_$(AM_DEFAULT_VERBOSITY))
CCBUILD_verbose_0 = @echo "  CCBUILD " $@;
endif

# tpp
tpp_SOURCES = tpp.c

tpp$(EXEEXT): tpp.o
	$(CCBUILD_verbose)$(CC_FOR_BUILD) -o $@ $+ $(LDFLAGS) 

tpp.o : tpp.c
	$(CCBUILD_verbose)$(CC_FOR_BUILD) $(CFLAGS) -c -o $@ $(srcdir)/tpp.c $(DEFAULT_INCLUDES) \
	         -DPACKAGE=\"$(PACKAGE)\" -DVERSION=\"$(PACKAGE_VERSION)\"
#end of tpp


lib_LTLIBRARIES = libmcxx-utils.la libmcxx-extstruct.la

if WINDOWS_BUILD
no_undefined=-no-undefined
else
no_undefined=
endif

END=

libmcxx_utils_la_CFLAGS = -Wall -DLIBUTILS_DLL_EXPORT
libmcxx_utils_la_SOURCES = \
						  mcxx_refcount.c \
						  mcxx_refcount.h \
						  uniquestr.h \
						  char_hash.c \
                          libutils-common.h \
                          filename.c \
                          filename.h \
                          string_utils.h \
                          string_utils.c \
                          red_black_tree.c \
                          red_black_tree.h \
                          mem.c \
                          mem.h \
                          $(END)

libmcxx_utils_la_LDFLAGS= -avoid-version $(no_undefined)


libmcxx_extstruct_la_CFLAGS = -Wall -DLIBEXTSTRUCT_DLL_EXPORT
libmcxx_extstruct_la_SOURCES = \
                              extstruct.h \
                              extstruct.c 
libmcxx_extstruct_la_LDFLAGS=-avoid-version $(no_undefined)
libmcxx_extstruct_la_LIBADD=libmcxx-utils.la

dist-hook: tpp$(EXEEXT)

noinst_LIBRARIES = libperish.a
libperish_a_SOURCES=perish.c
