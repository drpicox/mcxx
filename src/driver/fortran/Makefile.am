##########################################################################
#  (C) Copyright 2006-2012 Barcelona Supercomputing Center               #
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

mcxxexec_PROGRAMS = mf03-prescanner
noinst_LTLIBRARIES = libmf03-driver.la


if ENABLE_PKGLIB_INSTALL
mcxxexecdir = $(pkglibdir)
else
mcxxexecdir = @bindir@
endif

common_cflags= -fexceptions \
			-I$(top_srcdir)/lib \
			-I$(top_srcdir)/src/frontend \
			-I$(top_srcdir)/src/frontend/fortran \
			-I$(top_srcdir)/src/mcxx_tl \
			-I$(top_srcdir)/src/driver \
			-I$(top_builddir)/lib \
			-I$(top_builddir)/src/frontend \
			-I$(top_builddir)/src/frontend/fortran \
			-I$(top_builddir)/src/mcxx_tl \
			-I$(top_builddir)/src/driver \
			-DPKGDATADIR=\"$(pkgdatadir)\" \
			-Wall

mf03_prescanner_CFLAGS= $(common_cflags)

mf03_prescanner_SOURCES = \
                          prescanner-driver.h \
                          prescanner-driver.c \
                          ../cxx-driver-utils.h \
                          ../cxx-driver-utils.c \
                          $(END)

mf03_prescanner_LDADD = \
   $(top_builddir)/gnulib/libgnulib.la \
   $(top_builddir)/lib/libmcxx-utils.la \
   $(top_builddir)/src/frontend/libmcxx-process.la \
   $(top_builddir)/src/frontend/fortran/libmf03-prescanner.la \
   $(END)

libmf03_driver_la_SOURCES = \
  cxx-driver-fortran.c \
  cxx-driver-fortran.h \
  $(END)

libmf03_driver_la_CFLAGS= $(common_cflags)
