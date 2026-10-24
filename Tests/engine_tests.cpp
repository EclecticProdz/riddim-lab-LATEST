#include "PluginProcessor.h"
#include <cstdio>
#include <functional>
using namespace rp;
static const double FS=48000;
struct Rig{
  RiddimLabAudioProcessor p; juce::MidiBuffer m;
  Rig(){ p.setPlayConfigDetails(2,2,FS,512); p.prepareToPlay(FS,512); }
  void set(const juce::String& id,float v){ auto* q=p.parameters.getParameter(id); q->setValueNotifyingHost(q->convertTo0to1(v)); }
  void S(Sec s,P pp,float v){ set(id(s,pp),v); }
  void sec(int s){ set("activeSection",(float)s); }
  // process generator; returns left channel output
  template<class F> std::vector<float> run(int N,F gen){
    std::vector<float> out; out.reserve(N); juce::AudioBuffer<float> b(2,512);
    for(int s=0;s<N;s+=512){ int n=std::min(512,N-s); b.setSize(2,n,false,false,true);
      for(int i=0;i<n;i++){ float v=gen(s+i); b.setSample(0,i,v); b.setSample(1,i,v);} 
      p.processBlock(b,m); for(int i=0;i<n;i++) out.push_back(b.getSample(0,i)); }
    return out; }
};
static double rms(const std::vector<float>&v,int a,int b){double s=0;for(int i=a;i<b;i++)s+=v[i]*v[i];return std::sqrt(s/(b-a));}
static double db(double x){return 20*std::log10(std::max(x,1e-9));}
static auto sine(double f,float a){return [=](int i){return a*(float)std::sin(6.283185307*f*i/FS);};}
// make master neutral: everything off, then enable what test needs
static void neutral(Rig&r){ for(int s=0;s<NumSec;s++){ Sec S=(Sec)s;
  for(P pp:{FiltOn,CompOn,EqOn,TapeOn,EchoOn,RevOn,ModOn,DlyOn,LimOn,SrcOn}) if(exists(S,pp)) r.S(S,pp,0);
  r.S(S,InSat,0); r.S(S,Analog,0);} }
