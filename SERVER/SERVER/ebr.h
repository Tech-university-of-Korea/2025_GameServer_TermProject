#pragma once
#include <queue>
#include <atomic>
#include <thread>

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

template <typename T>
class EBR {
private:
    inline static constexpr size_t ALIGN_SIZE = MIN_ARRAY_ALIGN_SIZE<EpochCounter>;

public:
    EBR() : _epoch_array{ std::make_unique<EpochCounter[]>(MAX_THREAD * ALIGN_SIZE) }, _epoch_counter{ 1 } { }
    ~EBR() { clear_thread_epoch(); }

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
        while (false == _free_queue.empty()) {
            auto ptr = _free_queue.front();
            _free_queue.pop();
            if (nullptr == ptr) {
                continue;
            }

            std::destroy_at(ptr);
            delete ptr;
        }

        _epoch_counter = 1;
    }

    void push_ptr(T* ptr) {
        ptr->_epoch_counter = _epoch_counter.load();
        _free_queue.push(ptr);
    }

    template <typename... Args> requires std::is_constructible_v<T, Args...>
    T* pop_ptr(Args&&... args) {
        if (true == _free_queue.empty()) {
            return new T{ std::forward<Args>(args)... };
        }

        auto ptr = _free_queue.front();
        for (size_t i = 0; i < MAX_THREAD * ALIGN_SIZE; i += ALIGN_SIZE) {
            if (_epoch_array[i] != 0 && _epoch_array[i] < ptr->_epoch_counter) {
                return new T{ std::forward<Args>(args)... };
            }
        }

        _free_queue.pop();

#ifdef DEBUG_PRINT
        std::cout << "Reuse Session Pointer\n";
#endif

        std::destroy_at(ptr);
        ptr = new(ptr) T{ std::forward<Args>(args)... };
        return ptr;
    }

public:
    inline static thread_local std::queue<T*> _free_queue{ };
    inline static const size_t MAX_THREAD = HARDWARE_CONCURRENCY;

private:
    EpochCounter _epoch_counter{ };
    const std::unique_ptr<EpochCounter[]> _epoch_array;
};

template <typename T>
class EBRGuard {
public:
    EBRGuard() = delete;
    EBRGuard(const EBRGuard&) = delete;
    EBRGuard(EBRGuard&&) = delete;
    EBRGuard& operator=(const EBRGuard&) = delete;
    EBRGuard& operator=(EBRGuard&&) = delete;

    EBRGuard(EBR<T>& ebr, const int32_t thread_id) : _ebr{ ebr }, _thread_id{ thread_id } {
        _ebr.begin_epoch(_thread_id);
    }

    ~EBRGuard() {
        _ebr.end_epoch(_thread_id);
    }

private:
    const int32_t _thread_id;
    EBR<T>& _ebr;
};