#pragma once
#include <algorithm>
struct UiRect { float x,y,width,height; };
struct UiLayout {
    bool wide;
    float width,height,panelWidth,tile,left,top;
    UiLayout(float w,float h):wide(w>=900),width(w),height(h),panelWidth(wide?300:w-24) {
        const float areaWidth=wide?w-348:w-24;
        const float areaHeight=wide?h-178:h-348;
        tile=std::max(1.0f,std::min(areaWidth/10,areaHeight/7));
        left=(wide?336:12)+(areaWidth-tile*10)/2;
        top=wide?94:264;
    }
    UiRect crop(int i) const {
        const int cols=wide?2:5;
        const float w=(panelWidth-(cols-1)*4)/cols;
        return {12+(i%cols)*(w+4),wide?(height<640?64.0f:94.0f)+(i/cols)*36:64.0f,w,32};
    }
    UiRect action(int i) const {
        const int cols=wide?2:3;
        const float w=(panelWidth-(cols-1)*4)/cols;
        return {12+(i%cols)*(w+4),(wide?(height<640?246.0f:340.0f):174.0f)+(i/cols)*38,w,34};
    }
    float detailTop() const { return wide?(height<640?178.0f:212.0f):102.0f; }
    float hintTop() const { return wide?(height<640?370.0f:466.0f):250.0f; }
};
