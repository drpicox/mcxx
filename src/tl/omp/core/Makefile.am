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

include $(top_srcdir)/src/tl/Makefile.phases

phases_LTLIBRARIES = libtlomp-core.la

libtlomp_core_la_CFLAGS = $(phases_cflags)
libtlomp_core_la_CXXFLAGS = $(phases_cflags) 

libtlomp_core_la_LDFLAGS = $(phases_ldflags)
libtlomp_core_la_LIBADD = $(phases_libadd) 

END=

EXTRA_DIST =
EXTRA_DIST += tl-omp-constructs.def \
			  $(END)

libtlomp_core_la_SOURCES = \
   tl-omp-core.hpp \
   tl-omp-core.cpp \
   tl-omp.hpp \
   tl-omp.cpp \
   tl-omp-deps.hpp \
   tl-omp-deps.cpp \
   tl-omp-tasks.hpp \
   tl-omp-tasks.cpp \
   tl-omp-target.hpp \
   tl-omp-target.cpp \
   tl-omp-collapse.cpp \
   tl-omp-reduction.hpp \
   tl-omp-reduction.cpp \
   tl-omp-reduction-builtin.cpp \
   $(END)

if SUPPORTED_SILENT_RULES
PYTHON_verbose = $(PYTHON_verbose_$(V))
PYTHON_verbose_ = $(PYTHON_verbose_$(AM_DEFAULT_VERBOSITY))
PYTHON_verbose_0 = @echo "  PYTHON" $@;
endif
