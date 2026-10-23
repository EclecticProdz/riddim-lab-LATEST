#include "RiddimDSP.h"
#include <cstdio>
using namespace riddim;
static const double FS=48000;
static int fails=0;
#define CHECK(name,cond,...) do{ bool okk=(cond); printf("%s %-46s ",okk?"PASS":"FAIL",name); printf(__VA_ARGS__); printf("\n"); if(!okk) fails++; }while(0)
static double db(double x){return 20*std::log10(std::max(x,1e-9));}
struct Bass{ std::vector<float> x; std::vector<int> onsets; };
// plucked reggae-ish bass line: fundamental + 2nd/3rd harmonic, 3 ms attack, 0.35 s decay, uneven note levels
static Bass makeBass(float scale,int seed,float h2=0.35f,float h3=0.18f){
  Bass b; int N=(int)(FS*14); b.x.assign(N,0); juce::Random rnd(seed);
  const float f0[]{41.2f,49.0f,55.0f,61.7f,73.4f,82.4f,36.7f,65.4f};
  for(int k=0;k<26;k++){ int on=(int)(FS*(0.3+k*0.5)); b.onsets.push_back(on);
    float f=f0[rnd.nextInt(8)]; float amp=scale*(0.08f+0.55f*rnd.nextFloat()); float tau=0.28f+0.2f*rnd.nextFloat();
    for(int i=0;i<(int)(FS*0.55)&&on+i<N;i++){ float t=i/(float)FS; float env=(1-std::exp(-t/0.003f))*std::exp(-t/tau);
      b.x[on+i]+=amp*env*(std::sin(6.2832f*f*t)+h2*std::sin(6.2832f*2*f*t+0.5f)+h3*std::sin(6.2832f*3*f*t+1.1f)); } }
  return b; }
static std::vector<float> run(const std::vector<float>& in,const SteadyBass::Params& p,float* maxGr=nullptr){
  SteadyBass s; s.prepare(FS); std::vector<float> L=in,R=in; /*dbg*/ float g=0;
  for(size_t i=0;i<in.size();i+=512){ int n=(int)std::min<size_t>(512,in.size()-i); g=std::max(g,s.process(&L[i],&R[i],n,p)); }
  if(maxGr)*maxGr=g; return L; }
static double rms(const std::vector<float>&v,int a,int b){double s=0;for(int i=a;i<b;i++)s+=v[i]*v[i];return std::sqrt(s/std::max(1,b-a));}
static std::vector<float> lowpass(const std::vector<float>&v,double fc){ Biquad a,b; a.lowpass(FS,fc,0.707); b.lowpass(FS,fc,0.707); std::vector<float> o(v.size()); for(size_t i=0;i<v.size();i++) o[i]=b.process(a.process(v[i])); return o; }
// per-note body level (dB) 80..380 ms after onset; and std dev
static double noteSpread(const std::vector<float>&y,const Bass& ref,double* meanOut=nullptr){ std::vector<double> d; for(size_t k=3;k<ref.onsets.size();k++){ int on=ref.onsets[k]; d.push_back(db(rms(y,on+(int)(0.08*FS),on+(int)(0.38*FS)))); }
  double m=0; for(double v:d)m+=v; m/=d.size(); double s=0; for(double v:d)s+=(v-m)*(v-m); if(meanOut)*meanOut=m; return std::sqrt(s/d.size()); }
static double onsetPeak(const std::vector<float>&y,const Bass&ref){ double acc=0; int c=0; for(size_t k=3;k<ref.onsets.size();k++){ int on=ref.onsets[k]; float pk=0; for(int i=0;i<(int)(0.03*FS);i++) pk=std::max(pk,std::abs(y[on+i])); double body=rms(y,on+(int)(0.08*FS),on+(int)(0.38*FS)); acc+=db(pk)-db(body); c++; } return acc/c; }
// low-band onset peak, out vs in (dB), averaged over the louder half of the notes (where gain reduction bites)
static double loudThump(const std::vector<float>&y,const std::vector<float>&x,const Bass& ref){ std::vector<std::pair<double,double>> v; for(size_t k=3;k<ref.onsets.size();k++){ int on=ref.onsets[k]; float po=0,pi=0; for(int i=0;i<(int)(0.03*FS);i++){ po=std::max(po,std::abs(y[on+i])); pi=std::max(pi,std::abs(x[on+i])); } v.push_back({db(pi),db(po)-db(pi)}); }
  std::sort(v.begin(),v.end()); double acc=0; int c=0; for(size_t i=v.size()/2;i<v.size();i++){acc+=v[i].second;c++;} return acc/c; }
