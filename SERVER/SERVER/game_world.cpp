#include "pch.h"
#include "game_world.h"

GameWorld::GameWorld(int32_t world_width, int32_t world_height) 
    : _sectors((world_height / VIEW_DIAM + 1) * (world_height / VIEW_DIAM + 1)),
    _sector_lock((world_height / VIEW_DIAM + 1) * (world_height / VIEW_DIAM + 1)),
    _world_w{ world_width }, _world_h{ world_height },
    _sector_w{ world_width / VIEW_DIAM + 1 }, _sector_h{ world_height / VIEW_DIAM + 1 } { }

GameWorld::~GameWorld() { }
