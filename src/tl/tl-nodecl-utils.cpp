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

#include "tl-nodecl-utils.hpp"
#include "tl-counters.hpp"
#include "tl-predicateutils.hpp"
#include "cxx-nodecl-deep-copy.h"
#include "cxx-utils.h"
#include <algorithm>

namespace Nodecl
{
    static void get_all_symbols_rec(Nodecl::NodeclBase n, TL::ObjectList<TL::Symbol>& result)
    {
        if (n.is_null())
            return;

        if (n.has_symbol())
        {
            if (n.is<Nodecl::ObjectInit>())
            {
                get_all_symbols_rec(n.get_symbol().get_value(), result);
            }
            result.insert(n.get_symbol());
        }

        TL::ObjectList<Nodecl::NodeclBase> children = n.children();

        for (TL::ObjectList<Nodecl::NodeclBase>::iterator it = children.begin();
                it != children.end();
                it++)
        {
            get_all_symbols_rec(*it, result);
        }
    }

    TL::ObjectList<TL::Symbol> Utils::get_all_symbols(Nodecl::NodeclBase n)
    {
        TL::ObjectList<TL::Symbol> sym_list;
        get_all_symbols_rec(n, sym_list);
        return sym_list;
    }

    static bool is_parameter_of_nonnested_function(TL::Symbol symbol, TL::Scope sc)
    {
        // This function returns true if this symbol is a parameter of a
        // function that is not the current one nor an enclosing one
        if (!symbol.is_parameter_of_a_function())
            return false;

        TL::Symbol current_function = sc.get_decl_context().current_scope->related_entry;
        if (!current_function.is_valid())
            return false;

        if (symbol.is_parameter_of(current_function))
            return false;
        else if (current_function.is_nested_function())
            return is_parameter_of_nonnested_function(symbol, current_function.get_scope());

        return true;
    }

    struct IsLocalSymbol : TL::Predicate<TL::Symbol>
    {
        private:
            TL::Scope _sc;

        public:
            IsLocalSymbol(Nodecl::NodeclBase root)
                : _sc(root.retrieve_context())
            {
            }

            virtual bool do_(const TL::Symbol& sym) const
            {
                // If its scope is contained in the base node one, then it is
                // "local"
                return sym.get_scope().scope_is_enclosed_by(_sc)
                    && !is_parameter_of_nonnested_function(sym, _sc);
            }
    };

    struct IsNonLocalSymbol : TL::Predicate<TL::Symbol>
    {
        private:
            TL::Scope _sc;

        public:
            IsNonLocalSymbol(Nodecl::NodeclBase root)
                : _sc(root.retrieve_context())
            {
            }

            virtual bool do_(const TL::Symbol& sym) const
            {
                // If its scope is not contained in the base node one, then it
                // is "nonlocal"
                return !sym.get_scope().scope_is_enclosed_by(_sc)
                    && !is_parameter_of_nonnested_function(sym, _sc);
            }
    };

    TL::ObjectList<TL::Symbol> Utils::get_local_symbols(Nodecl::NodeclBase n)
    {
        IsLocalSymbol local(n);
        return get_all_symbols(n).filter(local);
    }

    TL::ObjectList<TL::Symbol> Utils::get_nonlocal_symbols(Nodecl::NodeclBase n)
    {
        IsNonLocalSymbol non_local(n);
        return get_all_symbols(n).filter(non_local);
    }

    static void get_all_symbols_occurrences_rec(Nodecl::NodeclBase n, TL::ObjectList<Nodecl::Symbol> &result)
    {
        if (n.is_null())
            return;

        if (n.is<Nodecl::Symbol>())
        {
            result.append(n.as<Nodecl::Symbol>());
        }
        else if (n.is<Nodecl::ObjectInit>())
        {
            get_all_symbols_occurrences_rec(n.get_symbol().get_value(), result);
        }

        TL::ObjectList<Nodecl::NodeclBase> children = n.children();

        for (TL::ObjectList<Nodecl::NodeclBase>::iterator it = children.begin();
                it != children.end();
                it++)
        {
            get_all_symbols_occurrences_rec(*it, result);
        }
    }

    TL::ObjectList<Nodecl::Symbol> Utils::get_all_symbols_occurrences(Nodecl::NodeclBase n)
    {
        TL::ObjectList<Nodecl::Symbol> result;
        get_all_symbols_occurrences_rec(n, result);
        return result;
    }

    struct IsLocalOcurrence : TL::Predicate<Nodecl::Symbol>
    {
        private:
            IsLocalSymbol _pred;

        public:
            IsLocalOcurrence(Nodecl::NodeclBase root)
                : _pred(root)
            {
            }

            virtual bool do_(const Nodecl::Symbol& n) const
            {
                return _pred(n.get_symbol());
            }
    };

    struct IsNonLocalOcurrence : TL::Predicate<Nodecl::Symbol>
    {
        private:
            IsNonLocalSymbol _pred;

        public:
            IsNonLocalOcurrence(Nodecl::NodeclBase root)
                : _pred(root)
            {
            }

            virtual bool do_(const Nodecl::Symbol& n) const
            {
                return _pred(n.get_symbol());
            }
    };

    TL::ObjectList<Nodecl::Symbol> Utils::get_local_symbols_occurrences(Nodecl::NodeclBase n)
    {
        IsLocalOcurrence local(n);
        return get_all_symbols_occurrences(n).filter(local);
    }

    TL::ObjectList<Nodecl::Symbol> Utils::get_nonlocal_symbols_occurrences(Nodecl::NodeclBase n)
    {
        IsNonLocalOcurrence local(n);
        return get_all_symbols_occurrences(n).filter(local);
    }

    static void get_all_symbols_first_occurrence_rec(Nodecl::NodeclBase n, TL::ObjectList<Nodecl::Symbol> &result)
    {
        if (n.is_null())
            return;

        if (n.is<Nodecl::Symbol>())
        {
            result.insert(n.as<Nodecl::Symbol>(),
                    TL::ThisMemberFunctionConstAdapter<TL::Symbol, Nodecl::Symbol>(&Nodecl::Symbol::get_symbol));
        }
        else if (n.is<Nodecl::ObjectInit>())
        {
            get_all_symbols_first_occurrence_rec(n.get_symbol().get_value(), result);
        }

        TL::ObjectList<Nodecl::NodeclBase> children = n.children();
        for (TL::ObjectList<Nodecl::NodeclBase>::iterator it = children.begin();
                it != children.end();
                it++)
        {
            get_all_symbols_first_occurrence_rec(*it, result);
        }
    }

    TL::ObjectList<Nodecl::Symbol> Utils::get_all_symbols_first_occurrence(Nodecl::NodeclBase n)
    {
        TL::ObjectList<Nodecl::Symbol> result;
        get_all_symbols_first_occurrence_rec(n, result);
        return result;
    }

    TL::ObjectList<Nodecl::Symbol> Utils::get_local_symbols_first_occurrence(Nodecl::NodeclBase n)
    {
        IsLocalOcurrence local(n);
        return get_all_symbols_first_occurrence(n).filter(local);
    }

