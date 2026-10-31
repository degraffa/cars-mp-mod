/* CarsMP: 3-4 player split screen for Cars (2006, Steam build).
 * Loaded as a binkw32.dll proxy; patches the game in memory after the Steam stub has decrypted it. */
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
typedef struct{DWORD va;const unsigned char*o;const unsigned char*n;int len;}Patch;
typedef struct{DWORD va;}Clamp;
#include "patches.h"
#include "clamp.h"
#include "traps.h"
#include "traps3.h"
typedef void(__thiscall*F0)(void*);
typedef void(__thiscall*F1)(void*,DWORD);
typedef void(__thiscall*F2)(void*,DWORD,DWORD);
typedef void(__thiscall*F3)(void*,DWORD,DWORD,DWORD);
typedef void(__thiscall*F4)(void*,DWORD,DWORD,DWORD,DWORD);
typedef int (__thiscall*Cnt)(void*);
#define VT(o,off) ((*(void***)(o))[(off)/4])
#define GAME (*(BYTE**)0x779f34)
#define PADS (*(BYTE**)0x80c5c0)

static FILE*lg;
static void logf_(const char*f,...){ if(!lg)return; va_list a; va_start(a,f); vfprintf(lg,f,a); va_end(a); fflush(lg); }

int   mp_players=4;              /* wanted count, from CarsMP.ini */
int   mp_live=0;                 /* >2 while a 3-4 player race is set up */
DWORD mp_ext[8];                 /* 0,1 = cam 2,3   3,4 = view 2,3 (written by the game's creation loop) */
void *mp_map[2];                 /* action mappers for ports 2,3 */
int   mp_pad[2]={-1,-1};
DWORD mp_src;                    /* P2 mapper the clones were made from */
char  mp_name[2][64]={"mat","sal"};
char  mp_skin[2][64]={"",""};
int   mp_layout; volatile DWORD mp_frameno,mp_uiseen; int mp_full;

static const int nt=sizeof(traps)/sizeof(*traps), nt3=sizeof(traps3)/sizeof(*traps3);
static const int nc=sizeof(clamps)/sizeof(*clamps), np=sizeof(patches)/sizeof(*patches);

/* ---- camera/view array extension: world+0x30/+0x3c only have two slots ---- */
static DWORD*regp(CONTEXT*c,int r){switch(r){case 0:return &c->Eax;case 1:return &c->Ecx;case 2:return &c->Edx;case 3:return &c->Ebx;case 4:return &c->Esp;case 5:return &c->Ebp;case 6:return &c->Esi;default:return &c->Edi;}}
static int find(const DWORD*t,int n,DWORD a){int lo=0,hi=n-1;while(lo<=hi){int m=(lo+hi)/2;if(t[m]==a)return 1;if(t[m]<a)lo=m+1;else hi=m-1;}return 0;}
static DWORD world(void){BYTE*g=GAME;return g?*(DWORD*)(g+0x450):0;}
static int ext_read(DWORD o,DWORD*v){ if(o>=0x38&&o<0x40){*v=mp_ext[(o-0x38)/4];return 1;} if(o>=0x44&&o<0x4c){*v=mp_ext[3+(o-0x44)/4];return 1;} return 0; }
static LONG trap(EXCEPTION_POINTERS*e){CONTEXT*c=e->ContextRecord;DWORD a=(DWORD)e->ExceptionRecord->ExceptionAddress;
    if(find(traps3,nt3,a)){ /* 8b /r [x+y] */
        BYTE m=*(BYTE*)(a+1),sib=*(BYTE*)(a+2);DWORD x=*regp(c,sib&7),y=*regp(c,(sib>>3)&7),w=world(),o=(x==w)?y:(y==w)?x:0xffffffff,v;
        if(!ext_read(o,&v))v=*(DWORD*)(x+y);
        *regp(c,(m>>3)&7)=v;c->Eip=a+3;return EXCEPTION_CONTINUE_EXECUTION;}
    if(find(traps,nt,a)){ /* 8b /r [base+idx*scale+disp8] */
        BYTE m=*(BYTE*)(a+1),sib=*(BYTE*)(a+2),dsp=*(BYTE*)(a+3);
        DWORD base=*regp(c,sib&7),idx=*regp(c,(sib>>3)&7),off=idx*(1<<(sib>>6))+dsp,v;
        if(base!=world()||!ext_read(off,&v))v=*(DWORD*)(base+off);
        *regp(c,(m>>3)&7)=v;c->Eip=a+4;return EXCEPTION_CONTINUE_EXECUTION;}
    return EXCEPTION_CONTINUE_SEARCH;}
