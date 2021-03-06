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


#include "nanox-opencl.hpp"

#include "cxx-profile.h"
#include "cxx-cexpr.h"
#include "cxx-driver-utils.h"

#include "tl-devices.hpp"
#include "tl-nanos.hpp"
#include "tl-multifile.hpp"
#include "tl-compilerpipeline.hpp"
#include "tl-nodecl-utils-fortran.hpp"
#include "tl-symbol-utils.hpp"

#include "codegen-phase.hpp"

#include <errno.h>

using namespace TL;
using namespace TL::Nanox;

static std::string ocl_outline_name(const std::string & name)
{
    return "ocl_" + name;
}

void DeviceOpenCL::generate_ndrange_code(
        const TL::Symbol& called_task,
        const TL::Symbol& unpacked_function,
        const TargetInformation& target_info,
        const std::string filename,
        const std::string kernel_name,
        const TL::ObjectList<OutlineDataItem*>& data_items,
        Nodecl::Utils::SimpleSymbolMap* called_fun_to_outline_data_map,
        Nodecl::Utils::SymbolMap* outline_data_to_unpacked_fun_map,
        // Out
        TL::Source& code_ndrange)
{
    TL::Source code_ndrange_aux;
    Nodecl::Utils::SimpleSymbolMap called_fun_to_unpacked_fun_map;

    symbol_map_t* outline_data_to_unpacked_fun_map_internal = outline_data_to_unpacked_fun_map->get_symbol_map();
    const std::map<TL::Symbol, TL::Symbol>* called_task_map = called_fun_to_outline_data_map->get_simple_symbol_map();
    for (std::map<TL::Symbol, TL::Symbol>::const_iterator it = called_task_map->begin();
            it != called_task_map->end();
            it++)
    {
        TL::Symbol key = it->first;
        TL::Symbol value =
            outline_data_to_unpacked_fun_map_internal->map(
                    outline_data_to_unpacked_fun_map_internal, it->second.get_internal_symbol());
        called_fun_to_unpacked_fun_map.add_map(key, value);
    }

    // The arguments of the clause 'ndrange' must be updated because they are not
    // expressed in terms of the unpacked arguments
    TL::ObjectList<Nodecl::NodeclBase> new_ndrange, new_shmem;
    TL::ObjectList<Nodecl::NodeclBase> ndrange_args = target_info.get_ndrange();
    TL::ObjectList<Nodecl::NodeclBase> shmem_args = target_info.get_shmem();
    int num_args_ndrange = ndrange_args.size(),
        num_args_shmem = shmem_args.size();
    if (IS_FORTRAN_LANGUAGE)
    {
        for (int i = 0; i < num_args_ndrange; ++i)
        {
            Nodecl::NodeclBase argument = Nodecl::Utils::deep_copy(
                    ndrange_args[i],
                    unpacked_function.get_related_scope(),
                    *called_fun_to_outline_data_map);

            new_ndrange.append(Nodecl::Utils::deep_copy(
                        argument,
                        unpacked_function.get_related_scope(),
                        *outline_data_to_unpacked_fun_map));
        }

        for (int i = 0; i < num_args_shmem; ++i)
        {
            Nodecl::NodeclBase argument = Nodecl::Utils::deep_copy(
                    shmem_args[i],
                    unpacked_function.get_related_scope(),
                    *called_fun_to_outline_data_map);

            new_shmem.append(Nodecl::Utils::deep_copy(
                        argument,
                        unpacked_function.get_related_scope(),
                        *outline_data_to_unpacked_fun_map));
        }
    }
    else
    {
        for (int i = 0; i < num_args_ndrange; ++i)
        {
            new_ndrange.append(Nodecl::Utils::deep_copy(
                        ndrange_args[i],
                        unpacked_function.get_related_scope(),
                        called_fun_to_unpacked_fun_map));
        }

        for (int i = 0; i < num_args_shmem; ++i)
        {
            new_shmem.append(Nodecl::Utils::deep_copy(
                        shmem_args[i],
                        unpacked_function.get_related_scope(),
                        called_fun_to_unpacked_fun_map));
        }
    }

    bool dim_const = new_ndrange[0].is_constant();

    bool check_dim = !(new_ndrange[num_args_ndrange - 1].is_constant()
            && const_value_is_string(new_ndrange[num_args_ndrange - 1].get_constant())
            && (strcmp(const_value_string_unpack_to_string(new_ndrange[num_args_ndrange-1].get_constant()),"noCheckDim") == 0));

    int num_dim = 0;
    if (dim_const)
    {
        num_dim = const_value_cast_to_4(new_ndrange[0].get_constant());

        ERROR_CONDITION(num_dim < 1 || num_dim > 3,
                "invalid number of dimensions for 'ndrange' clause. Valid values: 1, 2 and 3." , 0);

        ERROR_CONDITION((((num_dim * 3) + 1 + !check_dim) != num_args_ndrange)
                && (((num_dim * 2) + 1 + !check_dim) != num_args_ndrange),
                "invalid number of arguments for 'ndrange' clause", 0);
    }

    std::string compiler_opts;
    if (CURRENT_CONFIGURATION->opencl_build_options != NULL)
    {
        compiler_opts = std::string(CURRENT_CONFIGURATION->opencl_build_options);
    }

    //Create OCL Kernel
    code_ndrange_aux << "nanos_err_t err;"
                     << "void* ompss_kernel_ocl = nanos_create_current_kernel(\""
                     <<         kernel_name << "\",\""
                     <<         filename << "\",\""
                     <<         compiler_opts << "\");";

    //Prepare setArgs
    unsigned int index_local = 0;
    TL::ObjectList<TL::Symbol> parameters_called = called_task.get_function_parameters();
    for (unsigned int i = 0; i < parameters_called.size(); ++i)
    {
        TL::Symbol unpacked_argument = called_fun_to_unpacked_fun_map.map(parameters_called[i]);

        // The attribute __global is deduced: the current argument will be __global if it has any copies
        bool is_global = false;
        if (unpacked_argument.get_type().no_ref().is_pointer()
                || unpacked_argument.get_type().no_ref().is_array())
        {
            for (TL::ObjectList<OutlineDataItem*>::const_iterator it = data_items.begin();
                    it != data_items.end() && !is_global;
                    ++it)
            {
                TL::Symbol outline_data_item_sym = (*it)->get_symbol();

                // If the outline data item has not a valid symbol, skip it
                if (!outline_data_item_sym.is_valid())
                    continue;

                // If the symbol of the current outline data item is not the
                // same as the unpacked_argument, skip it
                if(TL::Symbol(outline_data_to_unpacked_fun_map_internal->map(
                                outline_data_to_unpacked_fun_map_internal,
                                outline_data_item_sym.get_internal_symbol())) != unpacked_argument)
                    continue;

                is_global = !((*it)->get_copies().empty());
            }
        }

        bool is_local = !is_global && unpacked_argument.get_type().no_ref().is_pointer();

        if (is_global)
        {
            code_ndrange_aux
                << "err = nanos_opencl_set_bufferarg("
                <<      "ompss_kernel_ocl, "
                <<      i << ", "
                <<      as_symbol(unpacked_argument) <<");";
        }
        else if (is_local)
        {
            TL::Source sizeof_arg;
            if (index_local >= new_shmem.size())
            {
                std::cerr << called_task.get_locus_str()
                    << ": warning: The size of the local symbol '"
                    << unpacked_argument.get_name() << "' has not been specified in the 'shmem' clause, assuming zero" << std::endl;

                sizeof_arg << "0";
            }
            else
            {
                sizeof_arg << as_expression(new_shmem[index_local]);
            }

            code_ndrange_aux << "err = nanos_opencl_set_arg("
                <<      "ompss_kernel_ocl, "
                <<      i << ", "
                <<      sizeof_arg << ", "
                <<      "0);";
            ++index_local;
        }
        else
        {
            code_ndrange_aux << "err = nanos_opencl_set_arg("
                <<      "ompss_kernel_ocl, "
                <<      i << ", "
                <<      "sizeof(" << as_type(unpacked_argument.get_type().no_ref()) << "), "
                <<      "&" << as_symbol(unpacked_argument) <<");";
        }
    }


    //Build arrays with information from ndrange clause or pointing to the ndrange pointers
    if (!dim_const)
    {
        if (IS_FORTRAN_LANGUAGE)
        {
            internal_error("The number of dimensions is non-constant. This feature is not implemented yet in Fortran.", 0);
        }

        //Prepare ndrange calc pointers and arrays
        code_ndrange_aux
            << "int num_dim = " << as_expression(new_ndrange[0]) <<";"
            << "size_t offset_tmp[num_dim];"
            << "size_t offset_arr[num_dim];"
            << "size_t local_size_arr[num_dim];"
            << "size_t global_size_arr[num_dim];"
            << "size_t* local_size_ptr;"
            << "size_t* offset_ptr;"
            << "size_t* global_size_ptr;"
            << "size_t* final_local_size_ptr;"
            << as_type(TL::Type::get_bool_type()) << " local_size_zero = 0;"
            << "int i = 0;"
            ;
        if (num_args_ndrange == 3)
        {
            code_ndrange_aux
                << "for (i = 0; i < num_dim; ++i)"
                << "{"
                <<     "offset_tmp[i] = 0;"
                << "}"
                << "offset_ptr = offset_tmp;"
                << "global_size_ptr = " << as_expression(new_ndrange[1]) << ";"
                << "local_size_ptr = " << as_expression(new_ndrange[2]) << ";"
                ;
        }
        else if (num_args_ndrange == 4)
        {
            code_ndrange_aux
                << "offset_ptr = " << as_expression(new_ndrange[1]) << ";"
                << "global_size_ptr = " << as_expression(new_ndrange[2]) << ";"
                << "local_size_ptr = " << as_expression(new_ndrange[3]) << ";"
                ;
        }
        else
        {
            WARNING_MESSAGE("Invalid number of parameters for ndrange, when number of dimensions is not const, it must be 3 or 4",0);
        }

        //Check if local_size has zeros
        code_ndrange_aux
            << "for (i = 0; i < num_dim; ++i)"
            << "{"
            <<     "if (local_size_ptr[i] == 0)"
            <<     "{"
            <<         "local_size_zero = 1;"
            <<     "}"
            << " }"
            << "if (local_size_zero)"
            << "{"
            <<     "for (i = 0; i < num_dim; ++i)"
            <<     "{"
            <<         "local_size_ptr[i] = 1;"
            <<     "}"
            << "}"
            ;

        //Now do the rounding
        if (check_dim)
        {
            code_ndrange_aux
                << "for (i = 0; i < num_dim; ++i)"
                << "{"
                <<     "offset_arr[i] = offset_ptr[i];"

                <<     "local_size_arr[i] = (global_size_ptr[i] < local_size_ptr[i]) ? "
                <<         "global_size_ptr[i] : local_size_ptr[i];"

                <<     "global_size_arr[i] = (global_size_ptr[i] < local_size_ptr[i]) ? "
                <<         "global_size_ptr[i] : global_size_ptr[i] + ("
                <<             "(global_size_ptr[i] % local_size_ptr[i] == 0) ? "
                <<                  "0 : (local_size_ptr[i] - global_size_ptr[i] % local_size_ptr[i]));"
                << "}"
                ;
        }

        if (check_dim)
        {
            code_ndrange_aux
                << "if (local_size_zero)"
                << "{"
                <<     "final_local_size_ptr = 0;"
                << "}"
                << "else"
                << "{"
                <<     "final_local_size_ptr = local_size_arr;"
                << "}"
                //Launch kernel/ it will be freed inside, with ndrange calculated inside the checkDim loop
                << "nanos_exec_kernel(ompss_kernel_ocl, num_dim, offset_arr, final_local_size_ptr, global_size_arr);";
            ;
        }
        else
        {
            code_ndrange_aux
                << "if (local_size_zero)"
                << "{"
                <<     "final_local_size_ptr = 0;"
                << "}"
                << "else"
                << "{"
                <<     "final_local_size_ptr = local_size_ptr;"
                << "}"
                << "nanos_exec_kernel(ompss_kernel_ocl, num_dim, offset_ptr, final_local_size_ptr, global_size_ptr);"
                ;
        }
    }
    else
    {
        int num_dim_offset = num_dim;

        //Prepare ndrange calc pointers and arrays
        code_ndrange_aux
            << "int num_dim = " << as_expression(new_ndrange[0]) <<";"
            << "size_t offset_arr[num_dim];"
            << "size_t local_size_arr[num_dim];"
            << "size_t global_size_arr[num_dim];"
            << as_type(TL::Type::get_bool_type()) << " local_size_zero;"
            << "local_size_zero = 0;"
            ;

        for (int i = 1; i <= num_dim; ++i)
        {
            if (((num_dim * 3) + 1 + !check_dim) != num_args_ndrange)
            {
                num_dim_offset = 0;
                code_ndrange_aux << "offset_arr[" << i-1 << "] = 0;";
            }
            else
            {
                code_ndrange_aux
                    << "offset_arr[" << i-1 << "] = " << as_expression(new_ndrange[i]) << ";";
            }

            code_ndrange_aux
                << "local_size_arr[" << i-1 << "] = " << as_expression(new_ndrange[num_dim + num_dim_offset + i]) << ";"
                << "if (local_size_arr[" << i - 1 << "] == 0)"
                << "{"
                <<      "local_size_zero = 1;"
                << "}"
                << "global_size_arr[" << i-1 << "] = " << as_expression(new_ndrange[num_dim_offset + i]) << ";"
                ;
        }

        //Now do the rounding
        if (check_dim)
        {
            code_ndrange_aux
                << "if (!local_size_zero)"
                << "{"
                <<      "int i;"
                <<      "for (i = 0; i < num_dim; i = i + 1)"
                <<      "{"
                <<           "if (global_size_arr[i] < local_size_arr[i])"
                <<           "{"
                <<               "local_size_arr[i] = global_size_arr[i];"
                <<           "}"
                <<           "else"
                <<           "{"
                <<               "if (global_size_arr[i] % local_size_arr[i] != 0)"
                <<               "{"
                <<                   "global_size_arr[i] = global_size_arr[i]"
                <<                       " + (local_size_arr[i] - global_size_arr[i] % local_size_arr[i]);"
                <<               "}"
                <<           "}"
                <<      "}"
                << "}"
                ;
        }

        code_ndrange_aux
            << "if (local_size_zero)"
            << "{"
            //Launch kernel/ it will be freed inside, with ndrange calculated inside the checkDim loop
            <<      "err = nanos_exec_kernel(ompss_kernel_ocl, num_dim, offset_arr, 0, global_size_arr);"
            << "}"
            << "else"
            << "{"
            //Launch kernel/ it will be freed inside, with ndrange calculated inside the checkDim loop
            <<      "err = nanos_exec_kernel(ompss_kernel_ocl, num_dim, offset_arr, local_size_arr, global_size_arr);"
            << "}"
            ;
    }

    if (IS_FORTRAN_LANGUAGE)
    {
        Source::source_language = SourceLanguage::C;

        Nodecl::NodeclBase code_ndrange_tree = code_ndrange_aux.parse_statement(unpacked_function.get_related_scope());

        Source::source_language = SourceLanguage::Current;

        code_ndrange << as_statement(code_ndrange_tree);
    }
    else
    {
        code_ndrange << code_ndrange_aux;
    }
}

