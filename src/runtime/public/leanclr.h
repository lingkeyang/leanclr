#pragma once

#include <cstring>
#include "metadata/rt_metadata.h"
#include "vm/pinvoke.h"

namespace leanclr
{

    constexpr size_t EVAL_STACK_OBJECT_SIZE = 8; // Assuming RtStackObject is 8 bytes

    template <typename T>
    constexpr size_t get_eval_stack_size_for_type()
    {
        return (sizeof(T) + EVAL_STACK_OBJECT_SIZE - 1) / EVAL_STACK_OBJECT_SIZE;
    }

    template <typename T>
    void push_argument_to_eval_stack(interp::RtStackObject* eval_stack, size_t& offset, const T& value)
    {
        *(T*)(eval_stack + offset) = value;
        offset += get_eval_stack_size_for_type<T>();
    }

    template <typename T>
    void get_argument_from_eval_stack(const interp::RtStackObject* eval_stack, size_t& offset, T* value)
    {
        *value = *(T*)(eval_stack + offset);
        offset += get_eval_stack_size_for_type<T>();
    }

    template <typename T>
    T get_argument_from_eval_stack(const interp::RtStackObject* eval_stack, size_t& offset)
    {
        size_t old_offset = offset;
        offset += get_eval_stack_size_for_type<T>();
        return *(T*)(eval_stack + old_offset);
    }

    template <typename T>
    void set_return_value_to_eval_stack(interp::RtStackObject* eval_stack, const T& value)
    {
        *(T*)eval_stack = value;
    }

    inline void set_return_value_to_eval_stack(interp::RtStackObject* eval_stack, void* obj, size_t byte_size)
    {
        std::memcpy(eval_stack, obj, byte_size);
    }

    void register_pinvoke_func(const char* name, vm::PInvokeFunction func, vm::PInvokeInvoker invoker);

} // namespace leanclr