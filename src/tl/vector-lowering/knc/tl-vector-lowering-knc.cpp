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

#include "tl-vector-lowering-knc.hpp"
#include "tl-source.hpp"

namespace TL 
{
    namespace Vectorization
    {
        KNCVectorLowering::KNCVectorLowering() 
        {
        }

        void KNCVectorLowering::visit(const Nodecl::ObjectInit& node) 
        {
            TL::Source intrin_src;
            
            if(node.has_symbol())
            {
                TL::Symbol sym = node.get_symbol();

                // Vectorizing initialization
                Nodecl::NodeclBase init = sym.get_value();
                if(!init.is_null())
                {
                    walk(init);
                }
            }
        }

        void KNCVectorLowering::visit(const Nodecl::VectorAdd& node) 
        { 
            TL::Type type = node.get_type().basic_type();

            TL::Source intrin_src;

            intrin_src << "_mm512_add";

            // Postfix
            if (type.is_float()) 
            { 
                intrin_src << "_ps"; 
            } 
            else if (type.is_double()) 
            { 
                intrin_src << "_pd"; 
            } 
            else if (type.is_signed_int() ||
                    type.is_unsigned_int()) 
            { 
                intrin_src << "_epi32"; 
            } 
            else
            {
                running_error("KNC Lowering: Node %s at %s has an unsupported type.", 
                        ast_print_node_type(node.get_kind()),
                        locus_to_str(node.get_locus()));
            }      
            //std::cerr << node.get_lhs().prettyprint() << " " << node.get_rhs().prettyprint();

            walk(node.get_lhs());
            walk(node.get_rhs());

            intrin_src << "(";
            intrin_src << as_expression(node.get_lhs());
            intrin_src << ", ";
            intrin_src << as_expression(node.get_rhs());
            intrin_src << ")";

            Nodecl::NodeclBase function_call = 
                    intrin_src.parse_expression(node.retrieve_context());

            node.replace(function_call);
        }                                                 

        void KNCVectorLowering::visit(const Nodecl::VectorMinus& node) 
        { 
            TL::Type type = node.get_type().basic_type();

            TL::Source intrin_src;

            intrin_src << "_mm512_sub";

            // Postfix
            if (type.is_float()) 
            { 
                intrin_src << "_ps"; 
            } 
            else if (type.is_double()) 
            { 
                intrin_src << "_pd"; 
            } 
            else if (type.is_signed_int() ||
                    type.is_unsigned_int()) 
            { 
                intrin_src << "_epi32"; 
            } 
            else
            {
                running_error("KNC Lowering: Node %s at %s has an unsupported type.", 
                        ast_print_node_type(node.get_kind()),
                        locus_to_str(node.get_locus()));
            }      

            walk(node.get_lhs());
            walk(node.get_rhs());

            intrin_src << "(";
            intrin_src << as_expression(node.get_lhs());
            intrin_src << ", ";
            intrin_src << as_expression(node.get_rhs());
            intrin_src << ")";

            Nodecl::NodeclBase function_call = 
                    intrin_src.parse_expression(node.retrieve_context());

            node.replace(function_call);
        }                                                 

        void KNCVectorLowering::visit(const Nodecl::VectorMul& node) 
        {
            TL::Type result_type = node.get_type().basic_type();
            TL::Type first_op_type = node.get_rhs().get_type().basic_type();
            TL::Type second_op_type = node.get_lhs().get_type().basic_type();

            TL::Source intrin_src;

            intrin_src << "_mm512_mul";

            // Postfix
            if (result_type.is_float() &&
                    first_op_type.is_float() &&
                    second_op_type.is_float())
            {
                intrin_src << "_ps"; 
            } 
            else if (result_type.is_double() &&
                    first_op_type.is_double() &&
                    second_op_type.is_double())
            { 
                intrin_src << "_pd"; 
            }
            else if (result_type.is_signed_int() &&
                    first_op_type.is_signed_int() &&
                    second_op_type.is_signed_int())
            {
                intrin_src << "lo_epi32"; 
            } 
            /* 
               else if (type.is_signed_short_int() ||
               type.is_unsigned_short_int()) 
               { 
               intrin_src << "_epi16"; 
               } 
               else if (type.is_char() || 
               type.is_signed_char() ||
               type.is_unsigned_char()) 
               { 
               intrin_src << "_epi8"; 
               }
             */ 
            else
            {
                running_error("KNC Lowering: Node %s at %s has an unsupported type.", 
                        ast_print_node_type(node.get_kind()),
                        locus_to_str(node.get_locus()));
            }      

            walk(node.get_lhs());
            walk(node.get_rhs());

            intrin_src << "(";
            intrin_src << as_expression(node.get_lhs());
            intrin_src << ", ";
            intrin_src << as_expression(node.get_rhs());
            intrin_src << ")";

            Nodecl::NodeclBase function_call = 
                    intrin_src.parse_expression(node.retrieve_context());

            node.replace(function_call);
        }    

