/*--------------------------------------------------------------------
  (C) Copyright 2006-2013 Barcelona Supercomputing Center
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




#include <string.h>
#include "cxx-utils.h"
#include "cxx-ast.h"
#include "cxx-solvetemplate.h"
#include "cxx-instantiation.h"
#include "cxx-prettyprint.h"
#include "cxx-buildscope.h"
#include "cxx-typeutils.h"
#include "cxx-typededuc.h"
#include "cxx-exprtype.h"
#include "cxx-cexpr.h"
#include "cxx-ambiguity.h"
#include "cxx-scope.h"
#include "cxx-entrylist.h"
#include "cxx-graphviz.h"
#include "cxx-diagnostic.h"
#include "cxx-codegen.h"

#include "cxx-printscope.h"

AST instantiate_tree(AST orig_tree, decl_context_t context_of_being_instantiated);

static scope_entry_t* add_duplicate_member_to_class(decl_context_t context_of_being_instantiated,
        type_t* being_instantiated,
        scope_entry_t* member_of_template)
{
    scope_entry_t* new_member = new_symbol(context_of_being_instantiated,
            context_of_being_instantiated.current_scope,
            member_of_template->symbol_name);

    *new_member = *member_of_template;

    new_member->entity_specs.is_member = 1;
    new_member->entity_specs.is_instantiable = 0;
    new_member->entity_specs.is_user_declared = 0;
    new_member->entity_specs.is_member_of_anonymous = 0;
    new_member->decl_context = context_of_being_instantiated;
    new_member->entity_specs.class_type = being_instantiated;

    class_type_add_member(get_actual_class_type(being_instantiated), new_member);

    return new_member;
}

typedef
struct type_map_tag
{
    type_t* orig_type;
    type_t* new_type;
} type_map_t;

typedef
struct translation_info_tag
{
    decl_context_t context_of_template;
    decl_context_t context_of_being_instantiated;
} translation_info_t;
#if 0
static decl_context_t translation_function(decl_context_t decl_context, void *d)
{
    translation_info_t * p = (translation_info_t*)d;

    decl_context_t result = decl_context;

    DEBUG_CODE()
    {
        fprintf(stderr, "INSTANTIATION: Translating context %p (template = %p || being instantiated = %p)\n", 
                result.class_scope, 
                p->context_of_template.class_scope, 
                p->context_of_being_instantiated.class_scope);
    }

    if (result.class_scope == p->context_of_template.class_scope)
    {
        result.class_scope = p->context_of_being_instantiated.class_scope;
    }
    if (result.current_scope == p->context_of_template.current_scope)
    {
        result.current_scope = p->context_of_being_instantiated.class_scope;
    }

    return result;
}
#endif
static scope_entry_t* instantiate_template_type_member(type_t* template_type, 
        decl_context_t context_of_being_instantiated,
        scope_entry_t *member_of_template,
        type_t* being_instantiated, 
        char is_class,
        const locus_t* locus,
        type_map_t** template_map, 
        int *num_items_template_map)
{
    // This is the primary template
    template_parameter_list_t* template_parameters = template_type_get_template_parameters(template_type);


    template_parameter_list_t* updated_template_parameters = duplicate_template_argument_list(template_parameters);
    updated_template_parameters->enclosing = context_of_being_instantiated.template_parameters;

    decl_context_t new_context_for_template_parameters = context_of_being_instantiated;
    new_context_for_template_parameters.template_parameters = updated_template_parameters;

        
    // Update the template parameters
    int i;
    for (i = 0; i < updated_template_parameters->num_parameters; i++)
    {
        if (updated_template_parameters->arguments[i] != NULL)
        {
            updated_template_parameters->arguments[i] = update_template_parameter_value(
                    template_parameters->arguments[i],
                    new_context_for_template_parameters, 
                    locus);

            if (updated_template_parameters->arguments[i] == NULL)
            {
                error_printf("%s: could not instantiate template arguments of template type\n", 
                        locus_to_str(locus));
                return NULL;
            }
        }
    }

    type_t* base_type = NULL;

    if (is_class)
    {
        base_type = member_of_template->type_information;
    }
    else
    {
        base_type = update_type_for_instantiation(
                member_of_template->type_information,
                new_context_for_template_parameters,
                locus);
    }

    scope_entry_t* new_member = new_symbol(new_context_for_template_parameters, 
            new_context_for_template_parameters.current_scope,
            member_of_template->symbol_name);

    new_member->kind = SK_TEMPLATE;
    new_member->type_information = 
        get_new_template_type(updated_template_parameters,
                base_type,
                new_member->symbol_name,
                new_context_for_template_parameters,
                member_of_template->locus);

    new_member->entity_specs.is_member = 1;
    new_member->entity_specs.class_type = being_instantiated;

    new_member->locus = member_of_template->locus;

    type_map_t new_map;
    new_map.orig_type = template_type;
    new_map.new_type = new_member->type_information;

    DEBUG_CODE()
    {
        fprintf(stderr, "INSTANTIATION: Adding new template to template map\n");
    }

    P_LIST_ADD((*template_map), (*num_items_template_map), new_map);

    template_type_set_related_symbol(new_member->type_information, new_member);

    type_t* new_primary_template = template_type_get_primary_type(new_member->type_information);

    scope_entry_t* new_primary_symbol = named_type_get_symbol(new_primary_template);
    new_primary_symbol->decl_context = new_context_for_template_parameters;

    new_primary_symbol->entity_specs = named_type_get_symbol(
            template_type_get_primary_type(
                    template_specialized_type_get_related_template_type(member_of_template->type_information)))->entity_specs;

    new_primary_symbol->entity_specs.is_instantiable = 1;
    new_primary_symbol->entity_specs.is_user_declared = 0;
    new_primary_symbol->entity_specs.class_type = being_instantiated;

    class_type_add_member(
            get_actual_class_type(being_instantiated),
            new_primary_symbol);

    if (is_class)
    {
        // Fix some bits inherited from the original class type
        class_type_set_enclosing_class_type(get_actual_class_type(new_primary_template),
                get_actual_class_type(being_instantiated));
    }

    return new_member;
}

static void instantiate_emit_member_function(scope_entry_t* entry, const locus_t* locus);

static void instantiate_member(type_t* selected_template UNUSED_PARAMETER, 
        type_t* being_instantiated, 
        scope_entry_t* member_of_template, 
        decl_context_t context_of_being_instantiated,
        const locus_t* locus,
        type_map_t** template_map, 
        int *num_items_template_map,
        type_map_t** enum_map,
        int *num_items_enum_map,
        type_map_t** anonymous_unions_map,
        int *num_items_anonymous_unions_map
        )
{
    DEBUG_CODE()
    {
        fprintf(stderr, "INSTANTIATION: Instantiating member '%s' at '%s'\n", 
                member_of_template->symbol_name,
                locus_to_str(member_of_template->locus));
    }

    switch (member_of_template->kind)
    {
        case SK_VARIABLE:
            {
                scope_entry_t* new_member = add_duplicate_member_to_class(context_of_being_instantiated,
                        being_instantiated,
                        member_of_template);


                if (is_named_class_type(member_of_template->type_information)
                        && named_type_get_symbol(member_of_template->type_information)->entity_specs.is_anonymous_union)
                {
                    // New class type
                    type_t* new_type = NULL;
                    // Lookup of related anonymous_unions type
                    int i;
                    for (i = 0; i < (*num_items_anonymous_unions_map); i++)
                    {
                        if ((*anonymous_unions_map)[i].orig_type == member_of_template->type_information)
                        {
                            new_type = (*anonymous_unions_map)[i].new_type;
                            break;
                        }
                    }

                    ERROR_CONDITION(new_type == NULL, "Anonymous union type not found in the map type!\n", 0);
                    new_member->type_information = new_type;
                }
                else
                {
                    new_member->type_information = update_type_for_instantiation(
                            new_member->type_information,
                            context_of_being_instantiated,
                            locus);
                }

                if (is_named_class_type(new_member->type_information))
                {
                    type_t* t = advance_over_typedefs(new_member->type_information);

                    scope_entry_t* class_entry = named_type_get_symbol(t);
                    instantiate_template_class_if_needed(class_entry, context_of_being_instantiated, locus);
                }

                if (new_member->entity_specs.is_bitfield)
                {
                    // Evaluate the bitfield expression
                    if (nodecl_is_constant(new_member->entity_specs.bitfield_size))
                    {
                        if (const_value_is_zero(
                                    const_value_gt(
                                        nodecl_get_constant(new_member->entity_specs.bitfield_size),
                                        const_value_get_zero(/* bytes*/ 4, /* sign */ 1))))
                        {
                            running_error("%s: error: invalid bitfield of size '%d'",
                                    locus_to_str(new_member->locus),
                                    const_value_cast_to_4(
                                        nodecl_get_constant(new_member->entity_specs.bitfield_size)));
                        }

                        new_member->entity_specs.bitfield_size_context =
                            context_of_being_instantiated;
                    }
                    else
                    {
                        // ???? FIXME - What about sizes that depend on a nontype template argument?
                        running_error("%s: error: bitfield specification is not a constant expression", 
                                locus_to_str(new_member->locus));
                    }
                }

                if (!nodecl_is_null(member_of_template->value))
                {
                    nodecl_t new_expr = instantiate_expression(member_of_template->value, context_of_being_instantiated);

                    // Update the value of the new instantiated member
                    new_member->value = new_expr;

                    if (nodecl_get_kind(new_expr) == NODECL_CXX_INITIALIZER
                            || nodecl_get_kind(new_expr) == NODECL_CXX_EQUAL_INITIALIZER
                            || nodecl_get_kind(new_expr) == NODECL_CXX_PARENTHESIZED_INITIALIZER
                            || nodecl_get_kind(new_expr) == NODECL_CXX_BRACED_INITIALIZER)
                    {
                        check_nodecl_initialization(new_expr, context_of_being_instantiated,
                                get_unqualified_type(new_member->type_information), &new_member->value);
                    }
                    else
                    {
                        // No need to check anything
                    }
                }

                DEBUG_CODE()
                {
                    fprintf(stderr, "INSTANTIATION: Member '%s' is a %s data member with type '%s'\n", 
                            new_member->symbol_name,
                            new_member->entity_specs.is_static ? "static" : "non-static",
                            print_type_str(new_member->type_information, context_of_being_instantiated));
                }

                break;
            }
        case SK_TYPEDEF:
            {
                scope_entry_t* new_member = add_duplicate_member_to_class(context_of_being_instantiated,
                        being_instantiated,
                        member_of_template);

                new_member->type_information = update_type_for_instantiation(
                        new_member->type_information,
                        context_of_being_instantiated,
                        locus);

                DEBUG_CODE()
                {
                    fprintf(stderr, "INSTANTIATION: Member '%s' is a typedef. Instantiated type is '%s'\n",
                            new_member->symbol_name,
                            print_type_str(new_member->type_information, context_of_being_instantiated));
                }
                ERROR_CONDITION(is_dependent_type(new_member->type_information), "Invalid type", 0);

                break;
            }
        case SK_ENUM:
            {
                scope_entry_t* new_member = add_duplicate_member_to_class(context_of_being_instantiated,
                        being_instantiated,
                        member_of_template);

                new_member->type_information = get_new_enum_type(context_of_being_instantiated);

                // FIXME!!!
                enum_type_set_underlying_type(new_member->type_information, get_signed_int_type());

                // Register a map

                type_map_t new_map;
                new_map.orig_type = member_of_template->type_information;
                new_map.new_type = get_user_defined_type(new_member);

                P_LIST_ADD((*enum_map), (*num_items_enum_map), new_map);

                break;
            }
        case SK_ENUMERATOR:
            {
                scope_entry_t* new_member = add_duplicate_member_to_class(context_of_being_instantiated,
                        being_instantiated,
                        member_of_template);

                type_t* new_type = NULL;
                // Lookup of related enum type
                int i;
                for (i = 0; i < (*num_items_enum_map); i++)
                {
                    if ((*enum_map)[i].orig_type == get_actual_enum_type(member_of_template->type_information))
                    {
                        new_type = (*enum_map)[i].new_type;
                        break;
                    }
                }

                // For named enums, the enum symbol should appear before in the class
                ERROR_CONDITION (new_type == NULL
                        && is_named_enumerated_type(member_of_template->type_information),
                        "Enum new type not found", 0);

                if (new_type == NULL)
                {
                    // Sign it now if is an unnamed enum
                    new_type = get_new_enum_type(context_of_being_instantiated);

                    type_map_t new_map;
                    new_map.orig_type = member_of_template->type_information;
                    new_map.new_type = new_type;

                    P_LIST_ADD((*enum_map), (*num_items_enum_map), new_map);
                }

                new_member->type_information = new_type;

                ERROR_CONDITION(nodecl_is_null(member_of_template->value),
                        "An enumerator always has a related expression", 0);

                new_member->value = instantiate_expression(member_of_template->value, context_of_being_instantiated);

                enum_type_add_enumerator(new_type, new_member);

                break;
            }
        case SK_CLASS:
            {
                if (!is_template_specialized_type(member_of_template->type_information))
                {
                    template_parameter_list_t* tpl_empty = xcalloc(1, sizeof(*tpl_empty));
                    tpl_empty->enclosing = context_of_being_instantiated.template_parameters;

                    decl_context_t new_context_of_being_instantiated = context_of_being_instantiated;
                    new_context_of_being_instantiated.template_parameters = tpl_empty;

                    scope_entry_t* new_fake_template_symbol = xcalloc(1, sizeof(*new_fake_template_symbol));
                    new_fake_template_symbol->kind = SK_TEMPLATE;
                    new_fake_template_symbol->symbol_name = member_of_template->symbol_name;
                    new_fake_template_symbol->locus = member_of_template->locus;

                    type_t* template_type = get_new_template_type(tpl_empty, 
                            member_of_template->type_information, 
                            member_of_template->symbol_name, 
                            new_context_of_being_instantiated, 
                            member_of_template->locus);

                    new_fake_template_symbol->type_information = template_type;
                    template_type_set_related_symbol(template_type, new_fake_template_symbol);

                    scope_entry_t* primary_template = named_type_get_symbol(template_type_get_primary_type(template_type));
                    primary_template->entity_specs.is_instantiable = 1;

                    type_t* primary_specialization = primary_template->type_information;

                    // Fix some bits inherited from the original class type
                    class_type_set_enclosing_class_type(get_actual_class_type(primary_specialization),
                            get_actual_class_type(being_instantiated));

                    set_is_complete_type(primary_specialization, is_complete_type(member_of_template->type_information));

                    scope_entry_t* new_member = named_type_get_symbol(
                            template_type_get_specialized_type(template_type,
                                tpl_empty,
                                new_context_of_being_instantiated,
                                member_of_template->locus));

                    insert_entry(context_of_being_instantiated.current_scope, new_member);

                    new_member->entity_specs = member_of_template->entity_specs;
                    new_member->entity_specs.is_member = 1;
                    new_member->entity_specs.is_user_declared = 0;
                    new_member->entity_specs.class_type = being_instantiated;

                    class_type_add_member(get_actual_class_type(being_instantiated), new_member);

                    AST orig_bases_tree, orig_body_tree;
                    class_type_get_instantiation_trees(member_of_template->type_information,
                            &orig_body_tree, &orig_bases_tree);

                    class_type_set_instantiation_trees(get_actual_class_type(new_member->type_information),
                            orig_body_tree, orig_bases_tree);


                    set_is_complete_type(new_member->type_information, 0);
                    set_is_dependent_type(new_member->type_information, /* is_dependent */ 0);

                    if (new_member->entity_specs.is_anonymous_union)
                    {
                        instantiate_template_class_if_needed(new_member, context_of_being_instantiated, locus);
                        // Add a mapping for this new anonymous union
                        type_map_t new_map;
                        new_map.orig_type = get_user_defined_type(member_of_template);
                        new_map.new_type = get_user_defined_type(new_member);

                        P_LIST_ADD((*anonymous_unions_map), (*num_items_anonymous_unions_map), new_map);

                        scope_entry_t* anon_member = finish_anonymous_class(new_member, context_of_being_instantiated);

                        anon_member->type_information = get_user_defined_type(new_member);

                        // Add this member to the current class
                        anon_member->entity_specs.is_member = 1;
                        anon_member->entity_specs.access = new_member->entity_specs.access;
                        anon_member->entity_specs.class_type = get_user_defined_type(new_member);

                        class_type_add_member(being_instantiated, anon_member);
                    }
                }
                else
                {
                    type_t* template_type = template_specialized_type_get_related_template_type(member_of_template->type_information);
                    type_t* primary_template = template_type_get_primary_type(template_type);

                    if (named_type_get_symbol(primary_template)->type_information == member_of_template->type_information)
                    {
                        scope_entry_t* new_member = instantiate_template_type_member(template_type,
                                context_of_being_instantiated,
                                member_of_template,
                                being_instantiated, 
                                /* is_class */ 1,
                                locus,
                                template_map, num_items_template_map);
                        if (new_member == NULL)
                            return;
                    }
                    else
                    {
                        type_t* new_template_type = NULL;
                        // Search in the map
                        int i;
                        DEBUG_CODE()
                        {
                            fprintf(stderr, "INSTANTIATION: Searching in template map (num_elems = %d)\n",
                                    *num_items_template_map);
                        }

                        for (i = 0; i < *num_items_template_map; i++)
                        {
                            if ((*template_map)[i].orig_type == template_type)
                            {
                                new_template_type = (*template_map)[i].new_type;
                                break;
                            }
                        }

                        if (new_template_type == NULL)
                            return;

                        template_parameter_list_t *template_params = duplicate_template_argument_list(
                                template_specialized_type_get_template_parameters(member_of_template->type_information));

                        template_params->enclosing = context_of_being_instantiated.template_parameters;

                        template_parameter_list_t *template_args = duplicate_template_argument_list(
                                template_specialized_type_get_template_arguments(member_of_template->type_information));

                        template_args->enclosing = context_of_being_instantiated.template_parameters;

                        for (i = 0; i < template_args->num_parameters; i++)
                        {
                            template_args->arguments[i] = update_template_parameter_value(
                                    template_args->arguments[i],
                                    context_of_being_instantiated, locus);
                        }

                        // Now ask a new specialization
                        type_t* new_template_specialized_type = template_type_get_specialized_type_after_type(new_template_type,
                                template_args,
                                member_of_template->type_information,
                                context_of_being_instantiated,
                                member_of_template->locus);

                        template_specialized_type_update_template_parameters(
                               named_type_get_symbol(new_template_specialized_type)->type_information,
                                template_params);

                        named_type_get_symbol(new_template_specialized_type)->entity_specs.is_instantiable = 1;
                        named_type_get_symbol(new_template_specialized_type)->entity_specs.is_user_declared = 0;

                        class_type_add_member(
                                get_actual_class_type(being_instantiated),
                                named_type_get_symbol(new_template_specialized_type));
                    }
                }
                break;
            }
        case SK_TEMPLATE:
            {
                internal_error("Code unreachable\n", 0);
                break;
            }
        case SK_FUNCTION:
            {
                scope_entry_t* new_member = NULL;
                if (!is_template_specialized_type(member_of_template->type_information))
                {
                    new_member = add_duplicate_member_to_class(context_of_being_instantiated,
                            being_instantiated,
                            member_of_template);

                    // FIXME - Maybe we should create also a 0-template like in classes?
                    new_member->type_information = update_type_for_instantiation(
                            new_member->type_information,
                            context_of_being_instantiated,
                            locus);

                    new_member->entity_specs.is_non_emitted = 1;
                    new_member->entity_specs.emission_template = member_of_template;
                    new_member->entity_specs.emission_handler = instantiate_emit_member_function;
                }
                else
                {
                    type_t* template_type = template_specialized_type_get_related_template_type(member_of_template->type_information);
                    type_t* primary_template = template_type_get_primary_type(template_type);

                    if (named_type_get_symbol(primary_template)->type_information != member_of_template->type_information)
                    {
                        internal_error("Code unreachable\n", 0);
                    }

                    new_member = instantiate_template_type_member(template_type,
                            context_of_being_instantiated,
                            member_of_template,
                            being_instantiated, 
                            /* is_class */ 0,
                            locus,
                            template_map, num_items_template_map);

                    if (new_member == NULL)
                        return;

                    new_member->defined = 1;
                    // new_member->entity_specs.is_non_emitted = 1;

                    // We work on the primary template
                    type_t* primary_type = template_type_get_primary_type(new_member->type_information);
                    new_member = named_type_get_symbol(primary_type);

                    new_member->entity_specs.is_inline = named_type_get_symbol(primary_template)->entity_specs.is_inline;
                    new_member->entity_specs.is_static = named_type_get_symbol(primary_template)->entity_specs.is_static;
                }

                DEBUG_CODE()
                {
                    fprintf(stderr, "INSTANTIATION: New member function '%s'\n",
                            print_decl_type_str(new_member->type_information, 
                                new_member->decl_context,
                                get_qualified_symbol_name(new_member, 
                                    new_member->decl_context)));
                }

                new_member->entity_specs.is_copy_constructor =
                    function_is_copy_constructor(new_member, being_instantiated);

                new_member->entity_specs.is_copy_assignment_operator =
                    function_is_copy_assignment_operator(new_member, being_instantiated);

                if (member_of_template->entity_specs.is_constructor)
                {
                    if (member_of_template->entity_specs.is_default_constructor)
                    {
                        class_type_set_default_constructor(get_actual_class_type(being_instantiated), new_member);
                    }
                }
                if (member_of_template->entity_specs.is_destructor)
                {
                    class_type_set_destructor(get_actual_class_type(being_instantiated), new_member);
                }

                break;
            }
            // This is only possible because of using declarations / or qualified members
            // which refer to dependent entities
            // case SK_DEPENDENT_ENTITY:
            //     {
            //         ERROR_CONDITION(member_of_template->language_dependent_value == NULL,
            //                 "Invalid expression for dependent entity", 0);

            //         scope_entry_list_t *entry_list = query_id_expression(context_of_being_instantiated, member_of_template->language_dependent_value);
            //         
            //         if (entry_list == NULL
            //                 || !entry_list_head(entry_list)->entity_specs.is_member)
            //         {
            //             running_error("%s: invalid using declaration '%s' while instantiating\n", 
            //                     ast_location(member_of_template->language_dependent_value),
            //                     prettyprint_in_buffer(member_of_template->language_dependent_value));
            //         }

            //         scope_entry_t* entry = entry_list_head(entry_list);
            //         if (!class_type_is_base(entry->entity_specs.class_type, 
            //                     get_actual_class_type(being_instantiated)))
            //         {
            //             running_error("%s: entity '%s' is not a member of a base of class '%s'\n",
            //                     ast_location(member_of_template->language_dependent_value),
            //                         get_qualified_symbol_name(entry,
            //                             context_of_being_instantiated),
            //                         get_qualified_symbol_name(named_type_get_symbol(being_instantiated), 
            //                             context_of_being_instantiated)
            //                     );
            //         }

            //         scope_entry_list_iterator_t* it = NULL;
            //         for (it = entry_list_iterator_begin(entry_list);
            //                 !entry_list_iterator_end(it);
            //                 entry_list_iterator_next(it))
            //         {
            //             entry = entry_list_iterator_current(it);
            //             class_type_add_member(get_actual_class_type(being_instantiated), entry);

            //             // Insert the symbol in the context
            //             insert_entry(context_of_being_instantiated.current_scope, entry);
            //         }
            //         entry_list_iterator_free(it);
            //         entry_list_free(entry_list);

            //         break;
            //     }
        case SK_USING:
        case SK_USING_TYPENAME:
            {
                // Two cases: a) the entity is actually dependent: it will have only one SK_DEPENDENT_ENTITY 
                //            b) the entity is not dependent: it may have more than one element
                scope_entry_list_t* entry_list = unresolved_overloaded_type_get_overload_set(member_of_template->type_information);
                scope_entry_t* entry = entry_list_head(entry_list);

                if (entry->kind == SK_DEPENDENT_ENTITY)
                {
                    ERROR_CONDITION(entry_list_size(entry_list) != 1, "Invalid list", 0);

                    entry_list = query_dependent_entity_in_context(context_of_being_instantiated,
                            entry,
                            member_of_template->locus);
                }

                introduce_using_entities(
                        nodecl_null(),
                        entry_list, context_of_being_instantiated,
                        named_type_get_symbol(being_instantiated),
                        /* is_class_scope */ 1,
                        member_of_template->entity_specs.access,
                        /* is_typename */ 0,
                        member_of_template->locus);

                scope_entry_t* used_hub_symbol = xcalloc(1, sizeof(*used_hub_symbol));
                used_hub_symbol->kind = SK_USING;
                used_hub_symbol->type_information = get_unresolved_overloaded_type(entry_list, NULL);
                used_hub_symbol->entity_specs.access = member_of_template->entity_specs.access;

                class_type_add_member(get_actual_class_type(being_instantiated), used_hub_symbol);
                break;
            }
        default:
            {
                internal_error("Unexpected member kind=%s\n", symbol_kind_name(member_of_template));
            }
    }
}
#if 0
static void instantiate_dependent_friend_class(
        type_t* being_instantiated,
        scope_entry_t* friend,
        decl_context_t context_of_being_instantiated,
        const locus_t* locus)
{
    scope_entry_t* new_friend = friend;
    if (is_dependent_type(friend->type_information))
    {
        type_t* new_type = update_type_for_instantiation(friend->type_information,
                context_of_being_instantiated,
                locus);
        if (!is_dependent_typename_type(new_type))
        {
            new_friend = named_type_get_symbol(new_type);
        }
        else
        {
            new_friend = xcalloc(1, sizeof(*new_friend));

            new_friend->symbol_name = friend->symbol_name;
            new_friend->kind = SK_DEPENDENT_FRIEND_CLASS;
            new_friend->type_information = new_type;
            new_friend->line = line;
            new_friend->file = filename;
            new_friend->entity_specs = friend->entity_specs;

            new_friend->decl_context = context_of_being_instantiated;

            // We need the context of the friend declaration because in the code generation
            // phase we must print the template arguments
            new_friend->related_decl_context = friend->decl_context;

            // Link the template parameters properly
            template_parameter_list_t *new_temp_param_list =
                duplicate_template_argument_list(friend->decl_context.template_parameters);
            new_temp_param_list->enclosing = context_of_being_instantiated.template_parameters;
            new_friend->decl_context.template_parameters = new_temp_param_list;

            // Copy the type tag of the 'friend' symbol to 'new_friend' symbol
            // This type_tag will be used in codegen
            enum type_tag_t friend_kind;
            friend_kind = get_dependent_entry_kind(friend->type_information);
            set_dependent_entry_kind(new_friend->type_information, friend_kind);
        }

        // If the new type is not dependent, we change the kind of
        // the new_friend symbol to SK_CLASS
        if (!is_dependent_type(new_friend->type_information))
        {
            new_friend->kind = SK_CLASS;
        }
    }
    else
    {
        // The kind of the symbol is SK_DEPENDENT_FRIEND_CLASS but
        // his type is not dependent -> ERROR, this shouldn't never happen
        internal_error("Code unreachable.",0);
    }

    class_type_add_friend_symbol(get_actual_class_type(being_instantiated), new_friend);
}
#endif

