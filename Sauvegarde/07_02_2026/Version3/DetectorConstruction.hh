#ifndef DETECTORCONSTRUCTION_HH
#define DETECTORCONSTRUCTION_HH


#include "G4VUserDetectorConstruction.hh"
#include "G4VPhysicalVolume.hh"
#include "G4LogicalVolume.hh"
#include "G4UnionSolid.hh"
#include "G4Box.hh"
#include "G4Tubs.hh"
#include "G4Sphere.hh"
#include "G4PVPlacement.hh"
#include "G4NistManager.hh"
#include "G4SDManager.hh"
#include "G4SystemOfUnits.hh"

#include "G4GenericMessenger.hh"
//#include "SensitiveDetector.hh"

class DetectorMessenger;

class DetectorConstruction : public G4VUserDetectorConstruction{
    public:
        DetectorConstruction();
        ~DetectorConstruction();

        G4LogicalVolume *GetScoringVolume() const {return fScoringVolume;}

        virtual G4VPhysicalVolume *Construct();

        void SetGDML(const G4bool isGDML );
        G4bool GetGDML() const { return fisGDML; }

        void ConstructGDML();

        void PrintAllMaterials();
        void PrintUsedMaterials();

        // =====================================================
        // NOUVEAU : Accesseurs pour le volume source (anode)
        // =====================================================
        G4LogicalVolume* GetAnodeLogicalVolume() const { return fLogicAnode; }
        G4VPhysicalVolume* GetAnodePhysicalVolume() const { return fPhysAnode; }
        
        // Retourne le solide de l'anode pour le test Inside()
        G4VSolid* GetAnodeSolid() const;
        
        // Retourne la boîte englobante de l'anode (en mm)
        void GetAnodeBoundingBox(G4double& xMin, G4double& xMax,
                                 G4double& yMin, G4double& yMax,
                                 G4double& zMin, G4double& zMax) const;

        // =====================================================
        // NOUVEAU : Accesseurs pour les couronnes d'eau
        // =====================================================
        static const G4int kNbWaterRings = 5;
        G4LogicalVolume* GetWaterRingLogicalVolume(G4int ringIndex) const;
        G4int GetNumberOfWaterRings() const { return kNbWaterRings; }

    private:
        void DefineMaterial();
        virtual void ConstructSDandField();
        void ConstructWaterRingsSystem();  // NOUVEAU : construction du système de couronnes
        //void AttachDNARegion();

        G4Box* solidWorld;
        G4Box* solidCube;
        G4Box* solidCube2;

        // Envellope Spherique
        G4Sphere* solidEnveloppe;
        G4LogicalVolume* logicEnveloppe = nullptr;
        G4VPhysicalVolume* physEnveloppe = nullptr;

        // Plan de Comptage
        G4Box* solidScorePlane = nullptr;;
        G4LogicalVolume* logicScorePlane = nullptr;
        G4VPhysicalVolume* physScorePlane = nullptr;

        // Cube de conversion et sphère supprimés

        G4Tubs* solidScintillator;
        G4Tubs* tubeCell;
        G4Tubs* solidWater;

        G4LogicalVolume* logicWorld = nullptr;
        G4LogicalVolume*  logicWaterBall = nullptr;
        G4LogicalVolume*  fScoringVolume = nullptr;

        G4VPhysicalVolume* physWorld = nullptr;

        // Sphère supprimée

        // =====================================================
        // NOUVEAU : Système de couronnes d'eau + conteneur PVC
        // Remplace ScorePlane4 à z = 68 mm
        // =====================================================
        // Conteneur PVC (ouvert côté source)
        G4Tubs* solidPVCBottom = nullptr;           // Fond du conteneur (1 mm)
        G4Tubs* solidPVCWall = nullptr;             // Parois latérales cylindriques
        G4LogicalVolume* logicPVCBottom = nullptr;
        G4LogicalVolume* logicPVCWall = nullptr;
        G4VPhysicalVolume* physPVCBottom = nullptr;
        G4VPhysicalVolume* physPVCWall = nullptr;
        
        // Couronnes d'eau concentriques (5 couronnes de 2 mm radial chacune)
        G4Tubs* solidWaterRing[kNbWaterRings];
        G4LogicalVolume* logicWaterRing[kNbWaterRings];
        G4VPhysicalVolume* physWaterRing[kNbWaterRings];

        G4Material* C8H8;
        G4Material* H2O;
        G4Material* MyAir;
        G4Material* MyVacuum;
        G4Material* MyWater;
        G4Material* MyPVC = nullptr;  // NOUVEAU : PVC pour le conteneur
        G4Material* MyGraphite = nullptr;  // CONE COMPTON : graphite pour le cône

        G4Element *C,*Na,*I;

        DetectorMessenger* fMessenger = nullptr;

        G4double xWorld, yWorld, zWorld;

        G4bool fisGDML;

        // =====================================================
        // NOUVEAU : Pointeurs vers le volume de l'anode tungstène
        // =====================================================
        G4LogicalVolume* fLogicAnode = nullptr;
        G4VPhysicalVolume* fPhysAnode = nullptr;

        //G4double fPosSource;
};
#endif
