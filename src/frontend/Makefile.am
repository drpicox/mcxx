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

lib_LTLIBRARIES = libmcxx-process.la libmcxx.la

SUBDIRS=. fortran

DIST_SUBDIRS=fortran

END=

EXTRA_DIST =
CLEANFILES =
BUILT_SOURCES = 

if WINDOWS_BUILD
no_undefined=-no-undefined
else
no_undefined=
endif

# Overall process library containing global data
libmcxx_process_la_CFLAGS=-DLIBMCXXPROCESS_DLL_EXPORT \
                          -I$(top_srcdir)/lib \
                          -I$(top_srcdir)/src/driver \
                          -I$(top_srcdir)/src/frontend \
                          -I$(top_srcdir)/src/frontend/fortran \
                          -I$(top_srcdir)/src/mcxx_tl \
                          -I$(top_builddir)/lib \
                          -I$(top_builddir)/src/driver \
                          -I$(top_builddir)/src/frontend \
                          -I$(top_builddir)/src/frontend/fortran \
                          -I$(top_builddir)/src/mcxx_tl \
                          $(END)

libmcxx_process_la_LDFLAGS=-avoid-version $(no_undefined)
libmcxx_process_la_LIBADD=$(top_builddir)/lib/libmcxx-utils.la
libmcxx_process_la_SOURCES = \
                             cxx-process.c \
                             cxx-process.h \
                             $(END)

# Mercurium C/C++ compiler runtime library
libmcxx_la_SOURCES = \
  cxx-limits.h \
  cxx-macros.h \
  cxx-locus.h \
  cxx-locus.c \
  cxx-asttype.h \
  cxx-asttype.c \
  cxx-asttype-str.c \
  cxx-asttype-str.h \
  cxx-asttype-str-internal.h \
  cxx-ast.c \
  cxx-ast.h \
  cxx-ast-fwd.h \
  cxx-ast-decls.h \
  cxx-nodecl.h \
  cxx-nodecl.c \
  cxx-nodecl-fwd.h \
  cxx-nodecl-decls.h \
  cxx-nodecl-checker.h \
  cxx-nodecl-checker.c \
  cxx-nodecl-output.h \
  cxx-nodecl-output.c \
  cxx-nodecl-visitor.h \
  cxx-nodecl-visitor.c \
  cxx-nodecl-shallow-copy.c \
  cxx-nodecl-deep-copy-fwd.h \
  cxx-nodecl-deep-copy.h \
  cxx-nodecl-deep-copy-base.c \
  cxx-nodecl-deep-copy.c \
  cxx-symbol-deep-copy-entity-specs.c \
  cxx-symbol-deep-copy.h \
  cxx-symbol-deep-copy.c \
  cxx-lexer.h \
  cxx-parser.h \
  cxx-parser-internal.h \
  cxx-parser.c \
  cxx-scanner.c \
  c99-scanner.c \
  c99-parser.h \
  c99-parser-internal.h \
  c99-parser.c \
  cxx-graphviz.h \
  cxx-graphviz.c \
  cxx-html.h \
  cxx-html.c \
  cxx-prettyprint.c \
  cxx-prettyprint.h \
  cxx-prettyprint-internal.h \
  cxx-utils.c \
  cxx-utils.h \
  cxx-scope.c \
  cxx-scope.h \
  cxx-entrylist-decls.h \
  cxx-entrylist.c \
  cxx-entrylist.h \
  cxx-scope-fwd.h \
  cxx-scope-decls.h \
  cxx-buildscope-decls.h \
  cxx-buildscope.c \
  cxx-buildscope.h \
  cxx-type-trie.h \
  cxx-type-trie.c \
  cxx-typeutils.c \
  cxx-typeutils.h \
  cxx-type-fwd.h \
  cxx-type-decls.h \
  cxx-typeenviron-decls.h \
  cxx-typeenviron.h \
  cxx-typeenviron.c \
  cxx-typeunif.c \
  cxx-typeunif.h \
  cxx-typeunif-decls.h \
  cxx-typededuc.c \
  cxx-typededuc.h \
  cxx-cexpr.c \
  cxx-cexpr.h \
  cxx-cexpr-fwd.h \
  cxx-cexpr-decls.h \
  cxx-ambiguity.c \
  cxx-ambiguity.h \
  cxx-solvetemplate.h \
  cxx-solvetemplate.c \
  cxx-printscope.c \
  cxx-printscope.h \
  cxx-instantiation.h \
  cxx-instantiation.c \
  cxx-dyninit.c \
  cxx-dyninit.h \
  cxx-tltype.h \
  cxx-tltype.c \
  cxx-exprtype-decls.h \
  cxx-exprtype.h \
  cxx-exprtype.c \
  cxx-koenig.h \
  cxx-koenig.c \
  cxx-gccsupport-decls.h \
  cxx-gccsupport.h \
  cxx-gccsupport.c \
  cxx-gccbuiltins.h \
  cxx-gccbuiltins-sse.h \
  cxx-gccbuiltins.c \
  cxx-gccspubuiltins.h \
  cxx-gccspubuiltins.c \
  cxx-mssupport.h \
  cxx-mssupport.c \
  cxx-pragma.h \
  cxx-pragma.c \
  cxx-upc.c \
  cxx-upc.h \
  cxx-cuda.c \
  cxx-cuda.h \
  cxx-typeorder.h \
  cxx-typeorder.c \
  cxx-overload.c \
  cxx-overload-decls.h \
  cxx-overload.h \
  cxx-codegen.h \
  cxx-diagnostic.c \
  cxx-diagnostic.h \
  cxx-placeholders.c \
  cxx-placeholders.h \
  libmcxx-common.h \
  $(END)

