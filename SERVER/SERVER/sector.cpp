#include "pch.h"
#include "sector.h"

Sectors::Sectors() { }

Sectors::~Sectors() { }

std::pair<int32_t, int32_t> Sectors::get_sector_idx(int32_t x, int32_t y) {
	return std::make_pair(x / VIEW_DIAM, y / VIEW_DIAM);
}

bool Sectors::is_valid_sector(int32_t sector_x, int32_t sector_y) {
	return (sector_x < SECTOR_WIDTH and sector_y < SECTOR_HEIGHT and sector_x > 0 and sector_y > 0);
}

std::mutex& Sectors::get_sector_lock(int32_t sector_x, int32_t sector_y) {
	return _sector_lock[sector_x][sector_y];
}

const std::unordered_set<int32_t>& Sectors::get_sector(int32_t sector_x, int32_t sector_y) {
	return _sectors[sector_x][sector_y];
}

std::pair<int32_t, int32_t> Sectors::update_sector(int32_t id, int32_t prev_x, int32_t prev_y, int32_t curr_x, int32_t curr_y) {
	auto [prev_sector_x, prev_sector_y] = get_sector_idx(prev_x, prev_y);
	auto [curr_sector_x, curr_sector_y] = get_sector_idx(curr_x, curr_y);
	if (prev_sector_x != curr_sector_x or prev_sector_y != curr_sector_y) {
		void* lock_addr1 = &_sector_lock[prev_sector_x][prev_sector_y];
		void* lock_addr2 = &_sector_lock[curr_sector_x][curr_sector_y];
		// Dead lock 회피를 위한 순서 제어. 2개의 같은 락에 대해선 항상 똑같은 순서
		if (lock_addr1 < lock_addr2) {
			std::lock_guard guard1{ _sector_lock[prev_sector_x][prev_sector_y] };
			std::lock_guard guard2{ _sector_lock[curr_sector_x][curr_sector_y] };
			_sectors[prev_sector_x][prev_sector_y].erase(id);
			_sectors[curr_sector_x][curr_sector_y].insert(id);
		}
		else {
			std::lock_guard guard1{ _sector_lock[curr_sector_x][curr_sector_y] };
			std::lock_guard guard2{ _sector_lock[prev_sector_x][prev_sector_y] };
			_sectors[prev_sector_x][prev_sector_y].erase(id);
			_sectors[curr_sector_x][curr_sector_y].insert(id);
		}
	}

	return std::make_pair(curr_sector_x, curr_sector_y);
}

std::pair<int32_t, int32_t> Sectors::insert(int32_t id, int32_t x, int32_t y) {
    auto [sector_x, sector_y] = get_sector_idx(x, y);
    std::lock_guard sector_gaurd{ _sector_lock[sector_x][sector_y] };
    _sectors[sector_x][sector_y].insert(id);

    return std::make_pair(sector_x, sector_y);
}

void Sectors::erase(int32_t id, int32_t x, int32_t y) {
	auto [sector_x, sector_y] = get_sector_idx(x, y);
	std::lock_guard sector_gaurd{ _sector_lock[sector_x][sector_y] };
	_sectors[sector_x][sector_y].erase(id);
}
