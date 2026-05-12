#ifndef CORA_FFI_HPP
#define CORA_FFI_HPP

#include "Cora/Detail/Common.hpp"
#include "Runtime/Value.hpp"
#include "Builtin/Builtin.hpp"
#include "Builtin/Module.hpp"
#include "Builtin/Class.hpp"

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <type_traits>
#include <stdexcept>

namespace cora::ffi {

    using namespace cora::compiler;

    // --- Type Casters ---

    template <typename T, typename Enable = void>
    struct type_caster;

    template <typename T>
    struct type_caster<T, std::enable_if_t<std::is_integral_v<T> && !std::is_same_v<T, bool>>> {
        static T cast(const runtime::value& v) { return static_cast<T>(v.AsNumber()); }
        static runtime::value cast(T v) { return runtime::value(static_cast<double>(v)); }
    };

    template <typename T>
    struct type_caster<T, std::enable_if_t<std::is_floating_point_v<T>>> {
        static T cast(const runtime::value& v) { return static_cast<T>(v.AsNumber()); }
        static runtime::value cast(T v) { return runtime::value(static_cast<double>(v)); }
    };

    template <> struct type_caster<std::string> {
        static std::string cast(const runtime::value& v) { return v.AsString(); }
        static runtime::value cast(const std::string& v) { return runtime::value(v); }
    };

    template <> struct type_caster<const char*> {
        static const char* cast(const runtime::value& v) { 
            static std::string s; // Warning: Not thread safe, but simple for now
            s = v.AsString();
            return s.c_str(); 
        }
        static runtime::value cast(const char* v) { return runtime::value(v); }
    };

    template <> struct type_caster<bool> {
        static bool cast(const runtime::value& v) { return v.AsBool(); }
        static runtime::value cast(bool v) { return runtime::value(v); }
    };

    template <typename T>
    struct type_caster<T*> {
        static T* cast(const runtime::value& v) { 
            if (v.IsPointer()) return static_cast<T*>(v.AsPointer());
            if (v.IsObject()) {
                auto obj = v.AsObject();
                auto it = obj->fields.find("__ptr__");
                if (it != obj->fields.end()) return static_cast<T*>(it->second.AsPointer());
            }
            throw std::runtime_error("FFI: expected pointer value");
        }
        static runtime::value cast(T* v) { return runtime::value(static_cast<void*>(v)); }
    };

    template <> struct type_caster<runtime::value> {
        static runtime::value cast(const runtime::value& v) { return v; }
    };

    // --- Cora Function Wrapper (Calling Cora from C++) ---

    class function {
    public:
        function() = default;
        function(std::shared_ptr<runtime::Callable> c) : m_callable(c) {}
        function(const runtime::value& v) {
            if (v.IsCallable()) m_callable = v.AsCallable();
        }

        template <typename... Args>
        runtime::value operator()(Args&&... args) const {
            if (!m_callable) throw std::runtime_error("FFI: attempting to call null function");
            std::vector<runtime::value> vargs;
            (vargs.push_back(type_caster<std::decay_t<Args>>::cast(std::forward<Args>(args))), ...);
            return m_callable->Call(vargs);
        }

        explicit operator bool() const { return static_cast<bool>(m_callable); }

    private:
        std::shared_ptr<runtime::Callable> m_callable;
    };

    inline function get_global(runtime::Scope& scope, const std::string& name) {
        auto var = scope.GetVariable(name);
        if (var && var->GetValue() && var->GetValue()->IsCallable()) {
            return function(var->GetValue()->AsCallable());
        }
        return function();
    }

    // --- Function Wrapping (Calling C++ from Cora) ---

    template <typename Ret, typename... Args, std::size_t... Is>
    Ret call_helper(const std::function<Ret(Args...)>& f, const std::vector<runtime::value>& args, std::index_sequence<Is...>) {
        return f(type_caster<std::decay_t<Args>>::cast(args[Is])...);
    }

    template <typename Ret, typename... Args>
    runtime::Function::Func wrap_function(Ret (*f)(Args...)) {
        return [f](const std::vector<runtime::value>& args) -> runtime::value {
            if (args.size() < sizeof...(Args)) {
                throw std::runtime_error("FFI: too few arguments");
            }
            if constexpr (std::is_void_v<Ret>) {
                call_helper(std::function<Ret(Args...)>(f), args, std::index_sequence_for<Args...>{});
                return runtime::value(nullptr);
            } else {
                return type_caster<std::decay_t<Ret>>::cast(call_helper(std::function<Ret(Args...)>(f), args, std::index_sequence_for<Args...>{}));
            }
        };
    }

