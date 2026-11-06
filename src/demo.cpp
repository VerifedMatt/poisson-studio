#include "database.hpp"
namespace ps {
void seedDemo(Database& db) {
    int lid=db.league(2,"_demo_v1","Demo league (synthetic)","Demo");
    if(!db.query("SELECT id FROM fixture WHERE league_id="+std::to_string(lid)+" LIMIT 1").empty())return;
    const char* names[]={"Northbridge FC","Riverside United","Eastford City","Westhaven Athletic"};
    int teams[4];for(int i=0;i<4;++i)teams[i]=db.team(2,"_demo:"+std::to_string(i),names[i]);
    std::mt19937 rng(42017);std::poisson_distribution<int> goals(1.45),corners(5),yellow(2),red(.12),shots(4.7);
    long long latest=0;db.exec("START TRANSACTION");
    try{
        for(int i=0;i<240;++i){Match m;m.league=lid;m.date=addDays(today(),-240+i);m.season=std::stoi(m.date.substr(0,4));m.status="FT";m.home=teams[i%4];m.away=teams[(i%4+1+(i/4)%3)%4];
            for(bool home:{true,false}){auto& counts=home?m.h:m.a;counts={goals(rng),corners(rng),yellow(rng),red(rng),shots(rng)};}
            latest=db.upsertMatch(m,2);
        }
        Match next;next.league=lid;next.date=addDays(today(),1);next.season=std::stoi(next.date.substr(0,4));next.status="NS";next.home=teams[0];next.away=teams[1];db.upsertMatch(next,2);
        auto rows=db.query("SELECT home_id,away_id FROM fixture WHERE id="+std::to_string(latest));
        for(int i=0;i<4;++i){db.exec("INSERT INTO player(provider,external_id,name) VALUES(2,"+std::to_string(900001+i)+","+db.quote("Demo Player "+std::to_string(i+1))+") ON DUPLICATE KEY UPDATE id=LAST_INSERT_ID(id)");auto pid=db.insertId();
            db.exec("INSERT INTO appearance(fixture_id,team_id,player_id,starter,position,minutes,minutes_source,goals,assists,shots,shots_on,yellow,red,studio_rating) VALUES("+std::to_string(latest)+","+*rows[0][i/2]+","+std::to_string(pid)+",1,'MF',90,2,0,0,2,1,0,0,6.1) ON DUPLICATE KEY UPDATE minutes=VALUES(minutes)");
        }
        News n;n.team=teams[1];n.title="Synthetic example: Riverside United midfielder ruled out with injury";n.category="Demo";n.url="demo://availability";n.published=today();n.expires=addDays(today(),7);selectRelevantNews(n,names[1]);n.reason="Synthetic example for testing relevance controls; not real news";db.saveNews(n);
        db.exec("COMMIT");
    }catch(...){db.exec("ROLLBACK");throw;}
}
void checkDemo(Database& db,const Settings& settings){
    auto location=db.query("SELECT @@datadir,@@port");auto actual=fs::weakly_canonical(fs::path(*location[0][0]));auto expected=fs::weakly_canonical(fs::absolute("data/mysql"));
    if(!fs::equivalent(actual,expected)||std::stoi(*location[0][1])!=settings.port)throw std::runtime_error("Integration check connected to a different database instance.");
    int league=0;for(auto& l:db.leagues())if(l.provider==2&&l.key=="_demo_v1")league=l.id;
    if(!league)throw std::runtime_error("Run --seed-demo first");auto rows=db.matches(league);if(rows.size()!=241)throw std::runtime_error("Unexpected demo fixture count");
    Simulation c;c.homeId=rows[0].home;c.awayId=rows[0].away;c.requested=10000;c.leagueName="Demo league (synthetic)";c.homeName="Demo home";c.awayName="Demo away";Job job;auto result=simulate(c,rows,{},job);
    for(int metric=0;metric<6;++metric){auto& d=result.distributions[metric];if(!d.available||d.n!=c.requested)throw std::runtime_error("Missing demo distribution");exportCsv(result,metric,"exports/demo_"+std::to_string(metric)+".csv");}
}
}