static LONG CALLBACK veh(EXCEPTION_POINTERS*e){
    DWORD code=e->ExceptionRecord->ExceptionCode;
    if(code==0x80000003)return trap(e);
    if(code==0xC0000005&&lg){CONTEXT*c=e->ContextRecord;
        fprintf(lg,"CRASH eip=%08lx addr=%08lx\n stack:",c->Eip,(DWORD)e->ExceptionRecord->ExceptionInformation[1]);
        DWORD*sp=(DWORD*)c->Esp;int n=0;for(int i=0;i<600&&n<16;i++){MEMORY_BASIC_INFORMATION m;if(!VirtualQuery(sp+i,&m,sizeof m)||m.State!=MEM_COMMIT)break;DWORD v=sp[i];if(v>=0x401000&&v<0x711000){fprintf(lg," %08lx",v);n++;}}
        fprintf(lg,"\n");fflush(lg);}
    return EXCEPTION_CONTINUE_SEARCH;}

/* ---- views ---- */
static void get_views(BYTE*w,void**cam,void**view,int n){
    cam[0]=*(void**)(w+0x30);cam[1]=*(void**)(w+0x34);cam[2]=(void*)mp_ext[0];cam[3]=(void*)mp_ext[1];
    view[0]=*(void**)(w+0x3c);view[1]=*(void**)(w+0x40);view[2]=(void*)mp_ext[3];view[3]=(void*)mp_ext[4];
    for(int i=n;i<4;i++){view[i]=0;cam[i]=0;}
}
static void quad_rects(void**view,int n){
    for(int i=0;i<n;i++){ if(!view[i])continue;
        int W=*(int*)((BYTE*)view[i]+0x1a4)/2,H=*(int*)((BYTE*)view[i]+0x1a8)/2;
        ((F4)0x433d10)(view[i],(i&1)?W:0,(i&2)?H:0,(n==3&&i==2)?W*2:W,H);}   /* three players: P3 gets the whole bottom half */
}
/* the shipped render switch dropped its layout 3/4 cases */
__attribute__((used)) void __cdecl mp_render(BYTE*w,int tbl,DWORD arg){
    BYTE*g=GAME; int n=(*(BYTE**)(g+0x468))[0x24]; if(n>4)n=4;
    void*cam[4],*view[4]; get_views(w,cam,view,n);
    int layout=*(int*)(w+0x54);
    ((F4)VT(g,0x68))(g,(DWORD)view[0],(DWORD)view[1],(DWORD)view[2],(DWORD)view[3]);
    for(int i=0;i<n;i++){ if(!cam[i]||!view[i])continue;
        ((F0)VT(cam[i],0x20))(cam[i]);
        ((F4)0x486120)(cam[i],layout,i,arg,i>0?(DWORD)cam[0]:0);
        ((F3)VT(view[i],0x30))(view[i],((DWORD*)0x7c494c)[tbl],*(DWORD*)0x7c4984,*(DWORD*)0x7c4988);
        void*v=(BYTE*)view[i]+0xc; ((F0)VT(v,0x20))(v);
    }
    quad_rects(view,n);
}
/* register cams and views 2,3 next to their siblings */
static void add_after(BYTE*parent,DWORD node,DWORD after,DWORD prio){
    ((F2)0x58f080)(parent,node,prio);
    if(*(DWORD*)(after+0x14)==(DWORD)parent&&*(DWORD*)(after+0xc)!=node){
        DWORD prev=*(DWORD*)(node+8),nxt=*(DWORD*)(node+0xc),an=*(DWORD*)(after+0xc);
        if(prev)*(DWORD*)(prev+0xc)=nxt; if(nxt)*(DWORD*)(nxt+8)=prev;
        *(DWORD*)(node+8)=after;*(DWORD*)(node+0xc)=an;*(DWORD*)(after+0xc)=node;if(an)*(DWORD*)(an+8)=node;
    }
}
__attribute__((used)) void __cdecl mp_attach(BYTE*w){
    if(mp_ext[0])add_after(w,mp_ext[0],*(DWORD*)(w+0x34),2);
    if(mp_ext[1])add_after(w,mp_ext[1],mp_ext[0],2);
    BYTE*m=*(BYTE**)(w+0x44); if(!m)return;
    if(mp_ext[3])add_after(m,mp_ext[3]+0xc,*(DWORD*)(w+0x40)+0xc,(DWORD)-3);
    if(mp_ext[4])add_after(m,mp_ext[4]+0xc,mp_ext[3]+0xc,(DWORD)-3);
}