    TL::ObjectList<Nodecl::Symbol> Utils::get_nonlocal_symbols_first_occurrence(Nodecl::NodeclBase n)
    {
        IsNonLocalOcurrence local(n);
        return get_all_symbols_first_occurrence(n).filter(local);
    }

    static void get_all_memory_accesses_rec(Nodecl::NodeclBase n, TL::ObjectList<Nodecl::NodeclBase>& result)
    {
        if (n.is_null())
            return;

        if (n.is<Nodecl::Symbol>() || n.is<Nodecl::ObjectInit>()
            || n.is<Nodecl::ArraySubscript>() || n.is<Nodecl::PointerToMember>()
            || n.is<Nodecl::Reference>() || n.is<Nodecl::Dereference>())
        {
            result.insert(n);
        }

        TL::ObjectList<Nodecl::NodeclBase> children = n.children();
        for (TL::ObjectList<Nodecl::NodeclBase>::iterator it = children.begin();
             it != children.end(); it++)
             {
                 get_all_memory_accesses_rec(*it, result);
             }
    }

    TL::ObjectList<Nodecl::NodeclBase> Utils::get_all_memory_accesses(Nodecl::NodeclBase n)
    {
        TL::ObjectList<Nodecl::NodeclBase> obj_list;
        get_all_memory_accesses_rec(n, obj_list);
        return obj_list;
    }

    static int cmp_trees_rec(nodecl_t n1, nodecl_t n2, bool skip_conversion_nodes)
    {
        if (nodecl_is_null(n1) == nodecl_is_null(n2))
        {
            if (!nodecl_is_null(n1))
            {
                if(skip_conversion_nodes)
                {
                    if(nodecl_get_kind(n1) == NODECL_CONVERSION)
                        return cmp_trees_rec(nodecl_get_child(n1, 0), n2, skip_conversion_nodes);
                    if(nodecl_get_kind(n2) == NODECL_CONVERSION)
                        return cmp_trees_rec(n1, nodecl_get_child(n2, 0), skip_conversion_nodes);
                }
                if (nodecl_get_kind(n1) == nodecl_get_kind(n2)) // kind
                {
                    if  (nodecl_get_symbol(n1) == nodecl_get_symbol(n2)) // symbol
                    {
                        if (nodecl_get_constant(n1) == nodecl_get_constant(n2)) // constant
                        {
                            // Everything looks equal in this single node, let's check our children
                            int equal = 0;
                            int i = 0;
                            while ((equal == 0)
                                && (i < MCXX_MAX_AST_CHILDREN))
                            {
                                equal = cmp_trees_rec(nodecl_get_child(n1, i), nodecl_get_child(n2, i),
                                                      skip_conversion_nodes);
                                i++;
                            }
                            return equal;
                        }
                        else if (nodecl_get_constant(n1) < nodecl_get_constant(n2)) // constant
                        {
                            return -1;
                        }
                        else // constant
                        {
                            return 1;
                        }
                    }
                    else if (nodecl_get_symbol(n1) < nodecl_get_symbol(n2)) // symbol
                    {
                        return -1;
                    }
                    else // symbol
                    {
                        return 1;
                    }
                }
                else if (nodecl_get_kind(n1) < nodecl_get_kind(n2)) // kind
                {
                    return -1;
                }
                else // kind
                {
                    return 1;
                }
            }
            else
            {
                return 0;
            }
        }
        else if (!nodecl_is_null(n1) && nodecl_is_null(n2))
        {
            return -1;
        }
        else
        {
            return 1;
        }
    }

    static bool equal_trees_rec(nodecl_t n1, nodecl_t n2, bool skip_conversion_nodes)
    {
        return (cmp_trees_rec(n1, n2, skip_conversion_nodes) == 0);
    }

    bool Utils::nodecl_is_arithmetic_op( Nodecl::NodeclBase n )
    {
        bool res = false;
        if ( n.is<Nodecl::Add>( ) || n.is<Nodecl::Minus>( )
            || n.is<Nodecl::Mul>( ) || n.is<Nodecl::Div>( )
            || n.is<Nodecl::Mod>( ) || n.is<Nodecl::Plus>( )
            || n.is<Nodecl::Preincrement>( ) || n.is<Nodecl::Postincrement>( )
            || n.is<Nodecl::Predecrement>( ) || n.is<Nodecl::Postdecrement>( )
            || Utils::nodecl_is_assignment_op( n ) || /* Fortran */ n.is<Nodecl::Power>( )
            || n.is<Nodecl::ArithmeticShr>( ) )
        {
            res = true;
        }
        return res;
    }

    bool Utils::nodecl_is_assignment_op ( Nodecl::NodeclBase n )
    {
        bool res = false;
        if ( n.is<Nodecl::Assignment>( ) || n.is<Nodecl::AddAssignment>( )
            || n.is<Nodecl::MinusAssignment>( ) || n.is<Nodecl::DivAssignment>( )
            || n.is<Nodecl::MulAssignment>( ) || n.is<Nodecl::ModAssignment>( )
            || n.is<Nodecl::ArithmeticShrAssignment>( ) || n.is<Nodecl::BitwiseShrAssignment>( )
            || n.is<Nodecl::BitwiseShlAssignment>( ) || n.is<Nodecl::BitwiseAndAssignment>( )
            || n.is<Nodecl::BitwiseOrAssignment>( ) || n.is<Nodecl::BitwiseXorAssignment>( ) )
        {
            res = true;
        }
        return res;
    }

    bool Utils::nodecl_is_bitwise_op( Nodecl::NodeclBase n )
    {
        bool res = false;
        if ( n.is<Nodecl::BitwiseAnd>( ) || n.is<Nodecl::BitwiseOr>( )
            || n.is<Nodecl::BitwiseXor>( ) || n.is<Nodecl::BitwiseNot>( )
            || n.is<Nodecl::BitwiseShr>( ) || n.is<Nodecl::BitwiseShl>( ))
        {
            res = true;
        }
        return res;
    }

    bool Utils::nodecl_is_comparison_op( Nodecl::NodeclBase n )
    {
        bool res = false;
        if ( n.is<Nodecl::Equal>( ) || n.is<Nodecl::Different>( )
            || n.is<Nodecl::LowerThan>( ) || n.is<Nodecl::GreaterThan>( )
            || n.is<Nodecl::LowerOrEqualThan>( ) || n.is<Nodecl::GreaterOrEqualThan>( ) )
        {
            res = true;
        }
        return res;
    }

    bool Utils::nodecl_is_literal( Nodecl::NodeclBase n )
    {
        bool res = false;
        if (n.is<Nodecl::BooleanLiteral>( ) || n.is<ComplexLiteral>() ||
            n.is<IntegerLiteral>( ) || n.is<FloatingLiteral>( ) || n.is<StringLiteral>( ))
        {
            res = true;
        }
        return res;
    }

    bool Utils::nodecl_is_logical_op( Nodecl::NodeclBase n )
    {
        bool res = false;
        if ( n.is<Nodecl::LogicalAnd>( ) || n.is<Nodecl::LogicalOr>( )
            || n.is<Nodecl::LogicalNot>( ) )
        {
            res = true;
        }
        return res;
    }

