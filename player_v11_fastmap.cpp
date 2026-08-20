#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include "game_api.h"

// V11 FastMap challenger
// Goals: (1) stop treating fog as a hard wall forever, (2) remove per-target
// BFS and unordered_set hot-path overhead, (3) allocate the 6 actions to the
// unit with the best marginal return on the current revealed topology.
namespace {
constexpr int H=17,W=17,N=H*W,S=6;
constexpr int FOG=-5,BOMB=-3,WALL=-1,STAY=4;
constexpr int DR[4]={-1,1,0,0};
constexpr int DC[4]={0,0,-1,1};
struct P{int r,c;};
inline int ID(int r,int c){return r*W+c;}
inline bool inside(int r,int c){return (unsigned)r<H && (unsigned)c<W;}
inline int md(P a,P b){return std::abs(a.r-b.r)+std::abs(a.c-b.c);}
inline bool knownFree(int v){return v!=FOG && v!=BOMB && v!=WALL;}
inline int stepDir(P a,P b){if(b.r==a.r-1&&b.c==a.c)return 0;if(b.r==a.r+1&&b.c==a.c)return 1;if(b.r==a.r&&b.c==a.c-1)return 2;if(b.r==a.r&&b.c==a.c+1)return 3;return STAY;}

struct Plan{
 std::array<int,S> a{}; int len=0; double ev=0,quality=0,urgency=0; P end{-1,-1};
 Plan(){a.fill(STAY);}
};

struct BFS{
 std::array<int16_t,N> par{}; std::array<int8_t,N> pa{}; std::array<int8_t,N> d{}; std::array<int16_t,N> q{};
};

inline void runBFS(const int g[H][W],P st,const std::array<uint8_t,N>& blocked,int maxd,BFS& b){
 b.par.fill(-1); b.pa.fill(-1); b.d.fill(-1); int h=0,t=0; int s=ID(st.r,st.c); b.q[t++]=s;b.d[s]=0;b.par[s]=s;
 while(h<t){int x=b.q[h++],r=x/W,c=x%W,dd=b.d[x]; if(dd>=maxd)continue;
  for(int k=0;k<4;k++){int nr=r+DR[k],nc=c+DC[k]; if(!inside(nr,nc))continue;int y=ID(nr,nc);if(b.d[y]>=0||blocked[y]||!knownFree(g[nr][nc]))continue;b.d[y]=dd+1;b.par[y]=x;b.pa[y]=k;b.q[t++]=y;}
 }
}

inline int appendPath(const BFS& b,int target,Plan& p,int room){
 int s=target,n=0; std::array<int,S> rev{}; while(b.par[s]!=s && n<room){rev[n++]=b.pa[s];s=b.par[s];if(s<0)return 0;} for(int i=n-1;i>=0;i--)p.a[p.len++]=rev[i]; return n;
}

struct Choice{int id=-1,val=0,d=99,rival=99;double ev=0,score=-1e100;};
inline Choice chooseGold(const int g[H][W],const BFS& b,const std::array<P,9>& rivals,int rc,int enemies,const std::array<uint8_t,N>& claimed){
 Choice z;
 for(int x=0;x<N;x++){int r=x/W,c=x%W,v=g[r][c],d=b.d[x]; if(v<=0||d<=0||d>S||claimed[x])continue;
  int ed=99,nd=99;P q{r,c};for(int i=0;i<rc;i++){int z0=md(rivals[i],q);if(i<enemies)ed=std::min(ed,z0);else nd=std::min(nd,z0);} double f=1.0;
  if(ed<d)f*=0.16;else if(ed==d)f*=0.30;else if(ed==d+1)f*=0.60;else if(ed==d+2)f*=0.84;
  if(nd<d)f*=0.48;else if(nd==d)f*=0.66;else if(nd==d+1)f*=0.84;
  double ev=.65*v*f; double sc=ev/(1.0+.52*d)+.035*v; if(sc>z.score){z={x,v,d,std::min(ed,nd),ev,sc};}
 } return z;
}

// Find a known frontier, then deliberately use at most ONE final action to
// enter adjacent fog. This is the critical new-map change: V9 reached the
// edge of a revealed island and then often burned the remaining budget on STAY.
inline void appendExplore(const int g[H][W],P st,int unit,const std::array<uint8_t,N>& blocked,int room,Plan& p){
 if(room<=0)return; BFS b;runBFS(g,st,blocked,room,b);int best=-1;double bs=-1e100;
 for(int x=0;x<N;x++){int d=b.d[x];if(d<0||d>room)continue;int r=x/W,c=x%W,fog=0;for(int k=0;k<4;k++){int nr=r+DR[k],nc=c+DC[k];if(inside(nr,nc)&&g[nr][nc]==FOG)fog++;}if(!fog)continue;
  int center=std::abs(r-8)+std::abs(c-8);double lane=(unit==0?0.025*(c-r):0.025*(r-c));double sc=3.8*fog-.24*d-.035*center+lane;if(sc>bs){bs=sc;best=x;}}
 if(best<0)return; int before=p.len;appendPath(b,best,p,room);room-=p.len-before;if(room<=0)return;P f{best/W,best%W};
 // Prefer fog steps that continue toward the board center, but do not chain blind moves.
 int ba=-1,bscore=-999;for(int k=0;k<4;k++){int nr=f.r+DR[k],nc=f.c+DC[k];if(!inside(nr,nc)||g[nr][nc]!=FOG)continue;int sc=20-(std::abs(nr-8)+std::abs(nc-8));if(unit==0)sc+=nc-nr;else sc+=nr-nc;if(sc>bscore){bscore=sc;ba=k;}}
 if(ba>=0&&p.len<S)p.a[p.len++]=ba;
}

inline Plan makePlan(const GameInput& game,P st,int budget,int unit,const std::array<uint8_t,N>& base,const std::array<P,9>& rivals,int rc,int enemies){
 Plan p;p.end=st;if(budget<=0)return p;std::array<uint8_t,N> claimed{};P cur=st;int left=budget;
 for(int chain=0;chain<3&&left>0;chain++){BFS b;runBFS(game.grid,cur,base,left,b);Choice z=chooseGold(game.grid,b,rivals,rc,enemies,claimed);if(z.id<0)break;
  if(chain==0){int slack=z.rival-z.d;if(slack<=0)p.urgency=1.8;else if(slack==1)p.urgency=1.0;else if(slack==2)p.urgency=.35;}
  int old=p.len;appendPath(b,z.id,p,left);int used=p.len-old;if(!used)break;left-=used;p.ev+=z.ev;p.quality+=z.score;claimed[z.id]=1;cur={z.id/W,z.id%W};}
 if(left>0){int old=p.len;appendExplore(game.grid,cur,unit,base,left,p);left-=p.len-old;}
 // End position based only on known movement prefix; a blind final step is still geometrically valid.
 cur=st;for(int i=0;i<p.len;i++)if(p.a[i]<4){cur.r+=DR[p.a[i]];cur.c+=DC[p.a[i]];}p.end=cur;return p;
}
}

