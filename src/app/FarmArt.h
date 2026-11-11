#pragma once
#include "core/World.h"
#include "raylib.h"
#include <cmath>
#include <algorithm>

// drawings scale with the plot; no image files needed
namespace art {
inline constexpr Color cream{244,231,197,255}, gold{235,187,99,255};
inline constexpr Color leaf{99,155,87,255}, darkLeaf{46,100,66,255};
inline Color cropColor(farm::CropId id) {
    constexpr Color colors[]={{223,91,70,255},{115,142,211,255},{239,162,67,255},{179,97,151,255},{240,119,133,255}};
    return colors[static_cast<int>(id)];
}
inline void plant(float x,float y,float size,farm::CropId id,double progress,double time) {
    const float growth=progress<.25?.42f:progress<.7?.72f:1.0f;
    const float s=size*growth;
    const float sway=static_cast<float>(std::sin(time*1.6+x))*.025f*s;
    DrawLineEx({x,y+s*.2f},{x+sway,y-s*.35f},std::max(1.0f,s*.08f),darkLeaf);
    DrawEllipse(static_cast<int>(x-s*.19f+sway),static_cast<int>(y-s*.2f),s*.23f,s*.10f,leaf);
    DrawEllipse(static_cast<int>(x+s*.18f+sway),static_cast<int>(y-s*.32f),s*.23f,s*.11f,darkLeaf);
    if(progress<.25) return;
    const Color fruit=progress<.7?Color{149,168,97,255}:cropColor(id);
    if(id==farm::CropId::Carrot) {
        DrawTriangle({x-s*.18f,y},{x,y+s*.42f},{x+s*.18f,y},fruit);
        DrawLineEx({x,y-s*.1f},{x-s*.23f,y-s*.55f},std::max(1.0f,s*.06f),leaf);
    } else if(id==farm::CropId::Blueberry) {
        for(int i=0;i<5;++i) DrawCircleV({x+(i%3-1)*s*.19f,y+(i/3)*s*.19f},s*.14f,fruit);
    } else if(id==farm::CropId::Strawberry) {
        DrawCircleV({x,y},s*.23f,fruit);
        DrawTriangle({x-s*.21f,y},{x,y+s*.34f},{x+s*.21f,y},fruit);
        for(int i=0;i<3;++i) DrawCircleV({x+(i-1)*s*.10f,y+s*.06f},std::max(.6f,s*.025f),cream);
    } else {
        DrawCircleV({x,y+s*.08f},s*.26f,fruit);
        DrawCircleV({x-s*.08f,y},s*.065f,Fade(cream,.55f));
        if(id==farm::CropId::Beet) DrawLineEx({x,y+s*.26f},{x+s*.08f,y+s*.4f},std::max(1.0f,s*.05f),fruit);
    }
}
inline void plot(Rectangle r,const farm::Tile& tile,int col,int row,double time) {
    DrawRectangleRounded(r,.12f,4,tile.owned?Color{125,86,58,255}:Color{static_cast<unsigned char>(59+(col+row)%2*4),91,65,255});
    if(tile.owned) {
        for(int i=1;i<=3;++i) {
            float y=r.y+r.height*i/4;
            DrawLineEx({r.x+3,y},{r.x+r.width-3,y},std::max(1.0f,r.height*.035f),Color{104,70,49,255});
        }
    } else {
        for(int i=0;i<3;++i) {
            const float x=r.x+r.width*(.2f+.27f*i),y=r.y+r.height*(.3f+.15f*((col+row+i)%3));
            DrawLineEx({x,y},{x-2,y-3},1,Color{83,119,76,255});
            DrawLineEx({x,y},{x+2,y-4},1,Color{83,119,76,255});
        }
    }
    if(!tile.planted) return;
    const double progress=tile.age/farm::cropDefinition(tile.crop)->cycleSeconds;
    for(int i=0;i<3;++i) plant(r.x+r.width*(.28f+.22f*i),r.y+r.height*(i==1?.34f:.59f),r.width*.44f,tile.crop,progress,time);
    DrawRectangleRounded({r.x+4,r.y+r.height-6,r.width-8,3},.5f,4,Color{65,49,36,255});
    DrawRectangleRec({r.x+4,r.y+r.height-6,static_cast<float>((r.width-8)*std::min(1.0,progress)),3},progress>=1?gold:leaf);
    if(progress>=1) DrawCircleV({r.x+r.width-5,r.y+5},2,gold);
}
inline void person(float x,float y,float size,Color shirt,double time,bool walking) {
    const float stride=walking?static_cast<float>(std::sin(time*9))*size*.22f:0;
    DrawEllipse(static_cast<int>(x),static_cast<int>(y+size*.8f),size*.8f,size*.28f,Fade(BLACK,.22f));
    DrawLineEx({x-size*.25f,y+size*.25f},{x-size*.25f+stride,y+size*.8f},size*.25f,Color{48,62,65,255});
    DrawLineEx({x+size*.25f,y+size*.25f},{x+size*.25f-stride,y+size*.8f},size*.25f,Color{48,62,65,255});
    DrawRectangleRounded({x-size*.55f,y-size*.4f,size*1.1f,size},.3f,4,shirt);
    DrawLineEx({x-size*.65f,y-size*.15f},{x-size*.75f,y+size*.3f+stride},size*.22f,cream);
    DrawLineEx({x+size*.65f,y-size*.15f},{x+size*.75f,y+size*.3f-stride},size*.22f,cream);
    DrawCircleV({x,y-size*.65f},size*.43f,Color{227,183,139,255});
    DrawRectangleRounded({x-size*.52f,y-size*1.25f,size*1.04f,size*.5f},.3f,4,gold);
    DrawEllipse(static_cast<int>(x),static_cast<int>(y-size*.8f),size*.8f,size*.16f,gold);
}
}
