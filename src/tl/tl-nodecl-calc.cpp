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

#include "cxx-codegen.h"

#include "tl-nodecl-calc.hpp"

namespace Nodecl
{
    Calculator::Calculator()
    {}
 
    const_value_t* Calculator::compute_const_value(Nodecl::NodeclBase val)
    {
        TL::ObjectList<const_value_t*> const_val = walk(val);
        if (const_val.empty())
            return NULL;
        else
            return const_val[0];
    }
 
    Calculator::Ret Calculator::unhandled_node(const Nodecl::NodeclBase& n)
    {
        Nodecl::NodeclBase unhandled_n = n;
        std::cerr << "Unhandled node type '" << ast_print_node_type(n.get_kind()) << "'"
                  << " while calculating constant value of expression '" << unhandled_n.prettyprint() << "'" << std::endl;
        return Ret();
    }
    
    Calculator::Ret Calculator::visit(const Nodecl::Symbol& n)
    {
        if (n.is_constant())
        {
            return TL::ObjectList<const_value_t*>(1, n.get_constant());
        }
        else
        {
            return TL::ObjectList<const_value_t*>();
        }
    }
    
    Calculator::Ret Calculator::visit(const Nodecl::Conversion& n)
    {
        TL::ObjectList<const_value_t*> nest = walk(n.get_nest());
        
        if (nest.empty())
        {
            return TL::ObjectList<const_value_t*>();
        }
        else
        {
            return TL::ObjectList<const_value_t*>(1, nest[0]);
        }
    }
    
    Calculator::Ret Calculator::visit(const Nodecl::Add& n)
    {
        TL::ObjectList<const_value_t*> lhs = walk(n.get_lhs());
        TL::ObjectList<const_value_t*> rhs = walk(n.get_rhs());
        
        if (lhs.empty() || rhs.empty())
        {
            return TL::ObjectList<const_value_t*>();
        }
        else
        {
            return TL::ObjectList<const_value_t*>(1, const_value_add(lhs[0], rhs[0]));
        }
    }
    
    Calculator::Ret Calculator::visit(const Nodecl::Minus& n)
    {
        TL::ObjectList<const_value_t*> lhs = walk(n.get_lhs());
        TL::ObjectList<const_value_t*> rhs = walk(n.get_rhs());
        
        if (lhs.empty() || rhs.empty())
        {
            return TL::ObjectList<const_value_t*>();
        }
        else
        {
            return TL::ObjectList<const_value_t*>(1, const_value_sub(lhs[0], rhs[0]));
        }
    }
    
    Calculator::Ret Calculator::visit(const Nodecl::Mul& n)
    {
        TL::ObjectList<const_value_t*> lhs = walk(n.get_lhs());
        TL::ObjectList<const_value_t*> rhs = walk(n.get_rhs());
        
        if (lhs.empty() || rhs.empty())
        {
            return TL::ObjectList<const_value_t*>();
        }
        else
        {
            return TL::ObjectList<const_value_t*>(1, const_value_mul(lhs[0], rhs[0]));
        }
    }
    
    Calculator::Ret Calculator::visit(const Nodecl::Div& n)
    {
        TL::ObjectList<const_value_t*> lhs = walk(n.get_lhs());
        TL::ObjectList<const_value_t*> rhs = walk(n.get_rhs());
        
        if (lhs.empty() || rhs.empty())
        {
            return TL::ObjectList<const_value_t*>();
        }
        else
        {
            return TL::ObjectList<const_value_t*>(1, const_value_div(lhs[0], rhs[0]));
        }
    }
    
    Calculator::Ret Calculator::visit(const Nodecl::Mod& n)
    {
        TL::ObjectList<const_value_t*> lhs = walk(n.get_lhs());
        TL::ObjectList<const_value_t*> rhs = walk(n.get_rhs());
        
        if (lhs.empty() || rhs.empty())
        {
            return TL::ObjectList<const_value_t*>();
        }
        else
        {
            return TL::ObjectList<const_value_t*>(1, const_value_mod(lhs[0], rhs[0]));
        }
    }
    