extern "C" GameOutput moveDecision(const GameInput& game){
 GameOutput out{};P st[2]={{game.players[0].units[0].position.x,game.players[0].units[0].position.y},{game.players[0].units[1].position.x,game.players[0].units[1].position.y}};
 std::array<P,9> rivals{};int rc=0,enemies=0;
 for(int u=0;u<2;u++){auto q=game.players[1].units[u].position;if(q.x>=0&&rc<9){rivals[rc++]={q.x,q.y};enemies++;}}
 for(int i=0;i<game.npc_count&&rc<9;i++)rivals[rc++]={game.npcs[i].position.x,game.npcs[i].position.y};
 std::array<uint8_t,N> base{};for(int i=0;i<rc;i++)if(inside(rivals[i].r,rivals[i].c))base[ID(rivals[i].r,rivals[i].c)]=1;
 // Precompute each unit's plan for every possible budget. This replaces V9's
 // repeated per-target BFS inside every k/order candidate.
 Plan tab[2][S+1];for(int u=0;u<2;u++)for(int b=0;b<=S;b++){auto blk=base;blk[ID(st[1-u].r,st[1-u].c)]=1;tab[u][b]=makePlan(game,st[u],b,u,blk,rivals,rc,enemies);}
 int bk=3;double best=-1e100;for(int k=0;k<=S;k++){const Plan&p0=tab[0][k];const Plan&p1=tab[1][S-k];int used=p0.len+p1.len;double sc=2.7*(p0.ev+p1.ev)+4.2*(p0.quality+p1.quality)+.24*used;
  // discourage both units converging to the same final cell / same tiny pocket
  int sep=md(p0.end,p1.end);if(sep<=1)sc-=1.0;if(sc>best){best=sc;bk=k;}}
 int order=(tab[1][S-bk].urgency>tab[0][bk].urgency)?1:0;out.k=bk;out.order=order;
 for(int u=0;u<2;u++)for(int i=0;i<S;i++)out.actions[u][i]=STAY;const Plan&p0=tab[0][bk];const Plan&p1=tab[1][S-bk];for(int i=0;i<p0.len&&i<S;i++)out.actions[0][i]=p0.a[i];for(int i=0;i<p1.len&&i<S;i++)out.actions[1][i]=p1.a[i];
 int visible=0,fog=0;for(int r=0;r<H;r++)for(int c=0;c<W;c++){if(game.grid[r][c]>0)visible+=game.grid[r][c];else if(game.grid[r][c]==FOG)fog++;}
 // New map is information-starved, but vision still costs gold. Buy only at
 // severe information shortage; blind frontier entry handles ordinary fog.
 out.vision=(visible==0&&fog>210&&game.players[0].gold>=120)?1:0;return out;
}
