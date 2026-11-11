#include "core/World.h"
#include "core/Session.h"
#include <cmath>
#include <iostream>
#include <limits>
int main() {
    int failures=0;
    auto check=[&](bool ok,const char* name){ if(!ok){std::cerr<<name<<'\n';++failures;} };
    auto close=[](double a,double b){return std::abs(a-b)<1e-7;};
    using namespace farm;
    GameState world;
    check(world.buyLand(1,1)==Result::NotAdjacent,"diagonal rejected");
    check(world.plant(-1,0)==Result::InvalidTile,"invalid tile");
    check(world.plant(0,0,static_cast<CropId>(99))==Result::InvalidCrop,"invalid crop");
    check(world.plant(0,0)==Result::Success,"first plot");
    check(close(world.plotPrice(CropId::Tomato),25*1.15),"plot cost curve");
    world.tick(9.0); check(world.harvest(0,0)==Result::NotReady,"early harvest");
    world.tick(1.0); check(world.harvest(0,0)==Result::Success,"boundary harvest");
    check(world.tile(0,0)->planted && world.tile(0,0)->age==0 && world.balance()==1035,"permanent crop");
    check(world.harvest(0,0)==Result::NotReady,"no duplicate harvest");
    world.tick(100.0); check(world.tile(0,0)->age==10 && world.balance()==1035,"manual crop stops");
    check(world.hireWorker(CropId::Tomato)==Result::Success && world.balance()==845,"worker collects mature crop once");
    check(world.hireWorker(CropId::Tomato)==Result::AlreadyAutomated,"worker unique");
    auto split=world; world.tick(1000.5); for(int i=0;i<10005;++i) split.tick(.1);
    check(close(world.balance(),split.balance()) && close(world.tile(0,0)->age,split.tile(0,0)->age),"step equivalence");
    double before=world.balance(); world.tick(3600.0);
    check(close(world.balance()-before,360*60),"closed form cycles");
    check(world.upgrade(CropId::Tomato)==Result::Success && world.yield(CropId::Tomato)==75,"upgrade multiplier");
    check(close(world.upgradePrice(CropId::Tomato),360),"upgrade cost curve");
    double age=world.tile(0,0)->age;
    check(world.tend(0,0)==Result::Success && close(world.tile(0,0)->age,age+1),"tend adds tenth cycle");
    check(world.tend(0,0)==Result::Cooldown,"cooldown enforced");
    world.tick(5.0); check(world.tend(0,0)==Result::Success,"cooldown expires");
    const auto balance=world.balance(); world.tick(-1.0);world.tick(std::numeric_limits<double>::infinity());
    check(world.balance()==balance,"invalid dt ignored");
    GameState movement; movement.move({1,1},.1);
    check(close(std::hypot(movement.player().x-.5,movement.player().y-.5),.3),"normalized movement");
    for(int i=0;i<100;++i)movement.move({1,1},.1);
    check(movement.player().x==9.8 && movement.player().y==6.8,"movement bounds");
    check(formatMoney(1000)=="1.00K" && formatMoney(1e6)=="1.00M","money formatting");
    for(int i=0;i<5;++i){GameState c; auto id=static_cast<CropId>(i);c.plant(0,0,id);c.hireWorker(id);auto b=c.balance();c.tick(crops[i].cycleSeconds*3);check(close(c.balance()-b,3*crops[i].harvestValue),"all crop cycles");}
    Session session;session.world().plant(0,0);session.togglePause();session.tick({});check(session.world().tile(0,0)->age==0,"pause stops simulation");
    GameState broke; for(int col=1;col<=5;++col)broke.buyLand(col,0);
    check(broke.plant(0,0)==Result::InsufficientFunds && broke.balance()==0,"unaffordable plot does not mutate");
    GameState shared; shared.plant(0,0);shared.hireWorker(CropId::Tomato);shared.buyLand(1,0);shared.plant(1,0);auto cash=shared.balance();shared.tick(10.0);
    check(close(shared.balance()-cash,120),"worker covers later plots");
    shared.buyLand(2,0);shared.plant(2,0,CropId::Carrot);shared.tend(0,0);
    check(shared.tend(2,0)==Result::Cooldown,"cooldown shared across crops");
    cash=shared.balance(); shared.tick(60.0);
    check(shared.tile(2,0)->age==6 && close(shared.balance()-cash,720),"worker does not automate other crop");
    check(!shared.nextGoal().empty(),"next goal exists");
    session.requestRestart();session.cancelRestart();check(session.paused(),"cancel preserves pause");
    session.requestRestart();check(session.confirmRestart()&&session.world().balance()==1000&&!session.paused(),"restart resets session");
    return failures?1:0;
}
