#pragma once

#include "vm/intrinsics.h"

namespace leanclr::intrinsics
{

class SystemNumericsVector
{
  public:
    static utils::Span<vm::IntrinsicEntry> get_intrinsic_entries();

    // Get whether hardware acceleration is supported
    static RtResult<bool> get_is_hardware_accelerated();
};

} // namespace leanclr::intrinsics
