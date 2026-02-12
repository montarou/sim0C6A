#ifndef SPHERE_HIT_HH
#define SPHERE_HIT_HH

#include "G4VHit.hh"
#include "G4THitsCollection.hh"
#include "G4Threading.hh"
#include "G4Allocator.hh"
#include "G4ThreeVector.hh"
#include "globals.hh"

// Hit élémentaire enregistré par SphereSurfaceSD
class SphereHit : public G4VHit {
public:
    SphereHit();
    virtual ~SphereHit();
    SphereHit(const SphereHit&);

    // operateurs
    const SphereHit& operator=(const SphereHit&);
    G4bool operator==(const SphereHit&) const;

    //  Identifiants de contexte
    void SetEventID(G4int id) { fEventID = id; };
    G4int GetEventID() const { return fEventID; };

    void SetTrackID(G4int track) {fTrackID = track;};
    G4int GetTrackID() const { return fTrackID;};

    // Géométrie & énergie
    void SetPosition(const G4ThreeVector& pos) { fPosition = pos; }
    G4ThreeVector GetPosition() const{ return fPosition;}

    void SetEnergy(G4double e) { fEnergy = e; }
    G4double GetEnergy() const { return fEnergy; }

    void SetEdep(G4double edep) {fEdep = edep;};
    G4double GetEdep() const{ return fEdep;}

    void SetTime(G4double time) {fTime = time;};
    G4double GetTime() const{ return fTime;}

    // Contexte particule & volume
    void SetParticleName(const G4String& name) {fParticleName = name;};
    G4String GetParticleName() const{ return fParticleName;}

    void SetVolumeName(const G4String& name) {fVolumeName = name;};
    G4String GetVolumeName() const{ return fVolumeName;}

    // Identifiant particule & processus du step ---
    void    SetPDG(G4int v)                 { fPDG = v; }
    G4int   GetPDG()                  const { return fPDG; }

    void          SetProcessName(const G4String& s)  { fProcessName = s; }
    const G4String& GetProcessName()           const { return fProcessName; }

    void    SetProcessType(G4int v)           { fProcessType = v; }   // enum G4ProcessType
    G4int   GetProcessType()            const { return fProcessType; }

    void    SetProcessSubType(G4int v)        { fProcessSubType = v; } // e.g. G4EmProcessSubType
    G4int   GetProcessSubType()         const { return fProcessSubType; }


    // Allocateur thread-local (convention Geant4)
    void* operator new(size_t);
    void operator delete(void*);

    // methode derivées de la classe de base
    virtual void Draw() override;
    virtual void Print() override;

private:

    G4int fEventID = -1;
    G4int fTrackID = -1;

    // Kinematics & space
    G4ThreeVector fPosition;    // mm
    G4double fEnergy;           // kinetic energy (MeV or internal units)
    G4double fTime;             // global time (ns)
    G4double fEdep;             // deposited energy (MeV or internal units)

    // Context
    G4String fParticleName;
    G4String fVolumeName;

    // Particle & process identifiers
    G4int         fPDG           = 0;      // PDG code (e.g. 11, 22, ...)
    G4String      fProcessName;            // e.g. "eIoni", "eBrem", "compt"
    G4int         fProcessType   = -1;     // enum G4ProcessType
    G4int         fProcessSubType= -1;     // e.g. G4EmProcessSubType
};

using SphereHitsCollection = G4THitsCollection<SphereHit>;

extern G4ThreadLocal G4Allocator<SphereHit>* SphereHitAllocator;

inline void* SphereHit::operator new(size_t){
    if(!SphereHitAllocator) SphereHitAllocator = new G4Allocator<SphereHit>;
    return SphereHitAllocator->MallocSingle();
}

inline void SphereHit::operator delete(void* hit){
    SphereHitAllocator->FreeSingle((SphereHit*) hit);
}

#endif // SPHERE_HIT_HH