libmcxx_la_LIBADD = \
					$(top_builddir)/lib/libmcxx-utils.la \
					$(top_builddir)/lib/libmcxx-extstruct.la \
                    libmcxx-process.la \
					$(quadmath_LIBS) \
					$(END)

if WINDOWS_BUILD
dll_exports=-Wl,$(srcdir)/dll-exports.def
else
dll_exports=
endif

# This is for Win32
EXTRA_DIST += dll-exports.def

libmcxx_la_LDFLAGS = $(dll_exports) -avoid-version $(no_undefined)

libmcxx_la_CFLAGS = -DLIBMCXX_DLL_EXPORT \
                    -Wall \
					-Wshadow \
					-Wextra \
					-std=gnu99 \
				    -I$(top_srcdir)/lib \
				    -I$(top_srcdir)/src/driver \
					-I$(top_srcdir)/src/mcxx_tl \
					-I$(top_srcdir)/src/frontend/fortran \
                    -fexceptions \
				    -DPKGDATADIR=\"$(pkgdatadir)\"


EXTRA_DIST += cxx-lexer.l

# AST node types
AST_NODE_TYPE_FILES =
AST_NODE_TYPE_FILES += cxx-asttype-all.def
AST_NODE_TYPE_FILES += cxx-asttype-base.def
AST_NODE_TYPE_FILES += cxx-asttype-extensions.def
AST_NODE_TYPE_FILES += cxx-asttype-gcc.def
AST_NODE_TYPE_FILES += cxx-asttype-pragma.def
AST_NODE_TYPE_FILES += cxx-asttype-superscalar.def
AST_NODE_TYPE_FILES += cxx-asttype-upc.def
AST_NODE_TYPE_FILES += cxx-asttype-omp.def
AST_NODE_TYPE_FILES += cxx-asttype-cuda.def
AST_NODE_TYPE_FILES += cxx-asttype-opencl.def
AST_NODE_TYPE_FILES += cxx-asttype-xl.def
AST_NODE_TYPE_FILES += cxx-asttype-ms.def
AST_NODE_TYPE_FILES += cxx-asttype-fortran.def
# Generated
AST_NODE_TYPE_FILES_GEN = 
AST_NODE_TYPE_FILES_GEN += cxx-asttype-nodecl.def

