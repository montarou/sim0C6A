#include "SphereHit.hh"

#include "G4VVisManager.hh"

#include "G4VisAttributes.hh"
#include "G4Circle.hh"
#include "G4Colour.hh"

#include "G4UnitsTable.hh"
#include "G4ios.hh"

G4ThreadLocal G4Allocator<SphereHit>* SphereHitAllocator=nullptr;

SphereHit::SphereHit()
: G4VHit(),fPosition(G4ThreeVector()),fEnergy(0.),fEdep(0.),fTime(0.),fParticleName(""),fVolumeName(""),fTrackID(-1) {}

SphereHit::~SphereHit() {}

SphereHit::SphereHit(const SphereHit& right): G4VHit(right),fEdep(right.fEdep),fPosition(right.fPosition),fTime(right.fTime), fParticleName(right.fParticleName),fVolumeName(right.fVolumeName),fTrackID(right.fTrackID) {}

const SphereHit& SphereHit::operator=(const SphereHit& right) {
    fPosition = right.fPosition;
    fEnergy = right.fEnergy;
    fEdep = right.fEdep;
    fTime = right.fTime;
    fParticleName = right.fParticleName;
    fVolumeName = right.fVolumeName;
    fTrackID = right.fTrackID;

    return *this;
}

G4bool SphereHit::operator==(const SphereHit& right) const {
    return (this == &right);
}

void SphereHit::Draw() {

    // visualiser le hit
    G4VVisManager* vis = G4VVisManager::GetConcreteInstance();
    if(vis) {
        G4Circle circle(fPosition);
        circle.SetScreenSize(0.05);
        circle.SetFillStyle(G4Circle::filled);
        circle.SetVisAttributes(G4VisAttributes(G4Colour(1,0,0))); //rouge
        vis->Draw(circle);
    }
}

void SphereHit::Print() {
    G4cout << "EventID = " << fEventID
    << ", Hit: position = " << G4BestUnit(fPosition, "Length")
    << ", énergie déposée = " << G4BestUnit(fEdep, "Energy")
    << ", time = " << fTime/CLHEP::ns
    << ", ns, particle = " << fParticleName
    << ", volume = " << fVolumeName<<G4endl;
}
