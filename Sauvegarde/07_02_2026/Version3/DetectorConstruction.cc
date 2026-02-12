#include "DetectorConstruction.hh"

#include "G4ProductionCuts.hh"
#include "G4Region.hh"
#include "G4RegionStore.hh"

#include "G4NistManager.hh"
#include "G4RunManager.hh"
#include "G4SDManager.hh"

#include "G4VisAttributes.hh"
#include "G4Colour.hh"

#include "DetectorMessenger.hh"

#include "G4SystemOfUnits.hh"

#include "G4GDMLParser.hh"
#include "G4Region.hh"
#include "G4RegionStore.hh"

// SphereSurfaceSD.hh supprimé (sphère supprimée)
#include "SurfaceSpectrumSD.hh"
#include "ScorePlane2SD.hh"
#include "ScorePlane3SD.hh"
#include "ScorePlane4SD.hh"
#include "ScorePlane5SD.hh"
// ScorePlane6SD supprimé

#include "G4AnalysisManager.hh"
#include "G4UserLimits.hh"

#include "G4LogicalVolumeStore.hh"
#include "G4Material.hh"
#include <set>
#include <string>

// NOUVEAU : Pour accéder au solide et calculer la bounding box
#include "G4VSolid.hh"
#include "G4VisExtent.hh"

// CONE COMPTON : pour le collimateur conique paramétrique
#include "G4Cons.hh"