/* ---- launch: give P3/P4 config and input ---- */
char mp_next[128];
static void race_setup(void);
/* versus road races and Piston Cup go to 3-4 players; mini-games are 1 on 1 rule sets and stay stock */
__attribute__((used)) void __cdecl mp_nextact(const char*n){
    if(!n||IsBadStringPtrA(n,127))return;
    strncpy(mp_next,n,127); logf_("activity load: %s\n",n);
    if(strstr(n,"_VS")&&(!strncmp(n,"RR_",3)||!strncmp(n,"PC_",3)))race_setup();
}
static void race_setup(void){
    BYTE*c=*(BYTE**)(GAME+0x468);
    c[0x27]=c[0x24]>2?2:c[0x24];
    if(*(int*)(c+0x10)!=2||c[0x24]!=2||mp_players<=2)return;
    memset(mp_ext,0,sizeof mp_ext); mp_src=0; mp_full=0;
    void*pl=PADS; int pads=((Cnt)0x6a0b20)(pl);
    int d0=*(int*)0x7c6414,p0=*(int*)0x7c641c,d1=*(int*)0x7c644c,p1=*(int*)0x7c6454;
    BYTE*mgr=*(BYTE**)(*(BYTE**)0x7b5b6c+0x20); int next=0;
    *(int*)((BYTE*)pl+0x12a74)=4;
    for(int i=2;i<mp_players;i++){
        c[0x7c+i]=c[0x7d];
        *(DWORD*)(c+0x80+4*i)=*(DWORD*)(c+0x84);
        *(DWORD*)(c+0x284+4*i)=i;
        while(next<pads&&((d0==1&&p0==next)||(d1==1&&p1==next)))next++;
        BYTE*o=*(BYTE**)(mgr+0x10+4*i); int has=next<pads;
        *(int*)(o+0x718)=has?1:4; *(int*)(o+0x71c)=has?next:-1;
        ((F1)VT(o,4))(o,1);
        if(!mp_map[i-2]){ ((F1)0x6a1210)(pl,(DWORD)&mp_map[i-2]); ((F0)0x6a2a40)((BYTE*)mp_map[i-2]+0x18); }
        mp_pad[i-2]=has?next:-1;
        ((F2)0x6a0e90)(pl,i,(DWORD)mp_map[i-2]);
        ((F1)0x669440)(o,i);
        logf_("P%d car=%s pad=%d of %d (p1 dev %d pad %d, p2 dev %d pad %d)\n",i+1,mp_name[i-2],has?next:-1,pads,d0,p0,d1,p1);
        next++;
    }
    c[0x24]=(BYTE)mp_players; mp_live=mp_players;
    logf_("race launch: %d players\n",mp_players);
}
/* a new world is built per activity: old cam/view pointers die with the old one */
__attribute__((used)) void __cdecl mp_worldstart(void){
    BYTE*g=GAME;BYTE*c=*(BYTE**)(g+0x468);const char*a=mp_next;
    memset(mp_ext,0,sizeof mp_ext); mp_src=0; mp_full=0;
    logf_("world: activity=%s count=%d mode=%d\n",a,c[0x24],*(int*)(c+0x10));
    if(mp_live>2&&!strstr(a,"_VS")){if(c[0x24]>2)c[0x24]=2;c[0x27]=c[0x24];mp_live=0;}
}
/* ---- per frame, on the main thread ---- */
static void clone_bindings(void){
    DWORD src=*(DWORD*)(PADS+0x12a80); if(!src||src==mp_src)return; mp_src=src;
    DWORD*sm=(DWORD*)(src+0x18); int cnt=sm[0],w0=3+*(int*)0x7c6454;   /* binding type = 3 + pad index */
    for(int z=0;z<2;z++){ if(!mp_map[z]||mp_pad[z]<0)continue; DWORD*dm=(DWORD*)((BYTE*)mp_map[z]+0x18); dm[0]=sm[0];
        for(int k=0;k<cnt*6;k++){int ty=(int)sm[1+k*3];dm[1+k*3]=(ty==w0)?(DWORD)(3+mp_pad[z]):(DWORD)ty;dm[2+k*3]=sm[2+k*3];dm[3+k*3]=0;}}
}
static void bind_cameras(void){
    for(int z=0;z<2;z++){DWORD cam=mp_ext[z];if(!cam)continue;DWORD ctl=*(DWORD*)(cam+0x38),tg=*(DWORD*)(cam+0x30);
        if(ctl&&tg&&!*(DWORD*)(ctl+0x128)){DWORD sub=*(DWORD*)(tg+0x6a4);if(sub){DWORD ifc=sub-0x74+0xd0;
            *(DWORD*)(ctl+0x26c)=*(DWORD*)(ifc+0xb8);*(DWORD*)(ctl+0x128)=ifc;*(DWORD*)(sub-0x74+0x30)|=2;}}}
}
/* full screen widgets (pause, results, rewards, dialogs) only draw on layouts 0-2 and the UI pass runs
   per view, so while one is up render a single full screen view, then go back to the split */
