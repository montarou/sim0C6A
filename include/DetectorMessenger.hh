#ifndef DETECTORMESSENGER_HH
#define DETECTORMESSENGER_HH

#include "G4UImessenger.hh"

class DetectorConstruction;

class G4UIdirectory;
class G4UIcmdWithAnInteger;
class G4UIcmdWithADoubleAndUnit;
class G4UIcmdWithABool;

class DetectorMessenger : public G4UImessenger {

public:
    DetectorMessenger(DetectorConstruction *det);
    ~ DetectorMessenger() override;

    void SetNewValue(G4UIcommand*,G4String par) override;

private:
    DetectorConstruction* fDetector;

    G4UIdirectory* fDirDetectorCmd;

    G4UIcmdWithABool* fisPetriBoxcmd;
    G4UIcmdWithABool* fisGDMLcmd;
    G4UIcmdWithADoubleAndUnit* fPosSourcecmd;
};
#endif
