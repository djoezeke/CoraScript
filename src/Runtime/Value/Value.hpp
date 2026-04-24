#ifndef CORA_RUNTIME_VALUE_VALUE_H
#define CORA_RUNTIME_VALUE_VALUE_H

namespace cora
{
    class Value
    {
    private:
        union
        {
            /* data */
        };

    public:
        Value(/* args */);
        ~Value();
    };

} // namespace cora

#endif // CORA_RUNTIME_VALUE_VALUE_H