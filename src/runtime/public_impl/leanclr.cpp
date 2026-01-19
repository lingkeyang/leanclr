#include "public/leanclr.h"
#include "vm/pinvoke.h"

namespace leanclr
{

void register_pinvoke_func(const char* name, vm::PInvokeFunction func, vm::PInvokeInvoker invoker)
{
    vm::PInvokes::register_pinvoke(name, func, invoker);
}
}