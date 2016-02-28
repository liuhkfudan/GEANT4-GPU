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
//
// 070606 fix with Valgrind by T. Koi
// 080409 Fix div0 error with G4FPE by T. Koi
// 080811 Comment out unused method SetBlocked and SetBuffered
//        Add required cleaning up in CleanUp by T. Koi
//
// P. Arce, June-2014 Conversion neutron_hp to particle_hp
//
#ifndef G4ParticleHPVector_h
#define G4ParticleHPVector_h 1

#include "G4ParticleHPDataPoint.hh"
#include "G4PhysicsVector.hh"
#include "G4ParticleHPInterpolator.hh"
#include "Randomize.hh"
#include "G4ios.hh"
#include <fstream>
#include "G4InterpolationManager.hh"
#include "G4Exp.hh"
#include "G4Log.hh"
#include "G4Pow.hh"
#include "G4ParticleHPInterpolator.hh"
#include "G4ParticleHPHash.hh"
#include <cmath>
#include <vector>

#define GEANT4_ENABLE_CUDA 1
#if GEANT4_ENABLE_CUDA
#include "/u50/pagnanmm/GEANT4-GPU/geant4.10.02/source/externals/cuda/include/G4ParticleHPVector_CUDA.hh"
#endif

#if defined WIN32-VC
  #include <float.h>
#endif

class G4ParticleHPVector
{
  friend G4ParticleHPVector & operator + (G4ParticleHPVector & left, G4ParticleHPVector & right);


  /******************************************
   * PUBLIC
   ******************************************/
  public:

  G4ParticleHPVector();
  G4ParticleHPVector(G4int n);
  ~G4ParticleHPVector();

  G4ParticleHPVector & operator = (const G4ParticleHPVector & right);
  G4double GetXsec(G4double e);
  void GetXsecBuffer(G4double * queryList, G4int length);
  void Dump();
  void ThinOut(G4double precision);
  void Merge(G4InterpolationScheme aScheme, G4double aValue, G4ParticleHPVector * active, G4ParticleHPVector * passive);
  G4double Sample(); // Samples X according to distribution Y
  G4double Get15percentBorder();
  G4double Get50percentBorder();


  /******************************************
   * Getters
   ******************************************/
  inline const G4ParticleHPDataPoint & GetPoint(G4int i) const
  {
    #if GEANT4_ENABLE_CUDA
      return cudaVector->GetPoint(i);
    #else
      return theData[i];
    #endif
  }

  inline G4int GetVectorLength() const
  {
    #if GEANT4_ENABLE_CUDA
      return cudaVector->GetVectorLength();
    #else
      return nEntries;
    #endif
  }

  inline G4double GetEnergy(G4int i) const
  {
    #if GEANT4_ENABLE_CUDA
      return cudaVector->GetX(i);
    #else
      return theData[i].GetX();
    #endif
  }

  inline G4double GetX(G4int i) const
  {
    #if GEANT4_ENABLE_CUDA
      return cudaVector->GetX(i);
    #else
      if (i<0) {
        i=0;
      }
      if(i>=GetVectorLength()) {
        i=GetVectorLength()-1;
      }
      return theData[i].GetX();
    #endif
  }

  inline G4double GetXsec(G4int i)
  {
    #if GEANT4_ENABLE_CUDA
      return cudaVector->GetY(i);
    #else
      return theData[i].GetY();
    #endif
  }

  // ---- Can put on GPU
  G4double GetXsec(G4double e, G4int min)
  {
    #if GEANT4_ENABLE_CUDA
      return cudaVector->GetXsec(e, min);
    #else
      G4int i;
      for(i=min ; i<nEntries; i++)
      {
        if(theData[i].GetX()>e) {
          break;
        }
      }
      G4int low = i-1;
      G4int high = i;
      if(i==0)
      {
        low = 0;
        high = 1;
      }
      else if(i==nEntries)
      {
        low = nEntries-2;
        high = nEntries-1;
      }
      G4double y;
      if(e<theData[nEntries-1].GetX())
      {
        // Protect against doubled-up x values
        if( (theData[high].GetX()-theData[low].GetX())/theData[high].GetX() < 0.000001)
        {
          y = theData[low].GetY();
        }
        else
        {
          y = theInt.Interpolate(theManager.GetScheme(high), e,
                                 theData[low].GetX(), theData[high].GetX(),
                                 theData[low].GetY(), theData[high].GetY());
        }
      }
      else
      {
        y=theData[nEntries-1].GetY();
      }
      return y;
    #endif
  }

