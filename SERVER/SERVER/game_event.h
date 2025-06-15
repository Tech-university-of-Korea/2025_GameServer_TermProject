#pragma once

enum class GameEventType {
    EVENT_KILL_ENEMY,
    EVENT_HEAL,
    EVENT_GET_DAMAGE
};

struct GameEvent {
    GameEventType event_type;
    int32_t sender;
};

struct GameEventKillEnemy : public GameEvent {
    static constexpr GameEventType type = GameEventType::EVENT_KILL_ENEMY;

    int32_t exp;
    //int32_t gold;
};

struct GameEventHeal : public GameEvent {
    static constexpr GameEventType type = GameEventType::EVENT_HEAL;

    int32_t heal_point;
};

struct GameEventGetDamage : public GameEvent {
    static constexpr GameEventType type = GameEventType::EVENT_GET_DAMAGE;

    int32_t damage;
};

template <typename Event>
    requires std::derived_from<Event, GameEvent>
inline Event* cast_event(GameEvent* event) 
{
    if (nullptr == event) {
        return nullptr;
    }

    return event->event_type == Event::type ? static_cast<Event*>(event) : nullptr;
}
