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

#ifndef TL_NANOX_NODECL_HPP
#define TL_NANOX_NODECL_HPP

#include "tl-compilerphase.hpp"
#include "tl-nodecl.hpp"

namespace TL { namespace Nanox {

    class Lowering : public TL::CompilerPhase
    {
        public:
            Lowering();

            virtual void phase_cleanup(DTO& data_flow);

            virtual void run(DTO& dto);
            virtual void pre_run(DTO& dto);

            Nodecl::List& get_extra_c_code();

            bool in_ompss_mode() const;
            bool instrumentation_enabled() const;
            bool final_clause_transformation_disabled() const;

        private:
            Nodecl::List _extra_c_code;

            FILE* _ancillary_file;
            FILE* get_ancillary_file();

            std::string _static_weak_symbols_str;
            bool _static_weak_symbols;
            void set_weaks_as_statics(const std::string& str);

            std::string _ompss_mode_str;
            bool _ompss_mode;
            void set_ompss_mode(const std::string& str);

            std::string _instrumentation_str;
            bool _instrumentation_enabled;
            void set_instrumentation(const std::string& str);

            std::string _final_clause_transformation_str;
            bool _final_clause_transformation_disabled;
            void set_disable_final_clause_transformation(const std::string& str);

            void finalize_phase(Nodecl::NodeclBase global_node);
            void set_openmp_programming_model(Nodecl::NodeclBase global_node);

            std::string _openmp_dry_run;
    };
} }

#endif // TL_NANOX_NODECL_HPP
