#pragma once
#include "core.hpp"
#include <mysql.h>
namespace ps {
class Database {
    MYSQL* conn=nullptr;
public:
    explicit Database(const Settings& s,bool createSchema=true);
    ~Database(); Database(const Database&)=delete; Database& operator=(const Database&)=delete;
    void exec(const std::string& sql);
    std::vector<std::vector<std::optional<std::string>>> query(const std::string& sql);
    std::string quote(const std::string& s);
    uint64_t insertId()const;
    void schema();
    std::vector<League> leagues();
    std::vector<Team> teams(int league);
    std::vector<Match> matches(int league,bool future=false);
    std::vector<News> news(int team=0);
    void saveNews(const News& n);
    int league(int provider,const std::string& key,const std::string& name,const std::string& country);
    int team(int provider,const std::string& key,const std::string& name);
    long long upsertMatch(const Match& m,int provider);
};
struct PublicLeague {const char* key;const char* name;const char* country;};
extern const std::vector<PublicLeague> publicLeagues;
void importPublic(Database& db,const std::string& code,Job& job,bool network=true);
void importCustomCsv(Database& db,const fs::path& path,const std::string& leagueName,Job& job);
void importApiCatalog(Database& db,const Settings& s,Job& job);
void importApiHistory(Database& db,const Settings& s,const League& league,bool details,int maxRequests,Job& job);
void searchNews(Database& db,const Team& team,const std::string& query,const std::string& feed,Job& job);
void enrichPublicPlayers(Database& db,const std::string& publicCode,int requestBudget,Job& job,bool refreshExisting=false);
void refreshNewsSelection(Database& db);
void seedDemo(Database& db);
void checkDemo(Database& db,const Settings& settings);
}
