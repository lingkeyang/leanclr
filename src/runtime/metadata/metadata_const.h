#pragma once

#include "vm/rt_managed_types.h"

namespace leanclr::metadata
{
class MetadataConst
{
  public:
    static RtResult<vm::RtObject*> decode_const_object(RtModuleDef* mod, EncodedTokenId token, const RtTypeSig* typeSig);
};
} // namespace leanclr::metadata
