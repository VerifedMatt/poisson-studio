#include "database.hpp"

namespace ps {
void CoverageRow::add(const CoverageRow& r) {
    completed+=r.completed;rosters+=r.rosters;startingXI+=r.startingXI;checked+=r.checked;
    appearances+=r.appearances;minutes+=r.minutes;ratings+=r.ratings;playerShots+=r.playerShots;
    for(size_t i=0;i<metrics.size();++i)metrics[i]+=r.metrics[i];
    if(!r.first.empty()&&(first.empty()||r.first<first))first=r.first;
    if(r.last>last)last=r.last;
}

std::vector<LineupFixture> Database::lineupFixtures(int league) {
    // Browsing stored players is independent of the model's rolling training window.
    std::vector<LineupFixture> out;
    for(auto& r:query("SELECT f.id,f.date,h.name,a.name,COUNT(p.player_id) FROM fixture f "
        "JOIN team h ON h.id=f.home_id JOIN team a ON a.id=f.away_id "
        "LEFT JOIN appearance p ON p.fixture_id=f.id WHERE f.league_id="+std::to_string(league)+
        " GROUP BY f.id,f.date,h.name,a.name ORDER BY f.date DESC,f.id DESC"))
        out.push_back({std::stoll(*r[0]),*r[1],*r[2],*r[3],std::stoi(*r[4])});
    return out;
}

QueryRows Database::lineupPlayers(long long fixture) {
    return query("SELECT t.name,p.name,CASE WHEN a.starter IS NULL THEN 'Unknown' "
        "WHEN a.starter=1 THEN 'Starting XI' ELSE 'Bench' END,a.position,a.minutes,a.goals,a.assists,"
        "a.shots,a.shots_on,a.yellow,a.red,COALESCE(a.rating,a.studio_rating),p.id,a.minutes_source,a.rating "
        "FROM appearance a JOIN player p ON a.player_id=p.id JOIN team t ON a.team_id=t.id "
        "WHERE a.fixture_id="+std::to_string(fixture)+" ORDER BY t.name,a.starter DESC,a.shirt,p.name");
}

std::vector<CoverageRow> Database::coverage() {
    const auto start=quote(yearsAgo(5)),end=quote(today());
    // Aggregate appearances before joining to avoid multiplying fixture denominators.
    auto rows=query("SELECT l.id,l.country,l.name,COALESCE(f.season,0),COUNT(f.id),MIN(f.date),MAX(f.date),"
        "SUM(f.gh IS NOT NULL AND f.ga IS NOT NULL),SUM(f.ch IS NOT NULL AND f.ca IS NOT NULL),"
        "SUM(f.yh IS NOT NULL AND f.ya IS NOT NULL),SUM(f.rh IS NOT NULL AND f.ra IS NOT NULL),"
        "SUM(h.shots_on IS NOT NULL AND a.shots_on IS NOT NULL AND f.status='FT'),"
        "SUM(COALESCE(p.n,0)>0),SUM(COALESCE(p.home_xi,0)=11 AND COALESCE(p.away_xi,0)=11),"
        "SUM((f.provider=0 AND (f.detail_flags & 32)=32) OR (f.provider=1 AND (f.detail_flags & 6)=6)),"
        "SUM(p.n),SUM(p.minutes),SUM(p.ratings),SUM(p.shots) "
        "FROM league l LEFT JOIN fixture f ON f.league_id=l.id AND f.date>="+start+" AND f.date<="+end+
        " AND f.status IN ('FT','AET','PEN') "
        "LEFT JOIN team_match_extra h ON h.fixture_id=f.id AND h.team_id=f.home_id "
        "LEFT JOIN team_match_extra a ON a.fixture_id=f.id AND a.team_id=f.away_id "
        "LEFT JOIN (SELECT x.fixture_id,COUNT(*) n,SUM(x.starter=1 AND x.team_id=g.home_id) home_xi,"
        "SUM(x.starter=1 AND x.team_id=g.away_id) away_xi,SUM(x.minutes IS NOT NULL) minutes,"
        "SUM(x.rating IS NOT NULL OR x.studio_rating IS NOT NULL) ratings,SUM(x.shots_on IS NOT NULL) shots "
        "FROM appearance x JOIN fixture g ON g.id=x.fixture_id WHERE g.date>="+start+" AND g.date<="+end+
        " AND g.status IN ('FT','AET','PEN') GROUP BY x.fixture_id) p ON p.fixture_id=f.id "
        "GROUP BY l.id,l.country,l.name,f.season ORDER BY l.country,l.name,f.season DESC");
    std::vector<CoverageRow> out;
    for(auto& r:rows) {
        auto count=[&](size_t i){return r[i]?std::stoull(*r[i]):0ull;};
        CoverageRow c;c.league=int(count(0));c.country=*r[1];c.name=*r[2];c.season=int(count(3));
        c.completed=count(4);c.first=r[5].value_or("");c.last=r[6].value_or("");
        for(size_t i=0;i<5;++i)c.metrics[i]=count(7+i);
        c.rosters=count(12);c.startingXI=count(13);c.checked=count(14);c.appearances=count(15);
        c.minutes=count(16);c.ratings=count(17);c.playerShots=count(18);out.push_back(c);
    }
    return out;
}
}
