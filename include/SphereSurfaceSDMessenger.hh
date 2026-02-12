#ifndef SphereSurfaceSDMessenger_h
#define SphereSurfaceSDMessenger_h 1

#include "G4UImessenger.hh"
#include "globals.hh"

class G4UIcmdWithAnInteger;
class SphereSurfaceSD;

class SphereSurfaceSDMessenger : public G4UImessenger {
public:
    SphereSurfaceSDMessenger(SphereSurfaceSD* sd);
    ~SphereSurfaceSDMessenger() override;

    void SetNewValue(G4UIcommand*, G4String) override;

private:
    SphereSurfaceSD* fDetector;
    G4UIcmdWithAnInteger* fVerboseCmd;
};

#endif
