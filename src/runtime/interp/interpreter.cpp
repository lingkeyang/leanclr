#include "interpreter.h"
#include "../vm/class.h"
#include "../metadata/module_def.h"
#include "hl_transformer.h"
#include "ll_transformer.h"
#include "machine_state.h"
#include "../vm/object.h"
#include "../vm/rt_array.h"
#include "../vm/method.h"
#include "../vm/runtime.h"
#include "../vm/internal_calls.h"
#include "../vm/intrinsics.h"
#include "../vm/rt_exception.h"
#include "../vm/enum.h"

namespace leanclr::interp
{

static RtResult<const RtInterpMethodInfo*> transform(const metadata::RtMethodInfo* method)
{
    metadata::RtClass* klass = method->parent;
    metadata::RtModuleDef* mod = !vm::Class::is_array_or_szarray(klass) ? klass->image : klass->parent->image;
    auto retMethodBody = mod->read_method_body(method->token);
    RET_ERR_ON_FAIL(retMethodBody);
    auto& optMethodBody = retMethodBody.unwrap();
    if (!optMethodBody)
    {
        RET_ERR(RtErr::ExecutionEngine);
    }

    metadata::RtMethodBody& methodBody = optMethodBody.value();
    size_t guessSize = methodBody.code_size * 32;
    size_t pageSize = 1024;
    alloc::MemPool pool(guessSize, pageSize, utils::MemOp::align_up(guessSize, pageSize));
    hl::Transformer hl_transformer(mod, method, methodBody, pool);
    RET_ERR_ON_FAIL(hl_transformer.transform());
    ll::Transformer ll_transformer(hl_transformer, pool);
    RET_ERR_ON_FAIL(ll_transformer.transform());
    return ll_transformer.build_interp_method_info();
}

RtResult<const RtInterpMethodInfo*> Interpreter::init_interpreter_method(const metadata::RtMethodInfo* method)
{
    assert(!method->interp_data);
    RET_ERR_ON_FAIL(vm::Class::initialize_all(method->parent));
    DECLARING_AND_UNWRAP_OR_RET_ERR_ON_FAIL(const RtInterpMethodInfo*, interp_method, transform(method));
    const_cast<metadata::RtMethodInfo*>(method)->interp_data = interp_method;
    RET_OK(interp_method);
}

#if LEANCLR_USE_COMPUTED_GOTO_DISPATCHER
#define LEANCLR_SWITCH0() goto* in_lables0[*ip]
#define LEANCLR_CONTINUE0() goto* in_lables0[*ip]
#define LEANCLR_CASE0(code) LABLE0_##code:
#else

#define LEANCLR_CASE_END()                         \
    ip = reinterpret_cast<const uint8_t*>(ir + 1); \
    continue;                                      \
    }

#define LEANCLR_CASE_END_LITE() \
    continue;                   \
    }