void DeviceOpenCL::create_outline(CreateOutlineInfo &info,
        Nodecl::NodeclBase &outline_placeholder,
        Nodecl::NodeclBase &output_statements,
        Nodecl::Utils::SymbolMap* &symbol_map)
{
    _opencl_tasks_processed=true;
    // Unpack DTO
    const std::string& outline_name = ocl_outline_name(info._outline_name);
    const Nodecl::NodeclBase& task_statements = info._task_statements;
    const Nodecl::NodeclBase& original_statements = info._original_statements;
    const TL::Symbol& called_task = info._called_task;
    bool is_function_task = info._called_task.is_valid();
    TL::ObjectList<OutlineDataItem*> data_items = info._data_items;

    output_statements = task_statements;

    ERROR_CONDITION(called_task.is_valid() && !called_task.is_function(),
            "The '%s' symbol is not a function", called_task.get_name().c_str());

    TL::Symbol current_function =
        original_statements.retrieve_context().get_decl_context().current_scope->related_entry;

    if (current_function.is_nested_function())
    {
        if (IS_C_LANGUAGE || IS_CXX_LANGUAGE)
            running_error("%s: error: nested functions are not supported\n",
                    original_statements.get_locus_str().c_str());


        if (IS_FORTRAN_LANGUAGE)
            running_error("%s: error: internal subprograms are not supported\n",
                    original_statements.get_locus_str().c_str());
    }


    Source extra_declarations;
    Source final_statements, initial_statements;

    // *** Unpacked (and forward in Fortran) function ***
    TL::Symbol unpacked_function, forward_function;
    if (IS_FORTRAN_LANGUAGE)
    {
        forward_function = new_function_symbol_forward(
                current_function,
                outline_name + "_forward",
                info);
        unpacked_function = new_function_symbol_unpacked(
                current_function,
                outline_name + "_unpack",
                info,
                // out
                symbol_map,
                initial_statements,
                final_statements);
    }
    else
    {
        unpacked_function = new_function_symbol_unpacked(
                current_function,
                outline_name + "_unpacked",
                info,
                // out
                symbol_map,
                initial_statements,
                final_statements);
    }


    Nodecl::NodeclBase unpacked_function_code, unpacked_function_body;
    SymbolUtils::build_empty_body_for_function(unpacked_function,
            unpacked_function_code,
            unpacked_function_body);

    if (IS_FORTRAN_LANGUAGE)
    {
        // Now get all the needed internal functions and replicate them in the outline
        Nodecl::Utils::Fortran::InternalFunctions internal_functions;
        internal_functions.walk(info._original_statements);

        Nodecl::List l;
        for (TL::ObjectList<Nodecl::NodeclBase>::iterator
                it2 = internal_functions.function_codes.begin();
                it2 != internal_functions.function_codes.end();
                it2++)
        {
            l.append(
                    Nodecl::Utils::deep_copy(*it2, unpacked_function.get_related_scope(), *symbol_map)
                    );
        }

        // unpacked_function_code.as<Nodecl::FunctionCode>().set_internal_functions(l);
    }

    Nodecl::Utils::append_to_top_level_nodecl(unpacked_function_code);

    //Get file clause, if not present, use the files passed in the command line (if any)
    std::string file = info._target_info.get_file();
    if (!file.empty())
    {
        bool found = false;
        for (int i = 0; i < ::compilation_process.num_translation_units && !found; ++i)
        {
            compilation_file_process_t* file_process = ::compilation_process.translation_units[i];
            translation_unit_t* current_translation_unit = file_process->translation_unit;
            const char* extension = get_extension_filename(current_translation_unit->input_filename);
            struct extensions_table_t* current_extension = fileextensions_lookup(extension, strlen(extension));

            if (current_extension->source_language == SOURCE_LANGUAGE_OPENCL)
            {
                found = (file == std::string(current_translation_unit->input_filename));
            }
        }

        if (!found)
        {
            running_error("%s: error: The OpenCL file indicated by the clause 'file' is not passed in the command line.\n",
                    original_statements.get_locus_str().c_str());
        }
    }
    else
    {
        int ocl_files = 0;
        for (int i = 0; i < ::compilation_process.num_translation_units; ++i)
        {
            compilation_file_process_t* file_process = ::compilation_process.translation_units[i];
            translation_unit_t* current_translation_unit = file_process->translation_unit;
            const char* extension = get_extension_filename(current_translation_unit->input_filename);
            struct extensions_table_t* current_extension = fileextensions_lookup(extension, strlen(extension));

            if (current_extension->source_language == SOURCE_LANGUAGE_OPENCL)
            {
                if (ocl_files > 0)
                    file += ",";

                file += std::string(current_translation_unit->input_filename);
                ocl_files++;
            }
        }

        if (ocl_files == 0)
        {
            running_error("%s: error: No file specified for kernel '%s'\n",
                    original_statements.get_locus_str().c_str(),
                    called_task.get_name().c_str());
        }
    }

    // Get the name of the kernel
    std::string kernel_name = info._target_info.get_name();
    if (kernel_name.empty())
    {
        // If the clause name is not present, use the name of the called task
        kernel_name = called_task.get_name();
    }

    Source ndrange_code;
    if (called_task.is_valid()
            && info._target_info.get_ndrange().size() > 0)
    {
        Nodecl::Utils::SimpleSymbolMap param_to_args_map =
            info._target_info.get_param_arg_map();

        generate_ndrange_code(called_task,
                unpacked_function,
                info._target_info,
                file,
                kernel_name,
                info._data_items,
                &param_to_args_map,
                symbol_map,
                ndrange_code);
    }


    Source unpacked_source;
    if (!IS_FORTRAN_LANGUAGE)
    {
        unpacked_source
            << "{";
    }
    
    unpacked_source
        << extra_declarations
        << initial_statements
        << ndrange_code
        //<< statement_placeholder(outline_placeholder)
        << final_statements
        ;

    if (!IS_FORTRAN_LANGUAGE)
    {
        unpacked_source
            << "}";
    }
    
    // Fortran may require more symbols
    if (IS_FORTRAN_LANGUAGE)
    {
        // Insert extra symbols
        TL::Scope unpacked_function_scope = unpacked_function_body.retrieve_context();

        Nodecl::Utils::Fortran::ExtraDeclsVisitor fun_visitor(symbol_map,
                unpacked_function_scope,
                current_function);
        if (is_function_task)
        {
            fun_visitor.insert_extra_symbol(info._called_task);
        }
        fun_visitor.insert_extra_symbols(task_statements);

        Nodecl::Utils::Fortran::append_used_modules(
                original_statements.retrieve_context(),
                unpacked_function_scope);

        if (is_function_task)
        {
            Nodecl::Utils::Fortran::append_used_modules(
                    info._called_task.get_related_scope(),
                    unpacked_function_scope);
        }

        // Add also used types
        add_used_types(data_items, unpacked_function.get_related_scope());

        // Now get all the needed internal functions and replicate them in the outline
        Nodecl::Utils::Fortran::InternalFunctions internal_functions;
        internal_functions.walk(info._original_statements);

        duplicate_internal_subprograms(internal_functions.function_codes,
                unpacked_function.get_related_scope(),
                symbol_map,
                output_statements);

        extra_declarations
            << "IMPLICIT NONE\n";
    }
    else if (IS_CXX_LANGUAGE)
    {
        if (!unpacked_function.is_member())
        {
            Nodecl::NodeclBase nodecl_decl = Nodecl::CxxDecl::make(
                    /* optative context */ nodecl_null(),
                    unpacked_function,
                    original_statements.get_locus());
            Nodecl::Utils::prepend_to_enclosing_top_level_location(original_statements, nodecl_decl);
        }
    }

    Nodecl::NodeclBase new_unpacked_body = unpacked_source.parse_statement(unpacked_function_body);
    unpacked_function_body.replace(new_unpacked_body);


    // **** Outline function *****
    ObjectList<std::string> structure_name;
    structure_name.append("args");
    ObjectList<TL::Type> structure_type;
    structure_type.append(
            TL::Type(get_user_defined_type(info._arguments_struct.get_internal_symbol())).get_lvalue_reference_to()
            );

    TL::Symbol outline_function = SymbolUtils::new_function_symbol(
            current_function,
            outline_name,
            TL::Type::get_void_type(),
            structure_name,
            structure_type);

    Nodecl::NodeclBase outline_function_code, outline_function_body;
    SymbolUtils::build_empty_body_for_function(outline_function,
            outline_function_code,
            outline_function_body);
    Nodecl::Utils::append_to_top_level_nodecl(outline_function_code);

    // Prepare arguments for the call to the unpack (or forward in Fortran)
    TL::Scope outline_function_scope(outline_function_body.retrieve_context());
    TL::Symbol structure_symbol = outline_function_scope.get_symbol_from_name("args");
    ERROR_CONDITION(!structure_symbol.is_valid(), "Argument of outline function not found", 0);

    Source unpacked_arguments, cleanup_code;

    for (TL::ObjectList<OutlineDataItem*>::iterator it = data_items.begin();
            it != data_items.end();
            it++)
    {
        if (!is_function_task
                && (*it)->get_is_cxx_this())
            continue;

        switch ((*it)->get_sharing())
        {
            case OutlineDataItem::SHARING_PRIVATE:
                {
                    // Do nothing
                    break;
                }
            case OutlineDataItem::SHARING_SHARED:
            case OutlineDataItem::SHARING_CAPTURE:
            case OutlineDataItem::SHARING_CAPTURE_ADDRESS:
                {
                    TL::Type param_type = (*it)->get_in_outline_type();

                    Source argument;
                    if (IS_C_LANGUAGE || IS_CXX_LANGUAGE)
                    {
                        // Normal shared items are passed by reference from a pointer,
                        // derreference here
                        if ((*it)->get_sharing() == OutlineDataItem::SHARING_SHARED
                                && !(IS_CXX_LANGUAGE && (*it)->get_symbol().get_name() == "this"))
                        {
                            if (!param_type.no_ref().depends_on_nonconstant_values())
                            {
                                argument << "*(args." << (*it)->get_field_name() << ")";
                            }
                            else
                            {
                                TL::Type ptr_type = (*it)->get_in_outline_type().references_to().get_pointer_to();
                                TL::Type cast_type = rewrite_type_of_vla_in_outline(ptr_type, data_items, structure_symbol);

                                argument << "*((" << as_type(cast_type) << ")args." << (*it)->get_field_name() << ")";
                            }
                        }
                        // Any other parameter is bound to the storage of the struct
                        else
                        {
                            if (!param_type.no_ref().depends_on_nonconstant_values())
                            {
                                argument << "args." << (*it)->get_field_name();
                            }
                            else
                            {
                                TL::Type cast_type = rewrite_type_of_vla_in_outline(param_type, data_items, structure_symbol);
                                argument << "(" << as_type(cast_type) << ")args." << (*it)->get_field_name();
                            }
                        }

                        if (IS_CXX_LANGUAGE
                                && (*it)->get_allocation_policy() == OutlineDataItem::ALLOCATION_POLICY_TASK_MUST_DESTROY)
                        {
                            internal_error("Not yet implemented: call the destructor", 0);
                        }
                    }
                    else if (IS_FORTRAN_LANGUAGE)
                    {
                        argument << "args % " << (*it)->get_field_name();

                        bool is_allocatable = (*it)->get_allocation_policy() & OutlineDataItem::ALLOCATION_POLICY_TASK_MUST_DEALLOCATE_ALLOCATABLE;
                        bool is_pointer = (*it)->get_allocation_policy() & OutlineDataItem::ALLOCATION_POLICY_TASK_MUST_DEALLOCATE_POINTER;

                        if (is_allocatable
                                || is_pointer)
                        {
                            cleanup_code
                                << "DEALLOCATE(args % " << (*it)->get_field_name() << ")\n"
                                ;
                        }
                    }
                    else
                    {
                        internal_error("running error", 0);
                    }

                    unpacked_arguments.append_with_separator(argument, ", ");
                    break;
                }
            case OutlineDataItem::SHARING_REDUCTION:
                {
                    // // Pass the original reduced variable as if it were a shared
                    Source argument;
                    if (IS_C_LANGUAGE || IS_CXX_LANGUAGE)
                    {
                        argument << "*(args." << (*it)->get_field_name() << ")";
                    }
                    else if (IS_FORTRAN_LANGUAGE)
                    {
                        argument << "args % " << (*it)->get_field_name();
                    }
                    unpacked_arguments.append_with_separator(argument, ", ");
                    break;
                }
            default:
                {
                    internal_error("Unexpected data sharing kind", 0);
                }
        }
    }

    Source outline_src,
           instrument_before,
           instrument_after;

    if (IS_C_LANGUAGE || IS_CXX_LANGUAGE)
    {
        Source unpacked_function_call;
        if (IS_CXX_LANGUAGE
                && !is_function_task
                && current_function.is_member()
                && !current_function.is_static())
        {
            unpacked_function_call << "args.this_->";
        }

        unpacked_function_call
            << unpacked_function.get_qualified_name() << "(" << unpacked_arguments << ");";

        outline_src
            << "{"
            <<      instrument_before
            <<      unpacked_function_call
            <<      instrument_after
            <<      cleanup_code
            << "}"
            ;

        if (IS_CXX_LANGUAGE)
        {
            if (!outline_function.is_member())
            {
                Nodecl::NodeclBase nodecl_decl = Nodecl::CxxDecl::make(
                        /* optative context */ nodecl_null(),
                        outline_function,
                        original_statements.get_locus());
                Nodecl::Utils::prepend_to_enclosing_top_level_location(original_statements, nodecl_decl);
            }
        }
    }
    else if (IS_FORTRAN_LANGUAGE)
    {
        Source outline_function_addr;

        outline_src
            << instrument_before << "\n"
            << "CALL " << outline_name << "_forward(" << outline_function_addr << unpacked_arguments << ")\n"
            << instrument_after << "\n"
            << cleanup_code
            ;

        outline_function_addr << "LOC(" << unpacked_function.get_name() << ")";
        if (!unpacked_arguments.empty())
        {
            outline_function_addr << ", ";
        }

        // Copy USEd information to the outline and forward functions
        TL::Symbol *functions[] = { &outline_function, &forward_function, NULL };

        for (int i = 0; functions[i] != NULL; i++)
        {
            TL::Symbol &function(*functions[i]);

            Nodecl::Utils::Fortran::append_used_modules(original_statements.retrieve_context(),
                    function.get_related_scope());

            add_used_types(data_items, function.get_related_scope());
        }

        // Generate ancillary code in C
        add_forward_code_to_extra_c_code(outline_name, data_items, outline_function_body);
    }
    else
    {
        internal_error("Code unreachable", 0);
    }

    if (instrumentation_enabled())
    {
        get_instrumentation_code(
                info._called_task,
                outline_function,
                outline_function_body,
                info._task_label,
                original_statements.get_locus(),
                instrument_before,
                instrument_after);
    }

    Nodecl::NodeclBase new_outline_body = outline_src.parse_statement(outline_function_body);
    outline_function_body.replace(new_outline_body);

    // Nodecl::Utils::prepend_to_enclosing_top_level_location(original_statements, outline_function_code);
    //
     //Dummy function call placeholder
     Source unpacked_ndr_code;
     unpacked_ndr_code << statement_placeholder(outline_placeholder);
     Nodecl::NodeclBase new_unpacked_ndr_code = unpacked_ndr_code.parse_statement(unpacked_function_body);
     outline_placeholder=new_unpacked_ndr_code;
}

