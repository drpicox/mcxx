/*--------------------------------------------------------------------
(C) Copyright 2006-2009 Barcelona Supercomputing Center 
Centro Nacional de Supercomputacion

This file is part of Mercurium C/C++ source-to-source compiler.

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


#ifndef EXTENSIBLE_SYMBOL_HPP
#define EXTENSIBLE_SYMBOL_HPP

#include <set>

#include "tl-nodecl.hpp"
#include "tl-symbol.hpp"

namespace TL
{  
    /*!
      This class is used to stored extended information of a Symbol.
      It can express:
      - the accessed member of an structure.
      - the set of accessed positions of an array.
    */
    class LIBTL_CLASS ExtensibleSymbol : public TL::Object
    {
        private:
            Symbol _sym;
            Nodecl::NodeclBase _n;
            
        public:
            // *** Constructors *** //
            
            //! Empty Constructor of an Extensible Symbol.
            /*!
              It builds an Extensible Symbol with non-associated Symbol.
             */
            ExtensibleSymbol();
            
            //! Constructor building an Extensible Symbol from a valid Symbol.
            /*!
              Use is_valid if the Symbol wrapped as an ExtensibleSymbol is eligible as an 
              extensible symbol.
              \param s Symbol which is wrapped in the new ExtensibleSymbol
              \param n Nodecl containg additional information about the Symbol like
                       the member accessed in a struct or the subscript of an array access.
                       By default, this parameter contains a null nodecl.
             */
            ExtensibleSymbol(Symbol s, Nodecl::NodeclBase n = Nodecl::NodeclBase::null());
            
            
            // *** Getters and Setters *** //
            
            //! Returns the name of the wrapped symbol.
            std::string get_name() const;
            
            //! Returns the type of the wrapped symbol.
            Type get_type() const;

            //! Returns the nodecl associated with the wrapped symbol.
            Nodecl::NodeclBase get_nodecl() const;           

            //! Returns true when the extensible symbol contains a symbols which do not represents
            //! neither an array access nor a member access, but a symbol.
            bool is_simple_symbol() const;
            
            //! Returns true when the extensible symbol contains an array access.
            /*!
             * This method is only valid when the extensible symbol wraps a non-simple symbol
             */            
            bool is_array_access() const;
            
            //! Returns true when the extensible symbol contains a member access.
            /*!
             * This method is only valid when the extensible symbol wraps a non-simple symbol
             */            
            bool is_member_access() const;
            
            //! Returns the symbol wrapped in the Extended Symbol
            Symbol get_symbol() const;
            
            //! Compares the content of two AST nodes
            bool equal_ast_nodes(nodecl_t t1, nodecl_t t2) const;
            
            //! Compares the one to one the nodes from two roots
            bool equal_trees_rec(nodecl_t t1, nodecl_t t2) const;
            
            //! Returns equals when the two nodes are the same
            /*!
             * Be the same here means that the two nodes have exactly the same nodes 
             * organized in the same way and containing the same symbols inside.
             * FIXME This comparison should compare canonical version of expressions
             */
            bool equal_nodecls(Nodecl::NodeclBase n1, Nodecl::NodeclBase n2) const;
            
            // *** Overloaded methods *** //
            bool operator==(const ExtensibleSymbol &es) const;
            bool operator<(const ExtensibleSymbol &es) const;
    };
    
    
    //! Compare class for ExtensibleSymbols
    /*!
      It is used as Comparison Class when a std::set of ExtensibleSymbols is built.
     */
    struct ExtensibleSymbol_comp
    {
        bool operator() (const ExtensibleSymbol& es1, const ExtensibleSymbol& es2) const
        { 
            return es1 < es2; 
        }
    };
    
    typedef std::set<ExtensibleSymbol, ExtensibleSymbol_comp> ext_sym_set;
}

#endif // EXTENSIBLE_SYMBOL_HPP