        void KNCVectorLowering::visit(const Nodecl::VectorDiv& node) 
        { 
            TL::Type type = node.get_type().basic_type();

            TL::Source intrin_src;

            // Intrinsic name
            intrin_src << "_mm512_div";

            // Postfix
            if (type.is_float()) 
            { 
                intrin_src << "_ps"; 
            } 
            else if (type.is_double()) 
            { 
                intrin_src << "_pd"; 
            } 
            else if (type.is_signed_int() ||
                    type.is_unsigned_int()) 
            { 
                intrin_src << "_epi32"; 
            } 
            else
            {
                running_error("KNC Lowering: Node %s at %s has an unsupported type.", 
                        ast_print_node_type(node.get_kind()),
                        locus_to_str(node.get_locus()));
            }      

            walk(node.get_lhs());
            walk(node.get_rhs());

            intrin_src << "(";
            intrin_src << as_expression(node.get_lhs());
            intrin_src << ", ";
            intrin_src << as_expression(node.get_rhs());
            intrin_src << ")";

            Nodecl::NodeclBase function_call = 
                    intrin_src.parse_expression(node.retrieve_context());

            node.replace(function_call);
        }                                                 

        void KNCVectorLowering::visit(const Nodecl::VectorLowerThan& node) 
        { 
            TL::Type type = node.get_lhs().get_type().basic_type();

            TL::Source intrin_src;

            // Intrinsic name
            intrin_src << "_mm512_cmplt";

            // Postfix
            if (type.is_float()) 
            { 
                intrin_src << "_ps"; 
            } 
            else if (type.is_signed_int() ||
                    type.is_unsigned_int()) 
            { 
                intrin_src << "_epi32"; 
            } 
            else
            {
                running_error("KNC Lowering: Node %s at %s has an unsupported type.", 
                        ast_print_node_type(node.get_kind()),
                        locus_to_str(node.get_locus()));
            }      

            walk(node.get_lhs());
            walk(node.get_rhs());

            intrin_src << "(";
            intrin_src << as_expression(node.get_lhs());
            intrin_src << ", ";
            intrin_src << as_expression(node.get_rhs());
            intrin_src << ")";

            Nodecl::NodeclBase function_call = 
                    intrin_src.parse_expression(node.retrieve_context());

            node.replace(function_call);
        }                                                 

        void KNCVectorLowering::visit(const Nodecl::VectorGreaterThan& node) 
        { 
            TL::Type type = node.get_lhs().get_type().basic_type();

            TL::Source intrin_src;

            // Intrinsic name
            intrin_src << "_mm512_cmpgt";

            // Postfix
            if (type.is_float()) 
            { 
                intrin_src << "_ps"; 
            } 
            else if (type.is_signed_int() ||
                    type.is_unsigned_int()) 
            { 
                intrin_src << "_epi32"; 
            } 
            else
            {
                running_error("KNC Lowering: Node %s at %s has an unsupported type.", 
                        ast_print_node_type(node.get_kind()),
                        locus_to_str(node.get_locus()));
            }      

            walk(node.get_lhs());
            walk(node.get_rhs());

            intrin_src << "(";
            intrin_src << as_expression(node.get_lhs());
            intrin_src << ", ";
            intrin_src << as_expression(node.get_rhs());
            intrin_src << ")";

            Nodecl::NodeclBase function_call = 
                    intrin_src.parse_expression(node.retrieve_context());

            node.replace(function_call);
        }                                                 