    bool Utils::nodecl_is_modifiable_lvalue( Nodecl::NodeclBase n )
    {
        return n.get_type().is_lvalue_reference( );
    }

    bool Utils::nodecl_contains_nodecl( Nodecl::NodeclBase container, Nodecl::NodeclBase contained )
    {
        bool result = false;

        if( Nodecl::Utils::equal_nodecls( container, contained ) )
        {
            result = true;
        }
        else if( container.is<Nodecl::Conversion>( ) )
        {
            result = nodecl_contains_nodecl( container.as<Nodecl::Conversion>( ).get_nest( ), contained );
        }
        else if( contained.is<Nodecl::Conversion>( ) )
        {
            result = nodecl_contains_nodecl( container, contained.as<Nodecl::Conversion>( ).get_nest( ) );
        }
        else if( container.is<Nodecl::Dereference>( ) )
        {
            if( contained.is<Nodecl::ArraySubscript>( ) )
            {
                Nodecl::NodeclBase container_rhs = container.as<Nodecl::Dereference>( ).get_rhs( );
                Nodecl::ArraySubscript contained_array = contained.as<Nodecl::ArraySubscript>( );
                Nodecl::NodeclBase contained_subscripted = contained_array.get_subscripted( );
                if( Nodecl::Utils::equal_nodecls( container_rhs, contained_subscripted ) )
                {
                    Nodecl::List contained_subscripts = contained_array.get_subscripts( ).as<Nodecl::List>( );
                    if( ( contained_subscripts.size( ) == 1 ) &&
                        contained_subscripts[0].is_constant( ) &&
                        const_value_is_zero( contained_subscripts[0].get_constant( ) ) )
                    {   // container: *array, contained: array[0]
                        result = true;
                    }
                }
            }
        }
        else if( container.is<Nodecl::ArraySubscript>( ) )
        {
            if( contained.is<Nodecl::ArraySubscript>( ) )
            {   // Check the positions of the array that are accessed
                Nodecl::ArraySubscript container_array = container.as<Nodecl::ArraySubscript>( );
                Nodecl::ArraySubscript contained_array = contained.as<Nodecl::ArraySubscript>( );
                if( equal_nodecls( container_array.get_subscripted( ), contained_array.get_subscripted( ) ) )
                {
                    Nodecl::List container_subscripts = container_array.get_subscripts( ).as<Nodecl::List>( );
                    Nodecl::List contained_subscripts = contained_array.get_subscripts( ).as<Nodecl::List>( );
                    Nodecl::List::iterator it1 = container_subscripts.begin( );
                    Nodecl::List::iterator it2 = contained_subscripts.begin( );
                    for( ; it1 != container_subscripts.end( ) && it2 != contained_subscripts.end( ) && !result; ++it1, ++it2 )
                    {
                        result = nodecl_contains_nodecl( *it1, *it2 );
                    }
                }
            }
        }
        else if( container.is<Nodecl::ClassMemberAccess>( ) )
        {
            Nodecl::NodeclBase lhs = contained.as<Nodecl::ClassMemberAccess>( ).get_lhs( );
            result = nodecl_contains_nodecl( container, lhs );
        }
        else if( container.is<Nodecl::Symbol>( ) )
        {
            if( contained.is<Nodecl::Reference>( ) &&
                contained.as<Nodecl::Reference>( ).get_rhs( ).is<ArraySubscript>( ) )
            {
                Nodecl::List contained_subscripts = contained.as<Nodecl::Reference>( ).get_rhs( ).as<ArraySubscript>( ).get_subscripts( ).as<Nodecl::List>( );
                if( ( contained_subscripts.size( ) == 1 ) &&
                    contained_subscripts[0].is_constant( ) &&
                    const_value_is_zero( contained_subscripts[0].get_constant( ) ) )
                {   // container: array, contained: &array[0]
                    result = true;
                }
            }
            else if( contained.is<Nodecl::ClassMemberAccess>( ) )
            {
                result = nodecl_contains_nodecl( container, contained.as<Nodecl::ClassMemberAccess>( ).get_lhs( ) );
            }
        }


        return result;
    }

    bool Utils::nodecl_is_in_nodecl_list( Nodecl::NodeclBase n, Nodecl::List l )
    {
        bool res = false;
        if( n.is<Nodecl::List>( ) )
        {
            ERROR_CONDITION( !n.is<List>( ), "Can't found a list found in a list", 0 );
        }
        else
        {
            for( Nodecl::List::iterator it = l.begin( ); it != l.end( ); ++it )
            {
                if( equal_nodecls( n, *it ) )
                {
                    res = true;
                    break;
                }
            }
        }
        return res;
    }

    bool Utils::equal_nodecls(Nodecl::NodeclBase n1, Nodecl::NodeclBase n2, bool skip_conversion_nodes)
    {
        nodecl_t n1_ = n1.get_internal_nodecl();
        nodecl_t n2_ = n2.get_internal_nodecl();

        if (nodecl_is_list(n1_) || nodecl_is_list(n2_))
        {
            std::cerr << "warning: method 'equal_nodecls' is implemented to compare nodecls containing trees with "
                      << " no lists inside. The method returns false but they can be the same tree" << std::endl;
            return false;
        }

        bool equals = equal_trees_rec(n1_, n2_, skip_conversion_nodes);
        return equals;
    }

    int Utils::cmp_nodecls(Nodecl::NodeclBase n1, Nodecl::NodeclBase n2, bool skip_conversion_nodes)
    {
        nodecl_t n1_ = n1.get_internal_nodecl();
        nodecl_t n2_ = n2.get_internal_nodecl();

        return cmp_trees_rec(n1_, n2_, skip_conversion_nodes);
    }

    size_t Utils::Nodecl_hash::operator() (const Nodecl::NodeclBase& n) const
    {
        return nodecl_hash_table(n.get_internal_nodecl());
    }

    bool Utils::Nodecl_comp::operator() (const Nodecl::NodeclBase& n1, const Nodecl::NodeclBase& n2) const
    {
        return equal_nodecls(n1, n2);
    }

    Utils::ReduceExpressionVisitor::ReduceExpressionVisitor( )
        : _calc( )
    {}

    void Utils::ReduceExpressionVisitor::visit_post( const Nodecl::Add& n )
    {
        NodeclBase lhs = n.get_lhs( );
        NodeclBase rhs = n.get_rhs( );
        if( lhs.is_constant( ) && const_value_is_zero( lhs.get_constant( ) ) )
        {   // 0 + t = t
            replace( n, rhs );
        }
        else if( rhs.is_constant( ) && const_value_is_zero( rhs.get_constant( ) ) )
        {   // t + 0 = t
            replace( n, lhs );
        }
        else if( lhs.is_constant( ) && rhs.is_constant( ) )
        {   // R1
            const_value_t* const_value = _calc.compute_const_value( n );
            Nodecl::NodeclBase new_n = const_value_to_nodecl(const_value);
            replace( n, new_n );
        }
        else if( rhs.is_constant( ) )
        {
            if( lhs.is<Add>( ) )
            {   // R6
                Add lhs_add = lhs.as<Add>( );
                NodeclBase lhs_lhs = lhs_add.get_lhs( );
                NodeclBase lhs_rhs = lhs_add.get_rhs( );
                if( lhs_lhs.is_constant( ) )
                {
                    NodeclBase c = Add::make( lhs_lhs, rhs, rhs.get_type( ) );
                    const_value_t* c_value = _calc.compute_const_value( c );
                    if( !const_value_is_zero( c_value ))
                    {
                        replace( n, Add::make( const_value_to_nodecl(c_value), lhs_rhs,
                                               rhs.get_type( ), n.get_locus( ) ) );
                    }
                    else
                    {
                        replace( n, lhs_rhs );
                    }
                }
            }
            else
            {   // R2
                replace( n, Add::make( rhs, lhs, lhs.get_type( ), n.get_locus( ) ) );
            }
        }
    }

