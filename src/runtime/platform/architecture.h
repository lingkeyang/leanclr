#pragma once

#include "vm/rt_managed_types.h"

namespace leanclr::os
{
class Architecture
{
  public:
    static vm::RtString* get_architecture_name();
    static vm::RtString* get_os_name();
};
} // namespace leanclr::os
