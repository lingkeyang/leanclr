#pragma once

#include "rt_managed_types.h"
#include "metadata/rt_metadata.h"
#include "interp/interp_defs.h"

namespace leanclr::vm
{

// Structure containing invoker type and pointer
using RtInvokerType = metadata::RtInvokerType;

struct InvokeTypeAndMethod
{
    RtInvokerType invoker_type;
    metadata::RtInvokeMethodPointer invoker;

    InvokeTypeAndMethod(RtInvokerType type, metadata::RtInvokeMethodPointer ptr) : invoker_type(type), invoker(ptr)
    {
    }
};

class Shim
{
  public:
    // Public functions
    static RtResult<InvokeTypeAndMethod> get_invoker(const metadata::RtMethodInfo* method);
    static metadata::RtInvokeMethodPointer get_virtual_invoker(const metadata::RtMethodInfo* method, const InvokeTypeAndMethod& invoker_data);
    static metadata::RtManagedMethodPointer get_method_pointer(const metadata::RtMethodInfo* method);
};

} // namespace leanclr::vm