    void Utils::ReduceExpressionVisitor::visit_post( const Nodecl::Div& n )
    {   // R10
        NodeclBase lhs = n.get_lhs();
        NodeclBase rhs = n.get_rhs();
        if( lhs.is_constant( ) && rhs.is_constant( ) &&
            const_value_is_zero( lhs.get_constant( ) ) && !const_value_is_zero( rhs.get_constant( ) ) )
        {
            replace( n, const_value_to_nodecl( const_value_get_zero( /*num_bytes*/ 4, /*sign*/1 ) ) );
        }
    }

    void Utils::ReduceExpressionVisitor::visit_post( const Nodecl::LowerOrEqualThan& n )
    {
        NodeclBase lhs = n.get_lhs();
        NodeclBase rhs = n.get_rhs();
        if( rhs.is_constant( ) )
        {
            if( lhs.is<Add>( ) )
            {   // R20
            Add lhs_add = lhs.as<Add>( );
            NodeclBase lhs_lhs = lhs_add.get_lhs( );
            NodeclBase lhs_rhs = lhs_add.get_rhs( );
            if( lhs_lhs.is_constant( ) )
            {
                NodeclBase c = Minus::make( rhs, lhs_lhs, rhs.get_type( ) );
                const_value_t* c_value = _calc.compute_const_value( c );
                replace( n, LowerOrEqualThan::make( lhs_rhs, const_value_to_nodecl( c_value ),
                                                    rhs.get_type( ), n.get_locus( ) ) );
            }
            }
        }
    }

    void Utils::ReduceExpressionVisitor::visit_post( const Nodecl::Minus& n )
    {
        NodeclBase lhs = n.get_lhs();
        NodeclBase rhs = n.get_rhs();
        if( lhs.is_constant( ) && rhs.is_constant( ) )
        {   // R3
            const_value_t* c_value = _calc.compute_const_value( n );
            replace( n, const_value_to_nodecl( c_value ) );
        }
        else if( rhs.is_constant( ) )
        {   // R4
            if( const_value_is_zero( rhs.get_constant( ) ) )
            {
                replace( n, lhs );
            }
            else
            {
                NodeclBase neg_rhs = Neg::make( rhs, rhs.get_type( ) );
                replace( n, Add::make( neg_rhs, lhs,
                                       lhs.get_type( ), n.get_locus( ) ) );
            }
        }
        else if( equal_nodecls( lhs, rhs ) )
        {
            replace( n, const_value_to_nodecl( const_value_get_zero( /*num_bytes*/ 4, /*sign*/1 ) ) );
        }
    }

    void Utils::ReduceExpressionVisitor::visit_post( const Nodecl::Mod& n )
    {
        NodeclBase lhs = n.get_lhs();
        NodeclBase rhs = n.get_rhs();
        if( ( rhs.is_constant() && lhs.is_constant() && const_value_is_one( lhs.get_constant( ) ) )
            || equal_nodecls( lhs, rhs ) )
        {   // R11
            replace( n, const_value_to_nodecl( const_value_get_zero( /*num_bytes*/ 4, /*sign*/1 ) ) );
        }
    }

    void Utils::ReduceExpressionVisitor::visit_post( const Nodecl::Mul& n )
    {
        NodeclBase lhs = n.get_lhs();
        NodeclBase rhs = n.get_rhs();
        if( ( lhs.is_constant( ) && const_value_is_zero( lhs.get_constant( ) ) )
            || ( rhs.is_constant( ) && const_value_is_zero( rhs.get_constant( ) ) ) )
        {   // 0 * t = t , t * 0 = t
            replace( n, const_value_to_nodecl( const_value_get_zero( /*num_bytes*/ 4, /*sign*/1 ) ) );
        }
        else if( lhs.is_constant( ) && rhs.is_constant( ) )
        {   // R7
            const_value_t* c_value = _calc.compute_const_value( n );
            replace( n, const_value_to_nodecl( c_value ) );
        }
        else if ( rhs.is_constant( ) )
        {
            if( const_value_is_zero( rhs.get_constant( ) ) )
            {
                replace( n, const_value_to_nodecl( const_value_get_zero( /*num_bytes*/ 4, /*sign*/1 ) ) );
            }
            else
            {
                if( lhs.is<Mul>( ) )
                {   // R9
                    Nodecl::Mul lhs_mul = lhs.as<Nodecl::Mul>( );
                    NodeclBase lhs_lhs = lhs_mul.get_lhs();
                    NodeclBase lhs_rhs = lhs_mul.get_rhs();
                    if( lhs_lhs.is_constant( ) )
                    {
                        if( const_value_is_zero( lhs_lhs.get_constant( ) ) )
                        {
                            replace( n, const_value_to_nodecl( const_value_get_zero( /*num_bytes*/ 4, /*sign*/1 ) ) );
                        }
                        else
                        {
                            NodeclBase c = Mul::make( lhs_lhs, rhs, rhs.get_type() );
                            const_value_t* c_value = _calc.compute_const_value( c );
                            replace( n, Mul::make( const_value_to_nodecl( c_value ), lhs_rhs,
                                                   rhs.get_type( ), n.get_locus( ) ) );
                        }
                    }
                }
                else
                {   // R8
                    replace( n, Mul::make( rhs, lhs, lhs.get_type( ), n.get_locus( ) ) );
                }
            }
        }
    }

    Nodecl::List Utils::get_all_list_from_list_node(Nodecl::List n)
    {
        while (n.get_parent().is<Nodecl::List>())
        {
            n = n.get_parent().as<Nodecl::List>();
        }

        return n;
    }

    void Utils::remove_from_enclosing_list(Nodecl::NodeclBase n)
    {
        Nodecl::NodeclBase parent = n.get_parent();

        if (!parent.is<Nodecl::List>())
            return;

        Nodecl::List l = Utils::get_all_list_from_list_node(parent.as<Nodecl::List>());

        Nodecl::List::iterator it = std::find(l.begin(), l.end(), n);

        if (it != l.end())
        {
            l.erase(it);
        }
    }

    TL::Symbol Utils::get_enclosing_function(Nodecl::NodeclBase n)
    {
        TL::Symbol result;
        TL::Scope sc = n.retrieve_context();

        decl_context_t decl_context = sc.get_decl_context();

        if (decl_context.block_scope != NULL)
        {
            result = decl_context.block_scope->related_entry;
        }
        else if (decl_context.function_scope != NULL)
        {
            result = decl_context.function_scope->related_entry;
        }

        return result;
    }

