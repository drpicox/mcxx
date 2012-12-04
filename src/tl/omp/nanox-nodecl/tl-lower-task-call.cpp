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

#include "tl-lowering-visitor.hpp"
#include "tl-nanos.hpp"
#include "tl-source.hpp"
#include "tl-counters.hpp"
#include "tl-nodecl-utils.hpp"
#include "tl-datareference.hpp"
#include "tl-devices.hpp"
#include "fortran03-typeutils.h"
#include "cxx-diagnostic.h"
#include "cxx-cexpr.h"
#include "fortran03-scope.h"

#include "tl-lower-task-common.hpp"

namespace TL { namespace Nanox {

typedef std::map<TL::Symbol, Nodecl::NodeclBase> sym_to_argument_expr_t;
typedef std::map<TL::Symbol, TL::Symbol> param_sym_to_arg_sym_t;

static void fill_map_parameters_to_arguments(
        TL::Symbol function,
        Nodecl::List arguments,
        sym_to_argument_expr_t& param_to_arg_expr)
{
    int i = 0;
    Nodecl::List::iterator it = arguments.begin();

    // If the current function is a non-static function and It is member of a
    // class, the first argument of the arguments list represents the object of
    // this class. Skip it!
    if (IS_CXX_LANGUAGE
            && !function.is_static()
            && function.is_member())
    {
        it++;
    }

    for (; it != arguments.end(); it++, i++)
    {
        Nodecl::NodeclBase expression;
        TL::Symbol parameter_sym;
        if (it->is<Nodecl::FortranNamedPairSpec>())
        {
            // If this is a Fortran style argument use the symbol
            Nodecl::FortranNamedPairSpec named_pair(it->as<Nodecl::FortranNamedPairSpec>());

            param_to_arg_expr[named_pair.get_name().get_symbol()] = named_pair.get_argument();
        }
        else
        {
            // Get the i-th parameter of the function
            ERROR_CONDITION(((signed int)function.get_related_symbols().size() <= i), "Too many parameters", 0);
            TL::Symbol parameter = function.get_related_symbols()[i];
            param_to_arg_expr[parameter] = *it;
        }
    }
}

static int outline_data_item_get_parameter_position(const OutlineDataItem& outline_data_item)
{
    TL::Symbol sym = outline_data_item.get_symbol();
    return (sym.is_parameter() ? sym.get_parameter_position(): -1);
}

// When a type has an expression update it using the parameter we will use in the outline function
static Nodecl::NodeclBase rewrite_expression_in_outline(Nodecl::NodeclBase node, const param_sym_to_arg_sym_t& map)
{
    if (node.is_null())
        return node;

    TL::Symbol sym = node.get_symbol();
    if (sym.is_valid())
    {
        if (sym.is_saved_expression())
        {
            return rewrite_expression_in_outline(sym.get_value(), map);
        }

        param_sym_to_arg_sym_t::const_iterator it = map.find(sym);
        if (it != map.end())
        {
            TL::Symbol sym = it->second;
            Nodecl::NodeclBase result = Nodecl::Symbol::make(
                    sym,
                    sym.get_filename(),
                    sym.get_line());

            result.set_type(sym.get_type());

            return result;
        }
    }

    TL::ObjectList<Nodecl::NodeclBase> children = node.children();
    for (TL::ObjectList<Nodecl::NodeclBase>::iterator it = children.begin();
            it != children.end();
            it++)
    {
        *it = rewrite_expression_in_outline(*it, map);
    }

    node.rechild(children);

    return node;
}

// This function updates the type of the parameter using the types of the
// encapsulating function
static TL::Type rewrite_type_in_outline(TL::Type t, const param_sym_to_arg_sym_t& map)
{
    if (!t.is_valid())
        return t;

    if (t.is_lvalue_reference())
    {
        return rewrite_type_in_outline(t.references_to(), map).get_lvalue_reference_to();
    }
    else if (t.is_pointer())
    {
        return (rewrite_type_in_outline(t.points_to(), map)).get_pointer_to();
    }
    else if (t.is_array())
    {
        TL::Type element_type = rewrite_type_in_outline(t.array_element(), map);

        Nodecl::NodeclBase lower_bound, upper_bound;
        t.array_get_bounds(lower_bound, upper_bound);

        lower_bound = rewrite_expression_in_outline(lower_bound.shallow_copy(), map);
        upper_bound = rewrite_expression_in_outline(upper_bound.shallow_copy(), map);

        if (!t.array_is_region())
        {
            return element_type.get_array_to(lower_bound, upper_bound,
                    CURRENT_COMPILED_FILE->global_decl_context);
        }
        else
        {
            Nodecl::NodeclBase region_lower_bound, region_upper_bound;
            t.array_get_region_bounds(region_lower_bound, region_upper_bound);

            region_lower_bound = rewrite_expression_in_outline(region_lower_bound.shallow_copy(), map);
            region_upper_bound = rewrite_expression_in_outline(region_upper_bound.shallow_copy(), map);

            return element_type.get_array_to_with_region(
                    lower_bound, upper_bound,
                    region_lower_bound, region_upper_bound,
                    CURRENT_COMPILED_FILE->global_decl_context);
        }
    }
    else
    {
        // Best effort
        return t;
    }
}


// ************************************************************************************
// ************************************************************************************
// ************************************************************************************
// C/C++
// ************************************************************************************
// ************************************************************************************
// ************************************************************************************

static TL::Type rewrite_dependency_type_c(TL::Type t, const param_sym_to_arg_sym_t& map);
static Nodecl::NodeclBase rewrite_expression_in_dependency_c(Nodecl::NodeclBase node, const param_sym_to_arg_sym_t& map)
{
    if (node.is_null())
        return node;

    TL::Symbol sym = node.get_symbol();
    if (sym.is_valid())
    {
        if (sym.is_saved_expression())
        {
            return rewrite_expression_in_dependency_c(sym.get_value(), map);
        }

        param_sym_to_arg_sym_t::const_iterator it = map.find(sym);
        if (it != map.end())
        {
            Nodecl::Symbol sym_ref = Nodecl::Symbol::make(it->second);

            TL::Type t = it->second.get_type();
            if (!t.is_any_reference())
                t = t.get_lvalue_reference_to();

            sym_ref.set_type(t);

            return sym_ref;
        }
    }

    TL::ObjectList<Nodecl::NodeclBase> children = node.children();
    for (TL::ObjectList<Nodecl::NodeclBase>::iterator it = children.begin();
            it != children.end();
            it++)
    {
        *it = rewrite_expression_in_dependency_c(*it, map);
    }

    node.rechild(children);

    // Update the types too
    node.set_type(rewrite_dependency_type_c(node.get_type(), map));

    return node;
}

static TL::Type rewrite_dependency_type_c(TL::Type t, const param_sym_to_arg_sym_t& map)
{
    if (!t.is_valid())
        return t;

    if (t.is_lvalue_reference())
    {
        return rewrite_dependency_type_c(t.references_to(), map).get_lvalue_reference_to();
    }
    else if (t.is_pointer())
    {
        return (rewrite_dependency_type_c(t.points_to(), map)).get_pointer_to();
    }
    else if (t.is_array())
    {
        TL::Type element_type = rewrite_dependency_type_c(t.array_element(), map);

        Nodecl::NodeclBase lower_bound, upper_bound;
        t.array_get_bounds(lower_bound, upper_bound);

        lower_bound = rewrite_expression_in_dependency_c(lower_bound.shallow_copy(), map);
        upper_bound = rewrite_expression_in_dependency_c(upper_bound.shallow_copy(), map);

        if (!t.array_is_region())
        {
            return element_type.get_array_to(lower_bound, upper_bound,
                    CURRENT_COMPILED_FILE->global_decl_context);
        }
        else
        {
            Nodecl::NodeclBase region_lower_bound, region_upper_bound;
            t.array_get_region_bounds(region_lower_bound, region_upper_bound);

            region_lower_bound = rewrite_expression_in_dependency_c(region_lower_bound.shallow_copy(), map);
            region_upper_bound = rewrite_expression_in_dependency_c(region_upper_bound.shallow_copy(), map);

            return element_type.get_array_to_with_region(
                    lower_bound, upper_bound,
                    region_lower_bound, region_upper_bound,
                    CURRENT_COMPILED_FILE->global_decl_context);
        }
    }
    else
    {
        // Best effort
        return t;
    }
}

static Nodecl::NodeclBase rewrite_single_dependency_c(Nodecl::NodeclBase node, const param_sym_to_arg_sym_t& map)
{
    if (node.is_null())
        return node;

    node.set_type(rewrite_dependency_type_c(node.get_type(), map));

    TL::ObjectList<Nodecl::NodeclBase> children = node.children();
    for (TL::ObjectList<Nodecl::NodeclBase>::iterator it = children.begin();
            it != children.end();
            it++)
    {
        *it = rewrite_single_dependency_c(*it, map);
    }

    node.rechild(children);

    // Update indexes where is due
    if (node.is<Nodecl::ArraySubscript>())
    {
        Nodecl::ArraySubscript arr_subscr = node.as<Nodecl::ArraySubscript>();
        arr_subscr.set_subscripts(
                rewrite_expression_in_dependency_c(arr_subscr.get_subscripts(), map));
    }
    else if (node.is<Nodecl::Shaping>())
    {
        Nodecl::Shaping shaping = node.as<Nodecl::Shaping>();
        shaping.set_shape(
                rewrite_expression_in_dependency_c(shaping.get_shape(), map));
    }

    return node;
}

static TL::ObjectList<OutlineDataItem::DependencyItem> rewrite_dependences_c(
        const TL::ObjectList<OutlineDataItem::DependencyItem>& deps,
        const param_sym_to_arg_sym_t& map)
{
    TL::ObjectList<OutlineDataItem::DependencyItem> result;
    for (TL::ObjectList<OutlineDataItem::DependencyItem>::const_iterator it = deps.begin();
            it != deps.end();
            it++)
    {
        Nodecl::NodeclBase copy = it->expression.shallow_copy();
        Nodecl::NodeclBase rewritten = rewrite_expression_in_dependency_c(copy, map);

        result.append( OutlineDataItem::DependencyItem(rewritten, it->directionality) );
    }

    return result;
}


static TL::ObjectList<OutlineDataItem::CopyItem> rewrite_copies_c(
        const TL::ObjectList<OutlineDataItem::CopyItem>& deps,
        const param_sym_to_arg_sym_t& param_sym_to_arg_sym)
{
    TL::ObjectList<OutlineDataItem::CopyItem> result;
    for (TL::ObjectList<OutlineDataItem::CopyItem>::const_iterator it = deps.begin();
            it != deps.end();
            it++)
    {
        Nodecl::NodeclBase copy = it->expression.shallow_copy();
        Nodecl::NodeclBase rewritten = rewrite_expression_in_dependency_c(copy, param_sym_to_arg_sym);

        result.append( OutlineDataItem::CopyItem(
                    rewritten,
                    it->directionality) );
    }

    return result;
}

static void copy_outline_data_item_c(
        OutlineDataItem& dest_info,
        const OutlineDataItem& source_info,
        const param_sym_to_arg_sym_t& param_sym_to_arg_sym)
{
    // We want the same field name
    dest_info.set_field_name(source_info.get_field_name());

    // Copy dependence directionality
    dest_info.get_dependences() = rewrite_dependences_c(source_info.get_dependences(), param_sym_to_arg_sym);

    // Copy copy directionality
    dest_info.get_copies() = rewrite_copies_c(source_info.get_copies(), param_sym_to_arg_sym);
}

void LoweringVisitor::visit_task_call_c(const Nodecl::OpenMP::TaskCall& construct)
{
    Nodecl::FunctionCall function_call = construct.get_call().as<Nodecl::FunctionCall>();
    ERROR_CONDITION(!function_call.get_called().is<Nodecl::Symbol>(), "Invalid ASYNC CALL!", 0);

    TL::Symbol called_sym = function_call.get_called().get_symbol();

    std::cerr << construct.get_locus() << ": note: call to task function '" << called_sym.get_qualified_name() << "'" << std::endl;

    // Get parameters outline info
    Nodecl::NodeclBase parameters_environment = construct.get_environment();
    OutlineInfo parameters_outline_info(parameters_environment);

    TaskEnvironmentVisitor task_environment;
    task_environment.walk(parameters_environment);

    // Fill arguments outline info using parameters
    OutlineInfo arguments_outline_info;

    //Copy target info table from parameter_outline_info to arguments_outline_info
    OutlineInfo::implementation_table_t implementation_table = parameters_outline_info.get_implementation_table();
    for (OutlineInfo::implementation_table_t::iterator it = implementation_table.begin();
            it != implementation_table.end();
            ++it)
    {
        ObjectList<std::string> devices=it->second.get_device_names();
        for (ObjectList<std::string>::iterator it2 = devices.begin();
                it2 != devices.end();
                ++it2)
        {
                arguments_outline_info.add_implementation(*it2, it->first);
                arguments_outline_info.append_to_ndrange(it->first,it->second.get_ndrange());
                arguments_outline_info.append_to_onto(it->first,it->second.get_onto());
    }

    // This map associates every parameter symbol with its argument expression
    sym_to_argument_expr_t param_to_arg_expr;
    param_sym_to_arg_sym_t param_sym_to_arg_sym;
    Nodecl::List arguments = function_call.get_arguments().as<Nodecl::List>();
    fill_map_parameters_to_arguments(called_sym, arguments, param_to_arg_expr);

    Scope sc = construct.retrieve_context();
    TL::ObjectList<TL::Symbol> new_arguments;

    Source initializations_src;

    // If the current function is a non-static function and It is member of a
    // class, the first argument of the arguments list represents the object of
    // this class
    if (IS_CXX_LANGUAGE
            && !called_sym.is_static()
            && called_sym.is_member())
    {
        Nodecl::NodeclBase class_object = *(arguments.begin());
        TL::Symbol this_symbol = called_sym.get_scope().get_symbol_from_name("this");
        ERROR_CONDITION(!this_symbol.is_valid(), "Invalid symbol", 0);

        Counter& arg_counter = CounterManager::get_counter("nanos++-outline-arguments");
        std::stringstream ss;
        ss << "mcc_arg_" << (int)arg_counter;
        TL::Symbol new_symbol = sc.new_symbol(ss.str());
        arg_counter++;

        new_symbol.get_internal_symbol()->kind = SK_VARIABLE;
        new_symbol.get_internal_symbol()->type_information = this_symbol.get_type().get_internal_type();
        new_symbol.get_internal_symbol()->entity_specs.is_user_declared = 1;

        if (IS_CXX_LANGUAGE)
        {
            // We need to declare explicitly this object in C++
            initializations_src
                << as_statement(Nodecl::CxxDef::make(Nodecl::NodeclBase::null(), new_symbol))
                ;
        }

        Nodecl::NodeclBase sym_ref = Nodecl::Symbol::make(this_symbol);
        sym_ref.set_type(this_symbol.get_type());

            // Direct initialization is enough
        new_symbol.get_internal_symbol()->value = sym_ref.get_internal_nodecl();

        new_arguments.append(new_symbol);

        OutlineDataItem& argument_outline_data_item = arguments_outline_info.get_entity_for_symbol(new_symbol);
        // This is a special kind of shared
        argument_outline_data_item.set_sharing(OutlineDataItem::SHARING_CAPTURE_ADDRESS);

        argument_outline_data_item.set_base_address_expression(
                Nodecl::Reference::make(
                    class_object,
                    new_symbol.get_type(),
                    function_call.get_filename(),
                    function_call.get_line()));
    }

    OutlineInfoRegisterEntities outline_register_entities(arguments_outline_info, sc);

    TL::ObjectList<OutlineDataItem*> data_items = parameters_outline_info.get_data_items();
    //Map so the device provider can translate between parameters and arguments
    Nodecl::Utils::SimpleSymbolMap param_to_args_map;

    // First register all symbols
    for (sym_to_argument_expr_t::iterator it = param_to_arg_expr.begin();
            it != param_to_arg_expr.end();
            it++)
    {
        // We search by parameter position here
        ObjectList<OutlineDataItem*> found = data_items.find(
                lift_pointer(functor(outline_data_item_get_parameter_position)),
                it->first.get_parameter_position_in(called_sym));

        if (found.empty())
        {
            internal_error("%s: error: cannot find parameter '%s' in OutlineInfo",
                    arguments.get_locus().c_str(),
                    it->first.get_name().c_str());
        }

        Counter& arg_counter = CounterManager::get_counter("nanos++-outline-arguments");
        // Create a new variable holding the value of the argument
        std::stringstream ss;
        ss << "mcc_arg_" << (int)arg_counter;
        TL::Symbol new_symbol = sc.new_symbol(ss.str());
        arg_counter++;

        // FIXME - Wrap this sort of things
        new_symbol.get_internal_symbol()->kind = SK_VARIABLE;
        new_symbol.get_internal_symbol()->type_information = it->first.get_type().get_internal_type();
        new_symbol.get_internal_symbol()->entity_specs.is_user_declared = 1;
        param_sym_to_arg_sym[it->first] = new_symbol;

        if (IS_CXX_LANGUAGE)
        {
            // We need to declare explicitly this object in C++ and initialize it properly
            initializations_src
                << as_statement(Nodecl::CxxDef::make(Nodecl::NodeclBase::null(), new_symbol))
                ;
        }
        else if (IS_C_LANGUAGE)
        {
            initializations_src
                << as_statement(Nodecl::ObjectInit::make(new_symbol))
                ;
        }

        if (it->first.get_type().is_class() && IS_CXX_LANGUAGE)
        {
            internal_error("Copy-construction of a class type is not yet implemented", 0);
        }
        else
        {
            // Direct initialization is enough
            new_symbol.get_internal_symbol()->value = it->second.shallow_copy().get_internal_nodecl();
        }

        new_arguments.append(new_symbol);

        Nodecl::Symbol sym_ref = Nodecl::Symbol::make(new_symbol);
        TL::Type t = new_symbol.get_type();
        if (!t.is_any_reference())
            t = t.get_lvalue_reference_to();
        sym_ref.set_type(t);

        outline_register_entities.add_capture_with_value(new_symbol, sym_ref);
        param_to_args_map.add_map(it->first,new_symbol);
    }
    
    //Add this map to target information, so DeviceProviders can translate 
    //Clauses in case it's needed, now we only add the same for every task, but in a future?
    OutlineInfo::implementation_table_t args_implementation_table = arguments_outline_info.get_implementation_table();    
    for (OutlineInfo::implementation_table_t::iterator it = args_implementation_table.begin();
            it != args_implementation_table.end();
            ++it) {
       arguments_outline_info.set_param_arg_map(param_to_args_map,it->first);
    }
    

    // Now update them (we don't do this in the previous traversal because we allow forward references)
    // like in
    //
    // #pragma omp task inout([n]a)
    // void f(int *a, int n);
    //
    // We will first see 'a' and then 'n' but the dependence on 'a' uses 'n', so we need the map fully populated
    for (sym_to_argument_expr_t::iterator it = param_to_arg_expr.begin();
            it != param_to_arg_expr.end();
            it++)
    {
        ERROR_CONDITION(param_sym_to_arg_sym.find(it->first) == param_sym_to_arg_sym.end(), "Symbol not found", 0);

        TL::Symbol &new_symbol = param_sym_to_arg_sym[it->first];

        OutlineDataItem& parameter_outline_data_item = parameters_outline_info.get_entity_for_symbol(it->first);

        OutlineDataItem& argument_outline_data_item = arguments_outline_info.get_entity_for_symbol(new_symbol);
        copy_outline_data_item_c(argument_outline_data_item, parameter_outline_data_item, param_sym_to_arg_sym);
    }

    // Prepend the assignments
    Nodecl::NodeclBase enclosing_expression_stmt = construct.get_parent();
    ERROR_CONDITION(!enclosing_expression_stmt.is<Nodecl::ExpressionStatement>(), "Invalid tree", 0);

    if (!initializations_src.empty())
    {
        Nodecl::NodeclBase assignments_tree = initializations_src.parse_statement(sc);
        Nodecl::Utils::prepend_items_before(enclosing_expression_stmt, assignments_tree);
    }

    // Now fix again arguments of the outline
    data_items = arguments_outline_info.get_data_items();
    for (TL::ObjectList<OutlineDataItem*>::iterator it = data_items.begin();
            it != data_items.end();
            it++)
    {
        TL::Symbol sym = (*it)->get_symbol();
        if (!sym.is_valid())
            continue;

        TL::Type updated_type = rewrite_type_in_outline((*it)->get_in_outline_type(),
                param_sym_to_arg_sym);
        (*it)->set_in_outline_type(updated_type);
    }

    TL::Symbol alternate_name;
    if (!function_call.get_alternate_name().is_null())
    {
        alternate_name = function_call.get_alternate_name().get_symbol();
    }

    // Craft a new function call with the new mcc_arg_X symbols
    TL::ObjectList<TL::Symbol>::iterator args_it = new_arguments.begin();
    TL::ObjectList<Nodecl::NodeclBase> arg_list;

    // If the current function is a non-static function and It is member of a
    // class, the first argument of the arguments list represents the object of
    // this class
    if (IS_CXX_LANGUAGE
            && !called_sym.is_static()
            && called_sym.is_member())
    {
        // The symbol which represents the object 'this' must be dereferenced
        Nodecl::NodeclBase nodecl_arg = Nodecl::Dereference::make(
                Nodecl::Symbol::make(*args_it,
                    function_call.get_filename(),
                    function_call.get_line()),
                args_it->get_type().points_to(),
                function_call.get_filename(),
                function_call.get_line());

        arg_list.append(nodecl_arg);
        args_it++;
    }

    for (sym_to_argument_expr_t::iterator params_it = param_to_arg_expr.begin();
            params_it != param_to_arg_expr.end();
            params_it++, args_it++)
    {
        Nodecl::NodeclBase nodecl_arg;

        nodecl_arg = Nodecl::Symbol::make(*args_it,
                function_call.get_filename(),
                function_call.get_line());
        nodecl_arg.set_type(args_it->get_type());

        arg_list.append(nodecl_arg);
    }

    Nodecl::List nodecl_arg_list = Nodecl::List::make(arg_list);

    Nodecl::NodeclBase called = function_call.get_called().shallow_copy();
    Nodecl::NodeclBase function_form = nodecl_null();
    Symbol called_symbol = called.get_symbol();

    if (IS_CXX_LANGUAGE
            && !called_symbol.is_valid()
            && called_symbol.get_type().is_template_specialized_type())
    {
        // FIXME - Could this ever happen?
        function_form =
            Nodecl::CxxFunctionFormTemplateId::make(
                    function_call.get_filename(),
                    function_call.get_line());

        TemplateParameters template_args =
            called.get_template_parameters();
        function_form.set_template_parameters(template_args);
    }

    Nodecl::NodeclBase expr_statement =
        Nodecl::ExpressionStatement::make(
                Nodecl::FunctionCall::make(
                    called,
                    nodecl_arg_list,
                    function_call.get_alternate_name().shallow_copy(),
                    function_form,
                    Type::get_void_type(),
                    function_call.get_filename(),
                    function_call.get_line()),
                function_call.get_filename(),
                function_call.get_line());

    TL::ObjectList<Nodecl::NodeclBase> list_stmt;
    list_stmt.append(expr_statement);

    Nodecl::NodeclBase statements = Nodecl::List::make(list_stmt);

    construct.as<Nodecl::OpenMP::Task>().set_statements(statements);

    Symbol function_symbol = Nodecl::Utils::get_enclosing_function(construct);

    emit_async_common(
            construct,
            function_symbol,
            called_symbol,
            statements,
            task_environment.priority,
            task_environment.is_untied,
            arguments_outline_info,
            &parameters_outline_info);
    }
}


// ************************************************************************************
// ************************************************************************************
// ************************************************************************************
// Fortran
// ************************************************************************************
// ************************************************************************************
// ************************************************************************************
//
//
// In Fortran, we use a very different approach to that of C/C++. Instead of keeping the arguments
// values (which is not always possible due to Fortran limitations) we will create a new function
// with the same parameters as the original task. This new function will have a simple body
// with a task that calls the original function

static void handle_save_expressions(decl_context_t function_context,
        TL::Type t,
        // Out
        Nodecl::Utils::SimpleSymbolMap& symbol_map,
        TL::ObjectList<TL::Symbol> &save_expressions)
{
    if (t.is_any_reference())
    {
        handle_save_expressions(function_context, t.references_to(), symbol_map, save_expressions);
    }
    else if (t.is_array())
    {
        Nodecl::NodeclBase lower, upper;
        t.array_get_bounds(lower, upper);

        struct params
        {
            Nodecl::NodeclBase& tree;
            params(Nodecl::NodeclBase& t) : tree(t) { }
        } args[2] = { lower, upper };

        for (int i = 0; i < 2; i++)
        {
            Nodecl::NodeclBase& tree(args[i].tree);

            if (!tree.is_null()
                    && tree.is<Nodecl::Symbol>()
                    && tree.get_symbol().is_saved_expression())
            {
                scope_entry_t* orig_save_expression = tree.get_symbol().get_internal_symbol();

                scope_entry_t* new_save_expression
                    = new_symbol(function_context,
                            function_context.current_scope,
                            orig_save_expression->symbol_name);
                new_save_expression->kind = SK_VARIABLE;
                new_save_expression->type_information = orig_save_expression->type_information;

                new_save_expression->entity_specs.is_saved_expression = 1;

                new_save_expression->value = nodecl_deep_copy(orig_save_expression->value,
                        function_context,
                        symbol_map.get_symbol_map());

                symbol_map.add_map(orig_save_expression, new_save_expression);

                save_expressions.append(new_save_expression);
            }
        }

        handle_save_expressions(function_context, t.array_element(), symbol_map, save_expressions);
    }
}


static TL::Symbol new_function_symbol_adapter(
        TL::Symbol current_function,
        TL::Symbol called_function,
        const std::string& function_name,

        // out
        Nodecl::Utils::SimpleSymbolMap &symbol_map,
        TL::ObjectList<TL::Symbol> &save_expressions)
{
    Scope sc = current_function.get_scope();

    decl_context_t decl_context = sc.get_decl_context();
    decl_context_t function_context;

    function_context = new_program_unit_context(decl_context);

    TL::ObjectList<TL::Symbol> parameters_of_new_function;

    TL::ObjectList<TL::Symbol> parameters_of_called_function = called_function.get_related_symbols();

    // Create symbols
    for (TL::ObjectList<TL::Symbol>::iterator it = parameters_of_called_function.begin();
            it != parameters_of_called_function.end();
            it++)
    {
        scope_entry_t* new_parameter_symbol
            = new_symbol(function_context, function_context.current_scope, uniquestr(it->get_name().c_str()));
        new_parameter_symbol->kind = SK_VARIABLE;
        new_parameter_symbol->type_information = it->get_type().get_internal_type();

        parameters_of_new_function.append(new_parameter_symbol);
        symbol_map.add_map(*it, new_parameter_symbol);
    }

    // Update types of types
    for (TL::ObjectList<TL::Symbol>::iterator it = parameters_of_new_function.begin();
            it != parameters_of_new_function.end();
            it++)
    {
        // This will register the extra symbols required by VLAs
        handle_save_expressions(function_context, it->get_type(), symbol_map, save_expressions);

        it->get_internal_symbol()->type_information =
            type_deep_copy(it->get_internal_symbol()->type_information,
                    function_context,

                    symbol_map.get_symbol_map());
    }

    // Now everything is set to register the function
    scope_entry_t* new_function_sym = new_symbol(decl_context, decl_context.current_scope, function_name.c_str());
    new_function_sym->entity_specs.is_user_declared = 1;

    new_function_sym->kind = SK_FUNCTION;
    new_function_sym->file = "";
    new_function_sym->line = 0;

    function_context.function_scope->related_entry = new_function_sym;
    function_context.block_scope->related_entry = new_function_sym;

    new_function_sym->related_decl_context = function_context;

    parameter_info_t* p_types = new parameter_info_t[parameters_of_new_function.size() + 1];

    parameter_info_t* it_ptypes = &(p_types[0]);
    for (ObjectList<TL::Symbol>::iterator it = parameters_of_new_function.begin();
            it != parameters_of_new_function.end();
            it++, it_ptypes++)
    {
        scope_entry_t* param = it->get_internal_symbol();

        symbol_set_as_parameter_of_function(param, new_function_sym, new_function_sym->entity_specs.num_related_symbols);

        P_LIST_ADD(new_function_sym->entity_specs.related_symbols,
                new_function_sym->entity_specs.num_related_symbols,
                param);

        it_ptypes->is_ellipsis = 0;
        it_ptypes->nonadjusted_type_info = NULL;
        it_ptypes->type_info = get_user_defined_type(param);
    }

    type_t *function_type = get_new_function_type(
            get_void_type(),
            p_types,
            parameters_of_new_function.size());

    new_function_sym->type_information = function_type;

    // Add the called symbol in the scope of the function
    insert_entry(function_context.current_scope, called_function.get_internal_symbol());

    // Propagate USE information
    new_function_sym->entity_specs.used_modules = current_function.get_internal_symbol()->entity_specs.used_modules;

    // If the current function is a module, make this new function a sibling of it
    if (current_function.is_in_module()
            && current_function.is_module_procedure())
    {
        new_function_sym->entity_specs.in_module = current_function.in_module().get_internal_symbol();
        new_function_sym->entity_specs.access = AS_PRIVATE;
        new_function_sym->entity_specs.is_module_procedure = 1;

        P_LIST_ADD(new_function_sym->entity_specs.in_module->entity_specs.related_symbols,
                new_function_sym->entity_specs.in_module->entity_specs.num_related_symbols,
                new_function_sym);
    }

    delete[] p_types;

    return new_function_sym;
}

static Nodecl::NodeclBase fill_adapter_function(
        TL::Symbol adapter_function,
        TL::Symbol called_function,
        Nodecl::Utils::SymbolMap &symbol_map,
        Nodecl::NodeclBase original_environment,
        TL::ObjectList<TL::Symbol> &save_expressions,

        // out
        Nodecl::NodeclBase& task_construct,
        Nodecl::NodeclBase& statements_of_task_seq,
        Nodecl::NodeclBase& new_environment
        )
{
    TL::ObjectList<Nodecl::NodeclBase> statements_of_function;

    TL::ObjectList<Nodecl::NodeclBase> statements_of_task_list;
    // Create one object init per save expression
    for (TL::ObjectList<TL::Symbol>::iterator it = save_expressions.begin();
            it != save_expressions.end();
            it++)
    {
        statements_of_function.append(
                Nodecl::ObjectInit::make(*it));
    }

    // Create a reference to the function
    Nodecl::NodeclBase function_ref = Nodecl::Symbol::make(called_function);
    function_ref.set_type(called_function.get_type().get_lvalue_reference_to());

    // Create the arguments of the call
    TL::ObjectList<Nodecl::NodeclBase> argument_list;
    TL::ObjectList<TL::Symbol> parameters_of_adapter_function = adapter_function.get_related_symbols();
    for (TL::ObjectList<TL::Symbol>::iterator it = parameters_of_adapter_function.begin();
            it != parameters_of_adapter_function.end();
            it++)
    {
        Nodecl::NodeclBase sym_ref = Nodecl::Symbol::make(*it);
        TL::Type t = it->get_type();
        if (!t.is_lvalue_reference())
            t = t.get_lvalue_reference_to();

        argument_list.append(sym_ref);
    }

    Nodecl::NodeclBase argument_seq = Nodecl::List::make(argument_list);

    // Create the call
    Nodecl::NodeclBase call_to_adapter =
        Nodecl::ExpressionStatement::make(
                Nodecl::FunctionCall::make(function_ref,
                    argument_seq,
                    /* alternate name */ Nodecl::NodeclBase::null(),
                    /* function form */ Nodecl::NodeclBase::null(),
                TL::Type::get_void_type()));

    statements_of_task_list.append(call_to_adapter);

    statements_of_task_seq = Nodecl::List::make(statements_of_task_list);

    // Update the environment of pragma omp task
    new_environment = Nodecl::Utils::deep_copy(original_environment,
            TL::Scope(CURRENT_COMPILED_FILE->global_decl_context),
            symbol_map);

    // Create the #pragma omp task
    task_construct = Nodecl::OpenMP::Task::make(new_environment, statements_of_task_seq);

    statements_of_function.append(task_construct);

    Nodecl::NodeclBase in_context = Nodecl::List::make(statements_of_function);

    Nodecl::NodeclBase context = Nodecl::Context::make(in_context, adapter_function.get_related_scope());

    Nodecl::NodeclBase function_code =
        Nodecl::FunctionCode::make(context,
                /* initializers */ Nodecl::NodeclBase::null(),
                /* internal_functions */ Nodecl::NodeclBase::null(),
                adapter_function);

    return function_code;
}

void LoweringVisitor::visit_task_call_fortran(const Nodecl::OpenMP::TaskCall& construct)
{
    Nodecl::FunctionCall function_call = construct.get_call().as<Nodecl::FunctionCall>();
    ERROR_CONDITION(!function_call.get_called().is<Nodecl::Symbol>(), "Invalid ASYNC CALL!", 0);

    TL::Symbol called_task_function = function_call.get_called().get_symbol();

    TL::Symbol current_function = Nodecl::Utils::get_enclosing_function(construct);

    if (current_function.is_nested_function())
    {
        error_printf("%s: error: call to task function '%s' from an internal subprogram is not supported\n",
                construct.get_locus().c_str(),
                called_task_function.get_qualified_name().c_str());
        return;
    }

    std::cerr << construct.get_locus()
        << ": note: call to task function '" << called_task_function.get_qualified_name() << "'" << std::endl;

    // Get parameters outline info
    Nodecl::NodeclBase parameters_environment = construct.get_environment();
    OutlineInfo parameters_outline_info(parameters_environment);

    // Fill arguments outline info using parameters
    OutlineInfo arguments_outline_info;

    Counter& adapter_counter = CounterManager::get_counter("nanos++-task-adapter");
    std::stringstream ss;
    ss << called_task_function.get_name() << "_adapter_" << (int)adapter_counter;
    adapter_counter++;

    TL::ObjectList<Symbol> save_expressions;

    Nodecl::Utils::SimpleSymbolMap symbol_map;
    TL::Symbol adapter_function = new_function_symbol_adapter(
            current_function,
            called_task_function,
            ss.str(),

            // out
            symbol_map,
            save_expressions);

    Nodecl::NodeclBase new_task_construct, new_statements, new_environment;
    Nodecl::NodeclBase adapter_function_code = fill_adapter_function(adapter_function,
            called_task_function,
            symbol_map,
            parameters_environment,
            save_expressions,

            // Out
            new_task_construct,
            new_statements,
            new_environment);

    Nodecl::Utils::prepend_to_enclosing_top_level_location(construct, adapter_function_code);

    OutlineInfo new_outline_info(new_environment);

    TaskEnvironmentVisitor task_environment;
    task_environment.walk(new_environment);

    // Symbol current_function = Nodecl::Utils::get_enclosing_function(construct);

    emit_async_common(
            new_task_construct,
            adapter_function,
            called_task_function, // Which one we want now?
            new_statements,
            task_environment.priority,
            task_environment.is_untied,
            new_outline_info,
            NULL);

    // Now call the adapter function instead of the original
    Nodecl::NodeclBase adapter_sym_ref = Nodecl::Symbol::make(adapter_function);
    adapter_sym_ref.set_type(adapter_function.get_type().get_lvalue_reference_to());

    // Add a map from the original called task to the adapter function
    symbol_map.add_map(called_task_function, adapter_function);

    // And replace everything with a call to the adapter function
    construct.replace(
            Nodecl::Utils::deep_copy(function_call, construct, symbol_map)
            );
}

void LoweringVisitor::visit(const Nodecl::OpenMP::TaskCall& construct)
{
    if (IS_C_LANGUAGE
            || IS_CXX_LANGUAGE)
    {
        visit_task_call_c(construct);
    }
    else if (IS_FORTRAN_LANGUAGE)
    {
        visit_task_call_fortran(construct);
    }
}

} }