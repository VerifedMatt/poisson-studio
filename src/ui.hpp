#pragma once
#include "database.hpp"
#include "imgui.h"
#include <thread>
namespace ps {
struct App {
    Settings settings;std::unique_ptr<Database> db;Job job;std::thread worker;
    std::vector<League> leagues;std::vector<Team> teams;std::vector<Match> matches,fixtures;std::vector<News> news;
    std::optional<Simulation> result,pending;bool refresh=false,connected=false;int page=0,leagueIndex=0,homeIndex=0,awayIndex=1,metric=0,plotTab=0,forceTab=-1;
    int simulations=100000,seed=20261106,halfLife=365,goalMax=6,cornerMax=14,cardMax=8,shotMax=10,publicIndex=0,apiBudget=100,newsTeam=0,fixtureIndex=0;
    bool useNews=false,apiDetails=true,pngRequested=false,newsWarning=false,reviewQueue=true;int publicBudget=100;double ridge=5;float totalLine=2.5f;ImVec2 exportPos{},exportSize{};
    float betGoalLine=2.5f,betCornerLine=9.5f,betCardLine=4.5f;int betCardKind=0;
    std::map<std::string,float> offeredOdds;std::string oddsMatch;
    std::string pngPath,lastExport;char leagueFilter[160]{},customCsv[1024]{},customName[120]{},newsQuery[240]="injury suspension red card",newsFeed[1024]{},exportName[180]="heatmap";
    std::vector<std::vector<std::optional<std::string>>> playerRows,extraRows;std::string playerStatus;
    std::vector<LineupFixture> playerFixtures;bool playersOnly=true;char playerFixtureFilter[160]{};
    std::vector<CoverageRow> coverageRows;bool coverageDirty=true;std::string coverageStorage,coverageError;int coverageLeague=0;
    explicit App(const Settings& s);~App();
    void connect();void reload();void chooseLeague(int index);void run();
    void start(std::function<void()> action);void draw();void plot();
    void dataPage();void newsPage();void playersPage();void settingsPage();
    void loadPlayers();void newsToggle(const char* label);void searchMatchNews();void suggestions();
    void refreshPlayers(long long preferred=0);void coveragePage();void refreshCoverage();
};
}
