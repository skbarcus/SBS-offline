//////////////////////////////////////////////////////////////////
//
//   ScalerScalerEvtHandler
//
//   Event handler for Hall A scalers (THaScalerEvtHandler)
//   R. Michaels,  Sept, 2014
//   
//   Lineage: 
//   - Based on TriScalerEvtHandler for tritium experiments (Hanjie Liu, UMass) 
//   - Adapted for SBS experiments (David Flay, JLab) 
//
//   This class does the following
//      For a particular set of event types (here, event type 140)
//      decode the scalers and put some variables into global variables.
//      The global variables can then appear in the Podd output tree T.
//      In addition, a tree "TS" is created by this class; it contains
//      just the scaler data by itself.  Note, the "fName" is concatenated
//      with "TS" to ensure the tree is unqiue; further, "fName" is
//      concatenated with the name of the global variables, for uniqueness.
//      The list of global variables and how they are tied to the
//      scaler module and channels is in the scaler.map file, or could
//      be hardcoded here.   
//      NOTE: if you don't have the scaler map file (e.g. db_LeftScalevt.dat)
//      there will be no variable output to the Trees.
//
//   To use in the analyzer, your setup script needs something like this
//       gHaEvtHandlers->Add (new LHRSScalerEvtHandler("Left","HA scaler event type 140"));
//
//   To enable debugging you may try this in the setup script
// 
//     LHRSScalerEvtHandler *lscaler = new LHRSScalerEvtHandler("Left","HA scaler event type 140");
//     lscaler->SetDebugFile("LeftScaler.txt");
//     gHaEvtHandlers->Add (lscaler);
//
/////////////////////////////////////////////////////////////////////

#include "LHRSScalerEvtHandler.h"

#include "THaAnalysisObject.h"
#include "THaEvtTypeHandler.h"
#include "THaCodaData.h"
#include "THaEvData.h"
#include "THaVarList.h"
#include "THaString.h"
#include "THaAnalyzer.h"

#include "GenScaler.h"
#include "Scaler3800.h"
#include "Scaler3801.h"
#include "Scaler1151.h"
#include "Scaler560.h"

#include "VarDef.h"
#include "Textvars.h" 

#include "TNamed.h"
#include "TMath.h"
#include "TString.h"

#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <sstream>


using namespace std;
using namespace Decoder;
using namespace THaString;

static const UInt_t ICOUNT    = 1;
static const UInt_t IRATE     = 2;
static const UInt_t MAXCHAN   = 32;
static const UInt_t MAXTEVT   = 5000;
static const UInt_t defaultDT = 4;

