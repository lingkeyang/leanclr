#include "assembly.h"

#include "metadata/module_def.h"
#include "alloc/general_allocation.h"
#include "alloc/mem_pool.h"
#include "utils/rt_unique_ptr.h"
#include "metadata/pe_image_reader.h"
#include "const_strs.h"
#include "settings.h"
#include "rt_array.h"
#include "class.h"
#include "reflection.h"

namespace leanclr::vm
{

RtResult<metadata::RtAssembly*> Assembly::load_corlib()
{
    return load_by_name(STR_CORLIB_NAME);
}

metadata::RtAssembly* Assembly::get_corlib()
{
    auto corlibMod = metadata::RtModuleDef::get_corlib_module();
    auto corlibAss = corlibMod ? corlibMod->get_assembly() : nullptr;
    return corlibAss;
}

metadata::RtAssembly* Assembly::find_by_name(const char* name_no_ext)
{
    metadata::RtModuleDef* mod = metadata::RtModuleDef::find_module(name_no_ext);
    return mod ? mod->get_assembly() : nullptr;
}

RtResult<metadata::RtAssembly*> Assembly::load_by_name(const char* name)
{
    metadata::RtModuleDef* mod = metadata::RtModuleDef::find_module(name);
    if (mod)
    {
        RET_OK(mod->get_assembly());
    }

    auto loader = vm::Settings::get_assembly_loader();
    if (!loader)
    {
        RET_ERR(RtErr::FileNotFound);
    }
    auto result = loader(name);
    DECLARING_AND_UNWRAP_OR_RET_ERR_ON_FAIL2(utils::Span<byte>, dllData, result);

    return load_from_data(dllData);
}

RtResult<metadata::RtAssembly*> Assembly::load_by_name(RtAppDomain* app_domain, const char* name_no_ext, RtObject* evidence, bool ref_only,
                                                       RtStackCrawlMark& stack_crawl_mark)
{
    // FIXME
    return load_by_name(name_no_ext);
}

RtResult<metadata::RtAssembly*> Assembly::load_from_data(utils::Span<byte> dllData)
{
    alloc::MemPool* pool = alloc::GeneralAllocation::new_any<alloc::MemPool>();
    utils::UniquePtr<alloc::MemPool> poolGuard(pool);
    utils::UniquePtr<byte> dllDataGuard(dllData.data());

    metadata::PeImageReader reader(dllData.data(), dllData.size());

    DECLARING_AND_UNWRAP_OR_RET_ERR_ON_FAIL(metadata::CliImage*, image, reader.ReadCliImage(*pool));
    RET_ERR_ON_FAIL(image->load_streams());
    RET_ERR_ON_FAIL(image->load_tables(*pool));

    metadata::RtAssembly* ass = alloc::GeneralAllocation::malloc_any_zeroed<metadata::RtAssembly>();
    metadata::RtModuleDef* mod = alloc::GeneralAllocation::new_any<metadata::RtModuleDef>(ass, *image, *pool);
    ass->mod = mod;
    RET_ERR_ON_FAIL(mod->load());

    if (metadata::RtModuleDef::find_module(mod->get_name_no_ext()))
    {
        mod->~RtModuleDef();
        RET_ERR(RtErr::ModuleAlreadyLoaded);
    }
    metadata::RtModuleDef::register_module_def(mod);

    // don't free mem pool if succ
    poolGuard.release();
    dllDataGuard.release();
    RET_OK(ass);
}

RtResult<metadata::RtAssembly*> Assembly::load_from_data(RtAppDomain* app_domain, RtArray* dll_data, RtArray* symbol_data, RtObject* evidence, bool ref_only)
{
    if (!dll_data)
    {
        RET_ERR(RtErr::ArgumentNull);
    }
    return load_from_data(utils::Span<uint8_t>(Array::get_array_data_start_as<uint8_t>(dll_data), Array::get_array_length(dll_data)));
}

RtResult<RtArray*> Assembly::get_types(metadata::RtAssembly* ass, bool exported_only)
{
    utils::Vector<metadata::RtClass*> types;
    RET_ERR_ON_FAIL(ass->mod->get_types(exported_only, types));

    metadata::RtClass* cls_type = Class::get_corlib_types().cls_systemtype;
    DECLARING_AND_UNWRAP_OR_RET_ERR_ON_FAIL(RtArray*, types_arr, Array::new_array_from_ele_klass(cls_type, static_cast<int32_t>(types.size())));

    for (size_t i = 0; i < types.size(); ++i)
    {
        DECLARING_AND_UNWRAP_OR_RET_ERR_ON_FAIL(RtReflectionType*, ref_type, Reflection::get_klass_reflection_object(types[i]));
        Array::set_array_data_at<RtReflectionType*>(types_arr, static_cast<int32_t>(i), ref_type);
    }
    RET_OK(types_arr);
}

} // namespace leanclr::vm