    void Utils::append_to_top_level_nodecl(Nodecl::NodeclBase n)
    {
        Nodecl::TopLevel top_level = Nodecl::NodeclBase(CURRENT_COMPILED_FILE->nodecl).as<Nodecl::TopLevel>();
        Nodecl::List list = top_level.get_top_level().as<Nodecl::List>();
        list.append(n);
    }

    namespace
    {
        void simple_replace(Nodecl::NodeclBase dest, Nodecl::NodeclBase src)
        {
            // Simple case
            Nodecl::NodeclBase nodecl_original_parent = dest.get_parent();
            ::nodecl_replace(dest.get_internal_nodecl(), src.get_internal_nodecl());

            // Reparent new children of dest
            ::nodecl_set_parent(dest.get_internal_nodecl(), nodecl_original_parent.get_internal_nodecl());
            for (int i = 0; i < MCXX_MAX_AST_CHILDREN; i++)
            {
                nodecl_t child = nodecl_get_child(dest.get_internal_nodecl(), i);
                if (!nodecl_is_null(child))
                {
                    ::nodecl_set_parent(nodecl_get_child(dest.get_internal_nodecl(), i), dest.get_internal_nodecl());
                }
            }
        }
    }

    void Utils::replace(Nodecl::NodeclBase dest, Nodecl::NodeclBase src)
    {
        ERROR_CONDITION(src.is_null(), "Invalid node", 0);

        if (src.is<Nodecl::List>()
                && !dest.is<Nodecl::List>())
        {
            Nodecl::List new_list = src.as<Nodecl::List>();
            List::iterator new_list_it = new_list.begin();

            if (new_list.size() == 1)
            {
                simple_replace(dest, *new_list_it);
            }
            else
            {
                ERROR_CONDITION(!dest.is_in_list(), "Cannot replace a non-list node by a list if the first is not inside a list", 0);

                simple_replace(dest, *new_list_it);
                new_list_it++;

                Nodecl::List parent_list = dest.get_parent().as<Nodecl::List>();
                Nodecl::List::iterator last_it = parent_list.last();

                for (; new_list_it != new_list.end(); new_list_it++)
                {
                    parent_list.insert(last_it + 1, *new_list_it);
                    // We may have a new last node now
                    last_it = new_list_it->get_parent().as<Nodecl::List>().last();
                }
            }
        }
        else
        {
            simple_replace(dest, src);
        }
    }

    bool Utils::is_in_list(Nodecl::NodeclBase n)
    {
        return (!n.get_parent().is_null()
                && n.get_parent().is<Nodecl::List>());
    }

    void Utils::prepend_items_before(Nodecl::NodeclBase n, Nodecl::NodeclBase items)
    {
        ERROR_CONDITION(!Utils::is_in_list(n), "Node is not in a list", 0);

        if (!items.is<Nodecl::List>())
        {
            items = Nodecl::List::make(items);
        }

        Nodecl::List list_items = items.as<Nodecl::List>();

        Nodecl::List list = n.get_parent().as<Nodecl::List>();
        Nodecl::List::iterator last_it = list.last();

        for (Nodecl::List::iterator it = list_items.begin();
                it != list_items.end();
                it++)
        {
            list.insert(last_it, *it);
            // We may have a new last node now
            last_it = list.last();
        }
    }

    void Utils::append_items_after(Nodecl::NodeclBase n, Nodecl::NodeclBase items)
    {
        ERROR_CONDITION(!Utils::is_in_list(n), "Node is not in a list", 0);

        if (!items.is<Nodecl::List>())
        {
            items = Nodecl::List::make(items);
        }

        Nodecl::List list_items = items.as<Nodecl::List>();

        Nodecl::List list = n.get_parent().as<Nodecl::List>();
        Nodecl::List::iterator last_it = list.last();

        for (Nodecl::List::iterator it = list_items.begin();
                it != list_items.end();
                it++)
        {
            list.insert(last_it + 1, *it);
            // We may have a new last node now
            last_it = it->get_parent().as<Nodecl::List>().last();
        }
    }

    void Utils::prepend_to_top_level_nodecl(Nodecl::NodeclBase n)
    {
        Nodecl::TopLevel top_level = Nodecl::NodeclBase(CURRENT_COMPILED_FILE->nodecl).as<Nodecl::TopLevel>();
        Nodecl::List list = top_level.get_top_level().as<Nodecl::List>();
        list.prepend(n);
    }

    Nodecl::NodeclBase Utils::advance_conversions(Nodecl::NodeclBase n)
    {
        while (n.is<Nodecl::Conversion>())
        {
            n = n.as<Nodecl::Conversion>().get_nest();
        }
        return n;
    }

    std::string Utils::get_elemental_operator_of_binary_expression(Nodecl::NodeclBase n)
    {
        return get_elemental_operator_of_binary_expression(n.get_kind());
    }

    std::string Utils::get_elemental_operator_of_binary_expression(node_t n)
    {
        switch (n)
        {
            case NODECL_ADD:
            case NODECL_ADD_ASSIGNMENT:
                return "+";
            case NODECL_MINUS:
            case NODECL_MINUS_ASSIGNMENT:
                return "-";
            case NODECL_MUL:
            case NODECL_MUL_ASSIGNMENT:
                return "*";
            case NODECL_DIV:
            case NODECL_DIV_ASSIGNMENT:
                return "/";
            case NODECL_MOD:
            case NODECL_MOD_ASSIGNMENT:
                return "%";
            case NODECL_BITWISE_SHL:
            case NODECL_BITWISE_SHL_ASSIGNMENT:
                return "<<";
            case NODECL_BITWISE_SHR:
            case NODECL_BITWISE_SHR_ASSIGNMENT:
            case NODECL_ARITHMETIC_SHR:
            case NODECL_ARITHMETIC_SHR_ASSIGNMENT:
                return ">>";
            case NODECL_BITWISE_AND:
            case NODECL_BITWISE_AND_ASSIGNMENT:
                return "&";
            case NODECL_BITWISE_OR:
            case NODECL_BITWISE_OR_ASSIGNMENT:
                return "|";
            case NODECL_BITWISE_XOR:
            case NODECL_BITWISE_XOR_ASSIGNMENT:
                return "^";
            default:
                return "";
        }
    }

    Nodecl::NodeclBase Utils::deep_copy(Nodecl::NodeclBase orig, TL::ReferenceScope ref_scope, Utils::SymbolMap& map)
    {
        Nodecl::NodeclBase result;

        result = ::nodecl_deep_copy(orig.get_internal_nodecl(),
                ref_scope.get_scope().get_decl_context(),
                map.get_symbol_map());

        return result;
    }

    Nodecl::NodeclBase Utils::deep_copy(Nodecl::NodeclBase orig, TL::ReferenceScope ref_scope)
    {
        Utils::SimpleSymbolMap empty_map;
        return deep_copy(orig, ref_scope, empty_map);
    }

