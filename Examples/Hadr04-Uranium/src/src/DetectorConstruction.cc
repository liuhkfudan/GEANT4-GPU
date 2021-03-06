//
// ********************************************************************
// * License and Disclaimer                                           *
// *                                                                  *
// * The  Geant4 software  is  copyright of the Copyright Holders  of *
// * the Geant4 Collaboration.  It is provided  under  the terms  and *
// * conditions of the Geant4 Software License,  included in the file *
// * LICENSE and available at  http://cern.ch/geant4/license .  These *
// * include a list of copyright holders.                             *
// *                                                                  *
// * Neither the authors of this software system, nor their employing *
// * institutes,nor the agencies providing financial support for this *
// * work  make  any representation or  warranty, express or implied, *
// * regarding  this  software system or assume any liability for its *
// * use.  Please see the license in the file  LICENSE  and URL above *
// * for the full disclaimer and the limitation of liability.         *
// *                                                                  *
// * This  code  implementation is the result of  the  scientific and *
// * technical work of the GEANT4 collaboration.                      *
// * By using,  copying,  modifying or  distributing the software (or *
// * any work based  on the software)  you  agree  to acknowledge its *
// * use  in  resulting  scientific  publications,  and indicate your *
// * acceptance of all terms of the Geant4 Software license.          *
// ********************************************************************
//
/// \file hadronic/Hadr03/src/DetectorConstruction.cc
/// \brief Implementation of the DetectorConstruction class
//
// $Id: DetectorConstruction.cc 70755 2013-06-05 12:17:48Z ihrivnac $
//

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......
//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

#include "DetectorConstruction.hh"
#include "DetectorMessenger.hh"
#include "G4Material.hh"
#include "G4NistManager.hh"

#include "G4Box.hh"
#include "G4LogicalVolume.hh"
#include "G4PVPlacement.hh"

#include "G4GeometryManager.hh"
#include "G4PhysicalVolumeStore.hh"
#include "G4LogicalVolumeStore.hh"
#include "G4SolidStore.hh"
#include "G4RunManager.hh"

#include "G4UnitsTable.hh"
#include "G4SystemOfUnits.hh"

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