    Calculator::Ret Calculator::visit(const Nodecl::BitwiseShr& n)
    {
        TL::ObjectList<const_value_t*> lhs = walk(n.get_lhs());
        TL::ObjectList<const_value_t*> rhs = walk(n.get_rhs());
        
        if (lhs.empty() || rhs.empty())
        {
            return TL::ObjectList<const_value_t*>();
        }
        else
        {
            return TL::ObjectList<const_value_t*>(1, const_value_shr(lhs[0], rhs[0]));
        }
    }

    Calculator::Ret Calculator::visit(const Nodecl::ArithmeticShr& n)
    {
        TL::ObjectList<const_value_t*> lhs = walk(n.get_lhs());
        TL::ObjectList<const_value_t*> rhs = walk(n.get_rhs());
        
        if (lhs.empty() || rhs.empty())
        {
            return TL::ObjectList<const_value_t*>();
        }
        else
        {
            return TL::ObjectList<const_value_t*>(1, const_value_shr(lhs[0], rhs[0]));
        }
    }
    
    Calculator::Ret Calculator::visit(const Nodecl::BitwiseShl& n)
    {
        TL::ObjectList<const_value_t*> lhs = walk(n.get_lhs());
        TL::ObjectList<const_value_t*> rhs = walk(n.get_rhs());
        
        if (lhs.empty() || rhs.empty())
        {
            return TL::ObjectList<const_value_t*>();
        }
        else
        {
            return TL::ObjectList<const_value_t*>(1, const_value_bitshl(lhs[0], rhs[0]));
        }
    }
    
    Calculator::Ret Calculator::visit(const Nodecl::Power& n)
    {
        TL::ObjectList<const_value_t*> lhs = walk(n.get_lhs());
        TL::ObjectList<const_value_t*> rhs = walk(n.get_rhs());
        
        if (lhs.empty() || rhs.empty())
        {
            return TL::ObjectList<const_value_t*>();
        }
        else
        {
            return TL::ObjectList<const_value_t*>(1, const_value_pow(lhs[0], rhs[0]));
        }            
    }
    
    Calculator::Ret Calculator::visit(const Nodecl::LogicalAnd& n)
    {
        TL::ObjectList<const_value_t*> lhs = walk(n.get_lhs());
        TL::ObjectList<const_value_t*> rhs = walk(n.get_rhs());
        
        if (lhs.empty() || rhs.empty())
        {
            return TL::ObjectList<const_value_t*>();
        }
        else
        {
            return TL::ObjectList<const_value_t*>(1, const_value_and(lhs[0], rhs[0]));
        }
    }
    
    Calculator::Ret Calculator::visit(const Nodecl::LogicalOr& n)
    {
        TL::ObjectList<const_value_t*> lhs = walk(n.get_lhs());
        TL::ObjectList<const_value_t*> rhs = walk(n.get_rhs());
        
        if (lhs.empty() || rhs.empty())
        {
            return TL::ObjectList<const_value_t*>();
        }
        else
        {
            return TL::ObjectList<const_value_t*>(1, const_value_or(lhs[0], rhs[0]));
        }
    }
    
    Calculator::Ret Calculator::visit(const Nodecl::BitwiseAnd& n)
    {
        TL::ObjectList<const_value_t*> lhs = walk(n.get_lhs());
        TL::ObjectList<const_value_t*> rhs = walk(n.get_rhs());
        
        if (lhs.empty() || rhs.empty())
        {
            return TL::ObjectList<const_value_t*>();
        }
        else
        {
            return TL::ObjectList<const_value_t*>(1, const_value_bitand(lhs[0], rhs[0]));
        }
    }
    Calculator::Ret Calculator::visit(const Nodecl::BitwiseOr& n)
    {
        TL::ObjectList<const_value_t*> lhs = walk(n.get_lhs());
        TL::ObjectList<const_value_t*> rhs = walk(n.get_rhs());
        
        if (lhs.empty() || rhs.empty())
        {
            return TL::ObjectList<const_value_t*>();
        }
        else
        {
            return TL::ObjectList<const_value_t*>(1, const_value_bitor(lhs[0], rhs[0]));
        }
    }
    
