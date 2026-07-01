#pragma once

#include <atomic>
#include <vector>
#include <cassert>

namespace p5cpp
{
    class AppContext
    {
    public:
        template <typename T>
        inline void registerService(T* instance)
        {
            const size_t typeId = getUniqueTypeId<T>();
            if (typeId >= services.size()) {
                services.resize(typeId + 1, nullptr);
            }

            services[typeId] = instance;
        }

        template <typename T>
        inline void unregisterService()
        {
            const size_t typeId = getUniqueTypeId<T>();
            if (typeId < services.size()) {
                services[typeId] = nullptr;
            }
        }

        template <typename T>
        T* getOrNull() const
        {
            const size_t typeId = getUniqueTypeId<T>();
            if (typeId >= services.size()) {
                return nullptr;
            }

            return static_cast<T*>(services[typeId]);
        }

        template <typename T>
        T& require() const
        {
            T* instance = getOrNull<T>();
            assert(instance != nullptr && "Required service not found");
            return *instance;
        }

    private:
        inline static size_t nextTypeId()
        {
            static std::atomic<size_t> nextTypeId {0};
            return nextTypeId++;
        }

        template <typename T>
        inline static size_t getUniqueTypeId()
        {
            static size_t typeId = nextTypeId();
            return typeId;
        }

        std::vector<void*> services;
    };
} // namespace p5cpp
