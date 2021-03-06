/*--------------------------------------------------------------------
( C) Copyright 2006-2013 Barcelona Supercomputing Center             *
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

#include <iostream>
#include <fstream>

#include "tl-analysis-utils.hpp"
#include "config.h"
#include "tl-node.hpp"
#include "tl-pcfg-visitor.hpp"      // For IPA analysis
#include "tl-rename-visitor.hpp"
#include "tl-use-def.hpp"

namespace TL {
namespace Analysis {

    // **************************************************************************************************** //
    // **************************** Class implementing use-definition analysis **************************** //

    UseDef::UseDef( ExtensibleGraph* graph )
            : _graph( graph )
    {}

    void UseDef::compute_usage( std::set<TL::Symbol> visited_functions,
                                ObjectList<Utils::ExtendedSymbolUsage> visited_global_vars,
                                bool ipa, Utils::nodecl_set ipa_arguments )
    {
        Node* graph = _graph->get_graph( );
        visited_global_vars.insert( _graph->get_global_variables( ) );
        compute_usage_rec( graph, visited_functions, visited_global_vars, ipa, ipa_arguments );
        ExtensibleGraph::clear_visits( graph );
    }

    void UseDef::compute_usage_rec( Node* current, std::set<TL::Symbol>& visited_functions,
                                    ObjectList<Utils::ExtendedSymbolUsage>& visited_global_vars,
                                    bool ipa, Utils::nodecl_set ipa_arguments )
    {
        if( !current->is_visited( ) )
        {
            current->set_visited( true );

            if( current->is_exit_node( ) )
                return;

            if( current->is_graph_node( )
                && !current->is_asm_def_node( ) && !current->is_asm_op_node( ) )
            {
                // Use-def info is computed from inner nodes to outer nodes
                compute_usage_rec( current->get_graph_entry_node( ),
                                   visited_functions, visited_global_vars,
                                   ipa, ipa_arguments );

                if( current->is_split_statement( ) )
                {   // Propagate the info from the subpart of the split node to the whole node
                    Node* split_node = current->get_graph_exit_node( )->get_parents( )[0];
                    Node* split_subpart = split_node->get_parents( )[0];

                    split_node->set_ue_var( split_subpart->get_ue_vars( ) );
                    split_node->set_killed_var( split_subpart->get_killed_vars( ) );
                    split_node->set_undefined_behaviour_var( split_subpart->get_undefined_behaviour_vars( ) );
                }

                // Propagate usage info from inner to outer nodes
                ExtensibleGraph::clear_visits( current );
                set_graph_node_use_def( current );
            }
            else
            {
                // Treat statements in the current node
                ObjectList<Nodecl::NodeclBase> stmts = current->get_statements( );
                for( ObjectList<Nodecl::NodeclBase>::iterator it = stmts.begin( ); it != stmts.end( ); ++it )
                {
                    UsageVisitor uv( current, visited_functions, visited_global_vars,
                                     ipa, _graph->get_scope( ), ipa_arguments );
                    uv.compute_statement_usage( *it );

                    std::set<TL::Symbol> visited_functions_ = uv.get_visited_functions( );
                    visited_functions.insert( visited_functions_.begin( ), visited_functions_.end( ) );
                    visited_global_vars.insert( uv.get_visited_global_variables( ) );
                }
            }

            // Compute usage form children
            ObjectList<Node*> children = current->get_children( );
            for( ObjectList<Node*>::iterator it = children.begin( ); it != children.end( ); ++it )
            {
                compute_usage_rec( *it,
                                   visited_functions, visited_global_vars,
                                   ipa, ipa_arguments );
            }
        }
    }

    /*!Try to insert a new variable in a list
     * If an englobing variable of the current variable already exists, then we don't include the variable
     * If any variable englobed by the current variable exists, then we delete the variable
     * If the variable is an array en we can form a range with the to access, we do that deleting the existing element of the list and
     * including the new ranged access
     */
    static Utils::ext_sym_set insert_var_in_list( Nodecl::NodeclBase var, Utils::ext_sym_set list )
    {
        Utils::ext_sym_set new_list;
        if( !Utils::ext_sym_set_contains_englobing_nodecl( var, list ) )
        {
            // Create a new list with the elements of 'list' that are not englobed by 'var'
            Utils::ext_sym_set aux_list;
            aux_list.insert( Utils::ExtendedSymbol( var ) );
            for( Utils::ext_sym_set::iterator it = list.begin( ); it != list.end( ); ++it )
            {
                if( !Utils::ext_sym_set_contains_englobing_nodecl( it->get_nodecl( ), aux_list ) )
                {
                    new_list.insert( *it );
                }
            }

            // Insert the new variable
            new_list.insert( var );
        }
        else
        {   // No need to insert the variable, its englobing symbol is already there
            // FIXME We can create ranges for array accesses here
            new_list = list;
        }
        return new_list;
    }

    /*!
     * Inserts the elements in 'l' to the list 'in_l' when they are not in the list 'killed' nor in 'undef'
     * When avoiding lists, it take cares of elements englobing the current variable and of elements englobed by the current variable
     */
    static Utils::ext_sym_set compute_use_def_with_children( Utils::ext_sym_set l, Utils::ext_sym_set in_l,
                                                             Utils::ext_sym_set& killed, Utils::ext_sym_set& undef,
                                                             char compute_undef )
    {
        Utils::ext_sym_set new_l = in_l;
        for( Utils::ext_sym_set::iterator it = l.begin( ); it != l.end( ); ++it )
        {
            Nodecl::NodeclBase var = it->get_nodecl( );
            if( !Utils::ext_sym_set_contains_englobing_nodecl( var, killed ) )
            {   // No englobing variable in the avoiding list 1
                // Look for variables in avoiding list 1 englobed by 'var'
                Utils::ext_sym_set aux_set;
                aux_set.insert( Utils::ExtendedSymbol( var ) );
                Utils::ext_sym_set::iterator itk = killed.begin( );
                for( ; itk != killed.end( ); ++itk )
                {
                    if( Utils::ext_sym_set_contains_englobing_nodecl(itk->get_nodecl( ), aux_set) )
                    {   // Delete from 'var' the englobed part of (*itk) and put the result in 'var'
                        // TODO
                        WARNING_MESSAGE( "Part of nodecl '%s' founded in the current var must be avoided. " \
                                         "A subpart is killed.", itk->get_nodecl( ).prettyprint( ).c_str( ),
                                         var.prettyprint( ).c_str( ) );
                        //                             var = nodecl_subtract(var, ita->get_nodecl( ) );
                        killed.erase( itk );
                        if( compute_undef == '1' )
                            new_l = insert_var_in_list( var, new_l );
                        else
                            undef.insert( var );
                        break;
                    }
                }

                if( !Utils::ext_sym_set_contains_englobing_nodecl( var, undef ) )
                {   // No englobing variable in the avoiding list 2
                    // Look for variables in avoiding list 2 englobed by 'var'
                    Utils::ext_sym_set aux_set_2; aux_set_2.insert( *it );
                    Utils::ext_sym_set::iterator itu = undef.begin( );
                    for( ; itu != undef.end( ); ++itu )
                    {
                        if( Utils::ext_sym_set_contains_englobing_nodecl( itu->get_nodecl( ), aux_set_2 ) )
                        {   // Delete from var the englobed part of (*itu) and put the result in 'var'
                            // TODO
                            WARNING_MESSAGE( "Part of nodecl founded in the current var must be avoided. "\
                                             "A subpart is undefined.", itu->get_nodecl( ).prettyprint( ).c_str( ),
                                             var.prettyprint( ).c_str( ) );
                            undef.erase( itu );
                            if( compute_undef == '1' )
                                new_l = insert_var_in_list( var, new_l );
                            else
                                undef.insert( var );
                            break;
                        }
                    }
                    if( itk == killed.end( ) && itu == undef.end( ) )
                    {
                        new_l = insert_var_in_list( var, new_l );
                    }
                }
            }
        }
        return new_l;
    }

    ObjectList<Utils::ext_sym_set> UseDef::get_use_def_over_nodes( Node* current )
    {
        ObjectList<Utils::ext_sym_set> use_def, use_def_aux;

        if( !current->is_visited( ) )
        {
            current->set_visited( true );

            // Use-Def in current node
            Utils::ext_sym_set ue_vars = current->get_ue_vars( );
            Utils::ext_sym_set killed_vars = current->get_killed_vars( );
            Utils::ext_sym_set undef_vars = current->get_undefined_behaviour_vars( );

            // Concatenate info from children nodes
            ObjectList<Node*> children = current->get_children( );
            Utils::ext_sym_set ue_children, killed_children, undef_children;
            for( ObjectList<Node*>::iterator it = children.begin( ); it != children.end( ); ++it )
            {
                use_def_aux = get_use_def_over_nodes( *it );
                if( !use_def_aux.empty( ) )
                {
                    ue_children = ext_sym_set_union( ue_children, use_def_aux[0] );
                    killed_children = ext_sym_set_union( killed_children, use_def_aux[1] );
                    undef_children = ext_sym_set_union( undef_children, use_def_aux[2] );
                }
            }

            // Append to current node info from children
            ue_vars = compute_use_def_with_children( ue_children, ue_vars,
                                                     killed_vars, undef_vars, /*compute_undef*/ '0' );
            undef_vars = compute_use_def_with_children( undef_children, undef_vars,
                                                        killed_vars, undef_vars, /*compute_undef*/ '1' );
            killed_vars = compute_use_def_with_children( killed_children, killed_vars,
                                                         killed_vars, undef_vars, /*compute_undef*/ '0' );

            use_def.append( ue_vars );
            use_def.append( killed_vars );
            use_def.append( undef_vars );
        }

        return use_def;
    }

    void UseDef::set_graph_node_use_def( Node* current )
    {
        if( current->is_graph_node( ) )
        {
            if( !current->is_visited( ) )
            {
                current->set_visited( true );

                Utils::ext_sym_set ue_vars, killed_vars, undef_vars;
                ObjectList<Utils::ext_sym_set> usage = get_use_def_over_nodes( current->get_graph_entry_node( ) );
                ue_vars = usage[0];
                killed_vars = usage[1];
                undef_vars = usage[2];

                if( current->is_omp_loop_node( ) || current->is_omp_sections_node( ) || current->is_omp_single_node( )
                    || current->is_omp_parallel_node( ) || current->is_omp_task_node( ) )
                {   // Take into account data-sharing clauses in Use-Def Task node computation
                    Nodecl::List environ =
                            current->get_graph_label( ).as<Nodecl::OpenMP::Task>( ).get_environment( ).as<Nodecl::List>( );
                    for( Nodecl::List::iterator it = environ.begin( ); it != environ.end( ); ++it )
                    {
                        if( it->is<Nodecl::OpenMP::Private>( ) )
                        {   // Remove any usage computed in the inner nodes,
                            // because is the usage of a copy of this variable
                            Nodecl::List private_syms = it->as<Nodecl::OpenMP::Private>( ).get_private_symbols( ).as<Nodecl::List>( );
                            for( Nodecl::List::iterator it_p = private_syms.begin( ); it_p != private_syms.end( ); ++it_p )
                            {
//                                 std::cerr << "Private symbol: " << it_p->prettyprint( ) << std::endl;
                                if( Utils::ext_sym_set_contains_nodecl( *it_p, undef_vars ) )
                                {
//                                     std::cerr << "     is in undef set" << std::endl;
                                    undef_vars.erase( Utils::ExtendedSymbol( *it_p ) );
                                }
                                else
                                {
                                    if( Utils::ext_sym_set_contains_nodecl( *it_p, ue_vars ) )
                                    {
                                        ue_vars.erase( Utils::ExtendedSymbol( *it_p ) );
                                    }
                                    if( Utils::ext_sym_set_contains_nodecl( *it_p, killed_vars ) )
                                    {
                                        killed_vars.erase( Utils::ExtendedSymbol( *it_p ) );
                                    }
                                }
                            }
                        }
                        if( it->is<Nodecl::OpenMP::Firstprivate>( ) )
                        {   // This variable is Upper Exposed in the task
                            Nodecl::List firstprivate_syms = it->as<Nodecl::OpenMP::Firstprivate>( ).get_firstprivate_symbols( ).as<Nodecl::List>( );
                            for( Nodecl::List::iterator it_fp = firstprivate_syms.begin( ); it_fp != firstprivate_syms.end( ); ++it_fp )
                            {
                                if( !Utils::ext_sym_set_contains_nodecl( *it_fp, ue_vars ) )
                                {
                                    ue_vars.insert( Utils::ExtendedSymbol( *it_fp ) );
                                }
                            }
                        }
                    }
                }

                current->set_ue_var( ue_vars );
                current->set_killed_var( killed_vars );
                current->set_undefined_behaviour_var( undef_vars );
            }
        }
        else
        {
            internal_error( "Cannot propagate use-def info from inner nodes to outer nodes "\
                            "in node '%d' with type '%s'. GRAPH_NODE expected\n",
                            current->get_id( ), current->get_type_as_string( ).c_str( ) );
        }
    }

    // ************************** End class implementing use-definition analysis ************************** //
    // **************************************************************************************************** //



    // **************************************************************************************************** //
    // ***************************** Class implementing use-definition visitor **************************** //

    static void get_use_def_variables( Node* actual, int id_target_node,
                                       Utils::ext_sym_set &ue_vars,
                                       Utils::ext_sym_set &killed_vars,
                                       Utils::ext_sym_set &undef_vars )
    {
        ObjectList<Node*> children = actual->get_children( );
        for( ObjectList<Node*>::iterator it = children.begin( ); it != children.end( ); ++it )
        {
            if( ( *it )->get_id( ) != id_target_node )
            {
                ue_vars = ext_sym_set_union( ue_vars, ( *it )->get_ue_vars( ) );
                killed_vars = ext_sym_set_union( killed_vars, ( *it )->get_killed_vars( ) );
                undef_vars = ext_sym_set_union( undef_vars, ( *it )->get_undefined_behaviour_vars( ) );

                get_use_def_variables( *it, id_target_node, ue_vars, killed_vars, undef_vars );
            }
        }
    }

    static Utils::nodecl_set get_arguments_list( sym_to_nodecl_map ref_params_to_args )
    {
        Utils::nodecl_set result;

        for( sym_to_nodecl_map::iterator it = ref_params_to_args.begin( );
             it != ref_params_to_args.end( ); ++it )
        {
            result.insert( it->second );
        }

        return result;
    }

    static sym_to_nodecl_map map_reference_params_to_args( ObjectList<TL::Symbol> parameters,
                                                           Nodecl::List arguments )
    {
        sym_to_nodecl_map ref_params_to_args;

        ObjectList<TL::Symbol>::iterator itp = parameters.begin( );
        Nodecl::List::iterator ita = arguments.begin( );
        for( ; itp != parameters.end( ); ++itp, ++ita )
        {
            Type param_type = itp->get_type( );
            if( ( param_type.is_any_reference( ) || param_type.is_pointer( ) ) )
            {
                ref_params_to_args[*itp] = *ita;
            }
        }

        return ref_params_to_args;
    }

    static sym_to_nodecl_map map_non_reference_params_to_args( ObjectList<TL::Symbol> parameters,
                                                               Nodecl::List arguments )
    {
        sym_to_nodecl_map non_ref_params_to_args;

        ObjectList<TL::Symbol>::iterator itp = parameters.begin( );
        Nodecl::List::iterator ita = arguments.begin( );
        for( ; itp != parameters.end( ); ++itp, ++ita )
        {
            Type param_type = itp->get_type( );
            if( !param_type.is_any_reference( ) && !param_type.is_pointer( ) )
            {
                // If some memory access in the argument is a symbol, then we add the tuple to the map
                ObjectList<Nodecl::NodeclBase> obj = Nodecl::Utils::get_all_memory_accesses( *ita );
                for( ObjectList<Nodecl::NodeclBase>::iterator it = obj.begin( ); it != obj.end( ); ++it )
                {
                    if( it->has_symbol( ) )
                    {
                        non_ref_params_to_args[*itp] = *ita;
                        break;
                    }
                }
            }
        }

        return non_ref_params_to_args;
    }

    UsageVisitor::UsageVisitor( Node* n,
                                std::set<Symbol> visited_functions,
                                ObjectList<Utils::ExtendedSymbolUsage> visited_global_vars,
                                bool ipa, Scope sc, Utils::nodecl_set ipa_arguments )
        : _node( n ), _define( false ), _current_nodecl( Nodecl::NodeclBase::null( ) ),
          _visited_functions( visited_functions ), _visited_global_vars( visited_global_vars ),
          _ipa( ipa ), _sc( sc ), _ipa_arguments( ipa_arguments )
    { }

    UsageVisitor::UsageVisitor( const UsageVisitor& v )
    {
        _node = v._node;
        _define = v._define;
        _current_nodecl = v._current_nodecl;
        _visited_functions = v._visited_functions;
        _visited_global_vars = v._visited_global_vars;
        _ipa = v._ipa;
        _sc = v._sc;
        _ipa_arguments = v._ipa_arguments;
    }

    std::set<Symbol> UsageVisitor::get_visited_functions( ) const
    {
        return _visited_functions;
    }

    ObjectList<Utils::ExtendedSymbolUsage> UsageVisitor::get_visited_global_variables( ) const
    {
        return _visited_global_vars;
    }

    bool UsageVisitor::variable_is_in_context( Nodecl::NodeclBase var )
    {
        // When IPA, only global variables and referenced parameters are in context
        // Otherwise, any variable is in context
        if( ( _ipa && ( !var.retrieve_context( ).scope_is_enclosed_by( _sc )
                         || ( _ipa_arguments.find( var ) != _ipa_arguments.end( ) ) ) )
               || !_ipa )
        {
             return true;
        }
        return false;
    }

    void UsageVisitor::compute_statement_usage( Nodecl::NodeclBase st )
    {
        walk( st );

        // Propagate Use-Def info from inner nodes to outer node
        if( _node->is_split_statement( ) )
        {
            Node* inner_graph = _node->get_graph_entry_node( )->get_children( )[0];
            Utils::ext_sym_set inner_ue = inner_graph->get_ue_vars( );
            Utils::ext_sym_set inner_killed = inner_graph->get_killed_vars( );
            Utils::ext_sym_set inner_undef = inner_graph->get_undefined_behaviour_vars( );

            _node->set_ue_var(
                    Utils::ext_sym_set_difference(
                            Utils::ext_sym_set_difference(
                                    Utils::ext_sym_set_union( _node->get_ue_vars( ),
                                                              inner_ue ),
                                    inner_killed ),
                            inner_undef ) );
            _node->set_killed_var(
                    Utils::ext_sym_set_difference(
                            Utils::ext_sym_set_union( _node->get_killed_vars(),
                                                      inner_killed ),
                            inner_undef ) );
            _node->set_undefined_behaviour_var(
                    Utils::ext_sym_set_union( _node->get_undefined_behaviour_vars( ),
                                              inner_undef ) );
        }
    }

    void UsageVisitor::unhandled_node( const Nodecl::NodeclBase& n )
    {
        nodecl_t internal_n = n.get_internal_nodecl( );
        WARNING_MESSAGE( "Unhandled node '%s' with type '%s 'during Use-Def Analysis'",
                         codegen_to_str( internal_n, nodecl_retrieve_context( internal_n ) ),
                         ast_print_node_type( n.get_kind( ) ) );
    }

    void UsageVisitor::binary_assignment_visit( Nodecl::NodeclBase lhs, Nodecl::NodeclBase rhs )
    {
        // Traverse the use of both the lhs and the rhs
        walk( rhs );
        walk( lhs );

        // Traverse the definition of the lhs
        _define = true;
        walk( lhs );
        _define = false;
    }

    void UsageVisitor::parse_parameter( std::string current_param, Nodecl::NodeclBase arg )
    {
        size_t first_slash_pos = current_param.find( "#" );
        if( first_slash_pos != std::string::npos )
        {   // Parameter is pointer
            // The address is used
            _node->set_ue_var( Utils::ExtendedSymbol( arg ) );
            size_t second_slash_pos = current_param.find( "#", first_slash_pos );
            std::string pointed_param_usage = current_param.substr( first_slash_pos, second_slash_pos - first_slash_pos );
            // TODO: What do we want to do with the pointed value??
        }
        else
        {
            ObjectList<Nodecl::NodeclBase> obj = Nodecl::Utils::get_all_memory_accesses( arg );
            for( ObjectList<Nodecl::NodeclBase>::iterator it_o = obj.begin( ); it_o != obj.end( ); ++it_o )
            {
                // Set all arguments as upper exposed
                _node->set_ue_var( Utils::ExtendedSymbol( *it_o ) );
            }
        }
    }

    bool UsageVisitor::parse_c_functions_file( Symbol func_sym, Nodecl::List args )
    {
        bool side_effects = true;

        std::string cLibFuncsPath = std::string( MCXX_ANALYSIS_DATA_PATH ) + "/cLibraryFunctionList" ;
        std::ifstream cLibFuncs( cLibFuncsPath.c_str( ) );
        if( cLibFuncs.is_open( ) )
        {
            std::string func_decl;
            while( cLibFuncs.good( ) )
            {
                getline( cLibFuncs, func_decl );
                if( func_decl.substr( 0, 2 ) != "//" )
                {
                    size_t open_parenth_pos = func_decl.find( "(" );
                    std::string func_name = func_decl.substr( 0, open_parenth_pos - 1 );
                    if( func_sym.get_name( ) == func_name )
                    {   // No global variable is read / written
                        // Check for parameters usage
                        side_effects = false;

                        size_t comma_pos = func_decl.find( "," );
                        if( comma_pos == std::string::npos )
                        {
                            comma_pos = func_decl.find( ")" );
                        }
                        size_t last_comma_pos = open_parenth_pos + 1;
                        std::string current_param;
                        Nodecl::List::iterator it = args.begin( );
                        while( comma_pos != std::string::npos && /* not a default parameter*/ it != args.end( ) )
                        {
                            current_param = func_decl.substr( last_comma_pos, comma_pos - last_comma_pos );
                            parse_parameter( current_param, *it );
                            it++;
                            last_comma_pos = comma_pos + 1;
                            comma_pos = func_decl.find( ",", last_comma_pos );
                        }
                        // Last parameter
                        if( it != args.end( ) )
                        {
                            current_param = func_decl.substr( last_comma_pos, func_decl.find( ")", last_comma_pos ) - last_comma_pos );
                            if( current_param == "..." )
                            {   // Arguments are supposed to be only used
                                ObjectList<Nodecl::NodeclBase> obj;
                                while( it != args.end( ) )
                                {
                                    obj = Nodecl::Utils::get_all_memory_accesses( *it );
                                    for( ObjectList<Nodecl::NodeclBase>::iterator it_o = obj.begin( ); it_o  != obj.end( ); ++it_o )
                                    {
                                        _node->set_ue_var( Utils::ExtendedSymbol( *it_o ) );
                                    }
                                    ++it;
                                }
                            }
                            else
                            {
                                parse_parameter( current_param, *it );
                            }
                        }
                    }
                }
            }

            if( side_effects && VERBOSE )
            {
//                 WARNING_MESSAGE( "Function's '%s' code not reached. Usage of global variables and "\
//                                  "reference parameters will be limited. If you know the side effects of this function, "\
//                                  "add it to the file and recompile your code. \n(If you recompile the compiler, "\
//                                  "you want to add the function in $MCC_HOME/src/tl/analysis/use_def/cLibraryFunctionList instead).",
//                                  func_sym.get_name( ).c_str( ), cLibFuncsPath.c_str( ) );
            }
            cLibFuncs.close();
        }
        else
        {
            WARNING_MESSAGE( "File containing C library calls Usage info cannot be opened. \n"\
                             "Path tried: '%s'", cLibFuncsPath.c_str( ) );
        }

        return side_effects;
    }

    void UsageVisitor::function_visit( Nodecl::NodeclBase called_sym, Nodecl::NodeclBase arguments )
    {
        TL::Symbol func_sym = called_sym.get_symbol( );

        // The function called must be analyzed only in case it has not been analyzed previously
        if( _visited_functions.find( func_sym ) == _visited_functions.end( ) )
        {
            Nodecl::FunctionCode called_func =
                func_sym.get_function_code( ).as<Nodecl::FunctionCode>( );

            ObjectList<TL::Symbol> params = func_sym.get_function_parameters( );
            Nodecl::List args = arguments.as<Nodecl::List>( );
            if( !called_func.is_null( ) )
            {
                Nodecl::FunctionCode copied_func =
                        called_func.shallow_copy( ).as<Nodecl::FunctionCode>( );

                // Create renaming map
                sym_to_nodecl_map renaming_map =
                        map_reference_params_to_args( params, args );

                // Rename the parameters with the arguments
                RenameVisitor rv( renaming_map );
                rv.rename_expressions( copied_func );

                // Create the PCFG for the renamed code
                PCFGVisitor pcfgv( Utils::generate_hashed_name( copied_func ), copied_func );
                ExtensibleGraph* pcfg = pcfgv.parallel_control_flow_graph( copied_func );

                _visited_global_vars.insert( pcfg->get_global_variables( ) );

                // Compute the Use-Def variables of the code
                _visited_functions.insert( func_sym );
                UseDef ue( pcfg );
                ue.compute_usage( _visited_functions, _visited_global_vars,
                                  /* ipa */ true, get_arguments_list( renaming_map ) );
                _visited_functions.erase( func_sym );

                // Set the node usage
                Node* pcfg_node = pcfg->get_graph( );
                    // reference parameters and global variables
                Utils::ext_sym_set ue_vars = pcfg_node->get_ue_vars( );
                Utils::ext_sym_set killed_vars = pcfg_node->get_killed_vars( );
                Utils::ext_sym_set undef_vars = pcfg_node->get_undefined_behaviour_vars( );
                    // value parameters
                sym_to_nodecl_map non_ref_params = map_non_reference_params_to_args( params, args );
                for( sym_to_nodecl_map::iterator it = non_ref_params.begin( );
                    it != non_ref_params.end( ); ++it )
                {
                    ObjectList<Nodecl::NodeclBase> syms = Nodecl::Utils::get_all_memory_accesses( it->second );
                    for( ObjectList<Nodecl::NodeclBase>::iterator it_s = syms.begin( ); it_s != syms.end( ); ++it_s )
                    {
                        ue_vars.insert( Utils::ExtendedSymbol( *it_s ) );
                    }
                }
                    // set the values
                _node->set_ue_var( ue_vars );
                _node->set_killed_var( killed_vars );
                _node->set_undefined_behaviour_var( undef_vars );

                // Propagate the function call Use-Def info to outer nodes
                // in case the call is inside a bigger instruction.
                // F.i.:   int b = foo(a, b)
                //         PCFG:
                //           ______________________________________________
                //          |  [SPLIT_STMT]                                |
                //          |  __________________________________________  |
                //          | | [FUNC_CALL]                              | |
                //          | |  _______       ___________       ______  | |
                //          | | |       |     |           |     |      | | |
                //          | | | ENTRY |---->| foo(a, b) |---->| EXIT | | |
                //          | | |_______|     |___________|     |______| | |
                //          | |__________________________________________| |
                //          |               _______|_______                |
                //          |              |               |               |
                //          |              | b = foo(a, b) |               |
                //          |              |_______________|               |
                //          |______________________________________________|
                //
                //         When computing Use-Def of "foo(a, b)", we can easily propagate
                //             the info to the node "b=foo(a, b)".
                //             We will propagate, the function Use-Def to the outer nodes
                //             of a FUNC_CALL node until we find a non-SPLIT_STMT node.
                Node* func_node = _node->get_outer_node();
                ERROR_CONDITION( func_node->is_function_call_node( ),
                                 "Outer node of a statement containing only a function call "\
                                 "must be of type FUNC_CALL. Instead, outer node of node %d, "\
                                 "with cal to function %s, has type %s\n", _node->get_id( ),
                                 called_sym.get_symbol( ).get_name( ).c_str( ),
                                 func_node->get_graph_type_as_string( ).c_str( ) );
                Node* outer_node = func_node->get_outer_node( );
                while( outer_node->is_split_statement( ) )
                {
                    ERROR_CONDITION( outer_node->get_graph_exit_node( )->get_parents().size( ) != 1,
                                     "Expecting only one node as parent of the Exit Node in "\
                                     "the SPLIT_STMT node %d", outer_node->get_id( ) );
                    Node* split_stmt_node = outer_node->get_graph_exit_node( )->get_parents()[0];
                    split_stmt_node->set_ue_var( ue_vars );
                    split_stmt_node->set_killed_var( killed_vars );
                    split_stmt_node->set_undefined_behaviour_var( undef_vars );

                    outer_node = outer_node->get_outer_node( );
                }
            }
            else
            {   // We do not have access to the called code
                // Check whether we have enough attributes in the function symbol
                // to determine the function side effects
                bool side_effects = true;

                if( func_sym.has_gcc_attributes( ) )
                {   // Check for information synthesized by gcc
                    ObjectList<GCCAttribute> gcc_attrs = func_sym.get_gcc_attributes( );
                    for( ObjectList<GCCAttribute>::iterator it = gcc_attrs.begin( );
                         it != gcc_attrs.end( ); ++it )
                    {
                        std::string attr_name = it->get_attribute_name( );
                        if( attr_name == "const" || attr_name == "pure" )
                        {   // No side effects except the return value.
                            // Only examine the arguments ( and global variables in 'pure' case)
                            side_effects = false;

                            Utils::ext_sym_set ue_vars;
                            // Set all parameters as used ( if not previously killed or undefined )
                            for( Nodecl::List::iterator it_arg = args.begin( ); it_arg != args.end( ); ++it_arg )
                            {
                                Utils::ExtendedSymbol es( *it_arg );
                                if( _node->get_killed_vars( ).find( es ) == _node->get_killed_vars( ).end( )
                                    && _node->get_undefined_behaviour_vars( ).find( es ) == _node->get_undefined_behaviour_vars( ).end( ) )
                                {
                                    ue_vars.insert( es );
                                }
                            }

                            if( attr_name == "pure" )
                            {   // Set all global variables as upper exposed ( if not previously killed or undefined )
                                for( ObjectList<Utils::ExtendedSymbolUsage>::iterator it_g =
                                    _visited_global_vars.begin( ); it_g != _visited_global_vars.end( ); ++it_g )
                                {
                                    Utils::ExtendedSymbol es( it_g->get_extended_symbol() );
                                    if( _node->get_killed_vars( ).find( es ) == _node->get_killed_vars( ).end( )
                                        && _node->get_undefined_behaviour_vars( ).find( es ) == _node->get_undefined_behaviour_vars( ).end( ) )
                                    {
                                        ue_vars.insert( es );
                                    }
                                }
                            }
                            _node->set_ue_var( ue_vars );
                        }
                    }
                }

                if( side_effects )
                {
                    // Check in Mercurium function attributes data-base
                    side_effects = parse_c_functions_file( func_sym, args );

                    // Still cannot determine which are the side effects of the function...
                    if( side_effects )
                    {
                        // Set all reference parameters to undefined
                        sym_to_nodecl_map ref_params = map_reference_params_to_args( params, args );
                        for( sym_to_nodecl_map::iterator it = ref_params.begin( );
                            it != ref_params.end( ); ++it )
                        {
                            if( Nodecl::Utils::nodecl_is_modifiable_lvalue( it->second ) )
                            {
                                _node->set_undefined_behaviour_var_and_recompute_use_and_killed_sets(
                                        Utils::ExtendedSymbol( it->second ) );
                            }
                        }

                        // Set the value passed parameters as upper exposed
                        sym_to_nodecl_map non_ref_params = map_non_reference_params_to_args( params, args );
                        for( sym_to_nodecl_map::iterator it = non_ref_params.begin( );
                            it != non_ref_params.end( ); ++it )
                        {
                            ObjectList<Nodecl::NodeclBase> obj = Nodecl::Utils::get_all_memory_accesses( it->second );
                            for( ObjectList<Nodecl::NodeclBase>::iterator it_o = obj.begin( ); it_o != obj.end( ); ++it_o )
                            {
                                _node->set_ue_var( Utils::ExtendedSymbol( *it_o ) );
                            }
                        }

                        // Set all global variables to undefined
                        for( ObjectList<Utils::ExtendedSymbolUsage>::iterator it =
                            _visited_global_vars.begin( ); it != _visited_global_vars.end( ); ++it )
                        {
                            _node->set_undefined_behaviour_var_and_recompute_use_and_killed_sets(
                                    it->get_extended_symbol() );
                        }
                    }
                }
            }
        }
        else
        {
            // We are in a recursive call
            // Set the value passed parameters as upper exposed
            ObjectList<TL::Symbol> params = func_sym.get_function_parameters( );
            Nodecl::List args = arguments.as<Nodecl::List>( );
            sym_to_nodecl_map non_ref_params = map_non_reference_params_to_args( params, args );
            for( sym_to_nodecl_map::iterator it = non_ref_params.begin( );
                it != non_ref_params.end( ); ++it )
            {
                ObjectList<Nodecl::NodeclBase> obj = Nodecl::Utils::get_all_memory_accesses( it->second );
                for( ObjectList<Nodecl::NodeclBase>::iterator it_o = obj.begin( ); it_o != obj.end( ); ++it_o )
                {
                    _node->set_ue_var( Utils::ExtendedSymbol( *it_o ) );
                }
            }

            // Check for the usage in the graph of the function to propagate Usage (Global variables and reference parameters)
            // until the point we are currently
            // TODO
        }
    }

    void UsageVisitor::unary_in_de_crement_visit( Nodecl::NodeclBase rhs )
    {
        // Use of the rhs
        walk( rhs );

        // Definition of the rhs
        _define = true;
        Nodecl::NodeclBase current_nodecl = _current_nodecl;
        _current_nodecl = Nodecl::NodeclBase::null( );
        walk( rhs );
        _current_nodecl = current_nodecl;
        _define = false;

    }

    void UsageVisitor::visit( const Nodecl::AddAssignment& n )
    {
        binary_assignment_visit( n.get_lhs( ), n.get_rhs( ) );
    }

    void UsageVisitor::visit( const Nodecl::ArithmeticShrAssignment& n )
    {
        binary_assignment_visit( n.get_lhs( ), n.get_rhs( ) );
    }

    void UsageVisitor::visit( const Nodecl::ArraySubscript& n )
    {
        Nodecl::NodeclBase current_nodecl = _current_nodecl;
        bool define = _define;

        // Use of the subscripts
        _define = false;
        _current_nodecl = Nodecl::NodeclBase::null( );
        walk( n.get_subscripts( ) );

        // Use of the ArraySubscript
        if( current_nodecl.is_null( ) )
        {
            _define = define;
            _current_nodecl = n;
        }
        walk( n.get_subscripted( ) );
        _current_nodecl = Nodecl::NodeclBase::null( );

        // If we were traversing some object, then the use of that access
        if( !current_nodecl.is_null( ) )
        {
            _define = define;   // Just in case
            _current_nodecl = current_nodecl;
            walk( n.get_subscripted( ) );
        }
    }

    void UsageVisitor::visit( const Nodecl::Assignment& n )
    {
        _define = false;
        walk( n.get_rhs( ) );
        _define = true;
        walk( n.get_lhs( ) );
        _define = false;
    }

    void UsageVisitor::visit( const Nodecl::BitwiseAndAssignment& n )
    {
        binary_assignment_visit( n.get_lhs( ), n.get_rhs( ) );
    }

    void UsageVisitor::visit( const Nodecl::BitwiseOrAssignment& n )
    {
        binary_assignment_visit( n.get_lhs( ), n.get_rhs( ) );
    }

    void UsageVisitor::visit( const Nodecl::BitwiseShlAssignment& n )
    {
        binary_assignment_visit( n.get_lhs( ), n.get_rhs( ) );
    }

    void UsageVisitor::visit( const Nodecl::BitwiseShrAssignment& n )
    {
        binary_assignment_visit( n.get_lhs( ), n.get_rhs( ) );
    }

    void UsageVisitor::visit( const Nodecl::BitwiseXorAssignment& n )
    {
        binary_assignment_visit( n.get_lhs( ), n.get_rhs( ) );
    }

    void UsageVisitor::visit( const Nodecl::ClassMemberAccess& n )
    {
        if( _current_nodecl.is_null( ) )
            _current_nodecl = n;

        // walk( n.get_lhs( ) );  // In a member access, the use/definition is always of the member, not the base
        walk( n.get_member( ) );

        if( !_current_nodecl.is_null( ) )
            _current_nodecl = Nodecl::NodeclBase::null( );
    }

    void UsageVisitor::visit( const Nodecl::Dereference& n )
    {
        Nodecl::NodeclBase current_nodecl = _current_nodecl;
        bool define = _define;

        // Use of the Dereferenced variable
        _define = false;
        _current_nodecl = Nodecl::NodeclBase::null( );
        walk( n.get_rhs( ) );

        // Use of the Dereference
        if( current_nodecl.is_null( ) )
        {
            _define = define;
            _current_nodecl = n;
        }
        walk( n.get_rhs( ) );
        _current_nodecl = Nodecl::NodeclBase::null( );

        // If we were traversing some object, then the use of that access
        if( !current_nodecl.is_null( ) )
        {
            _define = define;       // Just in case
            _current_nodecl = current_nodecl;
            walk( n.get_rhs( ) );
        }

        if( current_nodecl.is_null( ) )
            _current_nodecl = Nodecl::NodeclBase::null( );
    }

    void UsageVisitor::visit( const Nodecl::DivAssignment& n )
    {
        binary_assignment_visit( n.get_lhs( ), n.get_rhs( ) );
    }

    void UsageVisitor::visit( const Nodecl::FunctionCall& n )
    {
        function_visit( n.get_called( ), n.get_arguments( ) );
    }

    void UsageVisitor::visit( const Nodecl::MinusAssignment& n )
    {
        binary_assignment_visit( n.get_lhs( ), n.get_rhs( ) );
    }

    void UsageVisitor::visit( const Nodecl::ModAssignment& n )
    {
        binary_assignment_visit( n.get_lhs( ), n.get_rhs( ) );
    }

    void UsageVisitor::visit( const Nodecl::MulAssignment& n )
    {
        binary_assignment_visit( n.get_lhs( ), n.get_rhs( ) );
    }

    void UsageVisitor::visit( const Nodecl::ObjectInit& n )
    {
        Nodecl::Symbol n_sym = Nodecl::Symbol::make( n.get_symbol( ), n.get_locus() );
        _node->set_killed_var( Utils::ExtendedSymbol( n_sym ) );

        // Value of initialization, in case it exists
        walk( n.get_symbol( ).get_value( ) );
    }

    void UsageVisitor::visit( const Nodecl::PointerToMember& n )
    {
        internal_error( "PointerToMemeber not yet implemented in UsageVisitor.", 0 );
    }

    void UsageVisitor::visit( const Nodecl::Postdecrement& n )
    {
        unary_in_de_crement_visit( n.get_rhs( ) );
    }

    void UsageVisitor::visit( const Nodecl::Postincrement& n )
    {
        unary_in_de_crement_visit( n.get_rhs( ) );
    }

    void UsageVisitor::visit( const Nodecl::Predecrement& n )
    {
        unary_in_de_crement_visit( n.get_rhs( ) );
    }

    void UsageVisitor::visit( const Nodecl::Preincrement& n )
    {
        unary_in_de_crement_visit( n.get_rhs( ) );
    }

    void UsageVisitor::visit( const Nodecl::Range& n )
    {
        walk( n.get_lower() );
        walk( n.get_upper() );
        walk( n.get_stride() );
    }

    void UsageVisitor::visit( const Nodecl::Reference& n )
    {
        if( _current_nodecl.is_null( ) )
            _current_nodecl = n;

        walk( n.get_rhs( ) );

        if( !_current_nodecl.is_null( ) )
            _current_nodecl = Nodecl::NodeclBase::null( );
    }

    void UsageVisitor::visit( const Nodecl::Symbol& n )
    {
        Nodecl::NodeclBase var_in_use = n;
        if( !_current_nodecl.is_null( ) )
        {
            var_in_use = _current_nodecl;
        }

        if( variable_is_in_context( var_in_use ) )
        {
            if( _define )
                _node->set_killed_var( Utils::ExtendedSymbol( var_in_use ) );
            else
            {
                if( !Utils::ext_sym_set_contains_nodecl( var_in_use, _node->get_killed_vars() ) )
                    _node->set_ue_var( Utils::ExtendedSymbol( var_in_use ) );
            }
        }
    }

    void UsageVisitor::visit( const Nodecl::VirtualFunctionCall& n )
    {
        function_visit( n.get_called( ), n.get_arguments( ) );
    }

    // *************************** End class implementing use-definition visitor ************************** //
    // **************************************************************************************************** //

}
}