  inline G4double GetY(G4double x)
  {
    #if GEANT4_ENABLE_CUDA
      return cudaVector->GetXsec(x);
    #else
      return GetXsec(x);
    #endif
  }

  inline G4double GetY(G4int i)
  {
    #if GEANT4_ENABLE_CUDA
      return cudaVector->GetY(i);
    #else
      if (i<0) {
        i=0;
      }
      if(i>=GetVectorLength()){
        i=GetVectorLength()-1;
      }
      return theData[i].GetY();
    #endif
  }

  inline G4double GetY(G4int i) const
  {
    #if GEANT4_ENABLE_CUDA
      return cudaVector->GetY(i);
    #else
      if (i<0) {
        i=0;
      }
      if(i>=GetVectorLength()){
        i=GetVectorLength()-1;
      }
      return theData[i].GetY();
    #endif
  }
  // Should be able to run this on GPU
  G4double GetMeanX()
  {
    #if GEANT4_ENABLE_CUDA
      return cudaVector->GetMeanX();
    #else
      G4double result;
      G4double running = 0;
      G4double weighted = 0;
      for(G4int i=1; i<nEntries; i++)
      {
        running += theInt.GetBinIntegral(theManager.GetScheme(i-1),
                             theData[i-1].GetX(), theData[i].GetX(),
                             theData[i-1].GetY(), theData[i].GetY());
        weighted += theInt.GetWeightedBinIntegral(theManager.GetScheme(i-1),
                             theData[i-1].GetX(), theData[i].GetX(),
                             theData[i-1].GetY(), theData[i].GetY());
      }
      result = weighted / running;
      return result;
    #endif
  }

  inline G4double GetLabel()
  {
    #if GEANT4_ENABLE_CUDA
      return cudaVector->GetLabel();
    #else
      return label;
    #endif
  }

  inline G4double GetIntegral() // linear interpolation; use with care
  {
    #if GEANT4_ENABLE_CUDA
      return cudaVector->GetIntegral();
    #else
      if(totalIntegral<-0.5) {
        Integrate();
      }
      return totalIntegral;
    #endif
  }

  inline const G4InterpolationManager & GetInterpolationManager() const
  {
    #if GEANT4_ENABLE_CUDA
      return cudaVector->GetInterpolationManager();
    #else
      return theManager;
    #endif
  }

  inline G4InterpolationScheme GetScheme(G4int anIndex)
  {
    #if GEANT4_ENABLE_CUDA
      return cudaVector->GetScheme(anIndex);
    #else
      return theManager.GetScheme(anIndex);
    #endif
  }


  /******************************************
   * Setters
   ******************************************/
  inline void SetVerbose(G4int ff)
  {
    #if GEANT4_ENABLE_CUDA
      cudaVector->SetVerbose(ff);
    #else
      Verbose = ff;
    #endif
  }

  inline void SetPoint(G4int i, const G4ParticleHPDataPoint & it)
  {
    #if GEANT4_ENABLE_CUDA
      cudaVector->SetPoint(i, it);
    #else
      G4double x = it.GetX();
      G4double y = it.GetY();
      SetData(i, x, y);
    #endif
  }

  inline void SetData(G4int i, G4double x, G4double y)
  {
    #if GEANT4_ENABLE_CUDA
      cudaVector->SetData(i,x,y);
    #else
      Check(i);
      if(y>maxValue) {
        maxValue=y;
      }
      theData[i].SetData(x, y);
    #endif
  }

  inline void SetX(G4int i, G4double e)
  {
    #if GEANT4_ENABLE_CUDA
      cudaVector->SetX(i,e);
    #else
      Check(i);
      theData[i].SetX(e);
    #endif
  }

  inline void SetEnergy(G4int i, G4double e)
  {
    #if GEANT4_ENABLE_CUDA
      cudaVector->SetEnergy(i,e);
    #else
      Check(i);
      theData[i].SetX(e);
    #endif
  }

  inline void SetY(G4int i, G4double x)
  {
    #if GEANT4_ENABLE_CUDA
      cudaVector->SetY(i,x);
    #else
      Check(i);
      if(x>maxValue) maxValue=x;
      theData[i].SetY(x);
    #endif
  }

  inline void SetXsec(G4int i, G4double x)
  {
    #if GEANT4_ENABLE_CUDA
      cudaVector->SetXsec(i,x);
    #else
      Check(i);
      if(x>maxValue) {
        maxValue=x;
      }
      theData[i].SetY(x);
    #endif
  }