LHRSScalerEvtHandler::LHRSScalerEvtHandler(const char *name, const char* description)
  : THaEvtTypeHandler(name,description), evcount(0), fNormIdx(-1), fNormSlot(-1),
    dvars(0), fScalerTree(0)
{
  rdata = new UInt_t[MAXTEVT];
}
//______________________________________________________________________________
LHRSScalerEvtHandler::~LHRSScalerEvtHandler()
{
  delete [] rdata;
  if (fScalerTree) {
    delete fScalerTree;
  }
}
//______________________________________________________________________________
Int_t LHRSScalerEvtHandler::End( THaRunBase* r)
{
  if (fScalerTree) fScalerTree->Write();
  //Insert here the addition of summary filling
  THaAnalyzer* analyzer = THaAnalyzer::GetInstance();
  if(analyzer!=nullptr){// check that the analyzer actually exists... otherwise, skip
    const char* summaryfilename = analyzer->GetSummaryFileName();
    cout << "LHRSScalerEvtHandler Summary in " << summaryfilename << endl;
    if( strcmp(summaryfilename,"")!=0  ) {
      ofstream ostr(summaryfilename, std::ofstream::app);
      if( ostr ) {
	// Write to file via cout
	//streambuf* cout_buf = cout.rdbuf();
	//cout.rdbuf(ostr.rdbuf());
	TDatime now;
	ostr << "LHRS scalers Summary " //<< fRun->GetNumber()
	     << " completed " << now.AsString()
	     << endl << " count " << evcount << endl
	     << endl;

	for (UInt_t i = 0; i < scalerloc.size(); i++) {
	  TString name = scalerloc[i]->name; 
	  //tinfo = name + "/D";
	  //fScalerTree->Branch(name.Data(), &dvars[i], tinfo.Data(), 4000);
	  ostr << " Scaler " << name.Data() <<  " value: " << dvars[i] << endl;
	}	
	//std::vector<Decoder::GenScaler*> scalers;
	//std::vector<ScalerVar*> scalerloc;
	ostr << endl;
	
	//cout.rdbuf(cout_buf);
	ostr.close();
	
      }
      
    }
  }
  return 0;
}
//______________________________________________________________________________
Int_t LHRSScalerEvtHandler::Analyze(THaEvData *evdata)
{
  Int_t lfirst=1;

  if ( !IsMyEvent(evdata->GetEvType()) ) return -1;

  if (fDebugFile) {
    *fDebugFile << endl << "---------------------------------- "<<endl<<endl;
    *fDebugFile << "\nEnter LHRSScalerEvtHandler  for fName = "<<fName<<endl;
    EvDump(evdata);
  }

  if (lfirst && !fScalerTree) {

    lfirst = 0; // Can't do this in Init for some reason

    TString sname1 = "TS";
    TString sname2 = sname1 + fName;
    TString sname3 = fName + "  Scaler Data";

    if (fDebugFile) {
      *fDebugFile << "\nAnalyze 1st time for fName = "<<fName<<endl;
      *fDebugFile << sname2 << "      " <<sname3<<endl;
    }

    fScalerTree = new TTree(sname2.Data(),sname3.Data());
    fScalerTree->SetAutoSave(200000000);

    TString name, tinfo;

    name = "evcount";
    tinfo = name + "/D";
    fScalerTree->Branch(name.Data(), &evcount, tinfo.Data(), 4000);

    for (UInt_t i = 0; i < scalerloc.size(); i++) {
      name = scalerloc[i]->name; 
      tinfo = name + "/D";
      fScalerTree->Branch(name.Data(), &dvars[i], tinfo.Data(), 4000);
    }

  }  // if (lfirst && !fScalerTree)

  // Parse the data, load local data arrays.

  Int_t ndata = evdata->GetEvLength();
  if(fDebugFile) *fDebugFile << "NDATA = " << dec << ndata << std::endl;
  if (ndata >= static_cast<Int_t>(MAXTEVT)) {
	  cout << "LHRSScalerEvtHandler:: ERROR: Event length crazy "<<endl;
	  ndata = MAXTEVT-1;
  }

  if (fDebugFile) *fDebugFile<<"\n\nLHRSScalerEvtHandler :: Debugging event type "<<dec<<evdata->GetEvType()<<endl<<endl;

  // local copy of data
  // FIXME: event is ASCII, not 32-bit binary! We need to convert ASCII to 32-bit binary  
  for (Int_t i=0; i<ndata; i++) rdata[i] = evdata->GetRawData(i);

  Int_t nskip=0;
  UInt_t *P = rdata;
  // UInt_t *Pstop = rdata+ndata;
  int k=0;

  Int_t ifound=0;
  Int_t itimeout=0;
  UInt_t NScalers=0;

  // Added by D Flay (10/27/21) for parsing data
  std::string word[MAXTEVT]; 
  UInt_t A[MAXTEVT]; 
  int NWORDS=0;

  // skip the first 4 words because it looks like the first word associated with 
  // the scalers starts there... 
  P = P + 4;

  // do the conversion 
  char *pc      = (char *)P;
  NWORDS        = ParseData(pc,word,A);
  UInt_t *p     = A;  
  UInt_t *pstop = p + ndata - 4;  

  char msg[200];  

  if(fDebugFile){
     *fDebugFile << "**** parsed int array = " << p << ", NWORDS = " << dec << NWORDS << endl; 
     for(int ii=0;ii<NWORDS;ii++){
        sprintf(msg,"   i = %03d, word = %s, int = %u, hex = %02x",ii,word[ii].c_str(),p[ii],p[ii]); 
        *fDebugFile << msg << endl; 
     }
  }

  while (p < pstop && k < ndata) {
    if (fDebugFile) {
      // *fDebugFile << "p  and  pstop  "<< k++ << "   " << p << "   " << pstop << "   data = " << hex << *p << "   " << dec << endl;
      *fDebugFile << "k = " << k << " p  = " << p << " pstop = " << pstop << " data = " << hex << *p << " (hex), " << dec << endl;
    }
    nskip = 1;
    itimeout=0;
    NScalers = scalers.size();
    if (fDebugFile)*fDebugFile << "**** NUM SCALERS = " << NScalers << std::endl;
    for (UInt_t j=0; j<NScalers; j++) {
       if(fDebugFile) *fDebugFile << "**** DEBUG PRINT j = " << j << " ****" << std::endl;
       // bump pointer until scaler found, and don't decode if already found for this event.
       if (scalerloc[j]->found) continue;
       if (fDebugFile) *fDebugFile << "Slot " << scalers[j]->GetSlot() << endl;
       while (p < pstop) {
          if (scalers[j]->IsSlot(*p) == kTRUE) {
                  scalerloc[j]->found=kTRUE;
                  ifound = 1;
                  goto found1;
          }
          p++;
          if (itimeout++ > 5000) { // avoid infinite loop
                  std::cout << "LHRSScalerEvtHandler:: cannot find a slot "<< std::endl;
                  goto giveup1;
          }
       }
       found1:
	    if(p==pstop && ifound==0) break;
             if (fDebugFile)*fDebugFile << "\n[LHRSScalerEvtHandler::Analyze]: FOUND EVENT 140!" << std::endl;
	    nskip = scalers[j]->Decode(p);
	    if (fDebugFile && nskip > 1) {
		    *fDebugFile << "\n===== Scaler # "<<j<<"     fName = "<<fName<<"   nskip = "<<nskip<<endl;
		    scalers[j]->DebugPrint(fDebugFile);
	    }
	    if (nskip > 1) goto continue1;
    }
    continue1:
       p = p + nskip;
    k++; 
  }
      
  giveup1:
    if (fDebugFile) {
      *fDebugFile << "Finished with decoding.  "<<endl;
      *fDebugFile << "   Found flag   =  "<<ifound<<endl;
    }

  // L-HRS has headers which are different from R-HRS, but both are
  // event type 140 and come here.  If you found no headers, it was
  // the other arms event type.  (The arm is fName).
  if (!ifound) return 0;

  // The correspondance between dvars and the scaler and the channel
  // will be driven by a scaler.map file, or could be hard-coded.
  for (size_t i = 0; i < scalerloc.size(); i++) {
  	size_t ivar  = scalerloc[i]->ivar;
  	size_t idx   = scalerloc[i]->index;
  	size_t ichan = scalerloc[i]->ichan;
  	if (fDebugFile) *fDebugFile << "Debug dvars i = "<<i<<", var = "<<ivar<<", index = "<<idx<<", ch = "<<ichan<<endl;
  	if( ivar < scalerloc.size() && idx < scalers.size() && ichan < MAXCHAN ){
  		if (scalerloc[ivar]->ikind == ICOUNT) dvars[ivar] = scalers[idx]->GetData(ichan);
  		if (scalerloc[ivar]->ikind == IRATE)  dvars[ivar] = scalers[idx]->GetRate(ichan);
  		if (fDebugFile) *fDebugFile << "   dvars kind = "<<scalerloc[ivar]->ikind<<", value = "<<dvars[ivar]<<endl;
  	}else{
  		cout << "LHRSScalerEvtHandler:: ERROR:: incorrect index "<<ivar<<"  "<<idx<<"  "<<ichan<<endl;
  	}
  }

  evcount = evcount + 1.0;
  
  for (size_t j=0; j<scalers.size(); j++) {
  	scalers[j]->Clear("");
  	scalerloc[j]->found=kFALSE;
  }
  
  if (fDebugFile) *fDebugFile << "scaler tree ptr  "<<fScalerTree<<endl;
  
  if (fScalerTree) fScalerTree->Fill();
  
  return 1;
}
//______________________________________________________________________________
THaAnalysisObject::EStatus LHRSScalerEvtHandler::Init(const TDatime& date)
{
  const int LEN = 200;
  char cbuf[LEN];

  fStatus = kOK;
  fNormIdx = -1;

  // std::cout << "[LHRSScalerEventHandler::Init]: Initializing " << fName << "..." << std::endl;

  eventtypes.push_back(140);  // what events to look for

  TString dfile;
  dfile = fName + "scaler.txt";

  // Parse the map file which defines what scalers exist and the global variables.
  TString sname0 = "Scalevt";
  TString sname;
  sname = fName+sname0;

  FILE *fi = Podd::OpenDBFile(sname.Data(), date);
  if ( !fi ) {
     cout << "Cannot find db file for "<<fName<<" (file = " << sname << ") scaler event handler"<<endl;
     return kFileError;
  }

  string::size_type minus1 = string::npos;
  string::size_type pos1;
  const string scomment = "#";
  const string svariable = "variable";
  const string smap = "map";
  vector<string> dbline;

  while( fgets(cbuf, LEN, fi) != NULL) {
     std::string sinput(cbuf);
     if (fDebugFile) *fDebugFile << "string input "<<sinput<<endl;
     dbline = Podd::vsplit(sinput);
     if (dbline.size() > 0) {
             pos1 = FindNoCase(dbline[0],scomment);
             if (pos1 != minus1) continue;
             pos1 = FindNoCase(dbline[0],svariable);
             if (pos1 != minus1 && dbline.size()>4) {
                string sdesc = "";
                for (UInt_t j=5; j<dbline.size(); j++) sdesc = sdesc+" "+dbline[j];
                Int_t islot = atoi(dbline[1].c_str());
                Int_t ichan = atoi(dbline[2].c_str());
                Int_t ikind = atoi(dbline[3].c_str());
                if (fDebugFile)
                        *fDebugFile << "add var "<<dbline[1]<<"   desc = "<<sdesc<<"    islot= "<<islot<<"  "<<ichan<<"  "<<ikind<<endl;
                TString tsname(dbline[4].c_str());
                TString tsdesc(sdesc.c_str());
                AddVars(tsname,tsdesc,islot,ichan,ikind);
             }
             pos1 = FindNoCase(dbline[0],smap);
             if (pos1 != minus1 && dbline.size()>6) {
           	  Int_t imodel, icrate, islot, inorm;
           	  UInt_t header, mask;
           	  char cdum[20];
           	  sscanf(sinput.c_str(),"%s %d %d %d %x %x %d \n",cdum,&imodel,&icrate,&islot, &header, &mask, &inorm);
           	  if (fNormSlot >= 0 && fNormSlot != inorm) cout << "LHRSScalerEvtHandler:: WARN: contradictory norm slot "<<inorm<<endl;
           	  fNormSlot = inorm;  // slot number used for normalization.  This variable is not used but is checked.
           	  Int_t clkchan = -1;
           	  Double_t clkfreq = 1;
           	  if (dbline.size()>8) {
           		  clkchan = atoi(dbline[7].c_str());
           		  clkfreq = 1.0*atoi(dbline[8].c_str());
           	  }
           	  if (fDebugFile) {
           		  *fDebugFile << "map line "<<dec<<imodel<<"  "<<icrate<<"  "<<islot<<endl;
           		  *fDebugFile <<"   header  0x"<<hex<<header<<"  0x"<<mask<<dec<<"  "<<inorm<<"  "<<clkchan<<"  "<<clkfreq<<endl;
           	  }
           	  switch (imodel) {
           		  case 560:
           			  scalers.push_back(new Scaler560(icrate, islot));
           			  break;
           		  case 1151:
           			  scalers.push_back(new Scaler1151(icrate, islot));
           			  break;
           		  case 3800:
           			  scalers.push_back(new Scaler3800(icrate, islot));
           			  break;
           		  case 3801:
           			  scalers.push_back(new Scaler3801(icrate, islot));
           			  break;
           		  default: 
           			  std::cout << "LHRSScalerEvtHandler:: ERROR: Invalid model " << imodel << std::endl;
           	  }
           	  if (scalers.size() > 0) {
           		  UInt_t idx = scalers.size()-1;
           		  scalers[idx]->SetHeader(header, mask);
           		  // The normalization slot has the clock in it, so we automatically recognize it.
           		  // fNormIdx is the index in scaler[] and 
           		  // fNormSlot is the slot#, checked for consistency
           		  if (clkchan >= 0) {  
           			  scalers[idx]->SetClock(defaultDT, clkchan, clkfreq);
           			  fNormIdx = idx;
           			  if (islot != fNormSlot) cout << "LHRSScalerEvtHandler:: WARN: contradictory norm slot ! "<<islot<<endl;  
           			  if (fDebugFile) *fDebugFile <<"Setting scaler clock ... channel = "<<clkchan<<" ... freq = "<<clkfreq<<"   fNormIdx = "<<fNormIdx<<"  fNormSlot = "<<fNormSlot<<"  slot = "<<islot<<endl;
           		  }
           	  }
             }
     }
  }

  // need to do LoadNormScaler after scalers created and if fNormIdx found.
  Int_t nscalers = static_cast<Int_t>(scalers.size());
  if ( fNormIdx >= 0 && fNormIdx < nscalers ) {
	  for (Int_t i = 0; i < nscalers; i++) {
		  if (i==fNormIdx) continue;
		  scalers[i]->LoadNormScaler(scalers[fNormIdx]);
	  }
  }

#ifdef HARDCODED
  // This code is superseded by the parsing of a map file above.  It's another way ...
  if (fName == "Left") {
	  AddVars("TSbcmu1", "BCM x1 counts", 1, 4, ICOUNT);
	  AddVars("TSbcmu1r","BCM x1 rate",  1, 4, IRATE);
	  AddVars("TSbcmu3", "BCM u3 counts", 1, 5, ICOUNT);
	  AddVars("TSbcmu3r", "BCM u3 rate",  1, 5, IRATE);
  } else {
	  AddVars("TSbcmu1", "BCM x1 counts", 0, 4, ICOUNT);
	  AddVars("TSbcmu1r","BCM x1 rate",  0, 4, IRATE);
	  AddVars("TSbcmu3", "BCM u3 counts", 0, 5, ICOUNT);
	  AddVars("TSbcmu3r", "BCM u3 rate",  0, 5, IRATE);
  }
#endif

DefVars(); 

#ifdef HARDCODED
   // This code is superseded by the parsing of a map file above.  It's another way ...
   if (fName == "Left") {
   	scalers.push_back(new Scaler1151(1,0));
   	scalers.push_back(new Scaler3800(1,1));
   	scalers.push_back(new Scaler3800(1,2));
   	scalers.push_back(new Scaler3800(1,3));
   	scalers[0]->SetHeader(0xabc00000, 0xffff0000);
   	scalers[1]->SetHeader(0xabc10000, 0xffff0000);
   	scalers[2]->SetHeader(0xabc20000, 0xffff0000);
   	scalers[3]->SetHeader(0xabc30000, 0xffff0000);
   	scalers[0]->LoadNormScaler(scalers[1]);
   	scalers[1]->SetClock(4, 7, 1024);
   	scalers[2]->LoadNormScaler(scalers[1]);
   	scalers[3]->LoadNormScaler(scalers[1]);
   } else {
   	scalers.push_back(new Scaler3800(2,0));
   	scalers.push_back(new Scaler3800(2,0));
   	scalers.push_back(new Scaler1151(2,1));
   	scalers.push_back(new Scaler1151(2,2));
   	scalers[0]->SetHeader(0xceb00000, 0xffff0000);
   	scalers[1]->SetHeader(0xceb10000, 0xffff0000);
   	scalers[2]->SetHeader(0xceb20000, 0xffff0000);
   	scalers[3]->SetHeader(0xceb30000, 0xffff0000);
   	scalers[0]->SetClock(4, 7, 1024);
   	scalers[1]->LoadNormScaler(scalers[0]);
   	scalers[2]->LoadNormScaler(scalers[0]);
   	scalers[3]->LoadNormScaler(scalers[0]);
   }
#endif

   // Verify that the slots are not defined twice
   for (UInt_t i1=0; i1 < scalers.size()-1; i1++) {
      for (UInt_t i2=i1+1; i2 < scalers.size(); i2++) {
         if (scalers[i1]->GetSlot()==scalers[i2]->GetSlot())
              cout << "LHRSScalerEvtHandler:: WARN:  same slot defined twice"<<endl;
      }
   }
   // Identify indices of scalers[] vector to variables.
   for (UInt_t i=0; i < scalers.size(); i++) {
      for (UInt_t j = 0; j < scalerloc.size(); j++) {
         if (scalerloc[j]->islot==static_cast<UInt_t>(scalers[i]->GetSlot()))
              scalerloc[j]->index = i;
      }
   }

   if(fDebugFile) {
      *fDebugFile << "LHRSScalerEvtHandler:: Name of scaler bank "<<fName<<endl;
      for (UInt_t i=0; i<scalers.size(); i++) {
         *fDebugFile << "Scaler  #  "<<i<<endl;
         scalers[i]->SetDebugFile(fDebugFile);
         scalers[i]->DebugPrint(fDebugFile);
      }
   }
   for (size_t j=0; j<scalers.size(); j++) {
      scalers[j]->Clear("");
      scalerloc[j]->found=kFALSE;
   }

   return kOK;
}
//______________________________________________________________________________
void LHRSScalerEvtHandler::AddVars(TString name, TString desc, Int_t islot,
				  Int_t ichan, Int_t ikind)
{
  // need to add fName here to make it a unique variable.  (Left vs Right HRS, for example)
  // TString name1 = fName + name;
  TString name1 = Form("%s.%s",fName.Data(),name.Data());
  TString desc1 = fName + desc;
  // We don't yet know the correspondence between index of scalers[] and slots.
  // Will put that in later.
  ScalerVar *loc = new ScalerVar(name1, desc1, 0, islot, ichan, ikind);
  loc->ivar = scalerloc.size();  // ivar will be the pointer to the dvars array.
  scalerloc.push_back(loc);
}
//______________________________________________________________________________
void LHRSScalerEvtHandler::DefVars()
{
  // called after AddVars has finished being called.
  Int_t Nvars = scalerloc.size();
  if (Nvars == 0) return;
  dvars = new Double_t[Nvars];  // dvars is a member of this class
  memset(dvars, 0, Nvars*sizeof(Double_t));
  if (gHaVars) {
  	if(fDebugFile) *fDebugFile << "LHRSScalerEvtHandler:: Have gHaVars "<<gHaVars<<endl;
  } else {
  	cout << "No gHaVars ?!  Well, that's a problem !!"<<endl;
  	return;
  }
  if(fDebugFile) *fDebugFile << "LHRSScalerEvtHandler:: scalerloc size "<<scalerloc.size()<<endl;
  const Int_t* count = 0;
  for (UInt_t i = 0; i < scalerloc.size(); i++) {
  	gHaVars->DefineByType(scalerloc[i]->name.Data(), scalerloc[i]->description.Data(),
  			&dvars[i], kDouble, count);
  }
}
//______________________________________________________________________________
int LHRSScalerEvtHandler::ParseData(char *msg,std::string *word,UInt_t *word_int){
   // loop through the message (msg) and convert into data words 
   // - input:  a char array to parse (i.e., scaler data)  
   // - output: std::string array (word) and int array (word_int)     
   char data[200];
   strcpy(data,"");
  
   char *pEnd;
 
   // std::cout << "Message to decode: " << std::endl;
   // std::cout << msg << std::endl;
   
   int j=0;
   int length = strlen(msg);
   for(int i=0;i<length;i++){
      if(msg[i]=='\n'){
      	// now have a full word
      	word[j]     = data;
        word_int[j] = std::strtol(data,&pEnd,16);  // base 16 (hex)
      	// increment the index on the word array
      	j++;
      	// empty the constructed word 
      	strcpy(data,"");
      }else{
      	// not a new line, build the word  
      	sprintf(data,"%s%c",data,msg[i]);
      }
   }
   
   return j; // return the number of words 
}
//______________________________________________________________________________
ClassImp(LHRSScalerEvtHandler)
