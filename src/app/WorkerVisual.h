#pragma once
#include "core/World.h"
#include <cmath>
#include <vector>
// worker movement is cosmetic; production runs separately
inline farm::Position workerPosition(const farm::World& world, farm::CropId crop, double time) {
    std::vector<farm::Position> plots;
    for(int y=0;y<farm::World::rows;++y) for(int x=0;x<farm::World::columns;++x) {
        const auto* tile=world.tile(x,y);
        if(tile->planted && tile->crop==crop) plots.push_back({x+.5,y+.5});
    }
    if(plots.empty()) return {};
    if(!std::isfinite(time) || time<0) time=0;
    if(plots.size()==1) return {plots[0].x+.23*std::cos(time),plots[0].y+.23*std::sin(time)};
    double length=0;
    for(std::size_t i=0;i<plots.size();++i) {
        auto a=plots[i], b=plots[(i+1)%plots.size()]; length+=std::hypot(b.x-a.x,b.y-a.y);
    }
    double distance=std::fmod(time*1.4,length);
    for(std::size_t i=0;i<plots.size();++i) {
        auto a=plots[i], b=plots[(i+1)%plots.size()];double segment=std::hypot(b.x-a.x,b.y-a.y);
        if(distance<=segment) return {a.x+(b.x-a.x)*distance/segment,a.y+(b.y-a.y)*distance/segment};
        distance-=segment;
    }
    return plots.front();
}
