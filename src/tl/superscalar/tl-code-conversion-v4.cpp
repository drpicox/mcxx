/*
    Cell/SMP superscalar Compiler
    Copyright (C) 2007 Barcelona Supercomputing Center

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; version 2.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include <sstream>

#include "tl-augmented-symbol.hpp"
#include "tl-code-conversion-v4.hpp"
#include "tl-parameter-expression.hpp"
#include "tl-type-utils.hpp"


namespace TL
{
	CompilerPhase::PhaseStatus CodeConversion::_status;
	
	
	void CodeConversion::TaskCallHandler::preorder(Context ctx, AST_t node)
	{
	}
	
	
	void CodeConversion::TaskCallHandler::postorder(Context ctx, AST_t node)
	{
		ScopeLink scope_link = ctx.scope_link;
		Expression function_call(node, ctx.scope_link);
		
		if (!function_call.is_function_call())
		{
			std::cerr << __FILE__ << ":" << __LINE__ << ": " << "Internal compiler error" << std::endl;
			throw FatalException();
		}
		
		Expression function_called_expresion = function_call.get_called_expression();
		if (!function_called_expresion.is_id_expression())
		{
			// We do not handle indirect calls (through variables)
			return;
		}
		
		AugmentedSymbol symbol = function_called_expresion.get_id_expression().get_symbol();
		if (!symbol.is_task())
		{
			return;
		}
		
		Type type = symbol.get_type();
		ObjectList<Type> parameter_types = type.nonadjusted_parameters();
		RefPtr<ParameterRegionList> parameter_region_list = symbol.get_parameter_region_list();
		std::string function_name = symbol.get_qualified_name();
		
		ObjectList<Expression> arguments = function_call.get_argument_list();
		if (arguments.size() != parameter_types.size())
		{
			std::cerr << function_call.get_ast().get_locus() << " Error: Call to function '" << function_name << "' with incorrect number of parameters." << std::endl;
			CodeConversion::fail();
			return;
		}
		
		if (parameter_region_list->size() != parameter_types.size())
		{
			std::cerr << __FILE__ << ":" << __LINE__ << ": " << "Internal compiler error" << std::endl;
			throw FatalException();
		}
		
		// Get and check its corresponding statement expression
		Expression top_expression = function_call.get_top_enclosing_expression();
		AST_t statement_expression_ast = top_expression.original_tree();
		PredicateAST<LANG_IS_EXPRESSION_STATEMENT> expression_statement_predicate;
		if (!expression_statement_predicate(statement_expression_ast))
		{
			std::cerr << function_call.get_ast().get_locus() << " Error: Call to task '" << function_name << "' cannot return a value." << std::endl;
			CodeConversion::fail();
			return;
		}
		
		
		Source source;
		Source parameter_initializers_source;
		Source constant_redirection_source;
		Source parameter_region_source;
		Source add_task_code;
		
		source
			<< "{"
				<< constant_redirection_source
				<< parameter_region_source
				<< "css_parameter_t const parameters__cssgenerated[] = {" << parameter_initializers_source << "};"
				<< add_task_code
			<< "}";
		
		add_task_code
			<< "css_addTask("
				<< function_name << "_task_id__cssgenerated" << ", "
				<< (symbol.has_high_priority() ? "1" : "0") << ", "
				<< parameter_types.size() << ", "
				<< "parameters__cssgenerated" << ");";
		
		for (unsigned int index = 0; index < arguments.size(); index++)
		{
			Source direction_source;
			Source dimension_count_source;
			Source size_source;
			Source address_source;
			Source dimensions_source;
			
			parameter_initializers_source
				<< "{"
					<< direction_source
					<< ", " << dimension_count_source
					<< ", " << size_source
					<< ", " << address_source
					<< ", " << dimensions_source
				<< "}, ";
			
			Expression argument = arguments[index];
			RegionList region_list = (*parameter_region_list.get_pointer())[index];
			if (region_list.size() != 1)
			{
				std::cerr << function_call.get_ast().get_locus() << " Error: In call to task '" << function_name << "'. Sorry, this compiler version does not support more than one range per parameter." << std::endl;
				CodeConversion::fail();
				return;
			}
			
			Region &region = region_list[0];
			switch (region.get_direction())
			{
				case Region::INPUT_DIR:
					direction_source << "CSS_IN_DIR";
					break;
				case Region::OUTPUT_DIR:
					direction_source << "CSS_OUT_DIR";
					break;
				case Region::INOUT_DIR:
					direction_source << "CSS_INOUT_DIR";
					break;
				case Region::UNKNOWN_DIR:
					std::cerr << __FILE__ << ":" << __LINE__ << "Internal compiler error." << std::cerr;
					throw FatalException();
			}
			
			bool is_lvalue;
			Type argument_type = argument.get_type(is_lvalue);
			Type parameter_type = parameter_types[index];
			
			dimension_count_source << region.get_dimension_count();
			
			if (region.get_dimension_count() != 0)
			{
				//
				// An array
				//
				
				parameter_region_source
					<< "css_parameter_dimension_t parameter_" << index << "_dimension__cssgenerated[" << region.get_dimension_count() << "] = {";
				
				// The first dimension is base 1 (in terms of bytes)
				{
					Source dimension_base;
					dimension_base
						<< "sizeof("
							<< (
									parameter_type.is_pointer()
									? parameter_type.points_to()
									: TypeUtils::get_array_element_type(parameter_type, scope_link)
								).get_declaration(ctx.scope_link.get_scope(argument.get_ast()), std::string(""))
						<< ")";
					
					Expression parametrized_dimension_length = region[0].get_dimension_length();
					Expression parametrized_dimension_start = region[0].get_dimension_start();
					Expression parametrized_accessed_length = region[0].get_accessed_length();
					
					ParameterExpression::substitute(parametrized_dimension_length, arguments, scope_link);
					ParameterExpression::substitute(parametrized_dimension_start, arguments, scope_link);
					ParameterExpression::substitute(parametrized_accessed_length, arguments, scope_link);
					
					parameter_region_source
						<< "{"
							// Size
							<< dimension_base << " * " << "(" << parametrized_dimension_length.prettyprint() << ")"
							// Lower bound
							<< ", " << dimension_base << " * " << "(" << parametrized_dimension_start.prettyprint() << ")"
							// Upper bound
							<< ", " << dimension_base << " * " << "(" << parametrized_accessed_length.prettyprint() << ")"
						<< "}";
				}
				
				// The rest of the dimensions are automatically based on the previous one
				for (unsigned int dimension_index = 1; dimension_index < region.get_dimension_count(); dimension_index++)
				{
					Expression parametrized_dimension_length = region[dimension_index].get_dimension_length();
					Expression parametrized_dimension_start = region[dimension_index].get_dimension_start();
					Expression parametrized_accessed_length = region[dimension_index].get_accessed_length();
					
					ParameterExpression::substitute(parametrized_dimension_length, arguments, scope_link);
					ParameterExpression::substitute(parametrized_dimension_start, arguments, scope_link);
					ParameterExpression::substitute(parametrized_accessed_length, arguments, scope_link);
					
					parameter_region_source
						<< ", {"
							// Size
							<< parametrized_dimension_length.prettyprint()
							// Lower bound
							<< ", " << parametrized_dimension_start.prettyprint()
							// Upper bound
							<< ", " << parametrized_accessed_length.prettyprint()
						<< "}";
				}
				
				parameter_region_source
					<< "};";
				
				size_source
					<< "0";
				address_source
					<< argument.prettyprint();
				dimensions_source
					<< "parameter_" << index << "_dimension__cssgenerated";
				
			}
			else if (parameter_type.is_pointer())
			{
				//
				// A pointer
				//
				
				if (!argument_type.is_pointer() && !argument_type.is_array())
				{
					std::cerr << argument.get_ast().get_locus() << " Error: expression '" << argument.prettyprint() << "' passed as parameter number " << index+1 << " in call to task '" << function_name << "' must be a pointer or an array." << std::endl;
					CodeConversion::fail();
					return;
				}
				
				Type base_type = parameter_type.points_to();
				if (base_type.is_void())
				{
					// An opaque parameter, we should pass a pointer to it, since it is treated as a scalar
					if (region.get_direction() == Region::INPUT_DIR)
					{
						direction_source = Source("CSS_IN_SCALAR_DIR");
					}
					size_source
						<< "sizeof(void *)";
					
					std::ostringstream temporary_name;
					temporary_name << "parameter_" << index << "__cssgenerated";
					
					constant_redirection_source
						<< "void *" << temporary_name.str() << " = " << argument.prettyprint() << ";";
					address_source
						<< "&" << temporary_name.str();
					dimensions_source
						<< "(void *)0"; // NULL
				}
				else if (argument.is_literal())
				{
					if (region.get_direction() == Region::INPUT_DIR)
					{
						direction_source = Source("CSS_IN_SCALAR_DIR");
					}
					else
					{
						std::cerr << argument.get_ast().get_locus() << " Error: expression '" << argument.prettyprint() << "' passed as parameter number " << index+1 << " in call to task '" << function_name << "' is a literal passed as either output or inout." << std::endl;
						CodeConversion::fail();
						return;
					}
					// A "string" or an L"string"
					if (!base_type.is_char() && !base_type.is_wchar_t())
					{
						std::cerr << __FILE__ << ":" << __LINE__ << ": Internal compiler error" << std::endl;
						throw FatalException();
					}
					size_source
						<< "sizeof("
							<< base_type.get_declaration(scope_link.get_scope(argument.get_ast()), std::string(""))
						<< ")";
					address_source
						<< argument.prettyprint();
					dimensions_source
						<< "(void *)0"; // NULL
				}
				else
				{
					// A struct
					size_source
						<< "sizeof("
							<< base_type.get_declaration(scope_link.get_scope(argument.get_ast()), std::string(""))
						<< ")";
					address_source
						<< argument.prettyprint();
					dimensions_source
						<< "(void *)0"; // NULL
					
				}
			}
			else if (parameter_type.is_non_derived_type())
			{
				// A scalar
				if (region.get_direction() != Region::INPUT_DIR) {
					std::cerr << "Internal compiler error at " << __FILE__ << ":" << __LINE__ << std::endl;
					throw FatalException();
				}
				
				// Must not be a pointer but we have to pass its address anyway
				if (argument_type.is_pointer() || argument_type.is_array())
				{
					std::cerr << argument.get_ast().get_locus() << " Error: expression '" << argument.prettyprint() << "' passed as parameter number " << index+1 << " in call to task '" << function_name << "' can neither be a pointer nor an array." << std::endl;
					CodeConversion::fail();
					return;
				}
				
				if (region.get_direction() == Region::INPUT_DIR)
				{
					direction_source = Source("CSS_IN_SCALAR_DIR");
				}
				size_source
					<< "sizeof("
						<< parameter_type.get_declaration(ctx.scope_link.get_scope(argument.get_ast()), std::string(""))
					<< ")";
				
				if (is_lvalue && argument_type.is_same_type(parameter_type))
				{
					address_source
						<< "&(" << argument.prettyprint() << ")";
				}
				else
				{
					std::ostringstream temporary_name;
					temporary_name << "parameter_" << index << "__cssgenerated";
					constant_redirection_source
						<< parameter_type.get_declaration_with_initializer(
							ctx.scope_link.get_scope(argument.get_ast()),
							temporary_name.str(),
							argument.prettyprint()
						)
						<< ";";
					address_source
						<< "&" << temporary_name.str();
				}
				
				dimensions_source
					<< "(void *)0"; // NULL
				
			}
			else
			{
				// A derived type passed by value
				std::cerr << "Internal compiler error at " << __FILE__ << ":" << __LINE__ << std::endl;
				throw FatalException();
			}
		}
		
		AST_t tree = source.parse_statement(node, ctx.scope_link);
		// The replacement must be performed on the statement expression, since the replacement is a statement
		statement_expression_ast.replace(tree);
	}
	
	
	void CodeConversion::FunctionDefinitionHandler::handle_function_body(AST_t node, ScopeLink scope_link)
	{
		if (_do_traverse)
		{
			_body_traverser.traverse(node, scope_link);
		}
	}
	
	
	void CodeConversion::FunctionDefinitionHandler::preorder(Context ctx, AST_t node)
	{
	}
	
	
	void CodeConversion::FunctionDefinitionHandler::postorder(Context ctx, AST_t node)
	{
		FunctionDefinition function_definition(node, ctx.scope_link);
		AugmentedSymbol symbol = function_definition.get_function_name().get_symbol();
		
		std::string function_name = symbol.get_name();
		
		if (function_name.find("__cssgenerated") != std::string::npos) {
			// An adapter or similar generated code
			return;
		}
		
		// Handle non task function definitions
		if (!symbol.is_task())
		{
			if ( ((bool)symbol.is_on_task_side() && _generate_task_side) ||
				((bool)symbol.is_on_non_task_side() && _generate_non_task_side) )
			{
				// Generate the function definition
				handle_function_body(node, ctx.scope_link);
			}
			else
			{
				// Do not generate it
				_kill_list.push_back(node);
			}
			return;
		}
		
		// Erase the task if not generating task code
		if (!_generate_task_side)
		{
			// Do not generate the task definition
			_kill_list.push_back(node);
			return;
		}
		
	}
	
	
	void CodeConversion::DeclarationHandler::preorder(Context ctx, AST_t node)
	{
	}
	
	
	void CodeConversion::DeclarationHandler::postorder(Context ctx, AST_t node)
	{
		Declaration general_declaration(node, ctx.scope_link);
		
		bool is_extern = TypeUtils::is_extern_declaration(general_declaration);
		
		ObjectList<DeclaredEntity> declared_entities = general_declaration.get_declared_entities();
		int total_entities = declared_entities.size();
		
		if (total_entities > 0)
		{
			DeclaredEntity &first_entity = *(declared_entities.begin());
			Symbol symbol = first_entity.get_declared_symbol();
			Type declaration_type = symbol.get_type();
			
			// Handle all the declarators
			for (ObjectList<DeclaredEntity>::iterator it = declared_entities.begin(); it != declared_entities.end(); it++)
			{
				DeclaredEntity &entity = *it;
				AugmentedSymbol symbol = entity.get_declared_symbol();
				
				if (symbol.is_function())
				{
					std::string function_name = symbol.get_qualified_name();
					
					if (symbol.is_task())
					{
						if (!_generate_task_side)
						{
							entity.get_ast().remove_in_list();
							total_entities--;
						}
					}
					else
					{
						if ( ((bool)symbol.is_on_task_side() && _generate_task_side) ||
							((bool)symbol.is_on_non_task_side() && _generate_non_task_side) )
						{
							// Generate the declaration
						}
						else
						{
							// Do not generate it
							entity.get_ast().remove_in_list();
							total_entities--;
						}
					}
				}
				else
				{
					// Not a function
					// Get the real type
					if (symbol.is_variable() && !is_extern)
					{
						if (!_generate_non_task_side)
						{
							entity.get_ast().remove_in_list();
							total_entities--;
						}
					}
				}
			} // For each declared entity
			
			if (total_entities == 0)
			{
				// All declarators have been removed
				// Leave empty declaration for non builtin types, otherwise remove the declaration
				if (TypeUtils::get_basic_type(declaration_type).is_non_derived_type() || TypeUtils::get_basic_type(declaration_type).is_function())
				{
					_kill_list.push_back(node);
				}
			}
		} // If it had any declared entity
	}
	
	
	void CodeConversion::MallocHandler::preorder(Context ctx, AST_t node)
	{
	}
	
	
	void CodeConversion::MallocHandler::postorder(Context ctx, AST_t node)
	{
		Expression function_call(node, ctx.scope_link);
		
		Expression called_expresion = function_call.get_called_expression();
		ObjectList<Expression> arguments = function_call.get_argument_list();
		
		Source source;
		
		if (arguments.size() != 1)
		{
			std::cerr << function_call.get_ast().get_locus() << " Error: Invalid number of arguments in call to malloc." << std::endl;
			CodeConversion::fail();
			return;
		}
		
		ObjectList<Expression>::iterator it = arguments.begin();
		source
			<< "css_aligned_malloc" << "("
				<< it->prettyprint()
			<< ")";
		
		AST_t tree = source.parse_expression(node, ctx.scope_link);
		node.replace_with(tree);
	}
	
	
	void CodeConversion::CallocHandler::preorder(Context ctx, AST_t node)
	{
	}
	
	
	void CodeConversion::CallocHandler::postorder(Context ctx, AST_t node)
	{
		Expression function_call(node, ctx.scope_link);
		
		Expression called_expresion = function_call.get_called_expression();
		ObjectList<Expression> arguments = function_call.get_argument_list();
		
		Source source;
		
		if (arguments.size() != 2)
		{
			std::cerr << function_call.get_ast().get_locus() << " Error: Invalid number of arguments in call to calloc." << std::endl;
			CodeConversion::fail();
			return;
		}
		
		ObjectList<Expression>::iterator it = arguments.begin();
		source
			<< "css_aligned_calloc" << "("
				<< it->prettyprint();
		it++;
		source
				<< ", " << it->prettyprint()
			<< ")";
		
		AST_t tree = source.parse_expression(node, ctx.scope_link);
		node.replace_with(tree);
	}
	
	
	void CodeConversion::generate_task_ids_and_adapters(AST_t translation_unit, ScopeLink scope_link, bool generate_task_side, bool generate_non_task_side)
	{
		std::set<AugmentedSymbol> task_table;
		
		ObjectList<AST_t> function_definitions = translation_unit.depth_subtrees(FunctionDefinition::predicate);
		for (ObjectList<AST_t>::iterator it = function_definitions.begin(); it != function_definitions.end(); it++)
		{
			FunctionDefinition function_definition(*it, scope_link);
			AugmentedSymbol symbol = function_definition.get_function_name().get_symbol();
			if (symbol.is_task())
			{
				task_table.insert(symbol);
			}
		}
		
		ObjectList<AST_t> declared_entities = translation_unit.depth_subtrees(DeclaredEntity::predicate);
		for (ObjectList<AST_t>::iterator it = declared_entities.begin(); it != declared_entities.end(); it++)
		{
			DeclaredEntity declared_entity(*it, scope_link);
			AugmentedSymbol symbol = declared_entity.get_declared_symbol();
			if (symbol.is_task())
			{
				task_table.insert(symbol);
			}
		}
		
		for (std::set<AugmentedSymbol>::iterator it = task_table.begin(); it != task_table.end(); it++)
		{
			AugmentedSymbol symbol = *it;
			
			// Create the task id
			if (generate_non_task_side)
			{
				Source id_declaration_source;
				id_declaration_source
					<< "extern int const " << symbol.get_qualified_name() << "_task_id__cssgenerated" << ";";
				AST_t id_declaration_ast = id_declaration_source.parse_declaration(translation_unit, scope_link);
				translation_unit.prepend_to_translation_unit(id_declaration_ast);
			}
			
			// Generate the task adapter
			if (generate_task_side)
			{
				Source source;
				Source adapter_parameters;
				
				source
					<< "void __attribute__((weak)) " << symbol.get_qualified_name() << "_adapter__cssgenerated" << "(void **parameter_data)"
					<< "{"
						<< symbol.get_qualified_name() << "(" << adapter_parameters << ");"
					<< "}";
				
				Type task_type = symbol.get_type();
				ObjectList<Type> parameter_types = task_type.parameters();
				RefPtr<ParameterRegionList> parameter_region_list = symbol.get_parameter_region_list();
				for (unsigned int index = 0; index < parameter_region_list->size(); index++)
				{
					Type parameter_type = parameter_types[index];
					
					if (index != 0)
					{
						adapter_parameters
							<< ",";
					}
					if ((parameter_type.is_pointer() && !parameter_type.points_to().is_void()) || parameter_type.is_array())
					{
						adapter_parameters
							<< "parameter_data[" << index << "]";
					}
					else
					{
						adapter_parameters
							<< "*("
								<< "(" << parameter_type.get_pointer_to().get_declaration(scope_link.get_scope(symbol.get_point_of_declaration()), "") << ")"
								<< "parameter_data[" << index << "]"
							<< ")";
					}
				}
				
				AST_t tree = source.parse_global(symbol.get_point_of_declaration(), scope_link);
				symbol.get_point_of_declaration().append_to_translation_unit(tree);
			} // _generate_task_side
			
		}
		
		
	}
	
	
	void CodeConversion::run(DTO &dto)
	{
		_status = PHASE_STATUS_OK;
		
		try
		{
			Bool generate_task_side = dto["superscalar_generate_task_side"];
			Bool generate_non_task_side = dto["superscalar_generate_non_task_side"];
			Bool align_memory = dto["superscalar_align_memory"];
			
			AST_t translation_unit = dto["translation_unit"];
			ScopeLink scope_link = dto["scope_link"];
			
			ObjectList<AST_t> kill_list;
			
			
			if (generate_non_task_side)
			{
				generate_task_ids_and_adapters(translation_unit, scope_link, generate_task_side, generate_non_task_side);
			}
			
			DepthTraverse depth_traverse;
			
			// WARNING: order is important since function definitions appear to be also declarations
			PredicateAST<LANG_IS_FUNCTION_DEFINITION> function_definition_predicate;
			TraverseASTPredicate function_definition_traverser(function_definition_predicate, AST_t::NON_RECURSIVE);
			FunctionDefinitionHandler function_definition_handler(kill_list, scope_link, generate_task_side, generate_non_task_side, align_memory);
			depth_traverse.add_functor(function_definition_traverser, function_definition_handler);
			
			PredicateAST<LANG_IS_DECLARATION> declaration_predicate;
			TraverseASTPredicate declaration_traverser(declaration_predicate, AST_t::NON_RECURSIVE);
			DeclarationHandler declaration_handler(kill_list, generate_task_side, generate_non_task_side);
			depth_traverse.add_functor(declaration_traverser, declaration_handler);
			
			depth_traverse.traverse(translation_unit, scope_link);
			
			// Erase nodes from the code
			for (ObjectList<AST_t>::iterator it = kill_list.begin(); it != kill_list.end(); it++)
			{
				AST_t node = *it;
				node.remove_in_list();
			}
		}
		catch (FatalException ex)
		{
			_status = PHASE_STATUS_ERROR;
		}
		
		set_phase_status(_status);
	}

}


EXPORT_PHASE(TL::CodeConversion);
