#include "storage/ProgressStore.h"
#include "core/SaveCodec.h"
#include "app/WorkerVisual.h"
#include <chrono>
#include <cmath>
#include <filesystem>
#include <iostream>
int main() {
 int failures=0;auto check=[&](bool b,const char* n){if(!b){std::cerr<<n<<'\n';++failures;}};
 auto close=[](double a,double b){return std::abs(a-b)<1e-7;};
 farm::World base;base.plant(0,0);base.hireWorker(farm::CropId::Tomato);base.tick(2.0);base.markSaved(1000);
 auto w=base;auto report=farm::catchUp(w,1018);
 check(report.elapsed==18 && report.credited==18 && report.earnings==120 && w.tile(0,0)->age==0,"partial cycle catch-up");
 auto again=farm::catchUp(w,1018);check(again.earnings==0,"repeat timestamp no credit");
 w=base;report=farm::catchUp(w,1000+3600*20);check(report.credited==28800&&report.earnings==172800,"eight hour cap");
 w=base;report=farm::catchUp(w,1000+28800);check(report.credited==28800,"exact cap boundary");
 w=base;report=farm::catchUp(w,900);check(report.credited==0&&report.clockBackwards&&w.savedAt()==1000,"backward clock preserves high-water mark");
 report=farm::catchUp(w,1000);check(report.earnings==0,"clock recovery no double credit");
 farm::World old;old.plant(0,0);old.hireWorker(farm::CropId::Tomato);auto before=old.balance();report=farm::catchUp(old,2000);check(report.earnings==0&&old.balance()==before,"untimestamped save no invented elapsed time");
 farm::World manual;manual.plant(0,0);manual.markSaved(1000);before=manual.balance();farm::catchUp(manual,2000);check(manual.balance()==before&&manual.tile(0,0)->age==10,"unautomated crops mature without payouts");
 std::string error;auto text=farm::SaveCodec::encode(base);farm::World decoded;
 check(farm::SaveCodec::decode(text,decoded,error)&&decoded.savedAt()==1000,"timestamp roundtrip");
 auto legacy=text;legacy.replace(0,std::string("GRID_IDLE_SAVE 2\nsaved_at 1000\n").size(),"GRID_IDLE_SAVE 1\n");
 check(farm::SaveCodec::decode(legacy,decoded,error)&&decoded.savedAt()==0,"version one idle save loads");
 auto bad=text;auto pos=bad.find("saved_at 1000");bad.replace(pos,13,"saved_at -1");check(!farm::SaveCodec::decode(bad,decoded,error),"negative timestamp rejected");
 auto dir=std::filesystem::temp_directory_path()/("grid-offline-test-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
 auto path=dir/"idle.save";check(farm::SaveStore::save(path,base,error),"fixture save");
 farm::World live;check(farm::loadProgress(path,live,1020,report,error)&&report.earnings==120,"load credits and persists");
 auto snapshot=farm::SaveCodec::encode(live);check(farm::loadProgress(path,live,1020,report,error)&&report.earnings==0&&farm::SaveCodec::encode(live)==snapshot,"repeat disk load no duplicate earnings");
 auto staging=path;staging+=".writing";std::filesystem::create_directory(staging);
 check(!farm::loadProgress(path,live,1040,report,error)&&farm::SaveCodec::encode(live)==snapshot,"failed catch-up commit leaves live state unchanged");
 check(!farm::saveProgress(path,live,1040,error)&&live.savedAt()==1020,"failed save preserves live timestamp");
 std::filesystem::remove(staging);check(farm::loadProgress(path,live,1040,report,error)&&report.earnings==120,"retry pays exactly once");
 std::filesystem::remove_all(dir);
 auto visual=base;visual.buyLand(1,0);visual.plant(1,0);visual.buyLand(2,0);visual.plant(2,0,farm::CropId::Carrot);
 snapshot=farm::SaveCodec::encode(visual);
 auto a=workerPosition(visual,farm::CropId::Tomato,0),b=workerPosition(visual,farm::CropId::Tomato,1.0/1.4);
 check(close(a.x,.5)&&close(b.x,1.5)&&a.y==.5&&b.y==.5,"worker visits own plot centres");
 auto c=workerPosition(visual,farm::CropId::Carrot,0);check(c.x>2&&c.x<3&&c.y>0&&c.y<1,"single plot worker stays near crop");
 for(int i=0;i<1000;++i){auto v=workerPosition(visual,farm::CropId::Tomato,i*.1);check(v.x>=.5&&v.x<=1.5&&v.y==.5,"worker route bounded to assigned pair");}
 check(farm::SaveCodec::encode(visual)==snapshot,"worker presentation cannot mutate production");
 return failures?1:0;
}