        void KNCVectorLowering::visit(const Nodecl::VectorEqual& node) 
        { 
            TL::Type type = node.get_lhs().get_type().basic_type();

            TL::Source intrin_src;

            // Intrinsic name
            intrin_src << "_mm512_cmpeq";

            // Postfix
            if (type.is_float()) 
            { 
                intrin_src << "_ps"; 
            } 
            else if (type.is_signed_int() ||
                    type.is_unsigned_int()) 
            { 
                intrin_src << "_epi32"; 
            } 
            else
            {
                running_error("KNC Lowering: Node %s at %s has an unsupported type.", 
                        ast_print_node_type(node.get_kind()),
                        locus_to_str(node.get_locus()));
            }      

            walk(node.get_lhs());
            walk(node.get_rhs());

            intrin_src << "(";
            intrin_src << as_expression(node.get_lhs());
            intrin_src << ", ";
            intrin_src << as_expression(node.get_rhs());
            intrin_src << ")";

            Nodecl::NodeclBase function_call =
                intrin_src.parse_expression(node.retrieve_context());

            node.replace(function_call);
        }                                                 

        void KNCVectorLowering::visit(const Nodecl::VectorBitwiseAnd& node) 
        { 
            TL::Type type = node.get_type().basic_type();

            TL::Source intrin_src;
            TL::Source casting_src;

            // Postfix
            if (type.is_float()) 
            { 
                casting_src << "_mm512_castps_si512i(";
            } 
            else if (type.is_integral_type())
            { 
                casting_src << "(";
            } 
            else
            {
                running_error("KNC Lowering: Node %s at %s has an unsupported type.", 
                        ast_print_node_type(node.get_kind()),
                        locus_to_str(node.get_locus()));
            }      

            walk(node.get_lhs());
            walk(node.get_rhs());
 
            // Intrinsic name
            intrin_src << casting_src << "_mm512_and_epi32";

           
            intrin_src << "(";
            intrin_src << casting_src << as_expression(node.get_lhs()) << ")";
            intrin_src << ", ";
            intrin_src << casting_src << as_expression(node.get_rhs()) << ")";
            intrin_src << "))";

            Nodecl::NodeclBase function_call =
                intrin_src.parse_expression(node.retrieve_context());

            node.replace(function_call);
        }                                                 

        void KNCVectorLowering::visit(const Nodecl::VectorBitwiseOr& node) 
        { 
            TL::Type type = node.get_type().basic_type();

            TL::Source intrin_src;
            TL::Source casting_src;

            // Postfix
            if (type.is_float()) 
            { 
                casting_src << "_mm512_castps_si512(";
            } 
            else if (type.is_integral_type())
            { 
            } 
            else
            {
                running_error("KNC Lowering: Node %s at %s has an unsupported type.", 
                        ast_print_node_type(node.get_kind()),
                        locus_to_str(node.get_locus()));
            }      

            walk(node.get_lhs());
            walk(node.get_rhs());

            // Intrinsic name
            intrin_src << casting_src << "_mm512_or_epi32";


            intrin_src << "(";
            intrin_src << casting_src << as_expression(node.get_lhs()) << ")";
            intrin_src << ", ";
            intrin_src << casting_src << as_expression(node.get_rhs()) << ")";
            intrin_src << "))";

            Nodecl::NodeclBase function_call =
                intrin_src.parse_expression(node.retrieve_context());

            node.replace(function_call);
        }                                                 

        void KNCVectorLowering::visit(const Nodecl::VectorBitwiseXor& node) 
        { 
            TL::Type type = node.get_type().basic_type();

            TL::Source intrin_src;
            TL::Source casting_src;

            // Postfix
            if (type.is_float()) 
            { 
                casting_src << "_mm512_castps_si512(";
            } 
            else if (type.is_integral_type())
            { 
                casting_src << "(";
            }
            else
            {
                running_error("KNC Lowering: Node %s at %s has an unsupported type.", 
                        ast_print_node_type(node.get_kind()),
                        locus_to_str(node.get_locus()));
            }      

            walk(node.get_lhs());
            walk(node.get_rhs());

            // Intrinsic name
            intrin_src << casting_src << "_mm512_xor_epi32";

            intrin_src << "(";
            intrin_src << casting_src << as_expression(node.get_lhs()) << ")";
            intrin_src << ", ";
            intrin_src << casting_src << as_expression(node.get_rhs()) << ")";
            intrin_src << "))";

            Nodecl::NodeclBase function_call =
                intrin_src.parse_expression(node.retrieve_context());

            node.replace(function_call);
        }   