  inline void SetLabel(G4double aLabel)
  {
    #if GEANT4_ENABLE_CUDA
      cudaVector->SetLabel(aLabel);
    #else
      label = aLabel;
    #endif
  }

  inline void SetInterpolationManager(const G4InterpolationManager & aManager)
  {
    #if GEANT4_ENABLE_CUDA
      cudaVector->SetInterpolationManager(aManager);
    #else
      theManager = aManager;
    #endif
  }

  inline void SetInterpolationManager(G4InterpolationManager & aMan)
  {
    #if GEANT4_ENABLE_CUDA
      cudaVector->SetInterpolationManager(aMan);
    #else
      theManager = aMan;
    #endif
  }

  inline void SetScheme(G4int aPoint, const G4InterpolationScheme & aScheme)
  {
    #if GEANT4_ENABLE_CUDA
      cudaVector->SetScheme(aPoint, aScheme);
    #else
      theManager.AppendScheme(aPoint, aScheme);
    #endif
  }


  /******************************************
   * Computations
   ******************************************/
  void Init(std::istream & aDataFile, G4int total, G4double ux=1., G4double uy=1.)
  {
    #if GEANT4_ENABLE_CUDA
      cudaVector->Init(aDataFile, total, ux, uy);
    #else
      G4double x,y;
      for (G4int i=0;i<total;i++)
      {
        aDataFile >> x >> y;
        x*=ux;
        y*=uy;
        SetData(i,x,y);
        if(0 == nEntries%10)
        {
          theHash.SetData(nEntries-1, x, y);
        }
      }
    #endif
  }

  void Init(std::istream & aDataFile,G4double ux=1., G4double uy=1.)
  {
	G4int querySize =  1000;
	G4double *queryList;
	queryList = (G4double *)malloc(querySize*sizeof(G4double));
	for(int i = 0; i < querySize; i++){
		queryList[i] = (G4double)i/1000;
	}
  
    #if GEANT4_ENABLE_CUDA
      cudaVector->Init(aDataFile, ux, uy);
	  if(cudaVector->nEntries > querySize){
		// cudaVector->GetXsecBuffer(queryList, querySize);
		// for(int i = 0; i < querySize; i++){
			// G4double GPUres = queryList[i];
			// G4double CPUres = GetXsec(queryList[i]);
			// printf("GPU: %0.5e CPU: %0.5e \n", GPUres, CPUres);
		// }
	  }
    #else
      G4int total;
      aDataFile >> total;
      if(theData!=0) {
        delete [] theData;
      }
      theData = new G4ParticleHPDataPoint[total];
      nPoints=total;
      nEntries=0;
      theManager.Init(aDataFile);
      Init(aDataFile, total, ux, uy);
    #endif
  }

  inline void InitInterpolation(std::istream & aDataFile)
  {
    #if GEANT4_ENABLE_CUDA
      cudaVector->InitInterpolation(aDataFile);
    #else
      theManager.Init(aDataFile);
    #endif
  }

  void Hash()
  {
    #if GEANT4_ENABLE_CUDA
      // no hashing on cuda
    #else
      G4int i;
      G4double x, y;
      for(i=0 ; i<nEntries; i++)
      {
        if(0 == (i+1)%10)
        {
          x = GetX(i);
          y = GetY(i);
          theHash.SetData(i, x, y);
        }
      }
    #endif
  }

  void ReHash()
  {
    #if GEANT4_ENABLE_CUDA
      // no hasing on cuda
    #else
      theHash.Clear();
      Hash();
    #endif
  }

  inline void CleanUp()
  {
    #if GEANT4_ENABLE_CUDA
      cudaVector->CleanUp();
    #else
      nEntries=0;
      theManager.CleanUp();
      maxValue = -DBL_MAX;
      theHash.Clear();
      //080811 TK DB
      delete[] theIntegral;
      theIntegral = NULL;
    #endif
  }

