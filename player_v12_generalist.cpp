#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include "game_api.h"

// ============================================================
// GoldRush 2.0 - V12 Generalist
// Map-agnostic + low-overhead challenger.
// No map_id branches, no fixed-center bonus, no fixed lane split.
// ============================================================

namespace {
constexpr int H=17,W=17,N=289,S=6;
constexpr int FOG=-5,BOMB=-3,WALL=-1,STAY=4;
constexpr int DR[4]={-1,1,0,0};
constexpr int DC[4]={0,0,-1,1};

struct P{int r,c;};
inline bool inside(int r,int c){return (unsigned)r<H&&(unsigned)c<W;}
inline int ID(int r,int c){return r*W+c;}
inline P posOf(int x){return {x/W,x%W};}
inline int md(P a,P b){return std::abs(a.r-b.r)+std::abs(a.c-b.c);}
inline bool walkable(int v){return v!=FOG&&v!=BOMB&&v!=WALL;}

struct BFS{
    std::array<int8_t,N> d{};
    std::array<int16_t,N> par{};
    std::array<int8_t,N> pa{};
    std::array<int16_t,N> q{};
};

inline void runBFS(const int g[H][W],P st,const std::array<uint8_t,N>& blocked,int maxd,BFS& b){
    b.d.fill(-1);b.par.fill(-1);b.pa.fill(-1);
    int h=0,t=0,s=ID(st.r,st.c);b.q[t++]=s;b.d[s]=0;b.par[s]=s;
    while(h<t){
        int x=b.q[h++],r=x/W,c=x%W,dd=b.d[x];
        if(dd>=maxd)continue;
        for(int a=0;a<4;a++){
            int nr=r+DR[a],nc=c+DC[a];if(!inside(nr,nc))continue;
            int y=ID(nr,nc);if(b.d[y]>=0||blocked[y]||!walkable(g[nr][nc]))continue;
            b.d[y]=dd+1;b.par[y]=x;b.pa[y]=a;b.q[t++]=y;
        }
    }
}

inline void runETA(const int g[H][W],const std::array<P,9>& src,int begin,int end,std::array<int8_t,N>& d){
    d.fill(-1);std::array<int16_t,N> q{};int h=0,t=0;
    for(int i=begin;i<end;i++){
        P s=src[i];if(!inside(s.r,s.c))continue;int x=ID(s.r,s.c);
        if(d[x]==0)continue;d[x]=0;q[t++]=x;
    }
    while(h<t){
        int x=q[h++],r=x/W,c=x%W,dd=d[x];if(dd>=12)continue;
        for(int a=0;a<4;a++){
            int nr=r+DR[a],nc=c+DC[a];if(!inside(nr,nc)||!walkable(g[nr][nc]))continue;
            int y=ID(nr,nc);if(d[y]>=0)continue;d[y]=dd+1;q[t++]=y;
        }
    }
}

inline int appendPath(const BFS& b,int target,std::array<int,S>& act,int& len,int room){
    if(target<0||b.d[target]<=0)return 0;
    std::array<int,S> rev{};int n=0,x=target;
    while(b.par[x]!=x&&n<room){int a=b.pa[x];if(a<0)return 0;rev[n++]=a;x=b.par[x];if(x<0)return 0;}
    for(int i=n-1;i>=0;i--)act[len++]=rev[i];return n;
}

struct Choice{int id=-1,val=0,dist=99,rival=99;double ev=0,score=-1e100;};

inline Choice chooseGold(const int g[H][W],const BFS& b,int room,const std::array<int8_t,N>& enemyETA,const std::array<int8_t,N>& npcETA,const std::array<uint8_t,N>& claimed){
    Choice best;
    for(int x=0;x<N;x++){
        int r=x/W,c=x%W,v=g[r][c],d=b.d[x];
        if(v<=0||claimed[x]||d<=0||d>room)continue;
        int e=enemyETA[x]>=0?enemyETA[x]:99;
        int n=npcETA[x]>=0?npcETA[x]:99;
        double f=1.0;
        if(e<=d-2)f*=0.12;else if(e==d-1)f*=0.22;else if(e==d)f*=0.42;else if(e==d+1)f*=0.68;else if(e==d+2)f*=0.88;
        if(n<=d-2)f*=0.48;else if(n==d-1)f*=0.58;else if(n==d)f*=0.70;else if(n==d+1)f*=0.86;
        double ev=v*f;
        double score=ev/(1.0+0.55*d)+0.035*v;
        if(score>best.score)best={x,v,d,std::min(e,n),ev,score};
    }
    return best;
}

inline int fogAdj(const int g[H][W],int x){
    int r=x/W,c=x%W,n=0;for(int a=0;a<4;a++){int nr=r+DR[a],nc=c+DC[a];if(inside(nr,nc)&&g[nr][nc]==FOG)n++;}return n;
}

inline int reachableCount(const BFS& b){int n=0;for(int i=0;i<N;i++)if(b.d[i]>=0)n++;return n;}

inline int bestFogDir(const int g[H][W],P f,const std::array<int8_t,N>& enemyETA){
    int best=-1;double bs=-1e100;
    for(int a=0;a<4;a++){
        int nr=f.r+DR[a],nc=f.c+DC[a];if(!inside(nr,nc)||g[nr][nc]!=FOG)continue;
        int cont=0;for(int b=0;b<4;b++){int rr=nr+DR[b],cc=nc+DC[b];if(inside(rr,cc)&&g[rr][cc]==FOG)cont++;}
        int e=enemyETA[ID(f.r,f.c)]>=0?enemyETA[ID(f.r,f.c)]:99;
        double s=2.0*cont+0.10*std::min(e,8);if(s>bs){bs=s;best=a;}
    }
    return best;
}

struct Plan{
    std::array<int,S> a{};int len=0;double ev=0,q=0,urg=0,explore=0;int first=-1;P end{-1,-1};
    Plan(){a.fill(STAY);}
};

inline Plan makePlan(const GameInput& game,P st,int budget,const std::array<uint8_t,N>& blocked,const std::array<int8_t,N>& enemyETA,const std::array<int8_t,N>& npcETA){
    Plan p;p.end=st;if(budget<=0)return p;
    std::array<uint8_t,N> claimed{};P cur=st;int left=budget;

    for(int chain=0;chain<3&&left>0;chain++){
        BFS b;runBFS(game.grid,cur,blocked,left,b);
        Choice z=chooseGold(game.grid,b,left,enemyETA,npcETA,claimed);if(z.id<0)break;
        if(chain==0){p.first=z.id;int m=z.rival-z.dist;if(m<=0)p.urg=1.8;else if(m==1)p.urg=1.0;else if(m==2)p.urg=0.4;}
        int old=p.len;appendPath(b,z.id,p.a,p.len,left);int used=p.len-old;if(used<=0)break;
        left-=used;p.ev+=z.ev;p.q+=z.score;claimed[z.id]=1;cur=posOf(z.id);
    }

    // Explore only after no reachable positive-EV target remains.
    if(left>0){
        BFS b;runBFS(game.grid,cur,blocked,left,b);int reach=reachableCount(b),best=-1;double bs=-1e100;
        for(int x=0;x<N;x++){
            int d=b.d[x];if(d<0||d>left)continue;int f=fogAdj(game.grid,x);if(!f)continue;
            double scarcity=reach<20?1.70:(reach<40?1.35:1.0);
            double s=scarcity*3.0*f-0.32*d;if(s>bs){bs=s;best=x;}
        }
        if(best>=0){
            int old=p.len;appendPath(b,best,p.a,p.len,left);left-=p.len-old;p.explore+=std::max(0.0,bs);P f=posOf(best);
            // At most one blind step; then replan next turn.
            if(left>0){int a=bestFogDir(game.grid,f,enemyETA);if(a>=0){p.a[p.len++]=a;left--;p.explore+=1.0;}}
        }
    }

    P e=st;for(int i=0;i<p.len;i++){int a=p.a[i];if(a<4){e.r+=DR[a];e.c+=DC[a];}}p.end=e;return p;
}

inline int visibleGold(const int g[H][W]){int s=0;for(int r=0;r<H;r++)for(int c=0;c<W;c++)if(g[r][c]>0)s+=g[r][c];return s;}
inline int fogCount(const int g[H][W]){int s=0;for(int r=0;r<H;r++)for(int c=0;c<W;c++)if(g[r][c]==FOG)s++;return s;}
inline int knownFree(const int g[H][W]){int s=0;for(int r=0;r<H;r++)for(int c=0;c<W;c++)if(walkable(g[r][c]))s++;return s;}
}