DetectorConstruction::DetectorConstruction()
:G4VUserDetectorConstruction(),
 fPBox(0), fLBox(0), fMaterial(0), fDetectorMessenger(0)
{
  fBoxSize = 1*m;
  DefineMaterials();
  SetMaterial("Uranium_a");  
  fDetectorMessenger = new DetectorMessenger(this);
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

DetectorConstruction::~DetectorConstruction() { 
  delete fDetectorMessenger;
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

G4VPhysicalVolume* DetectorConstruction::Construct()
{
  return ConstructVolumes();
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

void DetectorConstruction::DefineMaterials()
{
  G4String  name, symbol;
G4double density, fractionmass , abundance;
G4int ncomponents, natoms, iz , a,  n;
G4NistManager* man = G4NistManager::Instance();
  
  // specific element name for thermal neutronHP
  // (see G4NeutronHPThermalScatteringNames.cc)
  // G4Element* H  = new G4Element("TS_H_of_Water" ,"H" , 1., 1.0079*g/mole);
  // G4Element* O  = new G4Element("Oxygen"        ,"O" , 8., 16.00*g/mole);

  // G4Material* H2O =
  // new G4Material("Water_ts", 1.000*g/cm3, ncomponents=2,
  //                        kStateLiquid, 593*kelvin, 150*bar);
  // H2O->AddElement(H, natoms=2);
  // H2O->AddElement(O, natoms=1);
  // H2O->GetIonisation()->SetMeanExcitationEnergy(78.0*eV);

  // // graphite
  // G4Isotope* C12 = new G4Isotope("C12", 6, 12);
  // G4Element* elC = new G4Element("TS_C_of_Graphite","C", ncomponents=1);
  // elC->AddIsotope(C12, 100.*perCent);
  // G4Material* graphite =
  // new G4Material("graphite", 2.27*g/cm3, ncomponents=1,
  //                        kStateSolid, 293*kelvin, 1*atmosphere);
  // graphite->AddElement(elC, natoms=1);

 ///G4cout << *(G4Material::GetMaterialTable()) << G4endl;

  G4Material* Uranium_Nist;
  G4Material* Uranium_a;
  G4Material* Uranium_b;

  Uranium_Nist =man->FindOrBuildMaterial("G4_U");
  G4Isotope* U4 = new G4Isotope("U234",92,234,234.04*g/mole);
  G4Isotope* U5 = new G4Isotope("U235",92,235,235.04*g/mole);
  G4Isotope* U8 = new G4Isotope("U238",92,238,238.05*g/mole);

  G4Element*  Uela=new G4Element(name="Uranium_element_a",symbol="Uela",ncomponents=3);
  Uela->AddIsotope(U4,abundance=0.01 *perCent);
  Uela->AddIsotope(U5,abundance=0.72 *perCent);
  Uela->AddIsotope(U8,abundance=99.27*perCent);

  G4Element*  Uelb=new G4Element(name="Uranium_element_b",symbol="Uelb",ncomponents=3);
  Uelb->AddIsotope(U4,abundance=0.01 *perCent);
  Uelb->AddIsotope(U4,abundance=99.27 *perCent);
  Uelb->AddIsotope(U4,abundance=0.72 *perCent);

  Uranium_a=new G4Material("Uranium_a",18.95*g/cm3,1);
  Uranium_a->AddElement(Uela,fractionmass=1.0);

  Uranium_b=new G4Material("Uranium_b",18.95*g/cm3,1);
  Uranium_b->AddElement(Uelb,fractionmass=1.0);

  G4cout<<" Uranium_a is  "<<G4endl<<    Uranium_a   <<G4endl;
  G4cout<<" Uranium_b is  "<<G4endl<<    Uranium_b   <<G4endl;
  G4cout<<" Uranium_Nist is  "<<G4endl<<    Uranium_Nist   <<G4endl;
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

G4Material* DetectorConstruction::MaterialWithSingleIsotope( G4String name,
                           G4String symbol, G4double density, G4int Z, G4int A)
{
 // define a material from an isotope
 //
 G4int ncomponents;
 G4double abundance, massfraction;

 G4Isotope* isotope = new G4Isotope(symbol, Z, A);
 
 G4Element* element  = new G4Element(name, symbol, ncomponents=1);
 element->AddIsotope(isotope, abundance= 100.*perCent);
 
 G4Material* material = new G4Material(name, density, ncomponents=1);
 material->AddElement(element, massfraction=100.*perCent);

 return material;
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

G4VPhysicalVolume* DetectorConstruction::ConstructVolumes()
{
  // Cleanup old geometry
  G4GeometryManager::GetInstance()->OpenGeometry();
  G4PhysicalVolumeStore::GetInstance()->Clean();
  G4LogicalVolumeStore::GetInstance()->Clean();
  G4SolidStore::GetInstance()->Clean();

  G4Box*
  sBox = new G4Box("Container",                         //its name
                   fBoxSize/2,fBoxSize/2,fBoxSize/2);   //its dimensions

  fLBox = new G4LogicalVolume(sBox,                     //its shape
                             fMaterial,                 //its material
                             fMaterial->GetName());     //its name

  fPBox = new G4PVPlacement(0,                          //no rotation
                            G4ThreeVector(),            //at (0,0,0)
                            fLBox,                      //its logical volume
                            fMaterial->GetName(),       //its name
                            0,                          //its mother  volume
                            false,                      //no boolean operation
                            0);                         //copy number
                           
  PrintParameters();
  
  //always return the root volume
  //
  return fPBox;
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

void DetectorConstruction::PrintParameters()
{
  G4cout << "\n The Box is " << G4BestUnit(fBoxSize,"Length")
         << " of " << fMaterial->GetName() 
         << "\n \n" << fMaterial << G4endl;
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

void DetectorConstruction::SetMaterial(G4String materialChoice)
{
  // search the material by its name
  G4Material* pttoMaterial =
     G4NistManager::Instance()->FindOrBuildMaterial(materialChoice);   
  
  if (pttoMaterial) { 
    if(fMaterial != pttoMaterial) {
      fMaterial = pttoMaterial;
      if(fLBox) { fLBox->SetMaterial(pttoMaterial); }
      G4RunManager::GetRunManager()->PhysicsHasBeenModified();
    }
  } else {
    G4cout << "\n--> warning from DetectorConstruction::SetMaterial : "
           << materialChoice << " not found" << G4endl;
  }              
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

void DetectorConstruction::SetSize(G4double value)
{
  fBoxSize = value;
  G4RunManager::GetRunManager()->ReinitializeGeometry();
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