    // Member function wrapper
    template <typename Class, typename Ret, typename... Args, std::size_t... Is>
    Ret call_member_helper(Ret (Class::*f)(Args...), Class* obj, const std::vector<runtime::value>& args, std::index_sequence<Is...>) {
        return (obj->*f)(type_caster<std::decay_t<Args>>::cast(args[Is])...);
    }

    template <typename Class, typename Ret, typename... Args>
    runtime::Function::Func wrap_member_function(Ret (Class::*f)(Args...)) {
        return [f](const std::vector<runtime::value>& args) -> runtime::value {
            if (args.empty()) throw std::runtime_error("FFI: member function call missing 'this'");
            Class* obj = type_caster<Class*>::cast(args[0]);
            std::vector<runtime::value> rest_args(args.begin() + 1, args.end());
            if (rest_args.size() < sizeof...(Args)) {
                throw std::runtime_error("FFI: too few arguments for member function");
            }
            if constexpr (std::is_void_v<Ret>) {
                call_member_helper(f, obj, rest_args, std::index_sequence_for<Args...>{});
                return runtime::value(nullptr);
            } else {
                return type_caster<std::decay_t<Ret>>::cast(call_member_helper(f, obj, rest_args, std::index_sequence_for<Args...>{}));
            }
        };
    }

    // Overload for std::function
    template <typename Ret, typename... Args>
    runtime::Function::Func wrap_function(std::function<Ret(Args...)> f) {
        return [f](const std::vector<runtime::value>& args) -> runtime::value {
            if (args.size() < sizeof...(Args)) {
                throw std::runtime_error("FFI: too few arguments");
            }
            if constexpr (std::is_void_v<Ret>) {
                call_helper(f, args, std::index_sequence_for<Args...>{});
                return runtime::value(nullptr);
            } else {
                return type_caster<std::decay_t<Ret>>::cast(call_helper(f, args, std::index_sequence_for<Args...>{}));
            }
        };
    }

    // --- Module & Class Registration ---

    class class_base {
    public:
        virtual ~class_base() = default;
        virtual std::shared_ptr<runtime::Object> object() = 0;
    };

    class module {
    public:
        module(std::string name) : m_name(std::move(name)) {
            m_object = builtin::MakeObject(m_name);
        }

        template <typename Func>
        module& def(const std::string& name, Func f) {
            auto wrapped = wrap_function(f);
            auto callable = std::make_shared<runtime::Function>(name, wrapped);
            m_object->fields[name] = runtime::value(std::static_pointer_cast<runtime::Callable>(callable));
            return *this;
        }

        std::shared_ptr<runtime::Object> object() const { return m_object; }

    private:
        std::string m_name;
        std::shared_ptr<runtime::Object> m_object;
    };

    template <typename T>
    class class_ : public class_base {
    public:
        class_(module& m, const std::string& name) : m_name(name) {
            m_class = std::make_shared<builtin::Class>(name);
            m.object()->fields[name] = runtime::value(m_class->Object());
        }

        template <typename... Args>
        class_& def_constructor() {
            auto ctor = [](const std::vector<runtime::value>& args) -> runtime::value {
                // Simplified: only support no-arg constructor for now
                T* obj = new T(); 
                auto wrapper = builtin::MakeObject("instance");
                wrapper->fields["__ptr__"] = runtime::value(static_cast<void*>(obj));
                return runtime::value(wrapper);
            };
            auto callable = std::make_shared<runtime::Function>(m_name, ctor);
            m_class->Object()->fields["__init__"] = runtime::value(std::static_pointer_cast<runtime::Callable>(callable));
            return *this;
        }

        template <typename Func>
        class_& def(const std::string& name, Func f) {
            if constexpr (std::is_member_function_pointer_v<Func>) {
                auto wrapped = wrap_member_function(f);
                m_class->WithMethod(name, wrapped);
            } else {
                auto wrapped = wrap_function(f);
                m_class->WithMethod(name, wrapped);
            }
            return *this;
        }

        std::shared_ptr<runtime::Object> object() override {
            return m_class->Object();
        }

    private:
        std::string m_name;
        std::shared_ptr<builtin::Class> m_class;
    };

} // namespace cora::ffi

#define CORA_MODULE(name, m) \
    extern "C" CORA_API void CoraInit_##name(cora::compiler::runtime::Scope& scope); \
    static void CoraInternalInit_##name(cora::ffi::module& m); \
    void CoraInit_##name(cora::compiler::runtime::Scope& scope) { \
        cora::ffi::module m(#name); \
        CoraInternalInit_##name(m); \
        cora::compiler::builtin::RegisterModule(scope, #name, m.object()); \
    } \
    void CoraInternalInit_##name(cora::ffi::module& m)

#endif // CORA_FFI_HPP