static void instantiate_dependent_friend_function(
        type_t* being_instantiated,
        scope_entry_t* friend,
        decl_context_t context_of_being_instantiated,
        const locus_t* locus)
{
    // At the end of this function, the symbol 'new_friend' will be added to
    // the set of friends
    scope_entry_t* new_friend = NULL;

    type_t* new_type = update_type_for_instantiation(friend->type_information,
            context_of_being_instantiated, locus);

    char is_template_id = nodecl_name_ends_in_template_id(friend->value);
    char is_templ_funct_decl =(is_template_specialized_type(friend->type_information) &&
            friend->decl_context.template_parameters->parameters != context_of_being_instantiated.template_parameters->parameters);
    char is_qualified = (nodecl_get_kind(friend->value) == NODECL_CXX_DEP_NAME_NESTED
            || nodecl_get_kind(friend->value) == NODECL_CXX_DEP_GLOBAL_NAME_NESTED);

    if (is_template_id)
    {
        scope_entry_list_t* candidates_list =
            entry_list_from_symbol_array(
                    friend->entity_specs.num_friend_candidates,
                    friend->entity_specs.friend_candidates);

        // Does candidates list contain a SK_DEPENDENT_ENTITY?
        if (candidates_list != NULL)
        {
            scope_entry_t* sym = entry_list_head(candidates_list);
            if (sym->kind == SK_DEPENDENT_ENTITY)
            {
                entry_list_free(candidates_list);

                // Try to solve the dependent entity
                candidates_list =
                    query_dependent_entity_in_context(
                        context_of_being_instantiated, sym, locus);
            }
        }

        if (candidates_list != NULL)
        {

            template_parameter_list_t* explicit_temp_params = NULL;
            nodecl_t new_name = instantiate_expression(friend->value, context_of_being_instantiated);

            if (nodecl_name_ends_in_template_id(new_name))
            {
                explicit_temp_params = nodecl_name_get_last_template_arguments(new_name);
            }
            else
            {
                type_t* nodecl_type = nodecl_get_type(new_name);
                ERROR_CONDITION(nodecl_type == NULL || !is_unresolved_overloaded_type(nodecl_type), "Code unreachable\n", 0);
                explicit_temp_params = unresolved_overloaded_type_get_explicit_template_arguments(nodecl_type);
            }

            // We may need to update the explicit template argument list
            template_parameter_list_t* updated_explicit_temp_params = NULL;
            if (explicit_temp_params != NULL)
            {
                updated_explicit_temp_params = update_template_argument_list(
                        context_of_being_instantiated, explicit_temp_params, locus);
            }

            new_friend = solve_template_function(candidates_list, updated_explicit_temp_params, new_type, locus);
        }

        if (new_friend == NULL)
        {
            error_printf("%s: function '%s' shall refer a specialization of a function template\n",
                    locus_to_str(locus), friend->symbol_name);
            return;
        }
        entry_list_free(candidates_list);
    }
    else
    {
        decl_flags_t decl_flags = DF_DEPENDENT_TYPENAME;
        if (!is_qualified)
        {
            decl_flags |= DF_ONLY_CURRENT_SCOPE;
        }

        decl_context_t lookup_context = context_of_being_instantiated;
        lookup_context.current_scope = lookup_context.namespace_scope;

        scope_entry_list_t* candidates_list =
            query_nodecl_name_flags(lookup_context, friend->value, decl_flags);

        // Does candidates list contain a SK_DEPENDENT_ENTITY?
        if (candidates_list != NULL)
        {
            scope_entry_t* sym = entry_list_head(candidates_list);
            if (sym->kind == SK_DEPENDENT_ENTITY)
            {
                entry_list_free(candidates_list);

                // Try to solve the dependent entity
                candidates_list =
                    query_dependent_entity_in_context(
                            context_of_being_instantiated, sym, locus);
            }
        }

        // 1. The declaration is not a template function
        if (!is_templ_funct_decl)
        {
            ERROR_CONDITION(is_dependent_type(new_type),
                    "At this point, this type cannot be dependent\n", 0);

            // 1.1 It's a qualified or unqualified template-id -> refers to a
            // specialization of a function template
            // This case has been already handled

            scope_entry_list_t* filtered_entry_list =
                filter_symbol_kind(candidates_list, SK_FUNCTION);

            // 1.2 It's a qualified/unqualified name -> refers to a nontemplate function
            scope_entry_list_iterator_t* it = NULL;
            for (it = entry_list_iterator_begin(candidates_list);
                    !entry_list_iterator_end(it) && new_friend == NULL;
                    entry_list_iterator_next(it))
            {
                scope_entry_t* sym_candidate = entry_list_iterator_current(it);
                if (sym_candidate->kind == SK_FUNCTION
                        && equivalent_types(new_type, sym_candidate->type_information))
                {
                    new_friend = sym_candidate;
                }
            }
            entry_list_free(filtered_entry_list);

            //  1.3 It's a qualified name and we have not found a candidate in 1.2 ->
            //  refers to a matching specialization of a template function
            if (new_friend == NULL && is_qualified)
            {
                nodecl_t new_name = instantiate_expression(friend->value, context_of_being_instantiated);

                // We may need to update the explicit template argument list
                template_parameter_list_t* expl_templ_param = NULL;
                template_parameter_list_t* nodecl_templ_param =
                    nodecl_name_get_last_template_arguments(new_name);
                if (nodecl_templ_param != NULL)
                {
                    expl_templ_param = update_template_argument_list(
                            context_of_being_instantiated, nodecl_templ_param, locus);
                }

                new_friend = solve_template_function(candidates_list, expl_templ_param, new_type, locus);
                if (new_friend == NULL)
                {
                    error_printf("%s: function '%s' shall refer a nontemplate function or a specialization of a function template\n",
                            locus_to_str(locus), friend->symbol_name);
                    return;
                }
            }

            //  1.4 It's a unqualified name and we have not found a candidate in 1.2 -> declares a non template function
            if (new_friend == NULL && !is_qualified)
            {
                // A few interesting details:
                //  - The new friend symbol must be created in the innermost enclosing namespace scope
                //  - This new friend has not template parameters
                new_friend = new_symbol(context_of_being_instantiated,
                        context_of_being_instantiated.namespace_scope, friend->symbol_name);
                new_friend->decl_context.current_scope = context_of_being_instantiated.namespace_scope;
                new_friend->decl_context.template_parameters = NULL;

                new_friend->kind = SK_FUNCTION;
                new_friend->locus = locus;
                new_friend->type_information = new_type;
                new_friend->entity_specs = friend->entity_specs;
                new_friend->defined = friend->defined;
            }
        }
        // 2. Otherwise, It is a template function declaration
        else
        {
            ERROR_CONDITION(!is_dependent_type(new_type),
                    "At this point, this type must be dependent\n", 0);

            // We need to alineate the template parameters of the updated type
            // Example:
            //
            //       template < typename _T1 >
            //       struct A
            //       {
            //           template < typename _T2 >
            //           friend void foo( _T1, _T2);
            //       };
            //
            //       template < typename _T >
            //       void foo (int, _T)
            //       {
            //       }
            //
            //       A<int> a;
            //
            // The instantiation of the depedent friend function declaration
            // shall refer to ::foo, but currently the template parameters of
            // the instantiated type are not correctly alineated and do not
            // match with the template parameters of ::foo.
            //
            // Idea: We duplicate the template parameter list and
            // replace all SK_TEMPLATE_TYPE_PARAMETER by a new symbol with
            // template_parameter_nesting = 1. Later, we update the new_type
            // with this new list of template parameters

            char something_has_changed = 0;
            template_parameter_list_t* alineated_temp_params =
                duplicate_template_argument_list(friend->decl_context.template_parameters);
            int i;
            for (i = 0; i < alineated_temp_params->num_parameters; ++i)
            {
                template_parameter_t* current_temp_param = alineated_temp_params->parameters[i];
                if (current_temp_param->entry != NULL
                        && current_temp_param->entry->kind == SK_TEMPLATE_TYPE_PARAMETER)
                {
                    something_has_changed = 1;

                    scope_entry_t* new_entry = xcalloc(1, sizeof(*new_entry));
                    memcpy(new_entry, current_temp_param->entry, sizeof(*current_temp_param->entry));
                    new_entry->entity_specs.template_parameter_nesting = 1;
                    current_temp_param->entry = new_entry;

                }
            }

            if (something_has_changed)
            {
                decl_context_t new_context = context_of_being_instantiated;
                new_context.template_parameters = alineated_temp_params;
                type_t* alineated_type = update_type_for_instantiation(new_type, new_context, locus);
                new_type = alineated_type;
            }

            scope_entry_list_t* filtered_entry_list = filter_symbol_kind(candidates_list, SK_TEMPLATE);

            scope_entry_list_iterator_t* it = NULL;
            for (it = entry_list_iterator_begin(candidates_list);
                    !entry_list_iterator_end(it) && new_friend == NULL;
                    entry_list_iterator_next(it))
            {
                scope_entry_t* template_candidate = entry_list_iterator_current(it);
                scope_entry_t* primary_symbol_candidate =
                    named_type_get_symbol(template_type_get_primary_type(template_candidate->type_information));

                if (primary_symbol_candidate->kind == SK_FUNCTION
                        && equivalent_types(new_type, primary_symbol_candidate->type_information))
                {
                    new_friend = primary_symbol_candidate;
                }
            }
            entry_list_free(filtered_entry_list);

            if (new_friend == NULL)
            {
                if (is_qualified)
                {
                    error_printf("%s: template function '%s' shall refer a declared template function\n",
                            locus_to_str(locus), friend->symbol_name);
                    return;
                }
                else
                {
                    // We create a new SK_TEMPLATE symbol
                    scope_entry_t* new_template = new_symbol(context_of_being_instantiated,
                            context_of_being_instantiated.namespace_scope, friend->symbol_name);
                    new_template->decl_context.current_scope = context_of_being_instantiated.namespace_scope;

                    new_template->kind = SK_TEMPLATE;
                    new_template->locus = locus;

                    // The new template symbol created to represent the
                    // instantiation of the current template friend function
                    // only needs the last level of template parameters (the
                    // others should be independent)
                    alineated_temp_params->enclosing = NULL;
                    lookup_context.template_parameters = alineated_temp_params;


                    // We create the new_type of the new template symbol
                    new_template->type_information = get_new_template_type(alineated_temp_params,
                            new_type, new_template->symbol_name, lookup_context, locus);

                    template_type_set_related_symbol(new_template->type_information, new_template);

                    // We copy the entity specs of the friend to the primary specialization of this new template
                    type_t* new_primary_type = template_type_get_primary_type(new_template->type_information);
                    scope_entry_t* new_primary_symbol = named_type_get_symbol(new_primary_type);
                    new_primary_symbol->entity_specs = friend->entity_specs;

                    // We never add the template symbol as a friend of the class
                    // being instantiated, we always add the primary specialization
                    new_friend = new_primary_symbol;
                }
            }

        }
        entry_list_free(candidates_list);
    }

    class_type_add_friend_symbol(get_actual_class_type(being_instantiated), new_friend);
}