//
DeviceOpenCL::DeviceOpenCL()
    : DeviceProvider(/* device_name */ std::string("opencl"))
{
    set_phase_name("Nanox OpenCL support");
    set_phase_description("This phase is used by Nanox phases to implement OpenCL device support");
}

void DeviceOpenCL::add_forward_code_to_extra_c_code(
        const std::string& outline_name,
        TL::ObjectList<OutlineDataItem*> data_items,
        Nodecl::NodeclBase parse_context)
{
    Source ancillary_source, parameters;

    ancillary_source
        << "extern void " << outline_name << "_forward_" << "(";
    int num_data_items = data_items.size();
    if (num_data_items == 0)
    {
        ancillary_source << "void (*outline_fun)(void)";
    }
    else
    {
        ancillary_source << "void (*outline_fun)(";
        if (num_data_items == 0)
        {
            ancillary_source << "void";
        }
        else
        {
            for (int i = 0; i < num_data_items; i++)
            {
                if (i > 0)
                {
                    ancillary_source << ", ";
                }
                ancillary_source << "void *p" << i;
            }
        }
        ancillary_source << ")";

        for (int i = 0; i < num_data_items; i++)
        {
            ancillary_source << ", void *p" << i;
        }
    }
    ancillary_source << ")\n{\n"
        // << "    extern int nanos_free(void*);\n"
        << "    extern int nanos_handle_error(int);\n\n"
        << "    outline_fun(";
    for (int i = 0; i < num_data_items; i++)
    {
        if (i > 0)
        {
            ancillary_source << ", ";
        }
        ancillary_source << "p" << i;
    }
    ancillary_source << ");\n";

    // Free all the allocated descriptors
    // bool first = true;
    // int i = 0;
    // for (TL::ObjectList<OutlineDataItem*>::iterator it = data_items.begin();
    //         it != data_items.end();
    //         it++, i++)
    // {
    //     OutlineDataItem &item (*(*it));

    //     if (item.get_symbol().is_valid()
    //             && item.get_sharing() == OutlineDataItem::SHARING_SHARED)
    //     {
    //         TL::Type t = item.get_symbol().get_type();

    //         if (!item.get_symbol().is_allocatable()
    //                 && t.is_lvalue_reference()
    //                 && t.references_to().is_array()
    //                 && t.references_to().array_requires_descriptor())
    //         {
    //             if (first)
    //             {
    //                 ancillary_source << "   nanos_err_t err;\n";
    //                 first = false;
    //             }

    //             ancillary_source
    //                 << "    err = nanos_free(p" << i << ");\n"
    //                 << "    if (err != NANOS_OK) nanos_handle_error(err);\n"
    //                 ;
    //         }
    //     }
    // }

    ancillary_source << "}\n\n";

    // Parse in C
    Source::source_language = SourceLanguage::C;

    Nodecl::List n = ancillary_source.parse_global(parse_context).as<Nodecl::List>();

    // Restore original source language (Fortran)
    Source::source_language = SourceLanguage::Current;

    _extra_c_code.append(n);
}

