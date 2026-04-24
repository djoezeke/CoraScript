#ifndef CORA_RUNTIME_GARBAGE_COLLECTOR_HPP
#define CORA_RUNTIME_GARBAGE_COLLECTOR_HPP

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <unordered_set>
#include <vector>

namespace cora::compiler::runtime
{
    class Object;
    class Value;
    class Scope;

    class GarbageCollector
    {
    public:
        struct Stats
        {
            std::size_t totalAllocated{0};
            std::size_t totalFreed{0};
            std::size_t objectsAllocated{0};
            std::size_t objectsFreed{0};
            std::size_t lastCollectionTime{0};
            std::size_t collectionCount{0};
        };

        GarbageCollector();
        ~GarbageCollector();

        GarbageCollector(const GarbageCollector &) = delete;
        GarbageCollector &operator=(const GarbageCollector &) = delete;

        std::shared_ptr<Object> CreateObject(const std::string &className);

        void RegisterRoot(void *root);
        void UnregisterRoot(void *root);

        void MakeMark(std::shared_ptr<Object> &object);
        void Collect();

        bool ShouldCollect() const;
        void SetThreshold(std::size_t bytes);

        const Stats &GetStats() const;
        void ResetStats();

    private:
        void MarkReachable(const std::shared_ptr<Object> &obj, std::unordered_set<void *> &marked);
        void MarkValue(const Value &value, std::unordered_set<void *> &marked);
        void Sweep();

    private:
        std::vector<void *> m_Roots;
        std::unordered_set<std::shared_ptr<Object>> m_AllObjects;
        std::vector<std::shared_ptr<Object>> m_DeadObjects;
        std::size_t m_AllocationThreshold{1024 * 1024};
        std::size_t m_BytesAllocated{0};
        Stats m_Stats;
    };

    extern GarbageCollector *g_GarbageCollector;

    inline GarbageCollector &GetGarbageCollector()
    {
        if (!g_GarbageCollector)
        {
            g_GarbageCollector = new GarbageCollector();
        }
        return *g_GarbageCollector;
    }

} // namespace cora::compiler::runtime

#endif
