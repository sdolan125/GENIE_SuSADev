//____________________________________________________________________________
/*!

\class    genie::SuSAv2MECPXSec

\brief    Computes the SuSAv2-MEC model differential cross section.
          Uses precomputed hadron tensor tables.
          Is a concrete implementation of the XSecAlgorithmI interface.

\author   Steven Gardiner <gardiner \at fnal.gov>
          Fermi National Acclerator Laboratory

\ref      G.D. Megias et al., "Meson-exchange currents and quasielastic
          predictions for charged-current neutrino-12C scattering in the
          superscaling approach," PRD 91 (2015) 073004

\created  November 2, 2018

\cpright  Copyright (c) 2003-2018, The GENIE Collaboration
          For the full text of the license visit http://copyright.genie-mc.org
          or see $GENIE/LICENSE
*/
//____________________________________________________________________________

#ifndef _SUSAV2_MEC_PXSEC_H_
#define _SUSAV2_MEC_PXSEC_H_

#include "Framework/EventGen/XSecAlgorithmI.h"
#include "Physics/HadronTensors/HadronTensorModelI.h"

namespace genie {

class XSecIntegratorI;

class SuSAv2MECPXSec : public XSecAlgorithmI {

public:

  SuSAv2MECPXSec();
  SuSAv2MECPXSec(string config);
  virtual ~SuSAv2MECPXSec();

  // XSecAlgorithmI interface implementation
  double XSec(const Interaction* i, KinePhaseSpace_t k) const;
  double Integral(const Interaction* i) const;
  bool   ValidProcess(const Interaction* i) const;

  // override the Algorithm::Configure methods to load configuration
  // data to private data members
  void Configure (const Registry & config);
  void Configure (string config);

  // Method specifically for evaluating nn/np pair probabilities
  double PairRatio(const Interaction* i) const;


private:

  /// Load algorithm configuration
  void LoadConfig (void);

  //debugging xsec draw
  void Scanq0q3 (void);
  void Scanq0q3_np (void);
  void Scanq0q3_electron (void);

  //for buildingRWhistograms
  void buildRWhistos (void);

  /// External scaling factor for this cross section
  double fXSecScale;

  /// Value of the CKM matrix element Vud to use when computing
  /// the differential cross section
  ///
  /// This factor is not included in the tabulated SuSAv2-MEC hadron
  /// tensor elements
  double fVud2;

  const genie::HadronTensorModelI* fHadronTensorModel;

  // Fermi momentum table used for scaling
  string fKFTable;

  // Binding energies: 
  double fEbHe;
  double fEbLi;
  double fEbC;
  double fEbO;
  double fEbMg;
  double fEbAr;
  double fEbCa;
  double fEbFe;
  double fEbNi;
  double fEbSn;
  double fEbAu;
  double fEbPb;

  /// GSL numerical integrator
  const XSecIntegratorI *  fXSecIntegrator;

};

} // genie namespace
#endif // _SUSAV2_MEC_PXSEC_H_