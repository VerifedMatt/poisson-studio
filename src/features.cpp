#include "core.hpp"
#include <set>
#include <sstream>
#include <iomanip>
#include <cctype>

namespace ps {
static std::string textValue(const json& j,const char* key) {
    if(!j.is_object()||!j.contains(key)||j[key].is_null())return {};
    return j[key].is_string()?j[key].get<std::string>():j[key].dump();
}
static std::optional<double> numeric(const json& j,const char* key) {
    try { auto s=textValue(j,key);size_t used=0;double v=std::stod(s,&used);
        if(used==s.size()&&std::isfinite(v))return v;
    }catch(...){}return {};
}
static int eventMinute(const json& event) {
    auto clock=event.value("clock",json::object());
    // Use football's nominal minute convention; added time is not counted twice.
    try {int n=std::stoi(textValue(clock,"displayValue"));if(n>=0&&n<=120)return n;}catch(...){}
    auto seconds=numeric(clock,"value");return seconds?std::clamp(int(std::ceil(*seconds/60)),0,120):-1;
}
PlayerMetrics parseEspnMetrics(const json& r,const json& doc) {
    PlayerMetrics out;
    for(auto& s:r.value("stats",json::array()))if(auto v=numeric(s,"value"))out.stats[textValue(s,"name")]=*v;
    for(auto key:{"minutesPlayed","minutes","totalMinutes"})if(out.stats.contains(key)) {
        double v=out.stats[key];if(v>=0&&v<=130){out.minutes=int(std::round(v));out.minutesSource=1;break;}
    }
    for(auto key:{"rating","playerRating","matchRating"}) {
        std::optional<double> v=out.stats.contains(key)?std::optional<double>(out.stats[key]):numeric(r,key);
        if(v&&*v>=0&&*v<=10){out.rating=*v;break;}
    }
    bool complete=false;int duration=90;
    auto header=doc.value("header",json::object());
    for(auto& c:header.value("competitions",json::array())) {
        auto status=c.value("status",json::object()).value("type",json::object());
        complete=status.value("completed",false);
        auto detail=textValue(status,"shortDetail");if(detail=="AET"||detail=="PEN")duration=120;
    }
    auto events=doc.value("keyEvents",json::array());
    for(auto& e:events)if(e.value("period",json::object()).value("number",0)==4)duration=120;
    if(out.minutes<0&&complete&&r.contains("starter")) {
        bool starter=r.value("starter",false),subIn=r.value("subbedIn",false),subOut=r.value("subbedOut",false);
        auto athlete=r.value("athlete",json::object());auto id=textValue(athlete,"id");
        int start=starter?0:-1,end=duration;bool foundIn=false,foundOut=false,foundRed=false;
        for(auto& e:events) {
            auto type=textValue(e.value("type",json::object()),"type");int minute=eventMinute(e);if(minute<0)continue;
            auto people=e.value("participants",json::array());
            if(type=="substitution"&&people.size()>=2) {
                if(textValue(people[0].value("athlete",json::object()),"id")==id&&subIn){start=minute;foundIn=true;}
                if(textValue(people[1].value("athlete",json::object()),"id")==id&&subOut){end=std::min(end,minute);foundOut=true;}
            }
            if(type=="red-card"||type=="yellow-red-card"||type=="second-yellow-card")for(auto& p:people)
                if(textValue(p.value("athlete",json::object()),"id")==id){end=std::min(end,minute);foundRed=true;}
        }
        for(auto& play:r.value("plays",json::array()))if(play.value("redCard",false)) {
            int minute=eventMinute(play);if(minute>=0){end=std::min(end,minute);foundRed=true;}
        }
        bool dismissal=out.stats.contains("redCards")&&out.stats.at("redCards")>0;
        if(!starter&&!subIn&&out.stats.contains("appearances")&&out.stats.at("appearances")==0)out.minutes=0;
        else if((starter||foundIn)&&(!subOut||foundOut)&&(!dismissal||foundRed)&&end>=start)out.minutes=std::clamp(end-start,0,duration);
        if(out.minutes>=0)out.minutesSource=2;
    }
    // A transparent local performance score, stored separately from provider ratings.
    // Missing essential observations or unused substitutes get no invented rating.
    bool enough=out.minutes>0;for(auto key:{"totalGoals","goalAssists","shotsOnTarget","yellowCards","redCards"})enough&=out.stats.contains(key);
    if(enough) {
        auto v=[&](const char* key){auto i=out.stats.find(key);return i==out.stats.end()?0.:i->second;};
        double rating=6+.85*v("totalGoals")+.55*v("goalAssists")+.12*v("shotsOnTarget")-.3*v("yellowCards")-1.2*v("redCards")-.7*v("ownGoals");
        if(textValue(r.value("position",json::object()),"abbreviation")=="G")rating+=.08*v("saves")-.25*v("goalsConceded");
        out.studioRating=std::round(std::clamp(rating,1.,10.)*10)/10;
    }
    return out;
}
static std::string lowercase(std::string s){for(auto& c:s)c=char(std::tolower(static_cast<unsigned char>(c)));return s;}
void selectRelevantNews(News& n,const std::string& teamName) {
    if(n.reviewed||n.decision!=0)return; // Never override a user's relevance decision.
    auto title=lowercase(n.title),team=lowercase(teamName);
    bool named=!team.empty()&&title.find(team)!=std::string::npos;
    const std::map<std::string,std::string> aliases={{"man united","manchester united"},{"man city","manchester city"},{"ath madrid","atletico madrid"},{"ath bilbao","athletic club"},{"paris sg","psg"},{"tottenham","spurs"},{"wolves","wolverhampton"}};
    if(auto it=aliases.find(team);it!=aliases.end())named|=title.find(it->second)!=std::string::npos;
    auto has=[&](const char* word){return title.find(word)!=std::string::npos;};
    bool injury=has("injur")||has("ruled out")||has("will miss"),suspension=has("suspend")||has("suspension")||has("red card")||has("match ban");
    bool positive=has("returns")||has("return to")||has("cleared to play")||has("back fit")||has("no injury")||has("not injured")||has("not suspended")||has("injury boost");
    bool irrelevant=has("women")||has("basketball")||has("transfer news")||has("last season")||has("injury time")||has("injury-time");
    n.selected=named&&(injury||suspension)&&!positive&&!irrelevant;
    n.category=suspension?"Suspension/cards":injury?"Injury":"Other";
    n.reason=!named?"Team not explicitly named in headline":positive?"Recovery/return headline; review before applying an absence":irrelevant?"Headline may concern another context":n.selected?"Team and availability keywords matched; confirm player, fixture and competition":"No clear injury or suspension signal";
    if(n.selected&&n.goalsFor==0&&n.goalsAgainst==0&&n.cornersFor==0&&n.cornersAgainst==0&&n.cards==0&&n.shotsFor==0&&n.shotsAgainst==0) {
        n.goalsFor=-3;n.goalsAgainst=2;n.shotsFor=-3;
    }
}
std::vector<BetCandidate> bettingCandidates(const Simulation& s,double goals,double corners,double cards,int cardMetric) {
    std::vector<BetCandidate> out;
    const auto& d=s.distributions[0];if(d.available&&d.n) {
        out.push_back({"Winner",s.homeName+" to win","winner:home",double(d.homeWins)/d.n});
        out.push_back({"Winner","Draw","winner:draw",double(d.draws)/d.n});
        out.push_back({"Winner",s.awayName+" to win","winner:away",double(d.awayWins)/d.n});
    }
    auto totals=[&](int metric,const std::string& name,double line) {
        const auto& d=s.distributions.at(metric);if(!d.available||!d.n)return;
        line=std::floor(line)+.5;uint64_t under=0;
        for(size_t i=0;i<d.total.size();++i)if(double(i)<line)under+=d.total[i];
        std::ostringstream label;label<<std::fixed<<std::setprecision(1)<<line;
        for(bool over:{false,true})out.push_back({name,std::string(over?"Over ":"Under ")+label.str(),name+":"+label.str()+":"+(over?"over":"under"),over?1.-double(under)/d.n:double(under)/d.n});
    };
    totals(0,"Goals",goals);totals(1,"Corners",corners);
    if(cardMetric<2||cardMetric>4)throw std::runtime_error("Invalid card market");
    totals(cardMetric,cardMetric==2?"Yellow cards":cardMetric==3?"Red cards":"Yellow + red",cards);
    return out;
}
void runFeatureTests() {
    auto check=[](bool yes,const char* what){if(!yes)throw std::runtime_error(what);};
    json doc={{"header",{{"competitions",json::array({{{"status",{{"type",{{"completed",true}}}}}}})}}},{"keyEvents",json::array()}};
    json r={{"athlete",{{"id","10"}}},{"starter",true},{"subbedIn",false},{"subbedOut",false},{"stats",json::array()}};
    for(auto key:{"totalGoals","goalAssists","shotsOnTarget","yellowCards","redCards"})r["stats"].push_back({{"name",key},{"value",0}});
    check(parseEspnMetrics(r,doc).minutes==90,"Full-match minutes");
    r["subbedOut"]=true;check(parseEspnMetrics(r,doc).minutes==-1,"Missing substitution must not invent minutes");
    doc["keyEvents"].push_back({{"type",{{"type","substitution"}}},{"clock",{{"displayValue","63'"}}},{"participants",json::array({{{"athlete",{{"id","20"}}}},{{"athlete",{{"id","10"}}}}})}});
    check(parseEspnMetrics(r,doc).minutes==63,"Outgoing minutes");
    r["athlete"]["id"]="20";r["starter"]=false;r["subbedOut"]=false;r["subbedIn"]=true;
    check(parseEspnMetrics(r,doc).minutes==27,"Incoming minutes");
    r["plays"]=json::array({{{"redCard",true},{"clock",{{"displayValue","80'"}}}}});
    check(parseEspnMetrics(r,doc).minutes==17,"Dismissal minutes");
    auto m=parseEspnMetrics(r,doc);check(!m.rating&&m.studioRating.has_value(),"Separate local and provider rating");
    r["rating"]="7.8";check(parseEspnMetrics(r,doc).rating==7.8,"String rating");r["rating"]=8.1;check(parseEspnMetrics(r,doc).rating==8.1,"Numeric rating");
    News n;n.title="Real Madrid defender ruled out with injury";selectRelevantNews(n,"Real Madrid");check(n.selected&&n.goalsFor<0,"Automatic news selection");
    n.decision=-1;n.selected=false;selectRelevantNews(n,"Real Madrid");check(!n.selected,"Rejected news stays rejected");
    n=News{};n.title="Barcelona injury update";selectRelevantNews(n,"Real Madrid");check(!n.selected,"Other team news excluded");
    n.title="Real Madrid injury-time winner";selectRelevantNews(n,"Real Madrid");check(!n.selected,"Injury time is not an injury");
    Simulation s;s.homeName="H";s.awayName="A";auto& d=s.distributions[0];d.available=true;d.n=100;d.homeWins=50;d.draws=30;d.awayWins=20;d.total={10,20,30,40};
    auto bets=bettingCandidates(s,2.5,9.5,4.5,2);check(bets.size()==5&&std::abs(bets[3].probability-.6)<1e-12&&std::abs(bets[4].probability-.4)<1e-12,"Total bet probabilities");
    check(bets[0].fairOdds()==2&&std::abs(bets[0].expectedReturn(2.2)-.1)<1e-12,"Fair odds and expected return");
}
}