static void full_view(int on){
    BYTE*g=GAME;BYTE*w=(BYTE*)world();if(!w)return;
    void*cam[4],*view[4];get_views(w,cam,view,mp_live);
    if(on){ mp_layout=*(int*)(w+0x54); *(int*)(w+0x54)=0;
        ((F4)VT(g,0x68))(g,(DWORD)view[0],0,0,0);
        int W=*(int*)((BYTE*)view[0]+0x1a4),H=*(int*)((BYTE*)view[0]+0x1a8); ((F4)0x433d10)(view[0],0,0,W,H);
    } else { *(int*)(w+0x54)=mp_layout;
        ((F4)VT(g,0x68))(g,(DWORD)view[0],(DWORD)view[1],(DWORD)view[2],(DWORD)view[3]);
        quad_rects(view,mp_live); }
}
int mp_uncap=0; DWORD mp_tid;
#ifdef TESTAI
static DWORD WINAPI sampler(LPVOID u){ for(;;){ Sleep(137); if(!mp_tid||mp_live<=2)continue; static int n; if(n++>40)return 0; HANDLE th=OpenThread(THREAD_ALL_ACCESS,0,mp_tid); if(!th)continue; SuspendThread(th); CONTEXT cx; cx.ContextFlags=CONTEXT_CONTROL; GetThreadContext(th,&cx);
    fprintf(lg,"S eip=%08lx:",cx.Eip); DWORD*sp=(DWORD*)cx.Esp;int q=0;for(int i=0;i<500&&q<6;i++){MEMORY_BASIC_INFORMATION mb;if(!VirtualQuery(sp+i,&mb,sizeof mb)||mb.State!=MEM_COMMIT)break;DWORD v=sp[i];if(v>=0x401000&&v<0x711000){fprintf(lg," %08lx",v);q++;}} fprintf(lg,"\n"); fflush(lg); ResumeThread(th); CloseHandle(th);} }
