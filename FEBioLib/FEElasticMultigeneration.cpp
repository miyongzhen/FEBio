#include "stdafx.h"
#include "FECore/tens4d.h"
#include "FEElasticMultigeneration.h"

// define the material parameters
// BEGIN_PARAMETER_LIST(FEElasticMultigeneration, FEElasticMaterial)
// END_PARAMETER_LIST();

//-----------------------------------------------------------------------------
vector<FEGenerationData*> FEElasticMultigeneration::m_MG;

//=============================================================================
FEMaterialPoint* FEMultigenerationMaterialPoint::Copy()
{
	FEMultigenerationMaterialPoint* pt = new FEMultigenerationMaterialPoint(*this);
	pt->m_pmat = m_pmat;
	pt->Fi = Fi;
	pt->Ji = Ji;
	if (m_pt) pt->m_pt = m_pt->Copy();
	return pt;
}

//-----------------------------------------------------------------------------
void FEMultigenerationMaterialPoint::Serialize(DumpFile& ar)
{
	if (m_pt) m_pt->Serialize(ar);
			
	if (ar.IsSaving())
	{
		for (int i=0; i < (int)Fi.size(); i++)
			ar << Fi[i];
		for (int i=0; i < (int)Ji.size(); i++)
			ar << Ji[i];
	}
	else
	{
		for (int i=0; i < (int)Fi.size(); i++)
			ar >> Fi[i];
		for (int i=0; i < (int)Ji.size(); i++)
			ar >> Ji[i];
	}
			
	if (m_pt) m_pt->Serialize(ar);
}

//-----------------------------------------------------------------------------
void FEMultigenerationMaterialPoint::Init(bool bflag)
{
	if (m_pt) m_pt->Init(bflag);

	if (bflag)
	{
		Fi.clear();
		Ji.clear();
	}

	// get the time
	double t = FEMaterialPoint::time;

	// Check if this constitutes a new generation
	int igen = m_pmat->CheckGeneration(t);
	t = m_pmat->m_MG[igen]->btime;
	if ((bflag == false) && (m_pmat->HasGeneration(igen) && (t>m_tgen)))
	{
		FEElasticMaterialPoint& pt = *((*this).ExtractData<FEElasticMaterialPoint>());
					
		// push back F and J to define relative deformation gradient of this generation
		mat3d F = pt.F;
		double J = pt.J; 
		Fi.push_back(F.inverse());
		Ji.push_back(1.0/J);

		m_tgen = t;
	}
}

//=============================================================================
void FEElasticMultigeneration::PushGeneration(FEGenerationData* G)
{
	m_MG.push_back (G);
}

//--------------------------------------------------------------------------------
// Check if time t constitutes a new generation and return that generation
int FEElasticMultigeneration::CheckGeneration(const double t)
{
	int ngen = m_MG.size();
	for (int igen=1; igen<ngen; ++igen)
	{
		if (t < m_MG[igen]->btime) return igen - 1;
	}
	return ngen - 1;
}

//--------------------------------------------------------------------------------
bool FEElasticMultigeneration::HasGeneration(const int igen)
{
	for (int i=0; i<(int)m_pMat.size(); ++i)
		if (m_pMat[i]->GetID() == igen) return true;

	return false;
}

//-----------------------------------------------------------------------------
void FEElasticMultigeneration::Init()
{
	FEElasticMaterial::Init();
	for (int i=0; i<(int)m_pMat.size(); i++)
		m_pMat[i]->Init();
}

//-----------------------------------------------------------------------------
mat3ds FEElasticMultigeneration::Stress(FEMaterialPoint& mp)
{
	FEMultigenerationMaterialPoint& pt = *mp.ExtractData<FEMultigenerationMaterialPoint>();
	FEElasticMaterialPoint& mpt = *mp.ExtractData<FEElasticMaterialPoint>();

	mat3ds s;
	
	// calculate stress
	s.zero();
	
	s = m_pMat[0]->Stress(mp);
	
	// extract deformation gradient
	mat3d Fs = mpt.F;
	double Js = mpt.J;
	
	for (int i=0; i < (int)pt.Fi.size(); ++i) 
	{
		// evaluate deformation gradient for this generation
		mat3d Fi = pt.Fi[i];
       	mpt.F = Fs*Fi;
		double Ji = pt.Ji[i];
		mpt.J = Js*Ji;
		// evaluate stress for this generation
		s += Ji*m_pMat[i+1]->Stress(mp);
	}

	// restore the material point deformation gradient
	mpt.F = Fs;
	mpt.J = Js;
	
	return s;
}

//-----------------------------------------------------------------------------
tens4ds FEElasticMultigeneration::Tangent(FEMaterialPoint& mp)
{
	FEMultigenerationMaterialPoint& pt = *mp.ExtractData<FEMultigenerationMaterialPoint>();
	FEElasticMaterialPoint& mpt = *mp.ExtractData<FEElasticMaterialPoint>();
	
	tens4ds c(0.);
	
	c = m_pMat[0]->Tangent(mp);

	// extract deformation gradient
	mat3d Fs = mpt.F;
	double Js = mpt.J;
	
	for (int i=0; i < (int)pt.Fi.size(); ++i) 
	{
		// evaluate deformation gradient for this generation
		mat3d Fi = pt.Fi[i];
       	mpt.F = Fs*Fi;
		double Ji = pt.Ji[i];
		mpt.J = Js*Ji;
		// evaluate stress for this generation
		c += Ji*m_pMat[i+1]->Tangent(mp);
	}
	
	// restore the material point deformation gradient
	mpt.F = Fs;
	mpt.J = Js;
	
	return c;
}