    Calculator::Ret Calculator::visit(const Nodecl::BitwiseXor& n)
    {
        TL::ObjectList<const_value_t*> lhs = walk(n.get_lhs());
        TL::ObjectList<const_value_t*> rhs = walk(n.get_rhs());
        
        if (lhs.empty() || rhs.empty())
        {
            return TL::ObjectList<const_value_t*>();
        }
        else
        {
            return TL::ObjectList<const_value_t*>(1, const_value_bitxor(lhs[0], rhs[0]));
        }
    }

    Calculator::Ret Calculator::visit(const Nodecl::Plus& n)
    {
        TL::ObjectList<const_value_t*> rhs = walk(n.get_rhs());
        
        if (rhs.empty())
        {
            return TL::ObjectList<const_value_t*>();
        }
        else
        {
            return TL::ObjectList<const_value_t*>(1, const_value_plus(rhs[0]));
        }
    }
    
    Calculator::Ret Calculator::visit(const Nodecl::Neg& n)
    {
        TL::ObjectList<const_value_t*> rhs = walk(n.get_rhs());
        
        if (rhs.empty())
        {
            return TL::ObjectList<const_value_t*>();
        }
        else
        {
            return TL::ObjectList<const_value_t*>(1, const_value_neg(rhs[0]));
        }
    }
    
    Calculator::Ret Calculator::visit(const Nodecl::BitwiseNot& n)
    {
        TL::ObjectList<const_value_t*> rhs = walk(n.get_rhs());
        
        if (rhs.empty())
        {
            return TL::ObjectList<const_value_t*>();
        }
        else
        {
            return TL::ObjectList<const_value_t*>(1, const_value_bitnot(rhs[0]));
        }
    }
    
    Calculator::Ret Calculator::visit(const Nodecl::LogicalNot& n)
    {
        TL::ObjectList<const_value_t*> rhs = walk(n.get_rhs());
        
        if (rhs.empty())
        {
            return TL::ObjectList<const_value_t*>();
        }
        else
        {
            return TL::ObjectList<const_value_t*>(1, const_value_not(rhs[0]));
        }
    }

    Calculator::Ret Calculator::visit(const Nodecl::Predecrement& n)
    {
        TL::ObjectList<const_value_t*> rhs = walk(n.get_rhs());
        
        if (rhs.empty())
        {
            return TL::ObjectList<const_value_t*>();
        }
        else
        {
            return TL::ObjectList<const_value_t*>(1, const_value_sub(rhs[0], const_value_get_unsigned_int((uint64_t) 1)));
        }
    }
    
    Calculator::Ret Calculator::visit(const Nodecl::Postdecrement& n)
    {
        TL::ObjectList<const_value_t*> rhs = walk(n.get_rhs());
        
        if (rhs.empty())
        {
            return TL::ObjectList<const_value_t*>();
        }
        else
        {
            return TL::ObjectList<const_value_t*>(1, const_value_sub(rhs[0], const_value_get_unsigned_int((uint64_t) 1)));
        }
    }
    
    Calculator::Ret Calculator::visit(const Nodecl::Preincrement& n)
    {
        TL::ObjectList<const_value_t*> rhs = walk(n.get_rhs());
        
        if (rhs.empty())
        {
            return TL::ObjectList<const_value_t*>();
        }
        else
        {
            return TL::ObjectList<const_value_t*>(1, const_value_add(rhs[0], const_value_get_unsigned_int((uint64_t) 1)));
        }
    }
    
    Calculator::Ret Calculator::visit(const Nodecl::Postincrement& n)
    {
        TL::ObjectList<const_value_t*> rhs = walk(n.get_rhs());
        
        if (rhs.empty())
        {
            return TL::ObjectList<const_value_t*>();
        }
        else
        {
            return TL::ObjectList<const_value_t*>(1, const_value_add(rhs[0], const_value_get_unsigned_int((uint64_t) 1)));
        }
    }
    