        void KNCVectorLowering::visit(const Nodecl::VectorLogicalOr& node) 
        { 
            running_error("KNC Lowering %s: 'logical or' operation (i.e., operator '||') is not supported in KNC. Try using 'bitwise or' operations (i.e., operator '|') instead if possible.",
                    locus_to_str(node.get_locus()));
        }                                                 

        void KNCVectorLowering::visit(const Nodecl::VectorNeg& node) 
        {
            TL::Type type = node.get_type().basic_type();

            TL::Source intrin_src;

            if (type.is_float()) 
            { 
                intrin_src << "_mm512_castsi512_ps(_mm512_xor_epi32(_mm512_set1_epi32(0x80000000), _mm512_castps_si512( ";
            } 
            else if (type.is_double()) 
            { 
                intrin_src << "_mm512_castsi512_pd(_mm512_xor_epi32(_mm512_set1_epi64(0x8000000000000000LL), _mm512_castpd_si512(";
            }
            else if (type.is_signed_int() ||
                    type.is_unsigned_int())
            {
                intrin_src << "(_mm512_sub_epi32( _mm512_setzero_si512(),";
            }
            else if (type.is_signed_short_int() ||
                    type.is_unsigned_short_int())
            {
                intrin_src << "(_mm512_sub_epi16( _mm512_setzero_si512(),";
            }
            else if (type.is_char() ||
                    type.is_signed_char() ||
                    type.is_unsigned_char())
            {
                intrin_src << "(_mm512_sub_epi8( _mm512_setzero_si512(),";
            }
            else
            {
                running_error("KNC Lowering: Node %s at %s has an unsupported type.", 
                        ast_print_node_type(node.get_kind()),
                        locus_to_str(node.get_locus()));
            } 

            walk(node.get_rhs());

            intrin_src << as_expression(node.get_rhs());
            intrin_src << ")))";

            Nodecl::NodeclBase function_call =
                intrin_src.parse_expression(node.retrieve_context());

            node.replace(function_call);
        }                                                 

