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

END=

SUBDIRS = . codegen analysis vectorizer omp vector-lowering hlt @TL_EXTRAS@

DIST_SUBDIRS= analysis \
              vectorizer \
              omp \
              instr \
              examples \
              old_examples \
              testing \
              hlt \
              old_hlt \
              vector-lowering \
              codegen \
              $(END)

lib_LTLIBRARIES = libtl.la

BUILT_SOURCES=
BUILT_SOURCES += $(top_builddir)/src/frontend/cxx-asttype.h
BUILT_SOURCES += tl-lexer-tokens.hpp

BUILT_SOURCES += tl-nodecl-visitor.hpp
BUILT_SOURCES += tl-nodecl-visitor.cpp
BUILT_SOURCES += tl-nodecl-fwd.hpp
BUILT_SOURCES += tl-nodecl.hpp
BUILT_SOURCES += tl-nodecl.cpp

CLEANFILES =
CLEANFILES += tl-lexer-tokens.hpp

COMMON_CFLAGS= -DLIBTL_DLL_EXPORT -Wall \
    -I $(top_srcdir)/src/tl/codegen/common \
    -I $(top_srcdir)/src/tl/codegen/base/cxx \
    -I $(top_srcdir)/src/tl/codegen/base/fortran \
    -I $(top_srcdir)/src/frontend \
    -I $(top_srcdir)/src/frontend/fortran \
    -I $(top_srcdir)/src/driver \
    -I $(top_srcdir)/src/mcxx_tl \
    -I $(top_srcdir)/lib \
    -I $(top_builddir)/src/tl/codegen/common \
    -I $(top_builddir)/src/tl/codegen/base/cxx \
    -I $(top_builddir)/src/tl/codegen/base/fortran \
    -I $(top_builddir)/src/frontend \
    -I $(top_builddir)/src/frontend/fortran \
    -I $(top_builddir)/src/driver \
    -I $(top_builddir)/src/mcxx_tl \
    -I $(top_builddir)/lib \
    $(END)

# Silent rules
if SUPPORTED_SILENT_RULES
PYTHON_verbose = $(PYTHON_verbose_$(V))
PYTHON_verbose_ = $(PYTHON_verbose_$(AM_DEFAULT_VERBOSITY))
PYTHON_verbose_0 = @echo "  PYTHON" $@;
endif

libtl_la_CFLAGS = $(COMMON_CFLAGS)
libtl_la_CXXFLAGS = $(COMMON_CFLAGS)

EXTRA_DIST=Makefile.phases \
           $(END)

libtl_la_SOURCES = \
    tl-common.hpp \
    tl-object.hpp \
    tl-object.cpp \
    tl-builtin.hpp \
    tl-builtin.cpp \
    tl-nodecl-visitor.hpp \
    tl-nodecl-visitor.cpp \
    tl-nodecl-fwd.hpp \
    tl-nodecl-base-fwd.hpp \
    tl-nodecl-base.hpp \
    tl-nodecl-base.cpp \
    tl-nodecl.hpp \
    tl-nodecl.cpp \
    tl-nodecl-utils.hpp \
    tl-nodecl-utils.cpp \
    tl-nodecl-utils-fortran.hpp \
    tl-nodecl-utils-fortran.cpp \
    tl-nodecl-utils-c.hpp \
    tl-nodecl-utils-c.cpp \
    tl-nodecl-calc.hpp \
    tl-nodecl-calc.cpp \
    tl-nodecl-replace.hpp \
    tl-nodecl-replace.cpp \
    tl-scope.hpp \
    tl-scope-fwd.hpp \
    tl-scope.cpp \
    tl-dto.hpp \
    tl-dto.cpp \
    tl-symbol-fwd.hpp \
    tl-symbol.hpp \
    tl-symbol.cpp \
    tl-symbol-utils.hpp \
    tl-symbol-utils.cpp \
    tl-compilerphase.hpp \
    tl-compilerphase.cpp \
    tl-lexer.hpp \
    tl-lexer.cpp \
    tl-lexer-tokens.hpp \
    tl-source-fwd.hpp \
    tl-source.hpp \
    tl-source.cpp \
    tl-type-fwd.hpp \
    tl-type.hpp \
    tl-type.cpp \
    tl-objectlist.hpp \
    tl-objectlist.cpp \
    tl-externalvars.hpp \
    tl-externalvars.cpp \
    tl-refptr.hpp \
    tl-multifile.hpp \
    tl-multifile.cpp \
    tl-compilerpipeline.cpp \
    tl-compilerpipeline.hpp \
    tl-counters.hpp \
    tl-counters.cpp \
    tl-setdto-phase.hpp \
    tl-setdto-phase.cpp \
    tl-predicate.hpp \
    tl-predicate.cpp \
    tl-functor.hpp \
    tl-functor.cpp \
    tl-handler.hpp \
    tl-handler.cpp \
    tl-predicateutils.hpp \
    tl-predicateutils.cpp \
    tl-clauses-info.hpp \
    tl-clauses-info.cpp \
    tl-pragmasupport.hpp \
    tl-pragmasupport.cpp \
    tl-datareference.hpp \
    tl-datareference.cpp \
    tl-replace.hpp \
    tl-replace.cpp \
    tl-modules-fwd.hpp \
    tl-modules.hpp \
    tl-modules.cpp \
    $(END)