  // merges the vectors active and passive into *this
  inline void Merge(G4ParticleHPVector * active, G4ParticleHPVector * passive)
  {
    #if GEANT4_ENABLE_CUDA
      cudaVector->Merge(&(*(active)->cudaVector), &(*(passive)->cudaVector));
    #else
      CleanUp();
      G4int s_tmp = 0, n=0, m_tmp=0;
      G4ParticleHPVector * tmp;
      G4int a = s_tmp, p = n, t;
      while (a<active->GetVectorLength()&&p<passive->GetVectorLength()) // Loop checking, 11.05.2015, T. Koi
      {
        if(active->GetEnergy(a) <= passive->GetEnergy(p))
        {
          G4double xa = active->GetEnergy(a);
          G4double yy = active->GetXsec(a);
          SetData(m_tmp, xa, yy);
          theManager.AppendScheme(m_tmp, active->GetScheme(a));
          m_tmp++;
          a++;
          G4double xp = passive->GetEnergy(p);

          //080409 TKDB
          //if( std::abs(std::abs(xp-xa)/xa)<0.001 ) p++;
          if ( !( xa == 0 ) && std::abs(std::abs(xp-xa)/xa)<0.001 ) p++;
        } else {
          tmp = active;
          t=a;
          active = passive;
          a=p;
          passive = tmp;
          p=t;
        }
      }
      while (a!=active->GetVectorLength()) // Loop checking, 11.05.2015, T. Koi
      {
        SetData(m_tmp, active->GetEnergy(a), active->GetXsec(a));
        theManager.AppendScheme(m_tmp++, active->GetScheme(a));
        a++;
      }
      while (p!=passive->GetVectorLength()) // Loop checking, 11.05.2015, T. Koi
      {
        if(std::abs(GetEnergy(m_tmp-1)-passive->GetEnergy(p))/passive->GetEnergy(p)>0.001)
        //if(std::abs(GetEnergy(m)-passive->GetEnergy(p))/passive->GetEnergy(p)>0.001)
        {
          SetData(m_tmp, passive->GetEnergy(p), passive->GetXsec(p));
          theManager.AppendScheme(m_tmp++, active->GetScheme(p));
        }
        p++;
      }
    #endif
  }

  G4double SampleLin() // Samples X according to distribution Y, linear int
  {
    #if GEANT4_ENABLE_CUDA
      return cudaVector->SampleLin();
    #else
      G4double result;
      if(theIntegral==0) {
        IntegrateAndNormalise();
      }
      if(GetVectorLength()==1)
      {
        result = theData[0].GetX();
      }
      else
      {
        G4int i;
        G4double rand = G4UniformRand();

        // this was replaced
        // for(i=1;i<GetVectorLength();i++)
        //      {
        //	if(rand<theIntegral[i]/theIntegral[GetVectorLength()-1]) break;
        //      }

        // by this (begin)
        for (i = GetVectorLength()-1; i >= 0 ; i--)
        {
        	if(rand > theIntegral[i]/theIntegral[GetVectorLength()-1])
            break;
        }
        if(i!=GetVectorLength()-1) {
          i++;
        }
        // until this (end)

        G4double x1, x2, y1, y2;
        y1 = theData[i-1].GetX();
        x1 = theIntegral[i-1];
        y2 = theData[i].GetX();
        x2 = theIntegral[i];
        if(std::abs((y2-y1)/y2)<0.0000001) // not really necessary, since the case is excluded by construction
        {
        	y1 = theData[i-2].GetX();
        	x1 = theIntegral[i-2];
        }
        result = theLin.Lin(rand, x1, x2, y1, y2);
      }
      return result;
    #endif
  }

  G4double * Debug()
  {
    #if GEANT4_ENABLE_CUDA
      return cudaVector->Debug();
    #else
      return theIntegral;
    #endif
  }