        void KNCVectorLowering::visit(const Nodecl::VectorConversion& node) 
        {
            const TL::Type& src_type = node.get_nest().get_type().basic_type().get_unqualified_type();
            const TL::Type& dst_type = node.get_type().basic_type().get_unqualified_type();

            TL::Source intrin_src;

            walk(node.get_nest());

            if (src_type.is_same_type(dst_type))
            {
                node.replace(node.get_nest());
                return;
            }
            else if ((src_type.is_signed_int() && dst_type.is_unsigned_int()) ||
                    (dst_type.is_signed_int() && src_type.is_unsigned_int()) ||
                    (src_type.is_signed_short_int() && dst_type.is_unsigned_short_int()) ||
                    (dst_type.is_signed_short_int() && src_type.is_unsigned_short_int()))
            {
                node.replace(node.get_nest());
                return;
            }
            else if (src_type.is_signed_int() &&
                    dst_type.is_float()) 
            { 
                intrin_src << "_mm512_cvtepi32_ps("; 
                intrin_src << as_expression(node.get_nest());
                intrin_src << ")"; 
            } 
            else if (src_type.is_float() &&
                    dst_type.is_signed_int()) 
            { 
                // C/C++ requires truncated conversion
                intrin_src << "_mm512_cvttps_epi32("; 
                intrin_src << as_expression(node.get_nest());
                intrin_src << ")"; 
            }
            /*
            else if (src_type.is_float() &&
                    dst_type.is_double()) 
            { 
                intrin_src << "ps_pd"; 
            } 
            else if (src_type.is_double() &&
                    dst_type.is_float()) 
            { 
                intrin_src << "pd_ps"; 
            } 
            else if (src_type.is_float() &&
                    (dst_type.is_signed_char() ||
                     dst_type.is_char())) 
            {
                // Saturated conversion
                intrin_src << "_mm512_packs_epi16("; 
                intrin_src << "_mm512_packs_epi32("; 
                intrin_src << "_mm512_cvttps_epi32("; 
                walk(node.get_nest());
                intrin_src << "),"; 
                intrin_src << "_mm512_castps_si512(";
                walk(node.get_nest());
                intrin_src << ")";
                //intrin_src << "_mm512_undefined_si512()"; 
                intrin_src << "),"; 
                intrin_src << "_mm512_castps_si512(";
                walk(node.get_nest());
                intrin_src << ")";
                //intrin_src << "_mm512_undefined_si512()"; 
                intrin_src << ")"; 
            } 
            else if (src_type.is_float() &&
                    dst_type.is_unsigned_char()) 
            {
                // Saturated conversion
                intrin_src << "_mm512_packus_epi16("; 
                intrin_src << "_mm512_packus_epi32("; 
                intrin_src << "_mm512_cvttps_epi32("; 
                walk(node.get_nest());
                intrin_src << "),"; 
                intrin_src << "_mm512_castps_si512(";
                walk(node.get_nest());
                intrin_src << ")";
                //intrin_src << "_mm512_undefined_si512()"; 
                intrin_src << "),"; 
                intrin_src << "_mm512_castps_si512(";
                walk(node.get_nest());
                //intrin_src << "_mm512_undefined_si512()"; 
                intrin_src << "))"; 
            } 
            else if (src_type.is_signed_int() &&
                    (dst_type.is_signed_char() ||
                     dst_type.is_char())) 
            {
                // Saturated conversion
                intrin_src << "_mm512_packs_epi16("; 
                intrin_src << "_mm512_packs_epi32("; 
                walk(node.get_nest());
                intrin_src << ","; 
                walk(node.get_nest());
                //intrin_src << "_mm512_undefined_si512()"; 
                intrin_src << "),"; 
                walk(node.get_nest());
                //intrin_src << "_mm512_undefined_si512()"; 
                intrin_src << ")"; 
            } 
            else if (src_type.is_signed_int() &&
                    dst_type.is_unsigned_char()) 
            {
                // Saturated conversion
                intrin_src << "_mm512_packus_epi16("; 
                intrin_src << "_mm512_packus_epi32("; 
                walk(node.get_nest());
                intrin_src << ","; 
                walk(node.get_nest());
                //intrin_src << "_mm512_undefined_si512()"; 
                intrin_src << ")"; 
                intrin_src << ",";
                walk(node.get_nest());
                //intrin_src << "_mm512_undefined_si512()"; 
                intrin_src << ")"; 
            }
            */
            else
            {
                fprintf(stderr, "KNC Lowering: Conversion at '%s' is not supported yet: %s\n", 
                        locus_to_str(node.get_locus()),
                        node.get_nest().prettyprint().c_str());
            }   

            Nodecl::NodeclBase function_call =
                intrin_src.parse_expression(node.retrieve_context());

            node.replace(function_call);
        }

        void KNCVectorLowering::visit(const Nodecl::VectorPromotion& node) 
        { 
            TL::Type type = node.get_type().basic_type();

            TL::Source intrin_src;

            // Intrinsic name
            intrin_src << "_mm512_set1";

            // Postfix
            if (type.is_float()) 
            { 
                intrin_src << "_ps"; 
            } 
            else if (type.is_double()) 
            { 
                intrin_src << "_pd"; 
            } 
            else if (type.is_signed_int() ||
                    type.is_unsigned_int()) 
            { 
                intrin_src << "_epi32"; 
            } 
            else if (type.is_signed_short_int() ||
                    type.is_unsigned_short_int()) 
            { 
                intrin_src << "_epi16"; 
            } 
            else if (type.is_char() || 
                    type.is_signed_char() ||
                    type.is_unsigned_char()) 
            { 
                intrin_src << "_epi8"; 
            } 
            else
            {
                running_error("KNC Lowering: Node %s at %s has an unsupported type.", 
                        ast_print_node_type(node.get_kind()),
                        locus_to_str(node.get_locus()));
            }      

            walk(node.get_rhs());
            
            intrin_src << "("; 
            intrin_src << as_expression(node.get_rhs());
            intrin_src << ")"; 

            Nodecl::NodeclBase function_call =
                intrin_src.parse_expression(node.retrieve_context());

            node.replace(function_call);
        }        

