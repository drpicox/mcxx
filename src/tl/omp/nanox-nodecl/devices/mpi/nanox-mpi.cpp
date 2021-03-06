/*--------------------------------------------------------------------
  (C) Copyright 2006-2011 Barcelona Supercomputing Center
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


#include "tl-devices.hpp"
#include "nanox-mpi.hpp"
#include "tl-nanos.hpp"
#include "tl-multifile.hpp"
#include "tl-compilerpipeline.hpp"
// #include "fortran03-scope.h"

//#include "cuda-aux.hpp"
//#include "tl-declarationclosure.hpp"

//#include "tl-cuda.hpp"
//#include "tl-omp-nanox.hpp"

#include "cxx-profile.h"
#include "codegen-phase.hpp"
#include "codegen-cxx.hpp"
#include "cxx-cexpr.h"
#include "filename.h"
#include "tl-nodecl-utils-fortran.hpp"
#include "tl-symbol-utils.hpp"
//#include "codegen-fortran.hpp"

//#include <iostream>
//#include <fstream>

#include <errno.h>
#include "cxx-driver-utils.h"

using namespace TL;
using namespace TL::Nanox;

static std::string get_outline_name(const std::string & name) {
    return "mpi_" + name;
}



static void preprocess_datasharing(TL::ObjectList<OutlineDataItem*>& data_items) {
    //If there are other non-mpi devices and we modify some data sharing, throw a warning
//    bool check_for_incompatibility=false;
//    bool is_incompatible=false;
//    
//    OutlineInfo::implementation_table_t implementation_table = outlineInfo.get_implementation_table();
//    for (OutlineInfo::implementation_table_t::iterator it = implementation_table.begin();
//            it != implementation_table.end() && !check_for_incompatibility;
//            ++it)
//    {
//        TargetInformation target_info = it->second;
//        ObjectList<std::string> devices = target_info.get_device_names();
//        for (ObjectList<std::string>::iterator it2 = devices.begin();
//                it2 != devices.end() && !check_for_incompatibility;
//                ++it2)
//        {
//            if (*it2!="mpi" || *it2!="MPI") check_for_incompatibility=true;
//        }
//    }
    if (IS_FORTRAN_LANGUAGE){
        for (TL::ObjectList<OutlineDataItem*>::iterator it = data_items.begin();
                it != data_items.end();
                it++)
        {
            if ((*it)->get_sharing()==OutlineDataItem::SHARING_CAPTURE){
                continue;
            }
             //std::cout << (*it)->get_symbol().get_name() << " es " << (*it)->get_sharing() << " con copias " << !(*it)->get_copies().empty() << " \n";
            if ((*it)->get_symbol().is_allocatable()){
                if ((*it)->get_symbol().is_from_module()){  
//                    is_incompatible = check_for_incompatibility && (*it)->get_sharing()!=OutlineDataItem::SHARING_SHARED;
                    (*it)->set_sharing(OutlineDataItem::SHARING_SHARED);
                    (*it)->get_copies().clear();
                } else {
                   //std::cout << (*it)->get_symbol().get_name() << "es privatye\n";
//                    is_incompatible = check_for_incompatibility && (*it)->get_sharing()!=OutlineDataItem::SHARING_PRIVATE;
                    (*it)->set_sharing(OutlineDataItem::SHARING_PRIVATE);
                    (*it)->get_copies().clear();
                }
            } else {
               if ((*it)->get_symbol().is_from_module()) {
//                   is_incompatible = check_for_incompatibility && (*it)->get_sharing()!=OutlineDataItem::SHARING_SHARED;
                   (*it)->set_sharing(OutlineDataItem::SHARING_SHARED);
               } else {            
                    if ((*it)->get_copies().empty()){
//                      is_incompatible = check_for_incompatibility && (*it)->get_sharing()!=OutlineDataItem::SHARING_PRIVATE;
                      (*it)->set_sharing(OutlineDataItem::SHARING_PRIVATE);
                    }
               }
            }
        }
    }
    
//    if (is_incompatible) std::cerr << "warning: error in MPI task, do not mix MPI device tasks with other devices (implements or multi-device)"
//            " in this situation " << std::endl;
    
//    std::vector<std::string> probanding;
//    probanding.push_back("waw");
//    probanding.push_back("SHARED");
//    probanding.push_back("SHARED CAPTUR");
//    probanding.push_back("SHARING_PRIVATE");
//    for (TL::ObjectList<OutlineDataItem*>::iterator it = data_items.begin();
//            it != data_items.end();
//            it++)
//    {
//         std::cout << (*it)->get_symbol().get_name() << " es " << probanding.at((*it)->get_sharing()) << " con copias " << !(*it)->get_copies().empty() << " \n";
//    }
////    
}



void DeviceMPI::add_forward_code_to_extra_c_code(
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
    if (IS_FORTRAN_LANGUAGE)
    Source::source_language = SourceLanguage::C;

    Nodecl::List n = ancillary_source.parse_global(parse_context).as<Nodecl::List>();

    // Restore original source language (Fortran)
    Source::source_language = SourceLanguage::Current;

    _extra_c_code.append(n);
}

void DeviceMPI::generate_additional_mpi_code(
        const TL::ObjectList<OutlineDataItem*>& data_items,
        const TL::Symbol& struct_args,
        const std::string& outline_name,
        TL::Source& code_host,
        TL::Source& code_device_pre,        
        TL::Source& code_device_post) {
    
    std::string ompss_get_mpi_type="ompss_get_mpi_type";
    const std::string& device_outline_name = get_outline_name(outline_name);

    TL::Type argument_type = ::get_user_defined_type(struct_args.get_internal_symbol());
    TL::ObjectList<TL::Symbol> parameters_called = argument_type.get_fields();
    TL::ObjectList<std::string> param_called_names;
    
    int num_params = parameters_called.size();    
    for (int i = 0; i < num_params; ++i) {
        param_called_names.append(parameters_called.at(i).get_name());
    }
    
    //We fill it manually with "Null" values
    //Nanox will search the right communicator and rank at runtime (based on the binding)
    TL::ObjectList<std::string> new_dev_info;
    new_dev_info.append("0");
    new_dev_info.append("-2");


    code_host << "MPI_Status ompss___status; "
            << "int err; ";
    
    code_device_pre << struct_args.get_name() << " args;"
            << "int err; "            
            << "MPI_Comm ompss_parent_comp; "            
            << "err= nanos_mpi_get_parent(&ompss_parent_comp);"
            << "MPI_Status ompss___status; ";

    Source typelist_src, blocklen_src, displ_src;
    //Source parameter_call;
    

    //If there are parameters, add/build the structures 
    if (num_params>0){
        int count_params=num_params;
        Source struct_mpi_create, host_call, device_call;



        host_call << " int id_func_ompss=" << "ompss_mpi_get_function_index_host((void *)" << device_outline_name << "_host)" << ";";
        host_call << " err=nanos_mpi_send_taskinit(&id_func_ompss, 1,  " << ompss_get_mpi_type  << "(\"__mpitype_ompss_signed_int\")," + new_dev_info[1] + " , " + new_dev_info[0] + ");";
        host_call << " err=nanos_mpi_send_datastruct( (void *) &args, 1,  ompss___datatype," + new_dev_info[1] + "," + new_dev_info[0] + ");";
        host_call << " err=nanos_mpi_recv_taskend(&id_func_ompss, 1,  " << ompss_get_mpi_type  << "(\"__mpitype_ompss_signed_int\")," + new_dev_info[1] + " , " + new_dev_info[0] + ",&ompss___status);";

        device_call << " err=nanos_mpi_recv_datastruct(&args, 1, ompss___datatype, 0, ompss_parent_comp, &ompss___status); ";

        for (int i = 0; i < num_params; ++i) { 
            //parameter_call.append_with_separator("args." + parameters_called[i].get_name(),",");
            std::string ompss_mpi_type = get_ompss_mpi_type(parameters_called[i].get_type());
            if (!parameters_called[i].is_from_module() && parameters_called[i].is_allocatable()){
                --count_params;
                continue;
            }
            //if (!IS_FORTRAN_LANGUAGE){
            //    displ_src.append_with_separator("((size_t) ( (char *)&((" + struct_args.get_name() + " *)0)->" + parameters_called[i].get_name() + " - (char *)0 ))", ",");
            //} else {
            //This seems to work correctly in both "languages"
            displ_src.append_with_separator("((size_t) ( (char *)&(args." + parameters_called[i].get_name() + ") - (char *)&args ))", ",");
            //}
            if (parameters_called[i].get_type().is_pointer()) {
                typelist_src.append_with_separator(ompss_get_mpi_type  + "(\"__mpitype_ompss_unsigned_long_long\")", ",");

                blocklen_src.append_with_separator("1", ",");
            } else {
                typelist_src.append_with_separator(ompss_mpi_type, ",");

                if (parameters_called[i].get_type().array_has_size()) {
                    blocklen_src.append_with_separator(parameters_called[i].get_type().array_get_size().prettyprint(), ",");
                } else {
                    blocklen_src.append_with_separator("1", ",");
                }
            }

        }
        
        struct_mpi_create << "MPI_Datatype ompss___datatype;"
                "MPI_Datatype ompss___typelist[" << count_params << "]= {" << typelist_src << "};"
                "int ompss___blocklen[" << count_params << "] = {" << blocklen_src << "};"
                "MPI_Aint ompss___displ[" << count_params << "] = {" << displ_src << "};";

        struct_mpi_create << "err= nanos_mpi_type_create_struct( " << count_params << ", ompss___blocklen, ompss___displ, ompss___typelist, &ompss___datatype); ";
        code_host << struct_mpi_create
                << host_call;
        code_device_pre << struct_mpi_create
                << device_call;
    //If there are no parameters, just send the order to start the task and wait for the ending ack
    } else {
        code_host << " int id_func_ompss=" << "ompss_mpi_get_function_index_host((void *)" << device_outline_name << "_host)" << ";";
        code_host << " err=nanos_mpi_send_taskinit(&id_func_ompss, 1,  " << ompss_get_mpi_type  << "(\"__mpitype_ompss_signed_int\")," + new_dev_info[1] + " , " + new_dev_info[0] + ");";
        code_host << " err=nanos_mpi_recv_taskend(&id_func_ompss, 1,  " << ompss_get_mpi_type  << "(\"__mpitype_ompss_signed_int\")," + new_dev_info[1] + " , " + new_dev_info[0] + ",&ompss___status);";
    }
    
    if (IS_CXX_LANGUAGE && Nanos::Version::interface_is_at_least("copies_api", 1003)){
        int counter=0;
        for (TL::ObjectList<OutlineDataItem*>::const_iterator it = data_items.begin();
                    it != data_items.end();
                    it++)
        {
            OutlineDataItem& data_item=(*(*it));
            TL::ObjectList<OutlineDataItem::CopyItem> copies = data_item.get_copies();
            TL::Symbol data_sym= data_item.get_symbol();

            //Only serialize when there are no copies and the symbol is serializable
            if (!copies.empty() && is_serializable(data_sym)){
                TL::Type ser_type = data_sym.get_type();
                TL::Symbol sym_serializer = ser_type.get_symbol();
                if (sym_serializer.get_type().is_pointer_to_class()){
                    ser_type= sym_serializer.get_type().get_pointer_to();
                    sym_serializer= sym_serializer.get_type().get_pointer_to().get_symbol();
                }
                int input=0;
                int output=0;
                for (TL::ObjectList<OutlineDataItem::CopyItem>::iterator copy_it = copies.begin();
                        copy_it != copies.end();
                        copy_it++)
                {
                    TL::DataReference data_ref(copy_it->expression);
                    OutlineDataItem::CopyDirectionality dir = copy_it->directionality;

                    Nodecl::NodeclBase address_of_object = data_ref.get_address_of_symbol();

                    input += (dir & OutlineDataItem::COPY_IN) == OutlineDataItem::COPY_IN;
                    output += (dir & OutlineDataItem::COPY_OUT) == OutlineDataItem::COPY_OUT;
                }

                //If no input, warning (a serializable object MUST be input)
                if (input==0){
                    std::cerr << data_sym.get_locus_str() << ": warning: when serializing an object it must be declared as copy_in, skipping serialization "  << std::endl;
                } else {
                    if (output!=0) code_device_pre << "nanos::omemstream " << " " << "outbuff_" << data_sym.get_name() << counter << "((char*)args." << data_sym.get_name() << ",2147483647);";   
                    code_device_pre << "nanos::imemstream " << " " << "buff_" << data_sym.get_name() << counter << "((char*)args." << data_sym.get_name() << ",2147483647);";                    
                    code_device_pre << sym_serializer.get_qualified_name() << " " << "tmp_" << data_sym.get_name() << counter << "(buff_" << data_sym.get_name() << counter << ");";
                    code_device_pre << "args." << data_sym.get_name() << "=&tmp_" << data_sym.get_name() << counter << ";";
                    //If there is an output, serialize the object after the task, so when nanox comes back to the device, the buffer is updated
                    if (output!=0){
                          code_device_post << "tmp_" << data_sym.get_name() << counter << ".serialize(outbuff_" << data_sym.get_name() << counter << ");";
                    }
                }
                ++counter;
            }
        }
    }
    code_device_post << "int ompss_id_func=" << _currTaskId << ";";
    code_device_post << "err= nanos_mpi_send_taskend(&ompss_id_func, 1, " << ompss_get_mpi_type  << "(\"__mpitype_ompss_signed_int\"), 0, ompss_parent_comp);";


}

/**
 * In MPI we generate three functions
 * _host function, function which it's called on the host (by nanox)
 * _device function, function which it's called on the device (by the daemon mercurium generates)
 * _unpacked function, function which it's called inside the _device function, and calls the original user-code function
 * @param info
 * @param outline_placeholder
 * @param output_statements
 * @param symbol_map
 */