extern "C" GameOutput moveDecision(const GameInput& game){
    GameOutput out{};
    P st[2]={{game.players[0].units[0].position.x,game.players[0].units[0].position.y},{game.players[0].units[1].position.x,game.players[0].units[1].position.y}};

    std::array<P,9> rivals{};int rc=0,enemies=0;
    for(int u=0;u<2;u++){
        auto q=game.players[1].units[u].position;
        if(q.x>=0&&q.y>=0&&rc<9){rivals[rc++]={q.x,q.y};enemies++;}
    }
    for(int i=0;i<game.npc_count&&rc<9;i++)rivals[rc++]={game.npcs[i].position.x,game.npcs[i].position.y};

    std::array<int8_t,N> enemyETA{},npcETA{};
    if(enemies>0)runETA(game.grid,rivals,0,enemies,enemyETA);else enemyETA.fill(-1);
    if(rc>enemies)runETA(game.grid,rivals,enemies,rc,npcETA);else npcETA.fill(-1);

    std::array<uint8_t,N> base{};
    for(int i=0;i<rc;i++)if(inside(rivals[i].r,rivals[i].c))base[ID(rivals[i].r,rivals[i].c)]=1;

    Plan tab[2][S+1];
    for(int u=0;u<2;u++)for(int b=0;b<=S;b++){
        auto blocked=base;P mate=st[1-u];if(inside(mate.r,mate.c))blocked[ID(mate.r,mate.c)]=1;
        tab[u][b]=makePlan(game,st[u],b,blocked,enemyETA,npcETA);
    }

    int bk=3;double best=-1e100;
    for(int k=0;k<=S;k++){
        const Plan&p0=tab[0][k];const Plan&p1=tab[1][S-k];int used=p0.len+p1.len;
        double sc=2.8*(p0.ev+p1.ev)+4.0*(p0.q+p1.q)+0.16*(p0.explore+p1.explore)+0.16*used;
        if(p0.first>=0&&p0.first==p1.first)sc-=4.0;
        int sep=md(p0.end,p1.end);if(sep==0)sc-=2.0;else if(sep==1)sc-=0.8;
        if(p0.ev>0&&p1.ev>0)sc+=0.05*std::min(p0.ev,p1.ev);
        if(sc>best){best=sc;bk=k;}
    }

    const Plan&p0=tab[0][bk];const Plan&p1=tab[1][S-bk];
    out.k=bk;out.order=(p1.urg>p0.urg+1e-9)?1:0;
    for(int u=0;u<2;u++)for(int i=0;i<S;i++)out.actions[u][i]=STAY;
    for(int i=0;i<p0.len&&i<S;i++)out.actions[0][i]=p0.a[i];
    for(int i=0;i<p1.len&&i<S;i++)out.actions[1][i]=p1.a[i];

    // State-based vision, not map-based vision.
    int vg=visibleGold(game.grid),fog=fogCount(game.grid),kf=knownFree(game.grid);
    out.vision=(vg==0&&fog>180&&kf<55&&game.players[0].gold>=120)?1:0;
    return out;
}
