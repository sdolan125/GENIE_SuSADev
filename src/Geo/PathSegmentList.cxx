//____________________________________________________________________________
/*
 Copyright (c) 2009, GENIE Neutrino MC Generator Collaboration
 For the full text of the license visit http://copyright.genie-mc.org
 or see $GENIE/LICENSE

 Author: Robert Hatcher <rhatcher@fnal.gov>
         FNAL - May 26, 2009

 For the class documentation see the corresponding header file.

 Important revisions after version 2.0.0 :

*/
//____________________________________________________________________________

#include <fstream>
#include <iomanip>
#include <sstream>
#include <cassert>

//#include "libxml/parser.h"
//#include "libxml/xmlmemory.h"

#include <TLorentzVector.h>
#include <TGeoVolume.h>
#include <TGeoMaterial.h>

#include "Geo/PathSegmentList.h"
#include "Messenger/Messenger.h"
#include "PDG/PDGCodeList.h"
#include "PDG/PDGLibrary.h"
#include "Utils/PrintUtils.h"
#include "Utils/MathUtils.h"
//#include "Utils/XmlParserUtils.h"

using std::ofstream;
using std::setw;
using std::setfill;
using std::endl;

using namespace genie;

//____________________________________________________________________________
namespace genie {
 ostream & operator << (ostream & stream, const PathSegment & ps)
 {
   ps.Print(stream);
   return stream;
 }
}
//____________________________________________________________________________
namespace genie {
 ostream & operator << (ostream & stream, const PathSegmentList & list)
 {
   list.Print(stream);
   return stream;
 }
}

namespace genie {
  namespace pathsegutils {
    string Vec3AsString (const TVector3 * vec)
    {
      int w=17, p=10;  // precision setting only affects ostringstream
      std::ostringstream fmt;
      fmt << "(" << std::setw(w)   << std::setprecision(p) << vec->x()
          << "," << std::setw(w)   << std::setprecision(p) << vec->y()
          << "," << std::setw(w+1) << std::setprecision(p) << vec->z() << ")";
      return fmt.str();
    }
  }
}

//___________________________________________________________________________
PathSegment::PathSegment(void) :
  fRayDist(0), fStepLength(0), fStepTrimLow(0), fStepTrimHigh(0), 
  fVolume(0), fMedium(0), fMaterial(0),
  fEnter(), fExit()
{
}

void PathSegment::DoCrossCheck(const TVector3& startpos, 
                               double& ddist, double& dstep) const
{
  double dist_recalc = (fEnter-startpos).Mag();
  ddist = dist_recalc - fRayDist;

  double step_recalc = (fExit-fEnter).Mag();
  dstep = step_recalc - fStepLength;
}

void PathSegment::Print(ostream & stream) const
{
  const char* vname = (fVolume)   ? fVolume->GetName()   : "no volume";
  const char* mname = (fMaterial) ? fMaterial->GetName() : "no material";
  stream << genie::pathsegutils::Vec3AsString(&fEnter) << " " 
    //<< genie::pathsegutils::Vec3AsString(&fExit) 
         << " raydist " << std::setw(12) << fRayDist 
         << " step " << std::setw(12) << fStepLength << " "
         << "[" << std::setw(12) << fStepTrimLow << ":"
         << std::setw(12) << fStepTrimHigh << "] "
         << std::setw(16) << vname << " '"
         << std::setw(18) << mname << "'";
}

//===========================================================================
//___________________________________________________________________________
PathSegmentList::PathSegmentList(void)
  : fDoCrossCheck(false)
{

}
//___________________________________________________________________________
PathSegmentList::PathSegmentList(const PathSegmentList & plist)
{
  this->Copy(plist);
}
//__________________________________________________________________________
PathSegmentList::~PathSegmentList()
{

}

//___________________________________________________________________________
void PathSegmentList::SetAllToZero(void)
{
  LOG("PathS", pDEBUG) << "SetAllToZero called";

  this->fStartPos.SetXYZ(0,0,1e37); // clear cache of position/direction
  this->fDirection.SetXYZ(0,0,0);   //
  this->fSegmentList.clear();       // clear the vector
  this->fMatStepSum.clear();        // clear the re-factorized info
}

