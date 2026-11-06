#pragma once
#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <map>
#include <mutex>
#include <numeric>
#include <optional>
#include <random>
#include <stdexcept>
#include <string>
#include <vector>
#include "json.hpp"

namespace ps {
using json=nlohmann::json;
namespace fs=std::filesystem;
struct League {int id=0; std::string key,name,country; int provider=0;};
struct Team {int id=0; std::string name;};
struct Match {
    long long id=0,external=0; int league=0,home=0,away=0,season=0,details=0;
    std::string date,status,homeName,awayName;
    // goals, corners, yellow cards, red cards, shots on target; -1 means missing.
    std::array<int,5> h{-1,-1,-1,-1,-1},a{-1,-1,-1,-1,-1};
};
struct News {
    long long id=0; int team=0; bool reviewed=false,selected=false; int decision=0;
    std::string title,url,published,category,expires,reason;
    float goalsFor=0,goalsAgainst=0,cornersFor=0,cornersAgainst=0,cards=0,shotsFor=0,shotsAgainst=0;
};
struct Settings {
    std::string host="127.0.0.1",user="poisson_app",database="poisson_studio",password,apiKey;
    int port=3317; bool autoStart=true; int apiDelayMs=6500;
};
struct Job {
    std::atomic<bool> busy{false},cancel{false}; std::atomic<double> progress{0};
    mutable std::mutex mutex; std::string status="Ready",error;
    void message(const std::string& s){std::lock_guard<std::mutex> l(mutex);status=s;}
    void fail(const std::string& s){std::lock_guard<std::mutex> l(mutex);error=s;status="Failed";}
    void check()const{if(cancel)throw std::runtime_error("Cancelled; completed imports are retained.");}
};
int dayNumber(const std::string& iso);
std::string today(),yearsAgo(int years),addDays(const std::string& d,int n),csvDate(const std::string& s);
std::string readFile(const fs::path& p);
void writeFile(const fs::path& p,const std::string& s);
std::vector<std::vector<std::string>> parseCsv(const std::string& s);
std::string urlEncode(const std::string& s);
std::string httpGet(const std::string& url,const std::string& headers="");
Settings loadSettings(); void saveSettings(const Settings& s);
void initializePrivateDatabase(int port=3317); bool startPrivateDatabase(const Settings& s);
void stopPrivateDatabase(const Settings& s);

struct Fit {
    bool available=false; double home=0,away=0,dispersion=0;
    int matches=0,homeSamples=0,awaySamples=0; std::string note;
};
struct Distribution {
    bool available=false; std::string name,note;
    int bins=0; std::vector<uint64_t> joint,total; std::vector<uint64_t> home,away;
    std::map<std::pair<int,int>,uint64_t> exact;
    uint64_t n=0,homeWins=0,draws=0,awayWins=0;
    double lambdaHome=0,lambdaAway=0,dispersion=0;
};
struct Simulation {
    uint64_t requested=100000,seed=20261106; int halfLife=365,maxGoals=6,maxCorners=14,maxCards=8,maxShots=10;
    double ridge=5; bool useNews=false; std::string cutoff;
    int homeId=0,awayId=0; std::string homeName,awayName,leagueName;
    std::array<Distribution,6> distributions; // goals,corners,yellow,red,all cards,shots on target
    int appliedNews=0,pendingNews=0;
};
Fit fitPoisson(const std::vector<Match>& rows,int metric,int home,int away,const std::string& cutoff,int halfLife,double ridge,Job* job=nullptr);
Simulation simulate(Simulation config,const std::vector<Match>& rows,const std::vector<News>& news,Job& job);
void exportCsv(const Simulation& result,int metric,const fs::path& path);
bool runCoreTests(std::string& result);
struct PlayerMetrics {
    std::map<std::string,double> stats;
    int minutes=-1,minutesSource=0; // 1: supplied, 2: event-derived
    std::optional<double> rating,studioRating;
};
PlayerMetrics parseEspnMetrics(const json& roster,const json& document);
void selectRelevantNews(News& n,const std::string& teamName);
struct BetCandidate {
    std::string group,pick,key; double probability=0;
    double fairOdds() const {return probability>0?1/probability:0;}
    double expectedReturn(double decimalOdds) const {return probability*decimalOdds-1;}
};
std::vector<BetCandidate> bettingCandidates(const Simulation& s,double goalLine,double cornerLine,double cardLine,int cardMetric);
void runFeatureTests();
}