        void KNCVectorLowering::visit(const Nodecl::VectorLiteral& node) 
        {
            TL::Type type = node.get_type().basic_type();

            TL::Source intrin_src;

            // Intrinsic name
            intrin_src << "_mm512_set";

            // Postfix
            if (type.is_float()) 
            { 
                intrin_src << "_ps"; 
            } 
            else if (type.is_double()) 
            { 
                intrin_src << "_pd"; 
            } 
            else if (type.is_signed_int() ||
                    type.is_unsigned_int()) 
            { 
                intrin_src << "_epi32"; 
            } 
            else if (type.is_signed_short_int() ||
                    type.is_unsigned_short_int()) 
            { 
                intrin_src << "_epi16"; 
            } 
            else if (type.is_char() || 
                    type.is_signed_char() ||
                    type.is_unsigned_char()) 
            { 
                intrin_src << "_epi8"; 
            } 
            else
            {
                running_error("KNC Lowering: Node %s at %s has an unsupported type.", 
                        ast_print_node_type(node.get_kind()),
                        locus_to_str(node.get_locus()));
            }      

            intrin_src << "(";

            Nodecl::List scalar_values =
                node.get_scalar_values().as<Nodecl::List>();

            Nodecl::List::const_iterator it = scalar_values.begin();
            walk((*it));
            intrin_src << as_expression(*it);
            it++;

            for (; it != scalar_values.end();
                    it++)
            {
                intrin_src << ", ";
                walk((*it));
                intrin_src << as_expression(*it);
            }

            intrin_src << ")"; 
        }        


        void KNCVectorLowering::visit(const Nodecl::VectorConditionalExpression& node) 
        { 
            TL::Type type = node.get_type().basic_type();

            TL::Source intrin_src;

            Nodecl::NodeclBase true_node = node.get_true();
            Nodecl::NodeclBase false_node = node.get_false();
            Nodecl::NodeclBase condition_node = node.get_condition();

            TL::Type true_type = true_node.get_type().basic_type();
            TL::Type false_type = false_node.get_type().basic_type();
            TL::Type condiition_type = condition_node.get_type();

            std::string casting;

            // Intrinsic name
            intrin_src << "_mm512_blend";

            // Postfix
            if (true_type.is_integral_type()
                    && false_type.is_integral_type())
            {
                // TODO _epi16
                intrin_src << "v_epi8";
            }
            else if (true_type.is_float()
                    && false_type.is_float())
            {
                // TODO _ps
                intrin_src << "v_ps";

                casting = "(";
                casting += print_type_str(TL::Type::get_float_type().get_vector_to(16).get_internal_type(),
                        node.retrieve_context().get_decl_context());
                casting += ")";
            }
            else if (true_type.is_double()
                    && false_type.is_double())
            {
                // TODO _pd
                intrin_src << "v_pd";
                casting = "(";
                casting += print_type_str(TL::Type::get_double_type().get_vector_to(16).get_internal_type(),
                        node.retrieve_context().get_decl_context());
                casting += ")";
            }
            else
            {
                running_error("KNC Lowering: Node %s at %s has an unsupported type.", 
                        ast_print_node_type(node.get_kind()),
                        locus_to_str(node.get_locus()));
            }

            walk(false_node);
            walk(true_node);
            walk(condition_node);

            intrin_src << "("; 
            intrin_src << as_expression(false_node); // False first!
            intrin_src << ", ";
            intrin_src << as_expression(true_node);
            intrin_src << ", "
                << casting;
            intrin_src << as_expression(condition_node);
            intrin_src << ")"; 

            Nodecl::NodeclBase function_call =
                intrin_src.parse_expression(node.retrieve_context());

            node.replace(function_call);
        }        

        void KNCVectorLowering::visit(const Nodecl::VectorAssignment& node) 
        {
            TL::Source intrin_src;

            walk(node.get_lhs());
            walk(node.get_rhs());

            intrin_src << as_expression(node.get_lhs());
            intrin_src << " = ";
            intrin_src << as_expression(node.get_rhs());

            Nodecl::NodeclBase function_call =
                intrin_src.parse_expression(node.retrieve_context());

            node.replace(function_call);
        }                                                 

