#pragma once

#include <iostream>
#include <format>
#include <thread>
#include <atomic>
#include <queue>

using EpochNumType = uint64_t;
using EpochCounter = std::atomic<EpochNumType>;
using ThreadIdType = int32_t;

inline const uint32_t HARDWARE_CONCURRENCY = std::thread::hardware_concurrency();
inline constexpr size_t CACHE_LINE_SIZE = std::hardware_constructive_interference_size;
//inline constexpr size_t CACHE_LINE_SIZE = std::hardware_destructive_interference_size;

template <typename T>
inline constexpr size_t MIN_ARRAY_ALIGN_SIZE = CACHE_LINE_SIZE / sizeof(T);

template <typename T>
concept HasEpochCount = requires(T t) {
    t.m_epoch_counter;
    { t.m_epoch_counter } -> std::same_as<EpochNumType>;
};

template <typename T>
class Ebr {
protected:
    inline static constexpr size_t ALIGN_SIZE = MIN_ARRAY_ALIGN_SIZE<std::atomic_uint64_t>;

public:
    Ebr() : m_ebr_array{ std::make_unique<EpochCounter[]>(MAX_THREAD * ALIGN_SIZE) }, m_epoch_counter{ 1 } {}
    ~Ebr() { }

public:
    void StartEpoch(const ThreadIdType id) {
        auto epoch = m_epoch_counter.fetch_add(1);
        m_ebr_array[id * ALIGN_SIZE] = epoch;
    }

    void EndEpoch(const ThreadIdType id) {
        m_ebr_array[id * ALIGN_SIZE] = 0;
    }

    void ClearThreadEpoch() {
        while (false == m_free_queue.empty()) {
            auto ptr = m_free_queue.front();
            m_free_queue.pop();
            if (nullptr == ptr) {
                continue;
            }

            std::destroy_at(ptr);
            delete ptr;
        }
    }

    void PushPointer(T* ptr) {
        ptr->m_epoch_counter = m_epoch_counter.load();
        m_free_queue.push(ptr);
    }

    template <typename... Args> requires std::is_constructible_v<T, Args...>
    T* PopPointer(Args&&... args) {
        if (true == m_free_queue.empty()) {
            return new T{ std::forward<Args>(args)... };
        }

        auto ptr = m_free_queue.front();
        for (size_t i = 0; i < MAX_THREAD * ALIGN_SIZE; i += ALIGN_SIZE) {
            if (0 != m_ebr_array[i] and m_ebr_array[i] < ptr->m_epoch_counter) {
                return new T{ std::forward<Args>(args)... };
            }
        }

        m_free_queue.pop();

        std::destroy_at(ptr);
        ptr = new(ptr) T{ std::forward<Args>(args)... };
        std::cout << std::format("Reuse Ptr\n");
        return ptr;
    }

public:
    inline static thread_local std::queue<T*> m_free_queue{ };
    inline static const size_t MAX_THREAD = HARDWARE_CONCURRENCY;

protected:
    EpochCounter m_epoch_counter{ };
    const std::unique_ptr<EpochCounter[]> m_ebr_array;
};

template <typename T>
class EBRGuard {
public:
    EBRGuard(Ebr<T>& Ebr) {
        m_ebr.StartEpoch();
    }

    ~EBRGuard() {
        m_ebr.EndEpoch();
    }

private:
    Ebr<T>& m_ebr;
};