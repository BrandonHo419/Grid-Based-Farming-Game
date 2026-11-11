#include "app/UiLayout.h"
#include "app/Actions.h"
#include "core/SaveCodec.h"
#include <iostream>
int main(){
 int failures=0;auto check=[&](bool v,const char* n){if(!v){std::cerr<<n<<'\n';++failures;}};
 for(auto size:{std::pair<float,float>{360,520},{900,520},{900,640},{1000,800},{1920,1080}}){
  UiLayout l(size.first,size.second);
  check(size.first<900 || l.wide,"landscape uses side panel even at minimum height");
  check(l.crop(4).y+l.crop(4).height<=l.detailTop(),"crop buttons above details");
  check(l.detailTop()+54<=l.action(0).y,"details above actions");
  check(l.action(5).y+l.action(5).height<=l.hintTop(),"feedback below actions");
  check(l.hintTop()+14<size.second-64,"feedback above footer");
  check(l.left>=0&&l.top>=0&&l.left+10*l.tile<=size.first&&l.top+7*l.tile<=size.second-80,"farm fits");
  for(int i=0;i<6;++i){auto r=l.action(i);check(r.x>=0&&r.x+r.width<=size.first&&r.y+r.height<size.second-80,"actions fit");
   if(!l.wide)check(r.y+r.height<l.top,"compact controls above farm");
   else check(r.x+r.width<l.left,"wide controls beside farm");
  }
 }
 farm::World w;auto before=farm::SaveCodec::encode(w);
 check(actionAvailability(w,1,{},farm::CropId::Tomato)==farm::Result::Success,"plant enabled");
 check(actionAvailability(w,2,{},farm::CropId::Tomato)==farm::Result::NotReady,"harvest unavailable");
 check(actionAvailability(w,0,{},farm::CropId::Tomato)==farm::Result::AlreadyOwned,"owned land unavailable");
 check(farm::SaveCodec::encode(w)==before,"availability checks do not mutate live world");
 performAction(w,1,{},farm::CropId::Tomato);performAction(w,3,{},farm::CropId::Tomato);
 check(actionAvailability(w,3,{},farm::CropId::Tomato)==farm::Result::AlreadyAutomated,"hired worker unavailable");
 for(int i=0;i<6;++i){auto clone=w;check(actionAvailability(w,i,{},farm::CropId::Tomato)==performAction(clone,i,{},farm::CropId::Tomato),"UI agrees with rules");}
 return failures?1:0;
}
