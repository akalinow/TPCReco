#if !defined(_LINE_H_)
#define _LINE_H_
#include <TRandom3.h>
#include <TVector3.h>
#include <vector>
//Helper class respresenting single line
class Line {
public:
    Line()=default;
  // Line():rng(nullptr),randomLength(false),randomTheta(false), randomPhi(false){}
    Line(TRandom3* rng, TVector3 stop=TVector3(0,0,1), TVector3 start=TVector3(0,0,0)):
    rng(rng), start(start), stop(stop), track(stop-start), 
    randomLength(false),randomTheta(false), randomPhi(false)
    {}
    Line(TRandom3* rng, double length, double theta, double phi, TVector3 start=TVector3(0,0,0)):
    rng(rng),start(start),stop(0,0,1),
    randomLength(false),randomTheta(false), randomPhi(false)
    {
        stop.SetMagThetaPhi(length,theta,phi);
        track=stop-start;
    }
    //If radnomtheta, -phi, -length are true, then generates new random values 
    void update(){
        if(randomLength){
            setLength(rng->Uniform(track.Mag()));
        }
        if(randomTheta){
            auto cos=rng->Uniform(-1,1);
            setTheta(TMath::ACos(cos));
        }
        if(randomPhi){
            setPhi(rng->Uniform(2*TMath::Pi()));
        }
    }    

    //Setters:

    inline void setRandom(bool theta=false,bool phi=false,bool length=false){
        randomTheta=theta;randomPhi=phi,randomLength=length; if(randomLength) randomLengthMax=track.Mag();
    }
    inline void setStart(const TVector3& s){start=s;track=stop-start;} //mm
    //  inline void setStart(TVector3 &&s){start=s;track=stop-start;}
    inline void setStart(double x, double y, double z){setStart(TVector3(x,y,z));} //mm
    inline void setStop(const TVector3& s){stop=s;track=stop-start;} //mm
    //  inline void setStop(TVector3&& s){stop=s;track=stop-start;}
    inline void setStop(double x,double y, double z){setStop(TVector3(x,y,z));} //mm
    inline void setLength(double l){track.SetMag(l),stop=start+track;} //mm
    inline void setTheta(double theta){track.SetTheta(theta),stop=start+track;} //rad
    inline void setPhi(double phi){track.SetPhi(phi),stop=start+track;} //rad
    inline void setThetaPhi(double theta, double phi){track.SetTheta(theta),track.SetPhi(phi),stop=start+track;} //rad
    //Getters:

    inline TVector3 getStart(){return start;} //mm
    inline TVector3 getStop(){return stop;} //mm
    inline double getLength(){return track.Mag();} //mm
    inline double getTheta(){return track.Theta();} //rad
    inline double getPhi(){return track.Phi();} //rad
    inline TVector3 GetSample(){return start+rng->Uniform()*track;} //mm
private:

    TRandom3 *rng;
    TVector3 start;
    TVector3 stop;
    TVector3 track;

    //double sigma;
    bool randomLength;
    bool randomTheta;
    bool randomPhi;
    double randomLengthMax{0};

};


#endif // _LINE_H_