#endif
__attribute__((used)) void __cdecl mp_frame(void){
    BYTE*g=GAME; if(!g)return; BYTE*c=*(BYTE**)(g+0x468); if(!c)return;
    mp_tid=GetCurrentThreadId();
    /* frame rate: the game defaults to a fixed 0.033s step per frame and a 30Hz limiter (timer+0x34).
       Switch to the engine's measured time step and raise the limiter. timer+0x38 stays at 1/30, so a
       frame slower than that still gets a capped step, same as stock */
    if(mp_uncap){ *(int*)0x7b5b60=0; BYTE*ren=*(BYTE**)0x7c6394; if(ren)*(int*)(ren+0x10)=0;
        *(float*)0x7b5c4c=1.0f/mp_uncap; }
    c[0x27]=c[0x24]>2?2:c[0x24];
    mp_frameno++;
    if(mp_live<=2)return;
    if(c[0x24]<=2){mp_live=0;mp_full=0;return;}
    clone_bindings(); bind_cameras();
    { static int streak; int seen=(mp_frameno-mp_uiseen)<=2; streak=seen?streak+1:0;
      int want=mp_full?seen:(streak>=6);
      if(want!=mp_full&&mp_ext[3]){mp_full=want;full_view(want);} }
#ifdef TESTAI
    { static DWORD t0,f0; DWORD t=GetTickCount(); if(!t0){t0=t;f0=mp_frameno;} if(t-t0>=5000){logf_("fps %.1f dt=%.4f\n",(mp_frameno-f0)*1000.0/(t-t0),*(float*)(*(BYTE**)0x7b5b6c+0x280));t0=t;f0=mp_frameno;} }
    { static int fr; if(!mp_full&&++fr==900){ BYTE*act=*(BYTE**)(*(BYTE**)(g+0x44c)+0x4c); DWORD*v=(DWORD*)(act+0xcb0); int cnt=*(int*)(act+0xc2c);
        logf_("TESTAI: %d racers\n",cnt);
        for(int i=0;i<cnt;i++){ DWORD r=*(DWORD*)(v[0]+4*i); if(*(int*)(r+0x80)==1){ BYTE*ct=*(BYTE**)(r+0x8c); *(int*)(ct+0x3d4)=1; ((F0)VT(ct,0x20))(ct); logf_(" ai on racer %d\n",i);} } } }
#endif
}
extern void hook_hudreg(void),hook_viewloop(void),hook_render(void),hook_attach(void),hook_pname(void),hook_pskin(void),hook_ai(void),hook_frame(void),hook_world(void),hook_act(void),hook_uidraw(void);
__asm__(".text\n"
 /* 00535f90 HUD vehicle registration only has two slots */
 ".globl _hook_hudreg\n_hook_hudreg:\n  cmpl $2,4(%esp)\n  jl 1f\n  ret $8\n1:\n  pushl %ebp\n  movl 0xc(%esp),%ebp\n  pushl $0x535f95\n  ret\n"
 /* 00556fd8 creation loop: continue into mp_ext past index 1 */
 ".globl _hook_viewloop\n_hook_viewloop:\n  addl $4,%esi\n  cmpl $2,%eax\n  jne 1f\n  movl $_mp_ext+12,%esi\n1:\n"
 "  movl 0x779f34,%ecx\n  movl 0x468(%ecx),%ecx\n  movzbl 0x24(%ecx),%ecx\n  cmpl $2,%ecx\n  jge 2f\n  movl $2,%ecx\n2:\n  cmpl %ecx,%eax\n  pushl $0x556fde\n  ret\n"
 /* 005567dd render switch fallthrough */
 ".globl _hook_render\n_hook_render:\n  jne 1f\n  pushl $0x5567e3\n  ret\n1:\n  pushal\n  pushl 0x34(%esp)\n  pushl %ebp\n  pushl %esi\n  call _mp_render\n  addl $12,%esp\n  popal\n  pushl $0x5569ba\n  ret\n"
 /* 00555d89 end of scene attach */
 ".globl _hook_attach\n_hook_attach:\n  pushal\n  pushl %esi\n  call _mp_attach\n  addl $4,%esp\n  popal\n  popl %edi\n  popl %esi\n  movl $1,%eax\n  pushl $0x555d90\n  ret\n"
 /* 004396bb / 0043970b name and skin slots collide past index 1 */
 ".globl _hook_pname\n_hook_pname:\n  pushl $0x40\n  leal -0x40(%edx,%edi),%eax\n  cmpl $2,%ebp\n  jl 1f\n  movl %ebp,%eax\n  subl $2,%eax\n  shll $6,%eax\n  addl $_mp_name,%eax\n1:\n  pushl $0x4396c1\n  ret\n"
 ".globl _hook_pskin\n_hook_pskin:\n  pushl $0x40\n  addl %edi,%ecx\n  cmpl $2,%ebp\n  jl 1f\n  movl %ebp,%ecx\n  subl $2,%ecx\n  shll $6,%ecx\n  addl $_mp_skin,%ecx\n1:\n  pushl %ecx\n  pushl $0x439710\n  ret\n"
 /* 004395ea each extra human replaces one AI car */
 ".globl _hook_ai\n_hook_ai:\n  movzbl 0x25(%eax),%eax\n  cmpl $2,%ecx\n  jle 1f\n  addl $2,%eax\n  subl %ecx,%eax\n  jge 1f\n  xorl %eax,%eax\n1:\n  movl %eax,0xc24(%ebx)\n  pushl $0x4395f4\n  ret\n"
 /* 005046f1 full screen widget draw: note the frame */
 ".globl _hook_uidraw\n_hook_uidraw:\n  movl _mp_frameno,%ecx\n  movl %ecx,_mp_uiseen\n  movl 0x779f34,%ecx\n  pushl $0x5046f7\n  ret\n"
 /* 00440350 activity load */
 ".globl _hook_act\n_hook_act:\n  pushal\n  pushl 0x24(%esp)\n  call _mp_nextact\n  addl $4,%esp\n  popal\n  movl 0x779f34,%eax\n  pushl $0x440355\n  ret\n"
 /* 005577f6 world build */
 ".globl _hook_world\n_hook_world:\n  pushal\n  call _mp_worldstart\n  popal\n  pushl $0x556e40\n  ret\n"
 /* 0058c35d main loop, input tick call */
 ".globl _hook_frame\n_hook_frame:\n  pushal\n  call _mp_frame\n  popal\n  pushl $0x657c50\n  ret\n"
);
typedef struct{DWORD va;const char*o;int len;void*to;}Hook;
static const Hook hooks[]={
 {0x5046f1,"\x8b\x0d\x34\x9f\x77\x00",6,hook_uidraw},
 {0x440350,"\xa1\x34\x9f\x77\x00",5,hook_act},
 {0x535f90,"\x55\x8b\x6c\x24\x0c",5,hook_hudreg},
 {0x5567dd,"\x0f\x85\xd7\x01\x00\x00",6,hook_render},
 {0x4396bb,"\x6a\x40\x8d\x44\x3a\xc0",6,hook_pname},
 {0x43970b,"\x6a\x40\x03\xcf\x51",5,hook_pskin},
 {0x4395ea,"\x0f\xb6\x40\x25\x89\x83\x24\x0c\x00\x00",10,hook_ai},
 {0x555d89,"\x5f\x5e\xb8\x01\x00\x00\x00",7,hook_attach},
 {0x556fd8,"\x83\xc6\x04\x83\xf8\x02",6,hook_viewloop},
};
static const int nh=sizeof(hooks)/sizeof(*hooks);
static void wbyte(DWORD a,BYTE v){DWORD o;VirtualProtect((void*)a,1,PAGE_EXECUTE_READWRITE,&o);*(BYTE*)a=v;VirtualProtect((void*)a,1,o,&o);}
static int ready(int report){int ok=1;
    #define BAD(...) {ok=0;if(report)logf_(__VA_ARGS__);else return 0;}
    for(int k=0;k<np;k++) if(memcmp((void*)patches[k].va,patches[k].o,patches[k].len))BAD("MISMATCH %08lx\n",patches[k].va);
    for(int k=0;k<nc;k++) if(*(BYTE*)clamps[k].va!=0x24)BAD("MISMATCH clamp %08lx\n",clamps[k].va);
    for(int k=0;k<nh;k++) if(memcmp((void*)hooks[k].va,hooks[k].o,hooks[k].len))BAD("MISMATCH hook %08lx\n",hooks[k].va);
    for(int k=0;k<nt;k++) if(*(BYTE*)traps[k]!=0x8b)BAD("MISMATCH trap %08lx\n",traps[k]);
    for(int k=0;k<nt3;k++) if(*(BYTE*)traps3[k]!=0x8b)BAD("MISMATCH trap %08lx\n",traps3[k]);
    if(*(BYTE*)0x58c35d!=0xe8||*(DWORD*)0x58c35e!=0x657c50-0x58c362)BAD("MISMATCH frame call\n");
    if(*(BYTE*)0x5577f6!=0xe8||*(DWORD*)0x5577f7!=0x556e40-0x5577fb)BAD("MISMATCH world call\n");
    return ok;}