static void instantiate_bases(
        type_t* selected_class_type,
        type_t* instantiated_class_type,
        decl_context_t context_of_being_instantiated,
        const locus_t* locus);

static void instantiate_specialized_template_class(type_t* selected_template,
        type_t* being_instantiated,
        template_parameter_list_t* template_arguments,
        const locus_t* locus)
{
    DEBUG_CODE()
    {
        fprintf(stderr, "INSTANTIATION: Instantiating class '%s'\n", 
                print_declarator(being_instantiated));
    }

    ERROR_CONDITION(!is_named_class_type(being_instantiated), "Must be a named class", 0);

    scope_entry_t* being_instantiated_sym = named_type_get_symbol(being_instantiated);

    AST instantiation_body = NULL;
    AST instantiation_base_clause = NULL;
    class_type_get_instantiation_trees(get_actual_class_type(selected_template), 
            &instantiation_body, &instantiation_base_clause);

    instantiation_body = ast_copy_for_instantiation(instantiation_body);
    instantiation_base_clause = ast_copy_for_instantiation(instantiation_base_clause);

    // Update the template parameter with the deduced template parameters
    decl_context_t instantiation_context = being_instantiated_sym->decl_context;

    // But the selected_template might be a nested one in a dependent context so we must update
    // the enclosing template arguments with those of the original class
    ERROR_CONDITION(being_instantiated_sym->decl_context.template_parameters == NULL, "Wrong nesting in template parameters", 0);

    template_arguments->enclosing = being_instantiated_sym->decl_context.template_parameters->enclosing;

    // Our instantiation context is ready
    instantiation_context.template_parameters = template_arguments;

    template_specialized_type_update_template_parameters(being_instantiated_sym->type_information,
            instantiation_context.template_parameters);

    decl_context_t inner_decl_context = new_class_context(instantiation_context, 
            being_instantiated_sym);

    being_instantiated_sym->decl_context = instantiation_context;

    class_type_set_inner_context(being_instantiated_sym->type_information, inner_decl_context);

    //translation_info_t translation_info;
    //translation_info.context_of_template = class_type_get_inner_context(get_actual_class_type(selected_template));
    //translation_info.context_of_being_instantiated = inner_decl_context;

    // From now this class acts as instantiated
    being_instantiated_sym->entity_specs.is_instantiated = 1;

    if (instantiation_base_clause != NULL)
    {
        instantiate_bases(
                get_actual_class_type(selected_template),
                get_actual_class_type(being_instantiated),
                inner_decl_context,
                locus
                );
    }
    
    // Inject the class name
    scope_entry_t* injected_symbol = new_symbol(inner_decl_context, 
            inner_decl_context.current_scope, being_instantiated_sym->symbol_name);

    *injected_symbol = *being_instantiated_sym;

    injected_symbol->do_not_print = 1;
    injected_symbol->entity_specs.is_member = 1;
    injected_symbol->entity_specs.class_type = get_user_defined_type(being_instantiated_sym);
    injected_symbol->entity_specs.is_injected_class_name = 1;

    /*
     * Note that the standard allows code like this one
     *
     * template <typename _T>
     * struct A { };
     *
     * template <>
     * struct A<int> 
     * {
     *   typedef int K;  (1)
     *   A<int>::K k;    (2)
     * };
     *
     * So we can use the name 'A<int>::K' inside the class provided 'K' has been declared
     * before the point we refer it (so: switching declarations (1) and (2) would not work).
     *
     * This also affects the injected class-name, so, for another example
     *
     * template <typename _T>
     * struct B
     * {
     *   typedef _T P;             (3)
     *   typename B::P k;          (4)
     * };
     *
     * Note that in a partial, or not at all, specialized class the injected
     * class name is dependent so 'typename' is mandatory (like in (4)). It is
     * redundant since the injected-class name obviously refers to the current
     * class so no ambiguity would arise between identifiers and typenames.
     * Seems that a DR has been filled on that.
     *
     * All this explanation is here just to justify that the class is already
     * complete and independent just before start the parsing of its members.
     * Otherwise all the machinery would try to instantiate it again and again
     * (and this is not good at all).
     */
    set_is_complete_type(being_instantiated, /* is_complete */ 1);
    set_is_dependent_type(being_instantiated, /* is_dependent */ 0);

    set_is_complete_type(get_actual_class_type(being_instantiated), /* is_complete */ 1);
    set_is_dependent_type(get_actual_class_type(being_instantiated), /* is_dependent */ 0);

    type_map_t* template_map = NULL;
    int num_items_template_map = 0;

    type_map_t* enum_map = NULL;
    int num_items_enum_map = 0;

    type_map_t* anonymous_unions_map = NULL;
    int num_items_anonymous_unions_map = 0;

    scope_entry_list_t * members = class_type_get_members(get_actual_class_type(selected_template));
    scope_entry_list_t * friends = class_type_get_friends(get_actual_class_type(selected_template));
    DEBUG_CODE()
    {
        fprintf(stderr, "INSTANTIATION: Have to instantiate %d members\n", entry_list_size(members));
    }

    scope_entry_list_iterator_t* it = NULL;
    for (it = entry_list_iterator_begin(members);
            !entry_list_iterator_end(it);
            entry_list_iterator_next(it))
    {
        scope_entry_t* member = entry_list_iterator_current(it);

        instantiate_member(selected_template,
                being_instantiated,
                member,
                inner_decl_context,
                locus,
                &template_map, &num_items_template_map,
                &enum_map, &num_items_enum_map,
                &anonymous_unions_map, &num_items_anonymous_unions_map);
    }
    entry_list_iterator_free(it);
    entry_list_free(members);

    // Friends
    for (it = entry_list_iterator_begin(friends);
            !entry_list_iterator_end(it);
            entry_list_iterator_next(it))
    {
        scope_entry_t* friend = entry_list_iterator_current(it);

        if (friend->kind == SK_DEPENDENT_FRIEND_FUNCTION)
        {
            instantiate_dependent_friend_function(being_instantiated,
                    friend, inner_decl_context, locus);
        }
        else if(friend->kind == SK_DEPENDENT_FRIEND_CLASS)
        {
            // instantiate_dependent_friend_class(being_instantiated, friend, inner_decl_context, locus);
        }
        else if (friend->kind == SK_CLASS)
        {

            // The symbol 'friend' may has a dependent type. Example:
            //
            //
            //    template < typename T1>
            //        struct B {};
            //
            //    template < typename T2>
            //        struct A
            //        {
            //            friend struct B<T2>; (1)
            //        };
            //
            //    A<int> foo; (2)
            //
            //
            // The symbol 'B<T2>' created in (1) has kind 'SK_CLASS' but his type is dependent
            // In the instantiation (2) we should modify his type
            
            scope_entry_t* new_friend = friend;
            if (is_dependent_type(friend->type_information))
            {
                type_t* new_type = update_type_for_instantiation(get_user_defined_type(friend),
                        inner_decl_context,
                        locus);
                new_friend = named_type_get_symbol(new_type);
            }

            class_type_add_friend_symbol(get_actual_class_type(being_instantiated), new_friend);
        }
        else if (friend->kind == SK_FUNCTION)
        {
            // This code is unreachable because all the dependent friend functions 
            // of a template class always will be a SK_DEPENDENT_FRIEND_FUNCTION.
            // (See function 'find_dependent_friend_function_declaration' in buildscope)
            internal_error("Code unreachable", 0);
        }
        else
        {
            internal_error("Code unreachable", 0);
        }
    }
    entry_list_iterator_free(it);
    entry_list_free(friends);

    // The symbol is defined after this
    being_instantiated_sym->defined = 1;

    // Finish the class (this order does not match the one used in buildscope, does it?)
    nodecl_t nodecl_finish_class = nodecl_null();
    DEBUG_CODE()
    {
        fprintf(stderr, "INSTANTIATION: Finishing class '%s'\n", 
                print_declarator(being_instantiated));
    }
    finish_class_type(get_actual_class_type(being_instantiated), being_instantiated, 
            being_instantiated_sym->decl_context, locus, &nodecl_finish_class);
    DEBUG_CODE()
    {
        fprintf(stderr, "INSTANTIATION: Class '%s' finished\n", 
                print_declarator(being_instantiated));
    }

    scope_entry_t* selected_template_sym = named_type_get_symbol(selected_template);
    if (selected_template_sym->entity_specs.is_member)
    {
        scope_entry_t* enclosing_class = named_type_get_symbol(selected_template_sym->entity_specs.class_type);
        class_type_add_member(enclosing_class->type_information, being_instantiated_sym);
    }

    DEBUG_CODE()
    {
        fprintf(stderr, "INSTANTIATION: End of instantiation of class '%s'\n", 
                print_declarator(being_instantiated));
    }
}