if WINDOWS_BUILD
no_undefined=-no-undefined
else
no_undefined=
endif

PARSER_INFO_C=$(builddir)/../frontend/c99-parser-internal.h
PARSER_INFO_CXX=$(builddir)/../frontend/cxx-parser-internal.h
PARSER_INFO_FORTRAN=$(builddir)/../frontend/fortran/fortran03-parser-internal.h

# Flex makes things very difficult
tl-lexer-tokens.hpp : $(PARSER_INFO_C) $(PARSER_INFO_CXX) $(PARSER_INFO_FORTRAN)
	$(AM_V_GEN)((\
	 echo "#ifndef TL_LEXER_TOKENS_HPP" ; \
	 echo "#define TL_LEXER_TOKENS_HPP" ; \
	 echo "namespace TL {"  ; \
	 echo "namespace TokensC {"  ; \
	 $(SED) -n '/enum\s\+yytokentype/,/}/p' $(PARSER_INFO_C) ; \
	 echo "}"  ; \
	 echo "namespace TokensCXX {" ; \
	 $(SED) -n '/enum\s\+yytokentype/,/}/p' $(PARSER_INFO_CXX) ; \
	 echo "}"  ; \
	 if [ -e "$(PARSER_INFO_FORTRAN)" ]; then \
	    echo "namespace TokensFortran {"  ; \
	    $(SED) -n '/enum\s\+yytokentype/,/}/p' $(PARSER_INFO_FORTRAN) ; \
	    echo "}"  ; \
	 fi; \
	 echo "}";  \
	 echo "#endif // TL_LEXER_TOKENS_HPP" ; \
	) > $@)

NODECL_DEPS=$(srcdir)/../frontend/nodecl-generator.py $(srcdir)/../frontend/cxx-nodecl.def

CLEANFILES += tl-nodecl-fwd.hpp
tl-nodecl-fwd.hpp : $(NODECL_DEPS)
	$(PYTHON_verbose)$(PYTHON) $(srcdir)/../frontend/nodecl-generator.py $(srcdir)/../frontend/cxx-nodecl.def cxx_nodecl_class_fwd_header > $@
CLEANFILES += tl-nodecl.hpp
tl-nodecl.hpp : $(NODECL_DEPS)
	$(PYTHON_verbose)$(PYTHON) $(srcdir)/../frontend/nodecl-generator.py $(srcdir)/../frontend/cxx-nodecl.def cxx_nodecl_class_header > $@
CLEANFILES += tl-nodecl.cpp
tl-nodecl.cpp : $(NODECL_DEPS)
	$(PYTHON_verbose)$(PYTHON) $(srcdir)/../frontend/nodecl-generator.py $(srcdir)/../frontend/cxx-nodecl.def cxx_nodecl_class_impl > $@
CLEANFILES += tl-nodecl-visitor.hpp
tl-nodecl-visitor.hpp : $(NODECL_DEPS)
	$(PYTHON_verbose)$(PYTHON) $(srcdir)/../frontend/nodecl-generator.py $(srcdir)/../frontend/cxx-nodecl.def cxx_visitor_decl > $@
CLEANFILES += tl-nodecl-visitor.cpp
tl-nodecl-visitor.cpp : $(NODECL_DEPS)
	$(PYTHON_verbose)$(PYTHON) $(srcdir)/../frontend/nodecl-generator.py $(srcdir)/../frontend/cxx-nodecl.def cxx_visitor_impl > $@

libtl_la_LDFLAGS = -avoid-version $(no_undefined)
libtl_la_LIBADD = \
	$(top_builddir)/lib/libmcxx-extstruct.la \
	$(top_builddir)/lib/libmcxx-utils.la \
	$(top_builddir)/src/frontend/libmcxx-process.la \
	$(top_builddir)/src/frontend/libmcxx.la \
	$(END)

libtl_la_LIBADD += $(top_builddir)/src/frontend/fortran/libmf03.la
