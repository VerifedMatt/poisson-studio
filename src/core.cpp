#include "core.hpp"
#include <fstream>
#include <sstream>
#include <iomanip>
#include <ctime>
#include <set>
namespace ps {
int dayNumber(const std::string& s){
    if(s.size()<10||s[4]!='-'||s[7]!='-')throw std::runtime_error("Invalid ISO date: "+s);
    int y=std::stoi(s.substr(0,4)),m=std::stoi(s.substr(5,2)),d=std::stoi(s.substr(8,2));
    std::chrono::year_month_day date{std::chrono::year{y},std::chrono::month{unsigned(m)},std::chrono::day{unsigned(d)}};
    if(!date.ok())throw std::runtime_error("Invalid calendar date: "+s);
    return int(std::chrono::sys_days(date).time_since_epoch().count());
}
static std::string dateString(std::chrono::year_month_day d){char b[16];snprintf(b,sizeof(b),"%04d-%02u-%02u",int(d.year()),unsigned(d.month()),unsigned(d.day()));return b;}
std::string today(){return dateString(std::chrono::year_month_day(std::chrono::floor<std::chrono::days>(std::chrono::system_clock::now())));}
std::string yearsAgo(int n){auto d=std::chrono::year_month_day(std::chrono::sys_days(std::chrono::days(dayNumber(today()))));d-=std::chrono::years(n);if(!d.ok())d=std::chrono::year_month_day(d.year(),d.month(),std::chrono::day(28));return dateString(d);}
std::string addDays(const std::string& d,int n){return dateString(std::chrono::year_month_day(std::chrono::sys_days(std::chrono::days(dayNumber(d)+n))));}
std::string csvDate(const std::string& s){if(s.size()>=10&&s[4]=='-'){dayNumber(s.substr(0,10));return s.substr(0,10);}std::istringstream in(s);std::string d,m,y;std::getline(in,d,'/');std::getline(in,m,'/');std::getline(in,y);if(y.size()==2)y="20"+y;char b[16];snprintf(b,sizeof(b),"%04d-%02d-%02d",std::stoi(y),std::stoi(m),std::stoi(d));dayNumber(b);return b;}
std::string readFile(const fs::path& p){std::ifstream in(p,std::ios::binary);if(!in)throw std::runtime_error("Cannot read "+p.string());return std::string(std::istreambuf_iterator<char>(in),{});}
void writeFile(const fs::path& p,const std::string& s){if(p.has_parent_path())fs::create_directories(p.parent_path());std::ofstream f(p,std::ios::binary);if(!f||!f.write(s.data(),s.size()))throw std::runtime_error("Cannot write "+p.string());}
std::vector<std::vector<std::string>> parseCsv(const std::string& s){
    std::vector<std::vector<std::string>> out;std::vector<std::string> row;std::string field;bool quoted=false;
    size_t begin=s.starts_with("\xEF\xBB\xBF")?3:0;
    for(size_t i=begin;i<s.size();++i){char c=s[i];if(c=='"'){if(quoted&&i+1<s.size()&&s[i+1]=='"'){field+='"';++i;}else quoted=!quoted;}
        else if(c==','&&!quoted){row.push_back(field);field.clear();}
        else if((c=='\n'||c=='\r')&&!quoted){if(c=='\r'&&i+1<s.size()&&s[i+1]=='\n')++i;row.push_back(field);field.clear();if(row.size()>1||!row[0].empty())out.push_back(row);row.clear();}
        else field+=c;}
    if(quoted)throw std::runtime_error("Unterminated CSV quoted field");
    if(!field.empty()||!row.empty()){row.push_back(field);out.push_back(row);}return out;
}
std::string urlEncode(const std::string& s){const char* h="0123456789ABCDEF";std::string out;for(unsigned char c:s){if((c>='a'&&c<='z')||(c>='A'&&c<='Z')||(c>='0'&&c<='9')||c=='-'||c=='_'||c=='.'||c=='~')out+=c;else{out+='%';out+=h[c>>4];out+=h[c&15];}}return out;}

Fit fitPoisson(const std::vector<Match>& all,int metric,int home,int away,const std::string& cutoff,int halfLife,double ridge,Job* job){
    Fit out;int now=dayNumber(cutoff),start=dayNumber(yearsAgo(5));std::map<int,int> ids;
    struct Row{int h,a,yh,ya;double w;};std::vector<Row> rows;
    for(const auto& m:all){if(m.date.empty()||m.h[metric]<0||m.a[metric]<0)continue;int day=dayNumber(m.date);if(day>=now||day<start)continue;
        auto index=[&](int id){auto [it,added]=ids.emplace(id,int(ids.size()));return it->second;};
        rows.push_back({index(m.home),index(m.away),m.h[metric],m.a[metric],std::exp(-std::log(2.)*(now-day)/halfLife)});
        if(m.home==home||m.away==home)++out.homeSamples;if(m.home==away||m.away==away)++out.awaySamples;
    }
    out.matches=int(rows.size());
    if(rows.size()<30||out.homeSamples<5||out.awaySamples<5){out.note="Insufficient recorded observations (need 30 league matches and 5 per selected team). Missing data are excluded.";return out;}
    int nt=int(ids.size()),dim=2+2*nt;std::vector<double> t(dim),grad(dim),diag(dim),candidate(dim);double sw=0,sh=0,sa=0;
    for(auto& r:rows){sw+=r.w;sh+=r.w*r.yh;sa+=r.w*r.ya;}
    // A tiny prior stabilizes genuinely rare counts (especially red cards).
    t[0]=std::log((sa+.05)/(sw+.05));t[1]=std::log((sh+.05)/(sa+.05));
    auto loss=[&](const std::vector<double>& v,bool derivatives){double value=0;if(derivatives){std::fill(grad.begin(),grad.end(),0);std::fill(diag.begin(),diag.end(),1e-8);}
        for(auto& r:rows){int ia=2+r.h,ib=2+r.a,da=2+nt+r.h,db=2+nt+r.a;double zh=v[0]+v[1]+v[ia]+v[db],za=v[0]+v[ib]+v[da];double mh=std::exp(zh),ma=std::exp(za);value+=r.w*(mh-r.yh*zh+ma-r.ya*za);
            if(derivatives){double eh=r.w*(mh-r.yh),ea=r.w*(ma-r.ya),hh=r.w*mh,ha=r.w*ma;for(int j:{0,1,ia,db}){grad[j]+=eh;diag[j]+=hh;}for(int j:{0,ib,da}){grad[j]+=ea;diag[j]+=ha;}}}
        for(int j=2;j<dim;++j){value+=ridge*v[j]*v[j]/2;if(derivatives){grad[j]+=ridge*v[j];diag[j]+=ridge;}}
        value+=.01*(v[0]*v[0]+v[1]*v[1]);if(derivatives)for(int j=0;j<2;++j){grad[j]+=.02*v[j];diag[j]+=.02;}return value;};
    double previous=loss(t,false);bool converged=false;
    for(int iter=0;iter<600;++iter){if(job&&iter%20==0)job->check();loss(t,true);double step=.5,next=previous;bool accepted=false;
        for(int trial=0;trial<18;++trial){for(int j=0;j<dim;++j)candidate[j]=t[j]-step*std::clamp(grad[j]/diag[j],-2.,2.);next=loss(candidate,false);if(std::isfinite(next)&&next<=previous){accepted=true;break;}step*=.5;}
        if(!accepted)break;double gain=previous-next;t.swap(candidate);previous=next;if(gain<1e-8*(1+std::abs(previous))){converged=true;break;}}
    int hi=ids.at(home),ai=ids.at(away);out.home=std::exp(t[0]+t[1]+t[2+hi]+t[2+nt+ai]);out.away=std::exp(t[0]+t[2+ai]+t[2+nt+hi]);
    double pearson=0;for(auto&r:rows){double h=std::exp(t[0]+t[1]+t[2+r.h]+t[2+nt+r.a]),a=std::exp(t[0]+t[2+r.a]+t[2+nt+r.h]);pearson+=(r.yh-h)*(r.yh-h)/std::max(h,.001)+(r.ya-a)*(r.ya-a)/std::max(a,.001);}
    out.dispersion=pearson/std::max(1,int(rows.size()*2)-dim);out.available=std::isfinite(out.home)&&std::isfinite(out.away)&&out.home<100&&out.away<100;
    out.note=converged?"Opponent-adjusted Poisson; recent matches receive more weight.":"Fit reached the convergence limit; treat this forecast cautiously.";
    if(out.dispersion>1.5)out.note+=" Historical variability exceeds Poisson: tail probabilities are approximate.";return out;
}
static void add(Distribution& d,int h,int a){++d.n;int hi=std::min(h,d.bins-1),ai=std::min(a,d.bins-1);++d.joint[hi*d.bins+ai];++d.home[hi];++d.away[ai];if(size_t(h+a)>=d.total.size())d.total.resize(h+a+1);++d.total[h+a];++d.exact[{h,a}];if(h>a)++d.homeWins;else if(h<a)++d.awayWins;else ++d.draws;}
Simulation simulate(Simulation c,const std::vector<Match>& rows,const std::vector<News>& news,Job& job){
    if(c.requested<100||c.requested>100000000)throw std::runtime_error("Choose 100 to 100,000,000 simulations.");
    if(c.homeId==c.awayId)throw std::runtime_error("Choose two different teams.");if(c.cutoff.empty()||c.cutoff>today())c.cutoff=today();
    const char* names[]={"Exact score","Corners","Yellow cards","Red cards","All cards (yellow + red)","Shots on target"};
    for(int k=0;k<5;++k){int metric=k==4?5:k;job.message(std::string("Fitting ")+names[metric]);auto f=fitPoisson(rows,k,c.homeId,c.awayId,c.cutoff,c.halfLife,c.ridge,&job);auto& d=c.distributions[metric];d.name=names[metric];d.note=f.note;d.available=f.available;d.lambdaHome=f.home;d.lambdaAway=f.away;d.dispersion=f.dispersion;}
    if(!c.distributions[0].available)throw std::runtime_error(c.distributions[0].note);
    c.appliedNews=0;c.pendingNews=0;
    if(c.useNews){double gfH=0,gfA=0,cfH=0,cfA=0,cardH=0,cardA=0,sfH=0,sfA=0;std::set<std::string> stories;
        for(auto& n:news){if(!n.selected||n.decision<0||(!n.expires.empty()&&n.expires<today()))continue;if(n.team!=c.homeId&&n.team!=c.awayId)continue;
            auto headline=n.title.substr(0,n.title.rfind(" - "));for(auto& c:headline)c=char(std::tolower(static_cast<unsigned char>(c)));
            if(!headline.empty()&&!stories.insert(std::to_string(n.team)+":"+headline).second)continue;
            ++c.appliedNews;if(!n.reviewed)++c.pendingNews;bool h=n.team==c.homeId;
            (h?gfH:gfA)+=n.goalsFor;(h?gfA:gfH)+=n.goalsAgainst;(h?cfH:cfA)+=n.cornersFor;(h?cfA:cfH)+=n.cornersAgainst;(h?cardH:cardA)+=n.cards;(h?sfH:sfA)+=n.shotsFor;(h?sfA:sfH)+=n.shotsAgainst;}
        std::array<double,6> dh{gfH,cfH,cardH,cardH,0,sfH},da{gfA,cfA,cardA,cardA,0,sfA};for(int k=0;k<6;++k){c.distributions[k].lambdaHome*=std::clamp(1+dh[k]/100,.25,2.);c.distributions[k].lambdaAway*=std::clamp(1+da[k]/100,.25,2.);}}
    auto& cards=c.distributions[4];cards.name=names[4];cards.available=c.distributions[2].available&&c.distributions[3].available;cards.lambdaHome=c.distributions[2].lambdaHome+c.distributions[3].lambdaHome;cards.lambdaAway=c.distributions[2].lambdaAway+c.distributions[3].lambdaAway;cards.note="Yellow + red card counts; not bookmaker booking points. Card definitions may differ by provider.";
    for(int k=0;k<6;++k){auto& d=c.distributions[k];d.bins=(k==0?c.maxGoals:k==1?c.maxCorners:k==5?c.maxShots:c.maxCards)+2;d.joint.assign(d.bins*d.bins,0);d.home.assign(d.bins,0);d.away.assign(d.bins,0);d.total.assign(32,0);}
    std::mt19937_64 rng(c.seed);std::array<std::poisson_distribution<int>,10> draw;
    for(int k=0;k<5;++k){auto& d=c.distributions[k==4?5:k];draw[2*k]=std::poisson_distribution<int>(std::max(1e-8,d.lambdaHome));draw[2*k+1]=std::poisson_distribution<int>(std::max(1e-8,d.lambdaAway));}
    job.message("Simulating goals, corners, cards and shots on target");
    for(uint64_t n=0;n<c.requested;++n){if(n%4096==0){job.check();job.progress=double(n)/c.requested;}int hg[5]{},ag[5]{};for(int k=0;k<5;++k){auto& d=c.distributions[k==4?5:k];if(d.available){hg[k]=draw[2*k](rng);ag[k]=draw[2*k+1](rng);add(d,hg[k],ag[k]);}}if(cards.available)add(cards,hg[2]+hg[3],ag[2]+ag[3]);}
    job.progress=1;job.message("Simulation complete");return c;
}
void exportCsv(const Simulation& r,int k,const fs::path& path){const auto& d=r.distributions.at(k);if(!d.available)throw std::runtime_error("No data for this market");std::ostringstream out;out<<"home_count,away_count,occurrences,probability,simulations\n"<<std::setprecision(12);for(auto& [score,n]:d.exact)out<<score.first<<","<<score.second<<","<<n<<","<<double(n)/d.n<<","<<d.n<<"\n";writeFile(path,out.str());json meta={{"home",r.homeName},{"away",r.awayName},{"league",r.leagueName},{"metric",d.name},{"simulations",d.n},{"seed",r.seed},{"cutoff",r.cutoff},{"half_life_days",r.halfLife},{"ridge",r.ridge},{"home_rate",d.lambdaHome},{"away_rate",d.lambdaAway},{"news_enabled",r.useNews},{"applied_news",r.appliedNews},{"note",d.note}};writeFile(path.string()+".json",meta.dump(2));}
bool runCoreTests(std::string& result){try{
    runFeatureTests();
    auto csv=parseCsv("\xEF\xBB\xBF" "A,B,C\r\n1,\"a,b\",\"x\"\"y\"\r\n");if(csv.size()!=2||csv[1][1]!="a,b"||csv[1][2]!="x\"y")throw std::runtime_error("CSV quoting");
    if(csvDate("06/11/26")!="2026-11-06"||addDays("2024-02-28",1)!="2024-02-29")throw std::runtime_error("Date parsing");
    std::vector<Match> data;std::mt19937 rng(71);std::poisson_distribution<int> h(1.7),a(1.2);for(int i=0;i<2400;++i){Match m;m.home=i%4+1;m.away=(i/4+i%4+1)%4+1;if(m.home==m.away)m.away=m.away%4+1;m.date=addDays(today(),-1-i%1400);m.h[0]=h(rng);m.a[0]=a(rng);data.push_back(m);}
    auto f=fitPoisson(data,0,1,2,today(),365,5);if(!f.available||std::abs(f.home-1.7)>.25||std::abs(f.away-1.2)>.25)throw std::runtime_error("Poisson fit known rates");
    auto missing=fitPoisson(data,1,1,2,today(),365,5);if(missing.available)throw std::runtime_error("Missing corners must not become zero");
    Simulation c;c.homeId=1;c.awayId=2;c.cutoff=today();c.requested=100000;c.seed=11;Job job;auto s=simulate(c,data,{},job);const auto& d=s.distributions[0];if(d.n!=100000||std::accumulate(d.joint.begin(),d.joint.end(),uint64_t(0))!=d.n||d.homeWins+d.draws+d.awayWins!=d.n)throw std::runtime_error("Simulation conservation");
    double theoretical=std::exp(-d.lambdaHome-d.lambdaAway),observed=double(d.exact.at({0,0}))/d.n;if(std::abs(theoretical-observed)>5*std::sqrt(theoretical*(1-theoretical)/d.n))throw std::runtime_error("Poisson 0-0 calibration");
    std::vector<News> items;items.emplace_back();items[0].team=1;items[0].reviewed=true;items[0].selected=true;items[0].expires=addDays(today(),1);items[0].goalsFor=-10;c.useNews=true;c.requested=1000;auto changed=simulate(c,data,items,job);if(std::abs(changed.distributions[0].lambdaHome-f.home*.9)>1e-8||changed.appliedNews!=1)throw std::runtime_error("Reviewed news effects");items[0].expires=addDays(today(),-1);auto expired=simulate(c,data,items,job);if(std::abs(expired.distributions[0].lambdaHome-f.home)>1e-8||expired.appliedNews)throw std::runtime_error("Expired news must not affect rates");items[0].expires=addDays(today(),1);items[0].reviewed=false;items[0].selected=false;auto unreviewed=simulate(c,data,items,job);if(unreviewed.appliedNews)throw std::runtime_error("Unreviewed news must not affect rates");
    std::poisson_distribution<int> shotH(4.5),shotA(3.2);for(auto& row:data){row.h[4]=shotH(rng);row.a[4]=shotA(rng);}c.useNews=false;c.requested=10000;auto shots=simulate(c,data,{},job);auto& sd=shots.distributions[5];if(!sd.available||sd.n!=10000||std::abs(sd.lambdaHome-4.5)>.5||std::abs(sd.lambdaAway-3.2)>.5||std::accumulate(sd.joint.begin(),sd.joint.end(),uint64_t(0))!=sd.n)throw std::runtime_error("Shots on target model / conservation");
    items[0].selected=true;items[0].shotsFor=-10;c.useNews=true;auto selected=simulate(c,data,items,job);if(selected.pendingNews!=1||std::abs(selected.distributions[5].lambdaHome-sd.lambdaHome*.9)>1e-8)throw std::runtime_error("Pending automatic news scenarios");items[0].decision=-1;auto rejected=simulate(c,data,items,job);if(rejected.appliedNews)throw std::runtime_error("Rejected news exclusion");
    result="PASS: CSV/dates, Poisson known rates, missing data, conservation, exact probability, shots-on-target fit, news selection/rejection/expiry, event-derived minutes, provider/local ratings, fair odds and expected returns.";return true;
}catch(const std::exception&e){result=std::string("FAIL: ")+e.what();return false;}}
}