static void instantiate_bases(
        type_t* selected_class_type,
        type_t* instantiated_class_type,
        decl_context_t context_of_being_instantiated,
        const locus_t* locus)
{
    int i, num_bases = class_type_get_num_bases(selected_class_type);

    DEBUG_CODE()
    {
        fprintf(stderr, "INSTANTIATION: Updating bases\n");
    }

    for (i = 0; i < num_bases; i++)
    {
        char is_virtual = 0;
        char is_dependent_base = 0;
        access_specifier_t access_specifier = AS_UNKNOWN;
        scope_entry_t* base_class_sym = class_type_get_base_num(selected_class_type, i, &is_virtual, 
                &is_dependent_base, &access_specifier);

        type_t* base_class_named_type = NULL;
        if (base_class_sym->kind == SK_DEPENDENT_ENTITY)
        {
            base_class_named_type = base_class_sym->type_information;
        }
        else
        {
            base_class_named_type = get_user_defined_type(base_class_sym);
        }

        DEBUG_CODE()
        {
            fprintf(stderr, "INSTANTIATION: Updating base class '%s'\n", 
                    print_declarator(base_class_named_type));
        }

        type_t* upd_base_class_named_type = update_type_for_instantiation(base_class_named_type,
                context_of_being_instantiated,
                locus);

        ERROR_CONDITION( is_dependent_type(upd_base_class_named_type), "Invalid base class update %s", 
                print_type_str(upd_base_class_named_type, context_of_being_instantiated));

        scope_entry_t* upd_base_class_sym = named_type_get_symbol(upd_base_class_named_type);

        // If the entity (being an independent one) has not been completed, then instantiate it
        instantiate_template_class_if_needed(upd_base_class_sym, context_of_being_instantiated, locus);

        class_type_add_base_class(instantiated_class_type, upd_base_class_sym, is_virtual, /* is_dependent */ 0, access_specifier);
    }

    DEBUG_CODE()
    {
        fprintf(stderr, "INSTANTIATION: Finished updating bases\n");
    }
}