EXTRA_DIST += $(AST_NODE_TYPE_FILES) $(AST_NODE_TYPE_FILES_GEN)

EXTRA_DIST += c99.l
EXTRA_DIST += c99.y
EXTRA_DIST += c99.y.in
EXTRA_DIST += c99-scanner.c

EXTRA_DIST += cxx03.l
EXTRA_DIST += cxx03.y
EXTRA_DIST += cxx03.y.in
EXTRA_DIST += cxx-scanner.c

ADDITIONAL_GRAMMARS = 
ADDITIONAL_GRAMMARS += cxx-subparse.y
ADDITIONAL_GRAMMARS += cxx-literal-nodes.y
ADDITIONAL_GRAMMARS += cxx-pragma.y
ADDITIONAL_GRAMMARS += cxx-omp.y
ADDITIONAL_GRAMMARS += cxx-superscalar.y
ADDITIONAL_GRAMMARS += cxx-ambig-handler.y
ADDITIONAL_GRAMMARS += cxx-placeholders.y
ADDITIONAL_GRAMMARS += c99-upc.y
ADDITIONAL_GRAMMARS += cxx-cuda.y
ADDITIONAL_GRAMMARS += cxx-opencl.y
ADDITIONAL_GRAMMARS += cxx-xl.y
ADDITIONAL_GRAMMARS += cxx-ms.y
ADDITIONAL_GRAMMARS += cxx-extensions.y
ADDITIONAL_GRAMMARS += cxx-fortran-compat.y

EXTRA_DIST += $(ADDITIONAL_GRAMMARS)

EXTRA_DIST += cxx-gccspubuiltins.def

# Built sources, they are built the first
# otherwise parallel compilation is likely to fail

BUILT_SOURCES += cxx-asttype.def
BUILT_SOURCES += cxx-asttype.c
BUILT_SOURCES += cxx-asttype.h

BUILT_SOURCES += c99.l
BUILT_SOURCES += cxx03.l

BUILT_SOURCES += c99.y
BUILT_SOURCES += cxx03.y

BUILT_SOURCES += cxx-parser.h
BUILT_SOURCES += c99-parser.h

EXTRA_DIST += gen-symbol-attrs.py
EXTRA_DIST += cxx-entity-specs.def

EXTRA_DIST += nodecl-generator.py
EXTRA_DIST += cxx-nodecl.def

TPP=$(top_builddir)/lib/tpp$(EXEEXT)

# Silent rules
if SUPPORTED_SILENT_RULES
BISON_verbose = $(BISON_verbose_$(V))
BISON_verbose_ = $(BISON_verbose_$(AM_DEFAULT_VERBOSITY))
BISON_verbose_0 = @echo "  BISON " $@;

FLEX_verbose = $(FLEX_verbose_$(V))
FLEX_verbose_ = $(FLEX_verbose_$(AM_DEFAULT_VERBOSITY))
FLEX_verbose_0 = @echo "  FLEX  " $@;

TPP_verbose = $(TPP_verbose_$(V))
TPP_verbose_ = $(TPP_verbose_$(AM_DEFAULT_VERBOSITY))
TPP_verbose_0 = @echo "  TPP   " $@;

PYTHON_verbose = $(PYTHON_verbose_$(V))
PYTHON_verbose_ = $(PYTHON_verbose_$(AM_DEFAULT_VERBOSITY))
PYTHON_verbose_0 = @echo "  PYTHON" $@;

GPERF_verbose = $(GPERF_verbose_$(V))
GPERF_verbose_ = $(GPERF_verbose_$(AM_DEFAULT_VERBOSITY))
GPERF_verbose_0 = @echo "  GPERF " $@;
endif

