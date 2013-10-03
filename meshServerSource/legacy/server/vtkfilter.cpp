#include "stdafx.h"
#include "vtkfilter.h"
#include "progressindicator.h"

bool CVTKFilter::DecimatePolyData(vtkPolyData* &pPD, CParametersMap &paramsIn)
{
	vtkDecimatePro *pDecimate = vtkDecimatePro::New();
	//vtkDecimate *pDecimate = vtkDecimate::New();
	if (!pDecimate)
		return false;
	vtkPolyData *pOut = vtkPolyData::New();
	if (!pOut)
	{
		pDecimate->Delete();
		return false;
	}

	pDecimate->SetBoundaryVertexDeletion(paramsIn.GetInt(wxString::FromAscii("decimate_boudary_vertex_deletion"), TRUE));
	pDecimate->SetDegree(paramsIn.GetInt(wxString::FromAscii("decimate_degree"), 25));
	pDecimate->SetPreserveTopology(paramsIn.GetInt(wxString::FromAscii("decimate_preserve_topology"), TRUE));
	pDecimate->SetTargetReduction(paramsIn.GetFloat(wxString::FromAscii("decimate_reduction"), 0.9));

	//{ these methods exist in vtkDecimate only (not in vtkDecimatePro)
	//pDecimate->SetPreserveEdges(paramsIn.GetInt("decimate_preserve_edges", FALSE));
	//pDecimate->SetMaximumIterations(paramsIn.GetInt("decimate_iterations", 6));
	//pDecimate->SetMaximumSubIterations(paramsIn.GetInt("decimate_subiterations", 2));
	//pDecimate->SetAspectRatio(paramsIn.GetFloat("decimate_aspect_ratio", 25.0));
	//pDecimate->SetInitialError(paramsIn.GetFloat("decimate_initial_error", 0.0));
	//pDecimate->SetErrorIncrement(paramsIn.GetFloat("decimate_error_increment", 0.005));
	//pDecimate->SetMaximumError(paramsIn.GetFloat("decimate_maximum_error", 0.1));
	//}


	pDecimate->AccumulateErrorOn();

	pDecimate->SetInput(pPD);

	CProgressIndicator *pPI = CProgressIndicator::GetInstance();
	void *pProgressTask = pPI->StartNewTask((vtkProcessObject*)pDecimate, wxString::FromAscii("Decimating"));

	pDecimate->Update();

	pPI->EndTask(pProgressTask);

	pOut->ShallowCopy(pDecimate->GetOutput());

	pDecimate->Delete();

	wxString strMsg;
	strMsg = wxString::Format(wxString::FromAscii(">>> Source points/3angles count = %d, %d"), pPD->GetNumberOfPoints(), pPD->GetNumberOfPolys());
	wxLogDebug(strMsg);
	strMsg = wxString::Format(wxString::FromAscii(">>> Decimated points/3angles count = %d, %d"), pOut->GetNumberOfPoints(), pOut->GetNumberOfPolys());
	wxLogDebug(strMsg);

	pPD->Delete();
	pPD = pOut;

	return true;
}

bool CVTKFilter::Strippify(vtkPolyData* &pPD, CParametersMap &paramsIn)
{
	//return true;//

	vtkStripper	*pStripper = vtkStripper::New();
	if (!pStripper)
		return false;
	
	vtkPolyData	*pOut = vtkPolyData::New();
	if (!pOut)
	{
		pStripper->Delete();
		return false;
	}

	pStripper->SetInput(pPD);
	pStripper->SetMaximumLength(100000);

	CProgressIndicator *pPI = CProgressIndicator::GetInstance();
	void *pProgressTask = pPI->StartNewTask((vtkProcessObject*)pStripper, wxString::FromAscii("Stripping"));

	pStripper->Update();

	pPI->EndTask(pProgressTask);

	pOut->ShallowCopy(pStripper->GetOutput());

	pStripper->Delete();

	pPD->Delete();
	pPD = pOut;

	return true;
}

