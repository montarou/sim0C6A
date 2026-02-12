#include "SphereSurfaceSDMessenger.hh"
#include "SphereSurfaceSD.hh"
#include "G4UIcmdWithAnInteger.hh"
#include "G4UIdirectory.hh"

SphereSurfaceSDMessenger::SphereSurfaceSDMessenger(SphereSurfaceSD* sd)
: fDetector(sd)
{
    auto dir = new G4UIdirectory("/spheresd/");
    dir->SetGuidance("Contrôle de la verbosité de SphereSurfaceSD.");

    fVerboseCmd = new G4UIcmdWithAnInteger("/spheresd/verbose", this);
    fVerboseCmd->SetGuidance("Définit le niveau de verbosité (0-2)");
    fVerboseCmd->SetParameterName("verboseLevel", false);
    fVerboseCmd->SetRange("verboseLevel >= 0");
}

SphereSurfaceSDMessenger::~SphereSurfaceSDMessenger() {
    delete fVerboseCmd;
}

void SphereSurfaceSDMessenger::SetNewValue(G4UIcommand* command, G4String value) {
    if (command == fVerboseCmd) {
        fDetector->SetVerbose(fVerboseCmd->GetNewIntValue(value));
    }
}
