#pragma once

template <typename T>
using RunTimeArray = const std::vector<T>;

inline constexpr int MAX_VIEW_RANGE = 7;

inline constexpr auto MAX_VIEW_DIAM = MAX_VIEW_RANGE * 2;

class GameWorld {
public:
    GameWorld(int32_t world_width, int32_t world_height);
    ~GameWorld();

public:
    std::pair<int32_t, int32_t> get_sector_idx();
    const std::unordered_set<int32_t>& get_sector(int32_t sector_x, int32_t sector_y);
    std::mutex& get_sector_lock(int32_t sector_x, int32_t sector_y);

    bool can_see_(int32_t from, int32_t to);

    void insert_in_world(int32_t id, int32_t x, int32_t y);
    void remove_in_world(int32_t id, int32_t x, int32_t y);
    void update_world_pos(int32_t id, int32_t old_x, int32_t old_y, int32_t x, int32_t y);

    bool change_world(int32_t id, GameWorld& other_world, int32_t old_x, int32_t old_y, int32_t new_x, int32_t new_y);

private:
    

private:
    int32_t _world_w;
    int32_t _world_h;
    int32_t _sector_w;
    int32_t _sector_h;
    RunTimeArray<std::unordered_set<int32_t>> _sectors;
    RunTimeArray<std::mutex> _sector_lock;

    std::unordered_set<int32_t> _players_in_world{ };
};