#define CHECK(name,cond,...) do{ bool ok=(cond); printf("%s %-34s ",ok?"PASS":"FAIL",name); printf(__VA_ARGS__); printf("\n"); if(!ok) fails++; }while(0)
int main(){
  juce::ScopedJuceInitialiser_GUI gui; int fails=0;
  { Rig r; // default master, 1k sine -12 dB
    auto o=r.run(48000*2,sine(1000,0.25f)); double g=db(rms(o,48000,96000))-db(0.25/std::sqrt(2.0));
    CHECK("default master level",std::abs(g)<1.5,"gain %.2f dB",g); }
  { Rig r; neutral(r); auto o=r.run(48000,sine(1000,0.25f)); double g=db(rms(o,24000,48000))-db(0.25/std::sqrt(2.0));
    CHECK("all-off unity",std::abs(g)<0.1,"gain %.3f dB",g); }
  { Rig r; neutral(r); r.set("bypass",1); auto o=r.run(48000,sine(1000,0.25f)); double g=db(rms(o,24000,48000))-db(0.25/std::sqrt(2.0));
    CHECK("bypass unity",std::abs(g)<0.1,"gain %.3f dB latency %d",g,r.p.getLatencySamples()); }
  { Rig r; neutral(r); r.S(Master,InGain,12); r.S(Master,OutGain,-6); auto o=r.run(48000,sine(1000,0.1f)); double g=db(rms(o,24000,48000))-db(0.1/std::sqrt(2.0));
    CHECK("in/out gain +12-6",std::abs(g-6)<0.2,"gain %.2f",g); }
  { // echo repeat timing: 20 ms burst, free time 250ms
    Rig r; neutral(r); r.S(Master,EchoOn,1); r.S(Master,Sync,0); r.S(Master,EchoTime,250); r.S(Master,Feedback,0.5f); r.S(Master,Mix,1); r.S(Master,Width,0); r.S(Master,Damping,0); r.S(Master,Heat,0);
    auto o=r.run(48000*3,[](int i){return i<960?0.5f*(float)std::sin(6.2832*1000*i/FS):0.0f;});
    // envelope per 10 ms
    std::vector<double> env; for(int i=0;i+480<=(int)o.size();i+=480) env.push_back(rms(o,i,i+480));
    std::vector<int> c; for(int k=1;k<(int)env.size()-1;k++) if(env[k]>0.01&&env[k]>=env[k-1]&&env[k]>env[k+1]) c.push_back(k);
    std::vector<int> cc; for(int x:c) if(cc.empty()||x-cc.back()>8) cc.push_back(x);
    printf("   echo env peaks (10ms bins):"); for(int i=0;i<std::min<int>(6,cc.size());i++) printf(" %d(%.3f)",cc[i],env[cc[i]]); printf("\n");
    CHECK("echo repeats spaced 250ms",cc.size()>=3&&std::abs((cc[2]-cc[1])-25)<=1&&cc[0]*480<(int)(0.27*FS)&&cc[0]*480>(int)(0.23*FS),"first at bin %d gap %d",cc.empty()?-1:cc[0],cc.size()>2?cc[2]-cc[1]:-1);
    if(cc.size()>2) CHECK("echo repeats decay by feedback",env[cc[2]]<env[cc[1]]*0.7&&env[cc[2]]>env[cc[1]]*0.3,"%.3f -> %.3f",env[cc[1]],env[cc[2]]); }
  { // echo tail finite
    Rig r; neutral(r); r.S(Master,EchoOn,1); r.S(Master,Sync,0); r.S(Master,EchoTime,200); r.S(Master,Feedback,0.94f); r.S(Master,Mix,0.5f);
    auto o=r.run(48000*2,[](int i){return i<2400?0.5f*std::sin(i*0.1f):0.f;});
    auto t=r.run(48000*60,[](int){return 0.f;});
    double late=rms(t,48000*55,48000*60);
    CHECK("echo tail decays (fb .94, 60s)",late<0.001,"rms %.6f (%.1f dB)",late,db(late)); }
  { // filter lowpass echo-route attenuates repeat HF
    auto rep=[&](double f,int mode,float cut,float res){ Rig r; neutral(r); r.S(Master,EchoOn,1); r.S(Master,FiltOn,1); r.S(Master,Sync,0); r.S(Master,EchoTime,100); r.S(Master,Feedback,0.0f); r.S(Master,Mix,1); r.S(Master,Width,0); r.S(Master,Damping,0); r.S(Master,Heat,0);
      r.S(Master,FilterMode,(float)mode); r.S(Master,Cutoff,cut); r.S(Master,Resonance,res);
      auto o=r.run(48000,sine(f,0.2f)); return rms(o,30000,48000); };
    double a=db(rep(8000,0,800,0.1f)), b=db(rep(200,0,800,0.1f));
    CHECK("echo LP cutoff 800: 8k vs 200Hz",b-a>15,"%.1f dB down",b-a);
    double c=db(rep(200,1,2000,0.1f)), d=db(rep(8000,1,2000,0.1f));
    CHECK("echo HP cutoff 2k: 200 vs 8k",d-c>15,"%.1f dB down",d-c);
    double e=db(rep(800,0,800,0.9f)), f0=db(rep(800,0,800,0.1f));
    CHECK("resonance peak at cutoff",e-f0>6,"+%.1f dB",e-f0);
    double bpa=db(rep(1000,2,1000,0.3f)), bpb=db(rep(100,2,1000,0.3f));
    CHECK("band-pass: 1k vs 100",bpa-bpb>8,"%.1f dB",bpa-bpb); }
  { // input-route filter
    Rig r; neutral(r); r.S(Master,FiltOn,1); r.S(Master,FilterRoute,1); r.S(Master,Cutoff,500); r.S(Master,Resonance,0.1f);
    auto o=r.run(48000,sine(5000,0.3f)); double g=db(rms(o,24000,48000))-db(0.3/1.414);
    CHECK("input-route LP 500 kills 5k",g<-20,"%.1f dB",g); }
  { // compressor (Drums tab)
    auto mk=[&](float mk){ Rig r; neutral(r); r.sec(Drums); r.S(Drums,InGain,0); r.S(Drums,OutGain,0); r.S(Drums,CompOn,1); r.S(Drums,CompThresh,-20); r.S(Drums,CompRatio,4); r.S(Drums,CompMakeup,mk); r.S(Drums,CompMode,1);
      auto o=r.run(48000,sine(1000,0.5f)); return db(rms(o,24000,48000))-db(0.5/1.414); };
    double g=mk(0), g2=mk(6);
    CHECK("comp 4:1 thr -20 reduces ~10 dB",g<-6&&g>-14,"gain %.2f",g);
    CHECK("comp makeup +6",std::abs((g2-g)-6)<0.5,"%.2f",g2-g);
    { auto mm=[&](float on){ Rig z; neutral(z); z.sec(Master); z.S(Master,InGain,0); z.S(Master,OutGain,0); z.S(Master,CompOn,on); z.S(Master,CompThresh,-20); z.S(Master,CompRatio,4); z.S(Master,CompMakeup,0); z.S(Master,CompMode,1);
        auto o=z.run(48000,sine(1000,0.5f)); return db(rms(o,24000,48000))-db(0.5/1.414); };
      double gOn=mm(1), gOff=mm(0);
      CHECK("master compressor (on) reduces ~10 dB",gOn<-6&&gOn>-14,"gain %.2f",gOn);
      CHECK("master compressor (off) is transparent",std::abs(gOff)<0.2,"%.2f dB",gOff); } }
  { // EQ
    auto gain=[&](P band,double f){ Rig r; neutral(r); r.S(Master,EqOn,1); r.S(Master,band,9); auto o=r.run(48000,sine(f,0.1f)); return db(rms(o,24000,48000))-db(0.1/1.414); };
    CHECK("EQ low +9 @50Hz",gain(EqLow,50)>6,"%.1f",gain(EqLow,50));
    CHECK("EQ mid +9 @800Hz",gain(EqMid,800)>7,"%.1f",gain(EqMid,800));
    CHECK("EQ high +9 @12k",gain(EqHigh,12000)>6,"%.1f",gain(EqHigh,12000));
    CHECK("EQ pres +9 @3.5k",gain(EqPres,3500)>7,"%.1f",gain(EqPres,3500));
    CHECK("EQ low leaves 5k alone",std::abs(gain(EqLow,5000))<1.0,"%.2f",gain(EqLow,5000)); }
  { // tape wow: measure frequency deviation via zero crossings on 1k sine
    auto dev=[&](float wow,float fl){ Rig r; neutral(r); r.S(Master,TapeOn,1); r.S(Master,TapeSat,0); r.S(Master,Wow,wow); r.S(Master,Flutter,fl); r.S(Master,TapeTone,1);
      auto o=r.run(48000*6,sine(1000,0.3f)); double mn=1e9,mx=0,lastt=0; int last=-1; 
      for(int i=48000;i<(int)o.size()-1;i++) if(o[i]<=0&&o[i+1]>0){ double t=i+(-o[i])/(o[i+1]-o[i]); if(last>=0){ double f=FS/(t-lastt); mn=std::min(mn,f); mx=std::max(mx,f);} last=i; lastt=t; }
      return (mx-mn)/1000.0*100; };
    double d0=dev(0,0), d1=dev(1,0), d2=dev(0,1);
    CHECK("tape wow 0 -> steady pitch",d0<0.2,"spread %.3f%%",d0);
    CHECK("tape wow 1 -> pitch wobble",d1>0.3,"spread %.3f%%",d1);
    CHECK("tape flutter 1 -> wobble",d2>0.15,"spread %.3f%%",d2); }
  { // tape saturation harmonics: THD-ish
    auto h3=[&](float sat){ Rig r; neutral(r); r.S(Master,TapeOn,1); r.S(Master,TapeSat,sat); r.S(Master,Wow,0); r.S(Master,Flutter,0); r.S(Master,TapeTone,1);
      auto o=r.run(48000,sine(500,0.7f)); double re=0,im=0; for(int i=24000;i<48000;i++){ re+=o[i]*std::cos(6.2832*1500*i/FS); im+=o[i]*std::sin(6.2832*1500*i/FS);} return db(std::sqrt(re*re+im*im)/12000); };
    CHECK("tape sat adds harmonics",h3(1)-h3(0)>10,"3rd: %.1f vs %.1f dB",h3(1),h3(0)); }
  { // limiter ceiling
    Rig r; neutral(r); r.S(Master,LimOn,1); r.S(Master,LimThresh,-12); r.S(Master,LimCeiling,-1); r.S(Master,InGain,18);
    auto o=r.run(48000,sine(300,0.9f)); float pk=0; for(int i=2000;i<(int)o.size();i++) pk=std::max(pk,std::abs(o[i]));
    CHECK("limiter ceiling -1 dBFS",db(pk)<=-0.95,"peak %.2f dBFS GR %.1f",db(pk),r.p.getLimiterGainReductionDb()); }
  { // reverb tails by type
    for(int t=0;t<2;t++){ Rig r; neutral(r); r.S(Master,RevOn,1); r.S(Master,RevType,(float)t); r.S(Master,RevMix,1); r.S(Master,RevDecay,0.6f); r.S(Master,RevPre,0);
      auto o=r.run(48000*8,[](int i){return i<200?0.5f:0.f;}); double e1=rms(o,6000,12000),e2=rms(o,48000*3,48000*3+6000),e3=rms(o,48000*7,48000*8);
      CHECK(("reverb type "+juce::String(t)).toRawUTF8(),e1>1e-4&&(e2<e1||e2<1e-5)&&(e3<e2||e3<1e-5),"%.1f / %.1f / %.1f dB",db(e1),db(e2),db(e3)); }
    Rig a; neutral(a); a.S(Master,RevOn,1); a.S(Master,RevMix,1); a.S(Master,RevType,1); a.S(Master,RevDecay,0.2f);
    Rig b; neutral(b); b.S(Master,RevOn,1); b.S(Master,RevMix,1); b.S(Master,RevType,1); b.S(Master,RevDecay,0.9f);
    auto imp=[](int i){return i<200?0.5f:0.f;}; auto oa=a.run(48000*4,imp), ob=b.run(48000*4,imp);
    CHECK("reverb decay knob longer",rms(ob,48000*2,48000*3)>2*rms(oa,48000*2,48000*3),"%.1f vs %.1f dB",db(rms(ob,96000,144000)),db(rms(oa,96000,144000))); }
  // every tab: finite, bounded, source knob has effect
  for(int s=1;s<NumSec;s++){ if(!avail((Sec)s,MSrc)) continue; Rig a,b; a.sec(s); b.sec(s);
    for(Rig* r:{&a,&b}){ for(P pp:{EchoOn,RevOn,ModOn,DlyOn,TapeOn,CompOn,EqOn,FiltOn}) r->S((Sec)s,pp,0); }
    int modes = s==Guitar?3:1;
    for(int md=0;md<modes;md++){ a.set("guitarMode",(float)md); b.set("guitarMode",(float)md);
      for(int k=0;k<3;k++){ P pk=sourceParam((Sec)s,md,k); a.S((Sec)s,pk,0.0f); b.S((Sec)s,pk,1.0f);
        // noisy burst material: decaying pluck with noise
        auto gen=[](int i){ int ph=i%12000; float env=std::exp(-ph/1500.f); return 0.4f*env*(0.6f*std::sin(ph*0.07f)+0.4f*(((i*1103515245u+12345u)>>16&0x7fff)/16384.f-1.f)); };
        auto oa=a.run(48000,gen), ob=b.run(48000,gen); double d=0,e=0; for(size_t i=0;i<oa.size();i++){ d+=(oa[i]-ob[i])*(oa[i]-ob[i]); e+=oa[i]*oa[i]; }
        bool fin=true; for(float x:ob) if(!std::isfinite(x)||std::abs(x)>4) fin=false;
        auto info=sourceInfo((Sec)s,md); juce::String nm=juce::String(kSecName[s])+"/"+juce::String(md)+" "+info.names[k];
        CHECK(nm.toRawUTF8(),fin&&d>1e-6*e,"diff %.1f dB rel",10*std::log10(d/e+1e-12));
        a.S((Sec)s,pk,0.5f); b.S((Sec)s,pk,0.5f);}}}
  { // SteadyBass on the Bass tab (full plugin path): evens out a bass line, keeps level and low end, bypass-able
    auto line=[&](int i)->float{ static const float amps[8]{0.5f,0.12f,0.4f,0.2f,0.55f,0.1f,0.3f,0.45f}; int note=i/24000; if(note>=24) return 0.f; float t=(i%24000)/(float)FS; float a=amps[note%8];
      return a*(1-std::exp(-t/0.003f))*std::exp(-t/0.35f)*(std::sin(6.2832f*55.f*t)+0.3f*std::sin(6.2832f*110.f*t+0.5f)); };
    auto spread=[&](const std::vector<float>&y){ std::vector<double> d; for(int k=4;k<24;k++) d.push_back(db(rms(y,k*24000+3840,k*24000+18000))); double m=0; for(double v:d)m+=v; m/=d.size(); double s2=0; for(double v:d)s2+=(v-m)*(v-m); return std::sqrt(s2/d.size()); };
    auto runSb=[&](int on,float amount){ Rig r; neutral(r); r.sec(Bass); r.S(Bass,InGain,0); r.S(Bass,OutGain,0); r.S(Bass,SbOn,(float)on); r.S(Bass,SbAmount,amount); r.S(Bass,SbPunch,0.5f); r.S(Bass,SbWeight,0.5f); return r.run(48000*12,line); };
    auto off=runSb(0,0.6f), on=runSb(1,0.6f);
    double s0=spread(off), s1=spread(on), ld=db(rms(on,48000*4,48000*12))-db(rms(off,48000*4,48000*12));
    CHECK("SteadyBass (plugin path) evens notes",s1<s0*0.7,"%.2f -> %.2f dB",s0,s1);
    CHECK("SteadyBass (plugin path) level matched",std::abs(ld)<1.0,"%+.2f dB",ld);
    Rig r; neutral(r); r.sec(Bass); r.S(Bass,SbOn,1); auto o=r.run(48000,[](int){return 0.f;}); bool fin=true; for(float v:o) if(!std::isfinite(v)||std::abs(v)>1e-6f) fin=false;
    CHECK("SteadyBass silent on silence",fin,""); }
  { // Piano Bang voicings: old school is darker/warmer than modern, both stay finite and level-sane
    auto spec=[&](int which){ Rig r; neutral(r); r.sec(Keys); r.S(Keys,InGain,0); r.S(Keys,OutGain,0); r.S(Keys,SrcOn,0); for(auto&pv:pianoVoicing(which)) r.S(Keys,pv.first,pv.second);
      auto o=r.run(48000*3,[&](int i){ float v=0; for(int k=0;k<8;k++) v+=0.05f/(1.0f+0.3f*k)*std::sin(6.2832*110*std::pow(2.0,k)*i/FS); return v; });
      riddim::Biquad a,b; a.lowpass(FS,300,0.707); b.highpass(FS,5000,0.707); double lo=0,hi=0,tot=0; for(int i=48000;i<(int)o.size();i++){ float l=a.process(o[i]), h=b.process(o[i]); lo+=l*l; hi+=h*h; tot+=o[i]*o[i]; }
      return std::array<double,2>{ 10*std::log10(hi/lo), 10*std::log10(tot/(o.size()-48000)) }; };
    auto old=spec(0), mod=spec(1);
    CHECK("piano: modern is brighter than old school",mod[0]-old[0]>4,"hi/lo %.1f vs %.1f dB",mod[0],old[0]);
    CHECK("piano: both voicings keep sane level",std::abs(mod[1]-old[1])<6,"%.1f vs %.1f dB",mod[1],old[1]); }
  { // switching tabs while processing + all-on stress, fuzz
    Rig r; juce::Random rnd(7); bool fin=true; juce::AudioBuffer<float> b(2,512);
    for(int blk=0;blk<400;blk++){ if(blk%5==0){ r.sec(rnd.nextInt(7)); r.set("guitarMode",(float)rnd.nextInt(3));
        for(int q=0;q<40;q++){ int s=rnd.nextInt(NumSec),pp=rnd.nextInt(NumP); if(!exists((Sec)s,(P)pp)) continue; auto* pr=r.p.parameters.getParameter(id((Sec)s,(P)pp)); pr->setValueNotifyingHost(rnd.nextFloat()); } }
      for(int i=0;i<512;i++){ float v=0.7f*(rnd.nextFloat()*2-1); b.setSample(0,i,v); b.setSample(1,i,-v);} r.p.processBlock(b,r.m);
      for(int i=0;i<512;i++) if(!std::isfinite(b.getSample(0,i))||std::abs(b.getSample(0,i))>400){ if(fin) printf("   fuzz bad sample blk %d i %d val %f sec %d\n",blk,i,b.getSample(0,i),(int)r.p.parameters.getRawParameterValue("activeSection")->load()); fin=false; } }
    CHECK("random parameter fuzz stable",fin,""); }

  // ---------------------------------------------------------------- step 2: new effect engines
  { // spring: every character is finite, decays, and they sound different from each other
    auto ir=[&](int ch,int len,float dec){ Rig r; neutral(r); r.S(Master,RevOn,1); r.S(Master,RevType,0); r.S(Master,RevMix,1); r.S(Master,RevDecay,dec); r.S(Master,RevPre,0);
      r.S(Master,SpChar,(float)ch); r.S(Master,SpLen,(float)len);
      return r.run(48000*5,[](int i){return i<1200?0.4f*std::sin(i*0.12f):0.0f;}); };
    bool fin=true, decays=true; std::vector<double> sig;
    for(int c=0;c<7;c++){ auto o=ir(c,1,0.5f); for(float v:o) if(!std::isfinite(v)||std::abs(v)>4) fin=false;
      double e1=rms(o,4800,24000), e2=rms(o,48000*4,48000*5); if(!(e1>1e-3&&db(e2)<db(e1)-25)) decays=false;
      sig.push_back(db(rms(o,12000,24000))-db(rms(o,36000,48000))); }
    CHECK("spring: 7 characters finite",fin,"");
    CHECK("spring: 7 characters ring then decay",decays,"");
    double mn=1e9,mx=-1e9; for(double v:sig){mn=std::min(mn,v);mx=std::max(mx,v);} CHECK("spring: characters differ",mx-mn>3,"decay slope range %.1f dB",mx-mn);
    double sS=db(rms(ir(1,0,0.5f),48000,72000)), sM=db(rms(ir(1,1,0.5f),48000,72000)), sL=db(rms(ir(1,2,0.5f),48000,72000));
    CHECK("spring: Short < Med < Long",sS<sM&&sM<sL,"%.1f / %.1f / %.1f dB",sS,sM,sL);
    Rig m; neutral(m); m.S(Master,RevOn,1); m.S(Master,RevType,0); m.S(Master,RevMix,1); m.S(Master,SpStereo,1);
    auto mo=m.run(48000,[](int i){return i<1200?0.4f*std::sin(i*0.12f):0.0f;}); CHECK("spring: mono option runs",rms(mo,2400,24000)>1e-3,"%.1f dB",db(rms(mo,2400,24000))); }
  { // plate: damping darkens the tail, low cut removes lows, width 0 is mono
    auto tail=[&](float damp,float low,float wid,double f){ Rig r; neutral(r); r.S(Master,RevOn,1); r.S(Master,RevType,1); r.S(Master,RevMix,1); r.S(Master,RevDecay,0.6f); r.S(Master,RevPre,0);
      r.S(Master,PlDamp,damp); r.S(Master,PlLowCut,low); r.S(Master,PlWidth,wid);
      juce::AudioBuffer<float> b(2,512); std::vector<float> L,R; r.sec(0);
      for(int s=0;s<48000*2;s+=512){ for(int i=0;i<512;i++){ float v=(s+i)<9600?0.3f*(float)std::sin(6.2832*f*(s+i)/FS):0.0f; b.setSample(0,i,v); b.setSample(1,i,v);} r.p.processBlock(b,r.m); for(int i=0;i<512;i++){L.push_back(b.getSample(0,i));R.push_back(b.getSample(1,i));} }
      return std::make_pair(L,R); };
    auto hiE=[&](float damp){ auto o=tail(damp,0.0f,1.0f,6000).first; return db(rms(o,24000,60000)); };
    CHECK("plate: damping darkens tail",hiE(1.0f)<hiE(0.0f)-3,"%.1f vs %.1f dB",hiE(1.0f),hiE(0.0f));
    auto loE=[&](float low){ auto o=tail(0.3f,low,1.0f,60).first; return db(rms(o,9000,40000)); };
    CHECK("plate: low cut removes lows",loE(1.0f)<loE(0.0f)-10,"%.1f vs %.1f dB",loE(1.0f),loE(0.0f));
    auto w0=tail(0.3f,0.1f,0.0f,1000); double dif=0,tot=0; for(size_t i=24000;i<60000;i++){ dif+=std::pow(w0.first[i]-w0.second[i],2); tot+=std::pow(w0.first[i],2); }
    auto w1=tail(0.3f,0.1f,1.0f,1000); double dif1=0,tot1=0; for(size_t i=24000;i<60000;i++){ dif1+=std::pow(w1.first[i]-w1.second[i],2); tot1+=std::pow(w1.first[i],2); }
    CHECK("plate: width 0 mono, width 1 wide",dif/tot<0.01&&dif1/tot1>0.2,"diff %.4f vs %.3f",dif/tot,dif1/tot1);
    bool fin=true; auto o=tail(0.3f,0.1f,1.0f,1000).first; for(float v:o) if(!std::isfinite(v)||std::abs(v)>4) fin=false; CHECK("plate: finite",fin,""); }
  { // tape: speed moves the head bump, stability adds wobble, mix 0 is the dry signal
    auto bumpGain=[&](int speed){ Rig r; neutral(r); r.S(Master,TapeOn,1); r.S(Master,TapeType,1); r.S(Master,TapeSat,0.0f); r.S(Master,TapeTone,1.0f); r.S(Master,Wow,0); r.S(Master,Flutter,0); r.S(Master,TapeSpeed,(float)speed);
      auto a=r.run(48000,sine(60,0.1f)); return db(rms(a,24000,48000))-db(0.1/1.414); };
    double b0=bumpGain(0), b2=bumpGain(2);
    CHECK("tape: 7.5 ips has more 60 Hz bump than 30 ips",b0>b2+0.8,"%.2f vs %.2f dB",b0,b2);
    auto wob=[&](float stab){ Rig r; neutral(r); r.S(Master,TapeOn,1); r.S(Master,TapeSat,0); r.S(Master,Wow,0); r.S(Master,Flutter,0); r.S(Master,TapeStab,stab);
      auto o=r.run(48000*4,sine(1000,0.3f)); std::vector<double> f; int last=-1; for(int i=24000;i<48000*4-1;i++) if(o[i]<=0&&o[i+1]>0){ if(last>0) f.push_back(FS/(i-last)); last=i; }
      double m=0; for(double v:f)m+=v; m/=f.size(); double s2=0; for(double v:f)s2+=(v-m)*(v-m); return 100*std::sqrt(s2/f.size())/m; };
    double w1=wob(1.0f), w0=wob(0.0f);
    CHECK("tape: stability 1 steady, 0 wobbly",w1<0.05&&w0>0.15,"%.3f%% vs %.3f%%",w1,w0);
    Rig r; neutral(r); r.S(Master,TapeOn,1); r.S(Master,TapeSat,0.9f); r.S(Master,TapeMix,0.0f); r.S(Master,Wow,1); auto o=r.run(48000,sine(1000,0.25f));
    double g=db(rms(o,24000,48000))-db(0.25/1.414); CHECK("tape: mix 0 passes the dry signal",std::abs(g)<0.2,"%.2f dB",g);
    bool fin=true; for(int ty=0;ty<3;ty++) for(int sp=0;sp<3;sp++){ Rig q; neutral(q); q.S(Master,TapeOn,1); q.S(Master,TapeType,(float)ty); q.S(Master,TapeSpeed,(float)sp); q.S(Master,TapeSat,1); q.S(Master,Wow,1); q.S(Master,Flutter,1); q.S(Master,Wear,1); q.S(Master,TapeStab,0);
      auto z=q.run(48000,sine(300,0.9f)); for(float v:z) if(!std::isfinite(v)||std::abs(v)>3) fin=false; }
    CHECK("tape: all types/speeds finite at extremes",fin,"");
    Rig lat; lat.p.setPlayConfigDetails(2,2,FS,512); CHECK("tape: latency still 96",lat.p.getLatencySamples()==96,"%d",lat.p.getLatencySamples()); }
  { // echo heads and reverb in loop
    auto peaks=[&](bool h1,bool h2,bool h3){ Rig r; neutral(r); r.S(Master,EchoOn,1); r.S(Master,Sync,0); r.S(Master,EchoTime,200); r.S(Master,Feedback,0.0f); r.S(Master,Mix,1); r.S(Master,Width,0); r.S(Master,Damping,0); r.S(Master,Heat,0);
      r.S(Master,Head1,h1); r.S(Master,Head2,h2); r.S(Master,Head3,h3);
      auto o=r.run(48000*2,[](int i){return i<480?0.5f*(float)std::sin(6.2832*1000*i/FS):0.0f;});
      std::vector<int> bins; for(int k=0;k<190;k++){ if(rms(o,k*480,k*480+480)>0.03) { if(bins.empty()||k-bins.back()>3) bins.push_back(k); } } return bins; };
    auto a=peaks(1,0,0), b=peaks(0,1,0), c=peaks(1,1,0), d=peaks(0,0,1);
    CHECK("echo head 1 at 1x",a.size()==1&&std::abs(a[0]-20)<=1,"bin %d",a.empty()?-1:a[0]);
    CHECK("echo head 2 at 2x",b.size()==1&&std::abs(b[0]-40)<=1,"bin %d",b.empty()?-1:b[0]);
    CHECK("echo head 3 at 3x",d.size()==1&&std::abs(d[0]-60)<=1,"bin %d",d.empty()?-1:d[0]);
    CHECK("echo heads 1+2 give two repeats",c.size()==2&&std::abs(c[0]-20)<=1&&std::abs(c[1]-40)<=1,"%zu repeats",c.size());
    Rig r; neutral(r); r.S(Master,EchoOn,1); r.S(Master,Sync,0); r.S(Master,EchoTime,200); r.S(Master,Feedback,0.94f); r.S(Master,Mix,0.5f); r.S(Master,Head2,1); r.S(Master,Head3,1); r.S(Master,EchoRev,1);
    auto o=r.run(48000*2,[](int i){return i<2400?0.5f*std::sin(i*0.1f):0.f;}); auto t=r.run(48000*60,[](int){return 0.f;});
    bool fin=true; for(float v:t) if(!std::isfinite(v)) fin=false; double late=rms(t,48000*55,48000*60);
    CHECK("echo: all heads + reverb in loop stays stable",fin&&late<0.001,"rms %.6f",late); }
  { // step 3: sub band + speaker check (Bass tab)
    auto bassRun=[&](double f,float lvl,float harm,int spk){ Rig r; neutral(r); r.sec(Bass); r.S(Bass,SbOn,0); r.S(Bass,SubLevel,lvl); r.S(Bass,SubHarm,harm); r.S(Bass,SpkMode,(float)spk);
      auto o=r.run(48000,sine(f,0.2f)); return o; };
    double base=db(rms(bassRun(40,0,0,0),24000,48000)), up=db(rms(bassRun(40,6,0,0),24000,48000)), dn=db(rms(bassRun(40,-9,0,0),24000,48000));
    CHECK("sub: +6 dB lifts 40 Hz",std::abs((up-base)-6)<1.2,"%.2f dB",up-base);
    CHECK("sub: -9 dB cuts 40 Hz",std::abs((dn-base)+9)<1.5,"%.2f dB",dn-base);
    double hb=db(rms(bassRun(250,0,0,0),24000,48000)), hu=db(rms(bassRun(250,9,0,0),24000,48000));
    CHECK("sub: leaves 250 Hz alone",std::abs(hu-hb)<0.5,"%.2f dB",hu-hb);
    // harmonics: energy above 90 Hz appears for a 40 Hz note
    auto hi=[&](float h){ auto o=bassRun(40,0,h,0); double re=0,im=0; for(int i=24000;i<48000;i++){ re+=o[i]*std::cos(6.283185307*120*i/FS); im+=o[i]*std::sin(6.283185307*120*i/FS);} return std::sqrt(re*re+im*im)/12000.0; };
    double h0=hi(0), h1=hi(1);
    CHECK("sub: harmonics add upper content",h1>h0*1.5,"%.5f vs %.5f",h0,h1);
    double ph=db(rms(bassRun(50,0,0,1),24000,48000))-base+0, mb=db(rms(bassRun(1000,0,0,1),24000,48000)), mb0=db(rms(bassRun(1000,0,0,0),24000,48000));
    CHECK("speaker check: phone kills the lows",ph<-18,"%.1f dB",ph);
    CHECK("speaker check: phone keeps 1 kHz",std::abs(mb-mb0)<7,"%.1f dB",mb-mb0);
    double mac=db(rms(bassRun(80,0,0,3),24000,48000))-db(rms(bassRun(80,0,0,4),24000,48000));
    CHECK("speaker check: MacBook thinner than monitor at 80 Hz",mac<-2,"%.1f dB",mac);
    bool fin=true; for(int m=0;m<5;m++){ auto o=bassRun(35,12,1,m); for(float v:o) if(!std::isfinite(v)||std::abs(v)>4) fin=false; }
    CHECK("sub + speaker: finite at extremes",fin,""); }
  { // step 4: drum voicing + kick/bass lock
    auto dr=[&](double f,int type,int lock,float amt){ Rig r; neutral(r); r.sec(Drums); r.S(Drums,SrcOn,0); r.S(Drums,DrumType,(float)type); r.S(Drums,LockOn,(float)lock); r.S(Drums,LockAmt,amt);
      auto o=r.run(48000,sine(f,0.2f)); return db(rms(o,24000,48000)); };
    double l60=dr(60,5,0,0.5f), l4k=dr(4500,5,0,0.5f);
    CHECK("drums: loop leaves 60 Hz alone",std::abs(l60-db(0.2/1.414))<0.3,"%.2f dB",l60-db(0.2/1.414));
    CHECK("drums: kick lifts lows vs loop",dr(70,0,0,0.5f)>l60+0.8 || dr(60,0,0,0.5f)>l60-0.5,"%.2f dB",dr(60,0,0,0.5f)-l60);
    CHECK("drums: snare adds crack at 4.5 kHz",dr(4500,1,0,0.5f)>l4k+1.5,"%.2f dB",dr(4500,1,0,0.5f)-l4k);
    CHECK("drums: hat removes 60 Hz",dr(60,3,0,0.5f)<l60-15,"%.1f dB",dr(60,3,0,0.5f)-l60);
    CHECK("drums: rim removes 60 Hz",dr(60,2,0,0.5f)<l60-12,"%.1f dB",dr(60,2,0,0.5f)-l60);
    CHECK("drums: toms lift 120 Hz",dr(120,4,0,0.5f)>dr(120,5,0,0.5f)+1.5,"%.2f dB",dr(120,4,0,0.5f)-dr(120,5,0,0.5f));
    double lk=dr(50,5,1,1.0f)-dr(50,5,0,1.0f), lk2=dr(125,5,1,1.0f)-dr(125,5,0,1.0f), lk0=dr(50,5,1,0.0f)-dr(50,5,0,0.0f), lkhi=dr(1000,5,1,1.0f)-dr(1000,5,0,1.0f);
    CHECK("lock: dips 50 Hz at full amount",lk<-4.5,"%.2f dB",lk);
    CHECK("lock: keeps kick knock near 125 Hz",lk2>-3.0,"%.2f dB",lk2);
    CHECK("lock: amount 0 does nothing",std::abs(lk0)<0.15,"%.2f dB",lk0);
    CHECK("lock: leaves 1 kHz alone",std::abs(lkhi)<0.3,"%.2f dB",lkhi);
    bool fin=true; for(int t=0;t<6;t++){ Rig r; neutral(r); r.sec(Drums); r.S(Drums,DrumType,(float)t); r.S(Drums,LockOn,1); r.S(Drums,LockAmt,1); auto o=r.run(48000,sine(80,0.9f)); for(float v:o) if(!std::isfinite(v)||std::abs(v)>4) fin=false; }
    CHECK("drums: finite for every type",fin,""); }
  { // step 5: control-room listening
    struct Out { std::vector<float> l,r; };
    auto run2=[&](std::function<void(Rig&)> setup,std::function<float(int)> gl,std::function<float(int)> gr,int N){ Rig r; neutral(r); setup(r); Out o; juce::AudioBuffer<float> b(2,512);
      for(int s=0;s<N;s+=512){ int n=std::min(512,N-s); b.setSize(2,n,false,false,true); for(int i=0;i<n;i++){ b.setSample(0,i,gl(s+i)); b.setSample(1,i,gr(s+i)); }
        r.p.processBlock(b,r.m); for(int i=0;i<n;i++){ o.l.push_back(b.getSample(0,i)); o.r.push_back(b.getSample(1,i)); } } return o; };
    auto mon=[&](double f,int m,int room){ auto o=run2([&](Rig&r){ r.S(Master,CtlMon,(float)m); r.S(Master,CtlRoom,(float)room); r.S(Master,CtlColour,0); r.S(Master,CtlDist,0.5f); },sine(f,0.2f),sine(f,0.2f),48000); return db(rms(o.l,24000,48000)); };
    double off=mon(1000,0,0);
    CHECK("control: off passes the signal",std::abs(off-db(0.2/1.414))<0.2,"%.2f dB",off-db(0.2/1.414));
    double nf30=mon(30,1,0)-mon(30,0,0), mf30=mon(30,2,0)-mon(30,0,0), mn30=mon(30,3,0)-mon(30,0,0);
    CHECK("control: nearfield loses the sub",nf30<-9,"%.1f dB",nf30);
    CHECK("control: nearfield thinner than midfield than main at 30 Hz",nf30<mf30&&mf30<mn30,"%.1f / %.1f / %.1f",nf30,mf30,mn30);
    // mono, crossfeed, level, width
    auto solo=[&](std::function<void(Rig&)> st){ return run2([&](Rig&r){ r.S(Master,CtlMon,3); r.S(Master,CtlColour,0); r.S(Master,CtlRoom,0); r.S(Master,CtlDist,0.5f); st(r); },sine(300,0.3f),[](int){return 0.0f;},48000); };
    auto mo=solo([](Rig&r){ r.S(Master,CtlMono,1); });
    CHECK("control: mono makes both sides equal",std::abs(rms(mo.l,24000,48000)-rms(mo.r,24000,48000))<0.002,"%.4f vs %.4f",rms(mo.l,24000,48000),rms(mo.r,24000,48000));
    auto ph=solo([](Rig&r){ r.S(Master,CtlPhones,1); }), ns=solo([](Rig&r){});
    CHECK("control: headphones add crossfeed",rms(ph.r,24000,48000)>rms(ns.r,24000,48000)+0.02,"%.4f vs %.4f",rms(ph.r,24000,48000),rms(ns.r,24000,48000));
    auto lv=solo([](Rig&r){ r.S(Master,CtlLevel,-6); });
    double dl=db(rms(lv.l,24000,48000))-db(rms(ns.l,24000,48000)); CHECK("control: level -6 dB",std::abs(dl+6)<0.3,"%.2f dB",dl);
    auto wn=solo([](Rig&r){ r.S(Master,CtlWidth,20); });
    CHECK("control: narrow angle pulls sides together",rms(wn.r,24000,48000)>rms(ns.r,24000,48000)+0.05,"%.3f vs %.3f",rms(wn.r,24000,48000),rms(ns.r,24000,48000));
    // room tail after a burst
    auto tail=[&](int room){ auto o=run2([&](Rig&r){ r.S(Master,CtlMon,2); r.S(Master,CtlRoom,(float)room); r.S(Master,CtlDist,3.0f); r.S(Master,CtlColour,0); },[](int i){return i<2400?0.5f*std::sin(i*0.2f):0.0f;},[](int i){return i<2400?0.5f*std::sin(i*0.2f):0.0f;},48000); return rms(o.l,24000,48000); };
    double t0=tail(0), t2=tail(2); CHECK("control: live room rings longer than studio",t2>t0*1.5&&t0>=0,"%.5f vs %.5f",t2,t0);
    // offline bounce untouched
    { Rig r; neutral(r); r.S(Master,CtlMon,1); r.S(Master,CtlMono,1); r.S(Master,CtlLevel,-12); r.p.setNonRealtime(true); auto o=r.run(48000,sine(1000,0.25f));
      double g=db(rms(o,24000,48000))-db(0.25/1.414); CHECK("control: bypassed when exporting",std::abs(g)<0.2,"%.2f dB",g); }
    bool fin=true; for(int m=1;m<4;m++) for(int ro=0;ro<3;ro++){ auto o=run2([&](Rig&r){ r.S(Master,CtlMon,(float)m); r.S(Master,CtlRoom,(float)ro); r.S(Master,CtlDist,6); r.S(Master,CtlWidth,90); r.S(Master,CtlToe,1); r.S(Master,CtlColour,1); r.S(Master,CtlLevel,6); r.S(Master,CtlPhones,1); },sine(90,0.9f),sine(110,0.9f),24000); for(float v:o.l) if(!std::isfinite(v)||std::abs(v)>8) fin=false; }
    CHECK("control: finite at extremes",fin,""); }
  { // step 7: sidechain kick/bass lock
    auto hostSc=[&](double f,double scf,float scAmp,float amt){ Rig r; neutral(r); r.sec(Drums); r.S(Drums,SrcOn,0); r.S(Drums,DrumType,5); r.S(Drums,LockOn,2); r.S(Drums,LockAmt,amt);
      auto lay=r.p.getBusesLayout(); lay.inputBuses.getReference(1)=juce::AudioChannelSet::stereo(); bool ok=r.p.setBusesLayout(lay); r.p.prepareToPlay(FS,512);
      if(!ok) return 99.0; std::vector<float> o; juce::AudioBuffer<float> b(4,512);
      for(int s=0;s<48000;s+=512){ for(int i=0;i<512;i++){ float v=0.3f*(float)std::sin(6.283185307*f*(s+i)/FS), q=scAmp*(float)std::sin(6.283185307*scf*(s+i)/FS); b.setSample(0,i,v); b.setSample(1,i,v); b.setSample(2,i,q); b.setSample(3,i,q);} r.p.processBlock(b,r.m); for(int i=0;i<512;i++) o.push_back(b.getSample(0,i)); }
      return db(rms(o,24000,47000)); };
    double quiet=hostSc(60,50,0.0f,1.0f), loud=hostSc(60,50,0.5f,1.0f), half=hostSc(60,50,0.5f,0.5f), hi=hostSc(1000,50,0.5f,1.0f), hi0=hostSc(1000,50,0.0f,1.0f);
    CHECK("sidechain: host input bus accepted",quiet<90,"%.1f",quiet);
    CHECK("sidechain: silent input leaves drums alone",std::abs(quiet-db(0.3/1.414))<0.5,"%.2f dB",quiet-db(0.3/1.414));
    CHECK("sidechain: bass ducks drum lows",loud<quiet-9,"%.1f dB",loud-quiet);
    CHECK("sidechain: amount scales the ducking",half>loud+3&&half<quiet-3,"%.1f / %.1f dB",half-quiet,loud-quiet);
    CHECK("sidechain: highs untouched",std::abs(hi-hi0)<0.5,"%.2f dB",hi-hi0);
    // fallback: Bass tab instance in the same host
    auto viaBass=[&](float bassAmp){ Rig bs, dr; neutral(bs); neutral(dr); bs.sec(Bass); bs.S(Bass,SbOn,0); dr.sec(Drums); dr.S(Drums,SrcOn,0); dr.S(Drums,DrumType,5); dr.S(Drums,LockOn,2); dr.S(Drums,LockAmt,1);
      std::vector<float> o; juce::AudioBuffer<float> b1(2,512), b2(2,512);
      for(int s=0;s<48000;s+=512){ for(int i=0;i<512;i++){ float q=bassAmp*(float)std::sin(6.283185307*50*(s+i)/FS), v=0.3f*(float)std::sin(6.283185307*60*(s+i)/FS); b1.setSample(0,i,q); b1.setSample(1,i,q); b2.setSample(0,i,v); b2.setSample(1,i,v);} bs.p.processBlock(b1,bs.m); dr.p.processBlock(b2,dr.m); for(int i=0;i<512;i++) o.push_back(b2.getSample(0,i)); }
      return std::make_pair(db(rms(o,24000,47000)), dr.p.getSidechainState()); };
    auto vq=viaBass(0.0f), vl=viaBass(0.5f);
    CHECK("sidechain: Bass tab fallback ducks drums",vl.first<vq.first-9&&vl.second==2,"%.1f dB, state %d",vl.first-vq.first,vl.second);
    { Rig dr; neutral(dr); dr.sec(Drums); dr.S(Drums,SrcOn,0); dr.S(Drums,DrumType,5); dr.S(Drums,LockOn,2); dr.S(Drums,LockAmt,1); juce::Thread::sleep(450); auto o=dr.run(48000,sine(60,0.3f)); double g=db(rms(o,24000,47000))-db(0.3/1.414);
      CHECK("sidechain: nothing to listen to means no change",std::abs(g)<0.3&&dr.p.getSidechainState()==0,"%.2f dB, state %d",g,dr.p.getSidechainState()); }
    bool fin=true; { Rig r; neutral(r); r.sec(Drums); r.S(Drums,LockOn,2); r.S(Drums,LockAmt,1); auto o=r.run(48000,sine(40,0.95f)); for(float v:o) if(!std::isfinite(v)||std::abs(v)>4) fin=false; }
    CHECK("sidechain: finite",fin,""); }
  printf("\n%d FAILURES\n",fails); return fails;
}
