#ifndef SteppingMessenger_h
#define SteppingMessenger_h 1

#include "G4UImessenger.hh"
#include "globals.hh"

class G4UIcmdWithAnInteger;
class SteppingAction;

class SteppingMessenger : public G4UImessenger {
public:
    SteppingMessenger(SteppingAction* stepping);
    virtual ~SteppingMessenger();

    virtual void SetNewValue(G4UIcommand*, G4String);

private:
    SteppingAction* fStepping;
    G4UIcmdWithAnInteger* fVerboseCmd;
};

#endif