void DeviceMPI::create_outline(CreateOutlineInfo &info,
        Nodecl::NodeclBase &outline_placeholder,
        Nodecl::NodeclBase &output_statements,
        Nodecl::Utils::SymbolMap* &symbol_map) {
    
    TL::ObjectList<OutlineDataItem*> data_items = info._data_items;
    preprocess_datasharing(data_items);
    
    symbol_map = new Nodecl::Utils::SimpleSymbolMap();
        

    // Unpack DTO 
    const std::string& device_outline_name = get_outline_name(info._outline_name);
    const Nodecl::NodeclBase& original_statements = info._original_statements;
    const TL::Symbol& called_task = info._called_task;
    bool is_function_task = called_task.is_valid();

    output_statements = original_statements;
    
    //OutlineInfo& outline_info = info._outline_info;
    
    //At first time we process a task, declare a function
    if (!_mpi_task_processed){
        _mpi_task_processed = true;
        Source search_function;
        search_function << "typedef float(*ptrToFunc)(float, float);";
        search_function << "extern int ompss_mpi_get_function_index_host(void* func);";
        if (IS_FORTRAN_LANGUAGE)
            Source::source_language = SourceLanguage::C;
        Nodecl::NodeclBase search_function_tree = search_function.parse_global(_root);
        Source::source_language = SourceLanguage::Current;
        Nodecl::Utils::prepend_to_enclosing_top_level_location(original_statements, search_function_tree);
    }

    ERROR_CONDITION(called_task.is_valid() && !called_task.is_function(),
            "The '%s' symbol is not a function", called_task.get_name().c_str());

    TL::Symbol current_function =
            original_statements.retrieve_context().get_decl_context().current_scope->related_entry;

    if (current_function.is_nested_function()) {
        if (IS_C_LANGUAGE || IS_CXX_LANGUAGE)
            running_error("%s: error: nested functions are not supported\n",
                original_statements.get_locus_str().c_str());        
    }

    Source unpacked_arguments, private_entities, cleanup_code;
    
    
    ObjectList<std::string> structure_name;
    ObjectList<TL::Type> structure_type;
    // Create the new unpacked function
    TL::Symbol device_function = SymbolUtils::new_function_symbol(
            current_function,
            device_outline_name + "_device",
            TL::Type::get_void_type(),
            structure_name,
            structure_type);
    
    Nodecl::NodeclBase device_function_code, device_function_body;
    SymbolUtils::build_empty_body_for_function(device_function,
            device_function_code,
            device_function_body);
    

    // Create the outline function
    //The outline function has always only one parameter which name is 'args'
    structure_name.append("args");

    //The type of this parameter is an struct (i. e. user defined type)
    structure_type.append(TL::Type(
            get_user_defined_type(
            info._arguments_struct.get_internal_symbol())).get_lvalue_reference_to());

    TL::Symbol host_function = SymbolUtils::new_function_symbol(
            current_function,
            device_outline_name + "_host",
            TL::Type::get_void_type(),
            structure_name,
            structure_type);
    
    Nodecl::NodeclBase host_function_code, host_function_body;
    SymbolUtils::build_empty_body_for_function(host_function,
            host_function_code,
            host_function_body);
    
    // Create the new unpacked function
    Source dummy_initial_statements, dummy_final_statements;
    TL::Symbol unpacked_function, forward_function;
    
    if (IS_FORTRAN_LANGUAGE)
    {
        forward_function = new_function_symbol_forward(
                current_function,
                device_outline_name + "_forward",
                info);
        unpacked_function = new_function_symbol_unpacked(
                current_function,
                device_outline_name + "_unpack",
                info,
                // out
                symbol_map,
                dummy_initial_statements,
                dummy_final_statements);
    }
    else
    {
        unpacked_function = new_function_symbol_unpacked(
                current_function,
                device_outline_name + "_unpacked",
                info,
                // out
                symbol_map,
                dummy_initial_statements,
                dummy_final_statements);
    }
    
    Nodecl::NodeclBase unpacked_function_code, unpacked_function_body;
    SymbolUtils::build_empty_body_for_function(unpacked_function,
            unpacked_function_code,
            unpacked_function_body);
    
    TL::Scope host_function_scope(host_function_body.retrieve_context());    
    TL::Symbol structure_symbol = host_function_scope.get_symbol_from_name("args");
    ERROR_CONDITION(!structure_symbol.is_valid(), "Argument of outline function not found", 0);

    std::map< TL::Symbol,TL::ObjectList<TL::Symbol> > modules_with_params;
    Source data_input_global;
    Source data_output_global;

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
                //If it's firstprivate (sharing capture), copy input and change address to the global/private var
                if ((*it)->get_symbol().is_fortran_common() || (*it)->get_symbol().is_from_module() || (*it)->get_symbol().get_scope().is_namespace_scope()){
                   std::string symbol_name=(*it)->get_symbol().get_name();
                   if ((*it)->get_sharing() == OutlineDataItem::SHARING_CAPTURE){
                       if (!(*it)->get_copies().empty())
                       data_input_global << "err =  nanos_memcpy(&" << symbol_name <<",args." << symbol_name <<",sizeof(" << symbol_name << "));";  
                       
                       data_input_global << "args." << symbol_name <<"= &" << symbol_name << ";"; 
                   }
                }
            case OutlineDataItem::SHARING_CAPTURE_ADDRESS:
                {  
                    //If is from module(fort)/common (fort)/global (C) and is sharing ca or sharing shared (no sharing capture)
                    //copy address and data
                    if (!(*it)->get_symbol().is_allocatable() && (*it)->get_sharing() != OutlineDataItem::SHARING_CAPTURE &&
                            ((*it)->get_symbol().is_fortran_common() || (*it)->get_symbol().is_from_module() || (*it)->get_symbol().get_scope().is_namespace_scope())){  
                        std::string symbol_name=(*it)->get_symbol().get_name();
                        data_input_global << "void* " << symbol_name << "_BACKUP =  args." << symbol_name <<";";   
                        
                        if (!(*it)->get_copies().empty())
                        data_input_global << "err =  nanos_memcpy(&" << symbol_name <<","<< symbol_name << "_BACKUP,sizeof(" << symbol_name << "));"; 
                        
                        data_input_global << "args." << symbol_name <<"= &" << symbol_name << ";"; 

                        if (!(*it)->get_copies().empty())
                        data_output_global << "err =  nanos_memcpy("<< symbol_name << "_BACKUP,&" << symbol_name <<",sizeof(" << symbol_name << "));";    
                    }
                    
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

                                argument << "*(" <</*(" << as_type(cast_type) << ")*/"args." << (*it)->get_field_name() << ")";
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
                                argument << /*"(" << as_type(cast_type) << ")*/"args." << (*it)->get_field_name();
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
                        //Build list with modules and vars when they are from module
                        if ((*it)->get_symbol().is_from_module()){
                            TL::Symbol mod_sym=(*it)->get_symbol().from_module();
                            std::map< TL::Symbol,TL::ObjectList<TL::Symbol> >::iterator mod_list= modules_with_params.find(mod_sym);
                            if (mod_list==modules_with_params.end()){
                                TL::ObjectList<TL::Symbol> list;
                                list.append((*it)->get_symbol());
                                modules_with_params.insert(std::pair<TL::Symbol,TL::ObjectList<TL::Symbol> >(mod_sym,list));
                            } else {
                                mod_list->second.append((*it)->get_symbol());                    
                            }
                        }
                        argument << "args % " << (*it)->get_field_name();

                        bool is_allocatable = (*it)->get_allocation_policy() & OutlineDataItem::ALLOCATION_POLICY_TASK_MUST_DEALLOCATE_ALLOCATABLE;
                        bool is_pointer = (*it)->get_allocation_policy() & OutlineDataItem::ALLOCATION_POLICY_TASK_MUST_DEALLOCATE_POINTER;

                        if (((*it)->get_symbol().is_from_module() && is_allocatable)
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

    Source code_host;
    Source code_device_pre;
    Source code_device_post;
    
    generate_additional_mpi_code(
            data_items,
            info._arguments_struct,
            info._outline_name,
            code_host,
            code_device_pre,
            code_device_post);

    Source extra_declarations;
    // Add a declaration of the unpacked function symbol in the original source
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
            fun_visitor.insert_extra_symbols(info._task_statements);

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
    }
    else if (IS_CXX_LANGUAGE) {
       if (!unpacked_function.is_member())
       {
            Nodecl::NodeclBase nodecl_decl = Nodecl::CxxDecl::make(
                    /* optative context */ nodecl_null(),
                    host_function,
                    original_statements.get_locus());
            Nodecl::Utils::prepend_to_enclosing_top_level_location(original_statements, nodecl_decl);
      }
    }
    
    

    Source unpacked_source;
    
    unpacked_source
             << "{"
            << extra_declarations
            << private_entities
            //<< code_host
            << statement_placeholder(outline_placeholder)
            << "}";
    
    
    if (IS_FORTRAN_LANGUAGE)
       Source::source_language = SourceLanguage::C;
    Nodecl::NodeclBase new_unpacked_body =
            unpacked_source.parse_statement(unpacked_function_body);    
    Source::source_language = SourceLanguage::Current;
    unpacked_function_body.replace(new_unpacked_body);


    // Add the unpacked function to the file
    Nodecl::Utils::prepend_to_enclosing_top_level_location(original_statements, unpacked_function_code);

    Source host_src,
            instrument_before,
            instrument_after;

    
    
    
    
    host_src
            << "{"
            << instrument_before
            << code_host
            << instrument_after;    
    
    if (!cleanup_code.empty()){
           Nodecl::NodeclBase cleanup_code_tree = cleanup_code.parse_statement(host_function_body);
           host_src << as_statement(cleanup_code_tree);
    }
    
    host_src << "}";

    if (IS_FORTRAN_LANGUAGE)
       Source::source_language = SourceLanguage::C;
    Nodecl::NodeclBase new_host_body = host_src.parse_statement(host_function_body);
    Source::source_language = SourceLanguage::Current;

    host_function_body.replace(new_host_body);
    
    Nodecl::Utils::prepend_to_enclosing_top_level_location(original_statements, host_function_code);


    
    Source unpacked_function_call;
    if (IS_C_LANGUAGE || IS_CXX_LANGUAGE)
    {
       if (IS_CXX_LANGUAGE
                && !is_function_task
                && current_function.is_member()
                && !current_function.is_static())
        {
            unpacked_function_call << "args.this_->";
        }

        unpacked_function_call
           << unpacked_function.get_qualified_name() << "(" << unpacked_arguments << ");";

        //TODO: Test this and check what it does (fsainz)
        if (IS_CXX_LANGUAGE)
        {
            if (!host_function.is_member())
            {
                Nodecl::NodeclBase nodecl_decl = Nodecl::CxxDecl::make(
                        /* optative context */ nodecl_null(),
                        host_function,
                        original_statements.get_locus());
                Nodecl::Utils::prepend_to_enclosing_top_level_location(original_statements, nodecl_decl);
            }
        }
    }
    else if (IS_FORTRAN_LANGUAGE)
    {
        Source unpacked_function_addr;
        unpacked_function_call
            << "CALL " << device_outline_name << "_forward(" << unpacked_function_addr << unpacked_arguments << ")\n"
            ;

        unpacked_function_addr << "LOC(" << unpacked_function.get_name() << ")";
        if (!unpacked_arguments.empty())
        {
            unpacked_function_addr << ", ";
        }

       // Copy USEd information to the outline and forward functions
        TL::Symbol *functions[] = { &device_function ,&host_function, &forward_function, NULL };

        for (int i = 0; functions[i] != NULL; i++)
        {
            TL::Symbol &function(*functions[i]);

            Nodecl::Utils::Fortran::append_used_modules(original_statements.retrieve_context(),
                    function.get_related_scope());

            add_used_types(data_items, function.get_related_scope());
        }

        // Generate ancillary code in C
        add_forward_code_to_extra_c_code(device_outline_name, data_items, outline_placeholder);
    }
    else
    {
        internal_error("Code unreachable", 0);
    }
    
    Source device_src;
    
    
    Nodecl::NodeclBase new_device_body;
    if (IS_C_LANGUAGE || IS_CXX_LANGUAGE)
    {
        device_src
                << "{"
                << code_device_pre
                << data_input_global
                << unpacked_function_call
                << data_output_global
                << code_device_post
                << "}"
                ;
        
       new_device_body = device_src.parse_statement(device_function_body);
    }
    else if (IS_FORTRAN_LANGUAGE)
    {
        //Generate the USE (module), ONLY: params
        Source mod_list_with_params;
        
        std::map< TL::Symbol,TL::ObjectList<TL::Symbol> >::iterator it_mods;
        for (it_mods= modules_with_params.begin(); it_mods!=modules_with_params.end(); it_mods++){
            mod_list_with_params << "USE " << it_mods->first.get_name() << ", ONLY: ";
            TL::ObjectList<TL::Symbol> lst_params=it_mods->second;
            TL::ObjectList<TL::Symbol>::iterator it_lst;
            Source par_list;
            for (it_lst= lst_params.begin(); it_lst!=lst_params.end(); it_lst++){
                par_list.append_with_separator(it_lst->get_name(),",");
            }
            mod_list_with_params << par_list << "\n";
        }
        Nodecl::NodeclBase mod_params_tree = mod_list_with_params.parse_statement(device_function_body);
        Source::source_language = SourceLanguage::C;
        Nodecl::NodeclBase code_pre = code_device_pre.parse_statement(device_function_body);
        device_src
                << as_statement(code_pre);
        
        Nodecl::NodeclBase data_input_tree;
        Nodecl::NodeclBase data_output_tree;
        if (!data_input_global.empty()){
            data_input_tree = data_input_global.parse_statement(device_function_body);
            device_src
                    << as_statement(data_input_tree);
        }
        Nodecl::NodeclBase code_post = code_device_post.parse_statement(device_function_body);
        device_src
                << unpacked_function_call;
        if (!data_output_global.empty()){        
            data_output_tree = data_output_global.parse_statement(device_function_body);
            device_src
                    << as_statement(data_output_tree);
        }
        device_src
                << as_statement(code_post);
        Source::source_language = SourceLanguage::Current;      
        
        new_device_body = device_src.parse_statement(device_function_body);        
    }
    else
    {
        internal_error("Code unreachable", 0);
    }
    
    device_function_body.replace(new_device_body);
    Nodecl::Utils::prepend_to_enclosing_top_level_location(original_statements, device_function_code);
        
    
    
    std::string append;
    if (IS_FORTRAN_LANGUAGE){
        append="_";


        _extraFortranDecls <<
               "extern void " + device_outline_name + "_host" << append  << "(struct " << info._arguments_struct.get_name() << " *const args);"
               "extern void " << device_outline_name << "_device"  << append << "(void);";
    }
    
    _sectionCodeHost.append_with_separator("(void*)" + host_function.get_qualified_name() + append,",");
    _sectionCodeDevice.append_with_separator("(void(*)())" + device_function.get_qualified_name() + append,",");
    
}

DeviceMPI::DeviceMPI()
: DeviceProvider(/* device_name */ std::string("mpi")) //, _cudaFilename(""), _cudaHeaderFilename("")
{
    set_phase_name("Nanox MPI support");
    set_phase_description("This phase is used by Nanox phases to implement MPI device support");
}

void DeviceMPI::get_device_descriptor(DeviceDescriptorInfo& info,
        Source &ancillary_device_description,
        Source &device_descriptor,
        Source &fortran_dynamic_init UNUSED_PARAMETER) {
    TargetInformation& target_information = info._target_info;
    const std::string& device_outline_name = get_outline_name(info._outline_name);
    if (Nanos::Version::interface_is_at_least("master", 5012)) {
        ObjectList<Nodecl::NodeclBase> onto_clause = target_information.get_onto();
        Nodecl::Utils::SimpleSymbolMap param_to_args_map = info._target_info.get_param_arg_map();
        
        //Set rank and comm, 0 and -2 means undefined so
        //runtime can pick any FREE spawned node
        //(user can specify any rank and any comm using onto clause)
        std::string assignedComm = "0";
        std::string assignedRank = "-2";
        if (onto_clause.size() >= 1 && onto_clause.at(0).get_symbol().is_valid() ) {
            assignedComm = as_symbol(param_to_args_map.map(onto_clause.at(0).get_symbol()));
        }
        if (onto_clause.size() >= 2 && onto_clause.at(1).get_symbol().is_valid()) {
            assignedRank = as_symbol(param_to_args_map.map(onto_clause.at(1).get_symbol()));
        }
        
        if (!IS_FORTRAN_LANGUAGE)
        {
            ancillary_device_description
                << "static nanos_mpi_args_t "
                << device_outline_name << "_mpi_args;"
                << device_outline_name << "_mpi_args.outline = (void(*)(void*))" << device_outline_name << "_host;"
                << device_outline_name << "_mpi_args.assignedComm = " << assignedComm << ";"
                << device_outline_name << "_mpi_args.assignedRank = " << assignedRank << ";";


            device_descriptor << "{ &nanos_mpi_factory, &" << device_outline_name << "_mpi_args }";
        }
        else
        {
            ancillary_device_description
                << "static nanos_mpi_args_t " << device_outline_name << "_args;"
                ;

            device_descriptor
                << "{"
                // factory, arg
                << "0, 0"
                << "}"
                ;

            fortran_dynamic_init
                << device_outline_name << "_args.outline = (void(*)(void*))&" << device_outline_name << "_host;"
                << device_outline_name << "_args.assignedComm = " << assignedComm << ";"
                << device_outline_name << "_args.assignedRank = " << assignedRank << ";"
                << "nanos_wd_const_data.devices[0].factory = &nanos_mpi_factory;"
                << "nanos_wd_const_data.devices[0].arg = &" << device_outline_name << "_args;"
                ;
        }
    } else {
        internal_error("Unsupported Nanos version.", 0);
    }
    
    _currTaskId++;    
}

bool DeviceMPI::remove_function_task_from_original_source() const
{
    return false;
}

bool DeviceMPI::allow_mandatory_creation() {
    return true;
}


void DeviceMPI::copy_stuff_to_device_file(const TL::ObjectList<Nodecl::NodeclBase>& stuff_to_be_copied) {
}

static std::ifstream::pos_type get_filesize(const char* filename)
{
    std::ifstream in(filename, std::ifstream::in | std::ifstream::binary);
    in.seekg(0, std::ifstream::end);
    return in.tellg(); 
}

static unsigned hash_str(const char* s)
{
   unsigned h = 31 /* also prime */;
   while (*s) {
     h = (h * 54059) ^ (s[0] * 76963);
     s++;
   }
   //Make sure this number is bigger than MASK_TASK_NUMBER (not sure if needed)
   return h+MASK_TASK_NUMBER+5;
}


void DeviceMPI::phase_cleanup(DTO& data_flow) {
    
    std::string original_filename = TL::CompilationProcess::get_current_file().get_filename();        
    original_filename =original_filename.substr(0, original_filename.find("."));
    Source _mpiDaemonMain;
    if (IS_FORTRAN_LANGUAGE){
        _mpiDaemonMain << "int ompss___mpi_daemon_main_() { "
                       << "   nanos_mpi_initf();	";   
    } else {
        _mpiDaemonMain << "int ompss___mpi_daemon_main(int argc, char* argv[]) { ";
        _mpiDaemonMain << "   nanos_mpi_init(&argc, &argv);	";
                
    }
    _mpiDaemonMain << "    nanos_sync_dev_pointers(ompss_mpi_masks, "<< MASK_TASK_NUMBER << ", ompss_mpi_filenames, ompss_mpi_file_sizes,"
       << "    ompss_mpi_file_ntasks,ompss_mpi_func_pointers_dev);"
       << "    return nanos_mpi_worker(ompss_mpi_func_pointers_dev);"
       << " }"; //END main

    Symbol main;
    if (IS_FORTRAN_LANGUAGE){
        Nodecl::List top_level_list = _root.as<Nodecl::TopLevel>().get_top_level().as<Nodecl::List>();
        bool found=false;
        for (Nodecl::List::iterator it = top_level_list.begin();
            it != top_level_list.end() && !found; 
            it++)
        {
           Nodecl::NodeclBase current_item = *it;
           if (current_item.is<Nodecl::FunctionCode>())
           {
               Nodecl::FunctionCode function_code = current_item.as<Nodecl::FunctionCode>();
               TL::Symbol function_sym = function_code.get_symbol();
               if (function_sym.get_internal_symbol()->kind==SK_PROGRAM){                   
                    type_t *function_type = get_new_function_type(
                            get_void_type(), NULL ,0);
                    
                   function_sym.get_internal_symbol()->kind=SK_FUNCTION;
                   function_sym.get_internal_symbol()->type_information=function_type;
                   main=function_sym;
                   found=true;
               }
           }
        }
    } else {        
        main = _root.retrieve_context().get_symbol_from_name("main");
    }
    
    //Create MPI sections
    //This section will be synchronized in "nanos_sync_dev_pointers" nanox call
    //so we have function pointers in the same order in both processes
    if (_mpi_task_processed || main.is_valid()) {
        Source functions_section;   
        
        //Extern declaration of fortran tasks in C file (needed, until codegen can do them, but it's this is unlikely to happen
        //because codegen doesn't know that we have two "scopes/languages")
        functions_section << _extraFortranDecls; 
        //Section with MASKS (this is just an array containing 989, so we can count how many files we compiled, when this finishes, next section
        //will not have that value)
        functions_section << "int (ompss_mpi_masks[]) __attribute__((weak)) __attribute__ ((section (\"ompss_file_mask\"))) = { "
                << MASK_TASK_NUMBER
                << "}; ";
        //Filename hash (so we can know in which order files linked)
        functions_section << "unsigned int(ompss_mpi_filenames[]) __attribute__((weak)) __attribute__ ((section (\"ompss_file_names\"))) = { "
                << hash_str(TL::CompilationProcess::get_current_file().get_filename().c_str())
                << "}; ";
        
        //File size (so we "ensure" that both files compiled exactly the same code)
        functions_section << "unsigned int (ompss_mpi_file_sizes[]) __attribute__((weak)) __attribute__ ((section (\"ompss_file_sizes\"))) = { "
                << get_filesize(TL::CompilationProcess::get_current_file().get_filename().c_str()) << _currTaskId
                << "}; ";
        //Number of tasks in file  (used to ensure that both files had the same number, and also for ordering)
        functions_section << "unsigned int (ompss_mpi_file_ntasks[]) __attribute__((weak)) __attribute__ ((section (\"ompss_mpi_file_n_tasks\"))) = { "
                << _currTaskId
                << "}; ";
        //Pointers to the host functions
        functions_section << "void (*ompss_mpi_func_pointers_host[]) __attribute__((weak)) __attribute__ ((section (\"ompss_func_pointers_host\"))) = { "
                << _sectionCodeHost
                << "}; ";
        //Pointers to the device functions
        functions_section << "void (*ompss_mpi_func_pointers_dev[])() __attribute__((weak)) __attribute__ ((section (\"ompss_func_pointers_dev\"))) = { "
                << _sectionCodeDevice
                << "}; ";
        
        if (IS_FORTRAN_LANGUAGE)
           Source::source_language = SourceLanguage::C;
        Nodecl::NodeclBase functions_section_tree = functions_section.parse_global(_root);
        Source::source_language = SourceLanguage::Current;
        if (IS_FORTRAN_LANGUAGE){
           _extra_c_code.prepend(functions_section_tree); 
        } else {
           Nodecl::Utils::append_to_top_level_nodecl(functions_section_tree); 
        }
    }
        
    
    if (main.is_valid()) {
            //Build a new main which calls to the OmpSs daemon or to the user main
            Source real_main;
            
            if (IS_FORTRAN_LANGUAGE){                
                real_main <<    "PROGRAM ompss_main\n"
                                "    IMPLICIT NONE\n"
                                "    INTEGER(4) :: nargs\n"
                                "    CHARACTER(LEN=24) :: arg\n"
                                "    INTEGER(4) :: err\n"
                                "    INTEGER(4), EXTERNAL :: ompss___mpi_daemon_main\n"
                                "    INTERFACE\n"
                                "      SUBROUTINE ompss___user_main()\n"
                                "          IMPLICIT NONE\n"
                                "      END SUBROUTINE ompss___user_main\n"
                                "    END INTERFACE\n"
                                "    nargs = iargc()\n"
                                "    CALL getarg(nargs, arg)\n"          
                                "    IF (nargs > 1 .AND. arg == \"" << TAG_MAIN_OMPSS << "\") THEN\n"
                                "      err = ompss___mpi_daemon_main()\n"
                                "    ELSE\n"
                                "      CALL " << main.get_name() << "()\n"
                                "    END IF\n"
                                "END PROGRAM ompss_main";                
            } else {
                real_main << "int ompss_tmp_main(int argc, char* argv[]) {"
                        << "int err;"
                        << "if (argc > 1 && !strcmp(argv[argc-1],\"" << TAG_MAIN_OMPSS << "\")){"
                        << "err=ompss___mpi_daemon_main(argc,argv);"
                        << "return 0;"
                        << "} else {";
                
                if (main.get_type().returns().is_signed_int() || main.get_type().returns().is_unsigned_int()){
                     real_main << "err= main(argc,argv);"
                        << "return err;"
                        << "}}"
                        ;
                } else {
                    real_main << "main(argc,argv);"
                        << "return 0;"
                        << "}}"
                        ;
                }
                
            }
        
            if (IS_FORTRAN_LANGUAGE)
               Source::source_language = SourceLanguage::C;
            Nodecl::NodeclBase newompss_main = _mpiDaemonMain.parse_global(_root);
            Source::source_language = SourceLanguage::Current;
            Nodecl::NodeclBase new_main = real_main.parse_global(main.get_function_code());    
            
            if (IS_FORTRAN_LANGUAGE){
               _extra_c_code.prepend(newompss_main); 
               Nodecl::Utils::append_to_top_level_nodecl(new_main); 
            } else {
               Nodecl::Utils::append_to_top_level_nodecl(newompss_main); 
               Nodecl::Utils::append_to_top_level_nodecl(new_main); 
               main.set_name("ompss___user_main");
               if (Nanos::Version::interface_is_at_least("copies_api", 1003)){
                  _root.retrieve_context().get_symbol_from_name("ompss_tmp_main").set_name("_nanox_main");
               } else {
                  _root.retrieve_context().get_symbol_from_name("ompss_tmp_main").set_name("main");                   
               }
            }
    }
    
    if (main.is_valid()){        
        //This function search for it's index in the pointer arrays
        //so we can pass it to the device array, we only add it on main
        Source search_function;
        //There can't be errors here, sooner or later we'll find the pointer (i hope)
        //If fortran, append _ so we can link correctly
        if (IS_FORTRAN_LANGUAGE){
            search_function << "int ompss_mpi_get_function_index_host_(void* func_pointer){";
        } else {
            search_function << "int ompss_mpi_get_function_index_host(void* func_pointer){";          
        }
        search_function << "int i=0;"
                           "for (i=0;ompss_mpi_func_pointers_host[i]!=func_pointer;i++);"
                           "return i;"
                           "}";       

        if (IS_FORTRAN_LANGUAGE)
          Source::source_language = SourceLanguage::C;
        Nodecl::NodeclBase search_function_tree = search_function.parse_global(_root);
        Source::source_language = SourceLanguage::Current;    

        if (IS_FORTRAN_LANGUAGE){
           _extra_c_code.append(search_function_tree); 
        } else {
           Nodecl::Utils::append_to_top_level_nodecl(search_function_tree); 
        }
    }
    
    if (_extra_c_code.is_null()) return;

    original_filename = TL::CompilationProcess::get_current_file().get_filename();
    std::string new_filename = "mpi_aux_nanox_outline_file_" + original_filename  + ".c";

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

void DeviceMPI::pre_run(DTO& dto) {
    _root = dto["nodecl"];
    _mpi_task_processed = false;
}

void DeviceMPI::run(DTO& dto) {
}

std::string DeviceMPI::get_ompss_mpi_type(Type type) {
    std::string result = "ompss_get_mpi_type(\"__mpitype_ompss_";
    if (type.is_char()) {
        result += "char";
    } else if (type.is_signed_short_int()) {
        result += "signed_short";
    } else if (type.is_signed_int()) {
        result += "signed_int";
    } else if (type.is_signed_long_int()) {
        result += "signed_long";
    } else if (type.is_signed_char()) {
        result += "signed_char";
    } else if (type.is_unsigned_char()) {
        result += "unsigned_char";
    } else if (type.is_unsigned_short_int()) {
        result += "unsigned_short";
    } else if (type.is_unsigned_int()) {
        result += "unsigned_int";
    } else if (type.is_unsigned_long_int()) {
        result += "unsigned_long";
    } else if (type.is_float()) {
        result += "float";
    } else if (type.is_double()) {
        result += "double";
    } else if (type.is_long_double()) {
        result += "long_double";
    } else if (type.is_bool()) {
        result += "bool";
    } else if (type.is_wchar_t()) {
        result += "wchar_t";
    } else {
        result += "byte";
    }
    result += "\")";
    return result;
}

EXPORT_PHASE(TL::Nanox::DeviceMPI);
