#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <winsock2.h>
#include <windows.h>
#include <winhttp.h>
#include <wincrypt.h>
#include <bcrypt.h>
#include <thread>
#include "database.hpp"
namespace ps {
static std::wstring wide(const std::string& s){if(s.empty())return {};int n=MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,s.data(),int(s.size()),nullptr,0);if(!n)throw std::runtime_error("Invalid UTF-8 string");std::wstring out(n,0);MultiByteToWideChar(CP_UTF8,0,s.data(),int(s.size()),out.data(),n);return out;}
struct Internet {HINTERNET h=nullptr;~Internet(){if(h)WinHttpCloseHandle(h);}operator HINTERNET()const{return h;}};
std::string httpGet(const std::string& url,const std::string& headers){
    if(!url.starts_with("https://"))throw std::runtime_error("Data sources must use HTTPS.");
    auto u=wide(url);URL_COMPONENTS parts{};parts.dwStructSize=sizeof(parts);parts.dwHostNameLength=DWORD(-1);parts.dwUrlPathLength=DWORD(-1);parts.dwExtraInfoLength=DWORD(-1);
    if(!WinHttpCrackUrl(u.c_str(),DWORD(u.size()),0,&parts))throw std::runtime_error("Invalid URL");
    std::wstring host(parts.lpszHostName,parts.dwHostNameLength),path(parts.lpszUrlPath,parts.dwUrlPathLength);if(parts.dwExtraInfoLength)path.append(parts.lpszExtraInfo,parts.dwExtraInfoLength);
    Internet session{WinHttpOpen(L"PoissonStudio/1.0",WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY,nullptr,nullptr,0)};if(!session.h)throw std::runtime_error("Cannot initialize HTTPS");WinHttpSetTimeouts(session,10000,10000,30000,30000);
    Internet connection{WinHttpConnect(session,host.c_str(),parts.nPort,0)};Internet request{WinHttpOpenRequest(connection,L"GET",path.c_str(),nullptr,nullptr,WINHTTP_DEFAULT_ACCEPT_TYPES,WINHTTP_FLAG_SECURE)};
    if(!request.h)throw std::runtime_error("Cannot open HTTPS request");DWORD decompression=WINHTTP_DECOMPRESSION_FLAG_ALL;WinHttpSetOption(request,WINHTTP_OPTION_DECOMPRESSION,&decompression,sizeof(decompression));auto hh=wide(headers);
    if(!WinHttpSendRequest(request,hh.empty()?nullptr:hh.c_str(),DWORD(hh.size()),nullptr,0,0,0)||!WinHttpReceiveResponse(request,nullptr))throw std::runtime_error("Network request failed (Windows error "+std::to_string(GetLastError())+"). Check connection/proxy.");
    DWORD status=0,len=sizeof(status);WinHttpQueryHeaders(request,WINHTTP_QUERY_STATUS_CODE|WINHTTP_QUERY_FLAG_NUMBER,nullptr,&status,&len,nullptr);if(status!=200)throw std::runtime_error("Provider returned HTTP "+std::to_string(status)+". Check coverage, credentials or rate limits.");
    std::string out;for(;;){DWORD count=0;if(!WinHttpQueryDataAvailable(request,&count))throw std::runtime_error("Failed to receive data");if(!count)break;if(out.size()+count>64*1024*1024)throw std::runtime_error("Response exceeds 64 MB limit");size_t pos=out.size();out.resize(pos+count);DWORD read=0;if(!WinHttpReadData(request,out.data()+pos,count,&read))throw std::runtime_error("Incomplete network response");out.resize(pos+read);}return out;
}
void saveSettings(const Settings& s){
    json cfg={{"host",s.host},{"port",s.port},{"user",s.user},{"database",s.database},{"auto_start",s.autoStart},{"api_delay_ms",s.apiDelayMs}};
    std::string secret=json({{"db_password",s.password},{"api_key",s.apiKey}}).dump();DATA_BLOB in{DWORD(secret.size()),reinterpret_cast<BYTE*>(secret.data())},out{};
    if(!CryptProtectData(&in,L"Poisson Studio local credentials",nullptr,nullptr,nullptr,CRYPTPROTECT_UI_FORBIDDEN,&out))throw std::runtime_error("Windows could not encrypt settings");
    writeFile("secrets.bin",std::string(reinterpret_cast<char*>(out.pbData),out.cbData));LocalFree(out.pbData);SecureZeroMemory(secret.data(),secret.size());writeFile("config.json",cfg.dump(2));
}
Settings loadSettings(){Settings s;if(fs::exists("config.json")){auto j=json::parse(readFile("config.json"));s.host=j.value("host",s.host);s.port=j.value("port",s.port);s.user=j.value("user",s.user);s.database=j.value("database",s.database);s.autoStart=j.value("auto_start",true);s.apiDelayMs=j.value("api_delay_ms",6500);}if(fs::exists("secrets.bin")){auto b=readFile("secrets.bin");DATA_BLOB in{DWORD(b.size()),reinterpret_cast<BYTE*>(b.data())},out{};if(!CryptUnprotectData(&in,nullptr,nullptr,nullptr,nullptr,CRYPTPROTECT_UI_FORBIDDEN,&out))throw std::runtime_error("Cannot decrypt settings for this Windows account");auto j=json::parse(std::string(reinterpret_cast<char*>(out.pbData),out.cbData));s.password=j.value("db_password","");s.apiKey=j.value("api_key","");SecureZeroMemory(out.pbData,out.cbData);LocalFree(out.pbData);}return s;}
static PROCESS_INFORMATION spawn(const std::wstring& command,const fs::path& log){
    SECURITY_ATTRIBUTES sec{sizeof(sec),nullptr,TRUE};HANDLE output=CreateFileW(log.c_str(),FILE_APPEND_DATA,FILE_SHARE_READ|FILE_SHARE_WRITE,&sec,OPEN_ALWAYS,FILE_ATTRIBUTE_NORMAL,nullptr);if(output==INVALID_HANDLE_VALUE)throw std::runtime_error("Cannot create server log");
    STARTUPINFOW info{};info.cb=sizeof(info);info.dwFlags=STARTF_USESTDHANDLES|STARTF_USESHOWWINDOW;info.wShowWindow=SW_HIDE;info.hStdOutput=info.hStdError=output;info.hStdInput=GetStdHandle(STD_INPUT_HANDLE);PROCESS_INFORMATION process{};std::wstring cmd=command;
    BOOL ok=CreateProcessW(nullptr,cmd.data(),nullptr,nullptr,TRUE,CREATE_NO_WINDOW,nullptr,nullptr,&info,&process);CloseHandle(output);if(!ok)throw std::runtime_error("Unable to start private MySQL (Windows error "+std::to_string(GetLastError())+")");CloseHandle(process.hThread);return process;
}
static fs::path mysqlDirectory(){auto lock=json::parse(readFile("dependencies.lock.json"));for(auto& entry:lock.at("dependencies"))if(entry.at("name")=="mysql"){auto version=entry.at("version").get<std::string>();if(version.find_first_not_of("0123456789.")!=std::string::npos)throw std::runtime_error("Invalid MySQL version in dependency lock");return fs::absolute("tools/mysql-"+version+"-winx64");}throw std::runtime_error("MySQL dependency is missing from the lock file");}
static std::wstring serverCommand(){auto exe=mysqlDirectory()/"bin/mysqld.exe";auto ini=fs::absolute("data/my.ini");return L"\""+exe.wstring()+L"\" --defaults-file=\""+ini.wstring()+L"\" --console";}
static bool connected(const Settings& s){try{Database db(s,false);return true;}catch(...){return false;}}
static void requireFreePort(int port){
    if(port<1024||port>65535)throw std::runtime_error("Choose a private database port between 1024 and 65535.");
    WSADATA wsa{};if(WSAStartup(MAKEWORD(2,2),&wsa))throw std::runtime_error("Cannot initialize Windows networking");
    SOCKET socketHandle=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);BOOL exclusive=TRUE;
    setsockopt(socketHandle,SOL_SOCKET,SO_EXCLUSIVEADDRUSE,reinterpret_cast<const char*>(&exclusive),sizeof(exclusive));
    sockaddr_in address{};address.sin_family=AF_INET;address.sin_port=htons(static_cast<u_short>(port));address.sin_addr.s_addr=htonl(INADDR_LOOPBACK);
    bool free=socketHandle!=INVALID_SOCKET&&bind(socketHandle,reinterpret_cast<sockaddr*>(&address),sizeof(address))==0;
    if(socketHandle!=INVALID_SOCKET)closesocket(socketHandle);WSACleanup();
    if(!free)throw std::runtime_error("Localhost port "+std::to_string(port)+" is in use. Select another port in Settings or setup.ps1 -Port.");
}
static void privateConfig(const Settings& s){
    auto base=mysqlDirectory(),data=fs::absolute("data/mysql");
    if(!fs::exists(base/"bin/mysqld.exe"))throw std::runtime_error("MySQL binaries are missing. Run Setup.cmd.");
    std::string ini="[mysqld]\nbasedir=\""+base.generic_string()+"\"\ndatadir=\""+data.generic_string()+"\"\nport="+std::to_string(s.port)+"\nbind-address=127.0.0.1\nmysqlx=OFF\ninnodb_buffer_pool_size=128M\ninnodb_redo_log_capacity=64M\nmax_connections=16\ntable_open_cache=256\nperformance_schema=OFF\nlocal_infile=OFF\n";
    writeFile("data/my.ini",ini);
}
bool startPrivateDatabase(const Settings& s){
    if(connected(s))return false;
    if(!s.autoStart||s.host!="127.0.0.1")throw std::runtime_error("Cannot connect to MySQL. Check Settings or run Setup.cmd.");
    if(!fs::exists("data/mysql/mysql"))throw std::runtime_error("Private MySQL is not initialized. Run Setup.cmd first.");
    requireFreePort(s.port);privateConfig(s);
    auto p=spawn(serverCommand(),fs::absolute("data/server-console.log"));
    for(int i=0;i<90;++i){if(connected(s)){CloseHandle(p.hProcess);return true;}if(WaitForSingleObject(p.hProcess,0)==WAIT_OBJECT_0){CloseHandle(p.hProcess);throw std::runtime_error("MySQL stopped during startup; inspect data/server-console.log");}std::this_thread::sleep_for(std::chrono::milliseconds(300));}
    CloseHandle(p.hProcess);throw std::runtime_error("Timed out starting private MySQL; inspect data/server-console.log");
}
void stopPrivateDatabase(const Settings& s){Database db(s,false);db.exec("SHUTDOWN");if(s.host=="127.0.0.1")for(int i=0;i<100;++i){try{requireFreePort(s.port);return;}catch(...){std::this_thread::sleep_for(std::chrono::milliseconds(100));}}}
static std::string randomPassword(){unsigned char b[24];if(BCryptGenRandom(nullptr,b,sizeof(b),BCRYPT_USE_SYSTEM_PREFERRED_RNG)!=0)throw std::runtime_error("Secure random generation failed");const char* h="0123456789abcdef";std::string s="Ps9!";for(auto c:b){s+=h[c>>4];s+=h[c&15];}return s;}
void initializePrivateDatabase(int port){
    if(fs::exists("data/mysql/mysql"))throw std::runtime_error("Database already initialized; existing data were left intact.");
    if(fs::exists("data/mysql")&&!fs::is_empty("data/mysql"))throw std::runtime_error("A partial database directory exists. Inspect its logs before retrying; setup will not erase it.");
    requireFreePort(port);auto base=mysqlDirectory(),data=fs::absolute("data/mysql");
    if(!fs::exists(base/"bin/mysqld.exe"))throw std::runtime_error("MySQL binaries missing. Run Setup.cmd.");fs::create_directories(data);
    Settings s;s.port=port;s.password=randomPassword();saveSettings(s);privateConfig(s);
    auto init=spawn(L"\""+(base/"bin/mysqld.exe").wstring()+L"\" --no-defaults --initialize-insecure --basedir=\""+base.wstring()+L"\" --datadir=\""+data.wstring()+L"\" --console",fs::absolute("data/initialize.log"));
    DWORD wait=WaitForSingleObject(init.hProcess,120000),code=1;GetExitCodeProcess(init.hProcess,&code);CloseHandle(init.hProcess);
    if(wait!=WAIT_OBJECT_0||code!=0)throw std::runtime_error("MySQL initialization failed; inspect data/initialize.log");
    auto file=fs::absolute("data/bootstrap-private.sql");
    std::string sql="ALTER USER 'root'@'localhost' IDENTIFIED BY '"+randomPassword()+"';\nCREATE DATABASE IF NOT EXISTS poisson_studio CHARACTER SET utf8mb4;\nCREATE USER 'poisson_app'@'localhost' IDENTIFIED BY '"+s.password+"';\nGRANT ALL PRIVILEGES ON poisson_studio.* TO 'poisson_app'@'localhost';\nGRANT SHUTDOWN ON *.* TO 'poisson_app'@'localhost';\n";
    writeFile(file,sql);SecureZeroMemory(sql.data(),sql.size());
    struct RemoveBootstrap {fs::path file;~RemoveBootstrap(){std::error_code ec;fs::remove(file,ec);}} cleanup{file};
    auto process=spawn(serverCommand()+L" --init-file=\""+file.wstring()+L"\"",fs::absolute("data/server-console.log"));bool ok=false;
    for(int i=0;i<100;++i){if(connected(s)){ok=true;break;}if(WaitForSingleObject(process.hProcess,0)==WAIT_OBJECT_0)break;std::this_thread::sleep_for(std::chrono::milliseconds(300));}
    if(!ok){CloseHandle(process.hProcess);throw std::runtime_error("Initial database startup failed; inspect data/server-console.log");}
    {Database db(s);db.exec("SHUTDOWN");}WaitForSingleObject(process.hProcess,15000);CloseHandle(process.hProcess);
}
}
