#ifndef MYTRACKINFO_HH
#define MYTRACKINFO_HH

#include "G4VUserTrackInformation.hh"
#include "G4ThreeVector.hh"
#include "globals.hh"

class MyTrackInfo : public G4VUserTrackInformation
{
public:
    MyTrackInfo();
    virtual ~MyTrackInfo();

    inline void SetEnteredCube(G4bool val) { enteredCube = val; }
    inline G4bool HasEnteredCube() const { return enteredCube; }

    void SetEnteredSphere(G4bool val) { enteredSphere = val; }
    G4bool HasEnteredSphere() const { return enteredSphere; }

    // Processus créateur
    void SetCreatorProcess(const G4String& name) { creatorProcess = name; }
    G4String GetCreatorProcess() const {
        return creatorProcess.empty() ? "unknown" : creatorProcess;
    }

    // ==================== Compton dans le cône graphite ====================
    // Flag : le primaire a-t-il subi au moins un Compton dans logicConeCompton ?
    void     SetComptonInCone(G4bool val)          { fComptonInCone = val; }
    G4bool   HasComptonInCone() const               { return fComptonInCone; }

    // Nombre total de diffusions Compton dans le cône pour ce track
    void     SetNComptonInCone(G4int n)             { fNComptonInCone = n; }
    G4int    GetNComptonInCone() const              { return fNComptonInCone; }
    void     IncrementNComptonInCone()              { fNComptonInCone++; }

    // Position (monde) de la dernière diffusion Compton dans le cône
    void            SetLastComptonPos(const G4ThreeVector& p) { fLastComptonPos = p; }
    G4ThreeVector   GetLastComptonPos() const                 { return fLastComptonPos; }

    // Énergie cinétique juste avant la dernière diffusion Compton dans le cône
    void     SetLastComptonEkin(G4double e)         { fLastComptonEkin = e; }
    G4double GetLastComptonEkin() const             { return fLastComptonEkin; }

private:
    G4bool enteredCube;
    G4bool enteredSphere;
    G4String creatorProcess;

    // Compton dans le cône
    G4bool        fComptonInCone;
    G4int         fNComptonInCone;
    G4ThreeVector fLastComptonPos;
    G4double      fLastComptonEkin;

};

#endif // MYTRACKINFO_HH