#define LEANCLR_SWITCH0() switch (*(ll::OpCodeValue0*)ip)
#define LEANCLR_CONTINUE0() continue
#define LEANCLR_CASE_BEGIN0(code) \
    case ll::OpCodeValue0::code:  \
    {                             \
        const auto ir = (ll::code*)ip;
#define LEANCLR_CASE_BEGIN_LITE0(code) \
    case ll::OpCodeValue0::code:       \
    {
#define LEANCLR_CASE_END0() LEANCLR_CASE_END()
#define LEANCLR_CASE_END_LITE0() LEANCLR_CASE_END_LITE()

#define LEANCLR_SWITCH1() switch (*(ll::OpCodeValue1*)ip1)
#define LEANCLR_CONTINUE1() continue
#define LEANCLR_CASE_BEGIN1(code) \
    case ll::OpCodeValue1::code:  \
    {                             \
        const auto ir = (ll::code*)ip;
#define LEANCLR_CASE_BEGIN_LITE1(code) \
    case ll::OpCodeValue1::code:       \
    {
#define LEANCLR_CASE_END1() LEANCLR_CASE_END()
#define LEANCLR_CASE_END_LITE1() LEANCLR_CASE_END_LITE()

#define LEANCLR_SWITCH2() switch (*(ll::OpCodeValue2*)ip2)
#define LEANCLR_CONTINUE2() continue
#define LEANCLR_CASE_BEGIN2(code) \
    case ll::OpCodeValue2::code:  \
    {                             \
        const auto ir = (ll::code*)ip;
#define LEANCLR_CASE_BEGIN_LITE2(code) \
    case ll::OpCodeValue2::code:       \
    {
#define LEANCLR_CASE_END2() LEANCLR_CASE_END()
#define LEANCLR_CASE_END_LITE2() LEANCLR_CASE_END_LITE()

#define LEANCLR_SWITCH3() switch (*(ll::OpCodeValue3*)ip3)
#define LEANCLR_CONTINUE3() continue
#define LEANCLR_CASE_BEGIN3(code) \
    case ll::OpCodeValue3::code:  \
    {                             \
        const auto ir = (ll::code*)ip;
#define LEANCLR_CASE_BEGIN_LITE3(code) \
    case ll::OpCodeValue3::code:       \
    {
#define LEANCLR_CASE_END3() LEANCLR_CASE_END()
#define LEANCLR_CASE_END_LITE3() LEANCLR_CASE_END_LITE()

#define LEANCLR_SWITCH4() switch (*(ll::OpCodeValue4*)ip4)
#define LEANCLR_CONTINUE4() continue
#define LEANCLR_CASE_BEGIN4(code) \
    case ll::OpCodeValue4::code:  \
    {                             \
        const auto ir = (ll::code*)ip;
#define LEANCLR_CASE_BEGIN_LITE4(code) case ll::OpCodeValue4::code:
#define LEANCLR_CASE_END4() LEANCLR_CASE_END()
#define LEANCLR_CASE_END_LITE4() LEANCLR_CASE_END_LITE()

#define LEANCLR_SWITCH5() switch (*(ll::OpCodeValue5*)ip5)
#define LEANCLR_CONTINUE5() continue
#define LEANCLR_CASE_BEGIN5(code) \
    case ll::OpCodeValue5::code:  \
    {                             \
        const auto ir = (ll::code*)ip;
#define LEANCLR_CASE_BEGIN_LITE5(code) \
    case ll::OpCodeValue5::code:       \
    {
#define LEANCLR_CASE_END5() LEANCLR_CASE_END()
#define LEANCLR_CASE_END_LITE5() LEANCLR_CASE_END_LITE()
#endif

template <typename T>
inline T get_stack_value_at(RtStackObject* base, size_t index)
{
    RtStackObject* obj = base + index;
    return *reinterpret_cast<T*>(obj);
}

template <typename T>
inline void set_stack_value_at(RtStackObject* base, size_t index, T value)
{
    RtStackObject* obj = base + index;
    *reinterpret_cast<T*>(obj) = value;
}

template <typename T>
inline T* get_ptr_stack_value_at(RtStackObject* base, size_t index)
{
    RtStackObject* obj = base + index;
    return reinterpret_cast<T*>(obj);
}

template <typename T>
inline T get_ind_stack_value_at(RtStackObject* base, size_t index)
{
    RtStackObject* obj = base + index;
    return *(reinterpret_cast<T*>(obj->ptr));
}

template <typename T>
inline void set_ind_stack_value_at(RtStackObject* base, size_t index, T value)
{
    RtStackObject* obj = base + index;
    *(reinterpret_cast<T*>(obj->ptr)) = value;
}

template <typename T>
inline T* get_resolved_data(const RtInterpMethodInfo* imi, size_t index)
{
    return (T*)imi->resolved_datas[index];
}

template <typename Src, typename Dst>
inline int32_t cast_float_to_small_int(Src value)
{
    return (int32_t)(Dst)(int32_t)(value);
}

template <typename Src, typename Dst>
inline int32_t cast_float_to_i32(Src value)
{
    if (value >= 0.0)
    {
        return (int32_t)(Dst)(value);
    }
    else
    {
        return (int32_t)(Dst)(int64_t)value;
    }
}

template <typename Src, typename Dst>
inline int64_t cast_float_to_i64(Src value)
{
    if (value >= 0.0)
    {
        return (int64_t)(uint64_t)value;
    }
    else
    {
        return (int64_t)(value);
    }
}

// Compiler-agnostic overflow checking wrappers
// Use built-in functions when available (GCC/Clang), fall back to manual checks for MSVC
#if defined(__GNUC__) || defined(__clang__)
// GCC and Clang support __builtin_*_overflow functions
#define CHECK_ADD_OVERFLOW_I32(a, b, result) __builtin_add_overflow(a, b, result)
#define CHECK_ADD_OVERFLOW_I64(a, b, result) __builtin_add_overflow(a, b, result)
#define CHECK_ADD_OVERFLOW_U32(a, b, result) __builtin_add_overflow(a, b, result)
#define CHECK_ADD_OVERFLOW_U64(a, b, result) __builtin_add_overflow(a, b, result)
#define CHECK_SUB_OVERFLOW_I32(a, b, result) __builtin_sub_overflow(a, b, result)
#define CHECK_SUB_OVERFLOW_I64(a, b, result) __builtin_sub_overflow(a, b, result)
#define CHECK_SUB_OVERFLOW_U32(a, b, result) __builtin_sub_overflow(a, b, result)
#define CHECK_SUB_OVERFLOW_U64(a, b, result) __builtin_sub_overflow(a, b, result)
#define CHECK_MUL_OVERFLOW_I32(a, b, result) __builtin_mul_overflow(a, b, result)
#define CHECK_MUL_OVERFLOW_I64(a, b, result) __builtin_mul_overflow(a, b, result)
#define CHECK_MUL_OVERFLOW_U32(a, b, result) __builtin_mul_overflow(a, b, result)
#define CHECK_MUL_OVERFLOW_U64(a, b, result) __builtin_mul_overflow(a, b, result)
#else
// Fallback for MSVC and other compilers: manual overflow checking
inline bool check_add_overflow_i32(int32_t a, int32_t b, int32_t* result)
{
    int64_t res = static_cast<int64_t>(a) + static_cast<int64_t>(b);
    if (res > INT32_MAX || res < INT32_MIN)
        return true;
    *result = static_cast<int32_t>(res);
    return false;
}
inline bool check_add_overflow_i64(int64_t a, int64_t b, int64_t* result)
{
    if ((b > 0 && a > INT64_MAX - b) || (b < 0 && a < INT64_MIN - b))
        return true;
    *result = a + b;
    return false;
}
inline bool check_add_overflow_u32(uint32_t a, uint32_t b, uint32_t* result)
{
    if (a > UINT32_MAX - b)
        return true;
    *result = a + b;
    return false;
}
inline bool check_add_overflow_u64(uint64_t a, uint64_t b, uint64_t* result)
{
    if (a > UINT64_MAX - b)
        return true;
    *result = a + b;
    return false;
}
inline bool check_sub_overflow_i32(int32_t a, int32_t b, int32_t* result)
{
    int64_t res = static_cast<int64_t>(a) - static_cast<int64_t>(b);
    if (res > INT32_MAX || res < INT32_MIN)
        return true;
    *result = static_cast<int32_t>(res);
    return false;
}
inline bool check_sub_overflow_i64(int64_t a, int64_t b, int64_t* result)
{
    if ((b > 0 && a < INT64_MIN + b) || (b < 0 && a > INT64_MAX + b))
        return true;
    *result = a - b;
    return false;
}
inline bool check_sub_overflow_u32(uint32_t a, uint32_t b, uint32_t* result)
{
    if (a < b)
        return true;
    *result = a - b;
    return false;
}
inline bool check_sub_overflow_u64(uint64_t a, uint64_t b, uint64_t* result)
{
    if (a < b)
        return true;
    *result = a - b;
    return false;
}
inline bool check_mul_overflow_i32(int32_t a, int32_t b, int32_t* result)
{
    if (b != 0 && (a > INT32_MAX / b || a < INT32_MIN / b))
        return true;
    if (a == INT32_MIN && b == -1) // Special case: overflow
        return true;
    *result = a * b;
    return false;
}
inline bool check_mul_overflow_i64(int64_t a, int64_t b, int64_t* result)
{
    if (b != 0 && (a > INT64_MAX / b || a < INT64_MIN / b))
        return true;
    if (a == INT64_MIN && b == -1) // Special case: overflow
        return true;
    *result = a * b;
    return false;
}
inline bool check_mul_overflow_u32(uint32_t a, uint32_t b, uint32_t* result)
{
    if (b != 0 && a > UINT32_MAX / b)
        return true;
    *result = a * b;
    return false;
}
inline bool check_mul_overflow_u64(uint64_t a, uint64_t b, uint64_t* result)
{
    if (b != 0 && a > UINT64_MAX / b)
        return true;
    *result = a * b;
    return false;
}
#define CHECK_ADD_OVERFLOW_I32(a, b, result) check_add_overflow_i32(a, b, result)
#define CHECK_ADD_OVERFLOW_I64(a, b, result) check_add_overflow_i64(a, b, result)
#define CHECK_ADD_OVERFLOW_U32(a, b, result) check_add_overflow_u32(a, b, result)
#define CHECK_ADD_OVERFLOW_U64(a, b, result) check_add_overflow_u64(a, b, result)
#define CHECK_SUB_OVERFLOW_I32(a, b, result) check_sub_overflow_i32(a, b, result)
#define CHECK_SUB_OVERFLOW_I64(a, b, result) check_sub_overflow_i64(a, b, result)
#define CHECK_SUB_OVERFLOW_U32(a, b, result) check_sub_overflow_u32(a, b, result)
#define CHECK_SUB_OVERFLOW_U64(a, b, result) check_sub_overflow_u64(a, b, result)
#define CHECK_MUL_OVERFLOW_I32(a, b, result) check_mul_overflow_i32(a, b, result)
#define CHECK_MUL_OVERFLOW_I64(a, b, result) check_mul_overflow_i64(a, b, result)
#define CHECK_MUL_OVERFLOW_U32(a, b, result) check_mul_overflow_u32(a, b, result)
#define CHECK_MUL_OVERFLOW_U64(a, b, result) check_mul_overflow_u64(a, b, result)
#endif

struct ThrowFlow
{
    vm::RtException* ex;
    InterpFrame* frame;
    const void* ip;
    size_t next_search_clause_idx;
    const RtInterpExceptionClause* cur_clause;
    bool handled;
};

struct LeaveFlow
{
    InterpFrame* frame;
    const void* src_ip;
    const void* target_ip;
    const RtInterpExceptionClause* cur_finally_clause;
    size_t next_search_clause_idx;
    size_t remain_finally_clause_count;
};

struct ExceptionFlow
{
    bool throw_flow;
    union
    {
        ThrowFlow throw_data;
        LeaveFlow leave_data;
    };
};

static utils::Vector<ExceptionFlow> s_exception_flows;

ExceptionFlow* peek_top_exception_flow()
{
    assert(!s_exception_flows.empty());
    return &s_exception_flows.back();
}

void push_throw_flow(vm::RtException* ex, InterpFrame* frame, const void* ip)
{
    ExceptionFlow flow;
    flow.throw_flow = true;
    auto& data = flow.throw_data;
    data.ex = ex;
    data.frame = frame;
    data.ip = ip;
    data.next_search_clause_idx = 0;
    data.cur_clause = nullptr;
    data.handled = false;
    s_exception_flows.push_back(flow);
}

void pop_throw_flow(vm::RtException* ex, InterpFrame* frame)
{
    assert(!s_exception_flows.empty());
    ExceptionFlow& flow = s_exception_flows.back();
    assert(flow.throw_flow);
    auto& data = flow.throw_data;
    assert(data.ex == ex);
    assert(data.frame == frame);
    s_exception_flows.pop_back();
}

void pop_leave_flow(InterpFrame* frame)
{
    assert(!s_exception_flows.empty());
    ExceptionFlow& flow = s_exception_flows.back();
    assert(!flow.throw_flow);
    auto& data = flow.leave_data;
    assert(data.frame == frame);
    s_exception_flows.pop_back();
}

void pop_outscope_flows(const RtInterpMethodInfo* imi, InterpFrame* frame, const void* ip)
{
    uint32_t ip_offset = static_cast<uint32_t>((const uint8_t*)ip - imi->codes);
    while (!s_exception_flows.empty())
    {
        ExceptionFlow& flow = s_exception_flows.back();
        if (flow.throw_flow)
        {
            auto& data = flow.throw_data;
            assert(data.cur_clause);
            if (data.frame != frame)
            {
                break;
            }
            if (data.cur_clause->is_in_handler_block(ip_offset))
            {
                break;
            }
        }
        else
        {
            auto& data = flow.leave_data;
            assert(data.cur_finally_clause);
            if (data.frame != frame)
            {
                break;
            }
            if (data.cur_finally_clause->is_in_handler_block(ip_offset))
            {
                break;
            }
        }
        s_exception_flows.pop_back();
    }
}

void pop_all_flow_of_cur_frame_exclude_last(InterpFrame* frame)
{
    while (!s_exception_flows.empty())
    {
        ExceptionFlow& flow = s_exception_flows.back();
        if (flow.throw_flow)
        {
            auto& data = flow.throw_data;
            if (data.frame != frame)
            {
                break;
            }
        }
        else
        {
            auto& data = flow.leave_data;
            assert(data.cur_finally_clause);
            if (data.frame != frame)
            {
                break;
            }
        }
        s_exception_flows.pop_back();
    }
}

void setup_filter_checker(const RtInterpExceptionClause* clause)
{
    assert(!s_exception_flows.empty());
    ExceptionFlow& flow = s_exception_flows.back();
    assert(flow.throw_flow);
    auto& data = flow.throw_data;
    assert(!data.handled);
    data.cur_clause = clause;
}

void setup_filter_handler(const RtInterpMethodInfo* imi, InterpFrame* frame, const void* handler_start_ip)
{
    assert(!s_exception_flows.empty());
    ExceptionFlow& flow = s_exception_flows.back();
    assert(flow.throw_flow);
    auto& data = flow.throw_data;
    assert(!data.handled);
    assert(data.cur_clause);
    assert(imi->codes + data.cur_clause->handler_begin_offset == handler_start_ip);

    ExceptionFlow new_flow = {};
    new_flow.throw_flow = true;
    auto& new_data = new_flow.throw_data;
    new_data.ex = data.ex;
    new_data.frame = data.frame;
    new_data.ip = data.ip;
    new_data.next_search_clause_idx = data.next_search_clause_idx;
    new_data.cur_clause = nullptr;
    new_data.handled = true;
    pop_outscope_flows(imi, frame, handler_start_ip);
    s_exception_flows.push_back(new_flow);
}

void setup_catch_handler(const RtInterpMethodInfo* imi, InterpFrame* frame, const RtInterpExceptionClause* clause, const void* handler_start_ip)
{
    assert(!s_exception_flows.empty());
    ExceptionFlow flow = s_exception_flows.back();
    s_exception_flows.pop_back();
    assert(flow.throw_flow);
    auto& data = flow.throw_data;
    assert(!data.handled);
    assert(imi->codes + clause->handler_begin_offset == handler_start_ip);

    ExceptionFlow new_flow = {};
    new_flow.throw_flow = true;
    auto& new_data = new_flow.throw_data;
    new_data.ex = data.ex;
    new_data.frame = data.frame;
    new_data.ip = data.ip;
    new_data.next_search_clause_idx = data.next_search_clause_idx;
    new_data.cur_clause = clause;
    new_data.handled = true;
    pop_outscope_flows(imi, frame, handler_start_ip);
    s_exception_flows.push_back(new_flow);
}

void setup_finally_or_fault_handler(const RtInterpMethodInfo* imi, const RtInterpExceptionClause* clause, const void* handler_start_ip)
{
    assert(!s_exception_flows.empty());
    ExceptionFlow& flow = s_exception_flows.back();
    assert(flow.throw_flow);
    auto& data = flow.throw_data;
    assert(!data.handled);
    assert(imi->codes + clause->handler_begin_offset == handler_start_ip);
    data.cur_clause = clause;
}

void push_leave_flow(InterpFrame* frame, const void* src_ip, const void* target_ip, const RtInterpExceptionClause* clause, size_t next_search_clause_idx,
                     size_t finally_clause_count)
{
    ExceptionFlow flow;
    flow.throw_flow = false;
    auto& data = flow.leave_data;
    data.frame = frame;
    data.src_ip = src_ip;
    data.target_ip = target_ip;
    data.cur_finally_clause = clause;
    data.next_search_clause_idx = next_search_clause_idx;
    data.remain_finally_clause_count = finally_clause_count;
    s_exception_flows.push_back(flow);
}

bool is_in_filter_check_flow(InterpFrame* frame)
{
    if (s_exception_flows.empty())
    {
        return false;
    }
    ExceptionFlow& flow = s_exception_flows.back();
    if (!flow.throw_flow)
    {
        return false;
    }
    auto& data = flow.throw_data;
    return data.frame == frame && !data.handled && data.cur_clause && data.cur_clause->flags == metadata::RtILExceptionClauseType::Filter;
}

vm::RtException* find_exception_in_enclosing_throw_flow(InterpFrame* frame, uint32_t ip_offset)
{
    for (size_t i = s_exception_flows.size(); i > 0; --i)
    {
        ExceptionFlow& flow = s_exception_flows[i - 1];
        if (flow.throw_flow)
        {
            auto& data = flow.throw_data;
            if (data.frame != frame)
            {
                return nullptr;
            }
            if (data.cur_clause && data.cur_clause->is_in_handler_block(ip_offset))
            {
                return data.ex;
            }
        }
    }
    return nullptr;
}

vm::RtException* get_exception_in_last_throw_flow(InterpFrame* frame, uint32_t ip_offset)
{
    assert(!s_exception_flows.empty());
    ExceptionFlow& flow = s_exception_flows.back();
    assert(flow.throw_flow);
    auto& data = flow.throw_data;
    assert(data.frame == frame);
    assert(data.cur_clause && (data.cur_clause->is_in_handler_block(ip_offset) || data.cur_clause->is_in_filter_block(ip_offset)));
    return data.ex;
}

#define RAISE_RUNTIME_ERROR(err)                                                       \
    if (is_in_filter_check_flow(frame))                                                \
    {                                                                                  \
        goto unwind_exception_handler;                                                 \
    }                                                                                  \
    {                                                                                  \
        vm::RtException* ex = vm::Exception::raise_error_as_exception(err, frame, ip); \
        push_throw_flow(ex, frame, ip);                                                \
        frame->save(ip);                                                               \
        goto unwind_exception_handler;                                                 \
    }

#define RAISE_RUNTIME_EXCEPTION(ex)                                            \
    if (is_in_filter_check_flow(frame))                                        \
    {                                                                          \
        goto unwind_exception_handler;                                         \
    }                                                                          \
    {                                                                          \
        vm::RtException* __ex = vm::Exception::raise_exception(ex, frame, ip); \
        push_throw_flow(__ex, frame, ip);                                      \
        frame->save(ip);                                                       \
        goto unwind_exception_handler;                                         \
    }

#define HANDLE_RAISE_RUNTIME_ERROR(type, ret, expr) \
    auto&& __##ret = (expr);                        \
    if (__##ret.is_err())                           \
    {                                               \
        RAISE_RUNTIME_ERROR(__##ret.unwrap_err());  \
    }                                               \
    type ret = __##ret.unwrap();

#define HANDLE_RAISE_RUNTIME_ERROR2(ret, expr)        \
    {                                                 \
        auto&& __temp = (expr);                       \
        if (__temp.is_err())                          \
        {                                             \
            RAISE_RUNTIME_ERROR(__temp.unwrap_err()); \
        }                                             \
        ret = __temp.unwrap();                        \
    }

#define HANDLE_RAISE_RUNTIME_ERROR_VOID(expr)         \
    {                                                 \
        auto&& __temp = (expr);                       \
        if (__temp.is_err())                          \
        {                                             \
            RAISE_RUNTIME_ERROR(__temp.unwrap_err()); \
        }                                             \
    }

#define TRY_RUN_CLASS_STATIC_CCTOR(klass)                                                      \
    {                                                                                          \
        if (vm::Class::is_cctor_not_finished(klass))                                           \
        {                                                                                      \
            HANDLE_RAISE_RUNTIME_ERROR_VOID(vm::Runtime::run_class_static_constructor(klass)); \
        }                                                                                      \
    }

#define ENTER_INTERP_FRAME(_method, _frame_base_idx, _next_ip)                                                    \
    frame->save(_next_ip);                                                                                        \
    HANDLE_RAISE_RUNTIME_ERROR2(frame, ms.enter_frame_from_interp(_method, eval_stack_base + (_frame_base_idx))); \
    ip = frame->ip;                                                                                               \
    goto method_start;

#define LEAVE_FRAME()                  \
    frame = ms.leave_frame(sp, frame); \
    if (!frame)                        \
    {                                  \
        goto end_loop;                 \
    }                                  \
    ip = frame->ip;                    \
    goto method_start;

template <typename T>
T* get_static_field_address(const metadata::RtFieldInfo* field)
{
    metadata::RtClass* klass = field->parent;
    return reinterpret_cast<T*>(klass->static_fields_data + field->offset);
}

RtResult<const RtStackObject*> Interpreter::execute(const metadata::RtMethodInfo* method, const interp::RtStackObject* params)
{
    MachineState& ms = MachineState::get_global_machine_state();
    MachineStateSavePoint sp(ms);
    DECLARING_AND_UNWRAP_OR_RET_ERR_ON_FAIL3(InterpFrame*, frame, ms.enter_frame_from_native(method, params));

#pragma region goto_lable
#if LEANCLR_USE_COMPUTED_GOTO_DISPATCHER
    static void* const in_labels0[] = {
        &&LABEL0_InitLocals1Short,
    };
#endif
#pragma endregion

    const uint8_t* ip = frame->ip;
    const RtStackObject* ret = frame->eval_stack_base;

method_start:
{
    RtStackObject* const eval_stack_base = frame->eval_stack_base;
    const RtInterpMethodInfo* const imi = frame->method->interp_data;
    // loop_start:
    while (true)
    {
        LEANCLR_SWITCH0()
        {
            LEANCLR_CASE_BEGIN0(InitLocals1Short)
            {
                RtStackObject* locals = eval_stack_base + ir->offset;
                locals->value = 0;
            }
            LEANCLR_CASE_END0()
            LEANCLR_CASE_BEGIN0(InitLocals2Short)
            {
                RtStackObject* locals = eval_stack_base + ir->offset;
                locals[0].value = 0;
                locals[1].value = 0;
            }
            LEANCLR_CASE_END0()
            LEANCLR_CASE_BEGIN0(InitLocals3Short)
            {
                RtStackObject* locals = eval_stack_base + ir->offset;
                locals[0].value = 0;
                locals[1].value = 0;
                locals[2].value = 0;
            }
            LEANCLR_CASE_END0()
            LEANCLR_CASE_BEGIN0(InitLocals4Short)
            {
                RtStackObject* locals = eval_stack_base + ir->offset;
                locals[0].value = 0;
                locals[1].value = 0;
                locals[2].value = 0;
                locals[3].value = 0;
            }
            LEANCLR_CASE_END0()
            LEANCLR_CASE_BEGIN0(InitLocalsShort)
            {
                std::memset(eval_stack_base + ir->offset, 0, ir->size * sizeof(RtStackObject));
            }
            LEANCLR_CASE_END0()
            LEANCLR_CASE_BEGIN0(LdLocI1Short)
            {
                RtStackObject* dst = eval_stack_base + ir->dst;
                RtStackObject* src = eval_stack_base + ir->src;
                dst->i32 = src->i8;
            }
            LEANCLR_CASE_END0()
            LEANCLR_CASE_BEGIN0(LdLocU1Short)
            {
                RtStackObject* dst = eval_stack_base + ir->dst;
                RtStackObject* src = eval_stack_base + ir->src;
                dst->i32 = src->u8;
            }
            LEANCLR_CASE_END0()
            LEANCLR_CASE_BEGIN0(LdLocI2Short)
            {
                RtStackObject* dst = eval_stack_base + ir->dst;
                RtStackObject* src = eval_stack_base + ir->src;
                dst->i32 = src->i16;
            }
            LEANCLR_CASE_END0()
            LEANCLR_CASE_BEGIN0(LdLocU2Short)
            {
                RtStackObject* dst = eval_stack_base + ir->dst;
                RtStackObject* src = eval_stack_base + ir->src;
                dst->i32 = src->u16;
            }
            LEANCLR_CASE_END0()
            LEANCLR_CASE_BEGIN0(LdLocI4Short)
            {
                RtStackObject* dst = eval_stack_base + ir->dst;
                RtStackObject* src = eval_stack_base + ir->src;
                dst->i32 = src->i32;
            }
            LEANCLR_CASE_END0()
            LEANCLR_CASE_BEGIN0(LdLocI8Short)
            {
                RtStackObject* dst = eval_stack_base + ir->dst;
                RtStackObject* src = eval_stack_base + ir->src;
                dst->i64 = src->i64;
            }
            LEANCLR_CASE_END0()
            LEANCLR_CASE_BEGIN0(LdStrShort)
            {
                RtStackObject* dst = eval_stack_base + ir->dst;
                vm::RtString* str = get_resolved_data<vm::RtString>(imi, ir->str_idx);
                dst->str = str;
            }
            LEANCLR_CASE_END0()
            LEANCLR_CASE_BEGIN0(LdNullShort)
            {
                RtStackObject* dst = eval_stack_base + ir->dst;
                dst->ptr = nullptr;
            }
            LEANCLR_CASE_END0()
            LEANCLR_CASE_BEGIN0(LdcI4I2Short)
            {
                RtStackObject* dst = eval_stack_base + ir->dst;
                dst->i32 = ir->value;
            }
            LEANCLR_CASE_END0()
            LEANCLR_CASE_BEGIN0(LdcI8I2Short)
            {
                RtStackObject* dst = eval_stack_base + ir->dst;
                dst->i64 = static_cast<int64_t>(ir->value);
            }
            LEANCLR_CASE_END0()
            LEANCLR_CASE_BEGIN0(LdcI8I4Short)
            {
                RtStackObject* dst = eval_stack_base + ir->dst;
                dst->i64 = static_cast<int64_t>(ir->value);
            }
            LEANCLR_CASE_END0()
            LEANCLR_CASE_BEGIN0(LdcI8I8Short)
            {
                RtStackObject* dst = eval_stack_base + ir->dst;
#if LEANCLR_SUPPORT_UNALIGNED_ACCESS
                dst->i64 = *(int64_t*)&ir->value_low;
#else
                dst->i64 = (static_cast<int64_t>(ir->value_low)) | ((static_cast<int64_t>(ir->value_high)) << 32);
#endif
            }
            LEANCLR_CASE_END0()
            LEANCLR_CASE_BEGIN_LITE0(RetNop)
            {
                LEAVE_FRAME();
            }
            LEANCLR_CASE_END_LITE0()
            LEANCLR_CASE_BEGIN_LITE0(Prefix1)
            {
                const uint8_t* ip1 = ip + 1;
                LEANCLR_SWITCH1()
                {
                    LEANCLR_CASE_BEGIN1(InitLocals)
                    {
                        std::memset(eval_stack_base + ir->offset, 0, ir->size * sizeof(RtStackObject));
                    }
                    LEANCLR_CASE_END1()
                    LEANCLR_CASE_BEGIN1(LdLocI1)
                    {
                        RtStackObject* dst = eval_stack_base + ir->dst;
                        RtStackObject* src = eval_stack_base + ir->src;
                        dst->i32 = src->i8;
                    }
                    LEANCLR_CASE_END1()
                    LEANCLR_CASE_BEGIN1(LdLocU1)
                    {
                        RtStackObject* dst = eval_stack_base + ir->dst;
                        RtStackObject* src = eval_stack_base + ir->src;
                        dst->i32 = src->u8;
                    }
                    LEANCLR_CASE_END1()
                    LEANCLR_CASE_BEGIN1(LdLocI2)
                    {
                        RtStackObject* dst = eval_stack_base + ir->dst;
                        RtStackObject* src = eval_stack_base + ir->src;
                        dst->i32 = src->i16;
                    }
                    LEANCLR_CASE_END1()
                    LEANCLR_CASE_BEGIN1(LdLocU2)
                    {
                        RtStackObject* dst = eval_stack_base + ir->dst;
                        RtStackObject* src = eval_stack_base + ir->src;
                        dst->i32 = src->u16;
                    }
                    LEANCLR_CASE_END1()
                    LEANCLR_CASE_BEGIN1(LdLocI4)
                    {
                        RtStackObject* dst = eval_stack_base + ir->dst;
                        RtStackObject* src = eval_stack_base + ir->src;
                        dst->i32 = src->i32;
                    }
                    LEANCLR_CASE_END1()
                    LEANCLR_CASE_BEGIN1(LdLocI8)
                    {
                        RtStackObject* dst = eval_stack_base + ir->dst;
                        RtStackObject* src = eval_stack_base + ir->src;
                        dst->i64 = src->i64;
                    }
                    LEANCLR_CASE_END1()
                    LEANCLR_CASE_BEGIN1(LdLocAny)
                    {
                        RtStackObject* dst = eval_stack_base + ir->dst;
                        RtStackObject* src = eval_stack_base + ir->src;
                        std::memcpy(dst, src, ir->size * sizeof(RtStackObject));
                    }
                    LEANCLR_CASE_END1()
                    LEANCLR_CASE_BEGIN1(LdLoca)
                    {
                        RtStackObject* dst = eval_stack_base + ir->dst;
                        RtStackObject* src = eval_stack_base + ir->src;
                        dst->ptr = src;
                    }
                    LEANCLR_CASE_END1()
                    LEANCLR_CASE_BEGIN1(StLocI1)
                    {
                        RtStackObject* dst = eval_stack_base + ir->dst;
                        RtStackObject* src = eval_stack_base + ir->src;
                        dst->i8 = src->i8;
                    }
                    LEANCLR_CASE_END1()
                    LEANCLR_CASE_BEGIN1(StLocI2)
                    {
                        RtStackObject* dst = eval_stack_base + ir->dst;
                        RtStackObject* src = eval_stack_base + ir->src;
                        dst->i16 = src->i16;
                    }
                    LEANCLR_CASE_END1()
                    LEANCLR_CASE_BEGIN1(StLocI4)
                    {
                        RtStackObject* dst = eval_stack_base + ir->dst;
                        RtStackObject* src = eval_stack_base + ir->src;
                        dst->i32 = src->i32;
                    }
                    LEANCLR_CASE_END1()
                    LEANCLR_CASE_BEGIN1(StLocI8)
                    {
                        RtStackObject* dst = eval_stack_base + ir->dst;
                        RtStackObject* src = eval_stack_base + ir->src;
                        dst->i64 = src->i64;
                    }
                    LEANCLR_CASE_END1()
                    LEANCLR_CASE_BEGIN1(StLocAny)
                    {
                        RtStackObject* dst = eval_stack_base + ir->dst;
                        RtStackObject* src = eval_stack_base + ir->src;
                        std::memcpy(dst, src, ir->size * sizeof(RtStackObject));
                    }
                    LEANCLR_CASE_END1()
                    LEANCLR_CASE_BEGIN1(LdNull)
                    {
                        RtStackObject* dst = eval_stack_base + ir->dst;
                        dst->ptr = nullptr;
                    }
                    LEANCLR_CASE_END1()
                    LEANCLR_CASE_BEGIN1(LdcI4I2)
                    {
                        RtStackObject* dst = eval_stack_base + ir->dst;
                        dst->i32 = static_cast<int32_t>(ir->value);
                    }
                    LEANCLR_CASE_END1()
                    LEANCLR_CASE_BEGIN1(LdcI4I4)
                    {
                        RtStackObject* dst = eval_stack_base + ir->dst;
                        dst->i32 = ir->value;
                    }
                    LEANCLR_CASE_END1()
                    LEANCLR_CASE_BEGIN1(LdcI8I2)
                    {
                        RtStackObject* dst = eval_stack_base + ir->dst;
                        dst->i64 = static_cast<int64_t>(ir->value);
                    }
                    LEANCLR_CASE_END1()
                    LEANCLR_CASE_BEGIN1(LdcI8I4)
                    {
                        RtStackObject* dst = eval_stack_base + ir->dst;
                        dst->i64 = static_cast<int64_t>(ir->value);
                    }
                    LEANCLR_CASE_END1()
                    LEANCLR_CASE_BEGIN1(LdcI8I8)
                    {
                        RtStackObject* dst = eval_stack_base + ir->dst;
#if LEANCLR_SUPPORT_UNALIGNED_ACCESS
                        dst->i64 = *(int64_t*)&ir->value_low;
#else
                        dst->i64 = (static_cast<int64_t>(ir->value_low)) | ((static_cast<int64_t>(ir->value_high)) << 32);
#endif
                    }
                    LEANCLR_CASE_END1()
                    LEANCLR_CASE_BEGIN1(LdStr)
                    {
                        RtStackObject* dst = eval_stack_base + ir->dst;
                        vm::RtString* str = get_resolved_data<vm::RtString>(imi, ir->str_idx);
                        dst->str = str;
                    }
                    LEANCLR_CASE_END1()
                    LEANCLR_CASE_BEGIN_LITE1(Br)
                    {
                        const auto* ir = (ll::Br*)ip;
                        ip = reinterpret_cast<const uint8_t*>(ip + ir->target_offset);
                    }
                    LEANCLR_CASE_END_LITE1()
                    LEANCLR_CASE_BEGIN_LITE1(BrTrueI4)
                    {
                        const auto* ir = (ll::BrTrueI4*)ip;
                        RtStackObject* cond = eval_stack_base + ir->condition;
                        if (cond->i32 != 0)
                        {
                            ip = reinterpret_cast<const uint8_t*>(ip + ir->target_offset);
                        }
                        else
                        {
                            ip = reinterpret_cast<const uint8_t*>(ir + 1);
                        }
                    }
                    LEANCLR_CASE_END_LITE1()
                    LEANCLR_CASE_BEGIN_LITE1(BrTrueI8)
                    {
                        const auto* ir = (ll::BrTrueI8*)ip;
                        RtStackObject* cond = eval_stack_base + ir->condition;
                        if (cond->i64 != 0)
                        {
                            ip = reinterpret_cast<const uint8_t*>(ip + ir->target_offset);
                        }
                        else
                        {
                            ip = reinterpret_cast<const uint8_t*>(ir + 1);
                        }
                    }
                    LEANCLR_CASE_END_LITE1()
                    LEANCLR_CASE_BEGIN_LITE1(BrFalseI4)
                    {
                        const auto* ir = (ll::BrFalseI4*)ip;
                        RtStackObject* cond = eval_stack_base + ir->condition;
                        if (cond->i32 == 0)
                        {
                            ip = reinterpret_cast<const uint8_t*>(ip + ir->target_offset);
                        }
                        else
                        {
                            ip = reinterpret_cast<const uint8_t*>(ir + 1);
                        }
                    }
                    LEANCLR_CASE_END_LITE1()
                    LEANCLR_CASE_BEGIN_LITE1(BrFalseI8)
                    {
                        const auto* ir = (ll::BrFalseI8*)ip;
                        RtStackObject* cond = eval_stack_base + ir->condition;
                        if (cond->i64 == 0)
                        {
                            ip = reinterpret_cast<const uint8_t*>(ip + ir->target_offset);
                        }
                        else
                        {
                            ip = reinterpret_cast<const uint8_t*>(ir + 1);
                        }
                    }
                    LEANCLR_CASE_END_LITE1()
                    LEANCLR_CASE_BEGIN_LITE1(BeqI4)
                    {
                        const auto* ir = (ll::BeqI4*)ip;
                        RtStackObject* op1 = eval_stack_base + ir->arg1;
                        RtStackObject* op2 = eval_stack_base + ir->arg2;
                        if (op1->i32 == op2->i32)
                        {
                            ip = reinterpret_cast<const uint8_t*>(ip + ir->target_offset);
                        }
                        else
                        {
                            ip = reinterpret_cast<const uint8_t*>(ir + 1);
                        }
                    }
                    LEANCLR_CASE_END_LITE1()
                    LEANCLR_CASE_BEGIN_LITE1(BeqI8)
                    {
                        const auto* ir = (ll::BeqI8*)ip;
                        RtStackObject* op1 = eval_stack_base + ir->arg1;
                        RtStackObject* op2 = eval_stack_base + ir->arg2;
                        if (op1->i64 == op2->i64)
                        {
                            ip = reinterpret_cast<const uint8_t*>(ip + ir->target_offset);
                        }
                        else
                        {
                            ip = reinterpret_cast<const uint8_t*>(ir + 1);
                        }
                    }
                    LEANCLR_CASE_END_LITE1()
                    LEANCLR_CASE_BEGIN_LITE1(BeqR4)
                    {
                        const auto* ir = (ll::BeqR4*)ip;
                        RtStackObject* op1 = eval_stack_base + ir->arg1;
                        RtStackObject* op2 = eval_stack_base + ir->arg2;
                        float left = op1->f32;
                        float right = op2->f32;
                        if (left == right && !std::isunordered(left, right))
                        {
                            ip = reinterpret_cast<const uint8_t*>(ip + ir->target_offset);
                        }
                        else
                        {
                            ip = reinterpret_cast<const uint8_t*>(ir + 1);
                        }
                    }
                    LEANCLR_CASE_END_LITE1()
                    LEANCLR_CASE_BEGIN_LITE1(BeqR8)
                    {
                        const auto* ir = (ll::BeqR8*)ip;
                        RtStackObject* op1 = eval_stack_base + ir->arg1;
                        RtStackObject* op2 = eval_stack_base + ir->arg2;
                        double left = op1->f64;
                        double right = op2->f64;
                        if (left == right && !std::isunordered(left, right))
                        {
                            ip = reinterpret_cast<const uint8_t*>(ip + ir->target_offset);
                        }
                        else
                        {
                            ip = reinterpret_cast<const uint8_t*>(ir + 1);
                        }
                    }
                    LEANCLR_CASE_END_LITE1()
                    LEANCLR_CASE_BEGIN_LITE1(BgeI4)
                    {
                        const auto* ir = (ll::BgeI4*)ip;
                        RtStackObject* op1 = eval_stack_base + ir->arg1;
                        RtStackObject* op2 = eval_stack_base + ir->arg2;
                        if (op1->i32 >= op2->i32)
                        {
                            ip = reinterpret_cast<const uint8_t*>(ip + ir->target_offset);
                        }
                        else
                        {
                            ip = reinterpret_cast<const uint8_t*>(ir + 1);
                        }
                    }
                    LEANCLR_CASE_END_LITE1()
                    LEANCLR_CASE_BEGIN_LITE1(BgeI8)
                    {
                        const auto* ir = (ll::BgeI8*)ip;
                        RtStackObject* op1 = eval_stack_base + ir->arg1;
                        RtStackObject* op2 = eval_stack_base + ir->arg2;
                        if (op1->i64 >= op2->i64)
                        {
                            ip = reinterpret_cast<const uint8_t*>(ip + ir->target_offset);
                        }
                        else
                        {
                            ip = reinterpret_cast<const uint8_t*>(ir + 1);
                        }
                    }
                    LEANCLR_CASE_END_LITE1()
                    LEANCLR_CASE_BEGIN_LITE1(BgeR4)
                    {
                        const auto* ir = (ll::BgeR4*)ip;
                        RtStackObject* op1 = eval_stack_base + ir->arg1;
                        RtStackObject* op2 = eval_stack_base + ir->arg2;
                        float left = op1->f32;
                        float right = op2->f32;
                        if (left >= right && !std::isunordered(left, right))
                        {
                            ip = reinterpret_cast<const uint8_t*>(ip + ir->target_offset);
                        }
                        else
                        {
                            ip = reinterpret_cast<const uint8_t*>(ir + 1);
                        }
                    }
                    LEANCLR_CASE_END_LITE1()
                    LEANCLR_CASE_BEGIN_LITE1(BgeR8)
                    {
                        const auto* ir = (ll::BgeR8*)ip;
                        RtStackObject* op1 = eval_stack_base + ir->arg1;
                        RtStackObject* op2 = eval_stack_base + ir->arg2;
                        double left = op1->f64;
                        double right = op2->f64;
                        if (left >= right && !std::isunordered(left, right))
                        {
                            ip = reinterpret_cast<const uint8_t*>(ip + ir->target_offset);
                        }
                        else
                        {
                            ip = reinterpret_cast<const uint8_t*>(ir + 1);
                        }
                    }
                    LEANCLR_CASE_END_LITE1()
                    LEANCLR_CASE_BEGIN_LITE1(BgtI4)
                    {
                        const auto* ir = (ll::BgtI4*)ip;
                        RtStackObject* op1 = eval_stack_base + ir->arg1;
                        RtStackObject* op2 = eval_stack_base + ir->arg2;
                        if (op1->i32 > op2->i32)
                        {
                            ip = reinterpret_cast<const uint8_t*>(ip + ir->target_offset);
                        }
                        else
                        {
                            ip = reinterpret_cast<const uint8_t*>(ir + 1);
                        }
                    }
                    LEANCLR_CASE_END_LITE1()
                    LEANCLR_CASE_BEGIN_LITE1(BgtI8)
                    {
                        const auto* ir = (ll::BgtI8*)ip;
                        RtStackObject* op1 = eval_stack_base + ir->arg1;
                        RtStackObject* op2 = eval_stack_base + ir->arg2;
                        if (op1->i64 > op2->i64)
                        {
                            ip = reinterpret_cast<const uint8_t*>(ip + ir->target_offset);
                        }
                        else
                        {
                            ip = reinterpret_cast<const uint8_t*>(ir + 1);
                        }
                    }
                    LEANCLR_CASE_END_LITE1()
                    LEANCLR_CASE_BEGIN_LITE1(BgtR4)
                    {
                        const auto* ir = (ll::BgtR4*)ip;
                        RtStackObject* op1 = eval_stack_base + ir->arg1;
                        RtStackObject* op2 = eval_stack_base + ir->arg2;
                        float left = op1->f32;
                        float right = op2->f32;
                        if (left > right && !std::isunordered(left, right))
                        {
                            ip = reinterpret_cast<const uint8_t*>(ip + ir->target_offset);
                        }
                        else
                        {
                            ip = reinterpret_cast<const uint8_t*>(ir + 1);
                        }
                    }
                    LEANCLR_CASE_END_LITE1()
                    LEANCLR_CASE_BEGIN_LITE1(BgtR8)
                    {
                        const auto* ir = (ll::BgtR8*)ip;
                        RtStackObject* op1 = eval_stack_base + ir->arg1;
                        RtStackObject* op2 = eval_stack_base + ir->arg2;
                        double left = op1->f64;
                        double right = op2->f64;
                        if (left > right && !std::isunordered(left, right))
                        {
                            ip = reinterpret_cast<const uint8_t*>(ip + ir->target_offset);
                        }
                        else
                        {
                            ip = reinterpret_cast<const uint8_t*>(ir + 1);
                        }
                    }
                    LEANCLR_CASE_END_LITE1()
                    LEANCLR_CASE_BEGIN_LITE1(BleI4)
                    {
                        const auto* ir = (ll::BleI4*)ip;
                        RtStackObject* op1 = eval_stack_base + ir->arg1;
                        RtStackObject* op2 = eval_stack_base + ir->arg2;
                        if (op1->i32 <= op2->i32)
                        {
                            ip = reinterpret_cast<const uint8_t*>(ip + ir->target_offset);
                        }
                        else
                        {
                            ip = reinterpret_cast<const uint8_t*>(ir + 1);
                        }
                    }
                    LEANCLR_CASE_END_LITE1()
                    LEANCLR_CASE_BEGIN_LITE1(BleI8)
                    {
                        const auto* ir = (ll::BleI8*)ip;
                        RtStackObject* op1 = eval_stack_base + ir->arg1;
                        RtStackObject* op2 = eval_stack_base + ir->arg2;
                        if (op1->i64 <= op2->i64)
                        {
                            ip = reinterpret_cast<const uint8_t*>(ip + ir->target_offset);
                        }
                        else
                        {
                            ip = reinterpret_cast<const uint8_t*>(ir + 1);
                        }
                    }
                    LEANCLR_CASE_END_LITE1()
                    LEANCLR_CASE_BEGIN_LITE1(BleR4)
                    {
                        const auto* ir = (ll::BleR4*)ip;
                        RtStackObject* op1 = eval_stack_base + ir->arg1;
                        RtStackObject* op2 = eval_stack_base + ir->arg2;
                        float left = op1->f32;
                        float right = op2->f32;
                        if (left <= right && !std::isunordered(left, right))
                        {
                            ip = reinterpret_cast<const uint8_t*>(ip + ir->target_offset);
                        }
                        else
                        {
                            ip = reinterpret_cast<const uint8_t*>(ir + 1);
                        }
                    }
                    LEANCLR_CASE_END_LITE1()
                    LEANCLR_CASE_BEGIN_LITE1(BleR8)
                    {
                        const auto* ir = (ll::BleR8*)ip;
                        RtStackObject* op1 = eval_stack_base + ir->arg1;
                        RtStackObject* op2 = eval_stack_base + ir->arg2;
                        double left = op1->f64;
                        double right = op2->f64;
                        if (left <= right && !std::isunordered(left, right))
                        {
                            ip = reinterpret_cast<const uint8_t*>(ip + ir->target_offset);
                        }
                        else
                        {
                            ip = reinterpret_cast<const uint8_t*>(ir + 1);
                        }
                    }
                    LEANCLR_CASE_END_LITE1()
                    LEANCLR_CASE_BEGIN_LITE1(BltI4)
                    {
                        const auto* ir = (ll::BltI4*)ip;
                        RtStackObject* op1 = eval_stack_base + ir->arg1;
                        RtStackObject* op2 = eval_stack_base + ir->arg2;
                        if (op1->i32 < op2->i32)
                        {
                            ip = reinterpret_cast<const uint8_t*>(ip + ir->target_offset);
                        }
                        else
                        {
                            ip = reinterpret_cast<const uint8_t*>(ir + 1);
                        }
                    }
                    LEANCLR_CASE_END_LITE1()
                    LEANCLR_CASE_BEGIN_LITE1(BltI8)
                    {
                        const auto* ir = (ll::BltI8*)ip;
                        RtStackObject* op1 = eval_stack_base + ir->arg1;
                        RtStackObject* op2 = eval_stack_base + ir->arg2;
                        if (op1->i64 < op2->i64)
                        {
                            ip = reinterpret_cast<const uint8_t*>(ip + ir->target_offset);
                        }
                        else
                        {
                            ip = reinterpret_cast<const uint8_t*>(ir + 1);
                        }
                    }
                    LEANCLR_CASE_END_LITE1()
                    LEANCLR_CASE_BEGIN_LITE1(BltR4)
                    {
                        const auto* ir = (ll::BltR4*)ip;
                        RtStackObject* op1 = eval_stack_base + ir->arg1;
                        RtStackObject* op2 = eval_stack_base + ir->arg2;
                        float left = op1->f32;
                        float right = op2->f32;
                        if (left < right && !std::isunordered(left, right))
                        {
                            ip = reinterpret_cast<const uint8_t*>(ip + ir->target_offset);
                        }
                        else
                        {
                            ip = reinterpret_cast<const uint8_t*>(ir + 1);
                        }
                    }
                    LEANCLR_CASE_END_LITE1()
                    LEANCLR_CASE_BEGIN_LITE1(BltR8)
                    {
                        const auto* ir = (ll::BltR8*)ip;
                        RtStackObject* op1 = eval_stack_base + ir->arg1;
                        RtStackObject* op2 = eval_stack_base + ir->arg2;
                        double left = op1->f64;
                        double right = op2->f64;
                        if (left < right && !std::isunordered(left, right))
                        {
                            ip = reinterpret_cast<const uint8_t*>(ip + ir->target_offset);
                        }
                        else
                        {
                            ip = reinterpret_cast<const uint8_t*>(ir + 1);
                        }
                    }
                    LEANCLR_CASE_END_LITE1()
                    LEANCLR_CASE_BEGIN_LITE1(BneUnI4)
                    {
                        const auto* ir = (ll::BneUnI4*)ip;
                        RtStackObject* op1 = eval_stack_base + ir->arg1;
                        RtStackObject* op2 = eval_stack_base + ir->arg2;
                        if (op1->u32 != op2->u32)
                        {
                            ip = reinterpret_cast<const uint8_t*>(ip + ir->target_offset);
                        }
                        else
                        {
                            ip = reinterpret_cast<const uint8_t*>(ir + 1);
                        }
                    }
                    LEANCLR_CASE_END_LITE1()
                    LEANCLR_CASE_BEGIN_LITE1(BneUnI8)
                    {
                        const auto* ir = (ll::BneUnI8*)ip;
                        RtStackObject* op1 = eval_stack_base + ir->arg1;
                        RtStackObject* op2 = eval_stack_base + ir->arg2;
                        if (op1->u64 != op2->u64)
                        {
                            ip = reinterpret_cast<const uint8_t*>(ip + ir->target_offset);
                        }
                        else
                        {
                            ip = reinterpret_cast<const uint8_t*>(ir + 1);
                        }
                    }
                    LEANCLR_CASE_END_LITE1()
                    LEANCLR_CASE_BEGIN_LITE1(BneUnR4)
                    {
                        const auto* ir = (ll::BneUnR4*)ip;
                        RtStackObject* op1 = eval_stack_base + ir->arg1;
                        RtStackObject* op2 = eval_stack_base + ir->arg2;
                        float left = op1->f32;
                        float right = op2->f32;
                        if (left != right || std::isunordered(left, right))
                        {
                            ip = reinterpret_cast<const uint8_t*>(ip + ir->target_offset);
                        }
                        else
                        {
                            ip = reinterpret_cast<const uint8_t*>(ir + 1);
                        }
                    }
                    LEANCLR_CASE_END_LITE1()
                    LEANCLR_CASE_BEGIN_LITE1(BneUnR8)
                    {
                        const auto* ir = (ll::BneUnR8*)ip;
                        RtStackObject* op1 = eval_stack_base + ir->arg1;
                        RtStackObject* op2 = eval_stack_base + ir->arg2;
                        double left = op1->f64;
                        double right = op2->f64;
                        if (left != right || std::isunordered(left, right))
                        {
                            ip = reinterpret_cast<const uint8_t*>(ip + ir->target_offset);
                        }
                        else
                        {
                            ip = reinterpret_cast<const uint8_t*>(ir + 1);
                        }
                    }
                    LEANCLR_CASE_END_LITE1()
                    LEANCLR_CASE_BEGIN_LITE1(BgeUnI4)
                    {
                        const auto* ir = (ll::BgeUnI4*)ip;
                        RtStackObject* op1 = eval_stack_base + ir->arg1;
                        RtStackObject* op2 = eval_stack_base + ir->arg2;
                        if (op1->u32 >= op2->u32)
                        {
                            ip = reinterpret_cast<const uint8_t*>(ip + ir->target_offset);
                        }
                        else
                        {
                            ip = reinterpret_cast<const uint8_t*>(ir + 1);
                        }
                    }
                    LEANCLR_CASE_END_LITE1()
                    LEANCLR_CASE_BEGIN_LITE1(BgeUnI8)
                    {
                        const auto* ir = (ll::BgeUnI8*)ip;
                        RtStackObject* op1 = eval_stack_base + ir->arg1;
                        RtStackObject* op2 = eval_stack_base + ir->arg2;
                        if (op1->u64 >= op2->u64)
                        {
                            ip = reinterpret_cast<const uint8_t*>(ip + ir->target_offset);
                        }
                        else
                        {
                            ip = reinterpret_cast<const uint8_t*>(ir + 1);
                        }
                    }
                    LEANCLR_CASE_END_LITE1()
                    LEANCLR_CASE_BEGIN_LITE1(BgeUnR4)
                    {
                        const auto* ir = (ll::BgeUnR4*)ip;
                        RtStackObject* op1 = eval_stack_base + ir->arg1;
                        RtStackObject* op2 = eval_stack_base + ir->arg2;
                        float left = op1->f32;
                        float right = op2->f32;
                        if (left >= right || std::isunordered(left, right))
                        {
                            ip = reinterpret_cast<const uint8_t*>(ip + ir->target_offset);
                        }
                        else
                        {
                            ip = reinterpret_cast<const uint8_t*>(ir + 1);
                        }
                    }
                    LEANCLR_CASE_END_LITE1()
                    LEANCLR_CASE_BEGIN_LITE1(BgeUnR8)
                    {
                        const auto* ir = (ll::BgeUnR8*)ip;
                        RtStackObject* op1 = eval_stack_base + ir->arg1;
                        RtStackObject* op2 = eval_stack_base + ir->arg2;
                        double left = op1->f64;
                        double right = op2->f64;
                        if (left >= right || std::isunordered(left, right))
                        {
                            ip = reinterpret_cast<const uint8_t*>(ip + ir->target_offset);
                        }
                        else
                        {
                            ip = reinterpret_cast<const uint8_t*>(ir + 1);
                        }
                    }
                    LEANCLR_CASE_END_LITE1()
                    LEANCLR_CASE_BEGIN_LITE1(BgtUnI4)
                    {
                        const auto* ir = (ll::BgtUnI4*)ip;
                        RtStackObject* op1 = eval_stack_base + ir->arg1;
                        RtStackObject* op2 = eval_stack_base + ir->arg2;
                        if (op1->u32 > op2->u32)
                        {
                            ip = reinterpret_cast<const uint8_t*>(ip + ir->target_offset);
                        }
                        else
                        {
                            ip = reinterpret_cast<const uint8_t*>(ir + 1);
                        }
                    }
                    LEANCLR_CASE_END_LITE1()
                    LEANCLR_CASE_BEGIN_LITE1(BgtUnI8)
                    {
                        const auto* ir = (ll::BgtUnI8*)ip;
                        RtStackObject* op1 = eval_stack_base + ir->arg1;
                        RtStackObject* op2 = eval_stack_base + ir->arg2;
                        if (op1->u64 > op2->u64)
                        {
                            ip = reinterpret_cast<const uint8_t*>(ip + ir->target_offset);
                        }
                        else
                        {
                            ip = reinterpret_cast<const uint8_t*>(ir + 1);
                        }
                    }
                    LEANCLR_CASE_END_LITE1()
                    LEANCLR_CASE_BEGIN_LITE1(BgtUnR4)
                    {
                        const auto* ir = (ll::BgtUnR4*)ip;
                        RtStackObject* op1 = eval_stack_base + ir->arg1;
                        RtStackObject* op2 = eval_stack_base + ir->arg2;
                        float left = op1->f32;
                        float right = op2->f32;
                        if (left > right || std::isunordered(left, right))
                        {
                            ip = reinterpret_cast<const uint8_t*>(ip + ir->target_offset);
                        }
                        else
                        {
                            ip = reinterpret_cast<const uint8_t*>(ir + 1);
                        }
                    }
                    LEANCLR_CASE_END_LITE1()
                    LEANCLR_CASE_BEGIN_LITE1(BgtUnR8)
                    {
                        const auto* ir = (ll::BgtUnR8*)ip;
                        RtStackObject* op1 = eval_stack_base + ir->arg1;
                        RtStackObject* op2 = eval_stack_base + ir->arg2;
                        double left = op1->f64;
                        double right = op2->f64;
                        if (left > right || std::isunordered(left, right))
                        {
                            ip = reinterpret_cast<const uint8_t*>(ip + ir->target_offset);
                        }
                        else
                        {
                            ip = reinterpret_cast<const uint8_t*>(ir + 1);
                        }
                    }
                    LEANCLR_CASE_END_LITE1()
                    LEANCLR_CASE_BEGIN_LITE1(BleUnI4)
                    {
                        const auto* ir = (ll::BleUnI4*)ip;
                        RtStackObject* op1 = eval_stack_base + ir->arg1;
                        RtStackObject* op2 = eval_stack_base + ir->arg2;
                        if (op1->u32 <= op2->u32)
                        {
                            ip = reinterpret_cast<const uint8_t*>(ip + ir->target_offset);
                        }
                        else
                        {
                            ip = reinterpret_cast<const uint8_t*>(ir + 1);
                        }
                    }
                    LEANCLR_CASE_END_LITE1()
                    LEANCLR_CASE_BEGIN_LITE1(BleUnI8)
                    {
                        const auto* ir = (ll::BleUnI8*)ip;
                        RtStackObject* op1 = eval_stack_base + ir->arg1;
                        RtStackObject* op2 = eval_stack_base + ir->arg2;
                        if (op1->u64 <= op2->u64)
                        {
                            ip = reinterpret_cast<const uint8_t*>(ip + ir->target_offset);
                        }
                        else
                        {
                            ip = reinterpret_cast<const uint8_t*>(ir + 1);
                        }
                    }
                    LEANCLR_CASE_END_LITE1()
                    LEANCLR_CASE_BEGIN_LITE1(BleUnR4)
                    {
                        const auto* ir = (ll::BleUnR4*)ip;
                        RtStackObject* op1 = eval_stack_base + ir->arg1;
                        RtStackObject* op2 = eval_stack_base + ir->arg2;
                        float left = op1->f32;
                        float right = op2->f32;
                        if (left <= right || std::isunordered(left, right))
                        {
                            ip = reinterpret_cast<const uint8_t*>(ip + ir->target_offset);
                        }
                        else
                        {
                            ip = reinterpret_cast<const uint8_t*>(ir + 1);
                        }
                    }
                    LEANCLR_CASE_END_LITE1()
                    LEANCLR_CASE_BEGIN_LITE1(BleUnR8)
                    {
                        const auto* ir = (ll::BleUnR8*)ip;
                        RtStackObject* op1 = eval_stack_base + ir->arg1;
                        RtStackObject* op2 = eval_stack_base + ir->arg2;
                        double left = op1->f64;
                        double right = op2->f64;
                        if (left <= right || std::isunordered(left, right))
                        {
                            ip = reinterpret_cast<const uint8_t*>(ip + ir->target_offset);
                        }
                        else
                        {
                            ip = reinterpret_cast<const uint8_t*>(ir + 1);
                        }
                    }
                    LEANCLR_CASE_END_LITE1()
                    LEANCLR_CASE_BEGIN_LITE1(BltUnI4)
                    {
                        const auto* ir = (ll::BltUnI4*)ip;
                        RtStackObject* op1 = eval_stack_base + ir->arg1;
                        RtStackObject* op2 = eval_stack_base + ir->arg2;
                        if (op1->u32 < op2->u32)
                        {
                            ip = reinterpret_cast<const uint8_t*>(ip + ir->target_offset);
                        }
                        else
                        {
                            ip = reinterpret_cast<const uint8_t*>(ir + 1);
                        }
                    }
                    LEANCLR_CASE_END_LITE1()
                    LEANCLR_CASE_BEGIN_LITE1(BltUnI8)
                    {
                        const auto* ir = (ll::BltUnI8*)ip;
                        RtStackObject* op1 = eval_stack_base + ir->arg1;
                        RtStackObject* op2 = eval_stack_base + ir->arg2;
                        if (op1->u64 < op2->u64)
                        {
                            ip = reinterpret_cast<const uint8_t*>(ip + ir->target_offset);
                        }
                        else
                        {
                            ip = reinterpret_cast<const uint8_t*>(ir + 1);
                        }
                    }
                    LEANCLR_CASE_END_LITE1()
                    LEANCLR_CASE_BEGIN_LITE1(BltUnR4)
                    {
                        const auto* ir = (ll::BltUnR4*)ip;
                        RtStackObject* op1 = eval_stack_base + ir->arg1;
                        RtStackObject* op2 = eval_stack_base + ir->arg2;
                        float left = op1->f32;
                        float right = op2->f32;
                        if (left < right || std::isunordered(left, right))
                        {
                            ip = reinterpret_cast<const uint8_t*>(ip + ir->target_offset);
                        }
                        else
                        {
                            ip = reinterpret_cast<const uint8_t*>(ir + 1);
                        }
                    }
                    LEANCLR_CASE_END_LITE1()
                    LEANCLR_CASE_BEGIN_LITE1(BltUnR8)
                    {
                        const auto* ir = (ll::BltUnR8*)ip;
                        RtStackObject* op1 = eval_stack_base + ir->arg1;
                        RtStackObject* op2 = eval_stack_base + ir->arg2;
                        double left = op1->f64;
                        double right = op2->f64;
                        if (left < right || std::isunordered(left, right))
                        {
                            ip = reinterpret_cast<const uint8_t*>(ip + ir->target_offset);
                        }
                        else
                        {
                            ip = reinterpret_cast<const uint8_t*>(ir + 1);
                        }
                    }
                    LEANCLR_CASE_END_LITE1()
                    LEANCLR_CASE_BEGIN_LITE1(Switch)
                    {
                        const auto* ir = (ll::Switch*)ip;
                        RtStackObject* index_obj = eval_stack_base + ir->index;
                        uint32_t idx = index_obj->u32;
                        if (idx < ir->num_targets)
                        {
                            const int32_t* target_offsets = reinterpret_cast<const int32_t*>(ir + 1);
                            int32_t target_offset = target_offsets[idx];
                            ip = reinterpret_cast<const uint8_t*>(ip + target_offset);
                        }
                        else
                        {
                            ip = reinterpret_cast<const uint8_t*>(reinterpret_cast<const uint8_t*>(ir + 1) + ir->num_targets * sizeof(int32_t));
                        }
                    }
                    LEANCLR_CASE_END_LITE1()
                    LEANCLR_CASE_BEGIN1(LdIndI1Short)
                    {
                        int8_t value = get_ind_stack_value_at<int8_t>(eval_stack_base, ir->src);
                        set_stack_value_at<int32_t>(eval_stack_base, ir->dst, static_cast<int32_t>(value));
                    }
                    LEANCLR_CASE_END1()
                    LEANCLR_CASE_BEGIN1(LdIndU1Short)
                    {
                        uint8_t value = get_ind_stack_value_at<uint8_t>(eval_stack_base, ir->src);
                        set_stack_value_at<int32_t>(eval_stack_base, ir->dst, static_cast<int32_t>(value));
                    }
                    LEANCLR_CASE_END1()
                    LEANCLR_CASE_BEGIN1(LdIndI2Short)
                    {
                        int16_t value = get_ind_stack_value_at<int16_t>(eval_stack_base, ir->src);
                        set_stack_value_at<int32_t>(eval_stack_base, ir->dst, static_cast<int32_t>(value));
                    }
                    LEANCLR_CASE_END1()
                    LEANCLR_CASE_BEGIN1(LdIndU2Short)
                    {
                        uint16_t value = get_ind_stack_value_at<uint16_t>(eval_stack_base, ir->src);
                        set_stack_value_at<int32_t>(eval_stack_base, ir->dst, static_cast<int32_t>(value));
                    }
                    LEANCLR_CASE_END1()
                    LEANCLR_CASE_BEGIN1(LdIndI4Short)
                    {
                        int32_t value = get_ind_stack_value_at<int32_t>(eval_stack_base, ir->src);
                        set_stack_value_at<int32_t>(eval_stack_base, ir->dst, value);
                    }
                    LEANCLR_CASE_END1()
                    LEANCLR_CASE_BEGIN1(LdIndI8Short)
                    {
                        int64_t value = get_ind_stack_value_at<int64_t>(eval_stack_base, ir->src);
                        set_stack_value_at<int64_t>(eval_stack_base, ir->dst, value);
                    }
                    LEANCLR_CASE_END1()
                    LEANCLR_CASE_BEGIN1(StIndI1Short)
                    {
                        int8_t value = get_stack_value_at<int8_t>(eval_stack_base, ir->src);
                        set_ind_stack_value_at<int8_t>(eval_stack_base, ir->dst, value);
                    }
                    LEANCLR_CASE_END1()
                    LEANCLR_CASE_BEGIN1(StIndI2Short)
                    {
                        int16_t value = get_stack_value_at<int16_t>(eval_stack_base, ir->src);
                        set_ind_stack_value_at<int16_t>(eval_stack_base, ir->dst, value);
                    }
                    LEANCLR_CASE_END1()
                    LEANCLR_CASE_BEGIN1(StIndI4Short)
                    {
                        int32_t value = get_stack_value_at<int32_t>(eval_stack_base, ir->src);
                        set_ind_stack_value_at<int32_t>(eval_stack_base, ir->dst, value);
                    }
                    LEANCLR_CASE_END1()
                    LEANCLR_CASE_BEGIN1(StIndI8Short)
                    {
                        int64_t value = get_stack_value_at<int64_t>(eval_stack_base, ir->src);
                        set_ind_stack_value_at<int64_t>(eval_stack_base, ir->dst, value);
                    }
                    LEANCLR_CASE_END1()
                    LEANCLR_CASE_BEGIN1(StIndI8I4Short)
                    {
                        int32_t value = get_stack_value_at<int32_t>(eval_stack_base, ir->src);
                        set_ind_stack_value_at<int64_t>(eval_stack_base, ir->dst, value);
                    }
                    LEANCLR_CASE_END1()
                    LEANCLR_CASE_BEGIN1(StIndI8U4Short)
                    {
                        uint32_t value = get_stack_value_at<uint32_t>(eval_stack_base, ir->src);
                        set_ind_stack_value_at<int64_t>(eval_stack_base, ir->dst, value);
                    }
                    LEANCLR_CASE_END1()
                    LEANCLR_CASE_BEGIN1(AddI4)
                    {
                        RtStackObject* dst = eval_stack_base + ir->dst;
                        RtStackObject* src1 = eval_stack_base + ir->arg1;
                        RtStackObject* src2 = eval_stack_base + ir->arg2;
                        dst->i32 = src1->i32 + src2->i32;
                    }
                    LEANCLR_CASE_END1()
                    LEANCLR_CASE_BEGIN1(AddI8)
                    {
                        RtStackObject* dst = eval_stack_base + ir->dst;
                        RtStackObject* src1 = eval_stack_base + ir->arg1;
                        RtStackObject* src2 = eval_stack_base + ir->arg2;
                        dst->i64 = src1->i64 + src2->i64;
                    }
                    LEANCLR_CASE_END1()
                    LEANCLR_CASE_BEGIN1(AddR4)
                    {
                        RtStackObject* dst = eval_stack_base + ir->dst;
                        RtStackObject* src1 = eval_stack_base + ir->arg1;
                        RtStackObject* src2 = eval_stack_base + ir->arg2;
                        dst->f32 = src1->f32 + src2->f32;
                    }
                    LEANCLR_CASE_END1()
                    LEANCLR_CASE_BEGIN1(AddR8)
                    {
                        RtStackObject* dst = eval_stack_base + ir->dst;
                        RtStackObject* src1 = eval_stack_base + ir->arg1;
                        RtStackObject* src2 = eval_stack_base + ir->arg2;
                        dst->f64 = src1->f64 + src2->f64;
                    }
                    LEANCLR_CASE_END1()
                    LEANCLR_CASE_BEGIN1(SubI4)
                    {
                        RtStackObject* dst = eval_stack_base + ir->dst;
                        RtStackObject* src1 = eval_stack_base + ir->arg1;
                        RtStackObject* src2 = eval_stack_base + ir->arg2;
                        dst->i32 = src1->i32 - src2->i32;
                    }
                    LEANCLR_CASE_END1()
                    LEANCLR_CASE_BEGIN1(SubI8)
                    {
                        RtStackObject* dst = eval_stack_base + ir->dst;
                        RtStackObject* src1 = eval_stack_base + ir->arg1;
                        RtStackObject* src2 = eval_stack_base + ir->arg2;
                        dst->i64 = src1->i64 - src2->i64;
                    }
                    LEANCLR_CASE_END1()
                    LEANCLR_CASE_BEGIN1(SubR4)
                    {
                        RtStackObject* dst = eval_stack_base + ir->dst;
                        RtStackObject* src1 = eval_stack_base + ir->arg1;
                        RtStackObject* src2 = eval_stack_base + ir->arg2;
                        dst->f32 = src1->f32 - src2->f32;
                    }
                    LEANCLR_CASE_END1()
                    LEANCLR_CASE_BEGIN1(SubR8)
                    {
                        RtStackObject* dst = eval_stack_base + ir->dst;
                        RtStackObject* src1 = eval_stack_base + ir->arg1;
                        RtStackObject* src2 = eval_stack_base + ir->arg2;
                        dst->f64 = src1->f64 - src2->f64;
                    }
                    LEANCLR_CASE_END1()
                    LEANCLR_CASE_BEGIN1(MulI4)
                    {
                        RtStackObject* dst = eval_stack_base + ir->dst;
                        RtStackObject* src1 = eval_stack_base + ir->arg1;
                        RtStackObject* src2 = eval_stack_base + ir->arg2;
                        dst->i32 = src1->i32 * src2->i32;
                    }
                    LEANCLR_CASE_END1()
                    LEANCLR_CASE_BEGIN1(MulI8)
                    {
                        RtStackObject* dst = eval_stack_base + ir->dst;
                        RtStackObject* src1 = eval_stack_base + ir->arg1;
                        RtStackObject* src2 = eval_stack_base + ir->arg2;
                        dst->i64 = src1->i64 * src2->i64;
                    }
                    LEANCLR_CASE_END1()
                    LEANCLR_CASE_BEGIN1(MulR4)
                    {
                        RtStackObject* dst = eval_stack_base + ir->dst;
                        RtStackObject* src1 = eval_stack_base + ir->arg1;
                        RtStackObject* src2 = eval_stack_base + ir->arg2;
                        dst->f32 = src1->f32 * src2->f32;
                    }
                    LEANCLR_CASE_END1()
                    LEANCLR_CASE_BEGIN1(MulR8)
                    {
                        RtStackObject* dst = eval_stack_base + ir->dst;
                        RtStackObject* src1 = eval_stack_base + ir->arg1;
                        RtStackObject* src2 = eval_stack_base + ir->arg2;
                        dst->f64 = src1->f64 * src2->f64;
                    }
                    LEANCLR_CASE_END1()
                    LEANCLR_CASE_BEGIN1(DivI4)
                    {
                        RtStackObject* dst = eval_stack_base + ir->dst;
                        RtStackObject* src1 = eval_stack_base + ir->arg1;
                        RtStackObject* src2 = eval_stack_base + ir->arg2;
                        int32_t dividend = src1->i32;
                        int32_t divisor = src2->i32;
                        if (divisor == 0)
                        {
                            RAISE_RUNTIME_ERROR(RtErr::DivideByZero);
                        }
                        if (divisor == -1 && dividend == INT32_MIN)
                        {
                            RAISE_RUNTIME_ERROR(RtErr::Arithmetic);
                        }
                        dst->i32 = dividend / divisor;
                    }
                    LEANCLR_CASE_END1()
                    LEANCLR_CASE_BEGIN1(DivI8)
                    {
                        RtStackObject* dst = eval_stack_base + ir->dst;
                        RtStackObject* src1 = eval_stack_base + ir->arg1;
                        RtStackObject* src2 = eval_stack_base + ir->arg2;
                        int64_t dividend = src1->i64;
                        int64_t divisor = src2->i64;
                        if (divisor == 0)
                        {
                            RAISE_RUNTIME_ERROR(RtErr::DivideByZero);
                        }
                        if (divisor == -1 && dividend == INT64_MIN)
                        {
                            RAISE_RUNTIME_ERROR(RtErr::Arithmetic);
                        }
                        dst->i64 = dividend / divisor;
                    }
                    LEANCLR_CASE_END1()
                    LEANCLR_CASE_BEGIN1(DivR4)
                    {
                        RtStackObject* dst = eval_stack_base + ir->dst;
                        RtStackObject* src1 = eval_stack_base + ir->arg1;
                        RtStackObject* src2 = eval_stack_base + ir->arg2;
                        dst->f32 = src1->f32 / src2->f32;
                    }
                    LEANCLR_CASE_END1()
                    LEANCLR_CASE_BEGIN1(DivR8)
                    {
                        RtStackObject* dst = eval_stack_base + ir->dst;
                        RtStackObject* src1 = eval_stack_base + ir->arg1;
                        RtStackObject* src2 = eval_stack_base + ir->arg2;
                        dst->f64 = src1->f64 / src2->f64;
                    }
                    LEANCLR_CASE_END1()
                    LEANCLR_CASE_BEGIN1(DivUnI4)
                    {
                        RtStackObject* dst = eval_stack_base + ir->dst;
                        RtStackObject* src1 = eval_stack_base + ir->arg1;
                        RtStackObject* src2 = eval_stack_base + ir->arg2;
                        uint32_t dividend = src1->u32;
                        uint32_t divisor = src2->u32;
                        if (divisor == 0)
                        {
                            RAISE_RUNTIME_ERROR(RtErr::DivideByZero);
                        }
                        dst->u32 = dividend / divisor;
                    }
                    LEANCLR_CASE_END1()
                    LEANCLR_CASE_BEGIN1(DivUnI8)
                    {
                        RtStackObject* dst = eval_stack_base + ir->dst;
                        RtStackObject* src1 = eval_stack_base + ir->arg1;
                        RtStackObject* src2 = eval_stack_base + ir->arg2;
                        uint64_t dividend = src1->u64;
                        uint64_t divisor = src2->u64;
                        if (divisor == 0)
                        {
                            RAISE_RUNTIME_ERROR(RtErr::DivideByZero);
                        }
                        dst->u64 = dividend / divisor;
                    }
                    LEANCLR_CASE_END1()
                    LEANCLR_CASE_BEGIN1(RemI4)
                    {
                        RtStackObject* dst = eval_stack_base + ir->dst;
                        RtStackObject* src1 = eval_stack_base + ir->arg1;
                        RtStackObject* src2 = eval_stack_base + ir->arg2;
                        int32_t dividend = src1->i32;
                        int32_t divisor = src2->i32;
                        if (divisor == 0)
                        {
                            RAISE_RUNTIME_ERROR(RtErr::DivideByZero);
                        }
                        if (divisor == -1 && dividend == INT32_MIN)
                        {
                            RAISE_RUNTIME_ERROR(RtErr::Arithmetic);
                        }
                        else
                        {
                            dst->i32 = dividend % divisor;
                        }
                    }
                    LEANCLR_CASE_END1()
                    LEANCLR_CASE_BEGIN1(RemI8)
                    {
                        RtStackObject* dst = eval_stack_base + ir->dst;
                        RtStackObject* src1 = eval_stack_base + ir->arg1;
                        RtStackObject* src2 = eval_stack_base + ir->arg2;
                        int64_t dividend = src1->i64;
                        int64_t divisor = src2->i64;
                        if (divisor == 0)
                        {
                            RAISE_RUNTIME_ERROR(RtErr::DivideByZero);
                        }
                        if (divisor == -1 && dividend == INT64_MIN)
                        {
                            RAISE_RUNTIME_ERROR(RtErr::Arithmetic);
                        }
                        else
                        {
                            dst->i64 = dividend % divisor;
                        }
                    }
                    LEANCLR_CASE_END1()
                    LEANCLR_CASE_BEGIN1(RemR4)
                    {
                        RtStackObject* dst = eval_stack_base + ir->dst;
                        RtStackObject* src1 = eval_stack_base + ir->arg1;
                        RtStackObject* src2 = eval_stack_base + ir->arg2;
                        dst->f32 = std::fmod(src1->f32, src2->f32);
                    }
                    LEANCLR_CASE_END1()
                    LEANCLR_CASE_BEGIN1(RemR8)
                    {
                        RtStackObject* dst = eval_stack_base + ir->dst;
                        RtStackObject* src1 = eval_stack_base + ir->arg1;
                        RtStackObject* src2 = eval_stack_base + ir->arg2;
                        dst->f64 = std::fmod(src1->f64, src2->f64);
                    }
                    LEANCLR_CASE_END1()
                    LEANCLR_CASE_BEGIN1(RemUnI4)
                    {
                        RtStackObject* dst = eval_stack_base + ir->dst;
                        RtStackObject* src1 = eval_stack_base + ir->arg1;
                        RtStackObject* src2 = eval_stack_base + ir->arg2;
                        uint32_t dividend = src1->u32;
                        uint32_t divisor = src2->u32;
                        if (divisor == 0)
                        {
                            RAISE_RUNTIME_ERROR(RtErr::DivideByZero);
                        }
                        dst->u32 = dividend % divisor;
                    }
                    LEANCLR_CASE_END1()
                    LEANCLR_CASE_BEGIN1(RemUnI8)
                    {
                        RtStackObject* dst = eval_stack_base + ir->dst;
                        RtStackObject* src1 = eval_stack_base + ir->arg1;
                        RtStackObject* src2 = eval_stack_base + ir->arg2;
                        uint64_t dividend = src1->u64;
                        uint64_t divisor = src2->u64;
                        if (divisor == 0)
                        {
                            RAISE_RUNTIME_ERROR(RtErr::DivideByZero);
                        }
                        dst->u64 = dividend % divisor;
                    }
                    LEANCLR_CASE_END1()
                    LEANCLR_CASE_BEGIN1(AndI4)
                    {
                        RtStackObject* dst = eval_stack_base + ir->dst;
                        RtStackObject* src1 = eval_stack_base + ir->arg1;
                        RtStackObject* src2 = eval_stack_base + ir->arg2;
                        dst->i32 = src1->i32 & src2->i32;
                    }
                    LEANCLR_CASE_END1()
                    LEANCLR_CASE_BEGIN1(AndI8)
                    {
                        RtStackObject* dst = eval_stack_base + ir->dst;
                        RtStackObject* src1 = eval_stack_base + ir->arg1;
                        RtStackObject* src2 = eval_stack_base + ir->arg2;
                        dst->i64 = src1->i64 & src2->i64;
                    }
                    LEANCLR_CASE_END1()
                    LEANCLR_CASE_BEGIN1(OrI4)
                    {
                        RtStackObject* dst = eval_stack_base + ir->dst;
                        RtStackObject* src1 = eval_stack_base + ir->arg1;
                        RtStackObject* src2 = eval_stack_base + ir->arg2;
                        dst->i32 = src1->i32 | src2->i32;
                    }
                    LEANCLR_CASE_END1()
                    LEANCLR_CASE_BEGIN1(OrI8)
                    {
                        RtStackObject* dst = eval_stack_base + ir->dst;
                        RtStackObject* src1 = eval_stack_base + ir->arg1;
                        RtStackObject* src2 = eval_stack_base + ir->arg2;
                        dst->i64 = src1->i64 | src2->i64;
                    }
                    LEANCLR_CASE_END1()
                    LEANCLR_CASE_BEGIN1(XorI4)
                    {
                        RtStackObject* dst = eval_stack_base + ir->dst;
                        RtStackObject* src1 = eval_stack_base + ir->arg1;
                        RtStackObject* src2 = eval_stack_base + ir->arg2;
                        dst->i32 = src1->i32 ^ src2->i32;
                    }
                    LEANCLR_CASE_END1()
                    LEANCLR_CASE_BEGIN1(XorI8)
                    {
                        RtStackObject* dst = eval_stack_base + ir->dst;
                        RtStackObject* src1 = eval_stack_base + ir->arg1;
                        RtStackObject* src2 = eval_stack_base + ir->arg2;
                        dst->i64 = src1->i64 ^ src2->i64;
                    }
                    LEANCLR_CASE_END1()
                    LEANCLR_CASE_BEGIN1(ShlI4)
                    {
                        RtStackObject* dst = eval_stack_base + ir->dst;
                        RtStackObject* src1 = eval_stack_base + ir->arg1;
                        RtStackObject* src2 = eval_stack_base + ir->arg2;
                        dst->i32 = src1->i32 << src2->i32;
                    }
                    LEANCLR_CASE_END1()
                    LEANCLR_CASE_BEGIN1(ShlI8)
                    {
                        RtStackObject* dst = eval_stack_base + ir->dst;
                        RtStackObject* src1 = eval_stack_base + ir->arg1;
                        RtStackObject* src2 = eval_stack_base + ir->arg2;
                        dst->i64 = src1->i64 << src2->i32;
                    }
                    LEANCLR_CASE_END1()
                    LEANCLR_CASE_BEGIN1(ShrI4)
                    {
                        RtStackObject* dst = eval_stack_base + ir->dst;
                        RtStackObject* src1 = eval_stack_base + ir->arg1;
                        RtStackObject* src2 = eval_stack_base + ir->arg2;
                        dst->i32 = src1->i32 >> src2->i32;
                    }
                    LEANCLR_CASE_END1()
                    LEANCLR_CASE_BEGIN1(ShrI8)
                    {
                        RtStackObject* dst = eval_stack_base + ir->dst;
                        RtStackObject* src1 = eval_stack_base + ir->arg1;
                        RtStackObject* src2 = eval_stack_base + ir->arg2;
                        dst->i64 = src1->i64 >> src2->i32;
                    }
                    LEANCLR_CASE_END1()
                    LEANCLR_CASE_BEGIN1(ShrUnI4)
                    {
                        RtStackObject* dst = eval_stack_base + ir->dst;
                        RtStackObject* src1 = eval_stack_base + ir->arg1;
                        RtStackObject* src2 = eval_stack_base + ir->arg2;
                        dst->u32 = src1->u32 >> src2->i32;
                    }
                    LEANCLR_CASE_END1()
                    LEANCLR_CASE_BEGIN1(ShrUnI8)
                    {
                        RtStackObject* dst = eval_stack_base + ir->dst;
                        RtStackObject* src1 = eval_stack_base + ir->arg1;
                        RtStackObject* src2 = eval_stack_base + ir->arg2;
                        dst->u64 = src1->u64 >> src2->i32;
                    }
                    LEANCLR_CASE_END1()
                    LEANCLR_CASE_BEGIN1(NegI4)
                    {
                        RtStackObject* dst = eval_stack_base + ir->dst;
                        RtStackObject* src = eval_stack_base + ir->src;
                        dst->i32 = -src->i32;
                    }
                    LEANCLR_CASE_END1()
                    LEANCLR_CASE_BEGIN1(NegI8)
                    {
                        RtStackObject* dst = eval_stack_base + ir->dst;
                        RtStackObject* src = eval_stack_base + ir->src;
                        dst->i64 = -src->i64;
                    }
                    LEANCLR_CASE_END1()
                    LEANCLR_CASE_BEGIN1(NegR4)
                    {
                        RtStackObject* dst = eval_stack_base + ir->dst;
                        RtStackObject* src = eval_stack_base + ir->src;
                        dst->f32 = -src->f32;
                    }
                    LEANCLR_CASE_END1()
                    LEANCLR_CASE_BEGIN1(NegR8)
                    {
                        RtStackObject* dst = eval_stack_base + ir->dst;
                        RtStackObject* src = eval_stack_base + ir->src;
                        dst->f64 = -src->f64;
                    }
                    LEANCLR_CASE_END1()
                    LEANCLR_CASE_BEGIN1(NotI4)
                    {
                        RtStackObject* dst = eval_stack_base + ir->dst;
                        RtStackObject* src = eval_stack_base + ir->src;
                        dst->i32 = ~src->i32;
                    }
                    LEANCLR_CASE_END1()
                    LEANCLR_CASE_BEGIN1(NotI8)
                    {
                        RtStackObject* dst = eval_stack_base + ir->dst;
                        RtStackObject* src = eval_stack_base + ir->src;
                        dst->i64 = ~src->i64;
                    }
                    LEANCLR_CASE_END1()
                    LEANCLR_CASE_BEGIN1(CeqI4)
                    {
                        RtStackObject* dst = eval_stack_base + ir->dst;
                        RtStackObject* op1 = eval_stack_base + ir->arg1;
                        RtStackObject* op2 = eval_stack_base + ir->arg2;
                        dst->i32 = (op1->i32 == op2->i32) ? 1 : 0;
                    }
                    LEANCLR_CASE_END1()
                    LEANCLR_CASE_BEGIN1(CeqI8)
                    {
                        RtStackObject* dst = eval_stack_base + ir->dst;
                        RtStackObject* op1 = eval_stack_base + ir->arg1;
                        RtStackObject* op2 = eval_stack_base + ir->arg2;
                        dst->i32 = (op1->i64 == op2->i64) ? 1 : 0;
                    }
                    LEANCLR_CASE_END1()
                    LEANCLR_CASE_BEGIN1(CeqR4)
                    {
                        RtStackObject* dst = eval_stack_base + ir->dst;
                        RtStackObject* op1 = eval_stack_base + ir->arg1;
                        RtStackObject* op2 = eval_stack_base + ir->arg2;
                        float left = op1->f32;
                        float right = op2->f32;
                        dst->i32 = (left == right && !std::isunordered(left, right)) ? 1 : 0;
                    }
                    LEANCLR_CASE_END1()
                    LEANCLR_CASE_BEGIN1(CeqR8)
                    {
                        RtStackObject* dst = eval_stack_base + ir->dst;
                        RtStackObject* op1 = eval_stack_base + ir->arg1;
                        RtStackObject* op2 = eval_stack_base + ir->arg2;
                        double left = op1->f64;
                        double right = op2->f64;
                        dst->i32 = (left == right && !std::isunordered(left, right)) ? 1 : 0;
                    }
                    LEANCLR_CASE_END1()
                    LEANCLR_CASE_BEGIN1(CgtI4)
                    {
                        RtStackObject* dst = eval_stack_base + ir->dst;
                        RtStackObject* op1 = eval_stack_base + ir->arg1;
                        RtStackObject* op2 = eval_stack_base + ir->arg2;
                        dst->i32 = (op1->i32 > op2->i32) ? 1 : 0;
                    }
                    LEANCLR_CASE_END1()
                    LEANCLR_CASE_BEGIN1(CgtI8)
                    {
                        RtStackObject* dst = eval_stack_base + ir->dst;
                        RtStackObject* op1 = eval_stack_base + ir->arg1;
                        RtStackObject* op2 = eval_stack_base + ir->arg2;
                        dst->i32 = (op1->i64 > op2->i64) ? 1 : 0;
                    }
                    LEANCLR_CASE_END1()
                    LEANCLR_CASE_BEGIN1(CgtR4)
                    {
                        RtStackObject* dst = eval_stack_base + ir->dst;
                        RtStackObject* op1 = eval_stack_base + ir->arg1;
                        RtStackObject* op2 = eval_stack_base + ir->arg2;
                        float left = op1->f32;
                        float right = op2->f32;
                        dst->i32 = (left > right && !std::isunordered(left, right)) ? 1 : 0;
                    }
                    LEANCLR_CASE_END1()
                    LEANCLR_CASE_BEGIN1(CgtR8)
                    {
                        RtStackObject* dst = eval_stack_base + ir->dst;
                        RtStackObject* op1 = eval_stack_base + ir->arg1;
                        RtStackObject* op2 = eval_stack_base + ir->arg2;
                        double left = op1->f64;
                        double right = op2->f64;
                        dst->i32 = (left > right && !std::isunordered(left, right)) ? 1 : 0;
                    }
                    LEANCLR_CASE_END1()
                    LEANCLR_CASE_BEGIN1(CgtUnI4)
                    {
                        RtStackObject* dst = eval_stack_base + ir->dst;
                        RtStackObject* op1 = eval_stack_base + ir->arg1;
                        RtStackObject* op2 = eval_stack_base + ir->arg2;
                        dst->i32 = (op1->u32 > op2->u32) ? 1 : 0;
                    }
                    LEANCLR_CASE_END1()
                    LEANCLR_CASE_BEGIN1(CgtUnI8)
                    {
                        RtStackObject* dst = eval_stack_base + ir->dst;
                        RtStackObject* op1 = eval_stack_base + ir->arg1;
                        RtStackObject* op2 = eval_stack_base + ir->arg2;
                        dst->i32 = (op1->u64 > op2->u64) ? 1 : 0;
                    }
                    LEANCLR_CASE_END1()
                    LEANCLR_CASE_BEGIN1(CgtUnR4)
                    {
                        RtStackObject* dst = eval_stack_base + ir->dst;
                        RtStackObject* op1 = eval_stack_base + ir->arg1;
                        RtStackObject* op2 = eval_stack_base + ir->arg2;
                        float left = op1->f32;
                        float right = op2->f32;
                        dst->i32 = (left > right || std::isunordered(left, right)) ? 1 : 0;
                    }
                    LEANCLR_CASE_END1()
                    LEANCLR_CASE_BEGIN1(CgtUnR8)
                    {
                        RtStackObject* dst = eval_stack_base + ir->dst;
                        RtStackObject* op1 = eval_stack_base + ir->arg1;
                        RtStackObject* op2 = eval_stack_base + ir->arg2;
                        double left = op1->f64;
                        double right = op2->f64;
                        dst->i32 = (left > right || std::isunordered(left, right)) ? 1 : 0;
                    }
                    LEANCLR_CASE_END1()
                    LEANCLR_CASE_BEGIN1(CltI4)
                    {
                        RtStackObject* dst = eval_stack_base + ir->dst;
                        RtStackObject* op1 = eval_stack_base + ir->arg1;
                        RtStackObject* op2 = eval_stack_base + ir->arg2;
                        dst->i32 = (op1->i32 < op2->i32) ? 1 : 0;
                    }
                    LEANCLR_CASE_END1()
                    LEANCLR_CASE_BEGIN1(CltI8)
                    {
                        RtStackObject* dst = eval_stack_base + ir->dst;
                        RtStackObject* op1 = eval_stack_base + ir->arg1;
                        RtStackObject* op2 = eval_stack_base + ir->arg2;
                        dst->i32 = (op1->i64 < op2->i64) ? 1 : 0;
                    }
                    LEANCLR_CASE_END1()
                    LEANCLR_CASE_BEGIN1(CltR4)
                    {
                        RtStackObject* dst = eval_stack_base + ir->dst;
                        RtStackObject* op1 = eval_stack_base + ir->arg1;
                        RtStackObject* op2 = eval_stack_base + ir->arg2;
                        float left = op1->f32;
                        float right = op2->f32;
                        dst->i32 = (left < right && !std::isunordered(left, right)) ? 1 : 0;
                    }
                    LEANCLR_CASE_END1()
                    LEANCLR_CASE_BEGIN1(CltR8)
                    {
                        RtStackObject* dst = eval_stack_base + ir->dst;
                        RtStackObject* op1 = eval_stack_base + ir->arg1;
                        RtStackObject* op2 = eval_stack_base + ir->arg2;
                        double left = op1->f64;
                        double right = op2->f64;
                        dst->i32 = (left < right && !std::isunordered(left, right)) ? 1 : 0;
                    }
                    LEANCLR_CASE_END1()
                    LEANCLR_CASE_BEGIN1(CltUnI4)
                    {
                        RtStackObject* dst = eval_stack_base + ir->dst;
                        RtStackObject* op1 = eval_stack_base + ir->arg1;
                        RtStackObject* op2 = eval_stack_base + ir->arg2;
                        dst->i32 = (op1->u32 < op2->u32) ? 1 : 0;
                    }
                    LEANCLR_CASE_END1()
                    LEANCLR_CASE_BEGIN1(CltUnI8)
                    {
                        RtStackObject* dst = eval_stack_base + ir->dst;
                        RtStackObject* op1 = eval_stack_base + ir->arg1;
                        RtStackObject* op2 = eval_stack_base + ir->arg2;
                        dst->i32 = (op1->u64 < op2->u64) ? 1 : 0;
                    }
                    LEANCLR_CASE_END1()
                    LEANCLR_CASE_BEGIN1(CltUnR4)
                    {
                        RtStackObject* dst = eval_stack_base + ir->dst;
                        RtStackObject* op1 = eval_stack_base + ir->arg1;
                        RtStackObject* op2 = eval_stack_base + ir->arg2;
                        float left = op1->f32;
                        float right = op2->f32;
                        dst->i32 = (left < right || std::isunordered(left, right)) ? 1 : 0;
                    }
                    LEANCLR_CASE_END1()
                    LEANCLR_CASE_BEGIN1(CltUnR8)
                    {
                        RtStackObject* dst = eval_stack_base + ir->dst;
                        RtStackObject* op1 = eval_stack_base + ir->arg1;
                        RtStackObject* op2 = eval_stack_base + ir->arg2;
                        double left = op1->f64;
                        double right = op2->f64;
                        dst->i32 = (left < right || std::isunordered(left, right)) ? 1 : 0;
                    }
                    LEANCLR_CASE_END1()
                    LEANCLR_CASE_BEGIN1(InitObjI1)
                    {
                        RtStackObject* addr_obj = eval_stack_base + ir->addr;
                        int8_t* addr = reinterpret_cast<int8_t*>(addr_obj->ptr);
                        *addr = 0;
                    }
                    LEANCLR_CASE_END1()
                    LEANCLR_CASE_BEGIN1(InitObjI2)
                    {
                        RtStackObject* addr_obj = eval_stack_base + ir->addr;
                        int16_t* addr = reinterpret_cast<int16_t*>(addr_obj->ptr);
                        *addr = 0;
                    }
                    LEANCLR_CASE_END1()
                    LEANCLR_CASE_BEGIN1(InitObjI4)
                    {
                        RtStackObject* addr_obj = eval_stack_base + ir->addr;
                        int32_t* addr = reinterpret_cast<int32_t*>(addr_obj->ptr);
                        *addr = 0;
                    }
                    LEANCLR_CASE_END1()
                    LEANCLR_CASE_BEGIN1(InitObjI8)
                    {
                        RtStackObject* addr_obj = eval_stack_base + ir->addr;
                        int64_t* addr = reinterpret_cast<int64_t*>(addr_obj->ptr);
                        *addr = 0;
                    }
                    LEANCLR_CASE_END1()
                    LEANCLR_CASE_BEGIN1(InitObjAny)
                    {
                        RtStackObject* addr_obj = eval_stack_base + ir->addr;
                        uint8_t* addr = reinterpret_cast<uint8_t*>(addr_obj->ptr);
                        std::memset(addr, 0, ir->size);
                    }
                    LEANCLR_CASE_END1()
                    LEANCLR_CASE_BEGIN1(CpObjI1)
                    {
                        RtStackObject* dst_obj = eval_stack_base + ir->dst;
                        RtStackObject* src_obj = eval_stack_base + ir->src;
                        int8_t* dst_addr = reinterpret_cast<int8_t*>(dst_obj->ptr);
                        const int8_t* src_addr = reinterpret_cast<const int8_t*>(src_obj->ptr);
                        *dst_addr = *src_addr;
                    }
                    LEANCLR_CASE_END1()
                    LEANCLR_CASE_BEGIN1(CpObjI2)
                    {
                        RtStackObject* dst_obj = eval_stack_base + ir->dst;
                        RtStackObject* src_obj = eval_stack_base + ir->src;
                        int16_t* dst_addr = reinterpret_cast<int16_t*>(dst_obj->ptr);
                        const int16_t* src_addr = reinterpret_cast<const int16_t*>(src_obj->ptr);
                        *dst_addr = *src_addr;
                    }
                    LEANCLR_CASE_END1()
                    LEANCLR_CASE_BEGIN1(CpObjI4)
                    {
                        RtStackObject* dst_obj = eval_stack_base + ir->dst;
                        RtStackObject* src_obj = eval_stack_base + ir->src;
                        int32_t* dst_addr = reinterpret_cast<int32_t*>(dst_obj->ptr);
                        const int32_t* src_addr = reinterpret_cast<const int32_t*>(src_obj->ptr);
                        *dst_addr = *src_addr;
                    }
                    LEANCLR_CASE_END1()
                    LEANCLR_CASE_BEGIN1(CpObjI8)
                    {
                        RtStackObject* dst_obj = eval_stack_base + ir->dst;
                        RtStackObject* src_obj = eval_stack_base + ir->src;
                        int64_t* dst_addr = reinterpret_cast<int64_t*>(dst_obj->ptr);
                        const int64_t* src_addr = reinterpret_cast<const int64_t*>(src_obj->ptr);
                        *dst_addr = *src_addr;
                    }
                    LEANCLR_CASE_END1()
                    LEANCLR_CASE_BEGIN1(CpObjAny)
                    {
                        RtStackObject* dst_obj = eval_stack_base + ir->dst;
                        RtStackObject* src_obj = eval_stack_base + ir->src;
                        std::memcpy(dst_obj->ptr, src_obj->cptr, ir->size);
                    }
                    LEANCLR_CASE_END1()
                    LEANCLR_CASE_BEGIN1(LdObjAny)
                    {
                        RtStackObject* dst = eval_stack_base + ir->dst;
                        RtStackObject* addr_obj = eval_stack_base + ir->addr;
                        std::memcpy(dst, addr_obj->cptr, ir->size);
                    }
                    LEANCLR_CASE_END1()
                    LEANCLR_CASE_BEGIN1(StObjAny)
                    {
                        RtStackObject* addr_obj = eval_stack_base + ir->addr;
                        RtStackObject* src = eval_stack_base + ir->src;
                        std::memcpy(addr_obj->ptr, src, ir->size);
                    }
                    LEANCLR_CASE_END1()
                    LEANCLR_CASE_BEGIN1(CastClass)
                    {
                        vm::RtObject* obj = get_stack_value_at<vm::RtObject*>(eval_stack_base, ir->obj);
                        if (obj)
                        {
                            metadata::RtClass* to_class = get_resolved_data<metadata::RtClass>(imi, ir->klass_idx);
                            if (!vm::Class::is_assignable_from(obj->klass, to_class))
                            {
                                RAISE_RUNTIME_ERROR(RtErr::InvalidCast);
                            }
                        }
                    }
                    LEANCLR_CASE_END1()
                    LEANCLR_CASE_BEGIN1(IsInst)
                    {
                        vm::RtObject* obj = get_stack_value_at<vm::RtObject*>(eval_stack_base, ir->obj);
                        vm::RtObject* result;
                        if (obj)
                        {
                            metadata::RtClass* to_class = get_resolved_data<metadata::RtClass>(imi, ir->klass_idx);
                            result = vm::Class::is_assignable_from(obj->klass, to_class) ? obj : nullptr;
                        }
                        else
                        {
                            result = nullptr;
                        }
                        set_stack_value_at<vm::RtObject*>(eval_stack_base, ir->dst, result);
                    }
                    LEANCLR_CASE_END1()
                    LEANCLR_CASE_BEGIN1(Box)
                    {
                        RtStackObject* src = eval_stack_base + ir->src;
                        metadata::RtClass* to_class = get_resolved_data<metadata::RtClass>(imi, ir->klass_idx);
                        HANDLE_RAISE_RUNTIME_ERROR(vm::RtObject*, boxed_obj, vm::Object::box_object(to_class, src));
                        set_stack_value_at<vm::RtObject*>(eval_stack_base, ir->dst, boxed_obj);
                    }
                    LEANCLR_CASE_END1()
                    LEANCLR_CASE_BEGIN1(Unbox)
                    {
                        vm::RtObject* boxed_obj = get_stack_value_at<vm::RtObject*>(eval_stack_base, ir->obj);
                        if (!boxed_obj)
                        {
                            RAISE_RUNTIME_ERROR(RtErr::NullReference);
                        }
                        metadata::RtClass* to_class = get_resolved_data<metadata::RtClass>(imi, ir->klass_idx);
                        HANDLE_RAISE_RUNTIME_ERROR(const void*, unbox_addr, vm::Object::unbox_ex(boxed_obj, to_class));
                        set_stack_value_at(eval_stack_base, ir->dst, unbox_addr);
                    }
                    LEANCLR_CASE_END1()
                    LEANCLR_CASE_BEGIN1(UnboxAny)
                    {
                        vm::RtObject* boxed_obj = get_stack_value_at<vm::RtObject*>(eval_stack_base, ir->obj);
                        metadata::RtClass* to_class = get_resolved_data<metadata::RtClass>(imi, ir->klass_idx);
                        RtStackObject* dst = eval_stack_base + ir->dst;
                        HANDLE_RAISE_RUNTIME_ERROR_VOID(vm::Object::unbox_any(boxed_obj, to_class, dst, true));
                    }
                    LEANCLR_CASE_END1()
                    LEANCLR_CASE_BEGIN1(NewArr)
                    {
                        int32_t length = get_stack_value_at<int32_t>(eval_stack_base, ir->length);
                        metadata::RtClass* element_class = get_resolved_data<metadata::RtClass>(imi, ir->arr_klass_idx);
                        HANDLE_RAISE_RUNTIME_ERROR(vm::RtArray*, new_array, vm::Array::new_array_from_array_type(element_class, length));
                        set_stack_value_at<vm::RtArray*>(eval_stack_base, ir->dst, new_array);
                    }
                    LEANCLR_CASE_END1()
                    LEANCLR_CASE_BEGIN1(LdLen)
                    {
                        vm::RtArray* array = get_stack_value_at<vm::RtArray*>(eval_stack_base, ir->arr);
                        if (!array)
                        {
                            RAISE_RUNTIME_ERROR(RtErr::NullReference);
                        }
                        set_stack_value_at(eval_stack_base, ir->dst, vm::Array::get_array_length(array));
                    }
                    LEANCLR_CASE_END1()
                    LEANCLR_CASE_BEGIN1(Ldelema)
                    {
                        vm::RtArray* array = get_stack_value_at<vm::RtArray*>(eval_stack_base, ir->arr);
                        if (!array)
                        {
                            RAISE_RUNTIME_ERROR(RtErr::NullReference);
                        }
                        int32_t index = get_stack_value_at<int32_t>(eval_stack_base, ir->index);
                        if (vm::Array::is_out_of_range(array, index))
                        {
                            RAISE_RUNTIME_ERROR(RtErr::IndexOutOfRange);
                        }
                        metadata::RtClass* arr_klass = array->klass;
                        metadata::RtClass* element_klass = vm::Class::get_array_element_class(arr_klass);
                        metadata::RtClass* check_klass = get_resolved_data<metadata::RtClass>(imi, ir->ele_klass_idx);
                        if (!vm::Class::is_pointer_element_compatible_with(element_klass, check_klass))
                        {
                            RAISE_RUNTIME_ERROR(RtErr::ArrayTypeMismatch);
                        }
                        const void* element_addr = vm::Array::get_array_element_address_as_ptr_void(array, index);
                        set_stack_value_at(eval_stack_base, ir->dst, element_addr);
                    }
                    LEANCLR_CASE_END1()
                    LEANCLR_CASE_BEGIN1(LdelemI1)
                    {
                        vm::RtArray* array = get_stack_value_at<vm::RtArray*>(eval_stack_base, ir->arr);
                        if (!array)
                        {
                            RAISE_RUNTIME_ERROR(RtErr::NullReference);
                        }
                        assert(vm::Array::get_array_element_size(array) == 1);
                        int32_t index = get_stack_value_at<int32_t>(eval_stack_base, ir->index);
                        if (vm::Array::is_out_of_range(array, index))
                        {
                            RAISE_RUNTIME_ERROR(RtErr::IndexOutOfRange);
                        }
                        int8_t value = vm::Array::get_array_data_at<int8_t>(array, index);
                        set_stack_value_at<int32_t>(eval_stack_base, ir->dst, static_cast<int32_t>(value));
                    }
                    LEANCLR_CASE_END1()
                    LEANCLR_CASE_BEGIN1(LdelemU1)
                    {
                        vm::RtArray* array = get_stack_value_at<vm::RtArray*>(eval_stack_base, ir->arr);
                        if (!array)
                        {
                            RAISE_RUNTIME_ERROR(RtErr::NullReference);
                        }
                        assert(vm::Array::get_array_element_size(array) == 1);
                        int32_t index = get_stack_value_at<int32_t>(eval_stack_base, ir->index);
                        if (vm::Array::is_out_of_range(array, index))
                        {
                            RAISE_RUNTIME_ERROR(RtErr::IndexOutOfRange);
                        }
                        uint8_t value = vm::Array::get_array_data_at<uint8_t>(array, index);
                        set_stack_value_at<int32_t>(eval_stack_base, ir->dst, static_cast<int32_t>(value));
                    }
                    LEANCLR_CASE_END1()
                    LEANCLR_CASE_BEGIN1(LdelemI2)
                    {
                        vm::RtArray* array = get_stack_value_at<vm::RtArray*>(eval_stack_base, ir->arr);
                        if (!array)
                        {
                            RAISE_RUNTIME_ERROR(RtErr::NullReference);
                        }
                        assert(vm::Array::get_array_element_size(array) == 2);
                        int32_t index = get_stack_value_at<int32_t>(eval_stack_base, ir->index);
                        if (vm::Array::is_out_of_range(array, index))
                        {
                            RAISE_RUNTIME_ERROR(RtErr::IndexOutOfRange);
                        }
                        int16_t value = vm::Array::get_array_data_at<int16_t>(array, index);
                        set_stack_value_at<int32_t>(eval_stack_base, ir->dst, static_cast<int32_t>(value));
                    }
                    LEANCLR_CASE_END1()
                    LEANCLR_CASE_BEGIN1(LdelemU2)
                    {
                        vm::RtArray* array = get_stack_value_at<vm::RtArray*>(eval_stack_base, ir->arr);
                        if (!array)
                        {
                            RAISE_RUNTIME_ERROR(RtErr::NullReference);
                        }
                        assert(vm::Array::get_array_element_size(array) == 2);
                        int32_t index = get_stack_value_at<int32_t>(eval_stack_base, ir->index);
                        if (vm::Array::is_out_of_range(array, index))
                        {
                            RAISE_RUNTIME_ERROR(RtErr::IndexOutOfRange);
                        }
                        uint16_t value = vm::Array::get_array_data_at<uint16_t>(array, index);
                        set_stack_value_at<int32_t>(eval_stack_base, ir->dst, static_cast<int32_t>(value));
                    }
                    LEANCLR_CASE_END1()
                    LEANCLR_CASE_BEGIN1(LdelemI4)
                    {
                        vm::RtArray* array = get_stack_value_at<vm::RtArray*>(eval_stack_base, ir->arr);
                        if (!array)
                        {
                            RAISE_RUNTIME_ERROR(RtErr::NullReference);
                        }
                        assert(vm::Array::get_array_element_size(array) == 4);
                        int32_t index = get_stack_value_at<int32_t>(eval_stack_base, ir->index);
                        if (vm::Array::is_out_of_range(array, index))
                        {
                            RAISE_RUNTIME_ERROR(RtErr::IndexOutOfRange);
                        }
                        int32_t value = vm::Array::get_array_data_at<int32_t>(array, index);
                        set_stack_value_at<int32_t>(eval_stack_base, ir->dst, value);
                    }
                    LEANCLR_CASE_END1()
                    LEANCLR_CASE_BEGIN1(LdelemI8)
                    {
                        vm::RtArray* array = get_stack_value_at<vm::RtArray*>(eval_stack_base, ir->arr);
                        if (!array)
                        {
                            RAISE_RUNTIME_ERROR(RtErr::NullReference);
                        }
                        assert(vm::Array::get_array_element_size(array) == 8);
                        int32_t index = get_stack_value_at<int32_t>(eval_stack_base, ir->index);
                        if (vm::Array::is_out_of_range(array, index))
                        {
                            RAISE_RUNTIME_ERROR(RtErr::IndexOutOfRange);
                        }
                        int64_t value = vm::Array::get_array_data_at<int64_t>(array, index);
                        set_stack_value_at<int64_t>(eval_stack_base, ir->dst, value);
                    }
                    LEANCLR_CASE_END1()
                    LEANCLR_CASE_BEGIN1(LdelemI)
                    {
                        vm::RtArray* array = get_stack_value_at<vm::RtArray*>(eval_stack_base, ir->arr);
                        if (!array)
                        {
                            RAISE_RUNTIME_ERROR(RtErr::NullReference);
                        }
                        assert(vm::Array::get_array_element_size(array) == PTR_SIZE);
                        int32_t index = get_stack_value_at<int32_t>(eval_stack_base, ir->index);
                        if (vm::Array::is_out_of_range(array, index))
                        {
                            RAISE_RUNTIME_ERROR(RtErr::IndexOutOfRange);
                        }
                        intptr_t value = vm::Array::get_array_data_at<intptr_t>(array, index);
                        set_stack_value_at<intptr_t>(eval_stack_base, ir->dst, value);
                    }
                    LEANCLR_CASE_END1()
                    LEANCLR_CASE_BEGIN1(LdelemR4)
                    {
                        vm::RtArray* array = get_stack_value_at<vm::RtArray*>(eval_stack_base, ir->arr);
                        if (!array)
                        {
                            RAISE_RUNTIME_ERROR(RtErr::NullReference);
                        }
                        assert(vm::Array::get_array_element_size(array) == 4);
                        int32_t index = get_stack_value_at<int32_t>(eval_stack_base, ir->index);
                        if (vm::Array::is_out_of_range(array, index))
                        {
                            RAISE_RUNTIME_ERROR(RtErr::IndexOutOfRange);
                        }
                        float value = vm::Array::get_array_data_at<float>(array, index);
                        set_stack_value_at<float>(eval_stack_base, ir->dst, value);
                    }
                    LEANCLR_CASE_END1()
                    LEANCLR_CASE_BEGIN1(LdelemR8)
                    {
                        vm::RtArray* array = get_stack_value_at<vm::RtArray*>(eval_stack_base, ir->arr);
                        if (!array)
                        {
                            RAISE_RUNTIME_ERROR(RtErr::NullReference);
                        }
                        assert(vm::Array::get_array_element_size(array) == 8);
                        int32_t index = get_stack_value_at<int32_t>(eval_stack_base, ir->index);
                        if (vm::Array::is_out_of_range(array, index))
                        {
                            RAISE_RUNTIME_ERROR(RtErr::IndexOutOfRange);
                        }
                        double value = vm::Array::get_array_data_at<double>(array, index);
                        set_stack_value_at<double>(eval_stack_base, ir->dst, value);
                    }
                    LEANCLR_CASE_END1()
                    LEANCLR_CASE_BEGIN1(LdelemRef)
                    {
                        vm::RtArray* array = get_stack_value_at<vm::RtArray*>(eval_stack_base, ir->arr);
                        if (!array)
                        {
                            RAISE_RUNTIME_ERROR(RtErr::NullReference);
                        }
                        assert(vm::Array::get_array_element_size(array) == PTR_SIZE);
                        int32_t index = get_stack_value_at<int32_t>(eval_stack_base, ir->index);
                        if (vm::Array::is_out_of_range(array, index))
                        {
                            RAISE_RUNTIME_ERROR(RtErr::IndexOutOfRange);
                        }
                        vm::RtObject* value = vm::Array::get_array_data_at<vm::RtObject*>(array, index);
                        set_stack_value_at<vm::RtObject*>(eval_stack_base, ir->dst, value);
                    }
                    LEANCLR_CASE_END1()
                    LEANCLR_CASE_BEGIN1(LdelemAnyRef)
                    {
                        vm::RtArray* array = get_stack_value_at<vm::RtArray*>(eval_stack_base, ir->arr);
                        if (!array)
                        {
                            RAISE_RUNTIME_ERROR(RtErr::NullReference);
                        }
                        int32_t index = get_stack_value_at<int32_t>(eval_stack_base, ir->index);
                        if (vm::Array::is_out_of_range(array, index))
                        {
                            RAISE_RUNTIME_ERROR(RtErr::IndexOutOfRange);
                        }
                        vm::RtObject* value = vm::Array::get_array_data_at<vm::RtObject*>(array, index);
                        metadata::RtClass* check_klass = get_resolved_data<metadata::RtClass>(imi, ir->ele_klass_idx);
                        if (value && !vm::Class::is_assignable_from(value->klass, check_klass))
                        {
                            RAISE_RUNTIME_ERROR(RtErr::ArrayTypeMismatch);
                        }
                        set_stack_value_at<vm::RtObject*>(eval_stack_base, ir->dst, value);
                    }
                    LEANCLR_CASE_END1()
                    LEANCLR_CASE_BEGIN1(LdelemAnyVal)
                    {
                        vm::RtArray* array = get_stack_value_at<vm::RtArray*>(eval_stack_base, ir->arr);
                        if (!array)
                        {
                            RAISE_RUNTIME_ERROR(RtErr::NullReference);
                        }
                        int32_t index = get_stack_value_at<int32_t>(eval_stack_base, ir->index);
                        if (vm::Array::is_out_of_range(array, index))
                        {
                            RAISE_RUNTIME_ERROR(RtErr::IndexOutOfRange);
                        }
                        metadata::RtClass* ele_klass = vm::Array::get_array_element_class(array);
                        metadata::RtClass* check_klass = get_resolved_data<metadata::RtClass>(imi, ir->ele_klass_idx);
                        if (!vm::Class::is_pointer_element_compatible_with(ele_klass, check_klass))
                        {
                            RAISE_RUNTIME_ERROR(RtErr::ArrayTypeMismatch);
                        }
                        assert(ir->ele_size == vm::Array::get_array_element_size(array));
                        const void* src_addr = vm::Array::get_array_element_address_with_size_as_ptr_void(array, index, ir->ele_size);
                        RtStackObject* dst = eval_stack_base + ir->dst;
                        std::memcpy(dst, src_addr, ir->ele_size);
                    }
                    LEANCLR_CASE_END1()
                    LEANCLR_CASE_BEGIN1(StelemI1)
                    {
                        vm::RtArray* array = get_stack_value_at<vm::RtArray*>(eval_stack_base, ir->arr);
                        if (!array)
                        {
                            RAISE_RUNTIME_ERROR(RtErr::NullReference);
                        }
                        assert(vm::Array::get_array_element_size(array) == 1);
                        int32_t index = get_stack_value_at<int32_t>(eval_stack_base, ir->index);
                        if (vm::Array::is_out_of_range(array, index))
                        {
                            RAISE_RUNTIME_ERROR(RtErr::IndexOutOfRange);
                        }
                        int8_t value = get_stack_value_at<int8_t>(eval_stack_base, ir->value);
                        vm::Array::set_array_data_at<int8_t>(array, index, value);
                    }
                    LEANCLR_CASE_END1()
                    LEANCLR_CASE_BEGIN1(StelemI2)
                    {
                        vm::RtArray* array = get_stack_value_at<vm::RtArray*>(eval_stack_base, ir->arr);
                        if (!array)
                        {
                            RAISE_RUNTIME_ERROR(RtErr::NullReference);
                        }
                        assert(vm::Array::get_array_element_size(array) == 2);
                        int32_t index = get_stack_value_at<int32_t>(eval_stack_base, ir->index);
                        if (vm::Array::is_out_of_range(array, index))
                        {
                            RAISE_RUNTIME_ERROR(RtErr::IndexOutOfRange);
                        }
                        int16_t value = get_stack_value_at<int16_t>(eval_stack_base, ir->value);
                        vm::Array::set_array_data_at<int16_t>(array, index, value);
                    }
                    LEANCLR_CASE_END1()
                    LEANCLR_CASE_BEGIN1(StelemI4)
                    {
                        vm::RtArray* array = get_stack_value_at<vm::RtArray*>(eval_stack_base, ir->arr);
                        if (!array)
                        {
                            RAISE_RUNTIME_ERROR(RtErr::NullReference);
                        }
                        assert(vm::Array::get_array_element_size(array) == 4);
                        int32_t index = get_stack_value_at<int32_t>(eval_stack_base, ir->index);
                        if (vm::Array::is_out_of_range(array, index))
                        {
                            RAISE_RUNTIME_ERROR(RtErr::IndexOutOfRange);
                        }
                        int32_t value = get_stack_value_at<int32_t>(eval_stack_base, ir->value);
                        vm::Array::set_array_data_at<int32_t>(array, index, value);
                    }
                    LEANCLR_CASE_END1()
                    LEANCLR_CASE_BEGIN1(StelemI8)
                    {
                        vm::RtArray* array = get_stack_value_at<vm::RtArray*>(eval_stack_base, ir->arr);
                        if (!array)
                        {
                            RAISE_RUNTIME_ERROR(RtErr::NullReference);
                        }
                        assert(vm::Array::get_array_element_size(array) == 8);
                        int32_t index = get_stack_value_at<int32_t>(eval_stack_base, ir->index);
                        if (vm::Array::is_out_of_range(array, index))
                        {
                            RAISE_RUNTIME_ERROR(RtErr::IndexOutOfRange);
                        }
                        int64_t value = get_stack_value_at<int64_t>(eval_stack_base, ir->value);
                        vm::Array::set_array_data_at<int64_t>(array, index, value);
                    }
                    LEANCLR_CASE_END1()
                    LEANCLR_CASE_BEGIN1(StelemI)
                    {
                        vm::RtArray* array = get_stack_value_at<vm::RtArray*>(eval_stack_base, ir->arr);
                        if (!array)
                        {
                            RAISE_RUNTIME_ERROR(RtErr::NullReference);
                        }
                        assert(vm::Array::get_array_element_size(array) == PTR_SIZE);
                        int32_t index = get_stack_value_at<int32_t>(eval_stack_base, ir->index);
                        if (vm::Array::is_out_of_range(array, index))
                        {
                            RAISE_RUNTIME_ERROR(RtErr::IndexOutOfRange);
                        }
                        intptr_t value = get_stack_value_at<intptr_t>(eval_stack_base, ir->value);
                        vm::Array::set_array_data_at<intptr_t>(array, index, value);
                    }
                    LEANCLR_CASE_END1()
                    LEANCLR_CASE_BEGIN1(StelemR4)
                    {
                        vm::RtArray* array = get_stack_value_at<vm::RtArray*>(eval_stack_base, ir->arr);
                        if (!array)
                        {
                            RAISE_RUNTIME_ERROR(RtErr::NullReference);
                        }
                        assert(vm::Array::get_array_element_size(array) == 4);
                        int32_t index = get_stack_value_at<int32_t>(eval_stack_base, ir->index);
                        if (vm::Array::is_out_of_range(array, index))
                        {
                            RAISE_RUNTIME_ERROR(RtErr::IndexOutOfRange);
                        }
                        float value = get_stack_value_at<float>(eval_stack_base, ir->value);
                        vm::Array::set_array_data_at<float>(array, index, value);
                    }
                    LEANCLR_CASE_END1()
                    LEANCLR_CASE_BEGIN1(StelemR8)
                    {
                        vm::RtArray* array = get_stack_value_at<vm::RtArray*>(eval_stack_base, ir->arr);
                        if (!array)
                        {
                            RAISE_RUNTIME_ERROR(RtErr::NullReference);
                        }
                        assert(vm::Array::get_array_element_size(array) == 8);
                        int32_t index = get_stack_value_at<int32_t>(eval_stack_base, ir->index);
                        if (vm::Array::is_out_of_range(array, index))
                        {
                            RAISE_RUNTIME_ERROR(RtErr::IndexOutOfRange);
                        }
                        double value = get_stack_value_at<double>(eval_stack_base, ir->value);
                        vm::Array::set_array_data_at<double>(array, index, value);
                    }
                    LEANCLR_CASE_END1()
                    LEANCLR_CASE_BEGIN1(StelemRef)
                    {
                        vm::RtArray* array = get_stack_value_at<vm::RtArray*>(eval_stack_base, ir->arr);
                        if (!array)
                        {
                            RAISE_RUNTIME_ERROR(RtErr::NullReference);
                        }
                        assert(vm::Array::get_array_element_size(array) == PTR_SIZE);
                        int32_t index = get_stack_value_at<int32_t>(eval_stack_base, ir->index);
                        if (vm::Array::is_out_of_range(array, index))
                        {
                            RAISE_RUNTIME_ERROR(RtErr::IndexOutOfRange);
                        }
                        vm::RtObject* value = get_stack_value_at<vm::RtObject*>(eval_stack_base, ir->value);
                        vm::Array::set_array_data_at<vm::RtObject*>(array, index, value);
                    }
                    LEANCLR_CASE_END1()
                    LEANCLR_CASE_BEGIN1(StelemAnyRef)
                    {
                        vm::RtArray* array = get_stack_value_at<vm::RtArray*>(eval_stack_base, ir->arr);
                        if (!array)
                        {
                            RAISE_RUNTIME_ERROR(RtErr::NullReference);
                        }
                        int32_t index = get_stack_value_at<int32_t>(eval_stack_base, ir->index);
                        if (vm::Array::is_out_of_range(array, index))
                        {
                            RAISE_RUNTIME_ERROR(RtErr::IndexOutOfRange);
                        }
                        vm::RtObject* value = get_stack_value_at<vm::RtObject*>(eval_stack_base, ir->value);
                        metadata::RtClass* ele_klass = vm::Array::get_array_element_class(array);
                        if (value && !vm::Class::is_assignable_from(ele_klass, value->klass))
                        {
                            RAISE_RUNTIME_ERROR(RtErr::ArrayTypeMismatch);
                        }
                        vm::Array::set_array_data_at<vm::RtObject*>(array, index, value);
                    }
                    LEANCLR_CASE_END1()
                    LEANCLR_CASE_BEGIN1(StelemAnyVal)
                    {
                        vm::RtArray* array = get_stack_value_at<vm::RtArray*>(eval_stack_base, ir->arr);
                        if (!array)
                        {
                            RAISE_RUNTIME_ERROR(RtErr::NullReference);
                        }
                        int32_t index = get_stack_value_at<int32_t>(eval_stack_base, ir->index);
                        if (vm::Array::is_out_of_range(array, index))
                        {
                            RAISE_RUNTIME_ERROR(RtErr::IndexOutOfRange);
                        }
                        metadata::RtClass* ele_klass = vm::Array::get_array_element_class(array);
                        metadata::RtClass* check_klass = get_resolved_data<metadata::RtClass>(imi, ir->ele_klass_idx);
                        if (!vm::Class::is_pointer_element_compatible_with(ele_klass, check_klass))
                        {
                            RAISE_RUNTIME_ERROR(RtErr::ArrayTypeMismatch);
                        }
                        assert(ir->ele_size == vm::Array::get_array_element_size(array));
                        RtStackObject* src = eval_stack_base + ir->value;
                        void* dst_addr = const_cast<void*>(vm::Array::get_array_element_address_with_size_as_ptr_void(array, index, ir->ele_size));
                        std::memcpy(dst_addr, src, ir->ele_size);
                    }
                    LEANCLR_CASE_END1()
                    LEANCLR_CASE_BEGIN1(MkRefAny)
                    {
                        metadata::RtClass* klass = get_resolved_data<metadata::RtClass>(imi, ir->klass_idx);
                        const void* src_addr = get_stack_value_at<const void*>(eval_stack_base, ir->addr);
                        vm::RtTypedReference* dst = get_ptr_stack_value_at<vm::RtTypedReference>(eval_stack_base, ir->dst);
                        dst->type_handle = klass->by_val;
                        dst->value = src_addr;
                        dst->klass = klass;
                    }
                    LEANCLR_CASE_END1()
                    LEANCLR_CASE_BEGIN1(RefAnyVal)
                    {
                        vm::RtTypedReference* src = get_ptr_stack_value_at<vm::RtTypedReference>(eval_stack_base, ir->src);
                        metadata::RtClass* check_klass = get_resolved_data<metadata::RtClass>(imi, ir->klass_idx);
                        if (src->klass != check_klass)
                        {
                            RAISE_RUNTIME_ERROR(RtErr::InvalidCast);
                        }
                        set_stack_value_at<const void*>(eval_stack_base, ir->dst, src->value);
                    }
                    LEANCLR_CASE_END1()
                    LEANCLR_CASE_BEGIN1(RefAnyType)
                    {
                        vm::RtTypedReference* src = get_ptr_stack_value_at<vm::RtTypedReference>(eval_stack_base, ir->src);
                        set_stack_value_at<const metadata::RtTypeSig*>(eval_stack_base, ir->dst, src->type_handle);
                    }
                    LEANCLR_CASE_END1()
                    LEANCLR_CASE_BEGIN1(LdToken)
                    {
                        const void* token_data = get_resolved_data<void>(imi, ir->token_idx);
                        set_stack_value_at<const void*>(eval_stack_base, ir->dst, token_data);
                    }
                    LEANCLR_CASE_END1()
                    LEANCLR_CASE_BEGIN1(CkfiniteR4)
                    {
                        float value = get_stack_value_at<float>(eval_stack_base, ir->src);
                        if (!std::isfinite(value))
                        {
                            RAISE_RUNTIME_ERROR(RtErr::Overflow);
                        }
                    }
                    LEANCLR_CASE_END1()
                    LEANCLR_CASE_BEGIN1(CkfiniteR8)
                    {
                        double value = get_stack_value_at<double>(eval_stack_base, ir->src);
                        if (!std::isfinite(value))
                        {
                            RAISE_RUNTIME_ERROR(RtErr::Overflow);
                        }
                    }
                    LEANCLR_CASE_END1()
                    LEANCLR_CASE_BEGIN1(LocAlloc)
                    {
                        uint32_t size = get_stack_value_at<uint32_t>(eval_stack_base, ir->size);
                        void* data;
                        if (size > 0)
                        {
                            data = imi->init_locals ? alloc::GeneralAllocation::malloc_zeroed(size) : alloc::GeneralAllocation::malloc(size);
                        }
                        else
                        {
                            data = nullptr;
                        }
                        set_stack_value_at<void*>(eval_stack_base, ir->dst, data);
                    }
                    LEANCLR_CASE_END1()
                    LEANCLR_CASE_BEGIN1(Ldftn)
                    {
                        metadata::RtMethodInfo* method = get_resolved_data<metadata::RtMethodInfo>(imi, ir->method_idx);
                        set_stack_value_at(eval_stack_base, ir->dst, reinterpret_cast<void*>(method));
                    }
                    LEANCLR_CASE_END1()
                    LEANCLR_CASE_BEGIN1(Ldvirtftn)
                    {
                        vm::RtObject* obj = get_stack_value_at<vm::RtObject*>(eval_stack_base, ir->obj);
                        if (!obj)
                        {
                            RAISE_RUNTIME_ERROR(RtErr::NullReference);
                        }
                        const metadata::RtMethodInfo* method = get_resolved_data<metadata::RtMethodInfo>(imi, ir->method_idx);
                        assert(method->slot < method->parent->vtable_count);
                        DECLARING_AND_UNWRAP_OR_RET_ERR_ON_FAIL(const metadata::RtMethodInfo*, actual_method, vm::Method::get_virtual_method_impl(obj, method));
                        set_stack_value_at(eval_stack_base, ir->dst, actual_method);
                    }
                    LEANCLR_CASE_END1()
                    LEANCLR_CASE_BEGIN1(LdfldI1)
                    {
                        vm::RtObject* obj = get_stack_value_at<vm::RtObject*>(eval_stack_base, ir->obj);
                        if (!obj)
                        {
                            RAISE_RUNTIME_ERROR(RtErr::NullReference);
                        }
                        const int8_t* field_addr = reinterpret_cast<const int8_t*>(reinterpret_cast<const uint8_t*>(obj) + ir->offset);
                        int8_t value = *field_addr;
                        set_stack_value_at<int32_t>(eval_stack_base, ir->dst, static_cast<int32_t>(value));
                    }
                    LEANCLR_CASE_END1()
                    LEANCLR_CASE_BEGIN1(LdfldU1)
                    {
                        vm::RtObject* obj = get_stack_value_at<vm::RtObject*>(eval_stack_base, ir->obj);
                        if (!obj)
                        {
                            RAISE_RUNTIME_ERROR(RtErr::NullReference);
                        }
                        const uint8_t* field_addr = reinterpret_cast<const uint8_t*>(obj) + ir->offset;
                        uint8_t value = *field_addr;
                        set_stack_value_at<int32_t>(eval_stack_base, ir->dst, static_cast<int32_t>(value));
                    }
                    LEANCLR_CASE_END1()
                    LEANCLR_CASE_BEGIN1(LdfldI2)
                    {
                        vm::RtObject* obj = get_stack_value_at<vm::RtObject*>(eval_stack_base, ir->obj);
                        if (!obj)
                        {
                            RAISE_RUNTIME_ERROR(RtErr::NullReference);
                        }
                        const int16_t* field_addr = reinterpret_cast<const int16_t*>(reinterpret_cast<const uint8_t*>(obj) + ir->offset);
                        int16_t value = *field_addr;
                        set_stack_value_at<int32_t>(eval_stack_base, ir->dst, static_cast<int32_t>(value));
                    }
                    LEANCLR_CASE_END1()
                    LEANCLR_CASE_BEGIN1(LdfldU2)
                    {
                        vm::RtObject* obj = get_stack_value_at<vm::RtObject*>(eval_stack_base, ir->obj);
                        if (!obj)
                        {
                            RAISE_RUNTIME_ERROR(RtErr::NullReference);
                        }
                        const uint16_t* field_addr = reinterpret_cast<const uint16_t*>(reinterpret_cast<const uint8_t*>(obj) + ir->offset);
                        uint16_t value = *field_addr;
                        set_stack_value_at<int32_t>(eval_stack_base, ir->dst, static_cast<int32_t>(value));
                    }
                    LEANCLR_CASE_END1()
                    LEANCLR_CASE_BEGIN1(LdfldI4)
                    {
                        vm::RtObject* obj = get_stack_value_at<vm::RtObject*>(eval_stack_base, ir->obj);
                        if (!obj)
                        {
                            RAISE_RUNTIME_ERROR(RtErr::NullReference);
                        }
                        const int32_t* field_addr = reinterpret_cast<const int32_t*>(reinterpret_cast<const uint8_t*>(obj) + ir->offset);
                        int32_t value = *field_addr;
                        set_stack_value_at<int32_t>(eval_stack_base, ir->dst, value);
                    }
                    LEANCLR_CASE_END1()
                    LEANCLR_CASE_BEGIN1(LdfldI8)
                    {
                        vm::RtObject* obj = get_stack_value_at<vm::RtObject*>(eval_stack_base, ir->obj);
                        if (!obj)
                        {
                            RAISE_RUNTIME_ERROR(RtErr::NullReference);
                        }
                        const int64_t* field_addr = reinterpret_cast<const int64_t*>(reinterpret_cast<const uint8_t*>(obj) + ir->offset);
                        int64_t value = *field_addr;
                        set_stack_value_at<int64_t>(eval_stack_base, ir->dst, value);
                    }
                    LEANCLR_CASE_END1()
                    LEANCLR_CASE_BEGIN1(LdfldAny)
                    {
                        vm::RtObject* obj = get_stack_value_at<vm::RtObject*>(eval_stack_base, ir->obj);
                        if (!obj)
                        {
                            RAISE_RUNTIME_ERROR(RtErr::NullReference);
                        }
                        const uint8_t* field_addr = reinterpret_cast<const uint8_t*>(obj) + ir->offset;
                        RtStackObject* dst = eval_stack_base + ir->dst;
                        std::memcpy(dst, field_addr, ir->size);
                    }
                    LEANCLR_CASE_END1()
                    LEANCLR_CASE_BEGIN1(LdvfldI1)
                    {
                        RtStackObject* addr_obj = eval_stack_base + ir->obj;
                        const int8_t* field_addr = reinterpret_cast<const int8_t*>(reinterpret_cast<const uint8_t*>(addr_obj) + ir->offset);
                        int8_t value = *field_addr;
                        set_stack_value_at<int32_t>(eval_stack_base, ir->dst, static_cast<int32_t>(value));
                    }
                    LEANCLR_CASE_END1()
                    LEANCLR_CASE_BEGIN1(LdvfldU1)
                    {
                        RtStackObject* addr_obj = eval_stack_base + ir->obj;
                        const uint8_t* field_addr = reinterpret_cast<const uint8_t*>(addr_obj) + ir->offset;
                        uint8_t value = *field_addr;
                        set_stack_value_at<int32_t>(eval_stack_base, ir->dst, static_cast<int32_t>(value));
                    }
                    LEANCLR_CASE_END1()
                    LEANCLR_CASE_BEGIN1(LdvfldI2)
                    {
                        RtStackObject* addr_obj = eval_stack_base + ir->obj;
                        const int16_t* field_addr = reinterpret_cast<const int16_t*>(reinterpret_cast<const uint8_t*>(addr_obj) + ir->offset);
                        int16_t value = *field_addr;
                        set_stack_value_at<int32_t>(eval_stack_base, ir->dst, static_cast<int32_t>(value));
                    }
                    LEANCLR_CASE_END1()
                    LEANCLR_CASE_BEGIN1(LdvfldU2)
                    {
                        RtStackObject* addr_obj = eval_stack_base + ir->obj;
                        const uint16_t* field_addr = reinterpret_cast<const uint16_t*>(reinterpret_cast<const uint8_t*>(addr_obj) + ir->offset);
                        uint16_t value = *field_addr;
                        set_stack_value_at<int32_t>(eval_stack_base, ir->dst, static_cast<int32_t>(value));
                    }
                    LEANCLR_CASE_END1()
                    LEANCLR_CASE_BEGIN1(LdvfldI4)
                    {
                        RtStackObject* addr_obj = eval_stack_base + ir->obj;
                        const int32_t* field_addr = reinterpret_cast<const int32_t*>(reinterpret_cast<const uint8_t*>(addr_obj) + ir->offset);
                        int32_t value = *field_addr;
                        set_stack_value_at<int32_t>(eval_stack_base, ir->dst, value);
                    }
                    LEANCLR_CASE_END1()
                    LEANCLR_CASE_BEGIN1(LdvfldI8)
                    {
                        RtStackObject* addr_obj = eval_stack_base + ir->obj;
                        const int64_t* field_addr = reinterpret_cast<const int64_t*>(reinterpret_cast<const uint8_t*>(addr_obj) + ir->offset);
                        int64_t value = *field_addr;
                        set_stack_value_at<int64_t>(eval_stack_base, ir->dst, value);
                    }
                    LEANCLR_CASE_END1()
                    LEANCLR_CASE_BEGIN1(LdvfldAny)
                    {
                        RtStackObject* addr_obj = eval_stack_base + ir->obj;
                        const uint8_t* field_addr = reinterpret_cast<const uint8_t*>(addr_obj) + ir->offset;
                        RtStackObject* dst = eval_stack_base + ir->dst;
                        std::memmove(dst, field_addr, ir->size);
                    }
                    LEANCLR_CASE_END1()
                    LEANCLR_CASE_BEGIN1(Ldflda)
                    {
                        vm::RtObject* obj = get_stack_value_at<vm::RtObject*>(eval_stack_base, ir->obj);
                        if (!obj)
                        {
                            RAISE_RUNTIME_ERROR(RtErr::NullReference);
                        }
                        const uint8_t* field_addr = reinterpret_cast<const uint8_t*>(obj) + ir->offset;
                        set_stack_value_at(eval_stack_base, ir->dst, field_addr);
                    }
                    LEANCLR_CASE_END1()
                    LEANCLR_CASE_BEGIN1(StfldI1)
                    {
                        vm::RtObject* obj = get_stack_value_at<vm::RtObject*>(eval_stack_base, ir->obj);
                        if (!obj)
                        {
                            RAISE_RUNTIME_ERROR(RtErr::NullReference);
                        }
                        int8_t value = get_stack_value_at<int8_t>(eval_stack_base, ir->value);
                        int8_t* field_addr = reinterpret_cast<int8_t*>(reinterpret_cast<uint8_t*>(obj) + ir->offset);
                        *field_addr = value;
                    }
                    LEANCLR_CASE_END1()
                    LEANCLR_CASE_BEGIN1(StfldI2)
                    {
                        vm::RtObject* obj = get_stack_value_at<vm::RtObject*>(eval_stack_base, ir->obj);
                        if (!obj)
                        {
                            RAISE_RUNTIME_ERROR(RtErr::NullReference);
                        }
                        int16_t value = get_stack_value_at<int16_t>(eval_stack_base, ir->value);
                        int16_t* field_addr = reinterpret_cast<int16_t*>(reinterpret_cast<uint8_t*>(obj) + ir->offset);
                        *field_addr = value;
                    }
                    LEANCLR_CASE_END1()
                    LEANCLR_CASE_BEGIN1(StfldI4)
                    {
                        vm::RtObject* obj = get_stack_value_at<vm::RtObject*>(eval_stack_base, ir->obj);
                        if (!obj)
                        {
                            RAISE_RUNTIME_ERROR(RtErr::NullReference);
                        }
                        int32_t value = get_stack_value_at<int32_t>(eval_stack_base, ir->value);
                        int32_t* field_addr = reinterpret_cast<int32_t*>(reinterpret_cast<uint8_t*>(obj) + ir->offset);
                        *field_addr = value;
                    }
                    LEANCLR_CASE_END1()
                    LEANCLR_CASE_BEGIN1(StfldI8)
                    {
                        vm::RtObject* obj = get_stack_value_at<vm::RtObject*>(eval_stack_base, ir->obj);
                        if (!obj)
                        {
                            RAISE_RUNTIME_ERROR(RtErr::NullReference);
                        }
                        int64_t value = get_stack_value_at<int64_t>(eval_stack_base, ir->value);
                        int64_t* field_addr = reinterpret_cast<int64_t*>(reinterpret_cast<uint8_t*>(obj) + ir->offset);
                        *field_addr = value;
                    }
                    LEANCLR_CASE_END1()
                    LEANCLR_CASE_BEGIN1(StfldAny)
                    {
                        vm::RtObject* obj = get_stack_value_at<vm::RtObject*>(eval_stack_base, ir->obj);
                        if (!obj)
                        {
                            RAISE_RUNTIME_ERROR(RtErr::NullReference);
                        }
                        RtStackObject* src = eval_stack_base + ir->value;
                        uint8_t* field_addr = reinterpret_cast<uint8_t*>(obj) + ir->offset;
                        std::memcpy(field_addr, src, ir->size);
                    }
                    LEANCLR_CASE_END1()
                    LEANCLR_CASE_BEGIN1(LdsfldI1)
                    {
                        const metadata::RtFieldInfo* field = get_resolved_data<metadata::RtFieldInfo>(imi, ir->field_idx);
                        TRY_RUN_CLASS_STATIC_CCTOR(field->parent);
                        const int8_t* field_addr = get_static_field_address<int8_t>(field);
                        int8_t value = *field_addr;
                        set_stack_value_at<int32_t>(eval_stack_base, ir->dst, static_cast<int32_t>(value));
                    }
                    LEANCLR_CASE_END1()
                    LEANCLR_CASE_BEGIN1(LdsfldU1)
                    {
                        const metadata::RtFieldInfo* field = get_resolved_data<metadata::RtFieldInfo>(imi, ir->field_idx);
                        TRY_RUN_CLASS_STATIC_CCTOR(field->parent);
                        const uint8_t* field_addr = get_static_field_address<uint8_t>(field);
                        uint8_t value = *field_addr;
                        set_stack_value_at<int32_t>(eval_stack_base, ir->dst, static_cast<int32_t>(value));
                    }
                    LEANCLR_CASE_END1()
                    LEANCLR_CASE_BEGIN1(LdsfldI2)
                    {
                        const metadata::RtFieldInfo* field = get_resolved_data<metadata::RtFieldInfo>(imi, ir->field_idx);
                        TRY_RUN_CLASS_STATIC_CCTOR(field->parent);
                        const int16_t* field_addr = get_static_field_address<int16_t>(field);
                        int16_t value = *field_addr;
                        set_stack_value_at<int32_t>(eval_stack_base, ir->dst, static_cast<int32_t>(value));
                    }
                    LEANCLR_CASE_END1()
                    LEANCLR_CASE_BEGIN1(LdsfldU2)
                    {
                        const metadata::RtFieldInfo* field = get_resolved_data<metadata::RtFieldInfo>(imi, ir->field_idx);
                        TRY_RUN_CLASS_STATIC_CCTOR(field->parent);
                        const uint16_t* field_addr = get_static_field_address<uint16_t>(field);
                        uint16_t value = *field_addr;
                        set_stack_value_at<int32_t>(eval_stack_base, ir->dst, static_cast<int32_t>(value));
                    }
                    LEANCLR_CASE_END1()
                    LEANCLR_CASE_BEGIN1(LdsfldI4)
                    {
                        const metadata::RtFieldInfo* field = get_resolved_data<metadata::RtFieldInfo>(imi, ir->field_idx);
                        TRY_RUN_CLASS_STATIC_CCTOR(field->parent);
                        const int32_t* field_addr = get_static_field_address<int32_t>(field);
                        int32_t value = *field_addr;
                        set_stack_value_at<int32_t>(eval_stack_base, ir->dst, value);
                    }
                    LEANCLR_CASE_END1()
                    LEANCLR_CASE_BEGIN1(LdsfldI8)
                    {
                        const metadata::RtFieldInfo* field = get_resolved_data<metadata::RtFieldInfo>(imi, ir->field_idx);
                        TRY_RUN_CLASS_STATIC_CCTOR(field->parent);
                        const int64_t* field_addr = get_static_field_address<int64_t>(field);
                        int64_t value = *field_addr;
                        set_stack_value_at<int64_t>(eval_stack_base, ir->dst, value);
                    }
                    LEANCLR_CASE_END1()
                    LEANCLR_CASE_BEGIN1(LdsfldAny)
                    {
                        const metadata::RtFieldInfo* field = get_resolved_data<metadata::RtFieldInfo>(imi, ir->field_idx);
                        TRY_RUN_CLASS_STATIC_CCTOR(field->parent);
                        const uint8_t* field_addr = get_static_field_address<uint8_t>(field);
                        RtStackObject* dst = eval_stack_base + ir->dst;
                        std::memcpy(dst, field_addr, ir->size);
                    }
                    LEANCLR_CASE_END1()
                    LEANCLR_CASE_BEGIN1(Ldsflda)
                    {
                        const metadata::RtFieldInfo* field = get_resolved_data<metadata::RtFieldInfo>(imi, ir->field_idx);
                        metadata::RtClass* klass = field->parent;
                        TRY_RUN_CLASS_STATIC_CCTOR(klass);
                        const void* field_addr = get_static_field_address<void>(field);
                        set_stack_value_at(eval_stack_base, ir->dst, field_addr);
                    }
                    LEANCLR_CASE_END1()
                    LEANCLR_CASE_BEGIN1(LdsfldRvaData)
                    {
                        const void* rva_data = get_resolved_data<void>(imi, ir->data);
                        set_stack_value_at(eval_stack_base, ir->dst, rva_data);
                    }
                    LEANCLR_CASE_END1()
                    LEANCLR_CASE_BEGIN1(StsfldI1)
                    {
                        int32_t value = get_stack_value_at<int32_t>(eval_stack_base, ir->value);
                        const metadata::RtFieldInfo* field = get_resolved_data<metadata::RtFieldInfo>(imi, ir->field_idx);
                        metadata::RtClass* klass = field->parent;
                        TRY_RUN_CLASS_STATIC_CCTOR(klass);
                        int8_t* field_addr = get_static_field_address<int8_t>(field);
                        *field_addr = static_cast<int8_t>(value);
                    }
                    LEANCLR_CASE_END1()
                    LEANCLR_CASE_BEGIN1(StsfldI2)
                    {
                        int32_t value = get_stack_value_at<int32_t>(eval_stack_base, ir->value);
                        const metadata::RtFieldInfo* field = get_resolved_data<metadata::RtFieldInfo>(imi, ir->field_idx);
                        int16_t* field_addr = get_static_field_address<int16_t>(field);
                        *field_addr = static_cast<int16_t>(value);
                    }
                    LEANCLR_CASE_END1()
                    LEANCLR_CASE_BEGIN1(StsfldI4)
                    {
                        int32_t value = get_stack_value_at<int32_t>(eval_stack_base, ir->value);
                        const metadata::RtFieldInfo* field = get_resolved_data<metadata::RtFieldInfo>(imi, ir->field_idx);
                        metadata::RtClass* klass = field->parent;
                        TRY_RUN_CLASS_STATIC_CCTOR(klass);
                        int32_t* field_addr = get_static_field_address<int32_t>(field);
                        *field_addr = value;
                    }
                    LEANCLR_CASE_END1()
                    LEANCLR_CASE_BEGIN1(StsfldI8)
                    {
                        int64_t value = get_stack_value_at<int64_t>(eval_stack_base, ir->value);
                        const metadata::RtFieldInfo* field = get_resolved_data<metadata::RtFieldInfo>(imi, ir->field_idx);
                        metadata::RtClass* klass = field->parent;
                        TRY_RUN_CLASS_STATIC_CCTOR(klass);
                        int64_t* field_addr = get_static_field_address<int64_t>(field);
                        *field_addr = value;
                    }
                    LEANCLR_CASE_END1()
                    LEANCLR_CASE_BEGIN1(StsfldAny)
                    {
                        RtStackObject* src = eval_stack_base + ir->value;
                        const metadata::RtFieldInfo* field = get_resolved_data<metadata::RtFieldInfo>(imi, ir->field_idx);
                        metadata::RtClass* klass = field->parent;
                        TRY_RUN_CLASS_STATIC_CCTOR(klass);
                        uint8_t* field_addr = get_static_field_address<uint8_t>(field);
                        std::memcpy(field_addr, src, ir->size);
                    }
                    LEANCLR_CASE_END1()
                    LEANCLR_CASE_BEGIN1(RetVoid)
                    {
                        LEAVE_FRAME();
                    }
                    LEANCLR_CASE_END1()
                    LEANCLR_CASE_BEGIN1(RetI4)
                    {
                        eval_stack_base->i32 = get_stack_value_at<int32_t>(eval_stack_base, ir->src);
                        LEAVE_FRAME();
                    }
                    LEANCLR_CASE_END1()
                    LEANCLR_CASE_BEGIN1(RetI8)
                    {
                        eval_stack_base->i64 = get_stack_value_at<int64_t>(eval_stack_base, ir->src);
                        LEAVE_FRAME();
                    }
                    LEANCLR_CASE_END1()
                    LEANCLR_CASE_BEGIN1(RetAny)
                    {
                        std::memcpy(eval_stack_base, eval_stack_base + ir->src, ir->size * sizeof(RtStackObject));
                        LEAVE_FRAME();
                    }
                    LEANCLR_CASE_END1()
                    LEANCLR_CASE_BEGIN_LITE1(CallInterp)
                    {
                        const auto* ir = reinterpret_cast<const ll::CallInterp*>(ip);
                        const metadata::RtMethodInfo* target_method = get_resolved_data<metadata::RtMethodInfo>(imi, ir->method_idx);
                        if (vm::Method::is_static(target_method))
                        {
                            TRY_RUN_CLASS_STATIC_CCTOR(target_method->parent);
                        }
                        const uint8_t* next_ip = reinterpret_cast<const uint8_t*>(ir + 1);
                        ENTER_INTERP_FRAME(target_method, ir->frame_base, next_ip);
                    }
                    LEANCLR_CASE_END_LITE1()
                    LEANCLR_CASE_BEGIN_LITE1(CallVirtInterp)
                    {
                        const auto* ir = reinterpret_cast<const ll::CallVirtInterp*>(ip);
                        vm::RtObject* obj = get_stack_value_at<vm::RtObject*>(eval_stack_base, ir->frame_base);
                        if (!obj)
                        {
                            RAISE_RUNTIME_ERROR(RtErr::NullReference);
                        }
                        const metadata::RtMethodInfo* original_method = get_resolved_data<metadata::RtMethodInfo>(imi, ir->method_idx);
                        DECLARING_AND_UNWRAP_OR_RET_ERR_ON_FAIL(const metadata::RtMethodInfo*, actual_method,
                                                                vm::Method::get_virtual_method_impl(obj, original_method));
                        if (vm::Class::is_value_type(actual_method->parent))
                        {
                            set_stack_value_at(eval_stack_base, ir->frame_base, obj + 1);
                        }
                        if (actual_method->invoker_type == metadata::RtInvokerType::Interpreter)
                        {
                            ENTER_INTERP_FRAME(actual_method, ir->frame_base, reinterpret_cast<const uint8_t*>(ir + 1));
                        }
                        else
                        {
                            ip = reinterpret_cast<const uint8_t*>(ir + 1);
                            RtStackObject* frame_base = eval_stack_base + ir->frame_base;
                            HANDLE_RAISE_RUNTIME_ERROR_VOID(actual_method->invoke_method_ptr(actual_method->method_ptr, actual_method, frame_base, frame_base));
                        }
                    }
                    LEANCLR_CASE_END_LITE1()
                    LEANCLR_CASE_BEGIN_LITE1(CallInternalCall)
                    {
                        const auto* ir = reinterpret_cast<const ll::CallInternalCall*>(ip);
                        const metadata::RtMethodInfo* target_method = get_resolved_data<metadata::RtMethodInfo>(imi, ir->method_idx);
                        if (vm::Method::is_static(target_method))
                        {
                            TRY_RUN_CLASS_STATIC_CCTOR(target_method->parent);
                        }
                        ip = reinterpret_cast<const uint8_t*>(ir + 1);
                        RtStackObject* frame_base = eval_stack_base + ir->frame_base;
                        HANDLE_RAISE_RUNTIME_ERROR_VOID(target_method->invoke_method_ptr(target_method->method_ptr, target_method, frame_base, frame_base));
                    }
                    LEANCLR_CASE_END_LITE1()
                    LEANCLR_CASE_BEGIN_LITE1(CallIntrinsic)
                    {
                        const auto* ir = reinterpret_cast<const ll::CallIntrinsic*>(ip);
                        const metadata::RtMethodInfo* target_method = get_resolved_data<metadata::RtMethodInfo>(imi, ir->method_idx);
                        if (vm::Method::is_static(target_method))
                        {
                            TRY_RUN_CLASS_STATIC_CCTOR(target_method->parent);
                        }
                        ip = reinterpret_cast<const uint8_t*>(ir + 1);
                        RtStackObject* frame_base = eval_stack_base + ir->frame_base;
                        HANDLE_RAISE_RUNTIME_ERROR_VOID(target_method->invoke_method_ptr(target_method->method_ptr, target_method, frame_base, frame_base));
                    }
                    LEANCLR_CASE_END_LITE1()
                    LEANCLR_CASE_BEGIN_LITE1(CallPInvoke)
                    {
                        const auto* ir = reinterpret_cast<const ll::CallPInvoke*>(ip);
                        const metadata::RtMethodInfo* target_method = get_resolved_data<metadata::RtMethodInfo>(imi, ir->method_idx);
                        if (vm::Method::is_static(target_method))
                        {
                            TRY_RUN_CLASS_STATIC_CCTOR(target_method->parent);
                        }
                        ip = reinterpret_cast<const uint8_t*>(ir + 1);
                        RtStackObject* frame_base = eval_stack_base + ir->frame_base;
                        HANDLE_RAISE_RUNTIME_ERROR_VOID(target_method->invoke_method_ptr(target_method->method_ptr, target_method, frame_base, frame_base));
                    }
                    LEANCLR_CASE_END_LITE1()
                    LEANCLR_CASE_BEGIN_LITE1(CallRuntimeImplemented)
                    {
                        const auto* ir = reinterpret_cast<const ll::CallRuntimeImplemented*>(ip);
                        const metadata::RtMethodInfo* target_method = get_resolved_data<metadata::RtMethodInfo>(imi, ir->method_idx);
                        if (vm::Method::is_static(target_method))
                        {
                            TRY_RUN_CLASS_STATIC_CCTOR(target_method->parent);
                        }
                        ip = reinterpret_cast<const uint8_t*>(ir + 1);
                        RtStackObject* frame_base = eval_stack_base + ir->frame_base;
                        HANDLE_RAISE_RUNTIME_ERROR_VOID(target_method->invoke_method_ptr(target_method->method_ptr, target_method, frame_base, frame_base));
                    }
                    LEANCLR_CASE_END_LITE1()
                    LEANCLR_CASE_BEGIN_LITE1(CalliInterp)
                    {
                        const auto* ir = reinterpret_cast<const ll::CalliInterp*>(ip);
                        const metadata::RtMethodSig* method_sig = get_resolved_data<const metadata::RtMethodSig>(imi, ir->method_sig_idx);
                        const uint8_t* next_ip = reinterpret_cast<const uint8_t*>(ir + 1);
                        const metadata::RtMethodInfo* target_method = get_stack_value_at<const metadata::RtMethodInfo*>(eval_stack_base, ir->method_idx);
                        if (target_method->parameter_count != method_sig->params.size())
                        {
                            RAISE_RUNTIME_ERROR(RtErr::ExecutionEngine);
                        }

                        if (target_method->invoker_type == metadata::RtInvokerType::Interpreter)
                        {
                            ENTER_INTERP_FRAME(target_method, ir->frame_base, reinterpret_cast<const uint8_t*>(ir + 1));
                        }
                        else
                        {
                            ip = reinterpret_cast<const uint8_t*>(ir + 1);
                            RtStackObject* frame_base = eval_stack_base + ir->frame_base;
                            HANDLE_RAISE_RUNTIME_ERROR_VOID(target_method->invoke_method_ptr(target_method->method_ptr, target_method, frame_base, frame_base));
                        }
                    }
                    LEANCLR_CASE_END_LITE1()
                    LEANCLR_CASE_BEGIN1(BoxRefInplace)
                    {
                        const void* src_ptr = get_stack_value_at<const void*>(eval_stack_base, ir->src);
                        metadata::RtClass* to_klass = get_resolved_data<metadata::RtClass>(imi, ir->klass_idx);
                        HANDLE_RAISE_RUNTIME_ERROR(vm::RtObject*, boxed_obj, vm::Object::box_object(to_klass, src_ptr));
                        set_stack_value_at<vm::RtObject*>(eval_stack_base, ir->dst, boxed_obj);
                    }
                    LEANCLR_CASE_END1()
                    LEANCLR_CASE_BEGIN_LITE1(NewObjInterp)
                    {
                        const auto* ir = reinterpret_cast<const ll::NewObjInterp*>(ip);
                        const metadata::RtMethodInfo* ctor = get_resolved_data<metadata::RtMethodInfo>(imi, ir->method_idx);
                        metadata::RtClass* klass = ctor->parent;
                        TRY_RUN_CLASS_STATIC_CCTOR(klass);
                        HANDLE_RAISE_RUNTIME_ERROR(vm::RtObject*, obj, vm::Object::new_object(klass));
                        RtStackObject* frame_base = eval_stack_base + ir->frame_base;
                        std::memmove(frame_base + 1, frame_base, static_cast<size_t>(ir->total_params_stack_object_size) * sizeof(RtStackObject));
                        frame_base->obj = obj;
                        ENTER_INTERP_FRAME(ctor, ir->frame_base, reinterpret_cast<const uint8_t*>(ir + 1));
                    }
                    LEANCLR_CASE_END_LITE1()
                    LEANCLR_CASE_BEGIN_LITE1(NewValueTypeInterp)
                    {
                        const auto* ir = reinterpret_cast<const ll::NewValueTypeInterp*>(ip);
                        const metadata::RtMethodInfo* ctor = get_resolved_data<metadata::RtMethodInfo>(imi, ir->method_idx);
                        metadata::RtClass* klass = ctor->parent;
                        RtStackObject* original_frame_base = eval_stack_base + ir->frame_base;
                        const size_t value_stack_objects = InterpDefs::get_stack_object_size_by_byte_size(klass->instance_size_without_header);
                        RtStackObject* final_frame_base = original_frame_base + value_stack_objects;
                        std::memmove(final_frame_base + 1, original_frame_base,
                                     static_cast<size_t>(ir->total_params_stack_object_size) * sizeof(RtStackObject));
                        final_frame_base->ptr = original_frame_base;
                        std::memset(original_frame_base, 0, value_stack_objects * sizeof(RtStackObject));
                        ENTER_INTERP_FRAME(ctor, ir->frame_base + value_stack_objects, reinterpret_cast<const uint8_t*>(ir + 1));
                    }
                    LEANCLR_CASE_END_LITE1()
                    LEANCLR_CASE_BEGIN_LITE1(NewObjInternalCall)
                    {
                        const auto* ir = reinterpret_cast<const ll::NewObjInternalCall*>(ip);
                        const metadata::RtMethodInfo* target_method = get_resolved_data<metadata::RtMethodInfo>(imi, ir->method_idx);
                        TRY_RUN_CLASS_STATIC_CCTOR(target_method->parent);
                        ip = reinterpret_cast<const uint8_t*>(ir + 1);
                        vm::InternalCallInvoker invoker = vm::InternalCalls::get_internal_call_invoker_by_id(ir->invoker_idx);
                        RtStackObject* frame_base = eval_stack_base + ir->frame_base;
                        HANDLE_RAISE_RUNTIME_ERROR_VOID(invoker(target_method->method_ptr, target_method, frame_base, frame_base));
                    }
                    LEANCLR_CASE_END_LITE1()
                    LEANCLR_CASE_BEGIN1(NewObjIntrinsic)
                    {
                        const auto* ir = reinterpret_cast<const ll::NewObjIntrinsic*>(ip);
                        const metadata::RtMethodInfo* target_method = get_resolved_data<metadata::RtMethodInfo>(imi, ir->method_idx);
                        TRY_RUN_CLASS_STATIC_CCTOR(target_method->parent);
                        ip = reinterpret_cast<const uint8_t*>(ir + 1);
                        vm::InternalCallInvoker invoker = vm::Intrinsics::get_intrinsic_invoker_by_id(ir->invoker_idx);
                        RtStackObject* frame_base = eval_stack_base + ir->frame_base;
                        HANDLE_RAISE_RUNTIME_ERROR_VOID(invoker(target_method->method_ptr, target_method, frame_base, frame_base));
                    }
                    LEANCLR_CASE_END1()
                    LEANCLR_CASE_BEGIN1(Throw)
                    {
                        vm::RtException* ex = get_stack_value_at<vm::RtException*>(eval_stack_base, ir->ex);
                        if (!ex)
                        {
                            RAISE_RUNTIME_ERROR(RtErr::NullReference);
                        }
                        if (!vm::Class::is_exception_sub_class(ex->klass))
                        {
                            RAISE_RUNTIME_ERROR(RtErr::InvalidCast);
                        }
                        RAISE_RUNTIME_EXCEPTION(ex);
                    }
                    LEANCLR_CASE_END1()
                    LEANCLR_CASE_BEGIN1(Rethrow)
                    {
                        vm::RtException* ex = find_exception_in_enclosing_throw_flow(frame, static_cast<uint32_t>(ip - imi->codes));
                        if (!ex)
                        {
                            RAISE_RUNTIME_ERROR(RtErr::ExecutionEngine);
                        }
                        RAISE_RUNTIME_EXCEPTION(ex);
                    }
                    LEANCLR_CASE_END1()
                    LEANCLR_CASE_BEGIN_LITE1(LeaveTryWithFinally)
                    {
                        const auto* ir = reinterpret_cast<const ll::LeaveTryWithFinally*>(ip);
                        assert(ir->finally_clauses_count > 0);
                        assert(ir->first_finally_clause_index < imi->exception_clause_count);
                        const uint8_t* target_ip = ip + ir->target_offset;
                        const RtInterpExceptionClause* finally_clause = &imi->exception_clauses[ir->first_finally_clause_index];
                        push_leave_flow(frame, ip, target_ip, finally_clause, ir->first_finally_clause_index + 1, ir->finally_clauses_count - 1);
                        ip = imi->codes + finally_clause->handler_begin_offset;
                    }
                    LEANCLR_CASE_END_LITE1()
                    LEANCLR_CASE_BEGIN_LITE1(LeaveCatchWithFinally)
                    {
                        const auto* ir = reinterpret_cast<const ll::LeaveCatchWithFinally*>(ip);
                        assert(ir->finally_clauses_count > 0);
                        assert(ir->first_finally_clause_index < imi->exception_clause_count);
                        vm::RtException* ex = get_exception_in_last_throw_flow(frame, static_cast<uint32_t>(ip - imi->codes));
                        pop_throw_flow(ex, frame);
                        const uint8_t* target_ip = ip + ir->target_offset;
                        const RtInterpExceptionClause* finally_clause = &imi->exception_clauses[ir->first_finally_clause_index];
                        push_leave_flow(frame, ip, target_ip, finally_clause, ir->first_finally_clause_index + 1, ir->finally_clauses_count - 1);
                        ip = imi->codes + finally_clause->handler_begin_offset;
                    }
                    LEANCLR_CASE_END_LITE1()
                    LEANCLR_CASE_BEGIN_LITE1(LeaveCatchWithoutFinally)
                    {
                        const auto* ir = reinterpret_cast<const ll::LeaveCatchWithoutFinally*>(ip);
                        vm::RtException* ex = get_exception_in_last_throw_flow(frame, static_cast<uint32_t>(ip - imi->codes));
                        pop_throw_flow(ex, frame);
                        ip += ir->target_offset;
                    }
                    LEANCLR_CASE_END_LITE1()
                    LEANCLR_CASE_BEGIN_LITE1(EndFilter)
                    {
                        const auto* ir = reinterpret_cast<const ll::EndFilter*>(ip);
                        int32_t cond = get_stack_value_at<int32_t>(eval_stack_base, ir->cond);
                        if (cond)
                        {
                            ip = reinterpret_cast<const uint8_t*>(ir + 1);
                            setup_filter_handler(imi, frame, ip);
                            vm::RtException* ex = get_exception_in_last_throw_flow(frame, static_cast<uint32_t>(ip - imi->codes));
                            set_stack_value_at<vm::RtObject*>(eval_stack_base, imi->total_arg_and_local_stack_object_size, ex);
                        }
                        else
                        {
                            goto unwind_exception_handler;
                        }
                    }
                    LEANCLR_CASE_END_LITE1()
                    LEANCLR_CASE_BEGIN_LITE1(EndFinally)
                    {
                        goto unwind_exception_handler;
                    }
                    LEANCLR_CASE_END_LITE1()
                    LEANCLR_CASE_BEGIN_LITE1(EndFault)
                    {
                        goto unwind_exception_handler;
                    }
                    LEANCLR_CASE_END_LITE1()
#if !LEANCLR_USE_COMPUTED_GOTO_DISPATCHER
                default:
                {
                    assert(false && "Invalid opcode");
                    RAISE_RUNTIME_ERROR(RtErr::ExecutionEngine);
                }
#endif
                }
            }
            LEANCLR_CASE_END_LITE0()
            LEANCLR_CASE_BEGIN_LITE0(Prefix2)
            {
                const uint8_t* ip2 = ip + 1;
                LEANCLR_SWITCH2()
                {
                    LEANCLR_CASE_BEGIN2(LdIndI1)
                    {
                        int8_t value = get_ind_stack_value_at<int8_t>(eval_stack_base, ir->src);
                        set_stack_value_at<int32_t>(eval_stack_base, ir->dst, static_cast<int32_t>(value));
                    }
                    LEANCLR_CASE_END2()
                    LEANCLR_CASE_BEGIN2(LdIndU1)
                    {
                        uint8_t value = get_ind_stack_value_at<uint8_t>(eval_stack_base, ir->src);
                        set_stack_value_at<int32_t>(eval_stack_base, ir->dst, static_cast<int32_t>(value));
                    }
                    LEANCLR_CASE_END2()
                    LEANCLR_CASE_BEGIN2(LdIndI2)
                    {
                        int16_t value = get_ind_stack_value_at<int16_t>(eval_stack_base, ir->src);
                        set_stack_value_at<int32_t>(eval_stack_base, ir->dst, static_cast<int32_t>(value));
                    }
                    LEANCLR_CASE_END2()
                    LEANCLR_CASE_BEGIN2(LdIndU2)
                    {
                        uint16_t value = get_ind_stack_value_at<uint16_t>(eval_stack_base, ir->src);
                        set_stack_value_at<int32_t>(eval_stack_base, ir->dst, static_cast<int32_t>(value));
                    }
                    LEANCLR_CASE_END2()
                    LEANCLR_CASE_BEGIN2(LdIndI4)
                    {
                        int32_t value = get_ind_stack_value_at<int32_t>(eval_stack_base, ir->src);
                        set_stack_value_at<int32_t>(eval_stack_base, ir->dst, value);
                    }
                    LEANCLR_CASE_END2()
                    LEANCLR_CASE_BEGIN2(LdIndI8)
                    {
                        int64_t value = get_ind_stack_value_at<int64_t>(eval_stack_base, ir->src);
                        set_stack_value_at<int64_t>(eval_stack_base, ir->dst, value);
                    }
                    LEANCLR_CASE_END2()
                    LEANCLR_CASE_BEGIN2(StIndI1)
                    {
                        int32_t value = get_stack_value_at<int32_t>(eval_stack_base, ir->src);
                        set_ind_stack_value_at<int8_t>(eval_stack_base, ir->dst, static_cast<int8_t>(value));
                    }
                    LEANCLR_CASE_END2()
                    LEANCLR_CASE_BEGIN2(StIndI2)
                    {
                        int32_t value = get_stack_value_at<int32_t>(eval_stack_base, ir->src);
                        set_ind_stack_value_at<int16_t>(eval_stack_base, ir->dst, static_cast<int16_t>(value));
                    }
                    LEANCLR_CASE_END2()
                    LEANCLR_CASE_BEGIN2(StIndI4)
                    {
                        int32_t value = get_stack_value_at<int32_t>(eval_stack_base, ir->src);
                        set_ind_stack_value_at<int32_t>(eval_stack_base, ir->dst, value);
                    }
                    LEANCLR_CASE_END2()
                    LEANCLR_CASE_BEGIN2(StIndI8)
                    {
                        int64_t value = get_stack_value_at<int64_t>(eval_stack_base, ir->src);
                        set_ind_stack_value_at<int64_t>(eval_stack_base, ir->dst, value);
                    }
                    LEANCLR_CASE_END2()
                    LEANCLR_CASE_BEGIN2(StIndI8I4)
                    {
                        int32_t value = get_stack_value_at<int32_t>(eval_stack_base, ir->src);
                        set_ind_stack_value_at<int64_t>(eval_stack_base, ir->dst, static_cast<int64_t>(value));
                    }
                    LEANCLR_CASE_END2()
                    LEANCLR_CASE_BEGIN2(StIndI8U4)
                    {
                        uint32_t value = get_stack_value_at<uint32_t>(eval_stack_base, ir->src);
                        set_ind_stack_value_at<int64_t>(eval_stack_base, ir->dst, static_cast<int64_t>(value));
                    }
                    LEANCLR_CASE_END2()
                    LEANCLR_CASE_BEGIN2(ConvI1I4)
                    {
                        int32_t src = get_stack_value_at<int32_t>(eval_stack_base, ir->src);
                        int8_t value = static_cast<int8_t>(src);
                        set_stack_value_at<int32_t>(eval_stack_base, ir->dst, static_cast<int32_t>(value));
                    }
                    LEANCLR_CASE_END2()
                    LEANCLR_CASE_BEGIN2(ConvI1I8)
                    {
                        int64_t src = get_stack_value_at<int64_t>(eval_stack_base, ir->src);
                        int8_t value = static_cast<int8_t>(src);
                        set_stack_value_at<int32_t>(eval_stack_base, ir->dst, static_cast<int32_t>(value));
                    }
                    LEANCLR_CASE_END2()
                    LEANCLR_CASE_BEGIN2(ConvI1R4)
                    {
                        float src = get_stack_value_at<float>(eval_stack_base, ir->src);
                        int32_t value = cast_float_to_small_int<float, int8_t>(src);
                        set_stack_value_at<int32_t>(eval_stack_base, ir->dst, value);
                    }
                    LEANCLR_CASE_END2()
                    LEANCLR_CASE_BEGIN2(ConvI1R8)
                    {
                        double src = get_stack_value_at<double>(eval_stack_base, ir->src);
                        int32_t value = cast_float_to_small_int<double, int8_t>(src);
                        set_stack_value_at<int32_t>(eval_stack_base, ir->dst, value);
                    }
                    LEANCLR_CASE_END2()
                    LEANCLR_CASE_BEGIN2(ConvU1I4)
                    {
                        int32_t src = get_stack_value_at<int32_t>(eval_stack_base, ir->src);
                        uint8_t value = static_cast<uint8_t>(src);
                        set_stack_value_at<int32_t>(eval_stack_base, ir->dst, static_cast<int32_t>(value));
                    }
                    LEANCLR_CASE_END2()
                    LEANCLR_CASE_BEGIN2(ConvU1I8)
                    {
                        int64_t src = get_stack_value_at<int64_t>(eval_stack_base, ir->src);
                        uint8_t value = static_cast<uint8_t>(src);
                        set_stack_value_at<int32_t>(eval_stack_base, ir->dst, static_cast<int32_t>(value));
                    }
                    LEANCLR_CASE_END2()
                    LEANCLR_CASE_BEGIN2(ConvU1R4)
                    {
                        float src = get_stack_value_at<float>(eval_stack_base, ir->src);
                        int32_t value = cast_float_to_small_int<float, uint8_t>(src);
                        set_stack_value_at<int32_t>(eval_stack_base, ir->dst, value);
                    }
                    LEANCLR_CASE_END2()
                    LEANCLR_CASE_BEGIN2(ConvU1R8)
                    {
                        double src = get_stack_value_at<double>(eval_stack_base, ir->src);
                        int32_t value = cast_float_to_small_int<double, uint8_t>(src);
                        set_stack_value_at<int32_t>(eval_stack_base, ir->dst, value);
                    }
                    LEANCLR_CASE_END2()
                    LEANCLR_CASE_BEGIN2(ConvI2I4)
                    {
                        int32_t src = get_stack_value_at<int32_t>(eval_stack_base, ir->src);
                        int16_t value = static_cast<int16_t>(src);
                        set_stack_value_at<int32_t>(eval_stack_base, ir->dst, static_cast<int32_t>(value));
                    }
                    LEANCLR_CASE_END2()
                    LEANCLR_CASE_BEGIN2(ConvI2I8)
                    {
                        int64_t src = get_stack_value_at<int64_t>(eval_stack_base, ir->src);
                        int16_t value = static_cast<int16_t>(src);
                        set_stack_value_at<int64_t>(eval_stack_base, ir->dst, static_cast<int64_t>(value));
                    }
                    LEANCLR_CASE_END2()
                    LEANCLR_CASE_BEGIN2(ConvI2R4)
                    {
                        float src = get_stack_value_at<float>(eval_stack_base, ir->src);
                        int32_t value = cast_float_to_small_int<float, int16_t>(src);
                        set_stack_value_at<int32_t>(eval_stack_base, ir->dst, value);
                    }
                    LEANCLR_CASE_END2()
                    LEANCLR_CASE_BEGIN2(ConvI2R8)
                    {
                        double src = get_stack_value_at<double>(eval_stack_base, ir->src);
                        int32_t value = cast_float_to_small_int<double, int16_t>(src);
                        set_stack_value_at<int32_t>(eval_stack_base, ir->dst, value);
                    }
                    LEANCLR_CASE_END2()
                    LEANCLR_CASE_BEGIN2(ConvU2I4)
                    {
                        int32_t src = get_stack_value_at<int32_t>(eval_stack_base, ir->src);
                        uint16_t value = static_cast<uint16_t>(src);
                        set_stack_value_at<int32_t>(eval_stack_base, ir->dst, static_cast<int32_t>(value));
                    }
                    LEANCLR_CASE_END2()
                    LEANCLR_CASE_BEGIN2(ConvU2I8)
                    {
                        int64_t src = get_stack_value_at<int64_t>(eval_stack_base, ir->src);
                        uint16_t value = static_cast<uint16_t>(src);
                        set_stack_value_at<int64_t>(eval_stack_base, ir->dst, static_cast<int64_t>(value));
                    }
                    LEANCLR_CASE_END2()
                    LEANCLR_CASE_BEGIN2(ConvU2R4)
                    {
                        float src = get_stack_value_at<float>(eval_stack_base, ir->src);
                        int32_t value = cast_float_to_small_int<float, uint16_t>(src);
                        set_stack_value_at<int32_t>(eval_stack_base, ir->dst, value);
                    }
                    LEANCLR_CASE_END2()
                    LEANCLR_CASE_BEGIN2(ConvU2R8)
                    {
                        double src = get_stack_value_at<double>(eval_stack_base, ir->src);
                        int32_t value = cast_float_to_small_int<double, uint16_t>(src);
                        set_stack_value_at<int32_t>(eval_stack_base, ir->dst, value);
                    }
                    LEANCLR_CASE_END2()
                    LEANCLR_CASE_BEGIN2(ConvI4I8)
                    {
                        int32_t src = get_stack_value_at<int32_t>(eval_stack_base, ir->src);
                        set_stack_value_at<int32_t>(eval_stack_base, ir->dst, src);
                    }
                    LEANCLR_CASE_END2()
                    LEANCLR_CASE_BEGIN2(ConvI4R4)
                    {
                        float src = get_stack_value_at<float>(eval_stack_base, ir->src);
                        int32_t value = cast_float_to_i32<float, int32_t>(src);
                        set_stack_value_at<int32_t>(eval_stack_base, ir->dst, value);
                    }
                    LEANCLR_CASE_END2()
                    LEANCLR_CASE_BEGIN2(ConvI4R8)
                    {
                        double src = get_stack_value_at<double>(eval_stack_base, ir->src);
                        int32_t value = cast_float_to_i32<double, int32_t>(src);
                        set_stack_value_at<int32_t>(eval_stack_base, ir->dst, value);
                    }
                    LEANCLR_CASE_END2()
                    LEANCLR_CASE_BEGIN2(ConvU4I8)
                    {
                        uint32_t src = get_stack_value_at<uint32_t>(eval_stack_base, ir->src);
                        set_stack_value_at<uint32_t>(eval_stack_base, ir->dst, src);
                    }
                    LEANCLR_CASE_END2()
                    LEANCLR_CASE_BEGIN2(ConvU4R4)
                    {
                        float src = get_stack_value_at<float>(eval_stack_base, ir->src);
                        int32_t value = cast_float_to_i32<float, uint32_t>(src);
                        set_stack_value_at<int32_t>(eval_stack_base, ir->dst, value);
                    }
                    LEANCLR_CASE_END2()
                    LEANCLR_CASE_BEGIN2(ConvU4R8)
                    {
                        double src = get_stack_value_at<double>(eval_stack_base, ir->src);
                        int32_t value = cast_float_to_i32<double, uint32_t>(src);
                        set_stack_value_at<int32_t>(eval_stack_base, ir->dst, value);
                    }
                    LEANCLR_CASE_END2()
                    LEANCLR_CASE_BEGIN2(ConvI8I4)
                    {
                        int32_t src = get_stack_value_at<int32_t>(eval_stack_base, ir->src);
                        set_stack_value_at<int64_t>(eval_stack_base, ir->dst, static_cast<int64_t>(src));
                    }
                    LEANCLR_CASE_END2()
                    LEANCLR_CASE_BEGIN2(ConvI8U4)
                    {
                        uint32_t src = get_stack_value_at<uint32_t>(eval_stack_base, ir->src);
                        set_stack_value_at<int64_t>(eval_stack_base, ir->dst, static_cast<int64_t>(src));
                    }
                    LEANCLR_CASE_END2()
                    LEANCLR_CASE_BEGIN2(ConvI8R4)
                    {
                        float src = get_stack_value_at<float>(eval_stack_base, ir->src);
                        int64_t value = cast_float_to_i64<float, int64_t>(src);
                        set_stack_value_at<int64_t>(eval_stack_base, ir->dst, value);
                    }
                    LEANCLR_CASE_END2()
                    LEANCLR_CASE_BEGIN2(ConvI8R8)
                    {
                        double src = get_stack_value_at<double>(eval_stack_base, ir->src);
                        int64_t value = cast_float_to_i64<double, int64_t>(src);
                        set_stack_value_at<int64_t>(eval_stack_base, ir->dst, value);
                    }
                    LEANCLR_CASE_END2()
                    LEANCLR_CASE_BEGIN2(ConvU8I4)
                    {
                        uint32_t src = get_stack_value_at<uint32_t>(eval_stack_base, ir->src);
                        set_stack_value_at<uint64_t>(eval_stack_base, ir->dst, static_cast<uint64_t>(src));
                    }
                    LEANCLR_CASE_END2()
                    LEANCLR_CASE_BEGIN2(ConvU8R4)
                    {
                        float src = get_stack_value_at<float>(eval_stack_base, ir->src);
                        int64_t value = cast_float_to_i64<float, uint64_t>(src);
                        set_stack_value_at<int64_t>(eval_stack_base, ir->dst, value);
                    }
                    LEANCLR_CASE_END2()
                    LEANCLR_CASE_BEGIN2(ConvU8R8)
                    {
                        double src = get_stack_value_at<double>(eval_stack_base, ir->src);
                        int64_t value = cast_float_to_i64<double, uint64_t>(src);
                        set_stack_value_at<int64_t>(eval_stack_base, ir->dst, value);
                    }
                    LEANCLR_CASE_END2()
                    LEANCLR_CASE_BEGIN2(ConvR4I4)
                    {
                        int32_t src = get_stack_value_at<int32_t>(eval_stack_base, ir->src);
                        set_stack_value_at<float>(eval_stack_base, ir->dst, static_cast<float>(src));
                    }
                    LEANCLR_CASE_END2()
                    LEANCLR_CASE_BEGIN2(ConvR4I8)
                    {
                        int64_t src = get_stack_value_at<int64_t>(eval_stack_base, ir->src);
                        set_stack_value_at<float>(eval_stack_base, ir->dst, static_cast<float>(src));
                    }
                    LEANCLR_CASE_END2()
                    LEANCLR_CASE_BEGIN2(ConvR4R8)
                    {
                        double src = get_stack_value_at<double>(eval_stack_base, ir->src);
                        set_stack_value_at<float>(eval_stack_base, ir->dst, static_cast<float>(src));
                    }
                    LEANCLR_CASE_END2()
                    LEANCLR_CASE_BEGIN2(ConvR8I4)
                    {
                        int32_t src = get_stack_value_at<int32_t>(eval_stack_base, ir->src);
                        set_stack_value_at<double>(eval_stack_base, ir->dst, static_cast<double>(src));
                    }
                    LEANCLR_CASE_END2()
                    LEANCLR_CASE_BEGIN2(ConvR8I8)
                    {
                        int64_t src = get_stack_value_at<int64_t>(eval_stack_base, ir->src);
                        set_stack_value_at<double>(eval_stack_base, ir->dst, static_cast<double>(src));
                    }
                    LEANCLR_CASE_END2()
                    LEANCLR_CASE_BEGIN2(ConvR8R4)
                    {
                        float src = get_stack_value_at<float>(eval_stack_base, ir->src);
                        set_stack_value_at<double>(eval_stack_base, ir->dst, static_cast<double>(src));
                    }
                    LEANCLR_CASE_END2()
                    LEANCLR_CASE_BEGIN2(LdelemaReadOnly)
                    {
                        vm::RtArray* array = get_stack_value_at<vm::RtArray*>(eval_stack_base, ir->arr);
                        if (!array)
                        {
                            RAISE_RUNTIME_ERROR(RtErr::NullReference);
                        }
                        int32_t index = get_stack_value_at<int32_t>(eval_stack_base, ir->index);
                        if (vm::Array::is_out_of_range(array, index))
                        {
                            RAISE_RUNTIME_ERROR(RtErr::IndexOutOfRange);
                        }
                        const void* element_addr = vm::Array::get_array_element_address_as_ptr_void(array, index);
                        set_stack_value_at(eval_stack_base, ir->dst, element_addr);
                    }
                    LEANCLR_CASE_END2()
                    LEANCLR_CASE_BEGIN2(InitBlk)
                    {
                        void* dst = get_stack_value_at<void*>(eval_stack_base, ir->addr);
                        uint8_t value = get_stack_value_at<uint8_t>(eval_stack_base, ir->value);
                        uint32_t size = get_stack_value_at<uint32_t>(eval_stack_base, ir->size);
                        std::memset(dst, value, size);
                    }
                    LEANCLR_CASE_END2()
                    LEANCLR_CASE_BEGIN2(CpBlk)
                    {
                        void* dst = get_stack_value_at<void*>(eval_stack_base, ir->dst);
                        const void* src = get_stack_value_at<const void*>(eval_stack_base, ir->src);
                        uint32_t size = get_stack_value_at<uint32_t>(eval_stack_base, ir->size);
                        std::memcpy(dst, src, size);
                    }
                    LEANCLR_CASE_END2()
                    LEANCLR_CASE_BEGIN2(GetEnumLongHashCode)
                    {
                        int64_t value = get_ind_stack_value_at<int64_t>(eval_stack_base, ir->value_ptr);
                        int32_t hash = vm::Enum::get_enum_long_hash_code(value);
                        set_stack_value_at<int32_t>(eval_stack_base, ir->dst, hash);
                    }
                    LEANCLR_CASE_END2()
#if !LEANCLR_USE_COMPUTED_GOTO_DISPATCHER
                default:
                {
                    assert(false && "Invalid opcode");
                    RAISE_RUNTIME_ERROR(RtErr::ExecutionEngine);
                }
#endif
                }
            }
            LEANCLR_CASE_END_LITE0()
            LEANCLR_CASE_BEGIN_LITE0(Prefix3)
            {
                const uint8_t* ip3 = ip + 1;
                LEANCLR_SWITCH3()
                {
                    LEANCLR_CASE_BEGIN3(LdIndI2Unaligned)
                    {
                        const uint8_t* addr = get_stack_value_at<const uint8_t*>(eval_stack_base, ir->src);
                        int16_t value = utils::MemOp::read_i16_may_unaligned(addr);
                        set_stack_value_at<int32_t>(eval_stack_base, ir->dst, static_cast<int32_t>(value));
                    }

                    LEANCLR_CASE_END3()
                    LEANCLR_CASE_BEGIN3(LdIndU2Unaligned)
                    {
                        const uint8_t* addr = get_stack_value_at<const uint8_t*>(eval_stack_base, ir->src);
                        uint16_t value = utils::MemOp::read_u16_may_unaligned(addr);
                        set_stack_value_at<int32_t>(eval_stack_base, ir->dst, static_cast<int32_t>(value));
                    }

                    LEANCLR_CASE_END3()
                    LEANCLR_CASE_BEGIN3(LdIndI4Unaligned)
                    {
                        const uint8_t* addr = get_stack_value_at<const uint8_t*>(eval_stack_base, ir->src);
                        int32_t value = utils::MemOp::read_i32_may_unaligned(addr);
                        set_stack_value_at<int32_t>(eval_stack_base, ir->dst, value);
                    }

                    LEANCLR_CASE_END3()
                    LEANCLR_CASE_BEGIN3(LdIndI8Unaligned)
                    {
                        const uint8_t* addr = get_stack_value_at<const uint8_t*>(eval_stack_base, ir->src);
                        int64_t value = utils::MemOp::read_i64_may_unaligned(addr);
                        set_stack_value_at<int64_t>(eval_stack_base, ir->dst, value);
                    }

                    LEANCLR_CASE_END3()
                    LEANCLR_CASE_BEGIN3(StIndI2Unaligned)
                    {
                        uint8_t* addr = get_stack_value_at<uint8_t*>(eval_stack_base, ir->dst);
                        int16_t value = get_stack_value_at<int16_t>(eval_stack_base, ir->src);
                        utils::MemOp::write_i16_may_unaligned(addr, value);
                    }

                    LEANCLR_CASE_END3()
                    LEANCLR_CASE_BEGIN3(StIndI4Unaligned)
                    {
                        uint8_t* addr = get_stack_value_at<uint8_t*>(eval_stack_base, ir->dst);
                        int32_t value = get_stack_value_at<int32_t>(eval_stack_base, ir->src);
                        utils::MemOp::write_i32_may_unaligned(addr, value);
                    }

                    LEANCLR_CASE_END3()
                    LEANCLR_CASE_BEGIN3(StIndI8Unaligned)
                    {
                        uint8_t* addr = get_stack_value_at<uint8_t*>(eval_stack_base, ir->dst);
                        int64_t value = get_stack_value_at<int64_t>(eval_stack_base, ir->src);
                        utils::MemOp::write_i64_may_unaligned(addr, value);
                    }

                    LEANCLR_CASE_END3()
                    LEANCLR_CASE_BEGIN3(AddOvfI4)
                    {
                        int32_t left = get_stack_value_at<int32_t>(eval_stack_base, ir->arg1);
                        int32_t right = get_stack_value_at<int32_t>(eval_stack_base, ir->arg2);
                        int32_t result;
                        if (CHECK_ADD_OVERFLOW_I32(left, right, &result))
                        {
                            RAISE_RUNTIME_ERROR(RtErr::Overflow);
                        }
                        set_stack_value_at<int32_t>(eval_stack_base, ir->dst, result);
                    }

                    LEANCLR_CASE_END3()
                    LEANCLR_CASE_BEGIN3(AddOvfI8)
                    {
                        int64_t left = get_stack_value_at<int64_t>(eval_stack_base, ir->arg1);
                        int64_t right = get_stack_value_at<int64_t>(eval_stack_base, ir->arg2);
                        int64_t result;
                        if (CHECK_ADD_OVERFLOW_I64(left, right, &result))
                        {
                            RAISE_RUNTIME_ERROR(RtErr::Overflow);
                        }
                        set_stack_value_at<int64_t>(eval_stack_base, ir->dst, result);
                    }

                    LEANCLR_CASE_END3()
                    LEANCLR_CASE_BEGIN3(AddOvfUnI4)
                    {
                        uint32_t left = get_stack_value_at<uint32_t>(eval_stack_base, ir->arg1);
                        uint32_t right = get_stack_value_at<uint32_t>(eval_stack_base, ir->arg2);
                        uint32_t result;
                        if (CHECK_ADD_OVERFLOW_U32(left, right, &result))
                        {
                            RAISE_RUNTIME_ERROR(RtErr::Overflow);
                        }
                        set_stack_value_at<uint32_t>(eval_stack_base, ir->dst, result);
                    }

                    LEANCLR_CASE_END3()
                    LEANCLR_CASE_BEGIN3(AddOvfUnI8)
                    {
                        uint64_t left = get_stack_value_at<uint64_t>(eval_stack_base, ir->arg1);
                        uint64_t right = get_stack_value_at<uint64_t>(eval_stack_base, ir->arg2);
                        uint64_t result;
                        if (CHECK_ADD_OVERFLOW_U64(left, right, &result))
                        {
                            RAISE_RUNTIME_ERROR(RtErr::Overflow);
                        }
                        set_stack_value_at<uint64_t>(eval_stack_base, ir->dst, result);
                    }

                    LEANCLR_CASE_END3()
                    LEANCLR_CASE_BEGIN3(MulOvfI4)
                    {
                        int32_t left = get_stack_value_at<int32_t>(eval_stack_base, ir->arg1);
                        int32_t right = get_stack_value_at<int32_t>(eval_stack_base, ir->arg2);
                        int32_t result;
                        if (CHECK_MUL_OVERFLOW_I32(left, right, &result))
                        {
                            RAISE_RUNTIME_ERROR(RtErr::Overflow);
                        }
                        set_stack_value_at<int32_t>(eval_stack_base, ir->dst, result);
                    }

                    LEANCLR_CASE_END3()
                    LEANCLR_CASE_BEGIN3(MulOvfI8)
                    {
                        int64_t left = get_stack_value_at<int64_t>(eval_stack_base, ir->arg1);
                        int64_t right = get_stack_value_at<int64_t>(eval_stack_base, ir->arg2);
                        int64_t result;
                        if (CHECK_MUL_OVERFLOW_I64(left, right, &result))
                        {
                            RAISE_RUNTIME_ERROR(RtErr::Overflow);
                        }
                        set_stack_value_at<int64_t>(eval_stack_base, ir->dst, result);
                    }

                    LEANCLR_CASE_END3()
                    LEANCLR_CASE_BEGIN3(MulOvfUnI4)
                    {
                        uint32_t left = get_stack_value_at<uint32_t>(eval_stack_base, ir->arg1);
                        uint32_t right = get_stack_value_at<uint32_t>(eval_stack_base, ir->arg2);
                        uint32_t result;
                        if (CHECK_MUL_OVERFLOW_U32(left, right, &result))
                        {
                            RAISE_RUNTIME_ERROR(RtErr::Overflow);
                        }
                        set_stack_value_at<uint32_t>(eval_stack_base, ir->dst, result);
                    }

                    LEANCLR_CASE_END3()
                    LEANCLR_CASE_BEGIN3(MulOvfUnI8)
                    {
                        uint64_t left = get_stack_value_at<uint64_t>(eval_stack_base, ir->arg1);
                        uint64_t right = get_stack_value_at<uint64_t>(eval_stack_base, ir->arg2);
                        uint64_t result;
                        if (CHECK_MUL_OVERFLOW_U64(left, right, &result))
                        {
                            RAISE_RUNTIME_ERROR(RtErr::Overflow);
                        }
                        set_stack_value_at<uint64_t>(eval_stack_base, ir->dst, result);
                    }

                    LEANCLR_CASE_END3()
                    LEANCLR_CASE_BEGIN3(SubOvfI4)
                    {
                        int32_t left = get_stack_value_at<int32_t>(eval_stack_base, ir->arg1);
                        int32_t right = get_stack_value_at<int32_t>(eval_stack_base, ir->arg2);
                        int32_t result;
                        if (CHECK_SUB_OVERFLOW_I32(left, right, &result))
                        {
                            RAISE_RUNTIME_ERROR(RtErr::Overflow);
                        }
                        set_stack_value_at<int32_t>(eval_stack_base, ir->dst, result);
                    }

                    LEANCLR_CASE_END3()
                    LEANCLR_CASE_BEGIN3(SubOvfI8)
                    {
                        int64_t left = get_stack_value_at<int64_t>(eval_stack_base, ir->arg1);
                        int64_t right = get_stack_value_at<int64_t>(eval_stack_base, ir->arg2);
                        int64_t result;
                        if (CHECK_SUB_OVERFLOW_I64(left, right, &result))
                        {
                            RAISE_RUNTIME_ERROR(RtErr::Overflow);
                        }
                        set_stack_value_at<int64_t>(eval_stack_base, ir->dst, result);
                    }

                    LEANCLR_CASE_END3()
                    LEANCLR_CASE_BEGIN3(SubOvfUnI4)
                    {
                        uint32_t left = get_stack_value_at<uint32_t>(eval_stack_base, ir->arg1);
                        uint32_t right = get_stack_value_at<uint32_t>(eval_stack_base, ir->arg2);
                        uint32_t result;
                        if (CHECK_SUB_OVERFLOW_U32(left, right, &result))
                        {
                            RAISE_RUNTIME_ERROR(RtErr::Overflow);
                        }
                        set_stack_value_at<uint32_t>(eval_stack_base, ir->dst, result);
                    }

                    LEANCLR_CASE_END3()
                    LEANCLR_CASE_BEGIN3(SubOvfUnI8)
                    {
                        uint64_t left = get_stack_value_at<uint64_t>(eval_stack_base, ir->arg1);
                        uint64_t right = get_stack_value_at<uint64_t>(eval_stack_base, ir->arg2);
                        uint64_t result;
                        if (CHECK_SUB_OVERFLOW_U64(left, right, &result))
                        {
                            RAISE_RUNTIME_ERROR(RtErr::Overflow);
                        }
                        set_stack_value_at<uint64_t>(eval_stack_base, ir->dst, result);
                    }

                    LEANCLR_CASE_END3()
                    LEANCLR_CASE_BEGIN3(InitObjI2Unaligned)
                    {
                        uint8_t* addr = get_stack_value_at<uint8_t*>(eval_stack_base, ir->addr);
                        utils::MemOp::write_i16_may_unaligned(addr, 0);
                    }

                    LEANCLR_CASE_END3()
                    LEANCLR_CASE_BEGIN3(InitObjI4Unaligned)
                    {
                        uint8_t* addr = get_stack_value_at<uint8_t*>(eval_stack_base, ir->addr);
                        utils::MemOp::write_i32_may_unaligned(addr, 0);
                    }

                    LEANCLR_CASE_END3()
                    LEANCLR_CASE_BEGIN3(InitObjI8Unaligned)
                    {
                        uint8_t* addr = get_stack_value_at<uint8_t*>(eval_stack_base, ir->addr);
                        utils::MemOp::write_i64_may_unaligned(addr, 0);
                    }

                    LEANCLR_CASE_END3()
                    LEANCLR_CASE_BEGIN3(LdfldI1Large)
                    {
                        vm::RtObject* obj = get_stack_value_at<vm::RtObject*>(eval_stack_base, ir->obj);
                        if (!obj)
                        {
                            RAISE_RUNTIME_ERROR(RtErr::NullReference);
                        }
                        const int8_t* field_addr = reinterpret_cast<const int8_t*>(reinterpret_cast<const uint8_t*>(obj) + ir->offset);
                        int8_t value = *field_addr;
                        set_stack_value_at<int32_t>(eval_stack_base, ir->dst, static_cast<int32_t>(value));
                    }

                    LEANCLR_CASE_END3()
                    LEANCLR_CASE_BEGIN3(LdfldU1Large)
                    {
                        vm::RtObject* obj = get_stack_value_at<vm::RtObject*>(eval_stack_base, ir->obj);
                        if (!obj)
                        {
                            RAISE_RUNTIME_ERROR(RtErr::NullReference);
                        }
                        const uint8_t* field_addr = reinterpret_cast<const uint8_t*>(obj) + ir->offset;
                        uint8_t value = *field_addr;
                        set_stack_value_at<int32_t>(eval_stack_base, ir->dst, static_cast<int32_t>(value));
                    }

                    LEANCLR_CASE_END3()
                    LEANCLR_CASE_BEGIN3(LdfldI2Large)
                    {
                        vm::RtObject* obj = get_stack_value_at<vm::RtObject*>(eval_stack_base, ir->obj);
                        if (!obj)
                        {
                            RAISE_RUNTIME_ERROR(RtErr::NullReference);
                        }
                        const int16_t* field_addr = reinterpret_cast<const int16_t*>(reinterpret_cast<const uint8_t*>(obj) + ir->offset);
                        int16_t value = *field_addr;
                        set_stack_value_at<int32_t>(eval_stack_base, ir->dst, static_cast<int32_t>(value));
                    }

                    LEANCLR_CASE_END3()
                    LEANCLR_CASE_BEGIN3(LdfldI2Unaligned)
                    {
                        vm::RtObject* obj = get_stack_value_at<vm::RtObject*>(eval_stack_base, ir->obj);
                        if (!obj)
                        {
                            RAISE_RUNTIME_ERROR(RtErr::NullReference);
                        }
                        const uint8_t* field_addr = reinterpret_cast<const uint8_t*>(obj) + ir->offset;
                        int16_t value = utils::MemOp::read_i16_may_unaligned(field_addr);
                        set_stack_value_at<int32_t>(eval_stack_base, ir->dst, static_cast<int32_t>(value));
                    }

                    LEANCLR_CASE_END3()
                    LEANCLR_CASE_BEGIN3(LdfldU2Large)
                    {
                        vm::RtObject* obj = get_stack_value_at<vm::RtObject*>(eval_stack_base, ir->obj);
                        if (!obj)
                        {
                            RAISE_RUNTIME_ERROR(RtErr::NullReference);
                        }
                        const uint16_t* field_addr = reinterpret_cast<const uint16_t*>(reinterpret_cast<const uint8_t*>(obj) + ir->offset);
                        uint16_t value = *field_addr;
                        set_stack_value_at<int32_t>(eval_stack_base, ir->dst, static_cast<int32_t>(value));
                    }

                    LEANCLR_CASE_END3()
                    LEANCLR_CASE_BEGIN3(LdfldU2Unaligned)
                    {
                        vm::RtObject* obj = get_stack_value_at<vm::RtObject*>(eval_stack_base, ir->obj);
                        if (!obj)
                        {
                            RAISE_RUNTIME_ERROR(RtErr::NullReference);
                        }
                        const uint8_t* field_addr = reinterpret_cast<const uint8_t*>(obj) + ir->offset;
                        uint16_t value = utils::MemOp::read_u16_may_unaligned(field_addr);
                        set_stack_value_at<int32_t>(eval_stack_base, ir->dst, static_cast<int32_t>(value));
                    }

                    LEANCLR_CASE_END3()
                    LEANCLR_CASE_BEGIN3(LdfldI4Large)
                    {
                        vm::RtObject* obj = get_stack_value_at<vm::RtObject*>(eval_stack_base, ir->obj);
                        if (!obj)
                        {
                            RAISE_RUNTIME_ERROR(RtErr::NullReference);
                        }
                        const int32_t* field_addr = reinterpret_cast<const int32_t*>(reinterpret_cast<const uint8_t*>(obj) + ir->offset);
                        int32_t value = *field_addr;
                        set_stack_value_at<int32_t>(eval_stack_base, ir->dst, value);
                    }

                    LEANCLR_CASE_END3()
                    LEANCLR_CASE_BEGIN3(LdfldI4Unaligned)
                    {
                        vm::RtObject* obj = get_stack_value_at<vm::RtObject*>(eval_stack_base, ir->obj);
                        if (!obj)
                        {
                            RAISE_RUNTIME_ERROR(RtErr::NullReference);
                        }
                        const uint8_t* field_addr = reinterpret_cast<const uint8_t*>(obj) + ir->offset;
                        int32_t value = utils::MemOp::read_i32_may_unaligned(field_addr);
                        set_stack_value_at<int32_t>(eval_stack_base, ir->dst, value);
                    }

                    LEANCLR_CASE_END3()
                    LEANCLR_CASE_BEGIN3(LdfldI8Large)
                    {
                        vm::RtObject* obj = get_stack_value_at<vm::RtObject*>(eval_stack_base, ir->obj);
                        if (!obj)
                        {
                            RAISE_RUNTIME_ERROR(RtErr::NullReference);
                        }
                        const int64_t* field_addr = reinterpret_cast<const int64_t*>(reinterpret_cast<const uint8_t*>(obj) + ir->offset);
                        int64_t value = *field_addr;
                        set_stack_value_at<int64_t>(eval_stack_base, ir->dst, value);
                    }

                    LEANCLR_CASE_END3()
                    LEANCLR_CASE_BEGIN3(LdfldI8Unaligned)
                    {
                        vm::RtObject* obj = get_stack_value_at<vm::RtObject*>(eval_stack_base, ir->obj);
                        if (!obj)
                        {
                            RAISE_RUNTIME_ERROR(RtErr::NullReference);
                        }
                        const uint8_t* field_addr = reinterpret_cast<const uint8_t*>(obj) + ir->offset;
                        int64_t value = utils::MemOp::read_i64_may_unaligned(field_addr);
                        set_stack_value_at<int64_t>(eval_stack_base, ir->dst, value);
                    }

                    LEANCLR_CASE_END3()
                    LEANCLR_CASE_BEGIN3(LdfldAnyLarge)
                    {
                        vm::RtObject* obj = get_stack_value_at<vm::RtObject*>(eval_stack_base, ir->obj);
                        if (!obj)
                        {
                            RAISE_RUNTIME_ERROR(RtErr::NullReference);
                        }
                        const uint8_t* src_addr = reinterpret_cast<const uint8_t*>(obj) + ir->offset;
                        RtStackObject* dst = eval_stack_base + ir->dst;
                        std::memcpy(dst, src_addr, ir->size);
                    }

                    LEANCLR_CASE_END3()
                    LEANCLR_CASE_BEGIN3(LdvfldI1Large)
                    {
                        const uint8_t* src_addr = reinterpret_cast<const uint8_t*>(eval_stack_base + ir->obj) + ir->offset;
                        int8_t value = *reinterpret_cast<const int8_t*>(src_addr);
                        set_stack_value_at<int32_t>(eval_stack_base, ir->dst, static_cast<int32_t>(value));
                    }

                    LEANCLR_CASE_END3()
                    LEANCLR_CASE_BEGIN3(LdvfldU1Large)
                    {
                        const uint8_t* src_addr = reinterpret_cast<const uint8_t*>(eval_stack_base + ir->obj) + ir->offset;
                        uint8_t value = *reinterpret_cast<const uint8_t*>(src_addr);
                        set_stack_value_at<int32_t>(eval_stack_base, ir->dst, static_cast<int32_t>(value));
                    }

                    LEANCLR_CASE_END3()
                    LEANCLR_CASE_BEGIN3(LdvfldI2Large)
                    {
                        const uint8_t* src_addr = reinterpret_cast<const uint8_t*>(eval_stack_base + ir->obj) + ir->offset;
                        int16_t value = *reinterpret_cast<const int16_t*>(src_addr);
                        set_stack_value_at<int32_t>(eval_stack_base, ir->dst, static_cast<int32_t>(value));
                    }

                    LEANCLR_CASE_END3()
                    LEANCLR_CASE_BEGIN3(LdvfldI2Unaligned)
                    {
                        const uint8_t* src_addr = reinterpret_cast<const uint8_t*>(eval_stack_base + ir->obj) + ir->offset;
                        int16_t value;
                        std::memcpy(&value, src_addr, sizeof(value));
                        set_stack_value_at<int32_t>(eval_stack_base, ir->dst, static_cast<int32_t>(value));
                    }

                    LEANCLR_CASE_END3()
                    LEANCLR_CASE_BEGIN3(LdvfldU2Large)
                    {
                        const uint8_t* src_addr = reinterpret_cast<const uint8_t*>(eval_stack_base + ir->obj) + ir->offset;
                        uint16_t value = *reinterpret_cast<const uint16_t*>(src_addr);
                        set_stack_value_at<int32_t>(eval_stack_base, ir->dst, static_cast<int32_t>(value));
                    }

                    LEANCLR_CASE_END3()
                    LEANCLR_CASE_BEGIN3(LdvfldU2Unaligned)
                    {
                        const uint8_t* src_addr = reinterpret_cast<const uint8_t*>(eval_stack_base + ir->obj) + ir->offset;
                        uint16_t value = utils::MemOp::read_u16_may_unaligned(src_addr);
                        set_stack_value_at<int32_t>(eval_stack_base, ir->dst, static_cast<int32_t>(value));
                    }

                    LEANCLR_CASE_END3()
                    LEANCLR_CASE_BEGIN3(LdvfldI4Large)
                    {
                        const uint8_t* src_addr = reinterpret_cast<const uint8_t*>(eval_stack_base + ir->obj) + ir->offset;
                        int32_t value = *reinterpret_cast<const int32_t*>(src_addr);
                        set_stack_value_at<int32_t>(eval_stack_base, ir->dst, value);
                    }

                    LEANCLR_CASE_END3()
                    LEANCLR_CASE_BEGIN3(LdvfldI4Unaligned)
                    {
                        const uint8_t* src_addr = reinterpret_cast<const uint8_t*>(eval_stack_base + ir->obj) + ir->offset;
                        int32_t value = utils::MemOp::read_i32_may_unaligned(src_addr);
                        set_stack_value_at<int32_t>(eval_stack_base, ir->dst, value);
                    }

                    LEANCLR_CASE_END3()
                    LEANCLR_CASE_BEGIN3(LdvfldI8Large)
                    {
                        const uint8_t* src_addr = reinterpret_cast<const uint8_t*>(eval_stack_base + ir->obj) + ir->offset;
                        int64_t value = *reinterpret_cast<const int64_t*>(src_addr);
                        set_stack_value_at<int64_t>(eval_stack_base, ir->dst, value);
                    }

                    LEANCLR_CASE_END3()
                    LEANCLR_CASE_BEGIN3(LdvfldI8Unaligned)
                    {
                        const uint8_t* src_addr = reinterpret_cast<const uint8_t*>(eval_stack_base + ir->obj) + ir->offset;
                        int64_t value = utils::MemOp::read_i64_may_unaligned(src_addr);
                        set_stack_value_at<int64_t>(eval_stack_base, ir->dst, value);
                    }

                    LEANCLR_CASE_END3()
                    LEANCLR_CASE_BEGIN3(LdvfldAnyLarge)
                    {
                        const uint8_t* src_addr = reinterpret_cast<const uint8_t*>(eval_stack_base + ir->obj) + ir->offset;
                        RtStackObject* dst = eval_stack_base + ir->dst;
                        // may overlap
                        std::memmove(dst, src_addr, ir->size);
                    }

                    LEANCLR_CASE_END3()
                    LEANCLR_CASE_BEGIN3(LdfldaLarge)
                    {
                        vm::RtObject* obj = get_stack_value_at<vm::RtObject*>(eval_stack_base, ir->obj);
                        if (!obj)
                        {
                            RAISE_RUNTIME_ERROR(RtErr::NullReference);
                        }
                        const uint8_t* field_addr = reinterpret_cast<const uint8_t*>(obj) + ir->offset;
                        set_stack_value_at(eval_stack_base, ir->dst, field_addr);
                    }

                    LEANCLR_CASE_END3()
                    LEANCLR_CASE_BEGIN3(StfldI1Large)
                    {
                        vm::RtObject* obj = get_stack_value_at<vm::RtObject*>(eval_stack_base, ir->obj);
                        if (!obj)
                        {
                            RAISE_RUNTIME_ERROR(RtErr::NullReference);
                        }
                        int8_t value = get_stack_value_at<int8_t>(eval_stack_base, ir->value);
                        int8_t* field_addr = reinterpret_cast<int8_t*>(reinterpret_cast<uint8_t*>(obj) + ir->offset);
                        *field_addr = value;
                    }

                    LEANCLR_CASE_END3()
                    LEANCLR_CASE_BEGIN3(StfldI2Large)
                    {
                        vm::RtObject* obj = get_stack_value_at<vm::RtObject*>(eval_stack_base, ir->obj);
                        if (!obj)
                        {
                            RAISE_RUNTIME_ERROR(RtErr::NullReference);
                        }
                        int16_t value = get_stack_value_at<int16_t>(eval_stack_base, ir->value);
                        int16_t* field_addr = reinterpret_cast<int16_t*>(reinterpret_cast<uint8_t*>(obj) + ir->offset);
                        *field_addr = value;
                    }

                    LEANCLR_CASE_END3()
                    LEANCLR_CASE_BEGIN3(StfldI2Unaligned)
                    {
                        vm::RtObject* obj = get_stack_value_at<vm::RtObject*>(eval_stack_base, ir->obj);
                        if (!obj)
                        {
                            RAISE_RUNTIME_ERROR(RtErr::NullReference);
                        }
                        int16_t value = get_stack_value_at<int16_t>(eval_stack_base, ir->value);
                        uint8_t* field_addr = reinterpret_cast<uint8_t*>(obj) + ir->offset;
                        utils::MemOp::write_i16_may_unaligned(field_addr, value);
                    }

                    LEANCLR_CASE_END3()
                    LEANCLR_CASE_BEGIN3(StfldI4Large)
                    {
                        vm::RtObject* obj = get_stack_value_at<vm::RtObject*>(eval_stack_base, ir->obj);
                        if (!obj)
                        {
                            RAISE_RUNTIME_ERROR(RtErr::NullReference);
                        }
                        int32_t value = get_stack_value_at<int32_t>(eval_stack_base, ir->value);
                        int32_t* field_addr = reinterpret_cast<int32_t*>(reinterpret_cast<uint8_t*>(obj) + ir->offset);
                        *field_addr = value;
                    }

                    LEANCLR_CASE_END3()
                    LEANCLR_CASE_BEGIN3(StfldI4Unaligned)
                    {
                        vm::RtObject* obj = get_stack_value_at<vm::RtObject*>(eval_stack_base, ir->obj);
                        if (!obj)
                        {
                            RAISE_RUNTIME_ERROR(RtErr::NullReference);
                        }
                        int32_t value = get_stack_value_at<int32_t>(eval_stack_base, ir->value);
                        uint8_t* field_addr = reinterpret_cast<uint8_t*>(obj) + ir->offset;
                        utils::MemOp::write_i32_may_unaligned(field_addr, value);
                    }

                    LEANCLR_CASE_END3()
                    LEANCLR_CASE_BEGIN3(StfldI8Large)
                    {
                        vm::RtObject* obj = get_stack_value_at<vm::RtObject*>(eval_stack_base, ir->obj);
                        if (!obj)
                        {
                            RAISE_RUNTIME_ERROR(RtErr::NullReference);
                        }
                        int64_t value = get_stack_value_at<int64_t>(eval_stack_base, ir->value);
                        int64_t* field_addr = reinterpret_cast<int64_t*>(reinterpret_cast<uint8_t*>(obj) + ir->offset);
                        *field_addr = value;
                    }

                    LEANCLR_CASE_END3()
                    LEANCLR_CASE_BEGIN3(StfldI8Unaligned)
                    {
                        vm::RtObject* obj = get_stack_value_at<vm::RtObject*>(eval_stack_base, ir->obj);
                        if (!obj)
                        {
                            RAISE_RUNTIME_ERROR(RtErr::NullReference);
                        }
                        int64_t value = get_stack_value_at<int64_t>(eval_stack_base, ir->value);
                        uint8_t* field_addr = reinterpret_cast<uint8_t*>(obj) + ir->offset;
                        utils::MemOp::write_i64_may_unaligned(field_addr, value);
                    }

                    LEANCLR_CASE_END3()
                    LEANCLR_CASE_BEGIN3(StfldAnyLarge)
                    {
                        vm::RtObject* obj = get_stack_value_at<vm::RtObject*>(eval_stack_base, ir->obj);
                        if (!obj)
                        {
                            RAISE_RUNTIME_ERROR(RtErr::NullReference);
                        }
                        const uint8_t* src_addr = reinterpret_cast<const uint8_t*>(eval_stack_base + ir->value);
                        uint8_t* dst_addr = reinterpret_cast<uint8_t*>(obj) + ir->offset;
                        std::memmove(dst_addr, src_addr, ir->size);
                    }

                    LEANCLR_CASE_END3()
#if !LEANCLR_USE_COMPUTED_GOTO_DISPATCHER
                default:
                {
                    assert(false && "Invalid opcode");
                    RAISE_RUNTIME_ERROR(RtErr::ExecutionEngine);
                }
#endif
                }
            }
            LEANCLR_CASE_END_LITE0()
            LEANCLR_CASE_BEGIN_LITE0(Prefix4)
            {
                const uint8_t* ip4 = ip + 1;

                LEANCLR_SWITCH4()
                {
                    LEANCLR_CASE_BEGIN4(Illegal)
                    {
                        assert(false && "never reach here");
                        RAISE_RUNTIME_ERROR(RtErr::ExecutionEngine);
                    }
                    LEANCLR_CASE_END4()
                    LEANCLR_CASE_BEGIN4(Nop)
                    {
                        // do nothing
                    }
                    LEANCLR_CASE_END4()
                    LEANCLR_CASE_BEGIN4(Arglist)
                    {
                        assert(false && "Not implemented");
                        RAISE_RUNTIME_ERROR(RtErr::ExecutionEngine);
                    }
                    LEANCLR_CASE_END4()
#if !LEANCLR_USE_COMPUTED_GOTO_DISPATCHER
                default:
                {
                    assert(false && "Invalid opcode");
                    RAISE_RUNTIME_ERROR(RtErr::ExecutionEngine);
                }
#endif
                }
            }
            LEANCLR_CASE_END_LITE0()
            LEANCLR_CASE_BEGIN_LITE0(Prefix5)
            {
                const uint8_t* ip5 = ip + 1;
                assert(false && "Not implemented");
                RAISE_RUNTIME_ERROR(RtErr::ExecutionEngine);
            }
            LEANCLR_CASE_END_LITE0()
#if !LEANCLR_USE_COMPUTED_GOTO_DISPATCHER
        default:
        {
            assert(false && "Invalid opcode");
            RAISE_RUNTIME_ERROR(RtErr::ExecutionEngine);
        }
#endif
        }
    }
}
unwind_exception_handler:

    while (true)
    {
        RtStackObject* const eval_stack_base = frame->eval_stack_base;
        const RtInterpMethodInfo* const imi = frame->method->interp_data;
        ExceptionFlow* cur_flow = peek_top_exception_flow();
        assert(cur_flow);
        const RtInterpExceptionClause* clauses = imi->exception_clauses;
        size_t clause_count = imi->exception_clause_count;
        if (cur_flow->throw_flow)
        {
            auto& data = cur_flow->throw_data;
            vm::RtException* ex = data.ex;
            uint32_t throw_ip_offset = static_cast<uint32_t>(reinterpret_cast<const uint8_t*>(data.ip) - imi->codes);
            bool handled = false;
            for (size_t i = data.next_search_clause_idx; i < clause_count; ++i)
            {
                data.next_search_clause_idx = static_cast<size_t>(i + 1);
                const RtInterpExceptionClause* clause = &clauses[i];
                if (!clause->is_in_try_block(throw_ip_offset))
                {
                    continue;
                }

                switch (clause->flags)
                {
                case metadata::RtILExceptionClauseType::Exception:
                {
                    if (!clause->ex_klass || vm::Class::is_assignable_from(ex->klass, clause->ex_klass))
                    {
                        ip = imi->codes + clause->handler_begin_offset;
                        setup_catch_handler(imi, frame, clause, ip);
                        set_stack_value_at<vm::RtObject*>(eval_stack_base, imi->total_arg_and_local_stack_object_size, ex);
                        handled = true;
                    }
                    break;
                }
                case metadata::RtILExceptionClauseType::Filter:
                {
                    ip = imi->codes + clause->handler_begin_offset;
                    setup_filter_checker(clause);
                    set_stack_value_at<vm::RtObject*>(eval_stack_base, imi->total_arg_and_local_stack_object_size, ex);
                    handled = true;
                    break;
                }
                case metadata::RtILExceptionClauseType::Finally:
                case metadata::RtILExceptionClauseType::Fault:
                {
                    ip = imi->codes + clause->handler_begin_offset;
                    setup_finally_or_fault_handler(imi, clause, ip);
                    set_stack_value_at<vm::RtObject*>(eval_stack_base, imi->total_arg_and_local_stack_object_size, ex);
                    handled = true;
                    break;
                }
                }
                if (handled)
                {
                    break;
                }
            }
            if (!handled)
            {
                pop_all_flow_of_cur_frame_exclude_last(frame);
                frame = ms.leave_frame(sp, frame);
                if (!frame)
                {
                    vm::Exception::set_current_exception(ex);
                    RET_ERR(RtErr::ManagedException);
                }
                push_throw_flow(ex, frame, frame->ip);
                // unwind to previous frame to continue searching
                continue;
            }
        }
        else
        {
            auto& data = cur_flow->leave_data;
            if (data.remain_finally_clause_count == 0)
            {
                ip = reinterpret_cast<const uint8_t*>(data.target_ip);
                pop_leave_flow(frame);
            }
            else
            {
                uint32_t src_ip_offset = static_cast<uint32_t>(reinterpret_cast<const uint8_t*>(data.src_ip) - imi->codes);
                uint32_t target_ip_offset = static_cast<uint32_t>(reinterpret_cast<const uint8_t*>(data.target_ip) - imi->codes);
                const RtInterpExceptionClause* next_finally_clause = nullptr;
                for (size_t i = data.next_search_clause_idx; i < clause_count; ++i)
                {
                    const RtInterpExceptionClause* clause = &clauses[i];
                    if (clause->try_begin_offset <= src_ip_offset && src_ip_offset < clause->try_end_offset)
                    {
                        data.next_search_clause_idx = static_cast<uint32_t>(i + 1);
                        if (clause->flags == metadata::RtILExceptionClauseType::Finally && clause->is_in_try_block(src_ip_offset) &&
                            !clause->is_in_try_block(target_ip_offset))
                        {
                            next_finally_clause = clause;
                            break;
                        }
                    }
                }
                assert(next_finally_clause);
                data.remain_finally_clause_count--;
                ip = imi->codes + next_finally_clause->handler_begin_offset;
            }
        }
        goto method_start;
    }
end_loop:
    RET_OK(ret);
}
} // namespace leanclr::interp