        void KNCVectorLowering::visit(const Nodecl::VectorLoad& node) 
        { 
            TL::Type type = node.get_type().basic_type();

            TL::Source intrin_src;

            // Intrinsic name
            intrin_src << "_mm512_load";

            // Postfix
            if (type.is_float()) 
            { 
                intrin_src << "_ps("; 
            } 
            else if (type.is_double()) 
            { 
                intrin_src << "_pd("; 
            } 
            else if (type.is_integral_type()) 
            { 
                intrin_src << "_epi32(("; 
                intrin_src << print_type_str(
                        TL::Type::get_void_type().get_pointer_to().get_internal_type(),
                        node.retrieve_context().get_decl_context());

                intrin_src << ")"; 
            } 
            else
            {
                running_error("KNC Lowering: Node %s at %s has an unsupported type.", 
                        ast_print_node_type(node.get_kind()),
                        locus_to_str(node.get_locus()));
            }

            walk(node.get_rhs());

            intrin_src << as_expression(node.get_rhs());
            intrin_src << ")"; 

            Nodecl::NodeclBase function_call =
                intrin_src.parse_expression(node.retrieve_context());

            node.replace(function_call);
        }

        void KNCVectorLowering::visit(const Nodecl::VectorStore& node) 
        { 
            TL::Type type = node.get_lhs().get_type().basic_type();

            TL::Source intrin_src;

            // Intrinsic name
            intrin_src << "_mm512_store";

            // Postfix
            if (type.is_float()) 
            { 
                intrin_src << "_ps("; 
            } 
            else if (type.is_double()) 
            { 
                intrin_src << "_pd("; 
            } 
            else if (type.is_integral_type()) 
            { 
                intrin_src << "_epi32((";
                intrin_src << print_type_str(
                        TL::Type::get_void_type().get_pointer_to().get_internal_type(),
                        node.retrieve_context().get_decl_context());
                intrin_src << ")";
            } 
            else
            {
                running_error("KNC Lowering: Node %s at %s has an unsupported type.", 
                        ast_print_node_type(node.get_kind()),
                        locus_to_str(node.get_locus()));
            }

            walk(node.get_lhs());
            walk(node.get_rhs());

            intrin_src << as_expression(node.get_lhs());
            intrin_src << ", ";
            intrin_src << as_expression(node.get_rhs());
            intrin_src << ")"; 

            Nodecl::NodeclBase function_call =
                intrin_src.parse_expression(node.retrieve_context());

            node.replace(function_call);
        }

        void KNCVectorLowering::visit(const Nodecl::VectorGather& node) 
        { 
            TL::Type type = node.get_type().basic_type();
            TL::Type index_type = node.get_strides().get_type().basic_type();

            TL::Source intrin_src;

            // Intrinsic name
            intrin_src << "_mm512_i32gather";

            std::string extract;

            // Postfix
            if (type.is_float()) 
            { 
                intrin_src << "_ps";
            } 
            else if (type.is_signed_int() || type.is_unsigned_int()) 
            { 
                intrin_src << "_epi32";
            }
            else
            {
                running_error("KNC Lowering: Node %s at %s has an unsupported type.", 
                        ast_print_node_type(node.get_kind()),
                        locus_to_str(node.get_locus()));
            }

            if (!index_type.is_signed_int() && !index_type.is_unsigned_int()) 
            {
                running_error("KNC Lowering: Node %s at %s has an unsupported type.", 
                        ast_print_node_type(node.get_kind()),
                        locus_to_str(node.get_locus()));
            }

            walk(node.get_base());
            walk(node.get_strides());

            intrin_src << "("; 

            intrin_src << as_expression(node.get_base());
            intrin_src << ", ";
            intrin_src << as_expression(node.get_strides());
            intrin_src << ", ";
            intrin_src << type.get_size();

            intrin_src << ")";

            
            Nodecl::NodeclBase function_call =
                intrin_src.parse_expression(node.retrieve_context());

            node.replace(function_call);
        }