    Calculator::Ret Calculator::visit(const Nodecl::IntegerLiteral& n)
    {
        return TL::ObjectList<const_value_t*>(1, n.get_constant());
    }
    
    Calculator::Ret Calculator::visit(const Nodecl::FloatingLiteral& n)
    {
        return TL::ObjectList<const_value_t*>(1, n.get_constant());
    }
    
    Calculator::Ret Calculator::visit(const Nodecl::ComplexLiteral& n)
    {
        return TL::ObjectList<const_value_t*>();
    }
    
    Calculator::Ret Calculator::visit(const Nodecl::BooleanLiteral& n)
    {
        return TL::ObjectList<const_value_t*>();
    }
    
    Calculator::Ret Calculator::visit(const Nodecl::StringLiteral& n)
    {
        return TL::ObjectList<const_value_t*>();
    }
    
    Calculator::Ret Calculator::visit(const Nodecl::Dereference& n)
    {
        return TL::ObjectList<const_value_t*>();
    }
    
    Calculator::Ret Calculator::visit(const Nodecl::ArraySubscript& n)
    {
        return TL::ObjectList<const_value_t*>();
    }

    Calculator::Ret Calculator::visit(const Nodecl::ClassMemberAccess& n)
    {
        TL::ObjectList<const_value_t*> member = walk(n.get_member());
        
        if (member.empty())
        {
            return TL::ObjectList<const_value_t*>();
        }
        else
        {
            return TL::ObjectList<const_value_t*>(1, member[0]);
        }
    }

    Calculator::Ret Calculator::visit(const Nodecl::Cast& n)
    {
        return walk(n.get_rhs());
    }
    
    Calculator::Ret Calculator::visit(const Nodecl::FunctionCall& n)
    {
        return TL::ObjectList<const_value_t*>();
    }
    
    Calculator::Ret Calculator::visit(const Nodecl::VirtualFunctionCall& n)
    {
        return TL::ObjectList<const_value_t*>();
    }
    
    Calculator::Ret Calculator::visit(const Nodecl::Equal& n)
    {
        TL::ObjectList<const_value_t*> lhs = walk(n.get_lhs());
        TL::ObjectList<const_value_t*> rhs = walk(n.get_rhs());
        
        if (!lhs.empty() && !rhs.empty())
        {
            const_value_t* equals;
            if (lhs[0] == rhs[0])
            {
                equals = const_value_get_one(/* bytes */ 4, /* signed */ 1);
            }
            else
            {
                equals = const_value_get_zero(/* bytes */ 4, /* signed */ 1);
            }
            return TL::ObjectList<const_value_t*>(1, equals);
        }
        else
        {
            return TL::ObjectList<const_value_t*>();
        }
    }
    
    Calculator::Ret Calculator::visit(const Nodecl::Different& n)
    {
        TL::ObjectList<const_value_t*> lhs = walk(n.get_lhs());
        TL::ObjectList<const_value_t*> rhs = walk(n.get_rhs());
        
        if (!lhs.empty() && !rhs.empty())
        {
            const_value_t* equals;
            if (lhs[0] == rhs[0])
            {
                equals = const_value_get_zero(/* bytes */ 4, /* signed */ 1);
            }
            else
            {
                equals = const_value_get_one(/* bytes */ 4, /* signed */ 1);
            }
            return TL::ObjectList<const_value_t*>(1, equals);
        }
        else
        {
            return TL::ObjectList<const_value_t*>();
        }
    }
    
