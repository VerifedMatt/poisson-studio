#include "ui.hpp"

namespace ps {
static std::string fraction(uint64_t count,uint64_t total) {
    if(!total)return "N/A (no records)";
    char value[100];snprintf(value,sizeof(value),"%llu / %llu (%.1f%%)",
        (unsigned long long)count,(unsigned long long)total,100.*count/total);return value;
}

void App::refreshCoverage() {
    coverageDirty=false;coverageError.clear();
    if(!db||!connected)return;
    try {
        auto rows=db->coverage();
        auto size=db->query("SELECT (SELECT COUNT(*) FROM player),(SELECT COUNT(*) FROM appearance),"
            "(SELECT COUNT(*) FROM match_event),(SELECT ROUND(SUM(data_length+index_length)/1024/1024,2) "
            "FROM information_schema.tables WHERE table_schema=DATABASE()),@@datadir,@@port");
        coverageRows=std::move(rows);
        coverageStorage="Entire database: "+*size[0][0]+" players | "+*size[0][1]+" appearances | "+*size[0][2]+
            " events | Estimated tables/indexes: "+size[0][3].value_or("0")+" MB\nConnected database: "+*size[0][4]+" (port "+*size[0][5]+")";
    }catch(const std::exception& e){coverageRows.clear();coverageStorage.clear();coverageError=e.what();job.fail(e.what());}
}

void App::coveragePage() {
    ImGui::Text("Database coverage");
    if(!connected){ImGui::TextWrapped("Connect to your database in Settings to view coverage.");return;}
    ImGui::BeginDisabled(job.busy);if(ImGui::Button("Refresh coverage"))coverageDirty=true;ImGui::EndDisabled();
    ImGui::SameLine();ImGui::TextDisabled("Rolling window: %s through %s",yearsAgo(5).c_str(),today().c_str());
    if(coverageDirty&&!job.busy)refreshCoverage();
    if(job.busy)ImGui::TextWrapped("Import or simulation running. Coverage refreshes when the job ends; current counts are the last snapshot.");
    if(!coverageError.empty()){ImGui::TextWrapped("Could not read coverage: %s",coverageError.c_str());return;}
    ImGui::TextWrapped("%s",coverageStorage.c_str());
    ImGui::TextWrapped("Percentages measure fields recorded for stored finished matches, not every match that occurred. The source's total five-year fixture count is unknown. Future fixtures are excluded; zero is a recorded value.");

    std::vector<CoverageRow> grouped;
    for(auto& row:coverageRows){auto it=std::find_if(grouped.begin(),grouped.end(),[&](auto& c){return c.league==row.league;});
        if(it==grouped.end()){CoverageRow c;c.league=row.league;c.country=row.country;c.name=row.name;grouped.push_back(c);it=grouped.end()-1;}it->add(row);}
    if(coverageLeague&&std::none_of(grouped.begin(),grouped.end(),[&](auto& r){return r.league==coverageLeague;}))coverageLeague=0;
    std::string selected="All stored competitions";for(auto& c:grouped)if(c.league==coverageLeague)selected=c.country+" / "+c.name;
    ImGui::SetNextItemWidth(500);if(ImGui::BeginCombo("Coverage scope",selected.c_str())){
        if(ImGui::Selectable("All stored competitions",coverageLeague==0))coverageLeague=0;
        for(auto& c:grouped)if(ImGui::Selectable((c.country+" / "+c.name+"##"+std::to_string(c.league)).c_str(),coverageLeague==c.league))coverageLeague=c.league;
        ImGui::EndCombo();
    }
    CoverageRow total;for(auto& c:grouped)if(!coverageLeague||c.league==coverageLeague)total.add(c);
    ImGui::Text("%llu finished matches | Recorded dates: %s to %s",(unsigned long long)total.completed,total.first.empty()?"--":total.first.c_str(),total.last.empty()?"--":total.last.c_str());
    ImGui::PushStyleVar(ImGuiStyleVar_CellPadding,ImVec2(8,5));
    ImGui::PushStyleColor(ImGuiCol_PlotHistogram,ImVec4(.24f,.40f,.48f,1));
    if(ImGui::BeginTable("Coverage progress",3,ImGuiTableFlags_RowBg|ImGuiTableFlags_BordersInnerH)){
        ImGui::TableSetupColumn("Recorded data",ImGuiTableColumnFlags_WidthFixed,250);
        ImGui::TableSetupColumn("Coverage (count / denominator)");ImGui::TableSetupColumn("Missing",ImGuiTableColumnFlags_WidthFixed,100);ImGui::TableHeadersRow();
        auto bar=[&](const char* name,uint64_t count,uint64_t denominator){ImGui::TableNextRow();ImGui::TableSetColumnIndex(0);ImGui::TextUnformatted(name);
            ImGui::TableSetColumnIndex(1);ImGui::ProgressBar(denominator?float(double(count)/denominator):0,ImVec2(-1,20),fraction(count,denominator).c_str());
            ImGui::TableSetColumnIndex(2);if(denominator)ImGui::Text("%llu",(unsigned long long)(denominator-count));else ImGui::TextDisabled("--");};
        const char* names[]={"Goals (both teams)","Corners (both teams)","Yellow cards (both teams)","Red cards (both teams)","Shots on target (both teams)"};
        for(int i=0;i<5;++i)bar(names[i],total.metrics[i],total.completed);
        bar("Any player records",total.rosters,total.completed);bar("Both starting XIs (11 + 11)",total.startingXI,total.completed);
        bar("Minutes / stored appearances",total.minutes,total.appearances);bar("Ratings / stored appearances",total.ratings,total.appearances);bar("On target / stored appearances",total.playerShots,total.appearances);
        ImGui::EndTable();
    }
    ImGui::PopStyleColor();ImGui::PopStyleVar();
    ImGui::TextWrapped("Ratings include provider and Studio scores; unused substitutes can be unrated. Player fields use stored appearances as their denominator. Both starting XIs requires exactly 11 identified starters per team, not merely a nonempty roster.");
    ImGui::TextWrapped("Player detail requests attempted for %llu finished matches. An empty provider response counts as attempted, not as a completed lineup.",(unsigned long long)total.checked);
    if(coverageLeague){ImGui::BeginDisabled(job.busy);if(ImGui::Button("Browse this competition's players")){for(int i=0;i<int(leagues.size());++i)if(leagues[i].id==coverageLeague){chooseLeague(i);page=3;break;}}ImGui::EndDisabled();}

    ImGui::SeparatorText(coverageLeague?"BY SEASON (WITHIN THE FIVE-YEAR WINDOW)":"BY COMPETITION (SELECT A SCOPE FOR SEASONS)");
    if(ImGui::BeginTable("Coverage breakdown",9,ImGuiTableFlags_Borders|ImGuiTableFlags_RowBg|ImGuiTableFlags_ScrollY|ImGuiTableFlags_ScrollX|ImGuiTableFlags_Resizable,ImVec2(0,220))){
        ImGui::TableSetupColumn(coverageLeague?"Season":"Competition",ImGuiTableColumnFlags_WidthFixed,260);
        for(auto h:{"Finished","Goals","Corners","Yellow","Red","On target","Players","Both XIs"})ImGui::TableSetupColumn(h,ImGuiTableColumnFlags_WidthFixed,155);
        ImGui::TableSetupScrollFreeze(1,1);ImGui::TableHeadersRow();
        bool bySeason=coverageLeague!=0;int shownLeague=coverageLeague;
        auto row=[&](const CoverageRow& c){ImGui::TableNextRow();ImGui::TableSetColumnIndex(0);
            std::string name=bySeason?(c.season?std::to_string(c.season):"No finished matches"):c.country+" / "+c.name;
            if(bySeason)ImGui::TextUnformatted(name.c_str());else if(ImGui::Selectable((name+"##"+std::to_string(c.league)).c_str()))coverageLeague=c.league;
            ImGui::TableSetColumnIndex(1);ImGui::Text("%llu",(unsigned long long)c.completed);
            for(int i=0;i<5;++i){ImGui::TableSetColumnIndex(i+2);ImGui::TextUnformatted(fraction(c.metrics[i],c.completed).c_str());}
            ImGui::TableSetColumnIndex(7);ImGui::TextUnformatted(fraction(c.rosters,c.completed).c_str());ImGui::TableSetColumnIndex(8);ImGui::TextUnformatted(fraction(c.startingXI,c.completed).c_str());};
        // Keep the table source stable even if a row changes next frame's scope.
        if(bySeason){for(auto& c:coverageRows)if(c.league==shownLeague)row(c);}else for(auto& c:grouped)row(c);
        ImGui::EndTable();
    }
}
}