static type_t* solve_template_for_instantiation(scope_entry_t* entry, decl_context_t decl_context UNUSED_PARAMETER, 
        template_parameter_list_t** deduced_template_arguments,
        const locus_t* locus)
{
    if (entry->kind != SK_CLASS
            && entry->kind != SK_TYPEDEF)
    {
        internal_error("Invalid symbol\n", 0);
    }

    if (entry->kind == SK_TYPEDEF)
    {
        entry = named_type_get_symbol(advance_over_typedefs(entry->type_information));
    }

    type_t* template_specialized_type = entry->type_information;

    DEBUG_CODE()
    {
        fprintf(stderr, "INSTANTIATION: Instantiating class '%s' at '%s'\n",
                print_type_str(get_user_defined_type(entry), entry->decl_context),
                locus_to_str(entry->locus));
    }


    if (!is_template_specialized_type(template_specialized_type)
            || !is_class_type(template_specialized_type)
            || !class_type_is_incomplete_independent(template_specialized_type))
    {
        internal_error("Symbol '%s' is not a class eligible for instantiation", entry->symbol_name);
    }

    type_t* template_type =
        template_specialized_type_get_related_template_type(template_specialized_type);


    type_t* selected_template = solve_class_template(
            template_type,
            get_user_defined_type(entry),
            deduced_template_arguments, locus);

    return selected_template;
}

