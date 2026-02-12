//============================================================================
// analyse_simulation.C
// Script ROOT pour l'analyse des données de simulation Geant4
// 
// Usage: root -l -b -q 'analyse_simulation.C("output.root")'
//
// MODIFICATIONS:
// - Ajout de WaterRings_passages aux distributions 2D
// - Limites X,Y étendues à ±20 mm
// - [30/01/2026] Correction des noms d'histogrammes de dose:
//   * Dose_total_1000evt -> Dose_total_10000evt
//   * Dose_ring*_1000evt -> Dose_ring*_10000evt
// - [30/01/2026] Correction des unités:
//   * Les histogrammes sont maintenant en pGy (pas µGy)
//   * Conversion pGy -> nGy: diviser par 1000 (pas multiplier)
//============================================================================

#include "TFile.h"
#include "TTree.h"
#include "TH1D.h"
#include "TH2D.h"
#include "TCanvas.h"
#include "TStyle.h"
#include "TLegend.h"
#include "TPaveText.h"
#include "TLatex.h"
#include "TMath.h"
#include "TColor.h"
#include "TROOT.h"
#include "TEllipse.h"

#include <iostream>
#include <vector>
#include <string>

void analyse_simulation(const char* filename = "output.root")
{
    //==========================================================================
    // Configuration du style
    //==========================================================================
    gStyle->SetOptStat(0);
    gStyle->SetOptTitle(1);
    gStyle->SetTitleFontSize(0.04);
    gStyle->SetLabelSize(0.035, "XY");
    gStyle->SetTitleSize(0.04, "XY");
    gStyle->SetPadLeftMargin(0.12);
    gStyle->SetPadRightMargin(0.15);
    gStyle->SetPadBottomMargin(0.12);
    gStyle->SetPalette(kRainBow);
    
    //==========================================================================
    // Ouverture du fichier
    //==========================================================================
    TFile *f = TFile::Open(filename);
    if (!f || f->IsZombie()) {
        std::cerr << "Erreur: impossible d'ouvrir " << filename << std::endl;
        return;
    }
    std::cout << "Fichier ouvert: " << filename << std::endl;
    
    //==========================================================================
    // Liste des plans de scoring (incluant WaterRings)
    //==========================================================================
    std::vector<std::string> planeNames = {
        "plane_passages",
        "ScorePlane2_passages", 
        "ScorePlane3_passages",
        "WaterRings_passages",
        "ScorePlane5_passages"
    };
    std::vector<std::string> planeLabels = {
        "z = 18 mm",
        "z = 28 mm",
        "z = 38 mm",
        "z = 65-68 mm (Water)", 
        "z = 70 mm"
    };
    std::vector<int> planeColors = {kBlue, kGreen+2, kOrange+1, kMagenta+1, kRed};
    
    //==========================================================================
    // Configuration des limites XY (modifiable)
    //==========================================================================
    const double xyMin = -20.0;  // mm
    const double xyMax = +20.0;  // mm
    const int nBinsXY = 100;
    
    //==========================================================================
    // 1. HISTOGRAMMES 2D : Distribution XY des particules
    //==========================================================================
    std::cout << "\n=== Création des histogrammes 2D (X,Y) - Limites: [" 
              << xyMin << ", " << xyMax << "] mm ===" << std::endl;
    
    // Canvas 3x2 pour 5 plans + 1 vide
    TCanvas *c1 = new TCanvas("c1", "Distribution 2D - Primaires", 1800, 1200);
    c1->Divide(3, 2);
    
    TCanvas *c2 = new TCanvas("c2", "Distribution 2D - Secondaires", 1800, 1200);
    c2->Divide(3, 2);
    
    for (int i = 0; i < 5; i++) {
        TTree *tree = (TTree*)f->Get(planeNames[i].c_str());
        if (!tree) {
            std::cout << "  Ntuple " << planeNames[i] << " non trouvé" << std::endl;
            continue;
        }
        
        std::cout << "  " << planeNames[i] << ": " << tree->GetEntries() << " entrées" << std::endl;
        
        // Tous les ntuples ont maintenant la même structure avec is_secondary
        const char* cutPrimary = "is_secondary==0 || parentID==0";
        const char* cutSecondary = "is_secondary==1 && parentID!=0";
        
        // Histogramme 2D primaires
        c1->cd(i+1);
        gPad->SetLogz();
        TH2D *h2d_prim = new TH2D(Form("h2d_prim_%d", i), 
            Form("Primaires - %s;X (mm);Y (mm)", planeLabels[i].c_str()),
            nBinsXY, xyMin, xyMax, nBinsXY, xyMin, xyMax);
        tree->Draw(Form("y_mm:x_mm>>h2d_prim_%d", i), cutPrimary, "COLZ");
        h2d_prim->SetMinimum(1);
        
        // Histogramme 2D secondaires
        c2->cd(i+1);
        gPad->SetLogz();
        TH2D *h2d_sec = new TH2D(Form("h2d_sec_%d", i),
            Form("Secondaires - %s;X (mm);Y (mm)", planeLabels[i].c_str()),
            nBinsXY, xyMin, xyMax, nBinsXY, xyMin, xyMax);
        tree->Draw(Form("y_mm:x_mm>>h2d_sec_%d", i), cutSecondary, "COLZ");
        h2d_sec->SetMinimum(1);
    }
    
    // Panneau 6: Légende / Info
    c1->cd(6);
    TPaveText *info1 = new TPaveText(0.1, 0.1, 0.9, 0.9, "NDC");
    info1->SetBorderSize(1);
    info1->SetFillColor(kWhite);
    info1->SetTextAlign(12);
    info1->AddText("#bf{Distribution 2D - Primaires}");
    info1->AddText("");
    info1->AddText("Plans de mesure:");
    info1->AddText("  z=18mm: Après collimateur");
    info1->AddText("  z=28mm: ScorePlane2");
    info1->AddText("  z=38mm: ScorePlane3");
    info1->AddText("  z=65-68mm: Couronnes d'eau");
    info1->AddText("  z=70mm: Après conteneur");
    info1->AddText("");
    info1->AddText(Form("Limites: X,Y #in [%.0f, %.0f] mm", xyMin, xyMax));
    info1->Draw();
    
    c2->cd(6);
    TPaveText *info2 = new TPaveText(0.1, 0.1, 0.9, 0.9, "NDC");
    info2->SetBorderSize(1);
    info2->SetFillColor(kWhite);
    info2->SetTextAlign(12);
    info2->AddText("#bf{Distribution 2D - Secondaires}");
    info2->AddText("");
    info2->AddText("Particules secondaires:");
    info2->AddText("  (is_secondary==1 && parentID!=0)");
    info2->AddText("");
    info2->AddText("Électrons, photons diffusés,");
    info2->AddText("particules de fluorescence, etc.");
    info2->AddText("");
    info2->AddText(Form("Limites: X,Y #in [%.0f, %.0f] mm", xyMin, xyMax));
    info2->Draw();
    
    c1->SaveAs("distribution_2D_primaires.png");
    c2->SaveAs("distribution_2D_secondaires.png");

    
    //==========================================================================
    // 2. DISTRIBUTION RADIALE (échelle log)
    //==========================================================================
    std::cout << "\n=== Création des distributions radiales ===" << std::endl;
    
    const double rMax = 25.0;  // Rayon max = 25 mm
    
    // -------------------------------------------------------------------------
    // Canvas pour PRIMAIRES - avec remplissage préalable pour trouver le max
    // -------------------------------------------------------------------------
    TCanvas *c3 = new TCanvas("c3", "Distribution radiale - Primaires", 1200, 800);
    c3->SetLogy();
    c3->SetGridx();
    c3->SetGridy();
    
    TLegend *leg3 = new TLegend(0.65, 0.60, 0.88, 0.88);
    leg3->SetBorderSize(1);
    leg3->SetFillColor(kWhite);
    
    // Étape 1: Créer et remplir tous les histogrammes (sans dessiner)
    std::vector<TH1D*> histos_prim;
    double maxY_prim = 0;
    
    for (int i = 0; i < 5; i++) {
        TTree *tree = (TTree*)f->Get(planeNames[i].c_str());
        if (!tree) {
            std::cout << "  [WARN] " << planeNames[i] << " non trouvé pour radiale primaires" << std::endl;
            histos_prim.push_back(nullptr);
            continue;
        }
        
        TH1D *h_r_prim = new TH1D(Form("h_r_prim_%d", i),
            "Distribution radiale - Particules primaires;Rayon (mm);Counts",
            100, 0, rMax);
        h_r_prim->SetLineColor(planeColors[i]);
        h_r_prim->SetLineWidth(2);
        
        // Remplir sans dessiner (option "goff")
        tree->Draw(Form("sqrt(x_mm*x_mm + y_mm*y_mm)>>h_r_prim_%d", i), 
                   "is_secondary==0 || parentID==0", "goff");
        
        histos_prim.push_back(h_r_prim);
        
        if (h_r_prim->GetMaximum() > maxY_prim) {
            maxY_prim = h_r_prim->GetMaximum();
        }
        std::cout << "  Radiale prim " << planeLabels[i] << ": " << h_r_prim->GetEntries() << " entrées, max=" << h_r_prim->GetMaximum() << std::endl;
    }
    
    // Étape 2: Dessiner tous les histogrammes
    bool firstPrim = true;
    for (int i = 0; i < 5; i++) {
        if (!histos_prim[i] || histos_prim[i]->GetEntries() == 0) continue;
        
        if (firstPrim) {
            histos_prim[i]->SetMaximum(maxY_prim * 1.2);
            histos_prim[i]->SetMinimum(0.5);  // Pour échelle log
            histos_prim[i]->GetXaxis()->SetRangeUser(0, rMax);
            histos_prim[i]->Draw("HIST");
            firstPrim = false;
        } else {
            histos_prim[i]->Draw("HIST SAME");
        }
        leg3->AddEntry(histos_prim[i], planeLabels[i].c_str(), "l");
    }
    leg3->Draw();
    c3->SaveAs("distribution_radiale_primaires.png");

    
    // -------------------------------------------------------------------------
    // Canvas pour SECONDAIRES
    // -------------------------------------------------------------------------
    TCanvas *c4 = new TCanvas("c4", "Distribution radiale - Secondaires", 1200, 800);
    c4->SetLogy();
    c4->SetGridx();
    c4->SetGridy();
    
    TLegend *leg4 = new TLegend(0.65, 0.60, 0.88, 0.88);
    leg4->SetBorderSize(1);
    leg4->SetFillColor(kWhite);
    
    // Étape 1: Créer et remplir tous les histogrammes
    std::vector<TH1D*> histos_sec;
    double maxY_sec = 0;
    
    for (int i = 0; i < 5; i++) {
        TTree *tree = (TTree*)f->Get(planeNames[i].c_str());
        if (!tree) {
            histos_sec.push_back(nullptr);
            continue;
        }
        
        TH1D *h_r_sec = new TH1D(Form("h_r_sec_%d", i),
            "Distribution radiale - Particules secondaires;Rayon (mm);Counts",
            100, 0, rMax);
        h_r_sec->SetLineColor(planeColors[i]);
        h_r_sec->SetLineWidth(2);
        
        tree->Draw(Form("sqrt(x_mm*x_mm + y_mm*y_mm)>>h_r_sec_%d", i),
                   "is_secondary==1 && parentID!=0", "goff");
        
        histos_sec.push_back(h_r_sec);
        
        if (h_r_sec->GetMaximum() > maxY_sec) {
            maxY_sec = h_r_sec->GetMaximum();
        }
        std::cout << "  Radiale sec " << planeLabels[i] << ": " << h_r_sec->GetEntries() << " entrées" << std::endl;
    }
    
    // Étape 2: Dessiner
    bool firstSec = true;
    for (int i = 0; i < 5; i++) {
        if (!histos_sec[i] || histos_sec[i]->GetEntries() == 0) continue;
        
        if (firstSec) {
            histos_sec[i]->SetMaximum(maxY_sec * 1.2);
            histos_sec[i]->SetMinimum(0.5);
            histos_sec[i]->GetXaxis()->SetRangeUser(0, rMax);
            histos_sec[i]->Draw("HIST");
            firstSec = false;
        } else {
            histos_sec[i]->Draw("HIST SAME");
        }
        leg4->AddEntry(histos_sec[i], planeLabels[i].c_str(), "l");
    }
    leg4->Draw();
    c4->SaveAs("distribution_radiale_secondaires.png");

    
    // -------------------------------------------------------------------------
    // Canvas pour TOUTES les particules
    // -------------------------------------------------------------------------
    TCanvas *c5 = new TCanvas("c5", "Distribution radiale - Toutes particules", 1200, 800);
    c5->SetLogy();
    c5->SetGridx();
    c5->SetGridy();
    
    TLegend *leg5 = new TLegend(0.65, 0.60, 0.88, 0.88);
    leg5->SetBorderSize(1);
    leg5->SetFillColor(kWhite);
    
    // Étape 1: Créer et remplir
    std::vector<TH1D*> histos_all;
    double maxY_all = 0;
    
    for (int i = 0; i < 5; i++) {
        TTree *tree = (TTree*)f->Get(planeNames[i].c_str());
        if (!tree) {
            histos_all.push_back(nullptr);
            continue;
        }
        
        TH1D *h_r_all = new TH1D(Form("h_r_all_%d", i),
            "Distribution radiale - Toutes particules;Rayon (mm);Counts",
            100, 0, rMax);
        h_r_all->SetLineColor(planeColors[i]);
        h_r_all->SetLineWidth(2);
        
        tree->Draw(Form("sqrt(x_mm*x_mm + y_mm*y_mm)>>h_r_all_%d", i), "", "goff");
        
        histos_all.push_back(h_r_all);
        
        if (h_r_all->GetMaximum() > maxY_all) {
            maxY_all = h_r_all->GetMaximum();
        }
    }
    
    // Étape 2: Dessiner
    bool firstAll = true;
    for (int i = 0; i < 5; i++) {
        if (!histos_all[i] || histos_all[i]->GetEntries() == 0) continue;
        
        if (firstAll) {
            histos_all[i]->SetMaximum(maxY_all * 1.2);
            histos_all[i]->SetMinimum(0.5);
            histos_all[i]->GetXaxis()->SetRangeUser(0, rMax);
            histos_all[i]->Draw("HIST");
            firstAll = false;
        } else {
            histos_all[i]->Draw("HIST SAME");
        }
        leg5->AddEntry(histos_all[i], planeLabels[i].c_str(), "l");
    }
    leg5->Draw();
    c5->SaveAs("distribution_radiale_toutes.png");

    
    //==========================================================================
    // 3. SPECTRE EN ENERGIE
    //==========================================================================
    std::cout << "\n=== Creation des spectres en energie ===" << std::endl;
    
    const double eMax = 60.0;  // Energie max en keV
    
    // -------------------------------------------------------------------------
    // TOUTES PARTICULES
    // -------------------------------------------------------------------------
    TCanvas *c6 = new TCanvas("c6", "Spectre en energie - Toutes particules", 1200, 800);
    c6->SetLogy();
    c6->SetGridx();
    c6->SetGridy();
    
    TLegend *leg6 = new TLegend(0.65, 0.60, 0.88, 0.88);
    leg6->SetBorderSize(1);
    leg6->SetFillColor(kWhite);
    
    // Etape 1: Creer et remplir tous les histogrammes
    std::vector<TH1D*> histos_E_all;
    double maxY_E_all = 0;
    
    for (int i = 0; i < 5; i++) {
        TTree *tree = (TTree*)f->Get(planeNames[i].c_str());
        if (!tree) {
            histos_E_all.push_back(nullptr);
            continue;
        }
        
        TH1D *h_E = new TH1D(Form("h_E_all_%d", i),
            "Spectre en energie - Toutes particules;Energie (keV);Counts",
            120, 0, eMax);
        h_E->SetLineColor(planeColors[i]);
        h_E->SetLineWidth(2);
        
        tree->Draw(Form("ekin_keV>>h_E_all_%d", i), "", "goff");
        histos_E_all.push_back(h_E);
        
        if (h_E->GetMaximum() > maxY_E_all) {
            maxY_E_all = h_E->GetMaximum();
        }
        std::cout << "  Energie all " << planeLabels[i] << ": " << h_E->GetEntries() << " entrees" << std::endl;
    }
    
    // Etape 2: Dessiner
    bool firstEAll = true;
    for (int i = 0; i < 5; i++) {
        if (!histos_E_all[i] || histos_E_all[i]->GetEntries() == 0) continue;
        
        if (firstEAll) {
            histos_E_all[i]->SetMaximum(maxY_E_all * 1.2);
            histos_E_all[i]->SetMinimum(0.5);
            histos_E_all[i]->GetXaxis()->SetRangeUser(0, eMax);
            histos_E_all[i]->Draw("HIST");
            firstEAll = false;
        } else {
            histos_E_all[i]->Draw("HIST SAME");
        }
        leg6->AddEntry(histos_E_all[i], planeLabels[i].c_str(), "l");
    }
    leg6->Draw();
    c6->SaveAs("distribution_energie_toutes.png");

    
    // -------------------------------------------------------------------------
    // PRIMAIRES SEULEMENT
    // -------------------------------------------------------------------------
    TCanvas *c7 = new TCanvas("c7", "Spectre en energie - Primaires", 1200, 800);
    c7->SetLogy();
    c7->SetGridx();
    c7->SetGridy();
    
    TLegend *leg7 = new TLegend(0.65, 0.60, 0.88, 0.88);
    leg7->SetBorderSize(1);
    leg7->SetFillColor(kWhite);
    
    // Etape 1: Creer et remplir
    std::vector<TH1D*> histos_E_prim;
    double maxY_E_prim = 0;
    
    for (int i = 0; i < 5; i++) {
        TTree *tree = (TTree*)f->Get(planeNames[i].c_str());
        if (!tree) {
            histos_E_prim.push_back(nullptr);
            continue;
        }
        
        TH1D *h_E_prim = new TH1D(Form("h_E_prim_%d", i),
            "Spectre en energie - Particules primaires;Energie (keV);Counts",
            120, 0, eMax);
        h_E_prim->SetLineColor(planeColors[i]);
        h_E_prim->SetLineWidth(2);
        
        tree->Draw(Form("ekin_keV>>h_E_prim_%d", i), 
                   "is_secondary==0 || parentID==0", "goff");
        histos_E_prim.push_back(h_E_prim);
        
        if (h_E_prim->GetMaximum() > maxY_E_prim) {
            maxY_E_prim = h_E_prim->GetMaximum();
        }
        std::cout << "  Energie prim " << planeLabels[i] << ": " << h_E_prim->GetEntries() << " entrees" << std::endl;
    }
    
    // Etape 2: Dessiner
    bool firstEPrim = true;
    for (int i = 0; i < 5; i++) {
        if (!histos_E_prim[i] || histos_E_prim[i]->GetEntries() == 0) continue;
        
        if (firstEPrim) {
            histos_E_prim[i]->SetMaximum(maxY_E_prim * 1.2);
            histos_E_prim[i]->SetMinimum(0.5);
            histos_E_prim[i]->GetXaxis()->SetRangeUser(0, eMax);
            histos_E_prim[i]->Draw("HIST");
            firstEPrim = false;
        } else {
            histos_E_prim[i]->Draw("HIST SAME");
        }
        leg7->AddEntry(histos_E_prim[i], planeLabels[i].c_str(), "l");
    }
    leg7->Draw();
    c7->SaveAs("distribution_energie_primaires.png");

    
    // -------------------------------------------------------------------------
    // SECONDAIRES SEULEMENT
    // -------------------------------------------------------------------------
    TCanvas *c8 = new TCanvas("c8", "Spectre en energie - Secondaires", 1200, 800);
    c8->SetLogy();
    c8->SetGridx();
    c8->SetGridy();
    
    TLegend *leg8 = new TLegend(0.65, 0.60, 0.88, 0.88);
    leg8->SetBorderSize(1);
    leg8->SetFillColor(kWhite);
    
    // Etape 1: Creer et remplir
    std::vector<TH1D*> histos_E_sec;
    double maxY_E_sec = 0;
    
    for (int i = 0; i < 5; i++) {
        TTree *tree = (TTree*)f->Get(planeNames[i].c_str());
        if (!tree) {
            histos_E_sec.push_back(nullptr);
            continue;
        }
        
        TH1D *h_E_sec = new TH1D(Form("h_E_sec_%d", i),
            "Spectre en energie - Particules secondaires;Energie (keV);Counts",
            120, 0, eMax);
        h_E_sec->SetLineColor(planeColors[i]);
        h_E_sec->SetLineWidth(2);
        
        tree->Draw(Form("ekin_keV>>h_E_sec_%d", i),
                   "is_secondary==1 && parentID!=0", "goff");
        histos_E_sec.push_back(h_E_sec);
        
        if (h_E_sec->GetMaximum() > maxY_E_sec) {
            maxY_E_sec = h_E_sec->GetMaximum();
        }
        std::cout << "  Energie sec " << planeLabels[i] << ": " << h_E_sec->GetEntries() << " entrees" << std::endl;
    }
    
    // Etape 2: Dessiner
    bool firstESec = true;
    for (int i = 0; i < 5; i++) {
        if (!histos_E_sec[i] || histos_E_sec[i]->GetEntries() == 0) continue;
        
        if (firstESec) {
            histos_E_sec[i]->SetMaximum(maxY_E_sec * 1.2);
            histos_E_sec[i]->SetMinimum(0.5);
            histos_E_sec[i]->GetXaxis()->SetRangeUser(0, eMax);
            histos_E_sec[i]->Draw("HIST");
            firstESec = false;
        } else {
            histos_E_sec[i]->Draw("HIST SAME");
        }
        leg8->AddEntry(histos_E_sec[i], planeLabels[i].c_str(), "l");
    }
    leg8->Draw();
    c8->SaveAs("distribution_energie_secondaires.png");

    
    //==========================================================================
    // 4. HISTOGRAMMES DE DOSE PAR 10000 EVENEMENTS (corrigé)
    //==========================================================================
    // IMPORTANT: Les histogrammes sont maintenant en pGy (pas µGy)
    // Noms: Dose_total_10000evt, Dose_ring*_10000evt
    //==========================================================================
    std::cout << "\n=== Recuperation des histogrammes de dose ===" << std::endl;
    
    TCanvas *c9 = new TCanvas("c9", "Dose par 10000 evt", 1800, 1000);
    c9->Divide(3, 2);
    
    // Dose totale par 10000 evt
    c9->cd(1);
    TH1D *h_dose_total = (TH1D*)f->Get("Dose_total_10000evt");
    if (h_dose_total && h_dose_total->GetEntries() > 0) {
        std::cout << "  Dose_total_10000evt: " << h_dose_total->GetEntries() << " entrees" 
                  << ", mean=" << h_dose_total->GetMean() << " pGy"
                  << ", RMS=" << h_dose_total->GetRMS() << " pGy" << std::endl;
        h_dose_total->SetTitle("Dose totale par 10000 evt;Dose (pGy);Counts");
        h_dose_total->SetLineColor(kBlack);
        h_dose_total->SetLineWidth(2);
        h_dose_total->SetFillColor(kGray);
        h_dose_total->Draw();
    } else {
        std::cout << "  [WARN] Dose_total_10000evt non trouve ou vide" << std::endl;
    }
    
    // Dose par anneau (par 10000 evt)
    int ringColors[5] = {kRed, kOrange+1, kYellow+1, kGreen+1, kCyan+1};
    for (int i = 0; i < 5; i++) {
        c9->cd(i+2);
        TH1D *h_dose_ring = (TH1D*)f->Get(Form("Dose_ring%d_10000evt", i));
        if (h_dose_ring && h_dose_ring->GetEntries() > 0) {
            std::cout << "  Dose_ring" << i << "_10000evt: " << h_dose_ring->GetEntries() 
                      << " entrees, mean=" << h_dose_ring->GetMean() << " pGy" << std::endl;
            h_dose_ring->SetTitle(Form("Dose anneau %d (r=%d-%d mm) par 10000 evt;Dose (pGy);Counts", 
                                       i, 2*i, 2*(i+1)));
            h_dose_ring->SetLineColor(ringColors[i]);
            h_dose_ring->SetLineWidth(2);
            h_dose_ring->SetFillColor(ringColors[i]-9);
            h_dose_ring->Draw();
        } else {
            std::cout << "  [WARN] Dose_ring" << i << "_10000evt non trouve ou vide" << std::endl;
        }
    }
    
    c9->SaveAs("dose_par_10000evt.png");

    
    //==========================================================================
    // 5. DOSE TOTALE PAR ANNEAU (avec pave dose totale)
    //==========================================================================
    // IMPORTANT: Les histogrammes _run sont maintenant en pGy
    // Conversion: 1 nGy = 1000 pGy
    //==========================================================================
    std::cout << "\n=== Creation de l'histogramme dose vs anneau ===" << std::endl;
    
    TCanvas *c10 = new TCanvas("c10", "Dose par anneau", 1000, 700);
    c10->SetGridy();
    
    // Recuperer les doses depuis les histogrammes _run (en pGy)
    // Affichage en nGy (1 nGy = 1000 pGy)
    TH1D *h_dose_vs_ring = new TH1D("h_dose_vs_ring", 
        "Dose deposee par anneau;Numero d'anneau;Dose (nGy)", 5, -0.5, 4.5);
    
    double dose_totale_pGy = 0.0;
    double dose_totale_nGy = 0.0;
    double doses_pGy[5] = {0, 0, 0, 0, 0};
    double doses_nGy[5] = {0, 0, 0, 0, 0};
    
    // Recuperer la dose totale du run (en pGy)
    TH1D *h_total_run = (TH1D*)f->Get("Dose_total_run");
    if (h_total_run && h_total_run->GetEntries() > 0) {
        dose_totale_pGy = h_total_run->GetMean();
        dose_totale_nGy = dose_totale_pGy / 1000.0;  // pGy -> nGy
        std::cout << "  Dose totale: " << dose_totale_pGy << " pGy = " 
                  << dose_totale_nGy << " nGy" << std::endl;
    }
    
    // Recuperer les doses par anneau (en pGy)
    for (int i = 0; i < 5; i++) {
        TH1D *h_ring = (TH1D*)f->Get(Form("Dose_ring%d_run", i));
        if (h_ring && h_ring->GetEntries() > 0) {
            doses_pGy[i] = h_ring->GetMean();
            doses_nGy[i] = doses_pGy[i] / 1000.0;  // pGy -> nGy
            h_dose_vs_ring->SetBinContent(i+1, doses_nGy[i]);
            std::cout << "  Anneau " << i << " (r=" << 2*i << "-" << 2*(i+1) 
                      << " mm): " << doses_pGy[i] << " pGy = " 
                      << doses_nGy[i] << " nGy" << std::endl;
        }
    }
    
    // Style de l'histogramme
    h_dose_vs_ring->SetFillColor(kAzure-4);
    h_dose_vs_ring->SetLineColor(kAzure+2);
    h_dose_vs_ring->SetLineWidth(2);
    h_dose_vs_ring->SetBarWidth(0.8);
    h_dose_vs_ring->SetBarOffset(0.1);
    h_dose_vs_ring->GetYaxis()->SetRangeUser(0, h_dose_vs_ring->GetMaximum() * 1.3);
    
    // Labels personnalises pour l'axe X
    h_dose_vs_ring->GetXaxis()->SetBinLabel(1, "0 (0-2mm)");
    h_dose_vs_ring->GetXaxis()->SetBinLabel(2, "1 (2-4mm)");
    h_dose_vs_ring->GetXaxis()->SetBinLabel(3, "2 (4-6mm)");
    h_dose_vs_ring->GetXaxis()->SetBinLabel(4, "3 (6-8mm)");
    h_dose_vs_ring->GetXaxis()->SetBinLabel(5, "4 (8-10mm)");
    h_dose_vs_ring->GetXaxis()->SetLabelSize(0.045);
    
    h_dose_vs_ring->Draw("BAR");
    
    // Ajouter les valeurs au-dessus des barres (en nGy)
    TLatex latex;
    latex.SetTextSize(0.030);
    latex.SetTextAlign(21);
    for (int i = 0; i < 5; i++) {
        latex.DrawLatex(i, doses_nGy[i] + h_dose_vs_ring->GetMaximum()*0.03, 
                        Form("%.2f nGy", doses_nGy[i]));
    }
    
    // Pave avec la dose totale (en pGy et nGy)
    TPaveText *pave = new TPaveText(0.55, 0.70, 0.88, 0.88, "NDC");
    pave->SetBorderSize(2);
    pave->SetFillColor(kWhite);
    pave->SetFillStyle(1001);
    pave->SetTextAlign(12);
    pave->SetTextFont(42);
    pave->AddText("#bf{Dose totale dans l'eau}");
    pave->AddText("");
    pave->AddText(Form("#bf{D_{total} = %.0f pGy}", dose_totale_pGy));
    pave->AddText(Form("= %.2f nGy", dose_totale_nGy));
    pave->Draw();
    
    c10->SaveAs("dose_par_anneau.png");

    
    //==========================================================================
    // 6. FIGURE RÉCAPITULATIVE : Comparaison énergies par plan
    //==========================================================================
    std::cout << "\n=== Création de la figure récapitulative ===" << std::endl;
    
    TCanvas *c11 = new TCanvas("c11", "Récapitulatif", 1600, 1200);
    c11->Divide(2, 2);
    
    // Panneau 1: Distribution 2D (plan principal z=18mm)
    c11->cd(1);
    gPad->SetLogz();
    TTree *tree0 = (TTree*)f->Get("plane_passages");
    if (tree0) {
        TH2D *h2d_recap = new TH2D("h2d_recap", 
            "Distribution XY (z=18mm);X (mm);Y (mm)", 
            nBinsXY, xyMin, xyMax, nBinsXY, xyMin, xyMax);
        tree0->Draw("y_mm:x_mm>>h2d_recap", "", "COLZ");
    }
    
    // Panneau 2: Spectre en énergie superposé
    c11->cd(2);
    gPad->SetLogy();
    gPad->SetGridx();
    gPad->SetGridy();
    
    TLegend *leg11 = new TLegend(0.55, 0.60, 0.88, 0.88);
    leg11->SetBorderSize(1);
    for (int i = 0; i < 5; i++) {
        TTree *tree = (TTree*)f->Get(planeNames[i].c_str());
        if (!tree) continue;
        
        TH1D *h_E = new TH1D(Form("h_E_recap_%d", i),
            "Spectre en énergie;Énergie (keV);Counts", 100, 0, 50);
        h_E->SetLineColor(planeColors[i]);
        h_E->SetLineWidth(2);
        tree->Draw(Form("ekin_keV>>h_E_recap_%d", i), "", i==0 ? "" : "SAME");
        leg11->AddEntry(h_E, planeLabels[i].c_str(), "l");
    }
    leg11->Draw();
    
    // Panneau 3: Distribution radiale superposée
    c11->cd(3);
    gPad->SetLogy();
    gPad->SetGridx();
    gPad->SetGridy();
    
    TLegend *leg11b = new TLegend(0.55, 0.60, 0.88, 0.88);
    leg11b->SetBorderSize(1);
    for (int i = 0; i < 5; i++) {
        TTree *tree = (TTree*)f->Get(planeNames[i].c_str());
        if (!tree) continue;
        
        TH1D *h_r = new TH1D(Form("h_r_recap_%d", i),
            "Distribution radiale;Rayon (mm);Counts", 100, 0, rMax);
        h_r->SetLineColor(planeColors[i]);
        h_r->SetLineWidth(2);
        tree->Draw(Form("sqrt(x_mm*x_mm+y_mm*y_mm)>>h_r_recap_%d", i), "", i==0 ? "" : "SAME");
        leg11b->AddEntry(h_r, planeLabels[i].c_str(), "l");
    }
    leg11b->Draw();
    
    // Panneau 4: Dose par anneau
    c11->cd(4);
    gPad->SetGridy();
    h_dose_vs_ring->Draw("BAR");
    
    TPaveText *pave2 = new TPaveText(0.50, 0.70, 0.88, 0.88, "NDC");
    pave2->SetBorderSize(2);
    pave2->SetFillColor(kWhite);
    pave2->AddText("#bf{Dose totale}");
    pave2->AddText(Form("#bf{%.2f nGy}", dose_totale_nGy));
    pave2->AddText(Form("(%.0f pGy)", dose_totale_pGy));
    pave2->Draw();
    
    c11->SaveAs("recapitulatif_simulation.png");

    
    //==========================================================================
    // 7. NOUVELLE FIGURE: Distribution 2D WaterRings uniquement
    //==========================================================================
    std::cout << "\n=== Création des distributions 2D WaterRings ===" << std::endl;
    
    TTree *treeWater = (TTree*)f->Get("WaterRings_passages");
    if (treeWater) {
        TCanvas *c12 = new TCanvas("c12", "WaterRings - Distribution 2D", 1400, 600);
        c12->Divide(2, 1);
        
        // Primaires dans l'eau
        c12->cd(1);
        gPad->SetLogz();
        TH2D *h2d_water_prim = new TH2D("h2d_water_prim",
            "WaterRings - Primaires (z=65-68mm);X (mm);Y (mm)",
            nBinsXY, xyMin, xyMax, nBinsXY, xyMin, xyMax);
        treeWater->Draw("y_mm:x_mm>>h2d_water_prim", "is_secondary==0 || parentID==0", "COLZ");
        h2d_water_prim->SetMinimum(1);
        
        // Cercles pour les anneaux d'eau (r = 2, 4, 6, 8, 10 mm)
        for (int r = 2; r <= 10; r += 2) {
            TEllipse *circle = new TEllipse(0, 0, r, r);
            circle->SetFillStyle(0);
            circle->SetLineColor(kBlack);
            circle->SetLineStyle(2);
            circle->Draw("SAME");
        }
        
        // Secondaires dans l'eau
        c12->cd(2);
        gPad->SetLogz();
        TH2D *h2d_water_sec = new TH2D("h2d_water_sec",
            "WaterRings - Secondaires (z=65-68mm);X (mm);Y (mm)",
            nBinsXY, xyMin, xyMax, nBinsXY, xyMin, xyMax);
        treeWater->Draw("y_mm:x_mm>>h2d_water_sec", "is_secondary==1 && parentID!=0", "COLZ");
        h2d_water_sec->SetMinimum(1);
        
        // Cercles pour les anneaux
        for (int r = 2; r <= 10; r += 2) {
            TEllipse *circle = new TEllipse(0, 0, r, r);
            circle->SetFillStyle(0);
            circle->SetLineColor(kBlack);
            circle->SetLineStyle(2);
            circle->Draw("SAME");
        }
        
        c12->SaveAs("distribution_2D_WaterRings.png");

        
        std::cout << "  WaterRings_passages: " << treeWater->GetEntries() << " entrées" << std::endl;
    }
    
    //==========================================================================
    // Fermeture et résumé
    //==========================================================================
    std::cout << "\n=== Fichiers générés ===" << std::endl;
    std::cout << "  - distribution_2D_primaires.png/.pdf" << std::endl;
    std::cout << "  - distribution_2D_secondaires.png/.pdf" << std::endl;
    std::cout << "  - distribution_2D_WaterRings.png/.pdf" << std::endl;
    std::cout << "  - distribution_radiale_primaires.png/.pdf" << std::endl;
    std::cout << "  - distribution_radiale_secondaires.png/.pdf" << std::endl;
    std::cout << "  - distribution_radiale_toutes.png/.pdf" << std::endl;
    std::cout << "  - distribution_energie_toutes.png/.pdf" << std::endl;
    std::cout << "  - distribution_energie_primaires.png/.pdf" << std::endl;
    std::cout << "  - distribution_energie_secondaires.png/.pdf" << std::endl;
    std::cout << "  - dose_par_10000evt.png/.pdf" << std::endl;
    std::cout << "  - dose_par_anneau.png/.pdf" << std::endl;
    std::cout << "  - recapitulatif_simulation.png/.pdf" << std::endl;
    
    std::cout << "\n=== Resume des doses ===" << std::endl;
    std::cout << "  Dose totale: " << dose_totale_pGy << " pGy = " << dose_totale_nGy << " nGy" << std::endl;
    for (int i = 0; i < 5; i++) {
        std::cout << "  Anneau " << i << " (r=" << 2*i << "-" << 2*(i+1) << " mm): " 
                  << doses_pGy[i] << " pGy = " << doses_nGy[i] << " nGy" << std::endl;
    }
    
    f->Close();
    std::cout << "\n=== Analyse terminee ===" << std::endl;
}