CLEANFILES += cxx-asttype.h
cxx-asttype.h : cxx-asttype.def nodecl-generator.py
	$(AM_V_GEN)( \
	echo "#ifndef CXX_ASTTYPE_H" > cxx-asttype.h; \
	echo "#define CXX_ASTTYPE_H" >> cxx-asttype.h; \
	echo "#include \"libmcxx-common.h\"" >> cxx-asttype.h; \
	echo "#include \"cxx-macros.h\"" >> cxx-asttype.h; \
	echo "MCXX_BEGIN_DECLS" >> cxx-asttype.h; \
	echo "/* This file has been generated. Every time you change $< it will be regenerated */" >> cxx-asttype.h; \
	echo "enum node_type {" >> cxx-asttype.h; \
	echo "AST_INVALID_NODE = 0,"  >> cxx-asttype.h; \
	$(SED) -e "/^[[:blank:]]*\(#.*\)\?$$/d" -e "s/$$/,/" < cxx-asttype.def >> cxx-asttype.h; \
	echo "AST_LAST_NODE" >> cxx-asttype.h; \
	echo "};" >> cxx-asttype.h; \
	echo "LIBMCXX_EXTERN char* ast_node_names[];" >> cxx-asttype.h; \
	echo "typedef enum node_type node_t;" >> cxx-asttype.h; \
	echo "MCXX_END_DECLS" >> cxx-asttype.h; \
	echo "#endif // CXX_ASTTYPE_H" >> cxx-asttype.h; \
    )

CLEANFILES += cxx-asttype.c
cxx-asttype.c : cxx-asttype.def cxx-asttype.h
	$(AM_V_GEN)( \
	echo "/* This file has been generated. Every time you change $< it will be regenerated */" > cxx-asttype.c; \
	echo "#include \"cxx-asttype.h\"" >> cxx-asttype.c; \
	echo "char* ast_node_names[] = {" >> cxx-asttype.c; \
	echo "[AST_INVALID_NODE] = \"AST_INVALID_NODE\", " >> cxx-asttype.c; \
	$(SED) -e "/^[[:blank:]]*\(#.*\)\?$$/d" -e "s/^.*$$/[&] = \"&\",/" < cxx-asttype.def >> cxx-asttype.c; \
	echo "[AST_LAST_NODE] = \"AST_LAST_NODE\", " >> cxx-asttype.c; \
	echo "};" >> cxx-asttype.c; \
	)

CLEANFILES += cxx-asttype.def
cxx-asttype.def : $(addprefix $(srcdir)/, $(AST_NODE_TYPE_FILES)) $(AST_NODE_TYPE_FILES_GEN)
	$(TPP_verbose)$(TPP) -o cxx-asttype.def -I $(srcdir) -I $(builddir) $(srcdir)/cxx-asttype-all.def

BUILT_SOURCES += cxx-asttype-str.gperf
CLEANFILES += cxx-asttype-str.gperf
cxx-asttype-str.gperf : cxx-asttype.def
	$(AM_V_GEN)( \
	echo "%{" > cxx-asttype-str.gperf; \
	echo "/* This file has been generated. Every time you change $+ it will be regenerated */" >> cxx-asttype-str.gperf; \
	echo "%}" >> cxx-asttype-str.gperf; \
	echo "struct node_str_t;" >> cxx-asttype-str.gperf; \
	echo "%%" >> cxx-asttype-str.gperf; \
	echo "\"AST_INVALID_NODE\", AST_INVALID_NODE" >> cxx-asttype-str.gperf; \
	$(SED) -e "/^[[:blank:]]*\(#.*\)\?$$/d" -e "s/^[[:blank:]]*\([^[:blank:]]\+\)[[:blank:]]*$$/\"\1\", \1/" < cxx-asttype.def >> cxx-asttype-str.gperf; \
	echo "\"AST_LAST_NODE\", AST_LAST_NODE" >> cxx-asttype-str.gperf; \
	echo "%%" >> cxx-asttype-str.gperf; \
    )