bool CVTKFilter::Smooth(vtkPolyData* &pPD, CParametersMap &paramsIn)
{

	// 2003.09.24 RFD: Added option to use WindowedSinc smoothing.

	if(paramsIn.GetInt(wxString::FromAscii("smooth_sinc_method"), FALSE)){
		vtkWindowedSincPolyDataFilter *pSmooth = vtkWindowedSincPolyDataFilter::New();
		if (!pSmooth)
			return false;

		vtkPolyData	*pOut = vtkPolyData::New();
		if (!pOut){
			pSmooth->Delete();
			return false;
		}
		pSmooth->SetNumberOfIterations(paramsIn.GetInt(wxString::FromAscii("smooth_iterations"), 20));
		pSmooth->SetPassBand(paramsIn.GetFloat(wxString::FromAscii("smooth_relaxation"), 0.05));
		pSmooth->SetFeatureAngle(paramsIn.GetFloat(wxString::FromAscii("smooth_feature_angle"), 45.0));
		pSmooth->SetEdgeAngle(paramsIn.GetFloat(wxString::FromAscii("smooth_edge_angle"), 15.0));
		pSmooth->SetBoundarySmoothing(paramsIn.GetInt(wxString::FromAscii("smooth_boundary"), TRUE));
		pSmooth->SetFeatureEdgeSmoothing(paramsIn.GetInt(wxString::FromAscii("smooth_feature_angle_smoothing"), FALSE));

		pSmooth->SetInput(pPD);

		CProgressIndicator *pPI = CProgressIndicator::GetInstance();
		void *pProgressTask = pPI->StartNewTask((vtkProcessObject*)pSmooth, wxString::FromAscii("Smoothing"));

		pSmooth->Update();

		pPI->EndTask(pProgressTask);

		pOut->ShallowCopy(pSmooth->GetOutput());

		pSmooth->Delete();

		pPD->Delete();
		pPD = pOut;
	}else{
		vtkSmoothPolyDataFilter	*pSmooth = vtkSmoothPolyDataFilter::New();
		if (!pSmooth)
			return false;

		vtkPolyData	*pOut = vtkPolyData::New();
		if (!pOut){
			pSmooth->Delete();
			return false;
		}
		pSmooth->SetNumberOfIterations(paramsIn.GetInt(wxString::FromAscii("smooth_iterations"), 20));
		pSmooth->SetRelaxationFactor(paramsIn.GetFloat(wxString::FromAscii("smooth_relaxation"), 0.1));
		pSmooth->SetFeatureAngle(paramsIn.GetFloat(wxString::FromAscii("smooth_feature_angle"), 45.0));
		pSmooth->SetEdgeAngle(paramsIn.GetFloat(wxString::FromAscii("smooth_edge_angle"), 15.0));
		pSmooth->SetBoundarySmoothing(paramsIn.GetInt(wxString::FromAscii("smooth_boundary"), TRUE));
		pSmooth->SetFeatureEdgeSmoothing(paramsIn.GetInt(wxString::FromAscii("smooth_feature_angle_smoothing"), FALSE));

		pSmooth->SetInput(pPD);

		CProgressIndicator *pPI = CProgressIndicator::GetInstance();
		void *pProgressTask = pPI->StartNewTask((vtkProcessObject*)pSmooth, wxString::FromAscii("Smoothing"));

		pSmooth->Update();

		pPI->EndTask(pProgressTask);

		pOut->ShallowCopy(pSmooth->GetOutput());

		pSmooth->Delete();

		pPD->Delete();
		pPD = pOut;
	}
	return true;
}

bool CVTKFilter::BuildNormals(vtkPolyData* &pPD, CParametersMap &paramsIn)
{
	vtkPolyDataNormals	*pNormalizer = vtkPolyDataNormals::New();
	if (!pNormalizer)
		return false;

	vtkPolyData	*pOut = vtkPolyData::New();
	if (!pOut)
	{
		pNormalizer->Delete();
		return false;
	}

	pNormalizer->ComputePointNormalsOn();
//	pNormalizer->ComputeCellNormalsOff();
	pNormalizer->SplittingOff();

	pNormalizer->SetInput(pPD);

	CProgressIndicator *pPI = CProgressIndicator::GetInstance();
	void *pProgressTask = pPI->StartNewTask((vtkProcessObject*)pNormalizer, wxString::FromAscii("Building normals"));

	pNormalizer->Update();

	pPI->EndTask(pProgressTask);

	pOut->ShallowCopy(pNormalizer->GetOutput());

	pNormalizer->Delete();

	pPD->Delete();
	pPD = pOut;

	return true;
}

bool CVTKFilter::CleanPolyData(vtkPolyData* &pPD, CParametersMap &paramsIn)
{
	vtkCleanPolyData	*pCleaner = vtkCleanPolyData::New();
	if (!pCleaner)
		return false;

	vtkPolyData	*pOut = vtkPolyData::New();
	if (!pOut)
	{
		pCleaner->Delete();
		return false;
	}

	pCleaner->ConvertPolysToLinesOff();
	pCleaner->ConvertStripsToPolysOff();
	pCleaner->PointMergingOn();

	pCleaner->SetInput(pPD);

	CProgressIndicator *pPI = CProgressIndicator::GetInstance();
	void *pProgressTask = pPI->StartNewTask((vtkProcessObject*)pCleaner, wxString::FromAscii("Cleaning poly data"));

	pCleaner->Update();

	pPI->EndTask(pProgressTask);

	pOut->ShallowCopy(pCleaner->GetOutput());

	pCleaner->Delete();

	pPD->Delete();
	pPD = pOut;

	return true;	
}
