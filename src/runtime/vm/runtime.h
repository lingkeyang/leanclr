#pragma once

#include "rt_managed_types.h"
#include "interp/interp_defs.h"

namespace leanclr::vm
{

class Runtime
{
  public:
    static RtResultVoid initialize();

    // Static constructor runners
    static RtResultVoid run_class_static_constructor(metadata::RtClass* klass);
    static RtResult<const metadata::RtMethodInfo*> get_module_constructor(metadata::RtModuleDef* module);
    static RtResultVoid run_module_static_constructor(metadata::RtModuleDef* module);

    // Method invocation functions
    static RtResult<RtObject*> invoke_with_run_cctor(const metadata::RtMethodInfo* method, RtObject* obj, const void* const* params);

    static RtResult<RtObject*> invoke_array_arguments_without_run_cctor(const metadata::RtMethodInfo* method, RtObject* obj, RtArray* params);

    static RtResult<RtObject*> invoke_array_arguments_with_run_cctor(const metadata::RtMethodInfo* method, RtObject* obj, RtArray* params);

    static RtResultVoid invoke_stackobject_arguments_without_run_cctor(const metadata::RtMethodInfo* method, const interp::RtStackObject* params,
                                                                       interp::RtStackObject* ret);

    static RtResultVoid invoke_stackobject_arguments_with_run_cctor(const metadata::RtMethodInfo* method, const interp::RtStackObject* params,
                                                                    interp::RtStackObject* ret);

  private:
};
} // namespace leanclr::vm
