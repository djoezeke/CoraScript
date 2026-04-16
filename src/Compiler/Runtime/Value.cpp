#include "Cora/Compiler/Runtime/Value.hpp"

#include <iomanip>
#include <sstream>

namespace cora
{
    namespace runtime
    {
        std::string Any::Repr() const
        {
            return std::string("Any");
        }

        std::string Null::Repr() const
        {
            return std::string("Null");
        }

        std::string Byte::Repr() const
        {
            return std::string("Byte");
        }

        std::string Float::Repr() const
        {
            return std::string("Float");
        }

        std::string Array::Repr() const
        {
            return std::string("Array");
        }

        std::string Object::Repr() const
        {
            return std::string("Object");
        }

        std::string String::Repr() const
        {
            return std::string("String");
        }

        std::string Integer::Repr() const
        {
            return std::string("Integer");
        }

        std::string Pointer::Repr() const
        {
            return std::string("Pointer");
        }

        std::string Reference::Repr() const
        {
            return std::string("Reference");
        }

        std::string Undefined::Repr() const
        {
            return std::string("Undefined");
        }

        std::ostream &operator<<(std::ostream &ostream, const Value *value)
        {
            if (value == nullptr)
            {
                return ostream << "<null>";
            }

            return ostream << value->Repr();
        }

    } // namespace runtime

} // namespace cora