static DWORD WINAPI worker(LPVOID u){
    lg=fopen("CarsMP.log","w");
    mp_players=GetPrivateProfileIntA("CarsMP","Players",4,".\\CarsMP.ini"); if(mp_players<2)mp_players=2; if(mp_players>4)mp_players=4;
    GetPrivateProfileStringA("CarsMP","Player3Car","mat",mp_name[0],64,".\\CarsMP.ini");
    GetPrivateProfileStringA("CarsMP","Player4Car","sal",mp_name[1],64,".\\CarsMP.ini");
    mp_uncap=GetPrivateProfileIntA("CarsMP","FPS",144,".\\CarsMP.ini"); if(mp_uncap>0&&mp_uncap<30)mp_uncap=30;
    logf_("CarsMP players=%d fps=%d\n",mp_players,mp_uncap);
    int ok=0; for(int i=0;i<60000&&!ok;i++){ok=ready(0);if(!ok)Sleep(5);}
    if(!ok){ready(1);logf_("ABORT: not the expected game build, nothing applied\n");return 0;}
    if(mp_players<=2){
        if(mp_uncap){DWORD o;VirtualProtect((void*)0x58c35e,4,PAGE_EXECUTE_READWRITE,&o);*(DWORD*)0x58c35e=(DWORD)hook_frame-0x58c362;VirtualProtect((void*)0x58c35e,4,o,&o);FlushInstructionCache(GetCurrentProcess(),NULL,0);}
        logf_("players=2: no multiplayer patches\n");return 0;}
    AddVectoredExceptionHandler(1,veh);
    for(int k=0;k<np;k++){DWORD o;VirtualProtect((void*)patches[k].va,patches[k].len,PAGE_EXECUTE_READWRITE,&o);memcpy((void*)patches[k].va,patches[k].n,patches[k].len);VirtualProtect((void*)patches[k].va,patches[k].len,o,&o);}
    for(int k=0;k<nc;k++)wbyte(clamps[k].va,0x27);
    for(int k=0;k<nt;k++)wbyte(traps[k],0xCC);
    for(int k=0;k<nt3;k++)wbyte(traps3[k],0xCC);
    for(int k=0;k<nh;k++){BYTE*a=(BYTE*)hooks[k].va;DWORD o;VirtualProtect(a,hooks[k].len,PAGE_EXECUTE_READWRITE,&o);memset(a,0x90,hooks[k].len);a[0]=0xE9;*(DWORD*)(a+1)=(DWORD)hooks[k].to-(hooks[k].va+5);VirtualProtect(a,hooks[k].len,o,&o);}
    {DWORD o;VirtualProtect((void*)0x58c35e,4,PAGE_EXECUTE_READWRITE,&o);*(DWORD*)0x58c35e=(DWORD)hook_frame-0x58c362;VirtualProtect((void*)0x58c35e,4,o,&o);}
    {DWORD o;VirtualProtect((void*)0x5577f7,4,PAGE_EXECUTE_READWRITE,&o);*(DWORD*)0x5577f7=(DWORD)hook_world-0x5577fb;VirtualProtect((void*)0x5577f7,4,o,&o);}
    FlushInstructionCache(GetCurrentProcess(),NULL,0);
#ifdef TESTAI
    CreateThread(NULL,0,sampler,NULL,0,NULL);
#endif
    logf_("ready: %d patches, %d clamps, %d traps, %d hooks\n",np,nc,nt+nt3,nh);
    return 0;
}
BOOL WINAPI DllMain(HINSTANCE h,DWORD r,LPVOID p){
    if(r==DLL_PROCESS_ATTACH){DisableThreadLibraryCalls(h);CreateThread(NULL,0,worker,NULL,0,NULL);}
    return TRUE;
}