//___________________________________________________________________________
void PathSegmentList::SetStartInfo(const TVector3& pos, const TVector3& dir)
{
  this->fStartPos  = pos;
  this->fDirection = dir;
}

//___________________________________________________________________________
bool PathSegmentList::IsSameStart(const TVector3& pos, const TVector3& dir) const
{
  return ( this->fStartPos == pos && this->fDirection == dir );
}

//___________________________________________________________________________
void PathSegmentList::FillMatStepSum(void) 
{
  fMatStepSum.clear();

  PathSegmentList::PathSegVCItr_t sitr = fSegmentList.begin();
  PathSegmentList::PathSegVCItr_t sitr_end = fSegmentList.end();
  for ( ; sitr != sitr_end ; ++sitr ) {
    const PathSegment& ps = *sitr;
    const TGeoMaterial* mat  = ps.fMaterial;
    // use the post-trim limits on how much material is stepped through
    double              step = ( ps.fStepTrimHigh - ps.fStepTrimLow );
    fMatStepSum[mat] += step;
  }

}

//___________________________________________________________________________
void PathSegmentList::Copy(const PathSegmentList & plist)
{
  fSegmentList.clear();
  fMatStepSum.clear();

  // copy the segments
  //vector<PathSegment>::const_iterator pl_iter;
  //for (pl_iter = plist.fSegmentList.begin(); pl_iter != plist.fSegmentList.end(); ++pl_iter) {
  //  this->fSegmentList.push_back( *pl_iter );
  //}

  // other elements
  fStartPos     = plist.fStartPos;
  fDirection    = plist.fDirection;
  fSegmentList  = plist.fSegmentList;
  fMatStepSum   = plist.fMatStepSum;
  fDoCrossCheck = plist.fDoCrossCheck;

}

//___________________________________________________________________________
void PathSegmentList::CrossCheck(double& mxddist, double& mxdstep) const
{

  double dstep, ddist;
  mxdstep = 0;
  mxddist = 0;
  PathSegmentList::PathSegVCItr_t sitr = fSegmentList.begin();
  PathSegmentList::PathSegVCItr_t sitr_end = fSegmentList.end();
  for ( ; sitr != sitr_end ; ++sitr ) {
    const PathSegment& ps = *sitr;
    ps.DoCrossCheck(fStartPos,ddist,dstep);
    double addist = TMath::Abs(ddist);
    double adstep = TMath::Abs(dstep);
    if ( addist > mxddist ) mxddist = addist;
    if ( adstep > mxdstep ) mxdstep = adstep;
  }

}

