#pragma once

#include "rt_base.h"

namespace leanclr::pal
{
class Hardware
{
  public:
    static constexpr bool is_hardware_accelerated()
    {
        return false;
    }
};
} // namespace leanclr::pal
