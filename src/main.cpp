#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <d3d11.h>
#include <dxgi.h>
#include "imgui.h"
#include "backends/imgui_impl_win32.h"
#include "backends/imgui_impl_dx11.h"
#include "ui.hpp"
#include <iostream>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

static ID3D11Device* device=nullptr;static ID3D11DeviceContext* context=nullptr;
static IDXGISwapChain* swapChain=nullptr;static ID3D11RenderTargetView* target=nullptr;
static void createTarget(){ID3D11Texture2D* back=nullptr;if(SUCCEEDED(swapChain->GetBuffer(0,IID_PPV_ARGS(&back)))){device->CreateRenderTargetView(back,nullptr,&target);back->Release();}}
static void destroyTarget(){if(target){target->Release();target=nullptr;}}
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND,UINT,WPARAM,LPARAM);
static LRESULT WINAPI wndProc(HWND w,UINT msg,WPARAM wp,LPARAM lp){if(ImGui_ImplWin32_WndProcHandler(w,msg,wp,lp))return true;switch(msg){case WM_SIZE:if(device&&wp!=SIZE_MINIMIZED){destroyTarget();swapChain->ResizeBuffers(0,LOWORD(lp),HIWORD(lp),DXGI_FORMAT_UNKNOWN,0);createTarget();}return 0;case WM_GETMINMAXINFO:{auto* m=reinterpret_cast<MINMAXINFO*>(lp);m->ptMinTrackSize.x=1080;m->ptMinTrackSize.y=800;return 0;}case WM_SYSCOMMAND:if((wp&0xfff0)==SC_KEYMENU)return 0;break;case WM_DESTROY:PostQuitMessage(0);return 0;}return DefWindowProcW(w,msg,wp,lp);}
static void savePng(const std::string& path,int x,int y,int width,int height){
    ID3D11Texture2D* source=nullptr;if(FAILED(swapChain->GetBuffer(0,IID_PPV_ARGS(&source))))throw std::runtime_error("Cannot access render buffer");D3D11_TEXTURE2D_DESC desc{};source->GetDesc(&desc);x=std::clamp(x,0,int(desc.Width)-1);y=std::clamp(y,0,int(desc.Height)-1);width=std::clamp(width,1,int(desc.Width)-x);height=std::clamp(height,1,int(desc.Height)-y);desc.Usage=D3D11_USAGE_STAGING;desc.BindFlags=0;desc.CPUAccessFlags=D3D11_CPU_ACCESS_READ;desc.MiscFlags=0;ID3D11Texture2D* staging=nullptr;
    if(FAILED(device->CreateTexture2D(&desc,nullptr,&staging))){source->Release();throw std::runtime_error("Cannot allocate export buffer");}context->CopyResource(staging,source);source->Release();D3D11_MAPPED_SUBRESOURCE map{};if(FAILED(context->Map(staging,0,D3D11_MAP_READ,0,&map))){staging->Release();throw std::runtime_error("Cannot read export buffer");}
    std::vector<unsigned char> pixels(size_t(width)*height*4);for(int row=0;row<height;++row)memcpy(pixels.data()+size_t(row)*width*4,static_cast<unsigned char*>(map.pData)+(y+row)*map.RowPitch+x*4,size_t(width)*4);context->Unmap(staging,0);staging->Release();ps::fs::create_directories(ps::fs::path(path).parent_path());if(!stbi_write_png(path.c_str(),width,height,4,pixels.data(),width*4))throw std::runtime_error("PNG write failed");
}
static int gui(ps::Settings settings,bool smoke){
    ImGui_ImplWin32_EnableDpiAwareness();WNDCLASSEXW wc{sizeof(wc),CS_CLASSDC,wndProc,0,0,GetModuleHandleW(nullptr),nullptr,nullptr,nullptr,nullptr,L"PoissonStudioWindow",nullptr};RegisterClassExW(&wc);
    HWND window=CreateWindowW(wc.lpszClassName,L"Poisson Studio | Football Probability Lab",WS_OVERLAPPEDWINDOW,80,30,1500,970,nullptr,nullptr,wc.hInstance,nullptr);
    DXGI_SWAP_CHAIN_DESC desc{};desc.BufferCount=2;desc.BufferDesc.Format=DXGI_FORMAT_R8G8B8A8_UNORM;desc.BufferUsage=DXGI_USAGE_RENDER_TARGET_OUTPUT;desc.OutputWindow=window;desc.SampleDesc.Count=1;desc.Windowed=TRUE;desc.SwapEffect=DXGI_SWAP_EFFECT_DISCARD;
    wchar_t software[8]{};bool useSoftware=GetEnvironmentVariableW(L"POISSON_SOFTWARE_RENDERER",software,8)&&software[0]==L'1';D3D_FEATURE_LEVEL level;D3D_FEATURE_LEVEL levels[]={D3D_FEATURE_LEVEL_11_0,D3D_FEATURE_LEVEL_10_0};HRESULT hr=D3D11CreateDeviceAndSwapChain(nullptr,useSoftware?D3D_DRIVER_TYPE_WARP:D3D_DRIVER_TYPE_HARDWARE,nullptr,0,levels,2,D3D11_SDK_VERSION,&desc,&swapChain,&device,&level,&context);
    if(FAILED(hr))hr=D3D11CreateDeviceAndSwapChain(nullptr,D3D_DRIVER_TYPE_WARP,nullptr,0,levels,2,D3D11_SDK_VERSION,&desc,&swapChain,&device,&level,&context);if(FAILED(hr))throw std::runtime_error("DirectX 11 initialization failed");createTarget();
    IMGUI_CHECKVERSION();ImGui::CreateContext();auto& io=ImGui::GetIO();io.ConfigFlags|=ImGuiConfigFlags_NavEnableKeyboard;io.IniFilename=nullptr;
    static const ImWchar ranges[]={0x0020,0x024f,0x0370,0x052f,0x2000,0x206f,0x20a0,0x20cf,0};
    const char* font="assets/Inter.ttf";
    ImFontConfig fontConfig;fontConfig.OversampleH=3;fontConfig.OversampleV=2;
    if(ps::fs::exists(font)){io.Fonts->AddFontFromFileTTF(font,18,&fontConfig,ranges);io.Fonts->AddFontFromFileTTF(font,23,&fontConfig,ranges);}else io.Fonts->AddFontDefault();
    ImGui::StyleColorsDark();auto& style=ImGui::GetStyle();style.WindowPadding=ImVec2(16,12);style.FramePadding=ImVec2(9,7);style.ItemSpacing=ImVec2(9,9);style.FrameRounding=6;style.ChildRounding=8;style.GrabRounding=5;style.PopupRounding=8;style.ScrollbarRounding=6;style.CellPadding=ImVec2(8,8);
    auto rgba=[](int r,int g,int b,float a=1.f){return ImVec4(r/255.f,g/255.f,b/255.f,a);};auto* color=style.Colors;
    color[ImGuiCol_Text]=rgba(231,237,249);color[ImGuiCol_TextDisabled]=rgba(147,162,187);
    color[ImGuiCol_WindowBg]=rgba(16,21,32);color[ImGuiCol_ChildBg]=rgba(23,31,46);color[ImGuiCol_PopupBg]=rgba(29,38,55);color[ImGuiCol_Border]=rgba(55,69,91);
    color[ImGuiCol_FrameBg]=rgba(34,45,65);color[ImGuiCol_FrameBgHovered]=rgba(45,60,84);color[ImGuiCol_FrameBgActive]=rgba(51,71,101);
    color[ImGuiCol_Button]=rgba(47,63,101);color[ImGuiCol_ButtonHovered]=rgba(70,88,145);color[ImGuiCol_ButtonActive]=rgba(89,106,168);
    color[ImGuiCol_Header]=rgba(49,65,108);color[ImGuiCol_HeaderHovered]=rgba(64,83,132);color[ImGuiCol_HeaderActive]=rgba(77,98,153);
    color[ImGuiCol_CheckMark]=rgba(137,166,255);color[ImGuiCol_SliderGrab]=rgba(123,148,235);color[ImGuiCol_SliderGrabActive]=rgba(164,184,255);
    color[ImGuiCol_Tab]=rgba(34,44,65);color[ImGuiCol_TabHovered]=rgba(65,84,132);color[ImGuiCol_TabSelected]=rgba(57,76,121);color[ImGuiCol_TabSelectedOverline]=rgba(137,166,255);
    color[ImGuiCol_Separator]=rgba(55,69,91);color[ImGuiCol_TableHeaderBg]=rgba(32,44,65);color[ImGuiCol_TableRowBgAlt]=rgba(74,97,135,.10f);
    color[ImGuiCol_TitleBg]=rgba(19,26,40);color[ImGuiCol_TitleBgActive]=rgba(38,52,81);color[ImGuiCol_NavCursor]=rgba(137,166,255);
    ImGui_ImplWin32_Init(window);ImGui_ImplDX11_Init(device,context);ShowWindow(window,smoke?SW_HIDE:SW_SHOWNORMAL);UpdateWindow(window);
    int smokeStage=0,settle=0;bool done=false;{
        ps::App app(settings);if(smoke){bool hasDemo=false;for(int i=0;i<int(app.leagues.size());++i)if(app.leagues[i].key=="_demo_v1"&&app.leagues[i].provider==2){app.chooseLeague(i);hasDemo=true;}if(!hasDemo)throw std::runtime_error("Run --seed-demo before the graphics smoke test");for(int i=0;i<int(app.teams.size());++i){if(app.teams[i].name=="Northbridge FC")app.homeIndex=i;if(app.teams[i].name=="Riverside United")app.awayIndex=i;}app.run();}
        while(!done){MSG msg;while(PeekMessageW(&msg,nullptr,0,0,PM_REMOVE)){TranslateMessage(&msg);DispatchMessageW(&msg);if(msg.message==WM_QUIT)done=true;}if(done)break;if(IsIconic(window)){Sleep(20);continue;}
            if(smoke&&app.result&&!app.job.busy){if(smokeStage<6){app.page=0;app.forceTab=smokeStage;}else if(smokeStage==6){app.page=3;if(settle==0)app.loadPlayers();}else if(smokeStage==7){app.page=2;app.newsTeam=app.awayIndex;app.reviewQueue=false;}else{app.page=0;if(settle==0)app.newsWarning=true;}if(++settle==8){app.pngRequested=true;app.pngPath=ps::fs::absolute("exports/smoke_"+std::to_string(smokeStage)+".png").string();}}
            ImGui_ImplDX11_NewFrame();ImGui_ImplWin32_NewFrame();ImGui::NewFrame();try{app.draw();}catch(const std::exception&e){app.job.fail(e.what());}ImGui::Render();const float clear[]={.055f,.075f,.105f,1};context->OMSetRenderTargets(1,&target,nullptr);context->ClearRenderTargetView(target,clear);ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
            if(app.pngRequested){try{auto pos=ImGui::GetMainViewport()->Pos;savePng(app.pngPath,int(app.exportPos.x-pos.x),int(app.exportPos.y-pos.y),int(app.exportSize.x),int(app.exportSize.y));app.lastExport=app.pngPath;app.job.message("PNG exported: "+app.pngPath);if(app.result){auto meta=ps::json{{"home",app.result->homeName},{"away",app.result->awayName},{"simulations",app.result->requested},{"news",app.result->useNews},{"metric",app.plotTab==1?"Total goals":app.result->distributions[app.metric].name},{"seed",app.result->seed}};ps::writeFile(app.pngPath+".json",meta.dump(2));}if(smoke){if(smokeStage==0){DXGI_SWAP_CHAIN_DESC d{};swapChain->GetDesc(&d);savePng(ps::fs::absolute("exports/app_screenshot.png").string(),0,0,d.BufferDesc.Width,d.BufferDesc.Height);}++smokeStage;settle=0;if(smokeStage==9)done=true;}}catch(const std::exception&e){app.job.fail(e.what());if(smoke)done=true;}app.pngRequested=false;}
            swapChain->Present(1,0);
            if(smoke&&!app.job.busy&&!app.result&&!app.pending){std::lock_guard<std::mutex> l(app.job.mutex);if(!app.job.error.empty()){std::cerr<<app.job.error<<"\n";done=true;}}
        }
    }
    ImGui_ImplDX11_Shutdown();ImGui_ImplWin32_Shutdown();ImGui::DestroyContext();destroyTarget();swapChain->Release();context->Release();device->Release();DestroyWindow(window);UnregisterClassW(wc.lpszClassName,wc.hInstance);return smoke&&smokeStage<9?1:0;
}
int main(int argc,char** argv){try{
    wchar_t path[32768];GetModuleFileNameW(nullptr,path,32768);ps::fs::current_path(ps::fs::path(path).parent_path());std::string mode=argc>1?argv[1]:"";
    if(mode=="--help"){std::cout<<"Poisson Studio - Windows x64\n\nSetup: --setup-db [PORT]\nOffline demo: --seed-demo\nChecks: --self-test, --db-check, --integration-check, --smoke, --coverage-v2\nImports: --import-league CODE, --import-all, --import-cache\nCSV: --import-csv PATH LEAGUE_NAME\nPlayers: --enrich CODE REQUESTS, --refresh-lineups CODE REQUESTS\nNews: --search-news CODE TEAM, --refresh-news-selection\nForecast: --simulate CODE HOME AWAY DRAWS\nOther: --version, --shutdown-db\n\nUse scripts/run.ps1 to wait for CLI completion in PowerShell.\n";return 0;}
    if(mode=="--version"){std::cout<<"Poisson Studio "<<(ps::fs::exists("VERSION")?ps::readFile("VERSION"):"development\n");return 0;}
    const std::vector<std::string> commands={"","--setup-db","--self-test","--seed-demo","--integration-check","--smoke","--db-check","--coverage-v2","--import-league","--import-csv","--import-all","--import-cache","--enrich","--refresh-lineups","--search-news","--refresh-news-selection","--simulate","--shutdown-db"};
    if(std::find(commands.begin(),commands.end(),mode)==commands.end())throw std::runtime_error("Unknown command. Use --help.");
    if(mode=="--self-test"){std::string result;bool ok=ps::runCoreTests(result);std::cout<<result<<"\n";ps::writeFile("tests/results.txt",result);return ok?0:1;}
    if(mode=="--setup-db"){int port=argc>2?std::stoi(argv[2]):3317;ps::initializePrivateDatabase(port);std::cout<<"Private MySQL initialized on 127.0.0.1:"<<port<<"; new credentials encrypted for this Windows user.\n";return 0;}
    auto settings=ps::loadSettings();bool owned=false;try{owned=ps::startPrivateDatabase(settings);}catch(const std::exception&e){if(!mode.empty())throw;MessageBoxA(nullptr,e.what(),"MySQL setup / connection",MB_OK|MB_ICONWARNING);}
    struct ServerGuard{const ps::Settings& settings;bool& owned;~ServerGuard(){if(owned)try{ps::stopPrivateDatabase(settings);}catch(...){}}} serverGuard{settings,owned};
    int code=0;
    if(mode=="--seed-demo"){ps::Database db(settings);ps::seedDemo(db);std::cout<<"Synthetic demo league ready; no real-world data or API credentials required.\n";}
    else if(mode=="--integration-check"){ps::Database db(settings);ps::checkDemo(db,settings);std::cout<<"Private database location, demo data, six fitted distributions and CSV exports passed.\n";}
    else if(mode=="--import-league"){if(argc<3)throw std::runtime_error("Usage: --import-league CODE");ps::Database db(settings);ps::Job job;ps::importPublic(db,argv[2],job,true);std::cout<<job.status<<"\n";}
    else if(mode=="--import-csv"){if(argc<4)throw std::runtime_error("Usage: --import-csv PATH LEAGUE_NAME");ps::Database db(settings);ps::Job job;ps::importCustomCsv(db,argv[2],argv[3],job);std::cout<<job.status<<"\n";}
    else if(mode=="--import-cache"||mode=="--import-all"){ps::Database db(settings);ps::Job job;int failures=0;for(auto& l:ps::publicLeagues){try{ps::importPublic(db,l.key,job,mode=="--import-all");std::cout<<job.status<<std::endl;}catch(const std::exception&e){std::cerr<<l.key<<": "<<e.what()<<std::endl;++failures;}}code=failures?1:0;}
    else if(mode=="--enrich"){if(argc<4)throw std::runtime_error("Usage: --enrich PUBLIC_LEAGUE REQUEST_BUDGET");ps::Database db(settings);ps::Job job;ps::enrichPublicPlayers(db,argv[2],std::stoi(argv[3]),job);std::cout<<job.status<<"\n";}
    else if(mode=="--refresh-lineups"){if(argc<4)throw std::runtime_error("Usage: --refresh-lineups PUBLIC_LEAGUE REQUEST_BUDGET");ps::Database db(settings);ps::Job job;ps::enrichPublicPlayers(db,argv[2],std::stoi(argv[3]),job,true);std::cout<<job.status<<"\n";}
    else if(mode=="--refresh-news-selection"){ps::Database db(settings);ps::refreshNewsSelection(db);auto news=db.news();int selected=0;for(auto& n:news)selected+=n.selected;std::cout<<selected<<" automatically selected news items\n";}
    else if(mode=="--search-news"){if(argc<4)throw std::runtime_error("Usage: --search-news PUBLIC_LEAGUE TEAM");ps::Database db(settings);ps::Job job;bool found=false;for(auto&l:db.leagues())if(l.key==argv[2]&&l.provider==0)for(auto&t:db.teams(l.id))if(t.name==argv[3]){ps::searchNews(db,t,"injury suspension red card","",job);ps::refreshNewsSelection(db);found=true;}if(!found)throw std::runtime_error("Team not found");std::cout<<job.status<<"\n";}
    else if(mode=="--simulate"){if(argc<6)throw std::runtime_error("Usage: --simulate LEAGUE HOME AWAY SIMULATIONS");ps::Database db(settings);ps::Simulation c;std::vector<ps::Match> rows;bool found=false;for(auto& l:db.leagues())if(l.key==argv[2]&&l.provider==0){c.leagueName=l.name;rows=db.matches(l.id);for(auto&t:db.teams(l.id)){if(t.name==argv[3]){c.homeId=t.id;c.homeName=t.name;}if(t.name==argv[4]){c.awayId=t.id;c.awayName=t.name;}}found=true;break;}if(!found||!c.homeId||!c.awayId)throw std::runtime_error("League or teams not found in the database");c.requested=std::stoull(argv[5]);c.cutoff=ps::today();ps::Job job;auto result=ps::simulate(c,rows,db.news(),job);for(int k=0;k<6;++k)if(result.distributions[k].available){auto& d=result.distributions[k];ps::exportCsv(result,k,"exports/cli_"+std::to_string(k)+".csv");std::cout<<d.name<<": "<<d.lambdaHome<<" / "<<d.lambdaAway<<", "<<d.n<<" simulations\n";}}
    else if(mode=="--db-check"){ps::Database db(settings);auto r=db.query("SELECT COUNT(*),SUM(gh IS NOT NULL),SUM(ch IS NOT NULL),SUM(yh IS NOT NULL),SUM(rh IS NOT NULL),MIN(date),MAX(date) FROM fixture");for(auto&row:r){for(auto&v:row)std::cout<<v.value_or("NULL")<<" | ";std::cout<<"\n";}auto extra=db.query("SELECT (SELECT COUNT(*) FROM player),(SELECT COUNT(*) FROM appearance),(SELECT COUNT(*) FROM match_event),(SELECT COUNT(*) FROM appearance_stat),(SELECT COUNT(*) FROM news),(SELECT ROUND(SUM(data_length+index_length)/1024/1024,2) FROM information_schema.tables WHERE table_schema=DATABASE())");std::cout<<"Players | Appearances | Events | Numeric player stats | News | Table/index MB\n";for(auto&row:extra){for(auto&v:row)std::cout<<v.value_or("NULL")<<" | ";std::cout<<"\n";}}
    else if(mode=="--coverage-v2"){ps::Database db(settings);auto rows=db.query("SELECT COUNT(*),SUM(minutes IS NOT NULL),SUM(minutes_source=2),SUM(rating IS NOT NULL),SUM(studio_rating IS NOT NULL),SUM(shots_on IS NOT NULL) FROM appearance");std::cout<<"Appearances | Minutes | Derived minutes | Provider ratings | Studio ratings | Player SoT\n";for(auto& row:rows){for(auto& v:row)std::cout<<v.value_or("NULL")<<" | ";std::cout<<"\n";}rows=db.query("SELECT COUNT(*) FROM fixture f JOIN team_match_extra h ON h.fixture_id=f.id AND h.team_id=f.home_id JOIN team_match_extra a ON a.fixture_id=f.id AND a.team_id=f.away_id WHERE h.shots_on IS NOT NULL AND a.shots_on IS NOT NULL AND f.status='FT'");std::cout<<"Matches with both SoT counts: "<<rows[0][0].value_or("0")<<"\n";}
    else if(mode=="--shutdown-db"){ps::stopPrivateDatabase(settings);owned=false;}
    else code=gui(settings,mode=="--smoke");
    return code;
}catch(const std::exception&e){std::cerr<<e.what()<<"\n";try{ps::writeFile("data/last-error.txt",e.what());}catch(...){}if(argc==1)MessageBoxA(nullptr,e.what(),"Poisson Studio",MB_OK|MB_ICONERROR);return 1;}}
