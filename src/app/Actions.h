#pragma once
#include "core/World.h"
#include "app/TileSelection.h"
inline farm::Result performAction(farm::World& world,int action,TileSelection selection,farm::CropId crop) {
    switch(action) {
    case 0:return world.buyLand(selection.column,selection.row);
    case 1:return world.plant(selection.column,selection.row,crop);
    case 2:return world.harvest(selection.column,selection.row);
    case 3:return world.hireWorker(crop);
    case 4:return world.upgrade(crop);
    case 5:return world.tend(selection.column,selection.row);
    default:return farm::Result::InvalidTile;
    }
}
inline farm::Result actionAvailability(const farm::World& world,int action,TileSelection selection,farm::CropId crop) {
    // check a copy so button availability uses the same rules as the action
    auto preview=world;
    return performAction(preview,action,selection,crop);
}
