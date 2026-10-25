#include "listening_state.h"
#include <assert.h>
#include <stdio.h>
static void init(li_state_t *s){li_init(s,NULL,123);s->voice_enabled=true;}
static void start(li_state_t *s,unsigned mode){s->menu=mode;li_key(s,2,false,0);if(mode!=2)li_key(s,2,false,0);}
int main(void){
 li_state_t s;init(&s);assert(s.progress.volume==60);start(&s,0);assert(s.page==LI_LISTEN&&s.count==48&&s.voice==0);
 uint32_t old=s.serial;li_key(&s,1,false,10);assert(s.voice==1);li_audio_done(&s,old,20);assert(!s.waiting&&!li_heard(&s));
 li_audio_done(&s,s.serial,30);assert(s.waiting&&li_heard(&s)==1);li_tick(&s,2529);assert(s.pos==1);li_tick(&s,2530);assert(s.pos==2);
 li_key(&s,2,false,2600);assert(s.paused&&s.voice==-1);li_tick(&s,20000);assert(s.pos==2);
 li_key(&s,2,false,20001);assert(!s.paused&&s.voice==2);li_tick(&s,600000);assert(s.page==LI_FINISH&&s.voice==-1&&s.timed_out);
 init(&s);start(&s,2);assert(s.page==LI_FINISH&&s.count==0);
 init(&s);s.progress.mistakes[383]=1;start(&s,2);assert(s.count==1&&s.voice==383);
 li_audio_done(&s,s.serial,10);li_tick(&s,2510);assert(s.page==LI_FINISH);assert(s.progress.mistakes[383]);
 for(unsigned seed=1;seed<200;seed++){
  li_init(&s,NULL,seed);s.voice_enabled=true;s.topic=seed%8;start(&s,1);assert(s.count==8);
  for(unsigned i=0;i<8;i++){
   unsigned id=li_current(&s);for(unsigned j=0;j<i;j++)assert(s.order[j]!=id);
   assert(s.options[0]!=s.options[1]);assert(s.options[0]==id||s.options[1]==id);
   li_key(&s,s.options[0]==id?0:1,false,100+i*10);li_key(&s,2,false,101+i*10);
   assert(s.page==LI_FEEDBACK&&s.right);li_key(&s,2,false,102+i*10);
  }
  assert(s.page==LI_FINISH&&s.correct==8&&s.progress.stamps[s.topic]==3);
 }
 init(&s);start(&s,1);unsigned id=li_current(&s);li_key(&s,s.options[0]!=id?0:1,false,1);li_key(&s,2,false,2);assert(s.progress.mistakes[id]&&s.correct==0);
 li_key(&s,2,true,3);assert(s.page==LI_HOME&&s.voice==-1);
 init(&s);start(&s,1);s.voice_enabled=false;li_key(&s,2,false,1);assert(s.page==LI_QUIZ&&!s.correct);
 li_progress_t corrupt={.version=1,.volume=255,.minutes=255,.stamps={255},.heard={255},.mistakes={255}};
 li_init(&s,&corrupt,0);assert(s.progress.volume==60&&s.progress.minutes==10&&s.progress.stamps[0]==0&&s.progress.heard[0]==1);
 li_progress_t previous={.version=1,.volume=45,.minutes=10,.heard={1},.mistakes={1},.stamps={2}};
 li_init(&s,&previous,1);assert(s.progress.version==2&&s.progress.volume==60&&s.dirty&&s.progress.stamps[0]==2&&s.progress.heard[0]&&s.progress.mistakes[0]);
 s.page=LI_SETTINGS;s.setting=0;for(unsigned i=0;i<5;i++)li_key(&s,2,false,i);assert(s.progress.volume==60);
 li_state_t again;li_init(&again,&s.progress,1);assert(again.progress.volume==60&&!again.dirty);
 init(&s);start(&s,0);s.started=UINT32_MAX-100;li_tick(&s,400);assert(s.page==LI_LISTEN);li_audio_done(&s,s.serial,UINT32_MAX-100);li_tick(&s,2500);assert(s.pos==1);
 /* Every current volume survives loading, including after a settings change. */
 for(unsigned level=20;level<=100;level+=20){
  li_progress_t p={.version=LI_SAVE_VERSION,.volume=level,.minutes=10};
  li_init(&s,&p,1);assert(s.progress.volume==level&&!s.dirty);
  s.page=LI_SETTINGS;li_key(&s,2,false,0);
  li_init(&again,&s.progress,1);assert(again.progress.volume==s.progress.volume);
 }
 for(unsigned level=15;level<=75;level+=15){
  li_progress_t p={.version=1,.volume=level,.minutes=10};
  li_init(&s,&p,1);assert(s.progress.volume==level/15*20&&s.dirty);
 }
 unsigned bad[]={0,15,30,75,101,255};
 for(unsigned i=0;i<sizeof(bad)/sizeof(bad[0]);i++){
  li_progress_t p={.version=LI_SAVE_VERSION,.volume=bad[i],.minutes=10};
  li_init(&s,&p,1);assert(s.progress.volume==60);
 }
 /* Idle timeout, wake-only gestures, normal next press, and clock wrap. */
 for(unsigned key=0;key<3;key++){
  li_screen_t screen;li_screen_init(&screen,100);
  li_screen_tick(&screen,100+LI_SCREEN_IDLE_MS-1);assert(!screen.off);
  li_screen_tick(&screen,100+LI_SCREEN_IDLE_MS);assert(screen.off);
  init(&s);s.menu=4;uint32_t serial=s.serial;
  assert(!li_screen_key(&screen,key,true,600101));assert(!screen.off);
  if(li_screen_key(&screen,key,false,600102))li_key(&s,key,true,600102);
  if(li_screen_key(&screen,key,false,600103))li_key(&s,key,false,600103);
  assert(s.page==LI_HOME&&s.menu==4&&s.serial==serial);
  assert(!li_screen_key(&screen,key,true,600200));
  assert(li_screen_key(&screen,key,false,600201));
  li_screen_tick(&screen,600201+LI_SCREEN_IDLE_MS-1);assert(!screen.off);
  li_screen_tick(&screen,600201+LI_SCREEN_IDLE_MS);assert(screen.off);
 }
 /* An ordinary press restarts the full timeout and keeps its normal action. */
 li_screen_t screen;li_screen_init(&screen,0);
 assert(!li_screen_key(&screen,0,true,LI_SCREEN_IDLE_MS-1));
 assert(li_screen_key(&screen,0,false,LI_SCREEN_IDLE_MS));
 li_screen_tick(&screen,2*LI_SCREEN_IDLE_MS-1);assert(!screen.off);
 li_screen_tick(&screen,2*LI_SCREEN_IDLE_MS);assert(screen.off);
 /* If a press event is lost, its action still only wakes the screen. */
 assert(!li_screen_key(&screen,2,false,2*LI_SCREEN_IDLE_MS+1));assert(!screen.off);
 li_screen_init(&screen,UINT32_MAX-100);
 li_screen_tick(&screen,LI_SCREEN_IDLE_MS-102);assert(!screen.off);
 li_screen_tick(&screen,LI_SCREEN_IDLE_MS-101);assert(screen.off);
 /* Playback still advances while the screen is off. */
 init(&s);start(&s,0);li_audio_done(&s,s.serial,0);li_tick(&s,2500);
 assert(screen.off&&s.pos==1);
 puts("Listening state: PASS (shuffle, marks, pause, timer, cancellation, volume reload/migration, screen timeout/wake, corrupt saves, wraparound)");
}
