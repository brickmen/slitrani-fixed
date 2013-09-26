// @(#)root/html:$Id: TLitSpontan.cpp 2008-06-11
// Author: F.X. Gentit <http://gentit.home.cern.ch/gentit/>

/*************************************************************************
 * Copyright (C) Those valid for CNRS software.                          *
 *************************************************************************/
#include "TROOT.h"
#include "TVector3.h"
#include "TFile.h"
#include "TGeoManager.h"
#include "TGeoPhysicalNode.h"
#include "TVirtualGeoTrack.h"
#include "TwoPadDisplay.h"
#include "TCleanOut.h"
#include "TLit.h"
#include "TLitPhys.h"
#include "TLitMedium.h"
#include "TLitVolume.h"
#include "TLitDetector.h"
#include "TLitResults.h"
#include "TLitMarrow.h"
#include "TLitPhoton.h"
#include "TLitSpontan.h"

ClassImp(TLitSpontan)
//______________________________________________________________________________
//
//  (A) - GENERAL DESCRIPTION OF TLITSPONTAN
//
//
//  With TLitSpontan, you can simulate many types of beam of photons :
//
//   (1) photons emitted from any point inside a TGeoVolume with an equal
//       probability
//   (2) photons emitted from a fixed point inside a TGeoVolume
//   (3) Photons emitted from a face of a TGeoVolume: first, a point is generated
//       inside the TGeoVolume with an equal probability. Then the point is translated
//       inside the TGeoVolume, along a given direction, until the edge of the
//       TGeoVolume. This allows you to simulate laser light or beam of photons
//       from an optical fibre.
//
//  The only types of photons that you cannot simulate with TLitSpontan are:
//
//   (1) the ones generated by the crossing of a particle, like a muon. For this,
//       look at class TLitBeam.
//   (2) the ones generated by an electromagnetic shower. For this, look
//       at class TLitCascade
//   (3) the ones generated by gammas of less than 1 Mev. For this, look
//       at class TLitGamma.
//
//  The 2 constructors of TLitSpontan contain as arguments:
//
//  name      : name of the spontaneous source
//  title     : title of the spontaneous source
//  path      : path name of the TGeoVolume inside which photon is generated
//              for instance "/TOP_1/REV_1/FIBT_1/FIB_1"
//  moving    : if moving is true [default false], the TGeoVolume acting as
//              photon cradle and pointed to by path will be moved from run
//              to run, by a call to method MoveCradle(). Notice that displacing
//              a TGeoVolume AFTER the geometry has been closed by a call to
//              gGeoManager->CloseGeometry() cannot be done simply by calling
//              TGeoVolume::RemoveNode() and TGeoVolume::AddNode(). Read
//              � "Representing Misalignments of the Ideal Geometry", p340 of the
//              ROOT user manual to understand how it is done, using class
//              TGeoPhysicalNode.
//  checkmove : if true [default false], ask TGeo to check that no extrusion
//              have occured during the move
//
//
//  All the characteristics of the emission of photons have been defined before
// booking TLitSpontan by a call to TLitVolume::SetEmission() and
// TLitVolume::SetEField(). SetEField() defines the characteristics of the E field,
// allowing possibly to ask for a polarized beam of photons. Look at these 2 methods
// for more infos.
//  It is vital to call TLitSpontan only AFTER the geometry of the setup is totally
// defined, after the call to TGeoManager::CloseGeometry().
//  With the 1st constructor, the spectrum of the emitted photons is the one of
// the TLitMedium of the TGeoVolume associated with "startV". It means that this
// TLitMedium has to be defined as fluorescent, i.e. a call to
//    TLitMedium::FindSpectrum() or
//    TLitMedium::SetLitSpectrum() has been done.
//  If you use the 2nd constructor, containing a value for the wavelength, then
// all photons will be generated with THIS value of wavelength NOT TAKING INTO
// ACCOUNT the fluorescent characteristics of the TLitMedium. In this case, it is
// not necessary that the TLitMedium be fluorescent.
//
//  To generate photons call the method TLitSpontan::Gen().
//
//  (B) - SIMULATING AN OPTICAL FIBRE
//
//  Let us give here a detailed description on how to simulate an optical
//fibre in SLitrani.
//
//     (B1) - Shape for simulating the fibre
//
//  Define a shape of type TGeoTube without hole to simulate the fibre. The 2
// last parameters of the call to TLitVolume::SetEmission() are:
//
//   emface : if true and srcfix false, photons are emitted from a face of the TGeoVolume
//            according to possibility (3) described above. DEFINE IT TRUE
//   dirfce : after having generated with equal probability a point anywhere inside the
//            TGeoVolume, the point is translated along direction "dirfce" until the
//            edge of the TGeoVolume. DEFINE IT PARALLEL to the axis of the TGeoTube.
//
//  Choose for the TLitMedium of the TGeoTube (for instance "plexi") the right
// index of refraction of your fibre and an absorption length of 0! Why ? It is
// important to understand that in SLitrani, the photons are emitted from the face
// of the fibre, BUT FROM WITHIN THE FIBRE ! It means that the photons have first
// to make the transition from within the material of the fibre (plexi) to-
// wards the material at the exit of the fibre (for instance air, or grease,
// or glue). As they have to make this transition, the possibility
// exists that the photon does not make a refraction (does not exit from
// the fibre) but makes a reflection (returns inside the fibre). If it
// happens, it is important that the photon be immediatly killed, since it
// has no chance to come back anymore. It is also important that these pho-
// tons, which have never left the fibre do not count among the emitted
// photons when doing for instance a calculation of efficiency. In SLitrani,
// photons having been absorbed into a material of absorption length strictly
// 0 are counted apart (into the counter fNpAbsL0 of TLitResults) and are sub-
// tracted from the generated photons when statistics calculations have to be
// done. This is the reason of this absorption length of 0.
//  You can ask why this extra complication of generating the photons from
// within the internal face of the fibre? Besides the annoyance described
// above of those photons returning back into the fibre, there is the second
// annoyance that the aperture of the photons of a fibre is generally given
// in air by the manufacturer of the fibre, so that the user of SLitrani has
// to convert aperture or angular distribution given in air into aperture
// and angular distribution inside the fibre. The answer is : to avoid the
// much bigger annoyance encountered when the material at the exit of the
// fibre is not air! Suppose for instance that you intend to do a study of
// many different glues that you intend to put at the exit of the fibre. You
// would have to recalculate aperture and angular distribution for each type
// of glue! With the method of SLitrani, you have to do it only once and you
// have nothing to do on the fibre when you change from one type of glue to
// an other type.
//
//     (B2) - Angular distribution of the photons from the fibre
//
//  If you have read the explanations given in (B1), you know at this point
// that you have to give the angular distribution of the photons INSIDE THE
// FIBRE and not in air. You specify the kind of distribution you want with
// the 4 first arguments of TLitVolume::SetEmission():
//
// kind   : type of distribution for the direction of the emitted photons around dir
//          [default on4pi]
//
//        on4pi        ==> photons are emitted isotropically, with the distribution
//                      sin(theta)*dtheta*dphi, on 4 pi.
//        flat         ==> photons are emitted isotropically, with the distribution
//                      sin(theta)*dtheta*dphi,with theta between 0 and tmax
//        sinuscosinus ==> photons are emitted with the non isotropic distribution
//                      sin(theta)*cos(theta)*dtheta*dphi ,with theta between 0 and
//                      tmax, favouring slightly the forward direction
//        provided     ==> photons are emitted using the distribution provided
//                      as a fit of type TSplineFit
//
// tmax   : maximum value for the theta angle around dir. tmax is given in degree
//          [default 180.0]
// dir    : axis around which photons are emitted, in local coordinate system of
//          TGeoVolume [default (0,0,1)
// fit    : in case kind == provided, fit to be used
//          [default 0]
//
//  In the case you provide yourself the distribution, case "provided",
// please be careful about the following NON TRIVIAL issues :
//    (1) - To know how to define yourself a distribution of type TSplineFit
// to be used to generate random numbers according to a distribution, please
// read the documentation about TSplineFit and in particular the documentation
// about the 2 methods UseForRandom() and GetRandom(). You have to call
// UseForRandom() !
//    (2) - your distribution has to generate theta in radians, not degrees.
//    (3) - you have to give a 2-dimensional distribution, not a 1-dimensio-
// nal distribution. If you have measured yourself the distribution, your
// measurement was very likely a 1-dimensional distribution, (you have
// displaced your instrument inside a plane and you have assumed isotropy in
// phi) so you have to multiply each measurement by sin(theta).
//    (4) - you have probably measured your distribution in air, so recalcu-
// lates all angles in the material of the fibre using the Fresnel formula
//        ni sin(ti) = nr sin(tr).
//    (5) - Even if you have provided yourself the distribution, the value
// for aperture still matters : if a theta angle is generated from your
// distribution which is bigger than aperture, then we start again until a
// value smaller than aperture is found.
//
//
//
//   (C) - Record of death point of photon when seen.
//
//  Look at the new method SetFillDeath() to get a crude way of
// registering the death point of the seen photons into CINT user
// defined histograms.
//
TLitSpontan::TLitSpontan(const char *name,const char *title, const char *path,
  Bool_t moving,Bool_t checkmove): TNamed(name,title) {
  //  Constructor to be used when the wavelength of the photon is generated
  // according to the TLitSpectrum of the TLitMedium of the TGeoVolume
  // associated with the TLitVolume "startV". It means that this TLitMedium has
  // to be defined as fluorescent, i.e. a call to
  //
  //    TLitMedium::FindSpectrum() or
  //    TLitMedium::SetLitSpectrum() has been done.
  //
  //  Arguments :
  //
  //  name      : name of the spontaneous source
  //  title     : title of the spontaneous source
  //  path      : path name of the TGeoVolume inside which photon is generated
  //              for instance "/TOP_1/REV_1/FIBT_1/FIB_1"
  //  moving    : if moving is true [default false], the TGeoVolume acting as
  //              photon cradle and pointed to by path will be moved from run
  //              to run, by a call to method MoveCradle(). Notice that displacing
  //              a TGeoVolume AFTER the geometry has been closed by a call to
  //              gGeoManager->CloseGeometry() cannot be done simply by calling
  //              TGeoVolume::RemoveNode() and TGeoVolume::AddNode(). Read
  //              � "Representing Misalignments of the Ideal Geometry", p340 of the
  //              ROOT user manual to understand how it is done, using class
  //              TGeoPhysicalNode.
  //  checkmove : if true [default false], ask TGeo to check that no extrusion
  //              have occured during the move
  //
  //  The characteristics of the emission of the photons have been previously
  // defined by a call to fStartLitVol->SetEmission(). Look at this method.
  //
  const char *met = "TLitSpontan";
  TString nameGeoV = "";
  Init();
  fSourcePath  = path;
  NameFromPath(nameGeoV);
  fStartGeoVol    = gGeoManager->FindVolumeFast(nameGeoV.Data());
  if (!fStartGeoVol) gCleanOut->MM(fatal,met,"TGeoVolume not found",ClassName());
  fStartLitVol = (TLitVolume*)fStartGeoVol->GetField();
  if (!fStartLitVol) gCleanOut->MM(fatal,met,"TLitVolume MUST be associated with TGeoVolume",ClassName());
  fWvlgthFixed = kFALSE;
  fMoving    = moving;
  fCheckMove = checkmove;
  if (fMoving) {
    fGeoPhysNode = new TGeoPhysicalNode(fSourcePath.Data());
    if (!fGeoPhysNode) gCleanOut->MM(fatal,met,"TGeoPhysicalNode not created",ClassName());
    if (nameGeoV.CompareTo(fStartGeoVol->GetName()))
      gCleanOut->MM(fatal,met,"Bad name for TGeoPhysicalNode",ClassName());
  }
  if (!TLitResults::fgResults) TLitResults::fgResults = new TObjArray();
  if (!gLitGs) gLitGs = new TLitResults("GlobStat","Global statistics of all runs",0);
  fLitMedium = fStartLitVol->GetLitMedium();
  if (!fLitMedium) {
    gCleanOut->MM(fatal,met,"Beam cradle has no TLitMedium",ClassName());
  }
  else {
    if (!fLitMedium->IsFluorescent()) {
      gCleanOut->MM(error,met,"Medium of TLitVolume not fluorescent",ClassName());
    }
  }
}
TLitSpontan::TLitSpontan(const char *name,const char *title, const char *path,
  Double_t wvlgth,Bool_t moving,Bool_t checkmove): TNamed(name,title) {
  //  Constructor to be used when the wavelength of the photon is NOT generated
  // according to the TLitSpectrum of the TLitMedium of the TGeoVolume
  // associated with the TLitVolume "startV", but is FIXED at the value wvlgth
  // for all generated photons
  //
  //  Arguments :
  //
  //  name      : name of the spontaneous source
  //  title     : title of the spontaneous source
  //  path      : path name of the TGeoVolume inside which photon is generated
  //              for instance "/TOP_1/REV_1/FIBT_1/FIB_1"
  //  wvlgth    : fixed value of wavelength for all photons, in nanometers.
  //  moving    : if moving is true [default false], the TGeoVolume acting as
  //              photon cradle and pointed to by path will be moved from run
  //              to run, by a call to method MoveCradle(). Notice that displacing
  //              a TGeoVolume AFTER the geometry has been closed by a call to
  //              gGeoManager->CloseGeometry() cannot be done simply by calling
  //              TGeoVolume::RemoveNode() and TGeoVolume::AddNode(). Read
  //              � "Representing Misalignments of the Ideal Geometry", p340 of the
  //              ROOT user manual to understand how it is done, using class
  //              TGeoPhysicalNode.
  //  checkmove : if true [default false], ask TGeo to check that no extrusion
  //              have occured during the move
  //
  //  The characteristics of the emission of the photons have been previously
  // defined by a call to fStartLitVol->SetEmission(). Look at this method.
  //
  const char *met = "TLitSpontan";
  TString nameGeoV = "";
  Init();
  fSourcePath  = path;
  NameFromPath(nameGeoV);
  fStartGeoVol    = gGeoManager->FindVolumeFast(nameGeoV.Data());
  if (!fStartGeoVol) gCleanOut->MM(fatal,met,"TGeoVolume not found",ClassName());
  fStartLitVol = (TLitVolume*)fStartGeoVol->GetField();
  fWvlgth      = wvlgth;
  fT0          = 0.0;
  fWvlgthFixed = kTRUE;
  fMoving    = moving;
  fCheckMove = checkmove;
  if (fMoving) {
    fGeoPhysNode = new TGeoPhysicalNode(fSourcePath.Data());
    if (!fGeoPhysNode) gCleanOut->MM(fatal,met,"TGeoPhysicalNode not created",ClassName());
    if (nameGeoV.CompareTo(fStartGeoVol->GetName()))
      gCleanOut->MM(fatal,met,"Bad name for TgeoPhysicalNode",ClassName());
  }
  if (!TLitResults::fgResults) TLitResults::fgResults = new TObjArray();
  if (!gLitGs) gLitGs = new TLitResults("GlobStat","Global statistics of all runs",0);
  fLitMedium = fStartLitVol->GetLitMedium();
  if (!fLitMedium) {
    gCleanOut->MM(fatal,met,"Beam cradle has no TLitMedium",ClassName());
  }
}
TLitSpontan::~TLitSpontan() {
  // Destructor. TLitSpontan is not owner of any of its pointers, except fPhot
  if (fPhot) {
    delete fPhot;
    fPhot = 0;
  }
}
void TLitSpontan::AllTracksToDraw() {
  // Call this method if you want all tracks to be drawn
  fDrawCode = 1000000;
}
void TLitSpontan::FillDeath() const {
  // This method is called when a photon dies, in order to record the
  //coordinates of the death point, for the cases where the photon is seen.
  Int_t bin;
  if ((fFillDeath) && (TLit::Get()->fSeen)) {
    bin = fHX0->Fill(TLit::Get()->fX0);
    bin = fHY0->Fill(TLit::Get()->fY0);
    bin = fHZ0->Fill(TLit::Get()->fZ0);
  }
}
void TLitSpontan::Gen(Int_t run, Int_t nphot, Double_t xparam, Bool_t runstat,
  Bool_t forgetlast) {
  //  Starts a run generating nphot photons. The parameters are the following :
  //
  //  (1) run    : run number. Arbitrary, but has to be greater than 0.
  //
  //  (2) nphot  : number of photons to be generated in this run.
  //
  //  (3) xparam : this parameter is used as abscissa in the plotting of the
  //               results by the class TLitMarrow. For instance, if you
  //   have a setup with a crystal and optical fibre and you make 10 runs
  //   varying the incident angle of the fibre, you can choose the incident
  //   angle of the fibre as xparam. All plots provided by class TLitMarrow will
  //   then have the incident angle of the fibre as x coordinate. You will get
  //   efficiency versus incident angle, and so on. The title of class TLitMarrow,
  //   pointed to by the global pointer gLitGp, will help giving a meaningful title
  //   to the x axis. For instance, calling:
  //
  //       ==> gLitGp->SetTitle("angle of fibre");
  //
  //   the title of the x axis of the histogram of efficiency will be:
  //
  //       ==> "Efficiency versus angle of fibre"
  //
  //    If you do not give xparam, or give a value smaller than -1.0e+20, all
  //   plots of TLitMarrow will have the run number as x coord.
  //
  //  (4) (5) runstat and forgetlast :
  //
  //    if runstat == true [Default] AND forgetlast == false [Default] :
  //
  //      in memory   : this run statistics keeped but this run histos deleted
  //                     when next run begins
  //      on the file : this run statistics and histos recorded
  //
  //            It is the only configuration which allows the working of
  //          TLitMarrow, i.e. allowing to show histograms of quantities
  //          varying as a function of a run dependent parameter.
  //          Usage : normal case : try it first.
  //
  //    if runstat == true AND forgetlast == true :
  //
  //      in memory   : this run statistics and histograms deleted when next
  //                     run begins.
  //      on the file : this run statistics and histos recorded
  //
  //          Disadvantage : TLitMarrow not working, gLitGp pointer unavailable
  //          Advantage    : no increase of memory with runs
  //                         per run histograms still available on file
  //          Usage : use this configuration when your number of runs is big
  //                  but you still want per run statistics and histograms
  //                  on file.
  //
  //    if runstat == false ( forgetlast true or false ) :
  //
  //      in memory   : no statistics and histograms for this run
  //      on the file : no statistics and histograms for this run
  //
  //          Disadvantage : TLitMarrow not working, gLitGp pointer unavailable
  //                         per run statistics and histograms not available
  //          Advantage    : no increase of memory with runs
  //                         a bit faster, half less histograms to fill
  //          Usage : use this configuration when your number of runs is very
  //                  big and you are not interested in the per run statistics
  //                  and histograms.
  //
  //  Notice that in any case, the global statistics and histograms for all
  // runs is always present in memory and on the file. This global statistics
  // is an object of class TLitResults, pointed to by the pointer gLitGs.
  //
  const Int_t    pdg = 22;      //Nb given to gamma by the PDG (irrelevant)
  const char    *met = "Gen";
  const Double_t eps = 1.0e-10;
  Bool_t   ok;
  Int_t    kTrack,bin;
  Int_t    kfluo;
  Axis_t   afluo;
  Short_t  ks;
  TVector3 x0,K,E;
  Bool_t IsgGpok;
  fRunStat    = runstat;
  fForgetLast = forgetlast;
  if (run <= 0) {
    gCleanOut->MM(error,met,"Run number must be >= 1",ClassName());
    gCleanOut->MM(error,met,"Abs() + 1 taken",ClassName());
    run = TMath::Abs(run);
    run += 1;
  }
  fRun = run;
  fFullName  = GetName();
  fFullTitle = GetTitle();
  TString srun = "";
  if (fRun<10000) srun.Append('0');
  if (fRun<1000) srun.Append('0');
  if (fRun<100) srun.Append('0');
  if (fRun<10) srun.Append('0');
  srun += fRun;
  fFullName.Append('_');
  fFullName.Append(srun);
  fFullTitle.Append(" run ");
  fFullTitle.Append(srun);
  gCleanOut->M(info,"");
  gCleanOut->MM(info,met,fFullTitle.Data(),ClassName());
  //initializes the summary statistics of class TLitMarrow
  IsgGpok = (runstat && (!fForgetLast));
  if (!gLitGp && IsgGpok) gLitGp = new TLitMarrow("Summary","Runs",IsgGpok);
  if (gLitCs) {
    // If gLitCs is different from 0 here, it is the gLitCs of the previous run
    if (fForgetLast) {
      // If fForgetLast is true, the gLitCs of the previous run is removed from
      //the collection and then deleted. The content of TLitDetector::fgLitDetRun
      //is also deleted. No increase of memory used from run to run, but statistics
      //calculation at the end only possible by reading the ROOT file
      TLitResults::fgResults->Remove(gLitCs);
      delete gLitCs;
      gLitCs = 0;
    }
    else {
      // If fForgetLast is false, the variable containing the statistics, like
      //fNpGener,fNpSeen,fNpLossAny,...and so on are preserved, but all histograms
      //are deleted. It is also the case for the detectors in TLitDetector::fgLitDetRun:
      //statistics variable preserved, but histograms deleted. gLitCs of the previous
      //run remains without its histograms inside collection TLitResults::fgResults,
      //allowing statistics calculations at the end of all runs, without opening
      //the ROOT file
      gLitCs->DelHistos();
    }
  }
  gLitCs = 0;
  //  Books a new gLitCs for this run. A new collection of detectors
  // TLitDetector::fgLitDetRun will also be booked
  if (fRunStat) gLitCs = new TLitResults(fFullName.Data(),fFullTitle.Data(),fRun,xparam);
  //Open the .root file for writing, if not yet done
  if (TLit::Get()->fFilesClosed) {
    TLit::Get()->OpenFilesW(runstat);
    gROOT->cd();
    if ((!runstat) && (gLitCs)) {
      delete gLitCs;
      gLitCs = 0;
    }
  }
  //  We have to force a call to TLitMedium::NewWavelengthAll(fWvlgth) and
  // TLitVolume::NewWavelengthAll(fWvlgth); in case where the radiation damage
  // changes from run to run
  if (fWvlgth>0.0) {
    TLitMedium::NewWavelengthAll(fWvlgth);
    TLitVolume::NewWavelengthAll(fWvlgth);
  }
  //loop on generation of photons
  TLitDetector::fgLastUsedDet = 0;
  gGeoManager->ClearTracks();
  for (kTrack=1;kTrack<=nphot;kTrack++) {
    gLitGs->fNpGener++;
    if (gLitCs) gLitCs->fNpGener++;
    //first determines wavelength and time of emission of photon and store results
    //in histograms
    if (!fWvlgthFixed) fLitMedium->WaveAndLife(fWvlgth,fT0,kfluo);
    //update all values depending upon wavelength
    TLitMedium::NewWavelengthAll(fWvlgth);
    TLitVolume::NewWavelengthAll(fWvlgth);
    bin = gLitGs->fHTimeAll->Fill(fT0);
    bin = gLitGs->fHWvlgthAll->Fill(fWvlgth);
    if (gLitCs) {
      bin = gLitCs->fHTimeAll->Fill(fT0);
      bin = gLitCs->fHWvlgthAll->Fill(fWvlgth);
    }
    if (!fWvlgthFixed) {
      if (gLitGs->HasFluo()) {
        afluo = kfluo + eps;
        bin = gLitGs->fHTimeEach->Fill(fT0,afluo);
        bin = gLitGs->fHWvlgthEach->Fill(fWvlgth,afluo);
      }  //if (gLitGs->HasFluo())
      if ((gLitCs) && (gLitCs->HasFluo())) {
        afluo = kfluo + eps;
        bin = gLitCs->fHTimeEach->Fill(fT0,afluo);
        bin = gLitCs->fHWvlgthEach->Fill(fWvlgth,afluo);
      }  //if (gLitCs)
    }  //if (!fWvlgthFixed)
    //generates the photon
    ok = fStartLitVol->Gen(fSourcePath.Data());
    x0 = fStartLitVol->GetWCSSourcePoint();
    K  = fStartLitVol->GetWCSSourceDir();
    E  = fStartLitVol->GetWCSE();
    ks = fStartLitVol->GetChooseIndex();
    fPhot = new TLitPhoton(fRun,kTrack,x0,fStartLitVol,fSourcePath.Data(),fWvlgth,fT0,K,E,ks);
    // In case drawing of track required
    fPhot->SetToDraw((fDrawCode == kTrack) || (fDrawCode >= 1000000));
    if (fPhot->GetToDraw()) {
      fRecordedTracks++;
      fTrackIndex   = gGeoManager->AddTrack(kTrack,pdg);
      fPhot->SetCurrentTrack(fTrackIndex);
      fPhot->GetCurrentTrack()->SetLineColor(fTrackColor);
      fTrackColor++;
      if (fTrackColor>9) fTrackColor = 1;
    }
    //follow the photon from its birth to its death
    fPhot->Move();
    if (gLitGs->fNpAbnorm>TLitPhys::Get()->Anomalies()) {
      gCleanOut->MM(fatal,met,"Too many anomalies",ClassName());
    }
    //record death point, if asked for
    if (fFillDeath) FillDeath();
    delete fPhot;
    fPhot = 0;
  }//end for (kTrack=1;kTrack<=nphot;kTrack++)
  if (fRecordedTracks) {
    if (!gTwoPad) TLit::Get()->BookCanvas();
    gGeoManager->DrawTracks();
  }
  gCleanOut->MMI(info,met,"End of Run","Nphot",nphot,ClassName());
  //Put statistics of this run into the tree.
  if (gLitCs) {
    gLitCs->Conclusion();
    TLit::Get()->fLitFile->cd();
    TLit::Get()->fLitNb += TLit::Get()->fLitTree->Fill();
    gROOT->cd();
  }
}
void TLitSpontan::Init() {
  // Initialization of class variables
  InitP();
  fMoving         = kFALSE;
  fCheckMove      = kFALSE;
  fFillDeath      = kFALSE;
  fDrawCode       = -1;
  fTrackColor     = 1;
  fRecordedTracks = 0;
  fTrackIndex     = 0;
  fWvlgth         = -1.0;
  fRun            = -1;
}
void TLitSpontan::InitP() {
  // Pointers to 0
  fGeoPhysNode = 0;
  fStartGeoVol = 0;
  fStartLitVol = 0;
  fLitMedium   = 0;
  fHX0         = 0;
  fHY0         = 0;
  fHZ0         = 0;
  fPhot        = 0;
}
void TLitSpontan::MoveCradle(TGeoMatrix *M,Bool_t todraw) {
  // Moves the photon cradle at the new position M
  if (fMoving) {
    fGeoPhysNode->Align(M,0,fCheckMove);
    fStartGeoVol = fGeoPhysNode->GetVolume();
    fStartLitVol->SetGeoVolume(fStartGeoVol);
    if (todraw) gGeoManager->GetTopVolume()->Draw("");
  }
  else gCleanOut->MM(error,"MoveCradle","Cradle has not been announced as moving",ClassName());
}
void TLitSpontan::NameFromPath(TString &s) const {
  // Returns name of TGeoVolume of beam cradle using fSourcePath
  Int_t k,N;
  Bool_t found = kFALSE;
  s = "";
  N = fSourcePath.Length();
  k = N-1;
  while ((k>=0) && (!found)) {
    if (fSourcePath[k]=='/') found = kTRUE;
    else k--;
  }
  if (!found) gCleanOut->MM(fatal,"NameFromPath","Bad path name",ClassName());
  k++;
  while ((k<N) && (fSourcePath[k]!='_')) {
    s.Append(fSourcePath[k]);
    k++;
  }
}
void TLitSpontan::Periodicity(Long64_t period) const {
  // Change periodicity of messages of generation of photons
  gCleanOut->SetPeriod1(period);
}
void TLitSpontan::Print() const {
  //Prints everything about spontaneous source
  //
  gCleanOut->MM(info,"Print",fName.Data(),ClassName());
  gCleanOut->MM(info,"Print",fTitle.Data(),ClassName());
}
Bool_t TLitSpontan::SetEmission(KindOfDist kind,Double_t tmax,TVector3 dir,
  const char *fitname,Bool_t srcfix,TVector3 source,Bool_t emface,TVector3 dirfce) {
  // Calls fStartLitVol->SetEmission(). Look at this method
  //  Declares that the TGeoVolume associated with this TLitVolume is able to emit
  // photons. All points and vectors of this method are given in the local coordinate
  // system of the TGeoVolume associated with this TLitVolume.
  //  There are 3 possibilities for the point of emission of photons by the
  // TGeoVolume:
  //
  //  (1) -  photons are emitted from any point inside the TGeoVolume with an equal
  //         probability [default]
  //  (2) -  [if srcfix true]: all photons are emitted from point "source" inside
  //         the TGeoVolume
  //  (3) -  [if emface true and srcfix false]: first, a point is generated inside
  //         the TGeoVolume with an equal probability. Then the point is translated
  //         inside the TGeoVolume, along direction dirfce, until the edge of the
  //         TGeoVolume. It is the way SLitrani generates surface emission, without
  //         having [as the old SLitrani] any handling of faces. This method has the
  //         advantage of working, whatever the kind of TGeoVolume.
  //         
  //  Arguments:
  //
  // kind   : type of distribution for the direction of the emitted photons around dir
  //          [default on4pi]
  //
  //        on4pi        ==> photons are emitted isotropically, with the distribution
  //                      sin(theta)*dtheta*dphi, on 4 pi.
  //        flat         ==> photons are emitted isotropically, with the distribution
  //                      sin(theta)*dtheta*dphi,with theta between 0 and tmax
  //        sinuscosinus ==> photons are emitted with the non isotropic distribution
  //                      sin(theta)*cos(theta)*dtheta*dphi ,with theta between 0 and
  //                      tmax, favouring slightly the forward direction
  //        provided     ==> photons are emitted using the distribution provided
  //                      as a fit of type TSplineFit
  //
  // tmax   : maximum value for the theta angle around dir. tmax is given in degree
  //          [default 180.0]
  // dir    : axis around which photons are emitted, in local coordinate system of
  //          TGeoVolume [default (0,0,1)
  // fitname: in case kind == provided, name of fit to be used
  //          [default ""]
  // srcfix : if true, all photons are emitted from the fixed point source [ given in
  //          local coordinate system of TGeoVolume ] inside TGeoVolume
  //          [default false]
  // source : if srcfix true, fixed point within the TGeoVolume from which all photons are
  //          emitted. if srcfix false, irrelevant
  // emface : if true and srcfix false, photons are emitted from a face of the TGeoVolume
  //          according to possibility (3) described above.
  // dirfce : after having generated with equal probability a point anywhere inside the
  //          TGeoVolume, the point is translated along direction "dirfce" until the
  //          edge of the TGeoVolume.
  //
  Bool_t ok;
  ok = fStartLitVol->SetEmission(kind,tmax,dir,fitname,srcfix,source,emface,dirfce);
  return ok;
}
void TLitSpontan::SetFillDeath(TH1F *h1,TH1F *h2,TH1F *h3) {
  //  To be called once in order that the coordinates of the photons seen
  // be recorded into the provided histograms : 
  // - h1 for the x World Coordinate
  // - h2 for the y World Coordinate
  // - h3 for the z World Coordinate
  //  The creation and deletion of the histograms h1/2/3 has to be handled
  // by the user inside his CINT code. h1/2/3 must have been booked before
  // calling SetFillDeath
  //
  fFillDeath = kTRUE;
  fHX0 = h1;
  fHY0 = h2;
  fHZ0 = h3;
}
void TLitSpontan::SetWvlgth(Double_t wvlgth) {
  //  Changes the mode of generation of wavelength to "fixed at wvlgth"
  fWvlgthFixed = kTRUE;
  fWvlgth      = wvlgth;
}
void TLitSpontan::TrackToDraw(Int_t k) {
  //
  // (k <  0)                ==> no track to be drawn
  // (k >= 0) && (k<1000000) ==> track k is to be drawn
  // (k >= 1000000)          ==> all tracks to be drawn
  //
  fDrawCode = k;
}