int main(){
  juce::ScopedJuceInitialiser_GUI gui;
  for(float scale:{1.0f,0.1f,1.6f}){
    auto b=makeBass(scale,5); SteadyBass::Params p; p.amount=0.6f; p.punch=0.5f; p.weight=0.5f;
    auto y=run(b.x,p); int a=(int)(FS*3),e=(int)(FS*13);
    printf("-- input scale %.1f (peak %.1f dBFS)\n",scale,db(*std::max_element(b.x.begin(),b.x.end())));
    double m0,m1; double s0=noteSpread(b.x,b,&m0), s1=noteSpread(y,b,&m1);
    if(scale==1.0f){ for(size_t k=3;k<12;k++){int on=b.onsets[k]; printf("   note %zu in %.1f out %.1f dB\n",k,db(rms(b.x,on+3840,on+18240)),db(rms(y,on+3840,on+18240)));} }
    CHECK("evens out note levels (spread dB)",s1<s0*0.65,"%.2f -> %.2f dB",s0,s1);
    double loud=db(rms(y,a,e))-db(rms(b.x,a,e));
    CHECK("matched loudness (long-term RMS)",std::abs(loud)<1.0,"%+.2f dB",loud);
    auto lb=lowpass(b.x,110), ly=lowpass(y,110);
    double lowd=db(rms(ly,a,e))-db(rms(lb,a,e));
    CHECK("low-end (<110 Hz) weight kept at matched level",lowd>-0.8,"%+.2f dB vs bypass",lowd);
    double ratio=(db(rms(ly,a,e))-db(rms(y,a,e)))-(db(rms(lb,a,e))-db(rms(b.x,a,e)));
    CHECK("low/total balance not thinner",ratio>-0.5,"%+.2f dB",ratio);
    double c0=onsetPeak(b.x,b),c1=onsetPeak(y,b);
    CHECK("note attack kept (onset peak vs body)",c1>c0-1.5,"%.2f -> %.2f dB",c0,c1);
    float pkOut=0; for(float v:y)pkOut=std::max(pkOut,std::abs(v)); float pkIn=0; for(float v:b.x)pkIn=std::max(pkIn,std::abs(v));
    CHECK("no added peaks / clipping",db(pkOut)<=db(pkIn)+4.0,"peak %+.1f dB vs input",db(pkOut)-db(pkIn));
  }
  { // comparison: ordinary compressor at matched loudness
    auto b=makeBass(1.0f,5); SteadyBass::Params p; p.amount=0.6f; p.punch=0.5f; p.weight=0.5f; auto y=run(b.x,p);
    std::vector<float> L=b.x,R=b.x; Compressor c; c.prepare(FS); Compressor::Params cp; cp.threshDb=-24; cp.ratio=4; cp.mode=1;
    for(size_t i=0;i<L.size();i+=512){ int n=(int)std::min<size_t>(512,L.size()-i); c.process(&L[i],&R[i],n,cp); }
    int a=(int)(FS*3),e=(int)(FS*13); double gm=db(rms(b.x,a,e))-db(rms(L,a,e)); for(auto&v:L) v*=std::pow(10.0,gm/20);
    auto ll=lowpass(L,110), ly=lowpass(y,110), lb=lowpass(b.x,110);
    printf("   reference: ordinary 4:1 'Punch' compressor, loudness-matched: low-band %+.2f dB, spread %.2f dB, onset %.2f dB\n",db(rms(ll,a,e))-db(rms(lb,a,e)),noteSpread(L,b),onsetPeak(L,b));
    printf("   SteadyBass:                                                   low-band %+.2f dB, spread %.2f dB, onset %.2f dB\n",db(rms(ly,a,e))-db(rms(lb,a,e)),noteSpread(y,b),onsetPeak(y,b)); }
  { // distortion on a steady 50 Hz sine
    std::vector<float> x((size_t)FS*4); for(size_t i=0;i<x.size();i++) x[i]=0.4f*std::sin(6.2832f*50*i/FS);
    SteadyBass::Params p; p.amount=1; auto y=run(x,p);
    double re=0,im=0,re1=0,im1=0; int a=(int)(FS*2); for(int i=a;i<(int)y.size();i++){ re+=y[i]*std::cos(6.2832*150*i/FS); im+=y[i]*std::sin(6.2832*150*i/FS); re1+=y[i]*std::cos(6.2832*50*i/FS); im1+=y[i]*std::sin(6.2832*50*i/FS);} 
    CHECK("steady 50 Hz: 3rd harmonic",db(std::hypot(re,im)/std::hypot(re1,im1))<-50,"%.1f dBc",db(std::hypot(re,im)/std::hypot(re1,im1))); }
  { // amount 0 is transparent, silence safe, no latency (impulse stays put)
    auto b=makeBass(1.0f,9); SteadyBass::Params p; p.amount=0; auto y=run(b.x,p); double d=0; for(size_t i=0;i<y.size();i++) d=std::max(d,(double)std::abs(y[i]-b.x[i]));
    CHECK("amount 0 ~ transparent",d<0.02,"max diff %.4f",d);
    std::vector<float> z((size_t)FS,0.f); auto yz=run(z,p); bool ok=true; int nbad=0; for(float v:yz) if(!std::isfinite(v)||std::abs(v)>1e-6f) {ok=false;nbad++;} printf("   bad %d first %g\n",nbad,yz[10]); float mx=0; for(float v:yz) mx=std::max(mx,std::abs(v)); CHECK("silence stays silent",ok,"max %g",mx);
    std::vector<float> im((size_t)4800,0.f); im[100]=0.5f; SteadyBass::Params q; q.amount=1; auto yi=run(im,q); int pk=0; for(int i=0;i<(int)yi.size();i++) if(std::abs(yi[i])>std::abs(yi[pk])) pk=i; CHECK("zero latency (impulse not delayed)",pk==100,"peak at %d",pk); }
  { // amount control: more amount -> more even
    auto b=makeBass(1.0f,5); double prev=1e9; bool mono=true; for(float am:{0.0f,0.3f,0.6f,1.0f}){ SteadyBass::Params p; p.amount=am; auto y=run(b.x,p); double s=noteSpread(y,b); if(s>prev+0.05) mono=false; prev=s; printf("   amount %.1f -> spread %.2f dB\n",am,s);} CHECK("amount knob monotonic",mono,""); }
  { // weight control, on a brighter bass (strong 2nd/3rd harmonics): more weight keeps the fundamental band fuller
    auto b=makeBass(1.0f,5,1.1f,0.9f); int a=(int)(FS*3),e=(int)(FS*13); auto lb=lowpass(b.x,110); double bal[2],sp[2],lo[2]; int k=0;
    for(float w:{0.0f,1.0f}){ SteadyBass::Params p; p.amount=0.8f; p.weight=w; auto y=run(b.x,p); auto ly=lowpass(y,110);
      bal[k]=(db(rms(ly,a,e))-db(rms(y,a,e)))-(db(rms(lb,a,e))-db(rms(b.x,a,e))); lo[k]=db(rms(ly,a,e))-db(rms(lb,a,e)); sp[k]=noteSpread(y,b); k++; }
    printf("   weight 0 -> 1: low-band balance %+.2f -> %+.2f dB, low-band level %+.2f -> %+.2f dB, note spread %.2f -> %.2f dB (input %.2f)\n",bal[0],bal[1],lo[0],lo[1],sp[0],sp[1],noteSpread(b.x,b));
    CHECK("weight knob keeps the fundamental band fuller",bal[1]>bal[0]+0.03&&lo[1]>lo[0]+0.03,"balance %+.2f -> %+.2f dB",bal[0],bal[1]);
    CHECK("weight knob keeps leveling effective",sp[1]<noteSpread(b.x,b)*0.8,"%.2f dB",sp[1]); }
  printf("\n%d FAILURES\n",fails); return fails; }
