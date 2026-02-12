//============================================================================
// analyse_compton_cone.C
// Script ROOT pour l'analyse des diffusions Compton dans le cône graphite
//
// Usage: root -l -b -q 'analyse_compton_cone.C("output.root")'
//
// Nécessite le ntuple "compton_cone_events" produit par la version modifiée
// de SteppingAction.cc / AnalysisManagerSetup.cc
//
// Histogrammes produits :
//   1. Carte (z, r) des interactions Compton
//   2. Histogramme 2D θ_incident vs θ_sortant
//   3. Histogramme 2D angle de diffusion vs énergie incidente
//   4. ΔE vs E_incident (avec courbe théorique Compton)
//   5. Distribution de l'angle de diffusion
//   6. Spectre en énergie avant/après Compton
//   7. Figure récapitulative
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
#include "TGraph.h"
#include "TLine.h"
#include "TProfile.h"

#include <iostream>
#include <cmath>

void analyse_compton_cone(const char* filename = "output.root")
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

    TTree *tree = (TTree*)f->Get("compton_cone_events");
    if (!tree) {
        std::cerr << "Erreur: ntuple 'compton_cone_events' non trouvé !" << std::endl;
        std::cerr << "Avez-vous utilisé la version modifiée de SteppingAction.cc ?" << std::endl;
        f->Close();
        return;
    }

    Long64_t nEntries = tree->GetEntries();
    std::cout << "=== Analyse Compton dans le cône graphite ===" << std::endl;
    std::cout << "  Ntuple compton_cone_events : " << nEntries << " entrées" << std::endl;

    if (nEntries == 0) {
        std::cerr << "Aucune entrée Compton trouvée." << std::endl;
        f->Close();
        return;
    }

    //==========================================================================
    // Paramètres géométriques du cône (pour annotation)
    //==========================================================================
    const double coneZfront = 1.90;   // mm
    const double coneZback  = 16.95;  // mm
    const double coneRout   = 3.15;   // mm
    const double me_c2      = 511.0;  // keV (masse de l'électron)

    //==========================================================================
    // 1. CARTE (z, r) DES INTERACTIONS COMPTON
    //==========================================================================
    std::cout << "\n=== 1. Carte (z, r) des interactions Compton ===" << std::endl;

    TCanvas *c1 = new TCanvas("c1_compton", "Carte (z,r) Compton", 1000, 700);
    c1->SetGridx();
    c1->SetGridy();

    TH2D *h_zr = new TH2D("h_zr_compton",
        "Diffusions Compton dans le c#hat{o}ne graphite;z (mm);r = #sqrt{x^{2}+y^{2}} (mm)",
        120, 0, 20, 60, 0, 4);
    tree->Draw("r_mm:z_mm>>h_zr_compton", "", "COLZ");
    h_zr->SetMinimum(1);

    // Tracer le contour du cône
    // Rin(z) = Rin_entry + (Rin_exit - Rin_entry) * (z - z_front) / (z_back - z_front)
    // Rin_entry = 3.0, Rin_exit = 1.0
    TGraph *gConeInner = new TGraph();
    TGraph *gConeOuter = new TGraph();
    for (int i = 0; i <= 100; i++) {
        double z = coneZfront + (coneZback - coneZfront) * i / 100.0;
        double t = (z - coneZfront) / (coneZback - coneZfront);
        double rin = 3.0 * (1.0 - t) + 1.0 * t;  // interpolation linéaire
        gConeInner->SetPoint(i, z, rin);
        gConeOuter->SetPoint(i, z, coneRout);
    }
    gConeInner->SetLineColor(kRed);
    gConeInner->SetLineWidth(2);
    gConeInner->SetLineStyle(2);
    gConeInner->Draw("L SAME");

    gConeOuter->SetLineColor(kRed);
    gConeOuter->SetLineWidth(2);
    gConeOuter->Draw("L SAME");

    TLegend *leg1 = new TLegend(0.55, 0.75, 0.84, 0.88);
    leg1->SetBorderSize(1);
    leg1->AddEntry(gConeInner, "Paroi interne (conique)", "l");
    leg1->AddEntry(gConeOuter, "Paroi externe (R=3.15mm)", "l");
    leg1->Draw();

    c1->SaveAs("compton_carte_zr.png");

    //==========================================================================
    // 2. HISTOGRAMME 2D : θ_incident vs θ_sortant
    //==========================================================================
    std::cout << "=== 2. θ_incident vs θ_sortant ===" << std::endl;

    TCanvas *c2 = new TCanvas("c2_compton", "theta_in vs theta_out", 1000, 800);
    c2->SetLogz();

    TH2D *h_theta_inout = new TH2D("h_theta_inout",
        "#theta_{incident} vs #theta_{sortant};#theta_{in} (deg);#theta_{out} (deg)",
        90, 0, 90, 90, 0, 90);
    tree->Draw("theta_out_deg:theta_in_deg>>h_theta_inout", "", "COLZ");
    h_theta_inout->SetMinimum(1);

    // Ligne diagonale θ_out = θ_in (pas de déviation)
    TLine *diag = new TLine(0, 0, 90, 90);
    diag->SetLineColor(kRed);
    diag->SetLineStyle(2);
    diag->SetLineWidth(2);
    diag->Draw("SAME");

    TLegend *leg2 = new TLegend(0.15, 0.75, 0.45, 0.88);
    leg2->SetBorderSize(1);
    leg2->AddEntry(diag, "#theta_{out} = #theta_{in} (pas de d#acute{e}viation)", "l");
    leg2->Draw();

    c2->SaveAs("compton_theta_in_vs_out.png");

    //==========================================================================
    // 3. HISTOGRAMME 2D : Angle de diffusion vs énergie incidente
    //==========================================================================
    std::cout << "=== 3. Angle de diffusion vs E_incident ===" << std::endl;

    TCanvas *c3 = new TCanvas("c3_compton", "Scatter angle vs E_in", 1000, 800);
    c3->SetLogz();

    TH2D *h_scatter_E = new TH2D("h_scatter_E",
        "Angle de diffusion Compton vs E_{incident};E_{incident} (keV);#theta_{scatter} (deg)",
        100, 0, 50, 180, 0, 180);
    tree->Draw("scatter_angle_deg:ekin_before_keV>>h_scatter_E", "", "COLZ");
    h_scatter_E->SetMinimum(1);

    c3->SaveAs("compton_scatter_angle_vs_E.png");

    //==========================================================================
    // 4. ΔE vs E_incident (avec courbes théoriques)
    //==========================================================================
    std::cout << "=== 4. ΔE vs E_incident ===" << std::endl;

    TCanvas *c4 = new TCanvas("c4_compton", "DeltaE vs E_in", 1200, 800);
    c4->SetLogz();

    TH2D *h_dE_E = new TH2D("h_dE_E",
        "#DeltaE vs E_{incident};E_{incident} (keV);#DeltaE = E_{in} - E_{out} (keV)",
        100, 0, 50, 100, 0, 10);
    tree->Draw("delta_ekin_keV:ekin_before_keV>>h_dE_E", "", "COLZ");
    h_dE_E->SetMinimum(1);

    // Courbe théorique : ΔE_max (backscattering, θ=180°)
    // E_out = E_in / (1 + 2*E_in/m_e c²)
    // ΔE_max = E_in - E_out = E_in * 2*E_in / (m_e c² + 2*E_in)
    TGraph *gDEmax = new TGraph();
    for (int i = 1; i <= 100; i++) {
        double E = 0.5 * i;  // keV
        double dE_max = E * 2.0 * E / (me_c2 + 2.0 * E);
        gDEmax->SetPoint(i-1, E, dE_max);
    }
    gDEmax->SetLineColor(kRed);
    gDEmax->SetLineWidth(2);
    gDEmax->SetLineStyle(1);
    gDEmax->Draw("L SAME");

    // Courbe théorique : ΔE pour θ=90°
    // E_out = E_in / (1 + E_in/m_e c²)
    // ΔE_90 = E_in² / (m_e c² + E_in)
    TGraph *gDE90 = new TGraph();
    for (int i = 1; i <= 100; i++) {
        double E = 0.5 * i;
        double dE_90 = E * E / (me_c2 + E);
        gDE90->SetPoint(i-1, E, dE_90);
    }
    gDE90->SetLineColor(kBlue);
    gDE90->SetLineWidth(2);
    gDE90->SetLineStyle(2);
    gDE90->Draw("L SAME");

    TLegend *leg4 = new TLegend(0.15, 0.70, 0.50, 0.88);
    leg4->SetBorderSize(1);
    leg4->AddEntry(gDEmax, "#DeltaE_{max} (#theta=180#circ, backscatter)", "l");
    leg4->AddEntry(gDE90, "#DeltaE (#theta=90#circ)", "l");
    leg4->Draw();

    c4->SaveAs("compton_deltaE_vs_E.png");

    //==========================================================================
    // 5. DISTRIBUTION DE L'ANGLE DE DIFFUSION
    //==========================================================================
    std::cout << "=== 5. Distribution de l'angle de diffusion ===" << std::endl;

    TCanvas *c5 = new TCanvas("c5_compton", "Scatter angle distribution", 1000, 700);
    c5->SetLogy();
    c5->SetGridx();
    c5->SetGridy();

    TH1D *h_scatter = new TH1D("h_scatter",
        "Distribution de l'angle de diffusion Compton;#theta_{scatter} (deg);Counts",
        180, 0, 180);
    tree->Draw("scatter_angle_deg>>h_scatter", "", "");
    h_scatter->SetLineColor(kBlue);
    h_scatter->SetLineWidth(2);
    h_scatter->SetFillColor(kAzure-4);
    h_scatter->SetFillStyle(3004);

    // Statistiques
    double meanAngle = h_scatter->GetMean();
    double rmsAngle = h_scatter->GetRMS();
    TPaveText *pave5 = new TPaveText(0.55, 0.70, 0.84, 0.88, "NDC");
    pave5->SetBorderSize(1);
    pave5->SetFillColor(kWhite);
    pave5->AddText(Form("#LT#theta_{scatter}#GT = %.1f#circ", meanAngle));
    pave5->AddText(Form("RMS = %.1f#circ", rmsAngle));
    pave5->AddText(Form("N_{entries} = %lld", (Long64_t)h_scatter->GetEntries()));
    pave5->Draw();

    c5->SaveAs("compton_scatter_angle_distribution.png");

    //==========================================================================
    // 6. SPECTRE EN ÉNERGIE AVANT/APRÈS COMPTON
    //==========================================================================
    std::cout << "=== 6. Spectre en énergie avant/après ===" << std::endl;

    TCanvas *c6 = new TCanvas("c6_compton", "Spectre E before/after", 1000, 700);
    c6->SetLogy();
    c6->SetGridx();
    c6->SetGridy();

    TH1D *h_Ebefore = new TH1D("h_Ebefore",
        "Spectre en #acute{e}nergie des Compton;E (keV);Counts",
        100, 0, 50);
    TH1D *h_Eafter = new TH1D("h_Eafter", "", 100, 0, 50);
    TH1D *h_deltaE = new TH1D("h_deltaE", "", 100, 0, 10);

    tree->Draw("ekin_before_keV>>h_Ebefore", "", "");
    tree->Draw("ekin_after_keV>>h_Eafter", "", "");

    h_Ebefore->SetLineColor(kBlue);
    h_Ebefore->SetLineWidth(2);
    h_Eafter->SetLineColor(kRed);
    h_Eafter->SetLineWidth(2);
    h_Eafter->SetLineStyle(2);

    double maxY6 = std::max(h_Ebefore->GetMaximum(), h_Eafter->GetMaximum()) * 1.5;
    h_Ebefore->SetMaximum(maxY6);
    h_Ebefore->Draw("HIST");
    h_Eafter->Draw("HIST SAME");

    TLegend *leg6 = new TLegend(0.55, 0.75, 0.84, 0.88);
    leg6->SetBorderSize(1);
    leg6->AddEntry(h_Ebefore, "E_{avant Compton}", "l");
    leg6->AddEntry(h_Eafter, "E_{apr#grave{e}s Compton}", "l");
    leg6->Draw();

    c6->SaveAs("compton_spectre_energie.png");

    //==========================================================================
    // 7. PERTE D'ÉNERGIE RELATIVE ΔE/E vs E_incident
    //==========================================================================
    std::cout << "=== 7. Perte relative ΔE/E vs E ===" << std::endl;

    TCanvas *c7 = new TCanvas("c7_compton", "DeltaE/E vs E", 1000, 700);
    c7->SetLogz();

    TH2D *h_fracE = new TH2D("h_fracE",
        "Perte relative d'#acute{e}nergie;E_{incident} (keV);#DeltaE/E_{incident} (%)",
        100, 0, 50, 100, 0, 20);
    tree->Draw("100*delta_ekin_keV/ekin_before_keV:ekin_before_keV>>h_fracE", 
               "ekin_before_keV>0", "COLZ");
    h_fracE->SetMinimum(1);

    // Courbe théorique ΔE_max/E = 2E/(m_e c² + 2E)
    TGraph *gFracMax = new TGraph();
    for (int i = 1; i <= 100; i++) {
        double E = 0.5 * i;
        double frac_max = 100.0 * 2.0 * E / (me_c2 + 2.0 * E);
        gFracMax->SetPoint(i-1, E, frac_max);
    }
    gFracMax->SetLineColor(kRed);
    gFracMax->SetLineWidth(2);
    gFracMax->Draw("L SAME");

    TLegend *leg7 = new TLegend(0.15, 0.80, 0.50, 0.88);
    leg7->SetBorderSize(1);
    leg7->AddEntry(gFracMax, "#DeltaE_{max}/E (#theta=180#circ)", "l");
    leg7->Draw();

    c7->SaveAs("compton_perte_relative_vs_E.png");

    //==========================================================================
    // 8. FIGURE RÉCAPITULATIVE
    //==========================================================================
    std::cout << "\n=== 8. Figure récapitulative ===" << std::endl;

    TCanvas *c8 = new TCanvas("c8_compton", "Récapitulatif Compton", 1800, 1200);
    c8->Divide(3, 2);

    c8->cd(1);
    h_zr->Draw("COLZ");
    gConeInner->Draw("L SAME");
    gConeOuter->Draw("L SAME");

    c8->cd(2);
    gPad->SetLogz();
    h_theta_inout->Draw("COLZ");
    diag->Draw("SAME");

    c8->cd(3);
    gPad->SetLogz();
    h_scatter_E->Draw("COLZ");

    c8->cd(4);
    gPad->SetLogz();
    h_dE_E->Draw("COLZ");
    gDEmax->Draw("L SAME");
    gDE90->Draw("L SAME");

    c8->cd(5);
    gPad->SetLogy();
    h_scatter->Draw("HIST");

    c8->cd(6);
    gPad->SetLogy();
    h_Ebefore->Draw("HIST");
    h_Eafter->Draw("HIST SAME");
    leg6->Draw();

    c8->SaveAs("compton_recapitulatif.png");

    //==========================================================================
    // 9. VÉRIFICATION FORMULE COMPTON : ΔE mesuré vs ΔE théorique
    //==========================================================================
    std::cout << "\n=== 9. Vérification formule Compton ===" << std::endl;

    TCanvas *c9 = new TCanvas("c9_compton", "Verification Compton formula", 1000, 800);

    // ΔE_théorique = E_in * (1 - 1/(1 + E_in/m_e c² * (1 - cos θ)))
    // = E_in * E_in * (1 - cos θ) / (m_e c² + E_in * (1 - cos θ))
    TH2D *h_verif = new TH2D("h_verif_compton",
        "V#acute{e}rification formule Compton;#DeltaE th#acute{e}orique (keV);#DeltaE mesur#acute{e} (keV)",
        100, 0, 5, 100, 0, 5);

    // Calculer ΔE théorique à partir de E_in et cos_scatter
    double ekin_before, cos_scatter_val, delta_ekin;
    tree->SetBranchAddress("ekin_before_keV", &ekin_before);
    tree->SetBranchAddress("cos_scatter", &cos_scatter_val);
    tree->SetBranchAddress("delta_ekin_keV", &delta_ekin);

    for (Long64_t i = 0; i < nEntries; i++) {
        tree->GetEntry(i);
        double dE_theo = ekin_before * ekin_before * (1.0 - cos_scatter_val) 
                         / (me_c2 + ekin_before * (1.0 - cos_scatter_val));
        h_verif->Fill(dE_theo, delta_ekin);
    }

    h_verif->Draw("COLZ");

    // Ligne y = x (accord parfait)
    TLine *line_verif = new TLine(0, 0, 5, 5);
    line_verif->SetLineColor(kRed);
    line_verif->SetLineWidth(2);
    line_verif->Draw("SAME");

    TLegend *leg9 = new TLegend(0.15, 0.78, 0.50, 0.88);
    leg9->SetBorderSize(1);
    leg9->AddEntry(line_verif, "#DeltaE_{mesur#acute{e}} = #DeltaE_{th#acute{e}o} (Compton)", "l");
    leg9->Draw();

    c9->SaveAs("compton_verification_formule.png");

    //==========================================================================
    // Résumé
    //==========================================================================
    std::cout << "\n=== Résumé ===" << std::endl;
    std::cout << "  Entrées Compton : " << nEntries << std::endl;
    std::cout << "  E_before moyen  : " << h_Ebefore->GetMean() << " keV" << std::endl;
    std::cout << "  E_after moyen   : " << h_Eafter->GetMean() << " keV" << std::endl;
    std::cout << "  ΔE moyen        : " << h_Ebefore->GetMean() - h_Eafter->GetMean() << " keV" << std::endl;
    std::cout << "  θ_scatter moyen : " << h_scatter->GetMean() << "°" << std::endl;

    std::cout << "\n=== Fichiers générés ===" << std::endl;
    std::cout << "  - compton_carte_zr.png" << std::endl;
    std::cout << "  - compton_theta_in_vs_out.png" << std::endl;
    std::cout << "  - compton_scatter_angle_vs_E.png" << std::endl;
    std::cout << "  - compton_deltaE_vs_E.png" << std::endl;
    std::cout << "  - compton_scatter_angle_distribution.png" << std::endl;
    std::cout << "  - compton_spectre_energie.png" << std::endl;
    std::cout << "  - compton_perte_relative_vs_E.png" << std::endl;
    std::cout << "  - compton_recapitulatif.png" << std::endl;
    std::cout << "  - compton_verification_formule.png" << std::endl;

    f->Close();
    std::cout << "\n=== Analyse Compton terminée ===" << std::endl;
}
