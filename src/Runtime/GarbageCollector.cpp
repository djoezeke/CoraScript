#include "GarbageCollector.hpp"
#include "Scope.hpp"
#include "Value.hpp"

#include <algorithm>
#include <chrono>

namespace cora::compiler::runtime
{
    GarbageCollector *g_GarbageCollector = nullptr;

    GarbageCollector::GarbageCollector()
    {
    }

    GarbageCollector::~GarbageCollector()
    {
        m_AllObjects.clear();
        m_DeadObjects.clear();
    }

    std::shared_ptr<Object> GarbageCollector::CreateObject(const std::string &className)
    {
        auto object = std::make_shared<Object>(className);
        m_AllObjects.insert(object);

        m_BytesAllocated += sizeof(Object) + className.size();
        m_Stats.objectsAllocated++;
        m_Stats.totalAllocated += sizeof(Object);

        if (ShouldCollect())
        {
            Collect();
        }

        return object;
    }

    void GarbageCollector::RegisterRoot(void *root)
    {
        if (root)
        {
            m_Roots.push_back(root);
        }
    }

    void GarbageCollector::UnregisterRoot(void *root)
    {
        auto it = std::find(m_Roots.begin(), m_Roots.end(), root);
        if (it != m_Roots.end())
        {
            m_Roots.erase(it);
        }
    }

    void GarbageCollector::MakeMark(std::shared_ptr<Object> &object)
    {
        if (object)
        {
            m_AllObjects.insert(object);
        }
    }

    bool GarbageCollector::ShouldCollect() const
    {
        return m_BytesAllocated >= m_AllocationThreshold;
    }

    void GarbageCollector::SetThreshold(std::size_t bytes)
    {
        m_AllocationThreshold = bytes;
    }

    const GarbageCollector::Stats &GarbageCollector::GetStats() const
    {
        return m_Stats;
    }

    void GarbageCollector::ResetStats()
    {
        m_Stats = Stats{};
    }

    void GarbageCollector::Collect()
    {
        auto start = std::chrono::high_resolution_clock::now();

        std::unordered_set<void *> reachable;

        for (void *root : m_Roots)
        {
            if (root)
            {
                reachable.insert(root);
            }
        }

        for (const auto &obj : m_AllObjects)
        {
            if (obj)
            {
                for (const auto &field : obj->fields)
                {
                    Markvalue(field.second, reachable);
                }
            }
        }

        Sweep();

        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        m_Stats.lastCollectionTime = duration;
        m_Stats.collectionCount++;
        m_BytesAllocated = 0;
    }

    void GarbageCollector::Markvalue(const value &value, std::unordered_set<void *> &marked)
    {
        if (value.IsObject())
        {
            auto obj = value.AsObject();
            if (obj)
            {
                MarkReachable(obj, marked);
            }
        }
        else if (value.IsCallable())
        {
            auto callable = value.AsCallable();
            if (callable)
            {
                marked.insert(callable.get());
            }
        }
    }

    void GarbageCollector::MarkReachable(const std::shared_ptr<Object> &obj, std::unordered_set<void *> &marked)
    {
        if (!obj)
        {
            return;
        }

        void *ptr = obj.get();
        if (marked.find(ptr) != marked.end())
        {
            return;
        }

        marked.insert(ptr);

        for (const auto &field : obj->fields)
        {
            Markvalue(field.second, marked);
        }
    }

    void GarbageCollector::Sweep()
    {
        std::vector<std::shared_ptr<Object>> toBeSswept;

        for (const auto &obj : m_AllObjects)
        {
            if (obj.use_count() == 1)
            {
                toBeSswept.push_back(obj);
            }
        }

        for (const auto &obj : toBeSswept)
        {
            m_AllObjects.erase(obj);
            m_Stats.objectsFreed++;
            m_Stats.totalFreed += sizeof(Object);
        }
    }

} // namespace cora::compiler::runtime