    namespace
    {
        bool is_in_top_level_list(Nodecl::NodeclBase list)
        {
            ERROR_CONDITION(!list.is<Nodecl::List>(), "Must be a list", 0);
            list = Nodecl::Utils::get_all_list_from_list_node(list.as<Nodecl::List>());

            Nodecl::TopLevel top_level = Nodecl::NodeclBase(CURRENT_COMPILED_FILE->nodecl).as<Nodecl::TopLevel>();
            Nodecl::List top_level_list = top_level.get_top_level().as<Nodecl::List>();

            return (list == top_level_list);
        }
    }

    void Utils::prepend_to_enclosing_top_level_location(Nodecl::NodeclBase current_location, Nodecl::NodeclBase items)
    {
        while (!current_location.is_null()
                && (!current_location.is<Nodecl::List>()
                || !is_in_top_level_list(current_location)))
        {
            current_location = current_location.get_parent();
        }

        if (!items.is<Nodecl::List>())
        {
            items = Nodecl::List::make(items);
        }
        Nodecl::List list_items = items.as<Nodecl::List>();

        ERROR_CONDITION(current_location.is_null(), "This should never be null", 0);
        ERROR_CONDITION(!current_location.is<Nodecl::List>(), "Thist must be a list", 0);

        // This is a list node inside the top level list
        Nodecl::List list = current_location.as<Nodecl::List>();

        Nodecl::List::iterator last_it = list.last();
        for (Nodecl::List::iterator it = list_items.begin();
                it != list_items.end();
                it++)
        {
            list.insert(last_it, *it);
            // We may have a new last node now
            last_it = it->get_parent().as<Nodecl::List>().last();
        }
    }

    void Utils::append_to_enclosing_top_level_location(Nodecl::NodeclBase current_location, Nodecl::NodeclBase items)
    {
        while (!current_location.is_null()
                && (!current_location.is<Nodecl::List>()
                || !is_in_top_level_list(current_location)))
        {
            current_location = current_location.get_parent();
        }

        if (!items.is<Nodecl::List>())
        {
            items = Nodecl::List::make(items);
        }
        Nodecl::List list_items = items.as<Nodecl::List>();

        ERROR_CONDITION(current_location.is_null(), "This should never be null", 0);
        ERROR_CONDITION(!current_location.is<Nodecl::List>(), "Thist must be a list", 0);

        // This is a list node inside the top level list
        Nodecl::List list = current_location.as<Nodecl::List>();

        Nodecl::List::iterator last_it = list.last();
        for (Nodecl::List::iterator it = list_items.begin();
                it != list_items.end();
                it++)
        {
            list.insert(last_it + 1, *it);
            // We may have a new last node now
            last_it = it->get_parent().as<Nodecl::List>().last();
        }
    }

    TL::ObjectList<Nodecl::NodeclBase> Utils::get_declarations_of_entity_at_top_level(TL::Symbol symbol)
    {
        TL::ObjectList<Nodecl::NodeclBase> result;
        Nodecl::TopLevel top_level = Nodecl::NodeclBase(CURRENT_COMPILED_FILE->nodecl).as<Nodecl::TopLevel>();
        Nodecl::List list = top_level.get_top_level().as<Nodecl::List>();
        for (Nodecl::List::iterator it = list.begin();
                it != list.end();
                it++)
        {
            if (it->is<Nodecl::CxxDecl>()
                    && it->get_symbol() == symbol)
            {
                result.append(*it);
            }
        }
        return result;
    }

    TL::ObjectList<Nodecl::NodeclBase> Utils::get_definitions_of_entity_at_top_level(TL::Symbol symbol)
    {
        TL::ObjectList<Nodecl::NodeclBase> result;
        Nodecl::TopLevel top_level = Nodecl::NodeclBase(CURRENT_COMPILED_FILE->nodecl).as<Nodecl::TopLevel>();
        Nodecl::List list = top_level.get_top_level().as<Nodecl::List>();
        for (Nodecl::List::iterator it = list.begin();
                it != list.end();
                it++)
        {
            if (it->is<Nodecl::CxxDef>()
                    && it->get_symbol() == symbol)
            {
                result.append(*it);
            }
        }
        return result;
    }

    TL::ObjectList<Nodecl::NodeclBase> Utils::get_declarations_or_definitions_of_entity_at_top_level(TL::Symbol symbol)
    {
        TL::ObjectList<Nodecl::NodeclBase> result;
        Nodecl::TopLevel top_level = Nodecl::NodeclBase(CURRENT_COMPILED_FILE->nodecl).as<Nodecl::TopLevel>();
        Nodecl::List list = top_level.get_top_level().as<Nodecl::List>();
        for (Nodecl::List::iterator it = list.begin();
                it != list.end();
                it++)
        {
            if ((it->is<Nodecl::CxxDef>()
                        || it->is<Nodecl::CxxDecl>())
                    && it->get_symbol() == symbol)
            {
                result.append(*it);
            }
        }
        return result;
    }

    struct LabelVisitor : ExhaustiveVisitor<void>
    {
        Utils::SimpleSymbolMap &_symbol_map;
        TL::Scope _sc;
        LabelVisitor(Utils::SimpleSymbolMap& symbol_map, TL::ReferenceScope ref_scope)
            : _symbol_map(symbol_map), _sc(ref_scope.get_scope()) { }

        void insert_new_label_symbol(TL::Symbol sym)
        {
            TL::Counter &counter = TL::CounterManager::get_counter("label_visitor");

            std::string register_name, symbol_name;
            if (IS_FORTRAN_LANGUAGE)
            {
                std::string label_name = sym.get_name();

                for (std::string::iterator it = label_name.begin(); it != label_name.end(); it++)
                {
                    // If this is not a numeric label give up
                    if (!(('0' <= (*it)) &&
                                ((*it) <= '9')))
                    {
                        std::cerr << "NOT A NUMERIC LABEL! |" << label_name << "|" << std::endl;
                        return;
                    }
                }

                int x;
                {
                    std::stringstream ss;
                    ss.str(label_name);

                    ss >> x;
                }

                // FIXME - Make this more robust!
                // Add 10000 to this label
                x += 10000 + (int)counter;

                std::stringstream ss;
                ss << x;

                symbol_name = ss.str();
                register_name = ".label_" + symbol_name;
            }
            else
            {
                std::stringstream ss;
                ss << symbol_name << "_" << (int)counter;

                symbol_name = ss.str();
                register_name = symbol_name;
            }
            counter++;

            decl_context_t decl_context = _sc.get_decl_context();
            scope_entry_t * new_label = ::new_symbol(decl_context, decl_context.function_scope, uniquestr(register_name.c_str()));
            new_label->symbol_name = uniquestr(symbol_name.c_str());
            new_label->kind = SK_LABEL;
            new_label->value = nodecl_shallow_copy(sym.get_value().get_internal_nodecl());

            _symbol_map.add_map(sym, new_label);
        }

        void visit(const Nodecl::Symbol &node)
        {
            TL::Symbol sym = node.get_symbol();
            // FORMAT references
            if (IS_FORTRAN_LANGUAGE
                    && sym.is_label()
                    && !sym.get_value().is_null())
            {
                insert_new_label_symbol(sym);
            }
        }

