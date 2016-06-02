#include "stdafx.h"
#include "FEFluidTractionLoad.h"
#include "FECore/FEModel.h"

//=============================================================================
BEGIN_PARAMETER_LIST(FEFluidTractionLoad, FESurfaceLoad)
	ADD_PARAMETER(m_scale, FE_PARAM_DOUBLE    , "scale"   );
	ADD_PARAMETER(m_TC   , FE_PARAM_DATA_ARRAY, "traction");
END_PARAMETER_LIST();

//-----------------------------------------------------------------------------
//! constructor
FEFluidTractionLoad::FEFluidTractionLoad(FEModel* pfem) : FESurfaceLoad(pfem), m_TC(FE_VEC3D)
{
	m_scale = 1.0;
	m_TC.set(vec3d(0,0,0));

	m_dofVX = pfem->GetDOFIndex("vx");
	m_dofVY = pfem->GetDOFIndex("vy");
	m_dofVZ = pfem->GetDOFIndex("vz");
}

//-----------------------------------------------------------------------------
//! allocate storage
void FEFluidTractionLoad::SetSurface(FESurface* ps)
{
	FESurfaceLoad::SetSurface(ps);
	m_TC.Create(ps); 
}

//-----------------------------------------------------------------------------
void FEFluidTractionLoad::UnpackLM(FEElement& el, vector<int>& lm)
{
	FEMesh& mesh = GetFEModel()->GetMesh();
	int N = el.Nodes();
	lm.resize(N*3);
	for (int i=0; i<N; ++i)
	{
		int n = el.m_node[i];
		FENode& node = mesh.Node(n);
		vector<int>& id = node.m_ID;

		lm[3*i  ] = id[m_dofVX];
		lm[3*i+1] = id[m_dofVY];
		lm[3*i+2] = id[m_dofVZ];
	}
}

//-----------------------------------------------------------------------------
//! Calculate the residual for the traction load
void FEFluidTractionLoad::Residual(const FETimePoint& tp, FEGlobalVector& R)
{
	FEModel& fem = R.GetFEModel();

	vector<double> fe;
	vector<int> elm;

	vec3d r0[FEElement::MAX_NODES];

	int i, n;
	int npr = (int)m_TC.size();
	for (int iel=0; iel<npr; ++iel)
	{
		FESurfaceElement& el = m_psurf->Element(iel);

        int ndof = 3*el.Nodes();
		fe.resize(ndof);

		// nr integration points
		int nint = el.GaussPoints();

		// nr of element nodes
		int neln = el.Nodes();

		// nodal coordinates
		for (i=0; i<neln; ++i) r0[i] = m_psurf->GetMesh()->Node(el.m_node[i]).m_r0;

		double* Gr, *Gs;
		double* N;
		double* w  = el.GaussWeights();

		vec3d dxr, dxs;

		// calculate the traction at the integration point
		vec3d t = m_TC.get<vec3d>(iel)*m_scale;

		// repeat over integration points
		zero(fe);
		for (n=0; n<nint; ++n)
		{
			N  = el.H(n);
			Gr = el.Gr(n);
			Gs = el.Gs(n);

			// calculate the tangent vectors
			dxr = dxs = vec3d(0,0,0);
			for (i=0; i<neln; ++i) 
			{
				dxr.x += Gr[i]*r0[i].x;
				dxr.y += Gr[i]*r0[i].y;
				dxr.z += Gr[i]*r0[i].z;

				dxs.x += Gs[i]*r0[i].x;
				dxs.y += Gs[i]*r0[i].y;
				dxs.z += Gs[i]*r0[i].z;
			}

			vec3d f = t*((dxr ^ dxs).norm()*w[n]);

			for (i=0; i<neln; ++i)
			{
                fe[3*i  ] += N[i]*f.x;
                fe[3*i+1] += N[i]*f.y;
                fe[3*i+2] += N[i]*f.z;
			}
		}

		// get the element's LM vector and adjust it
		UnpackLM(el, elm);
        
		// add element force vector to global force vector
		R.Assemble(el.m_node, elm, fe);
	}
}
