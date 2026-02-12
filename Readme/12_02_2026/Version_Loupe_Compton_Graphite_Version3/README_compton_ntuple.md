# Validation du Run Geant4 & Ntuple Compton

## Validation du run (1M événements) — 10/10 checks ✅

| # | Check | Résultat |
|---|-------|----------|
| 1 | Statistique Poisson (σ/√N = 1.10 ∈ [0.8, 1.3]) | ✅ |
| 2 | Edep_total == Σ Edep_rings (99 batches, tol. 0.01 keV) | ✅ |
| 3 | Conversion Edep→Dose par batch (écart 0.0002%) | ✅ |
| 4 | Conversion Edep→Dose par anneau (5 anneaux, écart < 0.003%) | ✅ |
| 5 | phot == Σ matériaux (987,631 = 987,631) | ✅ |
| 6 | rows == outward SpecSD (12,658) | ✅ |
| 7 | Conservation particules (N = phot + out + résidu) | ✅ |
| 8 | Σ Edep anneaux = Total run (diff = 0.0000 keV) | ✅ |
| 9 | ΔE ≤ ΔE_max Compton (101 entrées, 0 violations) | ✅ |
| 10 | Positions Compton dans cône [1.9, 16.95] mm | ✅ |

### Chiffres clés du run

- **Transmis (z=18mm)** : 12,658 / 1M = 1.266%
- **Compton dans cône** : ~10,001 (1.000% des primaires)
- **Redirigés Compton** : ~200 (1.6% des transmis seulement)
- **Ratio Compton/Photoélectrique dans graphite** : 3.0% → le photoélectrique domine
- **Dose totale eau** : 6.567 nGy

## Fichiers modifiés

### 1. `include/AnalysisManagerSetup.hh`
- Ajout de la déclaration `int GetComptonConeNtupleId();`

### 2. `src/AnalysisManagerSetup.cc`
- Variable statique `g_comptonConeNtupleId`
- Nouveau ntuple **`compton_cone_events`** (16 colonnes) :

| Col | Nom | Type | Description |
|-----|-----|------|-------------|
| 0 | eventID | int | ID événement |
| 1 | trackID | int | ID du track |
| 2 | n_compton_seq | int | N° séquentiel Compton pour ce track |
| 3 | ekin_before_keV | double | Énergie avant diffusion |
| 4 | ekin_after_keV | double | Énergie après diffusion |
| 5 | delta_ekin_keV | double | Perte d'énergie ΔE |
| 6 | x_mm | double | Position X |
| 7 | y_mm | double | Position Y |
| 8 | z_mm | double | Position Z |
| 9 | r_mm | double | Rayon √(x²+y²) |
| 10 | theta_in_deg | double | Angle polaire incident |
| 11 | phi_in_deg | double | Angle azimutal incident |
| 12 | theta_out_deg | double | Angle polaire sortant |
| 13 | phi_out_deg | double | Angle azimutal sortant |
| 14 | scatter_angle_deg | double | Angle de diffusion Compton |
| 15 | cos_scatter | double | cos(angle de diffusion) |

- Getter `GetComptonConeNtupleId()`

### 3. `src/SteppingAction.cc`
- Bloc Compton enrichi : au lieu de simplement tagger le track via `MyTrackInfo`,
  il remplit maintenant le ntuple `compton_cone_events` à **chaque** interaction Compton
- Calcul des directions `prePoint->GetMomentumDirection()` et `postPoint->GetMomentumDirection()`
- Angle de diffusion : `cos_scatter = dirIn · dirOut`, `scatter_angle = acos(cos_scatter)`

### 4. `analyse_compton_cone.C` (NOUVEAU)
Script ROOT produisant 9 figures :
1. Carte (z, r) avec contour du cône
2. θ_incident vs θ_sortant (2D)
3. Angle de diffusion vs E_incident (2D)
4. ΔE vs E_incident avec courbes théoriques (180° et 90°)
5. Distribution de l'angle de diffusion
6. Spectre énergie avant/après Compton
7. ΔE/E vs E_incident avec courbe ΔE_max/E théorique
8. Figure récapitulative (6 panneaux)
9. Vérification formule Compton (ΔE_mesuré vs ΔE_théorique)

## Utilisation

```bash
# Après recompilation et lancement du run :
root -l -b -q 'analyse_compton_cone.C("output.root")'
```

## Estimation du volume de données

Avec ~10,000 Compton/1M événements × 16 colonnes :
- ~160,000 valeurs par run de 1M
- Empreinte dans output.root : ~1-2 MB (négligeable)
