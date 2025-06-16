#pragma once

#include <queue>
#include <atomic>
#include <thread>

#include "utils.h"

#define DEBUG_PRINT

using ThreadIdType = int32_t;
using EpochNumType = uint64_t;
using EpochCounter = std::atomic<EpochNumType>;

inline const size_t HARDWARE_CONCURRENCY = std::thread::hardware_concurrency() * 2;
inline constexpr size_t CACHE_LINE_SIZE = std::hardware_destructive_interference_size;

template <typename T>
inline constexpr size_t MIN_ARRAY_ALIGN_SIZE = CACHE_LINE_SIZE / sizeof(T);

template <typename T>
concept HasEpochCount = requires(T t) {
    t._epoch_counter;
    { t._epoch_counter } -> std::same_as<EpochNumType>;
};

class SessionEbr {
private:
    inline static constexpr size_t ALIGN_SIZE = MIN_ARRAY_ALIGN_SIZE<EpochCounter>;

public:
    SessionEbr() : _epoch_array{ std::make_unique<EpochCounter[]>(MAX_THREAD * ALIGN_SIZE) }, _epoch_counter{ 1 } {}
    ~SessionEbr() { clear_thread_epoch(); }

public:
    EpochNumType get_curr_epoch_count() const {
        return _epoch_counter;
    }

    void begin_epoch(const ThreadIdType id) {
        auto epoch = _epoch_counter.fetch_add(1);
        _epoch_array[id * ALIGN_SIZE] = epoch;
    }

    void end_epoch(const ThreadIdType id) {
        _epoch_array[id * ALIGN_SIZE] = 0;
    }

    void clear_thread_epoch() {
        for (size_t i = 0; i < TAG_CNT; ++i) {
            while (false == _free_queue.at(i).empty()) {
                auto ptr = _free_queue.at(i).front();
                _free_queue.at(i).pop();
                if (nullptr == ptr) {
                    continue;
                }

                delete ptr;
            }
        }

        _epoch_counter = 1;
    }

    void push_ptr(ServerEntity* ptr) {
        ptr->_epoch_counter = _epoch_counter.load();
        size_t tag = static_cast<size_t>(ptr->get_object_tag());
        _free_queue.at(tag).push(ptr);
    }

    template <typename ObjectType, typename... Args> requires 
        std::derived_from<ObjectType, ServerEntity> and std::is_constructible_v<ObjectType, Args...>
    ObjectType* pop_ptr(Args&&... args) {
        size_t tag = static_cast<size_t>(GetObjectTag<ObjectType>::tag);
        if (true == _free_queue.at(tag).empty()) {
            return new ObjectType{ std::forward<Args>(args)... };
        }

        auto ptr = _free_queue.at(tag).front();
        for (size_t thread_idx = 0; thread_idx < MAX_THREAD * ALIGN_SIZE; thread_idx += ALIGN_SIZE) {
            if (_epoch_array[thread_idx] != 0 && _epoch_array[thread_idx] < ptr->_epoch_counter) {
                return new ObjectType{ std::forward<Args>(args)... };
            }
        }

        _free_queue.at(tag).pop();

#ifdef DEBUG_PRINT
        std::cout << "Reuse Session Pointer\n";
#endif

        std::destroy_at(ptr);
        auto ret = cast_ptr<ObjectType>(ptr);
        return new(ret) ObjectType{ std::forward<Args>(args)... };;
    }

public:
    inline static constexpr size_t TAG_CNT = static_cast<size_t>(ServerObjectTag::CNT);
    inline static thread_local std::array<std::queue<ServerEntity*>, TAG_CNT> _free_queue{ };
    inline static const size_t MAX_THREAD = HARDWARE_CONCURRENCY;

private:
    EpochCounter _epoch_counter{ };
    const std::unique_ptr<EpochCounter[]> _epoch_array;
};

class SessionEbrGuard {
public:
    SessionEbrGuard() = delete;
    SessionEbrGuard(const SessionEbrGuard&) = delete;
    SessionEbrGuard(SessionEbrGuard&&) = delete;
    SessionEbrGuard& operator=(const SessionEbrGuard&) = delete;
    SessionEbrGuard& operator=(SessionEbrGuard&&) = delete;

    SessionEbrGuard(SessionEbr& ebr, const int32_t thread_id) : _ebr{ ebr }, _thread_id{ thread_id } {
        _ebr.begin_epoch(_thread_id);
    }

    ~SessionEbrGuard() {
        _ebr.end_epoch(_thread_id);
    }

private:
    const int32_t _thread_id;
    SessionEbr& _ebr;
};