static void instantiate_template_class(scope_entry_t* entry, 
        decl_context_t decl_context, 
        type_t* selected_template, 
        template_parameter_list_t* deduced_template_arguments,
        const locus_t* locus)
{
    //Ignore typedefs
    if (entry->kind != SK_CLASS)
    {
        ERROR_CONDITION(!is_named_class_type(entry->type_information), "Invalid class type", 0);

        type_t* t = advance_over_typedefs(entry->type_information);
        entry = named_type_get_symbol(t);

        ERROR_CONDITION(entry->kind != SK_CLASS, "Invalid class symbol", 0);
    }
    
    if (selected_template == NULL)
        selected_template = solve_template_for_instantiation(entry, decl_context, &deduced_template_arguments, locus);

    if (selected_template != NULL)
    {
        if (is_incomplete_type(selected_template))
        {
            running_error("%s: instantiation of '%s' is not possible at this point since its most specialized template '%s' is incomplete\n", 
                    locus_to_str(locus),
                    print_type_str(get_user_defined_type(entry), decl_context),
                    print_type_str(selected_template, decl_context));
        }

        instantiate_specialized_template_class(selected_template, 
                get_user_defined_type(entry),
                deduced_template_arguments, locus);
    }
    else
    {
        running_error("%s: instantiation of '%s' is not possible at this point\n", 
                locus_to_str(locus), print_type_str(get_user_defined_type(entry), decl_context));
    }
}

