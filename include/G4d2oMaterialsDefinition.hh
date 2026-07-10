#ifndef G4d2oMaterialsDefinition_H
#define G4d2oMaterialsDefinition_H 1

#include "G4Isotope.hh"
#include "G4Element.hh"
#include "G4Material.hh"
#include "G4NistManager.hh"
#include "G4SystemOfUnits.hh"
#include "G4PhysicalConstants.hh"
#include "G4VPhysicalVolume.hh"
#include "G4SurfaceProperty.hh"
#include "inputVariables.hh"

class TFile;

enum materialName
{
    AIR, VINYLTOLUENE, ALUMINUM, LEAD, POLY, STEEL,
    PMMA, MUMETAL, COPPER, PLASTIC, VACUUM, H2O, D2O,
    FUSEDSILICA, BOROSILICATE, PHOTOCATHODE, TEFLON,
    TYVEK, DELRIN, EPOXY, CONCRETE
};

class G4d2oMaterialsDefinition
{
public:
    G4d2oMaterialsDefinition();
    ~G4d2oMaterialsDefinition();

    G4Material* GetMaterial(materialName matName);

    static void SetReflector(G4VPhysicalVolume* theExitingVolume,
                             G4VPhysicalVolume* theEnteringVolume,
                             G4double theReflectivity,
                             G4double theSigmaAlpha = 0.0,
                             G4SurfaceType type = dielectric_dielectric);

    static void SetTyvekReflector(G4VPhysicalVolume* theExitingVolume,
                                  G4VPhysicalVolume* theEnteringVolume,
                                  G4double theReflectivityScale,
                                  G4double theSpecularFraction,
                                  G4double theSigmaAlpha = 0.2,
                                  G4SurfaceType type = dielectric_dielectric);

protected:
    inputVariables* input;
    G4double h2oRefl;

private:
    G4NistManager* manager;
    G4Material *matAir, *matVinylToluene, *matAl, *matSteel;
    G4Material *matPb, *matPoly, *matPMMA, *matMuMetal, *matCopper;
    G4Material *matPlastic, *matVacuum, *matH2O, *matD2O;
    G4Material *matFusedSilica, *matBorosilicate, *matPhotoCathode;
    G4Material *matTeflon, *matTyvek, *matDelrin, *matEpoxy, *matConcrete;

    void SetUniformOpticalProperties(G4Material* theMat, G4double theIndex, G4double theAbsLength);
    void SetOpticalProperties(G4Material* theMat, G4double theIndex, G4double h2oReflF, G4String absLengthFile);
};

#endif
