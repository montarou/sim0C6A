#include "MyTrackInfo.hh"

MyTrackInfo::MyTrackInfo()
: G4VUserTrackInformation(),
  enteredCube(false), enteredSphere(false), creatorProcess("unknown"),
  fComptonInCone(false), fNComptonInCone(0),
  fLastComptonPos(0., 0., 0.), fLastComptonEkin(0.)
{}

MyTrackInfo::~MyTrackInfo() {}