BUILT_SOURCES += cxx-asttype-str-internal.h
CLEANFILES += cxx-asttype-str-internal.h
cxx-asttype-str-internal.h: cxx-asttype-str.gperf
if GPERF_BUILDING
	$(GPERF_verbose)$(GPERF) --language=ANSI-C --hash-function-name=ast_node_kind_hash --struct-type \
      --lookup-function-name=ast_node_name_to_kind_ --output=cxx-asttype-str-internal.h \
      --initializer-suffix=,AST_INVALID_NODE cxx-asttype-str.gperf
else
	@echo "*** ERROR: file cxx-asttype-str.gperf was modified but no suitable gperf was found during configure ***"
	@exit 1
endif

if FLEX_BUILDING
CLEANFILES += cxx-scanner.c
CLEANFILES += cxx03.l
endif

cxx-scanner.c : cxx03.l cxx-parser.h
if FLEX_BUILDING
	$(FLEX_verbose)$(FLEX) -Pmcxx -d -ocxx-scanner.c -8 cxx03.l
else
	@echo "*** ERROR: file cxx03.l was modified but no suitable flex was found during configure ***"
	@exit 1
endif

cxx03.l : $(srcdir)/cxx-lexer.l 
	$(TPP_verbose)(rm -f cxx03.l && $(TPP) -o cxx03.l -D CPLUSPLUS $(srcdir)/cxx-lexer.l && chmod -w cxx03.l)

if BISON_BUILDING
CLEANFILES += cxx-parser.c
CLEANFILES += cxx-parser-internal.h
CLEANFILES += cxx-parser.output
CLEANFILES += cxx03.y
endif

cxx03.y : $(srcdir)/cxx03.y.in $(addprefix $(srcdir)/, $(ADDITIONAL_GRAMMARS)) 
	$(TPP_verbose)(rm -f cxx03.y && $(TPP) -o cxx03.y -D CPLUSPLUS -I$(srcdir) $(srcdir)/cxx03.y.in && chmod -w cxx03.y)

cxx-parser.c : cxx03.y
if BISON_BUILDING
	$(BISON_verbose)$(BISON) -pmcxx --debug --defines=cxx-parser-internal.h --report=all --output=cxx-parser.c cxx03.y
else
	@echo "*** ERROR: file cxx03.y was modified but no suitable bison was found during configure ***"
	@exit 1
endif

# These are obtained when invoking bison, if we write a rule like 'a b : d' it
# breaks parallel compilations since bison gets invoked twice.
cxx-parser.h : cxx-parser.c
cxx-parser.output : cxx-parser.c

if FLEX_BUILDING
CLEANFILES += c99-scanner.c
CLEANFILES += c99.l
endif

c99-scanner.c : c99.l c99-parser.h
if FLEX_BUILDING
	$(FLEX_verbose)$(FLEX) -Pmc99 -d -oc99-scanner.c -8 c99.l
else
	@echo "*** ERROR: file c99.l was modified but no suitable flex was found during configure ***"
	@exit 1
endif

c99.l : $(srcdir)/cxx-lexer.l
	$(TPP_verbose)(rm -f c99.l && $(TPP) -o c99.l -D C99 $(srcdir)/cxx-lexer.l && chmod -w c99.l)

if BISON_BUILDING
CLEANFILES += c99-parser.c
CLEANFILES += c99-parser-internal.h
CLEANFILES += c99-parser.output
CLEANFILES += c99.y
endif

c99.y : $(srcdir)/c99.y.in $(addprefix $(srcdir)/, $(ADDITIONAL_GRAMMARS)) 
	$(TPP_verbose)(rm -f c99.y && $(TPP) -o c99.y -D C99 -I$(srcdir) $(srcdir)/c99.y.in)

c99-parser.c : c99.y
if BISON_BUILDING
	$(BISON_verbose)$(BISON) -pmc99 --debug --defines=c99-parser-internal.h --report=all --output=c99-parser.c c99.y
else
	@echo "*** ERROR: file c99.y was modified but no suitable bison was found during configure ***"
	@exit 1
endif

# These are obtained when invoking bison, if we write a rule like 'a b : d' it
# breaks parallel compilations since bison gets invoked twice.
c99-parser.h : c99-parser.c
c99-parser.output : c99-parser.c

