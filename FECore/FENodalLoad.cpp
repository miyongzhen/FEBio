/*This file is part of the FEBio source code and is licensed under the MIT license
listed below.

See Copyright-FEBio.txt for details.

Copyright (c) 2019 University of Utah, Columbia University, and others.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.*/

#include "stdafx.h"
#include "FENodalLoad.h"
#include "FENodeSet.h"
#include "DumpStream.h"

REGISTER_SUPER_CLASS(FENodalLoad, FENODALLOAD_ID);

//-----------------------------------------------------------------------------
BEGIN_FECORE_CLASS(FENodalLoad, FEModelComponent)
	ADD_PARAMETER(m_scale, "scale");
	ADD_PARAMETER(m_data , "value");
END_FECORE_CLASS();

//-----------------------------------------------------------------------------
FENodalLoad::FENodalLoad(FEModel* pfem) : FEModelComponent(pfem), m_data(FE_DOUBLE)
{
	m_scale = 1.0;
	m_dof = -1;
}

//-----------------------------------------------------------------------------
void FENodalLoad::Serialize(DumpStream& ar)
{
	FEModelComponent::Serialize(ar);
	if (ar.IsShallow()) return;
	ar & m_dof & m_item;
}

//-----------------------------------------------------------------------------
bool FENodalLoad::Init()
{
	return true;
}

//-----------------------------------------------------------------------------
void FENodalLoad::AddNode(int nid, double scale)
{
	m_item.push_back(nid);
	m_data.Add(scale);
}

//-----------------------------------------------------------------------------
void FENodalLoad::AddNodes(const FENodeSet& ns, double scale)
{
	int N = ns.size();
	for (int i = 0; i<N; ++i) AddNode(ns[i], scale);
}

//-----------------------------------------------------------------------------
void FENodalLoad::SetLoad(double s)
{
	m_scale = s;
}

//-----------------------------------------------------------------------------
//! Return the current value of the nodal load
double FENodalLoad::NodeValue(int n) const
{
	return m_scale*m_data.getValue(n);
}
