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

#include "tl-lowering-visitor.hpp"
#include "tl-counters.hpp"
#include "tl-source.hpp"
#include "tl-nanos.hpp"
#include "tl-datareference.hpp"

namespace TL { namespace Nanox {

struct TaskWaitVisitor : public Nodecl::ExhaustiveVisitor<void>
{
    public:
        bool is_noflush;

        TaskWaitVisitor()
            : is_noflush(false)
        {
        }

        void visit(const Nodecl::OpenMP::NoFlush&)
        {
            is_noflush = true;
        }
};

void LoweringVisitor::fill_dependences_taskwait(
        Nodecl::NodeclBase ctr,
        OutlineInfo& outline_info,
        // out
        Source& result_src
        )
{
    fill_dependences_internal(ctr, outline_info, Source(""), /* on_wait */ true, result_src);
}

void LoweringVisitor::emit_wait_async(Nodecl::NodeclBase construct,
        bool has_dependences,
        OutlineInfo& outline_info,
        bool is_noflush)
{
    Source src;

    if (!has_dependences)
    {
        src << "{"
            <<     "nanos_wd_t nanos_wd_ = nanos_current_wd();"
            <<     "nanos_err_t err;"
            <<     "err = nanos_wg_wait_completion(nanos_wd_, " << (is_noflush ? "1" : "0") << ");"
            <<     "if (err != NANOS_OK) nanos_handle_error(err);"
            << "}"
            ;
    }
    else
    {
        Source dependences;
        fill_dependences_taskwait(
                construct,
                outline_info,
                dependences);

        int num_dependences = count_dependences(outline_info);

        src << "{"
            <<     dependences
            <<     "nanos_err_t err = nanos_wait_on(" << num_dependences << ", dependences);"
            <<     "if (err != NANOS_OK) nanos_handle_error(err);"
            << "}"
            ;
    }


    FORTRAN_LANGUAGE()
    {
        // Parse in C
        Source::source_language = SourceLanguage::C;
    }

    Nodecl::NodeclBase n = src.parse_statement(construct);

    FORTRAN_LANGUAGE()
    {
        Source::source_language = SourceLanguage::Current;
    }

    construct.replace(n);
}

void LoweringVisitor::visit(const Nodecl::OpenMP::TaskwaitShallow& construct)
{
    OutlineInfo outline_info(Nodecl::NodeclBase::null());
    TaskWaitVisitor taskwait_info;
    taskwait_info.walk(construct.get_environment());

    emit_wait_async(construct, /* has_dependences */ false, outline_info, taskwait_info.is_noflush);
}

void LoweringVisitor::visit(const Nodecl::OpenMP::WaitOnDependences& construct)
{
    Nodecl::NodeclBase environment = construct.get_environment();
    OutlineInfo outline_info(environment);

    TaskWaitVisitor taskwait_info;
    taskwait_info.walk(construct.get_environment());

    emit_wait_async(construct, /* has_dependences */ true, outline_info, taskwait_info.is_noflush);
}

} }