//___________________________________________________________________________
void PathSegmentList::Print(ostream & stream) const
{
  stream << "\nPathSegmentList [-]" << endl;
  stream << "          start " << pathsegutils::Vec3AsString(&fStartPos)
         << " dir " << pathsegutils::Vec3AsString(&fDirection) << endl;

  double dstep, ddist, mxdstep = 0, mxddist = 0;
  int k = 0; 
  PathSegmentList::PathSegVCItr_t sitr = fSegmentList.begin();
  PathSegmentList::PathSegVCItr_t sitr_end = fSegmentList.end();
  for ( ; sitr != sitr_end ; ++sitr, ++k ) {
    const PathSegment& ps = *sitr;
    stream << "[" << setw(4) << k << "] " << ps;
    if ( fDoCrossCheck ) {
      ps.DoCrossCheck(fStartPos,ddist,dstep);
      double addist = TMath::Abs(ddist);
      double adstep = TMath::Abs(dstep);
      if ( addist > mxddist ) mxddist = addist;
      if ( adstep > mxdstep ) mxdstep = adstep;
      stream << " recalc diff" 
             << " dist " << std::setw(12) << ddist
             << " step " << std::setw(12) << dstep;
        }
    stream << std::endl;
  }

  if ( fDoCrossCheck )
    stream << "PathSegmentList " 
           << " mxddist " << mxddist 
           << " mxdstep " << mxdstep 
           << endl;
}
//___________________________________________________________________________
#ifdef PATH_SEGMENT_SUPPORT_XML
XmlParserStatus_t PathSegmentList::LoadFromXml(string filename)
{
  this->clear();
  PDGLibrary * pdglib = PDGLibrary::Instance();

  LOG("PathS", pINFO)
               << "Loading PathSegmentList from XML file: " << filename;

  xmlDocPtr xml_doc = xmlParseFile(filename.c_str() );

  if(xml_doc==NULL) {
    LOG("PathS", pERROR)
           << "XML file could not be parsed! [filename: " << filename << "]";
    return kXmlNotParsed;
  }

  xmlNodePtr xmlCur = xmlDocGetRootElement(xml_doc);

  if(xmlCur==NULL) {
    LOG("PathS", pERROR)
        << "XML doc. has null root element! [filename: " << filename << "]";
    return kXmlEmpty;
  }

  if( xmlStrcmp(xmlCur->name, (const xmlChar *) "path_length_list") ) {
    LOG("PathS", pERROR)
     << "XML doc. has invalid root element! [filename: " << filename << "]";
    return kXmlInvalidRoot;
  }

  LOG("PathS", pINFO) << "XML file was successfully parsed";

  xmlCur = xmlCur->xmlChildrenNode; // <path_length>'s

  // loop over all xml tree nodes that are children of the root node
  while (xmlCur != NULL) {

    // enter everytime you find a <path_length> tag
    if( (!xmlStrcmp(xmlCur->name, (const xmlChar *) "path_length")) ) {

       xmlNodePtr xmlPlVal = xmlCur->xmlChildrenNode;

       string spdgc = utils::str::TrimSpaces(
                         XmlParserUtils::GetAttribute(xmlCur, "pdgc"));

       string spl = XmlParserUtils::TrimSpaces(
                           xmlNodeListGetString(xml_doc, xmlPlVal, 1));

       LOG("PathS", pDEBUG) << "pdgc = " << spdgc << " --> pl = " << spl;

       int    pdgc = atoi( spdgc.c_str() );
       double pl   = atof( spl.c_str()   );

       TParticlePDG * p = pdglib->Find(pdgc);
       if(!p) {
         LOG("PathS", pERROR)
            << "No particle with pdgc " << pdgc
                             << " found. Will not load its path length";
       } else
            this->insert( map<int, double>::value_type(pdgc, pl) );

       xmlFree(xmlPlVal);
    }
       xmlCur = xmlCur->next;
  } // [end of] loop over tags within root elements

  xmlFree(xmlCur);
  return kXmlOK;
}
//___________________________________________________________________________
void PathSegmentList::SaveAsXml(string filename) const
{
//! Save path length list to XML file

  LOG("PathS", pINFO)
          << "Saving PathSegmentList as XML in file: " << filename;

  ofstream outxml(filename.c_str());
  if(!outxml.is_open()) {
    LOG("PathS", pERROR) << "Couldn't create file = " << filename;
    return;
  }
  outxml << "<?xml version=\"1.0\" encoding=\"ISO-8859-1\"?>";
  outxml << endl << endl;
  outxml << "<!-- generated by PathSegmentList::SaveAsXml() -->";
  outxml << endl << endl;

  outxml << "<path_length_list>" << endl << endl;

  PathSegmentList::const_iterator pl_iter;

  for(pl_iter = this->begin(); pl_iter != this->end(); ++pl_iter) {

    int    pdgc = pl_iter->first;
    double pl   = pl_iter->second; // path length

    outxml << "   <path_length pdgc=\"" << pdgc << "\">"
                                 << pl << "</path_length>" << endl;
  }
  outxml << "</path_length_list>";
  outxml << endl;

  outxml.close();

}
#endif
//___________________________________________________________________________
PathSegmentList & PathSegmentList::operator = (const PathSegmentList & list)
{
  this->Copy(list);
  return (*this);
}
//___________________________________________________________________________