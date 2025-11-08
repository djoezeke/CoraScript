#include "Cora/Runtime/Value.hpp"

namespace cora
{
    namespace runtime
    {
        std::string Any::Repr()
        {
            return std::string("Any");
        }

        std::string Null::Repr()
        {
            return std::string("Null");
        }

        std::string Byte::Repr()
        {
            return std::string("Byte");
        }

        std::string Float::Repr()
        {
            return std::string("Float");
        }

        std::string Array::Repr()
        {
            return std::string("Array");
        }

        std::string Object::Repr()
        {
            return std::string("Object");
        }

        std::string String::Repr()
        {
            return std::string("String");
        }

        std::string Integer::Repr()
        {
            return std::string("Integer");
        }

        std::string Pointer::Repr()
        {
            return std::string("Pointer");
        }

        std::ostream &operator<<(std::ostream &ostream, const Value *value)
        {
            return ostream;
        }

    } // namespace runtime

} // namespace cora