char template_class_needs_to_be_instantiated(scope_entry_t* entry)
{
    return (class_type_is_incomplete_independent(entry->type_information) // it is independent incomplete
            && !entry->entity_specs.is_instantiated); // and we need to instantiated at this point
}

// Tries to instantiate if the current class is an independent incomplete class (and it is not an explicit specialization being defined)
void instantiate_template_class_if_needed(scope_entry_t* entry, decl_context_t decl_context, const locus_t* locus)
{
    if (template_class_needs_to_be_instantiated(entry))
    {
        instantiate_template_class(entry, decl_context, /* selected_template */ NULL, /* deduced_template_arguments */ NULL, locus);
    }
}

// Used in overload as it temptatively tries to instantiate classes lest they
// were a based or a derived class of another
void instantiate_template_class_if_possible(scope_entry_t* entry, decl_context_t decl_context, const locus_t* locus)
{
    if (!template_class_needs_to_be_instantiated(entry))
        return;

    // Try to see if it can actually be instantiated
    template_parameter_list_t* deduced_template_arguments = NULL;
    type_t* selected_template =
        solve_template_for_instantiation(entry, decl_context, &deduced_template_arguments, locus);

    // No specialized template is eligible for it, give up
    if (selected_template == NULL
        || is_incomplete_type(selected_template))
        return;

    instantiate_template_class(entry, decl_context, selected_template, deduced_template_arguments, locus);
}

static nodecl_t nodecl_instantiation_units;
typedef
struct instantiation_item_tag
{
    scope_entry_t* symbol;
    const locus_t* locus;
} instantiation_item_t;

static instantiation_item_t** symbols_to_instantiate;
static int num_symbols_to_instantiate;

void instantiation_init(void)
{
    nodecl_instantiation_units = nodecl_null();
    symbols_to_instantiate = NULL;
    num_symbols_to_instantiate = 0;
}

static void instantiate_every_symbol(scope_entry_t* entry,
        const locus_t* locus);

nodecl_t instantiation_instantiate_pending_functions(void)
{
    int tmp_num_symbols_to_instantiate = num_symbols_to_instantiate;
    instantiation_item_t** tmp_symbols_to_instantiate = symbols_to_instantiate;

    num_symbols_to_instantiate = 0;
    symbols_to_instantiate = NULL;

    int i;
    for (i = 0; i < tmp_num_symbols_to_instantiate; i++)
    {
        instantiate_every_symbol(
                tmp_symbols_to_instantiate[i]->symbol,
                tmp_symbols_to_instantiate[i]->locus);

        xfree(tmp_symbols_to_instantiate[i]);
    }
    xfree(tmp_symbols_to_instantiate);

    if (num_symbols_to_instantiate != 0)
    {
        return instantiation_instantiate_pending_functions();
    }
    else
    {
        return nodecl_instantiation_units;
    }
}

static char compare_instantiate_items(instantiation_item_t* current_item, instantiation_item_t* new_item)
{
    return current_item->symbol == new_item->symbol;
}

void instantiation_add_symbol_to_instantiate(scope_entry_t* entry,
        const locus_t* locus)
{
    instantiation_item_t* item = xcalloc(1, sizeof(*item));
    item->symbol = entry;
    item->locus = locus;

    int old_num = num_symbols_to_instantiate;

    P_LIST_ADD_ONCE_FUN(symbols_to_instantiate, 
            num_symbols_to_instantiate, 
            item, 
            compare_instantiate_items);

    // Crummy way to know if it was added
    if (old_num == num_symbols_to_instantiate)
    {
        xfree(item);
    }
}

static void instantiate_template_function(scope_entry_t* entry, const locus_t* locus UNUSED_PARAMETER)
{
    DEBUG_CODE()
    {
        fprintf(stderr, "INSTANTIATION: Instantiating function '%s' with type '%s' at '%s\n",
                entry->symbol_name,
                print_type_str(entry->type_information, entry->decl_context),
                locus_to_str(entry->locus));
    }

#if 0
    // Update to the instantiation point
    entry->file = filename;
    entry->line = line;

    type_t* template_specialized_type = entry->type_information;

    type_t* template_type = template_specialized_type_get_related_template_type(template_specialized_type);
    scope_entry_t* template_symbol = template_type_get_related_symbol(template_type);

    // The primary specialization is a named type, even if the named type is a function!
    type_t* primary_specialization_type = template_type_get_primary_type(template_symbol->type_information);
    scope_entry_t* primary_specialization_function = named_type_get_symbol(primary_specialization_type);
    // type_t* primary_specialization_function_type = primary_specialization_function->type_information;

    // nodecl_t orig_function_definition = primary_specialization_function->entity_specs.function_code;

    // ast_dump_graphviz(nodecl_get_ast(orig_function_definition), stderr);

    // 
    // // Remove dependent types
    // AST dupl_function_definition = ast_copy_for_instantiation(orig_function_definition);

    // // Why do we do this?
    // // Temporarily disable ambiguity testing
    // char old_test_status = get_test_expression_status();
    // set_test_expression_status(0);

    // decl_context_t instantiation_context = entry->decl_context;

    // nodecl_t nodecl_function_code = nodecl_null();
    // build_scope_function_definition(dupl_function_definition, 
    //         entry, 
    //         instantiation_context, 
    //         // This is not entirely true
    //         /* is_template */ 1,
    //         /* is_explicit_instantiation */ 1,
    //         &nodecl_function_code);

    // entry->entity_specs.definition_tree = dupl_function_definition;

    // nodecl_instantiation_units = nodecl_concat_lists(nodecl_instantiation_units,
    //         nodecl_function_code);

    // set_test_expression_status(old_test_status);

    DEBUG_CODE()
    {
        fprintf(stderr, "INSTANTIATION: ended instantation of function template '%s'\n",
                print_declarator(template_specialized_type));
    }
#endif
}