void DeviceOpenCL::get_device_descriptor(DeviceDescriptorInfo& info,
        Source &ancillary_device_description,
        Source &device_descriptor,
        Source &fortran_dynamic_init)
{
    const std::string& device_outline_name = ocl_outline_name(info._outline_name);
    if (Nanos::Version::interface_is_at_least("master", 5012))
    {
        if (!IS_FORTRAN_LANGUAGE)
        {
            ancillary_device_description
                << comment("OpenCL device descriptor")
                << "static nanos_opencl_args_t "
                << device_outline_name << "_args;"
                << device_outline_name << "_args.outline = (void(*)(void*))" << device_outline_name << ";"
                ;

            device_descriptor << "{ &nanos_opencl_factory, &" << device_outline_name << "_args }";
        }
        else
        {
            ancillary_device_description
                << "static nanos_opencl_args_t " << device_outline_name << "_args;"
                ;

            device_descriptor
                << "{"
                // factory, arg
                << "0, 0"
                << "}"
                ;

            fortran_dynamic_init
                << device_outline_name << "_args.outline = (void(*)(void*))&" << device_outline_name << ";"
                << "nanos_wd_const_data.devices[" << info._fortran_device_index << "].factory = &nanos_opencl_factory;"
                << "nanos_wd_const_data.devices[" << info._fortran_device_index << "].arg = &" << device_outline_name << "_args;"
                ;
        }
    }
    else
    {
        internal_error("Unsupported Nanos version.", 0);
    }
}

