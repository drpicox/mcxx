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

#include "tl-omp-nanox.hpp"
#include "tl-nanos.hpp"

using namespace TL;
using namespace TL::Nanox;


OMPTransform::OMPTransform() : _compiler_alignment(true)
{
    set_phase_name("OpenMP for nanox");
    set_phase_description("This phase implements OpenMP targeting nanox runtime");

    on_directive_post["parallel"].connect(functor(&OMPTransform::parallel_postorder, *this));

    on_directive_post["task"].connect(functor(&OMPTransform::task_postorder, *this));

    on_directive_post["taskwait"].connect(functor(&OMPTransform::taskwait_postorder, *this));

    on_directive_post["single"].connect(functor(&OMPTransform::single_postorder, *this));

    on_directive_post["for"].connect(functor(&OMPTransform::for_postorder, *this));

    on_directive_post["atomic"].connect(functor(&OMPTransform::atomic_postorder, *this));

    on_directive_post["threadprivate"].connect(functor(&OMPTransform::threadprivate_postorder, *this));

    on_directive_post["barrier"].connect(functor(&OMPTransform::barrier_postorder, *this));

    on_directive_post["flush"].connect(functor(&OMPTransform::flush_postorder, *this));

    on_directive_post["target"].connect(functor(&OMPTransform::target_postorder, *this));

    register_parameter("instrument", 
            "Enables nanox instrumentation if set to '1'",
            _enable_instrumentation_str,
            "0").connect(functor(&OMPTransform::set_instrumentation, *this));

    register_parameter("compiler_alignment",
            "Let the compiler calculate variables alignment if set to '1'",
            _compiler_alignment_str, 
            "1").connect(functor(&OMPTransform::set_compiler_alignment, *this));

    on_directive_post["critical"].connect(functor(&OMPTransform::critical_postorder, *this));
    on_directive_post["master"].connect(functor(&OMPTransform::master_postorder, *this));

    on_directive_pre["sections"].connect(functor(&OMPTransform::sections_preorder, *this));
    on_directive_post["sections"].connect(functor(&OMPTransform::sections_postorder, *this));

    on_directive_pre["section"].connect(functor(&OMPTransform::section_preorder, *this));
    on_directive_post["section"].connect(functor(&OMPTransform::section_postorder, *this));
    
    // Not yet implemented
    on_directive_post["parallel|for"].connect(functor(&OMPTransform::unimplemented_yet, *this));
    on_directive_post["parallel|sections"].connect(functor(&OMPTransform::unimplemented_yet, *this));
    on_directive_post["flush"].connect(functor(&OMPTransform::unimplemented_yet, *this));
    on_directive_post["ordered"].connect(functor(&OMPTransform::unimplemented_yet, *this));
    on_directive_post["declare|reduction"].connect(functor(&OMPTransform::unimplemented_yet, *this));

    PragmaCustomCompilerPhase::warning_pragma_unused_clauses(true);
}

void OMPTransform::unimplemented_yet(PragmaCustomConstruct construct)
{
    running_error("%s: error: OpenMP construct/directive not implemented yet in Nanos++\n", 
            construct.get_ast().get_locus().c_str());
}

void OMPTransform::set_instrumentation(const std::string& str)
{
    _enable_instrumentation = false;
    parse_boolean_option(/* Parameter name */ "instrument", 
            /* Given value */ str, 
            /* Computed bool */ _enable_instrumentation, 
            /* Error message */  "Instrumentation disabled");
}

void OMPTransform::set_compiler_alignment(const std::string& str)
{
    parse_boolean_option("compiler_alignment", str, _compiler_alignment, "Assuming true.");
}

void OMPTransform::phase_cleanup(DTO& data_flow)
{
    _lock_names.clear();
    _converted_vlas.clear();
}

void OMPTransform::run(DTO& dto)
{
    if (strcmp(Nanos::Version::family.c_str(), "master") != 0 || Nanos::Version::version < 5000)
    {
        running_error("error: unsupported family '%s' and/or version '%d' of Nanos\n", 
                             Nanos::Version::family.c_str(), Nanos::Version::version);
    }

    OpenMP::OpenMPPhase::run(dto);
}

EXPORT_PHASE(TL::Nanox::OMPTransform)
