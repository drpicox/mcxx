/*--------------------------------------------------------------------
  (C) Copyright 2006-2013 Barcelona Supercomputing Center
                          Centro Nacional de Supercomputacion

  This file is part of Mercurium C/C++ source-to-source compiler.

  See AUTHORS file in the top level directory for information
  regarding developers and contributors.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 3 of the License, or (at your option ) any later version.

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

#ifndef TL_PCFG_VISITOR_HPP
#define TL_PCFG_VISITOR_HPP

#include <stack>

#include "tl-extensible-graph.hpp"
#include "tl-nodecl-visitor.hpp"
#include "tl-node.hpp"
#include "tl-pcfg-utils.hpp"


namespace TL {
namespace Analysis {

    class LIBTL_CLASS PCFGVisitor : public Nodecl::NodeclVisitor<TL::ObjectList<Node*> >
    {
    private:

        ExtensibleGraph* _pcfg;     /*!< Actual PCFG being built during the visit */

        PCFGVisitUtils* _utils;      /*!< Class storing temporary values for the construction of the graph */


        //! This method creates a list with the nodes in an specific subgraph
        /*!
        * \param node First node to be traversed. The method will visit all nodes from here.
        */
        void compute_catch_parents(Node* node);

        //! This method finds the parent nodes in a sequence of connected nodes
        /*!
         * The method fails when the sub-graph has more than one entry node.
         * Since this method is used specifically to collapse the nodes created while building the node of an expression
         * we expect to find only one entry node.
         * (We don't refer a node of BASIC_ENTRY_NODE type, but the first node in the sub-graph)
         * \param actual_node Node we are computing in this moment
         * \return The entry node of a sub-graph
         */
        ObjectList<Node*> get_first_nodes(Node* actual_node);

        //! This method merges a list of nodes containing an Expression into one
        /*!
        * The way the method merges the nodes depends on the kind of the nodes to be merged:
        * The nodes that are not a GRAPH NODE are deleted. The rest remain there to be the parents of the new node.
        * \param n Nodecl containing a Expression which will be wrapped in the new node
        * \param nodes_l List of nodes containing the different parts of an expression
        * \return The new node created
        */
        Node* merge_nodes(Nodecl::NodeclBase n, ObjectList<Node*> nodes_l);

        //! This is a wrapper method of #merge_nodes for the case having only one or two nodes to be merged
        /*!
        * \param n Nodecl containing a Expression which will be wrapped in the new node
        * \param first Pointer to the node containing one part of the new node
        * \param second Pointer to the node containing other part of the new node
        */
        Node* merge_nodes(Nodecl::NodeclBase n, Node* first, Node* second);

        //! This method creates a virtual synchronization node when tasks are not synchronized
        //! within the same function they are created, or they are synchronized in conditional situations
        void synchronize_tasks( );

        bool task_is_surely_synchronized( Node* task );

        bool same_parent_task( Node* task_1, Node* task_2 );

        // ************************************************************************************** //
        // ********************************** Visiting methods ********************************** //

        //! This method implements teh visitor for any kind of barrier: BarrierAtEnd, BarrierFull
        Ret visit_barrier( );

        //! This method implements the visitor for binary nodecls
        /*!
         * The nodes wrapped in this visitor method are:
         *   Add, AddAssignment, ArithmeticShr, ArithmeticShrAssignment, Assignment,
         *   BitwiseAnd, BitwiseAndAssignment, BitwiseOr, BitwiseOrAssignment,
         *   BitwiseShl, BitwiseShlAssignment, BitwiseShr, BitwiseShrAssignment,
         *   BitwiseXor, BitwiseXorAssignment, ClassMemberAccess, Concat, Different,
         *   Div, DivAssignment, Equal, GreaterOrEqualThan, GreaterThan, LogicalAnd,
         *   LogicalOr, LowerOrEqualThan, LowerThan, MinusAssignment, Mod, ModAssignment,
         *   Mul, MulAssignment, Offset, Offsetof, Power
         * \param lhs Nodecl to be visited
         * \param lhs Left-hand side of the nodecl
         * \param rhs Right-hand side of the nodecl
         */
        Ret visit_binary_node( const Nodecl::NodeclBase& n,
                               const Nodecl::NodeclBase& lhs, const Nodecl::NodeclBase& rhs );

        //! This method implements the visitor for a CaseStatement and for DefaultStatement
        /*!
         * \param n Nodecl containing the Case or the Default Statement
         * \return The graph node created while the Statement has been parsed
         */
        Ret visit_case_or_default( const Nodecl::NodeclBase& case_stmt, const Nodecl::NodeclBase& case_val );

        //! This method implements the visitor for a VirtualFunctionCall and a FunctionCall
        /*!
         * \param n Nodecl containinf the VirtualFunctionCall or the FunctionCall
         * \return The graph node created while the function call has been parsed
         */
        template <typename T>
        Ret visit_function_call( const T& n );

        //! This method implements the visitor for nodecls generating a unique node containing itself
        /*!
         * The nodes wrapped in this visitor method are:
         *   BooleanLiteral, ComplexLiteral, EmptyStatement, FloatingLiteral,
         *   IntegerLiteral, StringLiteral, Symbol, Type
         * \param n The nodecl
         */
        Ret visit_literal_node( const Nodecl::NodeclBase& n );

        //! This method implements the visitor for any kind of taskwait: TaskwaitDeep, TaskwaitShallow
        Ret visit_taskwait( );

        //! This method implements the visitor for unary nodecls
        /*!
         * The nodes wrapped in this visitor method are:
         *   BitwiseNot, Cast, Delete, DeleteArray, Dereference, LogicalNot, Neg, New, Plus,
         *   Postdecrement, Postincrement, Predecrement, Preincrement, Reference, Sizeof,
         *   Typeid
         * \param rhs Right-hand side
         */
        Ret visit_unary_node( const Nodecl::NodeclBase& n, const Nodecl::NodeclBase& rhs );

        // ******************************** END visiting methods ******************************** //
        // ************************************************************************************** //


        bool func_has_cyclic_calls_rec(Symbol reach_func, Symbol stop_func, ExtensibleGraph * graph);


    public:

        // ************************************************************************************** //
        // ************************************ Constructors ************************************ //

        //! Constructor building an empty PCFG
        PCFGVisitor( std::string name, Nodecl::NodeclBase nodecl );

        // ********************************** END constructors ********************************** //
        // ************************************************************************************** //



        // ************************************************************************************** //
        // ******************************** Non-visiting methods ******************************** //

        /*!Generates one PCFG per each function of an AST
         * \param n AST containing the code used to generate the PCFG
         */
        ExtensibleGraph* parallel_control_flow_graph( const Nodecl::NodeclBase& n );

        void set_actual_pcfg(ExtensibleGraph* graph);

        // ****************************** END non-visiting methods ****************************** //
        // ************************************************************************************** //



        // ************************* IPA ************************* //
        //! Computes the define-use chain of a node
        void compute_use_def_chains(Node* node);

        //! Analyse loops and ranged access to variables. Recomputes use-def and reaching definitions
        //! with the info of iterated accesses
        void analyse_loops(Node* node);

        //! Once the use-def chains are calculated for every graph, we are able to recalculate the use-def of every function call
        bool propagate_use_def_ipa(Node* node);

        bool func_has_cyclic_calls(Symbol actual_func, ExtensibleGraph* graph);


        // ************************************************************************************** //
        // ********************************** Visiting methods ********************************** //

        Ret unhandled_node( const Nodecl::NodeclBase& n );
        Ret visit( const Nodecl::Add& n );
        Ret visit( const Nodecl::AddAssignment& n );
        Ret visit( const Nodecl::Alignof& n );
        Ret visit( const Nodecl::ArithmeticShr& n );
        Ret visit( const Nodecl::ArithmeticShrAssignment& n );
        Ret visit( const Nodecl::ArraySubscript& n );
        Ret visit( const Nodecl::Assignment& n );
        Ret visit( const Nodecl::BitwiseAnd& n );
        Ret visit( const Nodecl::BitwiseAndAssignment& n );
        Ret visit( const Nodecl::BitwiseNot& n );
        Ret visit( const Nodecl::BitwiseOr& n );
        Ret visit( const Nodecl::BitwiseOrAssignment& n );
        Ret visit( const Nodecl::BitwiseShl& n );
        Ret visit( const Nodecl::BitwiseShlAssignment& n );
        Ret visit( const Nodecl::BitwiseShr& n );
        Ret visit( const Nodecl::BitwiseShrAssignment& n);
        Ret visit( const Nodecl::BitwiseXor& n );
        Ret visit( const Nodecl::BitwiseXorAssignment& n );
        Ret visit( const Nodecl::BooleanLiteral& n );
        Ret visit( const Nodecl::BreakStatement& n );
        Ret visit( const Nodecl::CaseStatement& n );
        Ret visit( const Nodecl::Cast& n );
        Ret visit( const Nodecl::CatchHandler& n );
        Ret visit( const Nodecl::ClassMemberAccess& n );
        Ret visit( const Nodecl::Comma& n );
        Ret visit( const Nodecl::ComplexLiteral& n );
        Ret visit( const Nodecl::CompoundExpression& n );
        Ret visit( const Nodecl::CompoundStatement& n );
        Ret visit( const Nodecl::Concat& n );
        Ret visit( const Nodecl::ConditionalExpression& n );
        Ret visit( const Nodecl::Context& n );
        Ret visit( const Nodecl::ContinueStatement& n );
        Ret visit( const Nodecl::Conversion& n );
        Ret visit( const Nodecl::CxxDef& n );
        Ret visit( const Nodecl::DefaultStatement& n );
        Ret visit( const Nodecl::Delete& n );
        Ret visit( const Nodecl::DeleteArray& n );
        Ret visit( const Nodecl::Dereference& n );
        Ret visit( const Nodecl::Different& n );
        Ret visit( const Nodecl::Div& n );
        Ret visit( const Nodecl::DivAssignment& n );
        Ret visit( const Nodecl::DoStatement& n );
        Ret visit( const Nodecl::EmptyStatement& n );
        Ret visit( const Nodecl::Equal& n );
        Ret visit( const Nodecl::ExpressionStatement& n );
        Ret visit( const Nodecl::FloatingLiteral& n );
        Ret visit( const Nodecl::ForStatement& n );
        Ret visit( const Nodecl::FunctionCall& n );
        Ret visit( const Nodecl::FunctionCode& n );
        Ret visit( const Nodecl::GccAsmDefinition& n );
        Ret visit( const Nodecl::GccAsmOperand& n );
        Ret visit( const Nodecl::GotoStatement& n );
        Ret visit( const Nodecl::GreaterOrEqualThan& n );
        Ret visit( const Nodecl::GreaterThan& n );
        Ret visit( const Nodecl::IfElseStatement& n );
        Ret visit( const Nodecl::IntegerLiteral& n );
        Ret visit( const Nodecl::LabeledStatement& n );
        Ret visit( const Nodecl::LogicalAnd& n );
        Ret visit( const Nodecl::LogicalNot& n );
        Ret visit( const Nodecl::LogicalOr& n );
        Ret visit( const Nodecl::LoopControl& n );
        Ret visit( const Nodecl::LowerOrEqualThan& n );
        Ret visit( const Nodecl::LowerThan& n );
        Ret visit( const Nodecl::Minus& n );
        Ret visit( const Nodecl::MinusAssignment& n );
        Ret visit( const Nodecl::Mod& n );
        Ret visit( const Nodecl::ModAssignment& n );
        Ret visit( const Nodecl::Mul& n );
        Ret visit( const Nodecl::MulAssignment& n );
        Ret visit( const Nodecl::Neg& n );
        Ret visit( const Nodecl::New& n );
        Ret visit( const Nodecl::ObjectInit& n );
        Ret visit( const Nodecl::Offset& n );
        Ret visit( const Nodecl::Offsetof& n );
        Ret visit( const Nodecl::OpenMP::Atomic& n );
        Ret visit( const Nodecl::OpenMP::Auto& n );
        Ret visit( const Nodecl::OpenMP::BarrierAtEnd& n );
        Ret visit( const Nodecl::OpenMP::BarrierFull& n );
        Ret visit( const Nodecl::OpenMP::BarrierSignal& n );
        Ret visit( const Nodecl::OpenMP::BarrierWait& n );
        Ret visit( const Nodecl::OpenMP::CombinedWorksharing& n );
        Ret visit( const Nodecl::OpenMP::CopyIn& n );
        Ret visit( const Nodecl::OpenMP::CopyInout& n );
        Ret visit( const Nodecl::OpenMP::CopyOut& n );
        Ret visit( const Nodecl::OpenMP::Critical& n );
        Ret visit( const Nodecl::OpenMP::CriticalName& n );
        Ret visit( const Nodecl::OpenMP::DepIn& n );
        Ret visit( const Nodecl::OpenMP::DepInout& n );
        Ret visit( const Nodecl::OpenMP::DepOut& n );
        Ret visit( const Nodecl::OpenMP::Firstprivate& n );
        Ret visit( const Nodecl::OpenMP::FlushAtEntry& n );
        Ret visit( const Nodecl::OpenMP::FlushAtExit& n );
        Ret visit( const Nodecl::OpenMP::FlushMemory& n );
        Ret visit( const Nodecl::OpenMP::For& n );
        Ret visit( const Nodecl::OpenMP::If& n );
        Ret visit( const Nodecl::OpenMP::Master& n );
        Ret visit( const Nodecl::OpenMP::Parallel& n );
        Ret visit( const Nodecl::OpenMP::ParallelSimdFor& n );
        Ret visit( const Nodecl::OpenMP::Priority& n );
        Ret visit( const Nodecl::OpenMP::Private& n );
        Ret visit( const Nodecl::OpenMP::Reduction& n );
        Ret visit( const Nodecl::OpenMP::ReductionItem& n );
        Ret visit( const Nodecl::OpenMP::Schedule& n );
        Ret visit( const Nodecl::OpenMP::Section& n );
        Ret visit( const Nodecl::OpenMP::Sections& n );
        Ret visit( const Nodecl::OpenMP::Shared& n );
        Ret visit( const Nodecl::OpenMP::Simd& n );
        Ret visit( const Nodecl::OpenMP::SimdFor& n );
        Ret visit( const Nodecl::OpenMP::SimdFunction& n );
        Ret visit( const Nodecl::OpenMP::Single& n );
        Ret visit( const Nodecl::OpenMP::Target& n );
        Ret visit( const Nodecl::OpenMP::Task& n );
        Ret visit( const Nodecl::OpenMP::TaskCall& n );
        Ret visit( const Nodecl::OpenMP::TaskwaitDeep& n );
        Ret visit( const Nodecl::OpenMP::TaskwaitShallow& n );
        Ret visit( const Nodecl::OpenMP::Untied& n );
        Ret visit( const Nodecl::OpenMP::WaitOnDependences& n );
        Ret visit( const Nodecl::ParenthesizedExpression& n );
        Ret visit( const Nodecl::Plus& n );
        Ret visit( const Nodecl::PointerToMember& n );
        Ret visit( const Nodecl::Postdecrement& n );
        Ret visit( const Nodecl::Postincrement& n );
        Ret visit( const Nodecl::Power& n );
        Ret visit( const Nodecl::PragmaCustomStatement& n );
        Ret visit( const Nodecl::Predecrement& n );
        Ret visit( const Nodecl::Preincrement& n );
        Ret visit( const Nodecl::Range& n );
        Ret visit( const Nodecl::Reference& n );
        Ret visit( const Nodecl::ReturnStatement& n );
        Ret visit( const Nodecl::Sizeof& n );
        Ret visit( const Nodecl::StringLiteral& n );
        Ret visit( const Nodecl::StructuredValue& n );
        Ret visit( const Nodecl::SwitchStatement& n );
        Ret visit( const Nodecl::Symbol& n );
        Ret visit( const Nodecl::Text& n );
        Ret visit( const Nodecl::Throw& n );
//      Ret visit( const Nodecl::TopLevel& n ); // This method is not implemented because PCFGVisitor must visit
                                                // sections of code creating a unique PCFG.
                                                // Use AnalysisSingleton methods instead.
        Ret visit( const Nodecl::TryBlock& n );
        Ret visit( const Nodecl::Type& n );
        Ret visit( const Nodecl::Typeid& n );
        Ret visit( const Nodecl::UnknownPragma& n );
        Ret visit( const Nodecl::VirtualFunctionCall& n );
        Ret visit( const Nodecl::WhileStatement& n );

        // ******************************** END visiting methods ******************************** //
        // ************************************************************************************** //
    };
}
}

#endif  // TL_PCFG_VISITOR_HPP
