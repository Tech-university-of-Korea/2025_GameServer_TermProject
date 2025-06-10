#pragma once

#include <concurrent_unordered_map.h>
#include <mutex>
#include <unordered_set>
#include <array>
#include "game_header.h"

inline constexpr int VIEW_RANGE = 7;

inline constexpr auto VIEW_DIAM = VIEW_RANGE * 2;
inline constexpr auto SECTOR_WIDTH = (MAP_WIDTH / VIEW_DIAM) + 1;
inline constexpr auto SECTOR_HEIGHT = (MAP_HEIGHT / VIEW_DIAM) + 1;

enum Dir {
	DIR_CENTER = 0,
	DIR_UP,
	DIR_DOWN,
	DIR_LEFT,
	DIR_RIGHT,
	DIR_LEFT_UP,
	DIR_RIGHT_UP,
	DIR_LEFT_DOWN,
	DIR_RIGHT_DOWN,

	DIR_CNT
};

constexpr std::array<const std::pair<int8_t, int8_t>, DIR_CNT> DIRECTIONS{
	std::pair<int8_t, int8_t>{ 0, 0 },
	std::pair<int8_t, int8_t>{ 0, -1 },
	std::pair<int8_t, int8_t>{ 0, 1 },
	std::pair<int8_t, int8_t>{ -1, 0 },
	std::pair<int8_t, int8_t>{ 1, 0 },
	std::pair<int8_t, int8_t>{ -1, -1 },
	std::pair<int8_t, int8_t>{ 1, -1 },
	std::pair<int8_t, int8_t>{ -1, 1 },
	std::pair<int8_t, int8_t>{ 1, 1 }
};

class Sectors {
public:
	Sectors();
	~Sectors();

public:
	std::pair<int32_t, int32_t> get_sector_idx(int32_t x, int32_t y);
	bool is_valid_sector(int32_t sector_x, int32_t sector_y);

	std::mutex& get_sector_lock(int32_t sector_x, int32_t sector_y);
	const std::unordered_set<int32_t>& get_sector(int32_t sector_x, int32_t sector_y);

	std::pair<int32_t, int32_t> update_sector(int32_t id, int32_t prev_x, int32_t prev_y, int32_t curr_x, int32_t curr_y);

	std::pair<int32_t, int32_t> insert(int32_t id, int32_t x, int32_t y);
	void erase(int32_t id, int32_t x, int32_t y);

private:
	std::array<std::array<std::mutex, SECTOR_WIDTH>, SECTOR_HEIGHT> _sector_lock{ };
	std::array<std::array<std::unordered_set<int32_t>, SECTOR_WIDTH>, SECTOR_HEIGHT> _sectors{ };
};