CLEANFILES    += cxx-entity-specs.h
BUILT_SOURCES += cxx-entity-specs.h
cxx-entity-specs.h : $(srcdir)/cxx-entity-specs.def $(srcdir)/gen-symbol-attrs.py
	$(PYTHON_verbose)$(PYTHON) $(srcdir)/gen-symbol-attrs.py $(srcdir)/cxx-entity-specs.def > $@

CLEANFILES    += cxx-symbol-deep-copy-entity-specs.c
BUILT_SOURCES += cxx-symbol-deep-copy-entity-specs.c
cxx-symbol-deep-copy-entity-specs.c : $(srcdir)/cxx-entity-specs.def $(srcdir)/gen-symbol-attrs.py
	$(PYTHON_verbose)$(PYTHON) $(srcdir)/gen-symbol-attrs.py $(srcdir)/cxx-entity-specs.def c_deep_copy_entity_specs > $@

NODECL_DEPS = $(srcdir)/cxx-nodecl.def $(srcdir)/nodecl-generator.py

CLEANFILES    += cxx-nodecl-checker.c
BUILT_SOURCES += cxx-nodecl-checker.c
cxx-nodecl-checker.c : $(NODECL_DEPS)
	$(PYTHON_verbose)$(PYTHON) $(srcdir)/nodecl-generator.py $(srcdir)/cxx-nodecl.def > $@

CLEANFILES    += cxx-nodecl-output.c
BUILT_SOURCES += cxx-nodecl-output.c
cxx-nodecl-output.c : $(NODECL_DEPS)
	$(PYTHON_verbose)$(PYTHON) $(srcdir)/nodecl-generator.py $(srcdir)/cxx-nodecl.def generation_routines_impl > $@

CLEANFILES    += cxx-nodecl-output.h
BUILT_SOURCES += cxx-nodecl-output.h
cxx-nodecl-output.h : $(NODECL_DEPS)
	$(PYTHON_verbose)$(PYTHON) $(srcdir)/nodecl-generator.py $(srcdir)/cxx-nodecl.def generation_routines_header > $@

CLEANFILES    += cxx-nodecl-visitor.h
BUILT_SOURCES += cxx-nodecl-visitor.h
cxx-nodecl-visitor.h : $(NODECL_DEPS)
	$(PYTHON_verbose)$(PYTHON) $(srcdir)/nodecl-generator.py $(srcdir)/cxx-nodecl.def c_visitor_decl > $@

CLEANFILES    += cxx-nodecl-visitor.c
BUILT_SOURCES += cxx-nodecl-visitor.c
cxx-nodecl-visitor.c : $(NODECL_DEPS)
	$(PYTHON_verbose)$(PYTHON) $(srcdir)/nodecl-generator.py $(srcdir)/cxx-nodecl.def c_visitor_def > $@

CLEANFILES    += cxx-nodecl-shallow-copy.c
BUILT_SOURCES += cxx-nodecl-shallow-copy.c
cxx-nodecl-shallow-copy.c : $(NODECL_DEPS)
	$(PYTHON_verbose)$(PYTHON) $(srcdir)/nodecl-generator.py $(srcdir)/cxx-nodecl.def c_shallow_copy_def > $@

CLEANFILES    += cxx-nodecl-deep-copy-base.c
BUILT_SOURCES += cxx-nodecl-deep-copy-base.c
cxx-nodecl-deep-copy-base.c : $(NODECL_DEPS)
	$(PYTHON_verbose)$(PYTHON) $(srcdir)/nodecl-generator.py $(srcdir)/cxx-nodecl.def c_deep_copy_def > $@

CLEANFILES    += cxx-asttype-nodecl.def
BUILT_SOURCES += cxx-asttype-nodecl.def
cxx-asttype-nodecl.def : $(NODECL_DEPS)
	$(PYTHON_verbose)$(PYTHON) $(srcdir)/nodecl-generator.py $(srcdir)/cxx-nodecl.def asttype_nodecl > $@