    Calculator::Ret Calculator::visit(const Nodecl::LowerThan& n)
    {
        TL::ObjectList<const_value_t*> lhs = walk(n.get_lhs());
        TL::ObjectList<const_value_t*> rhs = walk(n.get_rhs());
        
        if (!lhs.empty() && !rhs.empty())
        {
            const_value_t* equals;
            if (lhs[0] < rhs[0])
            {
                equals = const_value_get_one(/* bytes */ 4, /* signed */ 1);
            }
            else
            {
                equals = const_value_get_zero(/* bytes */ 4, /* signed */ 1);
            }
            return TL::ObjectList<const_value_t*>(1, equals);
        }
        else
        {
            return TL::ObjectList<const_value_t*>();
        }
    }
    
    Calculator::Ret Calculator::visit(const Nodecl::LowerOrEqualThan& n)
    {
        TL::ObjectList<const_value_t*> lhs = walk(n.get_lhs());
        TL::ObjectList<const_value_t*> rhs = walk(n.get_rhs());
        
        if (!lhs.empty() && !rhs.empty())
        {
            const_value_t* equals;
            if (lhs[0] == rhs[0])
            {
                equals = const_value_get_one(/* bytes */ 4, /* signed */ 1);
            }
            else
            {
                equals = const_value_get_zero(/* bytes */ 4, /* signed */ 1);
            }
            return TL::ObjectList<const_value_t*>(1, equals);
        }
        else
        {
            return TL::ObjectList<const_value_t*>();
        }
    }
    
    Calculator::Ret Calculator::visit(const Nodecl::GreaterThan& n)
    {
        TL::ObjectList<const_value_t*> lhs = walk(n.get_lhs());
        TL::ObjectList<const_value_t*> rhs = walk(n.get_rhs());
        
        if (!lhs.empty() && !rhs.empty())
        {
            const_value_t* equals;
            if (lhs[0] > rhs[0])
            {
                equals = const_value_get_one(/* bytes */ 4, /* signed */ 1);
            }
            else
            {
                equals = const_value_get_zero(/* bytes */ 4, /* signed */ 1);
            }
            return TL::ObjectList<const_value_t*>(1, equals);
        }
        else
        {
            return TL::ObjectList<const_value_t*>();
        }
    }
    
    Calculator::Ret Calculator::visit(const Nodecl::GreaterOrEqualThan& n)
    {
        TL::ObjectList<const_value_t*> lhs = walk(n.get_lhs());
        TL::ObjectList<const_value_t*> rhs = walk(n.get_rhs());
        
        if (!lhs.empty() && !rhs.empty())
        {
            const_value_t* equals;
            if (lhs[0] >= rhs[0])
            {
                equals = const_value_get_one(/* bytes */ 4, /* signed */ 1);
            }
            else
            {
                equals = const_value_get_zero(/* bytes */ 4, /* signed */ 1);
            }
            return TL::ObjectList<const_value_t*>(1, equals);
        }
        else
        {
            return TL::ObjectList<const_value_t*>();
        }
    }
    
    Calculator::Ret Calculator::visit(const Nodecl::ConditionalExpression& n)
    {
        TL::ObjectList<const_value_t*> condition = walk(n.get_condition());
        
        if (condition.empty())
        {
            return TL::ObjectList<const_value_t*>();
        }
        else
        {
            if (condition[0])
            {
                TL::ObjectList<const_value_t*> true_val = walk(n.get_true());
                if (true_val.empty())
                {
                    return TL::ObjectList<const_value_t*>();
                }
                else
                {
                    return TL::ObjectList<const_value_t*>(1, true_val[0]);
                }
            }
            else
            {
                TL::ObjectList<const_value_t*> false_val = walk(n.get_false());
                if (false_val.empty())
                {
                    return TL::ObjectList<const_value_t*>();
                }
                else
                {
                    return TL::ObjectList<const_value_t*>(1, false_val[0]);
                }
            }
        }
    }
    
    Calculator::Ret Calculator::visit(const Nodecl::Assignment& n)
    {
        TL::ObjectList<const_value_t*> rhs = walk(n.get_rhs());
        return rhs;
    }
    
    Calculator::Ret Calculator::visit(const Nodecl::Sizeof& n)
    {
        return TL::ObjectList<const_value_t*>();
    }
}
