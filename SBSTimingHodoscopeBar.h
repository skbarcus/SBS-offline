/* #ifndef ROOT_SBSTimingHodoscopeBar */
/* #define ROOT_SBSTimingHodoscopeBar */
#ifndef SBSTimingHodoscopeBar_h
#define SBSTimingHodoscopeBar_h


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// SBSTimingHodoscopeBar                                                     //
//                                                                           //
// Class to represent a timing hodoscope bar                                 //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

/* class SBSTimingHodoscopeBar; */
//class SBSTimingHodoscopePMT;

#include "TObject.h"
#include "TRef.h"
#include "SBSTimingHodoscopePMT.h"
#include "SBSData.h"

//380cm light attenuation length from data sheet for EJ200 (but doesn't say for what energy)

class SBSTimingHodoscopeBar : public TObject {
public: 
	SBSTimingHodoscopeBar(
			      Int_t barnum,
			      SBSTimingHodoscopePMT* leftpmt,
			      SBSTimingHodoscopePMT* rightpmt,
			      Int_t baroff);

	virtual ~SBSTimingHodoscopeBar();

	Int_t                  GetBarNum() const {return fBarNum;}
	SBSTimingHodoscopePMT* GetLPMT() { return fLPMT; }
	SBSTimingHodoscopePMT* GetRPMT() { return fRPMT; }
	Int_t                  GerBarOff() const {return fBarOff;}

	void SetBarNum( Int_t barnum ) {fBarNum=barnum;}
	void SetBarOff( Int_t baroff ) {fBarOff=baroff;}
	/* void SetLPMT( SBSTimingHodoscopePMT* leftpmt) {fLPMT=leftpmt;} */
	/* void SetRPMT( SBSTimingHodoscopePMT* rightpmt) {fRPMT=rightpmt;} */
	void ClearEvent();
	
	double GetMeanTime(){return fMeanTime;};
	double GetTimeDiff(){return fTimeDiff;};
	double GetMeanToT(){return fMeanToT;};
	double GetHitPos(){return fHitPos;};
	double GetElementPos(){return fElementPos;};
	SBSData::TDCHit GetLeftHit(){return fLeftHit;};
	SBSData::TDCHit GetRightHit(){return fRightHit;};

	void SetMeanTime(double val){fMeanTime = val;};
	void SetTimeDiff(double val){fTimeDiff = val;};
	void SetMeanToT(double val){fMeanToT = val;};
	void SetHitPos(double val){fHitPos = val;};
	void SetElementPos(double val){fElementPos = val;};
	void SetLeftHit(SBSData::TDCHit hit){fLeftHit = hit;};
	void SetRightHit(SBSData::TDCHit hit){fRightHit = hit;};
	
protected: 
	Int_t fBarNum;
	Int_t fBarOff;
	SBSTimingHodoscopePMT* fLPMT;
	SBSTimingHodoscopePMT* fRPMT;
	
	double fMeanTime;
	double fTimeDiff;
	double fMeanToT;
	double fHitPos;
	double fElementPos;
	SBSData::TDCHit fLeftHit;
	SBSData::TDCHit fRightHit;

public:
	ClassDef(SBSTimingHodoscopeBar,5) // Scintillator bar

};
////////////////////////////////////////////////////////////////////////////////

#endif
