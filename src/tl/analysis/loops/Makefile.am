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

include $(top_srcdir)/src/tl/Makefile.tl

lib_LTLIBRARIES = libloops_analysis.la

libloops_analysis_la_CFLAGS = $(tl_cflags)
libloops_analysis_la_CXXFLAGS = $(tl_cflags) -I $(srcdir)/../../ -I $(srcdir)/../common -I $(srcdir)/../pcfg
libloops_analysis_la_LDFLAGS = $(tl_ldflags)
libloops_analysis_la_LIBADD = $(tl_libadd) ../../libtl.la ../common/libanalysis_utils.la ../pcfg/libpcfg.la

END=

libloops_analysis_la_SOURCES = tl-iv-analysis.hpp \
                               tl-iv-analysis.cpp \
                               tl-loop-analysis.hpp \
                               tl-loop-analysis.cpp \
                               tl-renaming-visitor.hpp \
                               $(END)
