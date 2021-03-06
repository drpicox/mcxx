/*--------------------------------------------------------------------
  (C) Copyright 2006-2012 Barcelona Supercomputing Center
                          Centro Nacional de Supercomputacion
  
  This file is part of Mercurium C/C++ source-to-source compiler.
  
  See AUTHORS file in the top level directory for information
  regarding developers and contributors.
  
  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 3 of the License, or (at your option) any later version.
  
  Mercurium C/C++ source-to-source compiler is distributed in the hope
  that it will be useful, but WITHOUT ANY WARRANTY; without even the
  implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the GNU Lesser General Public License for more
  details.
  
  You should have received a copy of the GNU Lesser General Public
  License along with Mercurium C/C++ source-to-source compiler; if
  not, write to the Free Software Foundation, Inc., 675 Mass Ave,
  Cambridge, MA 02139, USA.
--------------------------------------------------------------------*/




#ifndef CXX_AMBIGUITY
#define CXX_AMBIGUITY

#include "libmcxx-common.h"
#include "cxx-macros.h"
#include "cxx-ast-decls.h"
#include "cxx-asttype.h"
#include "cxx-scope-decls.h"
#include "cxx-buildscope-decls.h"

MCXX_BEGIN_DECLS

typedef char ambiguity_check_intepretation_fun_t(AST, decl_context_t, void* info);
typedef int ambiguity_choose_interpretation_fun_t(AST current, AST previous, decl_context_t, void* info);
typedef char ambiguity_fallback_interpretation_fun_t(AST, decl_context_t, void* info);

LIBMCXX_EXTERN void solve_ambiguity_generic(AST a, decl_context_t decl_context, void *info,
        ambiguity_check_intepretation_fun_t* ambiguity_check_intepretation,
        ambiguity_choose_interpretation_fun_t* ambiguity_choose_interpretation,
        ambiguity_fallback_interpretation_fun_t* ambiguity_fallback_interpretation);

// Non contextual
LIBMCXX_EXTERN void solve_parameter_declaration_vs_type_parameter_class(AST a, decl_context_t decl_context);

// Contextual
LIBMCXX_EXTERN void solve_ambiguous_declaration(AST a, decl_context_t decl_context);
LIBMCXX_EXTERN void solve_ambiguous_declarator(AST a, decl_context_t decl_context);
LIBMCXX_EXTERN void solve_ambiguous_statement(AST a, decl_context_t decl_context);
LIBMCXX_EXTERN void solve_ambiguous_init_declarator(AST a, decl_context_t decl_context);
LIBMCXX_EXTERN void solve_ambiguous_for_init_statement(AST for_init_statement, decl_context_t decl_context);
LIBMCXX_EXTERN void solve_ambiguous_parameter_decl(AST parameter_declaration, decl_context_t decl_context);
LIBMCXX_EXTERN void solve_ambiguous_exception_decl(AST exception_decl, decl_context_t decl_context);
LIBMCXX_EXTERN void solve_ambiguous_type_specifier(AST type_specifier, decl_context_t decl_context);
LIBMCXX_EXTERN void solve_ambiguous_function_header(AST function_header, decl_context_t decl_context);
LIBMCXX_EXTERN void solve_ambiguous_template_argument(AST ambig_template_parameter, decl_context_t decl_context);
LIBMCXX_EXTERN void solve_ambiguous_nested_part(AST a, decl_context_t decl_context);

// To be deprecated
LIBMCXX_EXTERN char check_type_id_tree(AST type_id, decl_context_t decl_context);
// To be turned into a static
LIBMCXX_EXTERN char check_type_id_tree_or_class_template_name(AST type_id, decl_context_t decl_context);

LIBMCXX_EXTERN void solve_ambiguous_expression(AST ambig_expression, decl_context_t decl_context, nodecl_t* nodecl_output);

LIBMCXX_EXTERN char solve_ambiguous_list_of_expressions(AST ambiguous_list, decl_context_t decl_context, nodecl_t* nodecl_output);

LIBMCXX_EXTERN template_parameter_list_t* solve_ambiguous_list_of_template_arguments(AST ambiguous_list, decl_context_t decl_context);

LIBMCXX_EXTERN void solve_condition_ambiguity(AST a, decl_context_t decl_context);

LIBMCXX_EXTERN int either_type(AST t1, AST t2, node_t n1, node_t n2);

// States if we are checking ambiguities
LIBMCXX_EXTERN char checking_ambiguity(void);
LIBMCXX_EXTERN void enter_test_expression(void);
LIBMCXX_EXTERN void leave_test_expression(void);

// This is used when instantiating
LIBMCXX_EXTERN char get_test_expression_status(void);
LIBMCXX_EXTERN void set_test_expression_status(char c);

MCXX_END_DECLS

#endif // CXX_AMBIGUITY