        void KNCVectorLowering::visit(const Nodecl::VectorScatter& node) 
        { 
            TL::Type type = node.get_source().get_type().basic_type();
            TL::Type index_type = node.get_strides().get_type().basic_type();

            TL::Source intrin_src;
            std::string extract_index;
            std::string extract_source;

            // Intrinsic name
            intrin_src << "_mm512_i32scatter";

            // Indexes
            if (!index_type.is_signed_int() && !index_type.is_unsigned_int()) 
            { 
                running_error("KNC Lowering: Node %s at %s has an unsupported index type.", 
                        ast_print_node_type(node.get_kind()),
                        locus_to_str(node.get_locus()));
            }

            // Source
            if (type.is_float()) 
            { 
                intrin_src << "_ps";
            } 
            else if (type.is_signed_int() || type.is_unsigned_int()) 
            { 
                intrin_src << "_epi32";
            }
            else
            {
                running_error("KNC Lowering: Node %s at %s has an unsupported source type.", 
                        ast_print_node_type(node.get_kind()),
                        locus_to_str(node.get_locus()));
            }

            walk(node.get_base());
            walk(node.get_strides());
            walk(node.get_source());

            intrin_src << "(";

            intrin_src << as_expression(node.get_base());
            intrin_src << ", ";
            intrin_src << as_expression(node.get_strides());
            intrin_src << ", ";
            intrin_src << as_expression(node.get_source());
            intrin_src << ", ";
            intrin_src << type.get_size();

            intrin_src << ")";


            Nodecl::NodeclBase function_call =
                intrin_src.parse_expression(node.retrieve_context());

            node.replace(function_call);
        }

        void KNCVectorLowering::visit(const Nodecl::VectorFunctionCall& node) 
        {
            walk(node.get_arguments());

            Nodecl::FunctionCall function_call =
                Nodecl::FunctionCall::make(
                        node.get_called(),
                        node.get_arguments(),
                        node.get_alternate_name(),
                        node.get_function_form(),
                        node.get_type(),
                        node.get_locus());

            node.replace(function_call);
        }

        void KNCVectorLowering::visit(const Nodecl::VectorFabs& node) 
        {
            TL::Type type = node.get_type().basic_type();

            TL::Source intrin_src;

            walk(node.get_argument());

            // Handcoded implementations for float and double
            if (type.is_float()) 
            { 
                intrin_src << "_mm512_castsi512_ps(_mm512_and_epi32(_mm512_castps_si512(";
                intrin_src << as_expression(node.get_argument());
                intrin_src << "), _mm512_set1_epi32(0x7FFFFFFF)))"; 
            } 
            else if (type.is_double()) 
            { 
                intrin_src << "_mm512_castsi512_pd(_mm512_and_epi32(_mm512_castpd_si512(";
                intrin_src << as_expression(node.get_argument());
                intrin_src << "), _mm512_set1_epi64(0x7FFFFFFFFFFFFFFFLL)))"; 
            }
            else
            {
                /*
                // Intrinsic name
                intrin_src << "_mm512_abs";

                // Postfix
                if (type.is_signed_int() ||
                        type.is_unsigned_int()) 
                { 
                    intrin_src << "_epi32"; 
                } 
                else if (type.is_signed_short_int() ||
                        type.is_unsigned_short_int()) 
                { 
                    intrin_src << "_epi16"; 
                } 
                else if (type.is_char() || 
                        type.is_signed_char() ||
                        type.is_unsigned_char()) 
                { 
                    intrin_src << "_epi8"; 
                } 
                else
                {
                */
                    running_error("KNC Lowering: Node %s at %s has an unsupported type.", 
                            ast_print_node_type(node.get_kind()),
                            locus_to_str(node.get_locus()));
                //}
            }
            
            Nodecl::NodeclBase function_call =
                intrin_src.parse_expression(node.retrieve_context());
            
            node.replace(function_call);
        }

        Nodecl::NodeclVisitor<void>::Ret KNCVectorLowering::unhandled_node(const Nodecl::NodeclBase& n) 
        { 
            fprintf(stderr, "KNC Lowering: Unknown node %s at %s.\n",
                    ast_print_node_type(n.get_kind()),
                    locus_to_str(n.get_locus())); 
            /*
               running_error("KNC Lowering: Unknown node %s at %s.",
               ast_print_node_type(n.get_kind()),
               locus_to_str(n.get_locus())); 
             */
            return Ret(); 
        }
    }
}
