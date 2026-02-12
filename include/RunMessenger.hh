#ifndef RunMessenger_h
#define RunMessenger_h 1

#include "G4UImessenger.hh"
#include "globals.hh"

class RunAction;
class G4UIcmdWithAnInteger;
class G4UIdirectory;

class RunMessenger : public G4UImessenger {
public:
    RunMessenger(RunAction*);
    ~RunMessenger();

    void SetNewValue(G4UIcommand*, G4String) override;

private:
    RunAction* fRunAction;
    G4UIcmdWithAnInteger* fVerboseCmd;
};

#endif