  inline void Integrate()
  {
    #if GEANT4_ENABLE_CUDA
      cudaVector->Integrate();
    #else
      G4int i;
      if(nEntries == 1)
      {
        totalIntegral = 0;
        return;
      }
      G4double sum = 0;
      G4InterpolationScheme scheme1 = theManager.GetScheme(1);
      for(i=1;i<GetVectorLength();i++)
      {
        if(std::abs((theData[i].GetX()-theData[i-1].GetX())/theData[i].GetX())>0.0000001)
        {
          G4double x1 = theData[i-1].GetX();
          G4double x2 = theData[i].GetX();
          G4double y1 = theData[i-1].GetY();
          G4double y2 = theData[i].GetY();
          G4InterpolationScheme aScheme = theManager.GetScheme(i);
          if(aScheme==LINLIN||aScheme==CLINLIN||aScheme==ULINLIN)
          {
            sum+= 0.5*(y2+y1)*(x2-x1);
          }
          else if(aScheme==LINLOG||aScheme==CLINLOG||aScheme==ULINLOG)
          {
            G4double a = y1;
            G4double b = (y2-y1)/(G4Log(x2)-G4Log(x1));
            sum+= (a-b)*(x2-x1) + b*(x2*G4Log(x2)-x1*G4Log(x1));
          }
          else if(aScheme==LOGLIN||aScheme==CLOGLIN||aScheme==ULOGLIN)
          {
            G4double a = G4Log(y1);
            G4double b = (G4Log(y2)-G4Log(y1))/(x2-x1);
            sum += (G4Exp(a)/b)*(G4Exp(b*x2)-G4Exp(b*x1));
          }
          else if(aScheme==HISTO||aScheme==CHISTO||aScheme==UHISTO)
          {
            sum+= y1*(x2-x1);
          }
          else if(aScheme==LOGLOG||aScheme==CLOGLOG||aScheme==ULOGLOG)
          {
            G4double a = G4Log(y1);
            G4double b = (G4Log(y2)-G4Log(y1))/(G4Log(x2)-G4Log(x1));
            sum += (G4Exp(a)/(b+1))*(G4Pow::GetInstance()->powA(x2,b+1)-G4Pow::GetInstance()->powA(x1,b+1));
          }
          else
          {
            throw G4HadronicException(__FILE__, __LINE__, "Unknown interpolation scheme in G4ParticleHPVector::Integrate");
          }

        }
      }
      totalIntegral = sum;
    #endif
  }

  inline void IntegrateAndNormalise()
  {
    #if GEANT4_ENABLE_CUDA
      cudaVector->IntegrateAndNormalise();
    #else
      G4int i;
      if(theIntegral!=0) return;
      theIntegral = new G4double[nEntries];
      if(nEntries == 1)
      {
        theIntegral[0] = 1;
        return;
      }
      theIntegral[0] = 0;
      G4double sum = 0;
      G4double x1 = 0;
      G4double x0 = 0;
      for(i=1;i<GetVectorLength();i++)
      {
        x1 = theData[i].GetX();
        x0 = theData[i-1].GetX();
        if (std::abs(x1-x0) > std::abs(x1*0.0000001) )
        {
        	//********************************************************************
        	//EMendoza -> the interpolation scheme is not always lin-lin
        	/*
                sum+= 0.5*(theData[i].GetY()+theData[i-1].GetY())*(x1-x0);
        	*/
        	//********************************************************************
          G4InterpolationScheme aScheme = theManager.GetScheme(i);
          G4double y0 = theData[i-1].GetY();
          G4double y1 = theData[i].GetY();
        	G4double integ=theInt.GetBinIntegral(aScheme,x0,x1,y0,y1);
          #if defined WIN32-VC
          	if(!_finite(integ)){integ=0;}
          #elif defined __IBMCPP__
          	if(isinf(integ)||isnan(integ)){integ=0;}
          #else
          	if(std::isinf(integ)||std::isnan(integ)){integ=0;}
          #endif
          	sum+=integ;
          	//********************************************************************
        }
        theIntegral[i] = sum;
      }
      G4double total = theIntegral[GetVectorLength()-1];
      for(i=1;i<GetVectorLength();i++)
      {
        theIntegral[i]/=total;
      }
    #endif
  }

  inline void Times(G4double factor)
  {
    #if GEANT4_ENABLE_CUDA
      cudaVector->Times(factor);
    #else
      G4int i;
      for(i=0; i<nEntries; i++) {
        theData[i].SetY(theData[i].GetY()*factor);
      }
      if(theIntegral!=0) {
        theIntegral[i] *= factor;
      }
    #endif
  }


  /******************************************
   * PRIVATE
   ******************************************/
  private:
  void Check(G4int i);
  G4bool IsBlocked(G4double aX);

  //=======================================
  #if GEANT4_ENABLE_CUDA
    public:
    G4ParticleHPVector_CUDA * cudaVector;
  #else
    G4ParticleHPInterpolator theLin;
    G4double totalIntegral;

    G4ParticleHPDataPoint * theData;
    G4InterpolationManager theManager; // knows how to interpolate the data.
    G4double * theIntegral;
    G4int nEntries;
    G4int nPoints;
    G4double label;

    G4ParticleHPInterpolator theInt;
    G4int Verbose;
    G4int isFreed; // debug only

    G4ParticleHPHash theHash;
    G4double maxValue;
    std::vector<G4double> theBlocked;
    // std::vector<G4double> theBuffered;
    G4double the15percentBorderCash;
    G4double the50percentBorderCash;
  #endif
  //=======================================

};

#endif
