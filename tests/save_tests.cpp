#include "core/SaveCodec.h"
#include "storage/SaveStore.h"
#include "app/TileSelection.h"
#include <filesystem>
#include <chrono>
#include <fstream>
#include <iostream>
int main(){
 int failures=0;auto check=[&](bool b,const char* n){if(!b){std::cerr<<n<<'\n';++failures;}};
 farm::World w;w.plant(0,0);w.hireWorker(farm::CropId::Tomato);w.upgrade(farm::CropId::Tomato);w.tick(3.25);w.tend(0,0);
 std::string error; auto text=farm::SaveCodec::encode(w);farm::World out;
 check(farm::SaveCodec::decode(text,out,error)&&farm::SaveCodec::encode(out)==text,"exact idle roundtrip");
 auto future=w;future.tick(50.0);out.tick(50.0);check(farm::SaveCodec::encode(future)==farm::SaveCodec::encode(out),"restored production");
 for(auto bad:{std::string("GRID_FARM_SAVE 1"),text.substr(0,100),text+"garbage",std::string(17000,'x')}){auto before=farm::SaveCodec::encode(out);check(!farm::SaveCodec::decode(bad,out,error)&&farm::SaveCodec::encode(out)==before,"bad save transactional");}
 for(auto pair:{std::pair<std::string,std::string>{"money 525","money -1"},{"money 525","money 1e101"},{"money 525","money nan"},{"cooldown 5","cooldown 6"},{"workers 1","workers 2"},{"upgrades 1","upgrades 101"}}){auto bad=text;auto pos=bad.find(pair.first);check(pos!=std::string::npos,"mutation anchor");if(pos!=std::string::npos){bad.replace(pos,pair.first.size(),pair.second);check(!farm::SaveCodec::decode(bad,out,error),"invalid idle field rejected");}}
 auto dir=std::filesystem::temp_directory_path()/("grid-idle-test-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
 auto path=dir/"idle.save"; check(farm::SaveStore::save(path,w,error),"disk save");check(farm::SaveStore::load(path,out,error)&&farm::SaveCodec::encode(out)==text,"disk load");
 w.tick(1.0);check(farm::SaveStore::save(path,w,error),"replace save");
 auto staging=path;staging+=".writing";std::filesystem::create_directory(staging);check(!farm::SaveStore::save(path,out,error),"busy write rejected");
 check(farm::SaveStore::load(path,out,error)&&farm::SaveCodec::encode(out)==farm::SaveCodec::encode(w),"busy write preserves save");
 std::filesystem::remove_all(dir);
 TileSelection selection;selection.move(-1,-1);check(selection.column==0&&selection.row==0,"selection lower bound");for(int i=0;i<20;++i)selection.move(1,1);check(selection.column==9&&selection.row==6,"selection upper bound");
 return failures?1:0;
}
