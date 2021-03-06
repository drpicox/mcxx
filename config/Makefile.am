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

TPP=$(top_builddir)/lib/tpp$(EXEEXT)
CLEANFILES=
END=

CONFIG_MCXX=\
            config.mcxx \
            $(END)

# We need a dist hook to keep these files in sync with their template.*
# see below
CONFIG_FILES =  \
       50.config.cuda \
       50.config.gpu \
       10.config.omp-base \
       10.config.analysis-base \
       50.config.omp.ocl \
       50.config.omp.mercurium \
       50.config.omp.profiling \
       50.config.omp.fortran \
       50.config.omp.fpga \
       55.config.omp.intel \
       55.config.omp.intel.fortran \
       55.config.omp.ibm \
       55.config.omp.ibm.fortran \
       55.config.omp.nvidia \
       56.config.omp.mpi \
       57.config.omp.all \
       00.config.plain \
	   $(END)

INSTALL_CONFIG_FILES = @INSTALL_CONFIG_FILES@

dist-hook:
	for i in $(CONFIG_FILES) $(CONFIG_MCXX); \
    do \
       cp -v "$(srcdir)/template.$${i}" "$(distdir)" || exit $?; \
    done

CLEANFILES += $(CONFIG_FILES) $(CONFIG_MCXX)

# Replacements are defined in this file
include ./Makefile.replace

BUILT_SOURCES=$(CONFIG_MCXX) $(INSTALL_CONFIG_FILES)

# This is valid in parallel make since we are generating one goal file at a time
$(INSTALL_CONFIG_FILES) $(CONFIG_MCXX): % : template.% Makefile
	$(AM_V_GEN)( $(config_edit) "$(srcdir)/template.$@" > "$@.tmp"; mv -f "$@.tmp" "$@" )

# Where and what to install
pkgdata_DATA = \
		config.mcxx \
		$(END)

configdir = $(pkgdatadir)/config.d
config_DATA = \
        $(INSTALL_CONFIG_FILES)