static scope_entry_t* being_instantiated_now[MCXX_MAX_TEMPLATE_NESTING_LEVELS];
static int num_being_instantiated_now = 0;

void instantiate_template_function_if_needed(scope_entry_t* entry, const locus_t* locus)
{
    DEBUG_CODE()
    {
        fprintf(stderr, "INSTANTIATION: Instantiation request of template function '%s' at '%s\n",
                print_decl_type_str(entry->type_information, entry->decl_context, 
                    get_qualified_symbol_name(entry, entry->decl_context)),
                locus_to_str(locus));
    }

    // Do nothing if we are checking for ambiguities as this may cause havoc
    if (checking_ambiguity())
    {
        DEBUG_CODE()
        {
            fprintf(stderr, "INSTANTIATION: Not instantiating since we are checking for ambiguities\n");
        }
        return;
    }

    ERROR_CONDITION(entry->kind != SK_FUNCTION, "Invalid symbol", 0);

    type_t* template_specialized_type = entry->type_information;

    if (!is_template_specialized_type(template_specialized_type)
            || !is_function_type(template_specialized_type))
    {
        internal_error("Symbol '%s' is not a template function eligible for instantiation", entry->symbol_name);
    }

    type_t* template_type = template_specialized_type_get_related_template_type(template_specialized_type);
    scope_entry_t* template_symbol = template_type_get_related_symbol(template_type);
    
    // The primary specialization is a named type, even if the named type is a function!
    type_t* primary_specialization_type = template_type_get_primary_type(template_symbol->type_information);
    scope_entry_t* primary_specialization_function = named_type_get_symbol(primary_specialization_type);
    // type_t* primary_specialization_function_type = primary_specialization_function->type_information;

    // If the primary specialization is not defined, no instantiation may happen
    if (!primary_specialization_function->defined)
    {
        DEBUG_CODE()
        {
            fprintf(stderr, "INSTANTIATION: Not instantiating since primary template has not been defined\n");
        }
        return;
    }

    if (entry->defined)
    {
        DEBUG_CODE()
        {
            fprintf(stderr, "INSTANTIATION: Function already instantiated\n");
        }
        return;
    }

    int i;
    for (i = 0; i < num_being_instantiated_now; i++)
    {
        if (being_instantiated_now[i] == entry)
        {
            DEBUG_CODE()
            {
                fprintf(stderr, "INSTANTIATION: This function is currently being instantiated\n");
            }
            return;
        }
    }

    ERROR_CONDITION(num_being_instantiated_now == MCXX_MAX_TEMPLATE_NESTING_LEVELS, 
            "Too many instantiation template levels %d", MCXX_MAX_TEMPLATE_NESTING_LEVELS);

    being_instantiated_now[num_being_instantiated_now] = entry;
    num_being_instantiated_now++;

    instantiate_template_function(entry, locus);

    num_being_instantiated_now--;
    being_instantiated_now[num_being_instantiated_now] = NULL;
}

UNUSED_PARAMETER static void instantiate_default_arguments_of_function(scope_entry_t* entry UNUSED_PARAMETER)
{
    internal_error("Not yet implemented", 0);
#if 0
    // decl_context_t instantiation_context = entry->decl_context;
    // Update the default arguments if any
    if (entry->entity_specs.default_argument_info != NULL)
    {
        int i;
        for (i = 0; i < entry->entity_specs.num_parameters; i++)
        {
            // default_argument_info_t* argument_info = entry->entity_specs.default_argument_info[i];

            if (argument_info != NULL
                    && !nodecl_is_null(argument_info->argument)
                    && nodecl_is_cxx_dependent_expr(argument_info->argument))
            {
                decl_context_t dummy;
                AST expr = nodecl_unwrap_cxx_dependent_expr(argument_info->argument, &dummy);
                expr = ast_copy_for_instantiation(expr);

                nodecl_t new_nodecl = nodecl_null();
                char c = check_expression(expr, instantiation_context, &new_nodecl);

                // Do not update anything on error
                if (!c)
                    continue;

                ERROR_CONDITION(nodecl_is_cxx_dependent_expr(new_nodecl), "Invalid dependent nodecl when updating default argument %d", i);

                argument_info->argument = new_nodecl;
                argument_info->context = instantiation_context;
            }
        }
    }
#endif
}

static void instantiate_emit_member_function(scope_entry_t* entry UNUSED_PARAMETER, const locus_t* locus UNUSED_PARAMETER)
{
    // Do nothing
#if 0
    ERROR_CONDITION(entry->kind != SK_FUNCTION, "Invalid function", 0);

    ERROR_CONDITION(!entry->entity_specs.is_non_emitted, "Invalid function is not yet nonemitted", 0);

    DEBUG_CODE()
    {
        fprintf(stderr, "INSTANTIATION: Instantiation request of non-emitted (non template) function '%s' at '%s:%d\n",
                print_decl_type_str(entry->type_information, entry->decl_context, 
                    get_qualified_symbol_name(entry, entry->decl_context)),
                locus);
    }

    instantiate_default_arguments_of_function(entry);

    scope_entry_t* emission_template = entry->entity_specs.emission_template;
    ERROR_CONDITION(emission_template == NULL, "Invalid emission template\n", 0);

    if (!emission_template->defined)
    {
        DEBUG_CODE()
        {
            fprintf(stderr, "INSTANTIATION: Function not defined, do not emit\n");
        }
        return;
    }
    entry->defined = 0;

    int i;
    for (i = 0; i < num_being_instantiated_now; i++)
    {
        if (being_instantiated_now[i] == entry)
        {
            DEBUG_CODE()
            {
                fprintf(stderr, "INSTANTIATION: This function is currently being emitted\n");
            }
            return;
        }
    }

    being_instantiated_now[num_being_instantiated_now] = entry;
    num_being_instantiated_now++;

    entry->entity_specs.is_non_emitted = 0;
    entry->entity_specs.emission_handler = NULL;
    entry->entity_specs.emission_template = NULL;

    // Remove dependent types
    AST dupl_function_definition = ast_copy_for_instantiation(emission_template->entity_specs.definition_tree);

    nodecl_t nodecl_function_code = nodecl_null();
    build_scope_function_definition(dupl_function_definition, 
            entry, 
            entry->decl_context, 
            // FIXME - This is not entirely true
            /* is_template */ 1,
            /* is_explicit_instantiation */ 1,
            &nodecl_function_code);

    entry->entity_specs.definition_tree = dupl_function_definition;

    nodecl_instantiation_units = nodecl_concat_lists(nodecl_instantiation_units,
            nodecl_function_code);

    num_being_instantiated_now--;
    being_instantiated_now[num_being_instantiated_now] = NULL;
#endif
}

AST instantiate_tree(AST orig_tree UNUSED_PARAMETER, 
        decl_context_t context_of_being_instantiated UNUSED_PARAMETER)
{
    internal_error("Not supported anymore", 0);
}

static void instantiate_every_symbol(scope_entry_t* entry,
        const locus_t* locus)
{
    if (entry != NULL
            && entry->kind == SK_FUNCTION)
    {
        if (is_template_specialized_type(entry->type_information))
        {
            instantiate_template_function_if_needed(entry, 
                    locus);
        }
        else if (entry->entity_specs.is_non_emitted)
        {
            (entry->entity_specs.emission_handler)(entry, locus);
        }
    }
}

