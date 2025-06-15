#pragma once

#include "overlapped.h"
#include "game_event.h"

enum class ServerObjectTag {
    SESSION, NPC, LUA_NPC, CNT
};

enum ServerObjectState {
    ST_FREE,
    ST_ALLOC,
    ST_INGAME
};

class ServerObject abstract {
public:
    ServerObject(ServerObjectTag tag, int32_t id);
    virtual ~ServerObject();

public:
    bool is_player();
    bool is_npc();
    bool is_active();

    std::mutex& get_state_lock();
    ServerObjectState get_state();

    int32_t get_id();
    ServerObjectTag get_object_tag();
    int16_t get_x();
    int16_t get_y();
    std::pair<int16_t, int16_t> get_position();
    std::string_view get_name();
    int32_t get_hp();
    int64_t get_move_time();

    void lock_state();
    void unlock_state();

    void update_hp(int32_t diff);

    void update_position(int16_t x, int16_t y);
    void update_position_atomic(int16_t x, int16_t y);
    void update_active_state(bool active);
    bool update_active_state_cas(bool& old_state, bool new_state);
    void update_move_time(int64_t move_time);
    void change_state(ServerObjectState state);

    void update_view_list(std::unordered_set<int32_t>& view_list, int32_t sector_x, int32_t sector_y);
    void insert_player_view_list(std::unordered_set<int32_t>& view_list, int32_t sector_x, int32_t sector_y);

    virtual void process_game_event(GameEvent* event) abstract;

    virtual bool try_respawn(int32_t max_hp);

    template <typename Event, typename... Args> 
        requires std::derived_from<Event, GameEvent>
    void dispatch_game_event(int32_t sender_id, Args&&... args) {
        auto event = new Event{ Event::type, sender_id, args... };
        dispatch_event(event);
    }

private:
    void dispatch_event(GameEvent* event);

public:
    // EBR
    uint64_t _epoch_counter{ };

protected:
    // Object Tag
    const ServerObjectTag _tag{ };

    // object STATE
    std::mutex _state_lock{ };
    ServerObjectState _state{ ST_FREE };

    // INGAME INFORMATION
    char _name[MAX_ID_LENGTH]{ };
    int16_t _x{ 0 };
    int16_t _y{ 0 };
    volatile uint32_t _level{ 1 };
    volatile uint32_t _exp{ 0 };
    std::atomic_int32_t _hp{ 100 };

    // NETWORK
    int32_t _id{ SYSTEM_ID };

    // active state
    std::atomic_bool _is_active{ false };

    // for latency
    int64_t _last_move_time{ 0 };
};