#define CHECK_MAT(mat) \
if (!(mat)) G4Exception(__FUNCTION__, "MAT01", FatalException, "Matériau " #mat " non trouvé !");

DetectorConstruction::DetectorConstruction()
{
        fisGDML         = true;

        fMessenger = new DetectorMessenger(this);

        DefineMaterial();

        xWorld = 0.5*m;
        yWorld = 0.5*m;
        zWorld = 0.5*m;
        
        // Initialisation des tableaux pour les couronnes d'eau
        for (G4int i = 0; i < kNbWaterRings; i++) {
                solidWaterRing[i] = nullptr;
                logicWaterRing[i] = nullptr;
                physWaterRing[i] = nullptr;
        }
}

DetectorConstruction::~DetectorConstruction()
{
       delete fMessenger ;
}

void DetectorConstruction::SetGDML(const G4bool isGDML ){
        fisGDML   = isGDML;
        G4RunManager::GetRunManager()->ReinitializeGeometry();
}

void DetectorConstruction::DefineMaterial()
{
        G4NistManager *nist = G4NistManager::Instance();

        C8H8 = nist->FindOrBuildMaterial("G4_POLYSTYRENE");

       // Création du matériau N₂ (diazote gazeux)
        G4Element* N = nist->FindOrBuildElement("N");
        G4double density = 1.1848e-3 * g/cm3;
        G4int ncomponents = 1;
        G4Material* Dinitrogen = new G4Material("Dinitrogen", density, ncomponents, kStateGas);
        Dinitrogen->AddElement(N, 2);  // N₂

        // Création du dioxygène (O₂)
        // Récupère l'élément oxygène
        G4Element* O = nist->FindOrBuildElement("O");
        G4Material* Dioxygen = new G4Material("Dioxygen", 1.354e-3 * g/cm3, 1, kStateGas);
        Dioxygen->AddElement(O, 2);
        Dioxygen->SetChemicalFormula("O₂");

        // Réutilise le même élément O
        G4Material* Ozone = new G4Material("Ozone", 2.018e-3 * g/cm3, 1, kStateGas);
        Ozone->AddElement(O, 3);
        Ozone->SetChemicalFormula("O₃");

        G4Material* Argon            = nist->FindOrBuildMaterial("G4_Ar");CHECK_MAT(Argon);

        // Création du CarbonDioxide
        G4Element* C = nist->FindOrBuildElement("C");
        G4Material* CarbonDioxide = new G4Material("CarbonDioxide", 1.977e-3 * g/cm3, 2, kStateGas);
        CarbonDioxide->AddElement(C, 1);
        CarbonDioxide->AddElement(O, 2);
        CarbonDioxide->SetChemicalFormula("CO₂");

        G4Material* Neon             = nist->FindOrBuildMaterial("G4_Ne");CHECK_MAT(Neon);
        G4Material* Helium           = nist->FindOrBuildMaterial("G4_He");CHECK_MAT(Helium);

        // Création du Methane
        G4Element* H = nist->FindOrBuildElement("H");
        G4Material* Methane = new G4Material("Methane", 6.656e-4 * g/cm3, 2, kStateGas);
        Methane->AddElement(C, 1);
        Methane->AddElement(H, 4);
        Methane->SetChemicalFormula("CH₄");

        G4Material* Krypton          = nist->FindOrBuildMaterial("G4_Kr");CHECK_MAT(Krypton);
        G4Material* Dihydrogen       = nist->FindOrBuildMaterial("G4_H");CHECK_MAT(Dihydrogen);

        // Création du NitricOxide
        G4Material* NitricOxide = new G4Material("NitricOxide", 1.340e-3 * g/cm3, 2, kStateGas);
        NitricOxide->AddElement(N, 1);
        NitricOxide->AddElement(O, 1);
        NitricOxide->SetChemicalFormula("NO");

        // Création du NitrousOxide
        G4Material* NitrousOxide = new G4Material("NitrousOxide", 1.977e-3 * g/cm3, 2, kStateGas);
        NitrousOxide->AddElement(N, 2);
        NitrousOxide->AddElement(O, 1);
        NitrousOxide->SetChemicalFormula("N₂O");

        // Création du CarbonMonoxide
        G4Material* CarbonMonoxide = new G4Material("CarbonMonoxide", 1.250e-3 * g/cm3, 2, kStateGas);
        CarbonMonoxide->AddElement(C, 1);
        CarbonMonoxide->AddElement(O, 1);
        CarbonMonoxide->SetChemicalFormula("CO");

        G4Material* Xenon            = nist->FindOrBuildMaterial("G4_Xe");CHECK_MAT(Xenon);

        // Création du matériau NO₂
        G4Material* NitrogenDioxide = new G4Material("NitrogenDioxide", 3.2095e-3 * g/cm3, 2, kStateGas);
        NitrogenDioxide->AddElement(N, 1);  // 1 atome d'azote
        NitrogenDioxide->AddElement(O, 2);  // 2 atomes d'oxygène
        NitrogenDioxide->SetChemicalFormula("NO₂");

         // Création du Diodine
        G4Element* I = nist->FindOrBuildElement("I");
        G4Material* Diiodine = new G4Material("Diiodine", 4.93e-3 * g/cm3, 1, kStateGas);
        Diiodine->AddElement(I, 2);
        Diiodine->SetChemicalFormula("I₂");

        G4Material* Water= nist->FindOrBuildMaterial("G4_WATER");CHECK_MAT(Water);

        // Définition du milieux aqueux("MyWater")
        MyWater = new G4Material("H2O", 1.0000*g/cm3, 2);
        MyWater->AddElement(nist->FindOrBuildElement("H"),2);
        MyWater->AddElement(nist->FindOrBuildElement("O"),1);

        // Définition du vide ultra-haut ("MyVacuum") basé sur la composition de l'air raréfié
        MyVacuum = new G4Material("MyVacuum", 1.612e-12 * g/cm3, 17, kStateGas, 2.73 * kelvin, 1.0e-9 * pascal);

        MyVacuum->AddMaterial(Dinitrogen,       0.78084);
        MyVacuum->AddMaterial(Dioxygen,         0.20946);
        MyVacuum->AddMaterial(Argon,            0.00934);
        MyVacuum->AddMaterial(CarbonDioxide,    0.000417);
        MyVacuum->AddMaterial(Neon,             0.00001818);
        MyVacuum->AddMaterial(Helium,           0.00000524);
        MyVacuum->AddMaterial(Methane,          0.0000001745);
        MyVacuum->AddMaterial(Krypton,          0.000000114);
        MyVacuum->AddMaterial(Dihydrogen,       0.000000055);
        MyVacuum->AddMaterial(NitricOxide,      0.00000005);
        MyVacuum->AddMaterial(NitrousOxide,     0.00000003);
        MyVacuum->AddMaterial(CarbonMonoxide,   0.00000002);
        MyVacuum->AddMaterial(Xenon,            0.000000009);
        MyVacuum->AddMaterial(Ozone,            0.000000007);
        MyVacuum->AddMaterial(NitrogenDioxide,  0.000000002);
        MyVacuum->AddMaterial(Diiodine,         0.000000001);
        MyVacuum->AddMaterial(Water,          0.00040837357);

        // Définition de l'air complet
        MyAir = new G4Material("MyAir", 1.225e-3 * g/cm3, 17, kStateGas);
        MyAir->AddMaterial(Dinitrogen,       0.78084);
        MyAir->AddMaterial(Dioxygen,         0.20946);
        MyAir->AddMaterial(Argon,            0.00934);
        MyAir->AddMaterial(CarbonDioxide,    0.000417);
        MyAir->AddMaterial(Neon,             0.00001818);
        MyAir->AddMaterial(Helium,           0.00000524);
        MyAir->AddMaterial(Methane,          0.0000001745);
        MyAir->AddMaterial(Krypton,          0.000000114);
        MyAir->AddMaterial(Dihydrogen,       0.000000055);
        MyAir->AddMaterial(NitricOxide,      0.00000005);
        MyAir->AddMaterial(NitrousOxide,     0.00000003);
        MyAir->AddMaterial(CarbonMonoxide,   0.00000002);
        MyAir->AddMaterial(Xenon,            0.000000009);
        MyAir->AddMaterial(Ozone,            0.000000007);
        MyAir->AddMaterial(NitrogenDioxide,  0.000000002);
        MyAir->AddMaterial(Diiodine,         0.000000001);
        MyAir->AddMaterial(Water,            0.00040837357);

        // =====================================================
        // NOUVEAU : PVC (Polyvinyl Chloride) pour le conteneur
        // Formule : (C2H3Cl)n, densité ~ 1.4 g/cm³
        // =====================================================
        MyPVC = nist->FindOrBuildMaterial("G4_POLYVINYL_CHLORIDE");
        CHECK_MAT(MyPVC);
        G4cout << "[MATERIAL] PVC loaded: " << MyPVC->GetName() 
               << ", density = " << MyPVC->GetDensity()/(g/cm3) << " g/cm3" << G4endl;

        // =====================================================
        // CONE COMPTON : Graphite (carbone pur, densité 2.26 g/cm³)
        // Candidat optimal pour lentille Compton :
        //   - Z=6 → ratio Compton/photoélectrique élevé
        //   - ρ=2.26 → bonne probabilité d'interaction
        //   - Tenue thermique et mécanique excellente
        // Alternative : G4_PLEXIGLASS (PMMA, ρ=1.19) ou G4_POLYETHYLENE (ρ=0.94)
        // =====================================================
        MyGraphite = nist->FindOrBuildMaterial("G4_GRAPHITE");
        CHECK_MAT(MyGraphite);
        G4cout << "[MATERIAL] Graphite loaded: " << MyGraphite->GetName() 
               << ", density = " << MyGraphite->GetDensity()/(g/cm3) << " g/cm3" << G4endl;

}

void DetectorConstruction::ConstructGDML()
{

        solidWorld = new G4Box("solidWorld", xWorld, yWorld, zWorld);
        logicWorld = new G4LogicalVolume(solidWorld, MyAir, "logicWorld");
        physWorld = new G4PVPlacement(nullptr, {}, logicWorld, "physWorld", nullptr, false, 0, true);

        // Couleur pour visualisation
        G4VisAttributes* visAttr1= new G4VisAttributes(G4Colour(1, 0, 1, 0.2));
        visAttr1->SetVisibility(true);
        visAttr1->SetForceSolid(true);
        logicWorld->SetVisAttributes(visAttr1);

        // Chargement du Parser
        G4GDMLParser parser;

        // --- Enveloppe cubique centrée en (0,0,0), matériau : air ---
        const G4double hx = 5.0*cm;    // demi-dimension X = 50 mm
        const G4double hy = 5.0*cm;    // demi-dimension Y = 50 mm
        const G4double hz = 6.0*cm;    // demi-dimension Z = 60 mm

        auto solidEnveloppe  = new G4Box("solidEnveloppeGDML", hx, hy, hz);
        auto logicEnveloppe  = new G4LogicalVolume(solidEnveloppe, MyAir, "logicEnveloppeGDML");
        physEnveloppe = new G4PVPlacement(nullptr,G4ThreeVector(0., 0., 0.),logicEnveloppe,"physEnveloppeGDML",
                          logicWorld,false,0,true);

        // Couleur pour visualisation
        G4VisAttributes* visAttrEnveloppe = new G4VisAttributes(G4Colour(1, 1, 0, 0.1)); // jaune très transparent
        visAttrEnveloppe->SetVisibility(true);
        visAttrEnveloppe->SetForceSolid(true);
        logicEnveloppe->SetVisAttributes(visAttrEnveloppe);


        // Plan de scoring à z = +2 mm (juste après le porte collimateur)

        // épaisseur ultra-fine du plan (à ajuster si nécessaire)
        const G4double tPlane = 1.0*um;

        // Solide/logique du plan
        solidScorePlane = new G4Box("solidScorePlane", hx, hy, tPlane/2.0);
        logicScorePlane = new G4LogicalVolume(solidScorePlane, MyAir, "logicScorePlane");

        // Placement à z = +18 mm
        physScorePlane = new G4PVPlacement(0,
                        G4ThreeVector(0., 0., +18.0*mm), // centre du plan
                        logicScorePlane,
                        "physScorePlane",
                        logicEnveloppe,   // parent = enveloppe
                        false,
                        0,
                        true);

        auto vis = new G4VisAttributes(G4Colour(1.,0.,0.,1.0)); // rouge opaque
        vis->SetForceSolid(true);
        logicScorePlane->SetVisAttributes(vis);

        // -------------------------------------------------------------------
        // Plan de comptage supplémentaire à z = +10 mm apres le premier plan
        // Parallélépipède : demi-dimensions x=5cm, y=5cm, z=1mm
        // -------------------------------------------------------------------
        const G4double hxPlane2 = 5.0*cm;   // demi-dimension X = 50 mm
        const G4double hyPlane2 = 5.0*cm;   // demi-dimension Y = 50 mm
        const G4double hzPlane2 = 0.5*mm;   // demi-dimension Z = 0.5 mm

        G4Box* solidScorePlane2 = new G4Box("solidScorePlane2", hxPlane2, hyPlane2, hzPlane2);
        G4LogicalVolume* logicScorePlane2 = new G4LogicalVolume(solidScorePlane2, MyAir, "logicScorePlane2");

        new G4PVPlacement(0,
                        G4ThreeVector(0., 0., +28.0*mm), // centre à z = 28 mm
                        logicScorePlane2,
                        "physScorePlane2",
                        logicEnveloppe,   // parent = enveloppe
                        false,
                        0,
                        true);

        // Même couleur rouge opaque que le plan de scoring
        auto vis2 = new G4VisAttributes(G4Colour(1.,0.,0.,1.0)); // rouge opaque
        vis2->SetForceSolid(true);
        logicScorePlane2->SetVisAttributes(vis2);


        // -------------------------------------------------------------------
        // Plans de comptage supplémentaires à z = 2cm, 4cm, 6cm, 8cm, 10cm apres le premier plan
        // Mêmes dimensions : demi-x=5cm, demi-y=5cm, demi-z=0.5mm
        // Note : les plans à cm, cm et 1cm dépassent l'enveloppe (±60mm)
        //        donc ils sont placés dans logicWorld
        // -------------------------------------------------------------------
        
        // Plan à z = 2 cm apres le premier plan de scoring (dans enveloppe)
        G4Box* solidScorePlane3 = new G4Box("solidScorePlane3", hxPlane2, hyPlane2, hzPlane2);
        G4LogicalVolume* logicScorePlane3 = new G4LogicalVolume(solidScorePlane3, MyAir, "logicScorePlane3");
        new G4PVPlacement(0, G4ThreeVector(0., 0., +38.0*mm), logicScorePlane3,
                          "physScorePlane3", logicEnveloppe, false, 0, true);
        logicScorePlane3->SetVisAttributes(vis2);

        // ==========================================================================
        // NOUVEAU : Système de couronnes d'eau concentriques + conteneur PVC
        // Remplace ScorePlane4 à z = 68 mm
        // ==========================================================================
        // Géométrie :
        // - 5 couronnes d'eau concentriques, épaisseur radiale 2 mm chacune
        // - Couronne 0 (disque central) : r = 0-2 mm
        // - Couronne 1 : r = 2-4 mm, Couronne 2 : r = 4-6 mm
        // - Couronne 3 : r = 6-8 mm, Couronne 4 : r = 8-10 mm
        // - Épaisseur en Z de l'eau : 3 mm
        // - Conteneur PVC : parois 1 mm, hauteur 5 mm, ouvert côté source
        // - Fond de l'eau (surface z+) à z = 68 mm, au contact du fond PVC
        // ==========================================================================
        
        const G4double waterThickness = 3.0*mm;       // épaisseur eau en Z
        const G4double ringRadialThick = 2.0*mm;      // épaisseur radiale des couronnes
        const G4double waterRadius = 10.0*mm;         // rayon total = 5 × 2 mm
        const G4double pvcThickness = 1.0*mm;         // épaisseur parois PVC
        // const G4double containerHeight = 5.0*mm;      // SUPPRIMÉ - causait overlap
        const G4double containerWallHeight = waterThickness;  // CORRIGÉ: 3mm au lieu de 4mm
        
        // Position : fond de l'eau (surface z+) à z = 68 mm
        const G4double zWaterBackSurface = 68.0*mm;   // fond de l'eau
        const G4double zWaterFrontSurface = zWaterBackSurface - waterThickness; // 65 mm (côté source)
        const G4double zWaterCenter = zWaterBackSurface - waterThickness/2.0;   // 66.5 mm
        const G4double zPVCBottomCenter = zWaterBackSurface + pvcThickness/2.0; // 68.5 mm
        const G4double zPVCWallCenter = zWaterCenter;  // CORRIGÉ: 66.5mm au lieu de 67mm
        
        G4cout << "\n[GEOM] === Water Rings System ===" << G4endl;
        G4cout << "[GEOM] Water front surface z = " << zWaterFrontSurface/mm << " mm (côté source)" << G4endl;
        G4cout << "[GEOM] Water center z = " << zWaterCenter/mm << " mm" << G4endl;
        G4cout << "[GEOM] Water back surface z = " << zWaterBackSurface/mm << " mm (fond)" << G4endl;
        G4cout << "[GEOM] PVC bottom center z = " << zPVCBottomCenter/mm << " mm" << G4endl;
        
        // --- Fond du conteneur PVC (disque plein) ---
        solidPVCBottom = new G4Tubs("solidPVCBottom",
                                     0,                               // rayon interne = 0 (disque plein)
                                     waterRadius + pvcThickness,      // rayon externe = 11 mm
                                     pvcThickness/2.0,                // demi-épaisseur Z = 0.5 mm
                                     0, 360*deg);
        logicPVCBottom = new G4LogicalVolume(solidPVCBottom, MyPVC, "logicPVCBottom");
        physPVCBottom = new G4PVPlacement(0, 
                                          G4ThreeVector(0., 0., zPVCBottomCenter),
                                          logicPVCBottom,
                                          "physPVCBottom", 
                                          logicWorld, false, 0, true);
        
        // --- Parois latérales cylindriques du conteneur PVC ---
        solidPVCWall = new G4Tubs("solidPVCWall",
                                   waterRadius,                       // rayon interne = 10 mm
                                   waterRadius + pvcThickness,        // rayon externe = 11 mm
                                   containerWallHeight/2.0,           // demi-hauteur = 2 mm
                                   0, 360*deg);
        logicPVCWall = new G4LogicalVolume(solidPVCWall, MyPVC, "logicPVCWall");
        physPVCWall = new G4PVPlacement(0, 
                                        G4ThreeVector(0., 0., zPVCWallCenter),
                                        logicPVCWall,
                                        "physPVCWall", 
                                        logicWorld, false, 0, true);
        
        // Attributs visuels PVC (gris semi-transparent)
        auto visPVC = new G4VisAttributes(G4Colour(0.5, 0.5, 0.5, 0.7));
        visPVC->SetForceSolid(true);
        logicPVCBottom->SetVisAttributes(visPVC);
        logicPVCWall->SetVisAttributes(visPVC);
        
        // --- Couronnes d'eau concentriques ---
        // Palette de couleurs : dégradé de bleu (clair au centre, foncé à l'extérieur)
        const G4double blueShades[kNbWaterRings] = {0.5, 0.625, 0.75, 0.875, 1.0};
        
        for (G4int i = 0; i < kNbWaterRings; i++) {
            G4double rMin = i * ringRadialThick;
            G4double rMax = (i + 1) * ringRadialThick;
            
            G4String solidName = "solidWaterRing" + std::to_string(i);
            G4String logicName = "logicWaterRing" + std::to_string(i);
            G4String physName = "physWaterRing" + std::to_string(i);
            
            solidWaterRing[i] = new G4Tubs(solidName,
                                            rMin,                    // rayon interne
                                            rMax,                    // rayon externe
                                            waterThickness/2.0,      // demi-épaisseur Z = 1.5 mm
                                            0, 360*deg);
            
            logicWaterRing[i] = new G4LogicalVolume(solidWaterRing[i], MyWater, logicName);
            
            physWaterRing[i] = new G4PVPlacement(0,
                                                  G4ThreeVector(0., 0., zWaterCenter),
                                                  logicWaterRing[i],
                                                  physName,
                                                  logicWorld, false, 0, true);
            
            // Attributs visuels : dégradé de bleu
            auto visRing = new G4VisAttributes(G4Colour(0.0, 0.3, blueShades[i], 0.6));
            visRing->SetForceSolid(true);
            logicWaterRing[i]->SetVisAttributes(visRing);
            
            G4cout << "[GEOM] Water Ring " << i << ": r = [" << rMin/mm << ", " << rMax/mm << "] mm" << G4endl;
        }
        
        G4cout << "[GEOM] === End Water Rings System ===" << G4endl;
        // ==========================================================================
        // FIN du système de couronnes d'eau
        // ==========================================================================

        // Plan de comptage ScorePlane5 à z = 70 mm (juste après le conteneur PVC)
        G4Box* solidScorePlane5 = new G4Box("solidScorePlane5", hxPlane2, hyPlane2, hzPlane2);
        G4LogicalVolume* logicScorePlane5 = new G4LogicalVolume(solidScorePlane5, MyAir, "logicScorePlane5");
        new G4PVPlacement(0, G4ThreeVector(0., 0., +70*mm), logicScorePlane5,
                          "physScorePlane5", logicWorld, false, 0, true);
        logicScorePlane5->SetVisAttributes(vis2);

        // ScorePlane6 supprimé (était à z = +168 mm)
        // Cube de conversion et sphère de comptage supprimés définitivement

        // MiniX-EnveloppeTubeX-StainlessSteel304
        // Récupération du volume logique d'intérêt (MiniX-EnveloppeTubeX-StainlessSteel304)
        parser.Read("MiniX-EnveloppeTubeX-StainlessSteel304.gdml", false); // false -> pour ne pas valider le schema
        G4LogicalVolume* logicCollimator_2 = parser.GetVolume("MiniX-EnveloppeTubeX-StainlessSteel304");
        if (!logicCollimator_2) {
                G4Exception("MyDetectorConstruction::ConstructGDML", "GDML01", FatalException,
                            "Volume MiniX-EnveloppeTubeX-StainlessSteel304 non trouvé dans le fichier GDML.");
        }

        // Placer manuellement à une position arbitraire dans ton monde
        G4ThreeVector pos4(0, 0, 0*cm);
        new G4PVPlacement(nullptr, pos4, logicCollimator_2, "MiniX-EnveloppeTubeX-StainlessSteel304", logicEnveloppe, false, 0, true);

        // Couleur pour visualisation
        G4VisAttributes* visAttr4 = new G4VisAttributes(G4Colour(0, 0, 1, 1.0));
        visAttr4->SetVisibility(true);
        visAttr4->SetForceSolid(true);
        logicCollimator_2->SetVisAttributes(visAttr4);












        // MiniX-CollimateurSubAluminium-Aluminium
        // Récupération du volume logique d'intérêt (MiniX-CollimateurSubAluminium-Aluminium)

        /*
        // Cas du collimateur de 1 mm
        parser.Read("MiniX-CollimateurSubAluminium-Aluminium_1mm.gdml", false); // false -> pour ne pas valider le schema
        //G4LogicalVolume* logicCollimator_1 = parser.GetVolume("MiniX-CollimateurSubAluminium-Aluminium");
        G4LogicalVolume* logicCollimator_1 = parser.GetVolume("MiniX-CollimateurSubAluminium-Aluminium");
        if (!logicCollimator_1) {
                G4Exception("MyDetectorConstruction::ConstructGDML", "GDML01", FatalException,
                            "MiniX-CollimateurSubAluminium-Aluminium non trouvé dans le fichier GDML.");
        }

        // Placer dans logicEnveloppe (et non logicWorld) pour que les photons interagissent

       G4ThreeVector pos4_1mm(0, 0, 0);
        new G4PVPlacement(nullptr, pos4_1mm, logicCollimator_1, "MiniX-CollimateurSubAluminium-Aluminium", logicEnveloppe, false, 0, true);
        */

        /*
        // Cas du collimateur de 2 mm
        parser.Read("MiniX-CollimateurSubAluminium-Aluminium_2mm_rotX180.gdml", false); // false -> pour ne pas valider le schema
        G4LogicalVolume* logicCollimator_1 = parser.GetVolume("MiniX-Assembly-Collimation_2.0mm-CollimatorSubAluminium-Aluminium");
        if (!logicCollimator_1) {
                G4Exception("MyDetectorConstruction::ConstructGDML", "GDML01", FatalException,
                            "MiniX-CollimateurSubAluminium-Aluminium non trouvé dans le fichier GDML.");
        }

        // Placer dans logicEnveloppe (et non logicWorld) pour que les photons interagissent

        G4ThreeVector pos4_2mm(0, 0, 16.45*mm);
        new G4PVPlacement(nullptr, pos4_2mm, logicCollimator_1, "MiniX-Assembly-Collimation_2.0mm-CollimatorSubAluminium-Aluminium", logicEnveloppe, false, 0, true);
        */


        /*  ============================================================
         *  COMMENTÉ : Collimateur Al 3mm GDML (remplacé par cône graphite)
         *  ============================================================
        // Cas du collimateur de 3 mm
        parser.Read("MiniX-CollimateurSubAluminium-Aluminium_3mm_rotX180.gdml", false); // false -> pour ne pas valider le schema
        G4LogicalVolume* logicCollimator_1 = parser.GetVolume("MiniX-Assembly-Collimation_3.0mm-CollimatorSubAluminium-Aluminium");
        if (!logicCollimator_1) {
                G4Exception("MyDetectorConstruction::ConstructGDML", "GDML01", FatalException,
                            "MiniX-CollimateurSubAluminium-Aluminium non trouvé dans le fichier GDML.");
        }

        // Placer dans logicEnveloppe (et non logicWorld) pour que les photons interagissent

        G4ThreeVector pos4_3mm(0, 0, 16.45*mm);
        new G4PVPlacement(nullptr, pos4_3mm, logicCollimator_1, "MiniX-Assembly-Collimation_3.0mm-CollimatorSubAluminium-Aluminium", logicEnveloppe, false, 0, true);
         */

        // ==================================================================
        //  CONE CONCENTRATEUR COMPTON — Remplace Al + Laiton
        // ==================================================================
        //  PARAMÈTRES AJUSTABLES (modifier ici pour explorer d'autres configs)
        //
        //  Principe : le bore conique redirige les γ à grand angle par
        //  diffusion Compton dans un matériau bas-Z (graphite).
        //  À 10 keV, la perte d'énergie Compton est < 2% (régime Thomson).
        //
        //  Le cône s'inscrit dans le porte-collimateur Inox (R_int = 3.17 mm).
        //  R_outer est fixé à 3.15 mm pour laisser un jeu de 20 µm.
        // ==================================================================

        // --- Paramètres géométriques du cône ---
        const G4double coneRinEntry  = 3.00*mm;   // Rayon INTÉRIEUR côté source (large)
        const G4double coneRinExit   = 1.00*mm;   // Rayon INTÉRIEUR côté détecteur (étroit)
        const G4double coneRout      = 3.15*mm;   // Rayon EXTÉRIEUR (= R_int porte-collimateur)
        const G4double coneZfront    = 1.90*mm;    // z début (face côté source)
        const G4double coneZback     = 16.95*mm;   // z fin (face côté détecteur)

        // Calculs dérivés
        const G4double coneHalfLen   = (coneZback - coneZfront) / 2.0;  // demi-longueur
        const G4double coneZcenter   = (coneZfront + coneZback) / 2.0;  // centre en z
        const G4double coneHalfAngle = std::atan2(coneRinEntry - coneRinExit,
                                                   coneZback - coneZfront);

        // --- Matériau du cône ---
        // Choisir parmi : MyGraphite, ou remplacer par :
        //   nist->FindOrBuildMaterial("G4_PLEXIGLASS")   // PMMA, ρ=1.19
        //   nist->FindOrBuildMaterial("G4_POLYETHYLENE")  // PE, ρ=0.94
        //   nist->FindOrBuildMaterial("G4_BORON_CARBIDE") // B4C, ρ=2.52
        G4Material* coneMaterial = MyGraphite;

        // --- Construction du solide G4Cons ---
        //  G4Cons(name, Rmin1, Rmax1, Rmin2, Rmax2, Dz, SPhi, DPhi)
        //  "1" = face à -Dz (côté source),  "2" = face à +Dz (côté détecteur)
        auto solidCone = new G4Cons("solidConeCompton",
                                     coneRinEntry,   // Rmin1 (côté source, grand)
                                     coneRout,       // Rmax1 (côté source)
                                     coneRinExit,    // Rmin2 (côté détecteur, petit)
                                     coneRout,       // Rmax2 (côté détecteur)
                                     coneHalfLen,    // demi-longueur en z
                                     0.*deg,         // angle phi début
                                     360.*deg);      // angle phi fin (360° = complet)

        auto logicCone = new G4LogicalVolume(solidCone, coneMaterial, "logicConeCompton");

        // Placement dans l'enveloppe au centre z du cône
        new G4PVPlacement(nullptr,
                          G4ThreeVector(0., 0., coneZcenter),
                          logicCone,
                          "physConeCompton",
                          logicEnveloppe,
                          false, 0, true);

        // Visualisation : bleu semi-transparent
        auto visCone = new G4VisAttributes(G4Colour(0.2, 0.4, 0.8, 0.5));
        visCone->SetVisibility(true);
        visCone->SetForceSolid(true);
        logicCone->SetVisAttributes(visCone);

        // Log des paramètres
        G4cout << "\n[CONE COMPTON] === Configuration ===" << G4endl;
        G4cout << "[CONE COMPTON] Matériau      : " << coneMaterial->GetName()
               << " (ρ = " << coneMaterial->GetDensity()/(g/cm3) << " g/cm³)" << G4endl;
        G4cout << "[CONE COMPTON] R_in entrée   : " << coneRinEntry/mm << " mm (côté source)" << G4endl;
        G4cout << "[CONE COMPTON] R_in sortie   : " << coneRinExit/mm << " mm (côté détecteur)" << G4endl;
        G4cout << "[CONE COMPTON] R_extérieur   : " << coneRout/mm << " mm" << G4endl;
        G4cout << "[CONE COMPTON] z_début       : " << coneZfront/mm << " mm" << G4endl;
        G4cout << "[CONE COMPTON] z_fin         : " << coneZback/mm << " mm" << G4endl;
        G4cout << "[CONE COMPTON] Longueur      : " << 2*coneHalfLen/mm << " mm" << G4endl;
        G4cout << "[CONE COMPTON] Demi-angle    : " << coneHalfAngle/deg << "°" << G4endl;
        G4cout << "[CONE COMPTON] Épaisseur entrée : " << (coneRout - coneRinEntry)/mm << " mm" << G4endl;
        G4cout << "[CONE COMPTON] Épaisseur sortie : " << (coneRout - coneRinExit)/mm << " mm" << G4endl;
        G4cout << "[CONE COMPTON] ==========================\n" << G4endl;

        // Référence pour visAttr (pour éviter erreur de compilation plus bas)
        G4LogicalVolume* logicCollimator_1 = logicCone;


        /*
        // Cas du collimateur de 4 mm
        // Recuperation du volume logique d'interet (CollimateurAluminium-Aluminium)
        parser.Read("MiniX-CollimateurSubAluminium-Aluminium_4mm.gdml", false); // false -> pour ne pas valider le schema
        G4LogicalVolume* logicCollimator_1 = parser.GetVolume("CollimateurAluminium-Aluminium");
        if (!logicCollimator_1) {
                G4Exception("MyDetectorConstruction::ConstructGDML", "GDML01", FatalException,
                            "MiniX-CollimateurSubAluminium-Aluminium non trouvé dans le fichier GDML.");
        }

        // Placer dans logicEnveloppe (et non logicWorld) pour que les photons interagissent

        G4ThreeVector pos4_4mm(0, 0, 17.95*mm);
        new G4PVPlacement(nullptr, pos4_4mm, logicCollimator_1, "CollimateurAluminium-Aluminium", logicEnveloppe, false, 0, true);
        */

        /*
        // Cas du collimateur de 5 mm
        parser.Read("MiniX-CollimateurSubAluminium-Aluminium_5mm_rotX180_PMMA.gdml", false); // false -> pour ne pas valider le schema
        G4LogicalVolume* logicCollimator_1 = parser.GetVolume("MiniX-Assembly-Collimation_5.0mm-CollimatorSubAluminium-Aluminium");
        if (!logicCollimator_1) {
                G4Exception("MyDetectorConstruction::ConstructGDML", "GDML01", FatalException,
                            "MiniX-CollimateurSubAluminium-Aluminium non trouvé dans le fichier GDML.");
        }

        // Placer dans logicEnveloppe (et non logicWorld) pour que les photons interagissent

        G4ThreeVector pos4_3mm(0, 0, 16.45*mm);
        new G4PVPlacement(nullptr, pos4_3mm, logicCollimator_1, "MiniX-Assembly-Collimation_5.0mm-CollimatorSubAluminium-Aluminium", logicEnveloppe, false, 0, true);
         */

        // Couleur pour visualisation
        G4VisAttributes* visAttr5 = new G4VisAttributes(G4Colour(1, 0, 1, 0.3));
        visAttr5->SetVisibility(true);
        visAttr5->SetForceSolid(true);
        logicCollimator_1->SetVisAttributes(visAttr5);












        // MiniX-CollimateurSubLaiton-Brass

        /*
        // Cas du collimateur de 1 mm
        // Récupération du volume logique d'intérêt (MiniX-CollimateurSubLaiton-Brass)
        parser.Read("MiniX-CollimateurSubLaiton-Brass_1mm.gdml", false); // false -> pour ne pas valider le schema
        G4LogicalVolume* logicCollimator_3 = parser.GetVolume("MiniX-CollimateurSubLaiton-Brass");
        if (!logicCollimator_3) {
                G4Exception("MyDetectorConstruction::ConstructGDML", "GDML01", FatalException,
                            "MiniX-CollimateurSubLaiton-Brass non trouvé dans le fichier GDML.");
        }

        // Placer dans logicEnveloppe (et non logicWorld) pour que les photons interagissent
        new G4PVPlacement(nullptr, pos4_1mm, logicCollimator_3, "MiniX-CollimateurSubLaiton-Brass", logicEnveloppe, false, 0, true);
        */

        /*
        // Cas du collimateur de 2 mm
        // Récupération du volume logique d'intérêt (MiniX-Assembly-Collimation_2.0mm-CollimatorSubBrass-Brass)
        parser.Read("MiniX-CollimateurSubLaiton-Brass_2mm_rotX180.gdml", false); // false -> pour ne pas valider le schema
        G4LogicalVolume* logicCollimator_3 = parser.GetVolume("MiniX-Assembly-Collimation_2.0mm-CollimatorSubBrass-Brass");
        if (!logicCollimator_3) {
                G4Exception("MyDetectorConstruction::ConstructGDML", "GDML01", FatalException,
                            "MiniX-CollimateurSubLaiton-Brass non trouvé dans le fichier GDML.");
        }

        // Placer dans logicEnveloppe (et non logicWorld) pour que les photons interagissent
        new G4PVPlacement(nullptr, pos4_2mm, logicCollimator_3, "MiniX-Assembly-Collimation_2.0mm-CollimatorSubBrass-Brass", logicEnveloppe, false, 0, true);
        */



        /*  ============================================================
         *  COMMENTÉ : Collimateur Laiton 3mm GDML (remplacé par cône graphite)
         *  ============================================================
        // Cas du collimateur de 3 mm
        // Récupération du volume logique d'intérêt (MiniX-Assembly-Collimation_3.0mm-CollimatorSubBrass-Brass)
        parser.Read("MiniX-CollimateurSubLaiton-Brass_3mm_rotX180.gdml", false); // false -> pour ne pas valider le schema
        G4LogicalVolume* logicCollimator_3 = parser.GetVolume("MiniX-Assembly-Collimation_3.0mm-CollimatorSubBrass-Brass");
        if (!logicCollimator_3) {
                G4Exception("MyDetectorConstruction::ConstructGDML", "GDML01", FatalException,
                            "MiniX-CollimateurSubLaiton-Brass non trouvé dans le fichier GDML.");
        }

        // Placer dans logicEnveloppe (et non logicWorld) pour que les photons interagissent
        new G4PVPlacement(nullptr, pos4_3mm, logicCollimator_3, "MiniX-Assembly-Collimation_3.0mm-CollimatorSubBrass-Brass", logicEnveloppe, false, 0, true);
         */

        // Le cône graphite remplace aussi le laiton → pas besoin de logicCollimator_3 ici
        G4LogicalVolume* logicCollimator_3 = logicCone;  // alias pour la suite du code


        /*
        // Cas du collimateur de 4 mm
        // Récupération du volume logique d'intérêt (CollimateurLaiton-Brass)
        parser.Read("MiniX-CollimateurSubLaiton-Brass_4mm.gdml", false); // false -> pour ne pas valider le schema
        G4LogicalVolume* logicCollimator_3 = parser.GetVolume("CollimateurLaiton-Brass");
        if (!logicCollimator_3) {
                G4Exception("MyDetectorConstruction::ConstructGDML", "GDML01", FatalException,
                            "MiniX-CollimateurSubLaiton-Brass non trouvé dans le fichier GDML.");
        }

        // Placer dans logicEnveloppe (et non logicWorld) pour que les photons interagissent
        new G4PVPlacement(nullptr, pos4_4mm, logicCollimator_3, "CollimateurLaiton-Brass", logicEnveloppe, false, 0, true);


        // Couleur pour visualisation
        G4VisAttributes* visAttr6 = new G4VisAttributes(G4Colour(1, 0, 0, 0.3));
        visAttr6->SetVisibility(true);
        visAttr6->SetForceSolid(true);
        logicCollimator_3->SetVisAttributes(visAttr6);
        */

        /*
        // Cas du collimateur de 5 mm
        // Récupération du volume logique d'intérêt (MiniX-Assembly-Collimation_5.0mm-CollimatorSubBrass-Brass)
        parser.Read("MiniX-CollimateurSubLaiton-Brass_5mm_rotX180_PMMA.gdml", false); // false -> pour ne pas valider le schema
        G4LogicalVolume* logicCollimator_3 = parser.GetVolume("MiniX-Assembly-Collimation_5.0mm-CollimatorSubBrass-Brass");
        if (!logicCollimator_3) {
                G4Exception("MyDetectorConstruction::ConstructGDML", "GDML01", FatalException,
                            "MiniX-CollimateurSubLaiton-Brass non trouvé dans le fichier GDML.");
        }

        // Placer dans logicEnveloppe (et non logicWorld) pour que les photons interagissent
        new G4PVPlacement(nullptr, pos4_3mm, logicCollimator_3, "MiniX-Assembly-Collimation_5.0mm-CollimatorSubBrass-Brass", logicEnveloppe, false, 0, true);
        */

        // Couleur pour visualisation
        G4VisAttributes* visAttr6 = new G4VisAttributes(G4Colour(1, 0, 0, 0.3));
        visAttr6->SetVisibility(true);
        visAttr6->SetForceSolid(true);
        logicCollimator_3->SetVisAttributes(visAttr6);














        // MiniX-PorteCollimateur-StainlessSteel304
        // Récupération du volume logique d'intérêt ()
        parser.Read("MiniX-PorteCollimateur-StainlessSteel304.gdml", false); // false -> pour ne pas valider le schema
        G4LogicalVolume* logicCollimator_4 = parser.GetVolume("MiniX-PorteCollimateur-StainlessSteel304");
        if (!logicCollimator_4) {
                G4Exception("MyDetectorConstruction::ConstructGDML", "GDML01", FatalException,
                            "MiniX-PorteCollimateur-StainlessSteel304 non trouvé dans le fichier GDML.");
        }

        // Placer dans logicEnveloppe (et non logicWorld) pour que les photons interagissent
        new G4PVPlacement(nullptr, pos4, logicCollimator_4, "MiniX-PorteCollimateur-StainlessSteel304", logicEnveloppe, false, 0, true);

        // Couleur pour visualisation
        G4VisAttributes* visAttr7 = new G4VisAttributes(G4Colour(0, 1, 0, 0.1));
        visAttr7->SetVisibility(true);
        visAttr7->SetForceSolid(true);
        logicCollimator_4->SetVisAttributes(visAttr7);













        // MiniX-TubeXAlumine-DialuminiumTrioxide
        // Récupération du volume logique d'intérêt (MiniX-TubeXAlumine-DialuminiumTrioxide)
        parser.Read("MiniX-TubeXAlumine-DialuminiumTrioxide.gdml", false); // false -> pour ne pas valider le schema
        G4LogicalVolume* logicCollimator_5 = parser.GetVolume("MiniX-TubeXAlumine-DialuminiumTrioxide");
        if (!logicCollimator_5) {
                G4Exception("MyDetectorConstruction::ConstructGDML", "GDML01", FatalException,
                            "MiniX-TubeXAlumine-DialuminiumTrioxide non trouvé dans le fichier GDML.");
        }

        // Placer manuellement à une position arbitraire dans ton monde
        new G4PVPlacement(nullptr, pos4, logicCollimator_5, "MiniX-TubeXAlumine-DialuminiumTrioxide", logicEnveloppe, false, 0, true);

        // Couleur pour visualisation
        G4VisAttributes* visAttr8 = new G4VisAttributes(G4Colour(1, 1, 1, 0.));
        visAttr8->SetVisibility(true);
        visAttr8->SetForceSolid(true);
        logicCollimator_5->SetVisAttributes(visAttr8);









        // =====================================================
        // MODIFIÉ : MiniX-TubeXAnodeTungsten-Tungsten
        // On stocke les pointeurs pour l'utiliser comme source
        // =====================================================
        parser.Read("MiniX-TubeXAnodeTungsten-Tungsten.gdml", false);
        fLogicAnode = parser.GetVolume("MiniX-TubeXAnodeTungsten-Tungsten");
        if (!fLogicAnode) {
                G4Exception("MyDetectorConstruction::ConstructGDML", "GDML01", FatalException,
                            "MiniX-TubeXAnodeTungsten-Tungsten non trouvé dans le fichier GDML.");
        }

        // Placer et stocker le volume physique
        fPhysAnode = new G4PVPlacement(nullptr, pos4, fLogicAnode, 
                                       "MiniX-TubeXAnodeTungsten-Tungsten", 
                                       logicEnveloppe, false, 0, true);

        // Couleur pour visualisation (orange pour bien distinguer la source)
        G4VisAttributes* visAttr9 = new G4VisAttributes(G4Colour(1.0, 0.5, 0.0, 1.0));
        visAttr9->SetVisibility(true);
        visAttr9->SetForceSolid(true);
        fLogicAnode->SetVisAttributes(visAttr9);

        // Affichage des informations sur le volume source
        G4cout << "\n========== SOURCE VOLUMIQUE : ANODE TUNGSTÈNE ==========" << G4endl;
        G4cout << "Volume logique : " << fLogicAnode->GetName() << G4endl;
        G4cout << "Matériau       : " << fLogicAnode->GetMaterial()->GetName() << G4endl;
        
        // Afficher la bounding box
        G4double xMin, xMax, yMin, yMax, zMin, zMax;
        GetAnodeBoundingBox(xMin, xMax, yMin, yMax, zMin, zMax);
        G4cout << "Bounding box   :" << G4endl;
        G4cout << "  X : [" << xMin/mm << ", " << xMax/mm << "] mm" << G4endl;
        G4cout << "  Y : [" << yMin/mm << ", " << yMax/mm << "] mm" << G4endl;
        G4cout << "  Z : [" << zMin/mm << ", " << zMax/mm << "] mm" << G4endl;
        G4cout << "========================================================\n" << G4endl;








        // MiniX-TubeXContenuVideAnode_FenetreSortie-Vacuum
        // Récupération du volume logique d'intérêt (MiniX-TubeXContenuVideAnode_FenetreSortie-Vacuum)
        parser.Read("MiniX-TubeXContenuVideAnode_FenetreSortie-Vacuum.gdml", false); // false -> pour ne pas valider le schema
        G4LogicalVolume* logicCollimator_7 = parser.GetVolume("MiniX-TubeXContenuVideAnode_FenetreSortie-Vacuum");
        if (!logicCollimator_7) {
                G4Exception("MyDetectorConstruction::ConstructGDML", "GDML01", FatalException,
                            "MiniX-TubeXContenuVideAnode_FenetreSortie-Vacuum non trouvé dans le fichier GDML.");
        }

        // Placer manuellement à une position arbitraire dans ton monde
        new G4PVPlacement(nullptr, pos4, logicCollimator_7, "MiniX-TubeXContenuVideAnode_FenetreSortie-Vacuum", logicEnveloppe, false, 0, true);

        // Couleur pour visualisation
        G4VisAttributes* visAttr10 = new G4VisAttributes(G4Colour(1, 1, 1, 0.));
        visAttr10->SetVisibility(true);
        visAttr10->SetForceSolid(true);
        logicCollimator_7->SetVisAttributes(visAttr10);









        // MiniX-TubeXContenuVideCathode_Anode-Vacuum
        // Récupération du volume logique d'intérêt (MiniX-TubeXContenuVideCathode_Anode-Vacuum)
        parser.Read("MiniX-TubeXContenuVideCathode_Anode-Vacuum.gdml", false); // false -> pour ne pas valider le schema
        G4LogicalVolume* logicCollimator_8 = parser.GetVolume("MiniX-TubeXContenuVideCathode_Anode-Vacuum");
        if (!logicCollimator_8) {
                G4Exception("MyDetectorConstruction::ConstructGDML", "GDML01", FatalException,
                            "MiniX-TubeXContenuVideCathode_Anode-Vacuum non trouvé dans le fichier GDML.");
        }

        // Placer manuellement à une position arbitraire dans ton monde
        new G4PVPlacement(nullptr, pos4, logicCollimator_8, "MiniX-TubeXContenuVideCathode_Anode-Vacuum", logicEnveloppe, false, 0, true);

        // Couleur pour visualisation
        G4VisAttributes* visAttr11 = new G4VisAttributes(G4Colour(1, 1, 1, 0.));
        visAttr11->SetVisibility(true);
        visAttr11->SetForceSolid(true);
        logicCollimator_8->SetVisAttributes(visAttr11);












        // MiniX-TubeXFenetreBeryllium-Beryllium
        // Récupération du volume logique d'intérêt (MiniX-TubeXFenetreBeryllium-Beryllium)
        parser.Read("MiniX-TubeXFenetreBeryllium-Beryllium.gdml", false); // false -> pour ne pas valider le schema
        G4LogicalVolume* logicCollimator_9 = parser.GetVolume("MiniX-TubeXFenetreBeryllium-Beryllium");
        if (!logicCollimator_9) {
                G4Exception("MyDetectorConstruction::ConstructGDML", "GDML01", FatalException,
                            "MiniX-TubeXFenetreBeryllium-Beryllium non trouvé dans le fichier GDML.");
        }

        // Placer manuellement à une position arbitraire dans ton monde
        new G4PVPlacement(nullptr, pos4, logicCollimator_9, "MiniX-TubeXFenetreBeryllium-Beryllium", logicEnveloppe, false, 0, true);

        // Couleur pour visualisation
        G4VisAttributes* visAttr12 = new G4VisAttributes(G4Colour(1, 1, 0, 0.0));
        visAttr12->SetVisibility(true);
        visAttr12->SetForceSolid(true);
        logicCollimator_9->SetVisAttributes(visAttr12);

        // Appliquer des cuts à la région par défaut
        G4Region* defaultRegion = G4RegionStore::GetInstance()->GetRegion("DefaultRegionForTheWorld");

        if (defaultRegion) {
                G4ProductionCuts* cuts = new G4ProductionCuts();
                cuts->SetProductionCut(1.0*um, "gamma");
                cuts->SetProductionCut(1.0*um, "e-");
                cuts->SetProductionCut(1.0*um, "e+");
                cuts->SetProductionCut(1.0*um, "proton");

                defaultRegion->SetProductionCuts(cuts);

                G4cout << "[DetectorConstruction] Cuts appliqués : 1 µm pour gamma, e-, e+, proton" << G4endl;
}
}

// =====================================================
// NOUVEAU : Méthodes d'accès au volume de l'anode
// =====================================================

G4VSolid* DetectorConstruction::GetAnodeSolid() const
{
        if (fLogicAnode) {
                return fLogicAnode->GetSolid();
        }
        return nullptr;
}

void DetectorConstruction::GetAnodeBoundingBox(G4double& xMin, G4double& xMax,
                                               G4double& yMin, G4double& yMax,
                                               G4double& zMin, G4double& zMax) const
{
        // Valeurs par défaut si le volume n'existe pas
        xMin = -3.5*mm; xMax = 3.5*mm;
        yMin = -3.5*mm; yMax = 3.5*mm;
        zMin = -0.001*mm; zMax = 0.0*mm;

        if (fLogicAnode) {
                G4VSolid* solid = fLogicAnode->GetSolid();
                if (solid) {
                        // Utiliser GetExtent() pour obtenir la bounding box
                        G4VisExtent extent = solid->GetExtent();
                        xMin = extent.GetXmin();
                        xMax = extent.GetXmax();
                        yMin = extent.GetYmin();
                        yMax = extent.GetYmax();
                        zMin = extent.GetZmin();
                        zMax = extent.GetZmax();
                }
        }
}

G4VPhysicalVolume *DetectorConstruction::Construct()
{
        //solidWorld = new G4Box("solidWorld", xWorld, yWorld, zWorld);
        //logicWorld = new G4LogicalVolume(solidWorld, worldMat, "logicWorld");
        //physWorld = new G4PVPlacement(0, G4ThreeVector(0.,0.,0.),logicWorld,"physWord",0,false,0,true);

        if(fisGDML)
        {
                ConstructGDML();
                G4cout << "Construction GDML" << G4endl;
        }


        PrintUsedMaterials();

        // Affichage des volumes logiques et leurs matériaux
        G4cout << "\n=== Liste des volumes logiques et leurs matériaux ===" << G4endl;

        auto lvStore = G4LogicalVolumeStore::GetInstance();
        for (auto lv : *lvStore) {
                G4String volName = lv->GetName();
                G4Material* mat = lv->GetMaterial();
                G4String matName = (mat) ? mat->GetName() : "null";
                G4cout << " - " << volName << " : " << matName << G4endl;
        }

        G4cout << "=====================================================\n" << G4endl;

        // Définir une limite de step dans le Béryllium (épaisseur ≈ 0.145 mm → limite ≈ 0.0145 mm)
        //G4double stepLimitBe = 0.145 * mm / 20.0;
        G4UserLimits* userLimitsBe = new G4UserLimits(0.001*mm);

        // userLimitsSphere supprimé (sphère supprimée)

        // Attacher les limites aux volumes logiques si trouvés
        G4LogicalVolume* logicBe = G4LogicalVolumeStore::GetInstance()->GetVolume("MiniX-TubeXFenetreBeryllium-Beryllium");
        if (logicBe) {
                logicBe->SetUserLimits(userLimitsBe);
                G4cout << "[DetectorConstruction] Limite de step appliquée à Béryllium : "<< G4endl;
                //<< stepLimitBe / mm << " mm" << G4endl;
        } else {
                G4cerr << "[DetectorConstruction] ⚠️ Volume logique Béryllium non trouvé !" << G4endl;
        }

        // Section logicSphere supprimée (sphère supprimée)



        {
        G4cout << "\n[GEOM][ECHO]" << G4endl;
        // Plan à z = +6 mm
                if (auto* box = dynamic_cast<G4Box*>(solidScorePlane)) {
                        const G4double zPlaneCenter = 6.0*mm;
                        G4cout << "[GEOM] logicScorePlane center z = " << zPlaneCenter/mm << " mm"
                        << ", thickness = " << 2.0*box->GetZHalfLength()/um << " µm" << G4endl;
                        }
                        // Cube & sphère supprimés
                        G4cout << "[GEOM] Envelope: z ∈ [-60, +60] mm" << G4endl;
        }

        return physWorld;
}

void DetectorConstruction::ConstructSDandField()
{

        // Attachement conditionnel si GDML est actif
        if (fisGDML) {
                G4cout<<" ************************************* GDML Sensible ***************************************"<<G4endl;
        }

        // Récupérer le gestionnaire de SD
        G4SDManager* sdManager = G4SDManager::GetSDMpointer();

        // SD sphère supprimé (sphère supprimée)

        // SD spectral pour le plan +Z de l'enveloppe
        // Binning : 0 -> 60 keV en pas de 0.5 keV => 120 bins
        const G4double Emin_keV = 0.0;
        const G4double Emax_keV = 60.0;
        const G4int    nBins    = 120;
        const G4bool   onlyOutward = true; // flux sortant vers +Z

        auto* specSD = new SurfaceSpectrumSD("SpecSD", Emin_keV, Emax_keV, nBins, onlyOutward);
        sdManager->AddNewDetector(specSD);

        // Brancher le ntuple "plane_passages" créé dans SetupAnalysis
        extern int GetPlanePassageNtupleId();

        const int pid = GetPlanePassageNtupleId();
        if (pid >= 0) {
                specSD->SetPassageNtupleId(pid);
                G4cout << "[ANALYSIS] SpecSD ntupleId preset at construction: " << pid << G4endl;
        } else {
                G4cout << "[ANALYSIS] plane_passages ntupleId not ready at construction (id=" << pid
                << ") — will be set at BeginOfRunAction." << G4endl;
        }

        // Aire de la face (cm²) à partir de la géométrie réelle du plan
        auto* box = dynamic_cast<G4Box*>(solidScorePlane);
        if (box) {
                const G4double area_cm2 =
                4.0 * (box->GetXHalfLength()/cm) * (box->GetYHalfLength()/cm); // (2hx)*(2hy)
                specSD->SetArea_cm2(area_cm2);

        {
                auto* lvPlaneByName = G4LogicalVolumeStore::GetInstance()->GetVolume("logicScorePlane", /*verbose=*/false);
                if (lvPlaneByName) {
                        lvPlaneByName->SetSensitiveDetector(specSD);
                        G4cout << "[FIX] Attached SpecSD to " << lvPlaneByName->GetName()
                        << "  LV@" << lvPlaneByName << G4endl;
                } else {
                        G4cout << "[FIX][ERROR] LV 'logicScorePlane' not found in LogicalVolumeStore!" << G4endl;
                }
        }
        }


        // Attacher le SD spectral au plan mince + limite de pas + debug robuste
        if (logicScorePlane) {
                logicScorePlane->SetSensitiveDetector(specSD);

                // Limite de pas pour ne pas "sauter" le plan ultra-fin
                if (!logicScorePlane->GetUserLimits()) {
                        logicScorePlane->SetUserLimits(new G4UserLimits(0.05*mm));
                } else {
                        logicScorePlane->GetUserLimits()->SetMaxAllowedStep(0.05*mm);
                }

        // DEBUG : afficher le SD attaché
        auto* sd = logicScorePlane->GetSensitiveDetector();
        if (sd) G4cout << "[DEBUG] logicScorePlane SD = " << sd->GetName() << G4endl;
        else G4cout << "[WARN] logicScorePlane nul : SpecSD non attaché." << G4endl;
        } else {
                G4cout << "[ERROR] logicScorePlane nul : SD plan non attaché." << G4endl;
        }

        // =====================================================
        // SD pour le plan de comptage ScorePlane2 (z = 8 mm)
        // =====================================================
        extern int GetScorePlane2NtupleId();
        
        auto* scorePlane2SD = new ScorePlane2SD("ScorePlane2SD");
        sdManager->AddNewDetector(scorePlane2SD);
        
        // Connecter au ntuple
        const int sp2id = GetScorePlane2NtupleId();
        if (sp2id >= 0) {
                scorePlane2SD->SetNtupleId(sp2id);
                G4cout << "[ANALYSIS] ScorePlane2SD ntupleId preset at construction: " << sp2id << G4endl;
        } else {
                G4cout << "[ANALYSIS] ScorePlane2_passages ntupleId not ready at construction (id=" << sp2id
                       << ") — will be set at BeginOfRunAction." << G4endl;
        }
        
        // Attacher au volume logique ScorePlane2
        auto* lvScorePlane2 = G4LogicalVolumeStore::GetInstance()->GetVolume("logicScorePlane2", /*verbose=*/false);
        if (lvScorePlane2) {
                lvScorePlane2->SetSensitiveDetector(scorePlane2SD);
                G4cout << "[SD] Attached ScorePlane2SD to " << lvScorePlane2->GetName() << G4endl;
                
                // Limite de pas pour ne pas "sauter" le volume
                if (!lvScorePlane2->GetUserLimits()) {
                        lvScorePlane2->SetUserLimits(new G4UserLimits(0.1*mm));
                }
        } else {
                G4cout << "[ERROR] LV 'logicScorePlane2' not found in LogicalVolumeStore!" << G4endl;
        }

        // =====================================================
        // SD pour le plan de comptage ScorePlane3 (z = 10 mm)
        // =====================================================
        extern int GetScorePlane3NtupleId();
        
        auto* scorePlane3SD = new ScorePlane3SD("ScorePlane3SD");
        sdManager->AddNewDetector(scorePlane3SD);
        
        // Connecter au ntuple
        const int sp3id = GetScorePlane3NtupleId();
        if (sp3id >= 0) {
                scorePlane3SD->SetNtupleId(sp3id);
                G4cout << "[ANALYSIS] ScorePlane3SD ntupleId preset at construction: " << sp3id << G4endl;
        } else {
                G4cout << "[ANALYSIS] ScorePlane3_passages ntupleId not ready at construction (id=" << sp3id
                       << ") — will be set at BeginOfRunAction." << G4endl;
        }
        
        // Attacher au volume logique ScorePlane3
        auto* lvScorePlane3 = G4LogicalVolumeStore::GetInstance()->GetVolume("logicScorePlane3", /*verbose=*/false);
        if (lvScorePlane3) {
                lvScorePlane3->SetSensitiveDetector(scorePlane3SD);
                G4cout << "[SD] Attached ScorePlane3SD to " << lvScorePlane3->GetName() << G4endl;
                
                // Limite de pas pour ne pas "sauter" le volume
                if (!lvScorePlane3->GetUserLimits()) {
                        lvScorePlane3->SetUserLimits(new G4UserLimits(0.1*mm));
                }
        } else {
                G4cout << "[ERROR] LV 'logicScorePlane3' not found in LogicalVolumeStore!" << G4endl;
        }

        // =====================================================
        // SD pour les couronnes d'eau (remplace ScorePlane4 à z = 68 mm)
        // Le même SD est attaché à toutes les couronnes d'eau
        // =====================================================
        extern int GetScorePlane4NtupleId();
        
        auto* scorePlane4SD = new ScorePlane4SD("ScorePlane4SD");
        sdManager->AddNewDetector(scorePlane4SD);
        
        // Connecter au ntuple (réutilise le ntuple ScorePlane4 pour les couronnes d'eau)
        const int sp4id = GetScorePlane4NtupleId();
        if (sp4id >= 0) {
                scorePlane4SD->SetNtupleId(sp4id);
                G4cout << "[ANALYSIS] ScorePlane4SD (WaterRings) ntupleId preset at construction: " << sp4id << G4endl;
        } else {
                G4cout << "[ANALYSIS] WaterRings ntupleId not ready at construction (id=" << sp4id
                       << ") — will be set at BeginOfRunAction." << G4endl;
        }
        
        // Attacher aux volumes logiques des couronnes d'eau
        G4int nRingsAttached = 0;
        for (G4int i = 0; i < kNbWaterRings; i++) {
                G4String lvName = "logicWaterRing" + std::to_string(i);
                auto* lvRing = G4LogicalVolumeStore::GetInstance()->GetVolume(lvName, /*verbose=*/false);
                if (lvRing) {
                        lvRing->SetSensitiveDetector(scorePlane4SD);
                        G4cout << "[SD] Attached ScorePlane4SD to " << lvRing->GetName() << G4endl;
                        
                        if (!lvRing->GetUserLimits()) {
                                lvRing->SetUserLimits(new G4UserLimits(0.1*mm));
                        }
                        nRingsAttached++;
                } else {
                        G4cout << "[WARN] LV '" << lvName << "' not found in LogicalVolumeStore!" << G4endl;
                }
        }
        G4cout << "[SD] Total water rings with ScorePlane4SD attached: " << nRingsAttached << "/" << kNbWaterRings << G4endl;

        // =====================================================
        // SD pour le plan de comptage ScorePlane5 (z = 118 mm)
        // =====================================================
        extern int GetScorePlane5NtupleId();
        
        auto* scorePlane5SD = new ScorePlane5SD("ScorePlane5SD");
        sdManager->AddNewDetector(scorePlane5SD);
        
        // Connecter au ntuple
        const int sp5id = GetScorePlane5NtupleId();
        if (sp5id >= 0) {
                scorePlane5SD->SetNtupleId(sp5id);
                G4cout << "[ANALYSIS] ScorePlane5SD ntupleId preset at construction: " << sp5id << G4endl;
        } else {
                G4cout << "[ANALYSIS] ScorePlane5_passages ntupleId not ready at construction (id=" << sp5id
                       << ") — will be set at BeginOfRunAction." << G4endl;
        }
        
        // Attacher au volume logique ScorePlane5
        auto* lvScorePlane5 = G4LogicalVolumeStore::GetInstance()->GetVolume("logicScorePlane5", /*verbose=*/false);
        if (lvScorePlane5) {
                lvScorePlane5->SetSensitiveDetector(scorePlane5SD);
                G4cout << "[SD] Attached ScorePlane5SD to " << lvScorePlane5->GetName() << G4endl;
                
                if (!lvScorePlane5->GetUserLimits()) {
                        lvScorePlane5->SetUserLimits(new G4UserLimits(0.1*mm));
                }
        } else {
                G4cout << "[ERROR] LV 'logicScorePlane5' not found in LogicalVolumeStore!" << G4endl;
        }

        // ScorePlane6 supprimé

        // =====================================================
        // Limites de step pour les plans de comptage
        // =====================================================
        const G4double maxStepPlanes = 0.1*mm;  // Step max = 0.1 mm
        
        auto* lvStore = G4LogicalVolumeStore::GetInstance();
        
        for (int i = 4; i <= 5; ++i) {
                G4String lvName = "logicScorePlane" + std::to_string(i);
                auto* lv = lvStore->GetVolume(lvName, false);
                if (lv) {
                        if (!lv->GetUserLimits()) {
                                lv->SetUserLimits(new G4UserLimits(maxStepPlanes));
                        } else {
                                lv->GetUserLimits()->SetMaxAllowedStep(maxStepPlanes);
                        }
                        G4cout << "[STEP] Max step in " << lvName << " = " << maxStepPlanes/mm << " mm" << G4endl;
                } else {
                        G4cout << "[WARN] LV '" << lvName << "' not found - step limit not applied" << G4endl;
                }
        }

}

void DetectorConstruction::PrintUsedMaterials()
{
        auto lvStore = G4LogicalVolumeStore::GetInstance();
        std::set<G4Material*> uniqueMaterials;

        G4cout << "\n=== Volumes logiques et matériaux associés ===" << G4endl;
        for (auto lv : *lvStore) {
                G4Material* mat = lv->GetMaterial();
                G4String matName = (mat) ? mat->GetName() : "null";
                G4cout << " - " << lv->GetName() << " : " << matName << G4endl;
                if (mat) uniqueMaterials.insert(mat);
        }

        G4cout << "\n=== Matériaux utilisés dans la géométrie ===" << G4endl;
        for (auto mat : uniqueMaterials) {
                G4cout << " • " << mat->GetName()
                << " | densité = " << mat->GetDensity() / (g/cm3) << " g/cm3"
                << " | N composants = " << mat->GetNumberOfElements()
                << G4endl;

                for (size_t i = 0; i < mat->GetNumberOfElements(); ++i) {
                        const G4Element* elem = mat->GetElement(i);
                        G4double frac = mat->GetFractionVector()[i];
                        G4cout << "    → " << elem->GetName() << " (" << frac << ")" << G4endl;
                }
        }

        G4cout << "=============================================\n" << G4endl;
}

void DetectorConstruction::PrintAllMaterials()
{
        const G4MaterialTable* matTable = G4Material::GetMaterialTable();
        G4cout << "\n=== Tous les matériaux connus de Geant4 ===" << G4endl;
        for (auto mat : *matTable) {
                G4cout << " • " << mat->GetName() << G4endl;
        }
}

// =====================================================
// NOUVEAU : Accesseur pour les couronnes d'eau
// =====================================================
G4LogicalVolume* DetectorConstruction::GetWaterRingLogicalVolume(G4int ringIndex) const
{
        if (ringIndex >= 0 && ringIndex < kNbWaterRings) {
                return logicWaterRing[ringIndex];
        }
        G4cerr << "[ERROR] GetWaterRingLogicalVolume: invalid index " << ringIndex 
               << " (valid: 0-" << kNbWaterRings-1 << ")" << G4endl;
        return nullptr;
}