bool DeviceOpenCL::remove_function_task_from_original_source() const
{
    return true;
}

bool DeviceOpenCL::allow_mandatory_creation()
{
    return true;
}

void DeviceOpenCL::copy_stuff_to_device_file(const TL::ObjectList<Nodecl::NodeclBase>& stuff_to_be_copied)
{
    // Do nothing
}

void DeviceOpenCL::phase_cleanup(DTO& data_flow)
{    
    if (_opencl_tasks_processed){
        Source nanox_device_enable_section;
        nanox_device_enable_section << "__attribute__((weak)) char ompss_uses_opencl = 1;";
        if (IS_FORTRAN_LANGUAGE)
           Source::source_language = SourceLanguage::C;
        Nodecl::NodeclBase functions_section_tree = nanox_device_enable_section.parse_global(_root);
        Source::source_language = SourceLanguage::Current;
        if (IS_FORTRAN_LANGUAGE){
           _extra_c_code.prepend(functions_section_tree); 
        } else {
           Nodecl::Utils::append_to_top_level_nodecl(functions_section_tree); 
        }
        _opencl_tasks_processed = false;
    }
    
    if (_extra_c_code.is_null())
        return;

    std::string original_filename = TL::CompilationProcess::get_current_file().get_filename();
    std::string new_filename = "ocl_aux_nanox_outline_file_" + original_filename  + ".c";

    FILE* ancillary_file = fopen(new_filename.c_str(), "w");
    if (ancillary_file == NULL)
    {
        running_error("%s: error: cannot open file '%s'. %s\n",
                original_filename.c_str(),
                new_filename.c_str(),
                strerror(errno));
    }

    CURRENT_CONFIGURATION->source_language = SOURCE_LANGUAGE_C;

    compilation_configuration_t* configuration = ::get_compilation_configuration("auxcc");
    ERROR_CONDITION (configuration == NULL, "auxcc profile is mandatory when using Fortran", 0);

    // Make sure phases are loaded (this is needed for codegen)
    load_compiler_phases(configuration);

    TL::CompilationProcess::add_file(new_filename, "auxcc");

    ::mark_file_for_cleanup(new_filename.c_str());

    Codegen::CodegenPhase* phase = reinterpret_cast<Codegen::CodegenPhase*>(configuration->codegen_phase);
    phase->codegen_top_level(_extra_c_code, ancillary_file);

    CURRENT_CONFIGURATION->source_language = SOURCE_LANGUAGE_FORTRAN;

    fclose(ancillary_file);
    // Do not forget the clear the code for next files
    _extra_c_code.get_internal_nodecl() = nodecl_null();
}

void DeviceOpenCL::pre_run(DTO& dto)
{
    _root = dto["nodecl"];
    _opencl_tasks_processed = false;
}

void DeviceOpenCL::run(DTO& dto)
{
}

bool DeviceOpenCL::is_gpu_device() const
{
    return true;
}

EXPORT_PHASE(TL::Nanox::DeviceOpenCL);