        void visit(const Nodecl::LabeledStatement& stmt)
        {
            walk(stmt.get_statement());

            TL::Symbol sym = stmt.get_symbol();
            if (sym.is_label())
            {
                insert_new_label_symbol(sym);
            }
        }
    };

    Utils::LabelSymbolMap::LabelSymbolMap(
            Utils::SymbolMap* original_symbol_map,
            Nodecl::NodeclBase code,
            TL::ReferenceScope ref_scope)
        : _orig_symbol_map(original_symbol_map)
    {
        LabelVisitor visitor(_current_map, ref_scope);
        visitor.walk(code);
    }

    void Utils::update_symbols(Nodecl::NodeclBase node, SymbolMap& m)
    {
        if (node.is_null())
            return;

        TL::Symbol sym = node.get_symbol();
        if (!sym.is_valid())
            return;

        node.set_symbol(m.map(sym));

        TL::ObjectList<Nodecl::NodeclBase> children = node.children();
        for (TL::ObjectList<Nodecl::NodeclBase>::iterator it = children.begin();
                it != children.end();
                it++)
        {
            update_symbols(*it, m);
        }


    }
}

namespace TL
{
    // This is actually what OpenMP expects
    // Lower bound and upper bound are closed ranges:
    //      [lower_bound, upper_bound] if step is positive
    //      [upper_bound, lower_bound] if step is negative
    void ForStatement::analyze_loop_header()
    {
        Nodecl::NodeclBase lc = this->get_loop_header();
        if (lc.is<Nodecl::RangeLoopControl>())
        {
            // This is trivially true for ranged loops
            Nodecl::RangeLoopControl loop_control = lc.as<Nodecl::RangeLoopControl>();

            // Empty loops are obviously not allowed
            if (loop_control.get_lower().is_null())
            {
                _is_omp_valid = false;
                return;
            }

            _induction_var = loop_control.get_induction_variable();
            _lower_bound = loop_control.get_lower().shallow_copy();
            _upper_bound = loop_control.get_upper().shallow_copy();
            _step = loop_control.get_step().shallow_copy();

            _is_omp_valid = true;
        }
        else if (lc.is<Nodecl::LoopControl>())
        {
            Nodecl::LoopControl loop_control = lc.as<Nodecl::LoopControl>();
            Nodecl::List init_expr_list = loop_control.get_init().as<Nodecl::List>();
            Nodecl::NodeclBase test_expr = loop_control.get_cond();
            Nodecl::NodeclBase incr_expr = loop_control.get_next();

            // init-expr must have the following form
            //
            //   _induction_var = lb
            //   integer-type   _induction_var = lb
            //   random-access-iterator _induction_var = lb    // CURRENTLY NOT SUPPORTED
            //   pointer-type _induction_var = lb

            _induction_var = Nodecl::NodeclBase::null();

            _induction_variable_in_separate_scope = false;

            if (init_expr_list.size() != 1)
            {
                _is_omp_valid = false;
                return;
            }

            Nodecl::NodeclBase init_expr = init_expr_list.front();

            // _induction_var = lb
            if (init_expr.is<Nodecl::Assignment>())
            {
                Nodecl::NodeclBase lhs = init_expr.as<Nodecl::Assignment>().get_lhs();
                if (lhs.is<Nodecl::Symbol>())
                {
                    _induction_var = lhs;
                }

                Nodecl::NodeclBase rhs = init_expr.as<Nodecl::Assignment>().get_rhs();
                _lower_bound = rhs.shallow_copy();
            }
            // T _induction_var = lb
            else if (init_expr.is<Nodecl::ObjectInit>())
            {
                _induction_variable_in_separate_scope = true;
                _induction_var = init_expr;

                _lower_bound = _induction_var.get_symbol().get_value().shallow_copy();
            }
            else
            {
                _is_omp_valid = false;
                return;
            }

            if (_induction_var.is_null())
            {
                _is_omp_valid = false;
                return;
            }

            // test-expr must be
            //
            // _induction_var relational-op b
            // b relational-op _induction_var
            if ((test_expr.is<Nodecl::LowerThan>()
                        || test_expr.is<Nodecl::LowerOrEqualThan>()
                        || test_expr.is<Nodecl::GreaterThan>()
                        || test_expr.is<Nodecl::GreaterOrEqualThan>())
                    && (Nodecl::Utils::advance_conversions(test_expr.as<Nodecl::LowerThan>().get_lhs()).get_symbol()
                        == _induction_var.get_symbol()
                        || Nodecl::Utils::advance_conversions(test_expr.as<Nodecl::LowerThan>().get_rhs()).get_symbol()
                        == _induction_var.get_symbol()))

            {
                Nodecl::NodeclBase lhs = test_expr.as<Nodecl::LowerThan>().get_lhs();
                Nodecl::NodeclBase rhs = test_expr.as<Nodecl::LowerThan>().get_rhs();

                bool lhs_is_var = (Nodecl::Utils::advance_conversions(lhs).get_symbol() == _induction_var.get_symbol());

                if (test_expr.is<Nodecl::LowerThan>())
                {
                    if (lhs_is_var)
                    {
                        // x < E  this is like x <= (E - 1)
                        TL::Type t = lhs.get_type();

                        if (t.is_any_reference())
                            t = t.references_to();

                        if (rhs.is_constant())
                        {
                            _upper_bound = const_value_to_nodecl(
                                    const_value_sub(
                                        rhs.get_constant(),
                                        const_value_get_one(4, 1)));
                        }
                        else
                        {
                            _upper_bound = Nodecl::Minus::make(
                                    rhs.shallow_copy(),
                                    const_value_to_nodecl(const_value_get_one(4, 1)),
                                    t,
                                    rhs.get_locus());
                        }
                    }
                    else
                    {
                        // E < x this is like x > E this is like x >= E + 1
                        TL::Type t = lhs.get_type();

                        if (t.is_any_reference())
                            t = t.references_to();

                        if (lhs.is_constant())
                        {
                            _upper_bound = const_value_to_nodecl(
                                    const_value_add(
                                        lhs.get_constant(),
                                        const_value_get_one(4, 1)));
                        }
                        else
                        {
                            _upper_bound = Nodecl::Add::make(
                                    lhs.shallow_copy(),
                                    const_value_to_nodecl(const_value_get_one(4, 1)),
                                    t,
                                    lhs.get_locus());
                        }
                    }
                }
                else if (test_expr.is<Nodecl::LowerOrEqualThan>())
                {
                    if (lhs_is_var)
                    {
                        // x <= E
                        _upper_bound = rhs.shallow_copy();
                    }
                    else
                    {
                        // E <= x this is like x >= E
                        _upper_bound = lhs.shallow_copy();
                    }
                }
                else if (test_expr.is<Nodecl::GreaterThan>())
                {
                    if (lhs_is_var)
                    {
                        // x > E, this is like x >= E + 1
                        TL::Type t = rhs.get_type();

                        if (t.is_any_reference())
                            t = t.references_to();


                        if (rhs.is_constant())
                        {
                            _upper_bound = const_value_to_nodecl(
                                    const_value_add(
                                        rhs.get_constant(),
                                        const_value_get_one(4, 1)));
                        }
                        else
                        {
                            _upper_bound = Nodecl::Add::make(
                                    rhs.shallow_copy(),
                                    const_value_to_nodecl(const_value_get_one(4, 1)),
                                    t,
                                    rhs.get_locus());
                        }
                    }
                    else
                    {
                        // E > x this is like x < E, this is like x <= E - 1
                        TL::Type t = lhs.get_type();

                        if (t.is_any_reference())
                            t = t.references_to();

                        if (lhs.is_constant())
                        {
                            _upper_bound = const_value_to_nodecl(
                                    const_value_sub(
                                        lhs.get_constant(),
                                        const_value_get_one(4, 1)));
                        }
                        else
                        {
                            _upper_bound = Nodecl::Minus::make(
                                    lhs.shallow_copy(),
                                    const_value_to_nodecl(const_value_get_one(4, 1)),
                                    t,
                                    lhs.get_locus());
                        }
                    }
                }
                else if (test_expr.is<Nodecl::GreaterOrEqualThan>())
                {
                    if (lhs_is_var)
                    {
                        // x >= E
                        _upper_bound = rhs.shallow_copy();
                    }
                    else
                    {
                        // E >= x this is like x <= E
                        _upper_bound = lhs.shallow_copy();
                    }
                }
                else
                {
                    internal_error("Code unreachable", 0);
                }
            }
            else
            {
                _is_omp_valid = false;
                return;
            }

            // incr-expr must have the following form
            // ++_induction_var
            if (incr_expr.is<Nodecl::Preincrement>()
                    && incr_expr.as<Nodecl::Preincrement>().get_rhs().get_symbol() == _induction_var.get_symbol())
            {
                _step = const_value_to_nodecl(const_value_get_one(4, 1));
            }
            // _induction_var++
            else if (incr_expr.is<Nodecl::Postincrement>()
                    && incr_expr.as<Nodecl::Postincrement>().get_rhs().get_symbol() == _induction_var.get_symbol())
            {
                _step = const_value_to_nodecl(const_value_get_one(4, 1));
            }
            // --_induction_var
            else if (incr_expr.is<Nodecl::Predecrement>()
                    && incr_expr.as<Nodecl::Predecrement>().get_rhs().get_symbol() == _induction_var.get_symbol())
            {
                _step = const_value_to_nodecl(const_value_get_minus_one(4, 1));
            }
            // _induction_var--
            else if (incr_expr.is<Nodecl::Postdecrement>()
                    && incr_expr.as<Nodecl::Postdecrement>().get_rhs().get_symbol() == _induction_var.get_symbol())
            {
                _step = const_value_to_nodecl(const_value_get_minus_one(4, 1));
            }
            // _induction_var += incr
            else if (incr_expr.is<Nodecl::AddAssignment>()
                    && incr_expr.as<Nodecl::AddAssignment>().get_lhs().get_symbol() == _induction_var.get_symbol())
            {
                _step = incr_expr.as<Nodecl::AddAssignment>().get_rhs().shallow_copy();
            }
            // _induction_var -= incr
            else if (incr_expr.is<Nodecl::MinusAssignment>()
                    && incr_expr.as<Nodecl::MinusAssignment>().get_lhs().get_symbol() == _induction_var.get_symbol())
            {
                Nodecl::NodeclBase rhs = incr_expr.as<Nodecl::AddAssignment>().get_rhs();

                TL::Type t = incr_expr.as<Nodecl::AddAssignment>().get_rhs().get_type();

                if (t.is_any_reference())
                    t = t.references_to();

                if (rhs.is_constant())
                {
                    _step = const_value_to_nodecl(const_value_neg(rhs.get_constant()));
                }
                else
                {
                    _step = Nodecl::Neg::make(
                            rhs,
                            t,
                            rhs.get_locus());
                }
            }
            // _induction_var = _induction_var + incr
            else if (incr_expr.is<Nodecl::Assignment>()
                    && incr_expr.as<Nodecl::Assignment>().get_lhs().get_symbol() == _induction_var.get_symbol()
                    && incr_expr.as<Nodecl::Assignment>().get_rhs().is<Nodecl::Add>()
                    && incr_expr.as<Nodecl::Assignment>().get_rhs().as<Nodecl::Add>().get_lhs().get_symbol()
                    == _induction_var.get_symbol())
            {
                _step = incr_expr.as<Nodecl::Assignment>().get_rhs().as<Nodecl::Add>().get_rhs().shallow_copy();
            }
            // _induction_var = incr + _induction_var
            else if (incr_expr.is<Nodecl::Assignment>()
                    && incr_expr.as<Nodecl::Assignment>().get_lhs().get_symbol() == _induction_var.get_symbol()
                    && incr_expr.as<Nodecl::Assignment>().get_rhs().is<Nodecl::Add>()
                    && incr_expr.as<Nodecl::Assignment>().get_rhs().as<Nodecl::Add>().get_rhs().get_symbol()
                    == _induction_var.get_symbol())
            {
                _step = incr_expr.as<Nodecl::Assignment>().get_rhs().as<Nodecl::Add>().get_lhs().shallow_copy();
            }
            // _induction_var = _induction_var - incr
            else if (incr_expr.is<Nodecl::Assignment>()
                    && incr_expr.as<Nodecl::Assignment>().get_lhs().get_symbol() == _induction_var.get_symbol()
                    && incr_expr.as<Nodecl::Assignment>().get_rhs().is<Nodecl::Minus>()
                    && incr_expr.as<Nodecl::Assignment>().get_rhs().as<Nodecl::Minus>().get_lhs().get_symbol()
                    == _induction_var.get_symbol())
            {
                Nodecl::NodeclBase rhs = incr_expr.as<Nodecl::Assignment>().get_rhs().as<Nodecl::Minus>().get_rhs();

                TL::Type t = rhs.get_type();

                if (t.is_any_reference())
                    t = t.references_to();

                if (rhs.is_constant())
                {
                    _step = const_value_to_nodecl(
                            const_value_neg(rhs.get_constant()));
                }
                else
                {
                    _step = Nodecl::Neg::make(
                            rhs.shallow_copy(),
                            t,
                            rhs.get_locus());
                }
            }
            else
            {
                _is_omp_valid = false;
                return;
            }
        }
        else
        {
            internal_error("Code unreachable", 0);
        }

        _is_omp_valid = true;
    }

    bool ForStatement::is_omp_valid_loop() const
    {
        return _is_omp_valid;
    }

    TL::Symbol ForStatement::get_induction_variable() const
    {
        return _induction_var.get_symbol();
    }

    bool ForStatement::induction_variable_in_separate_scope() const
    {
        return _induction_variable_in_separate_scope;
    }

    Nodecl::NodeclBase ForStatement::get_lower_bound() const
    {
        return _lower_bound;
    }

    Nodecl::NodeclBase ForStatement::get_upper_bound() const
    {
        return _upper_bound;
    }

    Nodecl::NodeclBase ForStatement::get_step() const
    {
        return _step;
    }
}
