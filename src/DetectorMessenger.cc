
#include "G4UIcmdWithADoubleAndUnit.hh"
#include "G4UIcmdWithAnInteger.hh"
#include "G4UIcmdWithABool.hh"
#include "G4UIdirectory.hh"

#include "DetectorConstruction.hh"
#include "DetectorMessenger.hh"

DetectorMessenger::DetectorMessenger(DetectorConstruction *det)
: G4UImessenger(),
fDetector(det) {

    fDirDetectorCmd = new G4UIdirectory("/detector/");
    fDirDetectorCmd->SetGuidance("Change Geometry of Detector...");

    fisPetriBoxcmd = new G4UIcmdWithABool("/detector/setPetriBox",this);
    fisPetriBoxcmd->SetGuidance("Set The PetriBox Geometry");
    fisPetriBoxcmd->AvailableForStates(G4State_PreInit,G4State_Idle);

    fisGDMLcmd = new G4UIcmdWithABool("/detector/setGDML",this);
    fisGDMLcmd->SetGuidance("Set The GDML Geometry");
    fisGDMLcmd->AvailableForStates(G4State_PreInit,G4State_Idle);

    fPosSourcecmd = new G4UIcmdWithADoubleAndUnit("/detector/SetPosSource",this);
    fPosSourcecmd ->SetGuidance("Set The Source Position");
    fPosSourcecmd ->AvailableForStates(G4State_PreInit,G4State_Idle);
}

DetectorMessenger::~DetectorMessenger(){
    delete fDirDetectorCmd;
    delete fPosSourcecmd;
    delete fisPetriBoxcmd;
    delete fisGDMLcmd;
}

void DetectorMessenger::SetNewValue(G4UIcommand* command,G4String newValue) {

    if( command == fisPetriBoxcmd ) {}
    if( command == fisGDMLcmd ) {
        G4bool isGDML = fisGDMLcmd->GetNewBoolValue(newValue);
        fDetector->SetGDML(isGDML);
    }


}
