/*


HISTORY:
2003: InSolve wrote it.
2005.07 RFD: added ROI drawing capability. The mesh flood-fill code 
        (fillPatch) was contributed by Dave Akers.
2009.12 RK: Modified to use dynamic vtk and wx libraries.
            Tested for VTK-5.4.2, wxGTK-2.8.10 and wxGTK-gl-2.8.10
*/

#include "stdafx.h"
#include "3dview.h"
#include "mesh.h"
#include "text.h"
#include "cursor.h"
#include "helpers.h"
#include "serverconsole.h"
#include "vtkPolyDataSingleSourceShortestPath.h"
#include "vtkTransformPolyDataFilter.h"
#include "vtkPolyDataConnectivityFilter.h"

#include <list>
#include <vector>
//#ifdef __WXMSW__
//#include <hash_map>
//#else
//#include <hash_map.h>
//#endif
//#include <backward/hash_map>

double sqr(double x){ return(x*x); };

using namespace std;
using namespace __gnu_cxx;


ROI::ROI(CActor *roiActor){ 
	// points list should be empty
	actor = roiActor; 
	// We need our actors poly data to do things like connect points
	polyData = ((CMesh*)actor)->BuildVtkPolyDataTriangles();
}

ROI::~ROI(){
	removeAllPoints();
	if(polyData) polyData->Delete();
}

bool ROI::setActor(CActor *roiActor){ 
	actor = roiActor; 
	if(polyData) polyData->Delete();
	polyData = ((CMesh*)actor)->BuildVtkPolyDataTriangles();
	return(polyData!=NULL);
}

bool ROI::addPoint(int vertex, bool userDefined=true){
	if(vertex<0 || vertex>((CMesh*)actor)->nVertices){
		wxLogDebug(wxString::FromAscii("vertex out of range."));
		return(false);
	}
	roiPoint p;
	p.vertex = vertex;
	for(int i=0; i<4; i++)
		p.origColor[i] = ((CMesh*)actor)->pVertices[vertex].cColor.m_ucColor[i];
	p.userDefined = userDefined;
	points.push_back(p);
	// Paint the vertex
	((CMesh*)actor)->pVertices[vertex].cColor.r = 0;
	((CMesh*)actor)->pVertices[vertex].cColor.g = 0;
	((CMesh*)actor)->pVertices[vertex].cColor.b = 0;
	((CMesh*)actor)->pVertices[vertex].cColor.a = 255;
	//MixColors(pVertices[iVertex].cColor.m_ucColor, fColor, pVertices[iVertex].cColor.m_ucColor,1.0, 4);
	return(true);
}

void ROI::addConnectedPoint(int vertex){
	if(!points.empty()){
		int prevVertex = getCurVertex();
                if(vertex!=prevVertex){
                        // Compute the shortest path between two vertices:
			vtkPolyDataSingleSourceShortestPath *path = vtkPolyDataSingleSourceShortestPath::New();
			path->SetInput(polyData);
			path->SetStartVertex(prevVertex);
			path->SetEndVertex(vertex);
			path->Update();
			//((CServerConsole*)m_pConsole)->Log("Finished.");
			// Extract the path:
			vtkIdList *pathVerts = path->GetIdList();
			int curVert;
			for(int i=0; i<pathVerts->GetNumberOfIds(); i++){
				curVert = (int)pathVerts->GetId(i);
				addPoint(curVert, false);
			}
			path->Delete();
		}
	}
	addPoint(vertex, true);
}

void ROI::connectCurToFirst(){
	// we close the ROI by connecting the curPoint (the last one added) to the startPoint.
	if(points.front().vertex != points.back().vertex)
		addConnectedPoint(points.front().vertex);
}

void ROI::removePoint(){
	// Remove the last point, taking care to reset the vertex color
	for(int i=0; i<4; i++)
		((CMesh*)actor)->pVertices[points.back().vertex].cColor.m_ucColor[i] = points.back().origColor[i];
	points.pop_back();
}

void ROI::removeAllPoints(){
	while(!points.empty())
		removePoint();
}

void ROI::removeLastSegment(){
	// Remove all points starting at curPoint and working
	// back to the next user-defined point
	if(points.empty()) return;
	// We always remove the last one, even if it is user-defined.
	removePoint();
  while(!points.empty() && !points.back().userDefined){
		removePoint();
	}
}

int ROI::fill(int seedVert){
	if(points.empty() || !polyData) return(0);
	// Make sure the ROI is closed
	// *** TO DO: do a more intelligent test for closure!
	if(points.front().vertex != points.back().vertex)
		connectCurToFirst();

	// Initialize the polyData stuff that we need to efficiently traverse
	// the mesh and pluck cells.
	polyData->BuildCells();
	polyData->BuildLinks();

	vtkIdList *cells = vtkIdList::New();
	cells->Initialize();
	// We need to get a cell connected to the seed vertex.
	polyData->GetPointCells((vtkIdType)seedVert, cells);
	if(cells->GetNumberOfIds()<1) return(0);
	int n = fillPatch(cells->GetId(0));

	polyData->DeleteLinks();
	polyData->DeleteCells();
	return(n);
}

bool ROI::getPoints(CDoubleArray *inds){
	if(points.empty()) return false;
	//CDoubleArray *pts = new CDoubleArray;
	if(!inds->Create(1, points.size())) return false;
	pointList::iterator i;
	int n = 0;
  for(i=points.begin(); i!=points.end(); ++i){
		inds->SetValue((double)i->vertex,n);
		n++;
	}
	return true;
}

vector<int> *ROI::getPoints(){
	if(points.empty()) return NULL;
	vector<int> *inds = new vector<int>;
	pointList::iterator i;
  for(i=points.begin(); i!=points.end(); ++i)
		inds->push_back(i->vertex);
	return(inds);
}

// Not needed?
struct eqint {
	bool operator() (int i1, int i2) {
		return i1 == i2;
	}
}; 

int ROI::fillPatch(int initialID) {
	// Dave's original code was based on a old SGI version of the hash_map
	// template. It wouldn't compile under VC7 with the STL standard hash_map.
	// The following simplfied version (using defaults for the comparison)
	// seems to work, although I'm not sure why. 
	//hash_map<int, bool, hash<int>, eqint> visitedCells;
	//hash_map<int, bool> visitedCells;
	// 2009.02.26 RFD: replaced old C++ template hashmap with wx hashmap
	WX_DECLARE_HASH_MAP( int, bool, wxIntegerHash, wxIntegerEqual, cellHash );
        cellHash visitedCells;
    
	int n=0;

	// Copy the current (boundary) vertices (we'll be adding to them, so we need a copy)
	vector<int> *boundaryVerts = getPoints();
	if(boundaryVerts==NULL) return(0);

	visitedCells[initialID] = true;
	list<int> priorityQueue;
	//cerr << "pushing cellID " << initialID << " to start." << endl;
	priorityQueue.push_front (initialID);
	
	double seedPoint[3] = {0,0,0};
	bool firstTime = true;
	
	while (!priorityQueue.empty()) {
		bool cellMatches = false;
		// take the first thing off the queue:
		int cellID = *(priorityQueue.begin());
		priorityQueue.erase (priorityQueue.begin());
		vtkCell *cell = polyData->GetCell(cellID);
		vtkPoints *pts = polyData->GetPoints();
		double pt[3];
		bool exclude = false;
		for (int cellVertInd=0; cellVertInd<cell->GetNumberOfPoints(); cellVertInd++) {
			pts->GetPoint(cell->GetPointId(cellVertInd), pt);
			if (firstTime) {
				cellMatches = true;
				// xxx hack Why?
				if(cellVertInd==2) firstTime = false;
				for(int i = 0; i < 3; i++) {
					seedPoint[i] += 1.0/3.0*pt[i];
				}
			}else{
				// test to make sure the cell's vertices are not on boundary.
				// If any of this cell's vertices are in the boundary vertex list
				// then the cell is excluded.
				for(unsigned int boundVertInd=0; boundVertInd<boundaryVerts->size(); boundVertInd++){
					for(int cellVertInd=0; cellVertInd<cell->GetNumberOfPoints(); cellVertInd++) {
						if(cell->GetPointId(cellVertInd)==(vtkIdType)(*boundaryVerts)[boundVertInd])
							exclude = true;
					}
				}
				// Akers' original distance test- catches cells that are within 
				// the ellipsoid defined by patchDim.
				//double patchDim[3] = {10, 10, 10};
				//double sqrDistances[3] = {sqr(pt[0]-seedPoint[0]),
				//													sqr(pt[1]-seedPoint[1]),
				//													sqr(pt[2]-seedPoint[2])};
				//if (sqrDistances[0]/sqr(patchDim[0]/2.0) +
				//		sqrDistances[1]/sqr(patchDim[1]/2.0) +
				//		sqrDistances[2]/sqr(patchDim[2]/2.0) > 1) {
				//	exclude = true;
				//}
			}
		}
		if (!exclude) {
			cellMatches = true;
			for (int cellVertInd=0; cellVertInd<cell->GetNumberOfPoints(); cellVertInd++) {
				if(addPoint(cell->GetPointId(cellVertInd), false))
					n++;
			}
		}
		// add all neighbors to queue.
		if (cellMatches) {
			vtkIdList *neighboringVerts = vtkIdList::New();
			vtkIdList *neighboringCells = vtkIdList::New();
			for (int vert = 0; vert < 3; vert++) {
				neighboringVerts->InsertId((const vtkIdType)0, polyData->GetCell(cellID)->GetPointId(vert));
				polyData->GetCellNeighbors(cellID, neighboringVerts, neighboringCells);
				for (int c = 0; c < neighboringCells->GetNumberOfIds(); c++) {
					int cellIDFound = neighboringCells->GetId(c);
					if (visitedCells.find(cellIDFound) == visitedCells.end()) {
						visitedCells[cellIDFound] = true;
						priorityQueue.push_front (neighboringCells->GetId(c));
					}
				}
			}
		}
	}
	boundaryVerts->clear(); // unecessary?
	return(n);
} 

BEGIN_EVENT_TABLE(C3DView, wxGLCanvas)
	EVT_SIZE(C3DView::OnSize)
	EVT_PAINT(C3DView::OnPaint)
	EVT_ERASE_BACKGROUND(C3DView::OnEraseBackground)
	EVT_MOTION(C3DView::OnMouseMove)
	EVT_LEFT_DOWN(C3DView::OnMouseDown)
	EVT_RIGHT_DOWN(C3DView::OnMouseDown)
	EVT_MIDDLE_DOWN(C3DView::OnMouseDown)
	EVT_LEFT_UP(C3DView::OnMouseUp)
	EVT_RIGHT_UP(C3DView::OnMouseUp)
	EVT_MIDDLE_UP(C3DView::OnMouseUp)
	EVT_LEFT_DCLICK(C3DView::OnLeftDoubleClick)
	EVT_RIGHT_DCLICK(C3DView::OnRightDoubleClick)
	EVT_MOUSEWHEEL(C3DView::OnMouseWheel)
	EVT_KEY_DOWN(C3DView::OnKeyDown)
	EVT_KEY_UP(C3DView::OnKeyUp)
END_EVENT_TABLE()

BEGIN_HANDLER_TABLE(C3DView)
	COMMAND_HANDLER((char* const)"screenshot", C3DView::CommandScreenShot)
	COMMAND_HANDLER((char* const)"enable_lighting", C3DView::CommandEnableLighting)
	COMMAND_HANDLER((char* const)"refresh", C3DView::CommandRefresh)
	COMMAND_HANDLER((char* const)"set_size", C3DView::CommandSetSize)
	COMMAND_HANDLER((char* const)"set_window_title", C3DView::CommandSetWindowTitle)
	COMMAND_HANDLER((char* const)"get_cur_roi", C3DView::CommandGetCurRoi)
END_HANDLER_TABLE()


C3DView::C3DView(wxWindow* parent, const wxPoint& pos, const wxSize& size, wxFrame* pConsole)
:wxGLCanvas(parent, wxID_ANY, pos, size, 0, wxT("GLCanvas")){
	bInitialized = false;
	pSelectBuffer = NULL;
	taAxis = TA_NONE;
	pActiveActor = NULL;
	iNextScreenShotNumber = 0;
	m_mode = MODE_INTERACT;
	m_curRoi = NULL;
        m_Scene = NULL;
	m_pConsole = pConsole;

        SetFocus();     //RK 12/21/2009, added to direct keyboard inputs here.
}

C3DView::~C3DView(){
	wxASSERT(pSelectBuffer);
	if(pSelectBuffer)
		delete[] pSelectBuffer;
	// This causes a segfault?
	if(m_curRoi)
		delete m_curRoi;
        if(m_Scene)
                delete m_Scene;
}

void C3DView::OnSize(wxSizeEvent& event){
    if (!bInitialized)
	return;

	SetCurrent();
	// this is also necessary to update the context on some platforms
	wxGLCanvas::OnSize(event);
	
	SetCurrent();

        CCamera *pCamera = (CCamera *)m_Scene->GetActor(CScene::PA_CAMERA);
	
	wxASSERT(pCamera);

	int w, h;
	GetClientSize(&w, &h);

	pCamera->SetViewport(0, 0, w, h);

	Refresh(FALSE);
}

void C3DView::OnEraseBackground(wxEraseEvent& event){
}


void C3DView::InitScene(){
        m_Scene = new CScene;       // must be created after OpenGL is initiated
        CCamera *pCamera = (CCamera *)m_Scene->GetActor(CScene::PA_CAMERA);
        wxASSERT(pCamera);
        int w, h;
        GetClientSize(&w, &h);
        pCamera->SetViewport(0, 0, w, h);
        pCamera->SetFrustum(w, h, -2000, 2000);
        pCamera->UpdateProjection();
}


void C3DView::OnPaint( wxPaintEvent& event ){
	wxPaintDC dc(this);
	
	if (!GetContext())
		return;

	SetCurrent();

	if (!bInitialized) 
	{
		InitGL();
                InitScene();            //RK 12/21/09: initialize m_Scene after InitGL
                bInitialized = true;
	}

	this->Timer.Start();

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        m_Scene->Render();

	//glFlush();
	glFinish();	//wait for GL operations to complete

	SwapBuffers();

	wxLogDebug(wxString::FromAscii("> Total scene rendered in %d ms"), this->Timer.GetInterval());

	this->Timer.Stop();
}

void C3DView::InitGL(){
    SetCurrent();

	glGetIntegerv(GL_MAX_NAME_STACK_DEPTH, &iGLNameStackSize);
	pSelectBuffer = new GLuint[iGLNameStackSize];
	wxASSERT(pSelectBuffer);

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glEnable(GL_COLOR_MATERIAL);

//	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

//	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

	glEnable(GL_LIGHTING);
	SetupMaterial();
}

void C3DView::OnMouseMove(wxMouseEvent& event){
	//wxSize	size = GetClientSize();
	this->Timer.Start();
	SetCurrent();
        //CCamera *pCamera = (CCamera *)m_Scene->GetActor(CScene::PA_CAMERA);
	//wxASSERT(pCamera);
	bool	bRefreshNeeded = false;
	if(m_mode==MODE_INTERACT){
		if (event.ControlDown()){
			bRefreshNeeded = OnMouseMoveTransformActor(event);
		}else if (taAxis == TA_NONE){
			bRefreshNeeded = OnMouseMoveTransformCamera(event);
		}else if (pActiveActor)	// taAxis != TA_NONE here
			bRefreshNeeded = OnMouseMoveTransformArrow(event);
	}
	m_ClickPos = event.GetPosition();
	
	if (bRefreshNeeded){
                m_Scene->siSceneInfo.pPolyBuffer->UpdateTransform();
		
		Refresh(FALSE);
		wxLogDebug(wxString::FromAscii("> OnMouseMove processed in %d ms"), this->Timer.GetInterval());
	}
	this->Timer.Stop();
}

void C3DView::MoveArrowToActiveActorOrigin(){
	CTransformArrow *pArrows[3];
        pArrows[0] = (CTransformArrow *)m_Scene->GetActor(CScene::PA_MANIP_X);
        pArrows[1] = (CTransformArrow *)m_Scene->GetActor(CScene::PA_MANIP_Y);
        pArrows[2] = (CTransformArrow *)m_Scene->GetActor(CScene::PA_MANIP_Z);

	for (int iArrow = 0; iArrow < 3; iArrow++){
		pArrows[iArrow]->vOrigin = pActiveActor->vOrigin;
		pArrows[iArrow]->AttachToCameraSpace(pActiveActor->IsAttachedToCameraSpace());
	}
}

bool C3DView::OnMouseMoveTransformActor(wxMouseEvent& event){
	if (!pActiveActor)
		return false;

        CCamera *pCamera = (CCamera *)m_Scene->GetActor(CScene::PA_CAMERA);

	if (event.LeftIsDown()){
		double fScreenToWorld = pCamera->GetFrustumHeight() / pCamera->GetViewportHeight();
		double fDeltaX = double(event.GetX() - m_ClickPos.x) * fScreenToWorld;
		double fDeltaY = double(event.GetY() - m_ClickPos.y) * fScreenToWorld;

		// RFD: needed to create CVector separately for gcc.
		CVector cv;
		if (pActiveActor->IsAttachedToCameraSpace())
			cv = CVector(fDeltaX, -fDeltaY, 0);
		else
			cv = CVector(fDeltaX, -fDeltaY, 0) * pCamera->mRotation.Inverse();
		pActiveActor->Move(cv);
	}else if (event.RightIsDown()){
		wxSize	size	= GetClientSize();

		CMatrix3x3	mTransform;

		if (event.GetX() != m_ClickPos.x)
		{
			double	fRotY	= (event.GetX() - m_ClickPos.x)*2*M_PI/size.GetWidth();

			CVector vAxisY(0, 1, 0);
			if (!pActiveActor->IsAttachedToCameraSpace())
				vAxisY = vAxisY * pCamera->mRotation.Inverse();

			mTransform = CMatrix3x3::Rotation(vAxisY, fRotY);
			pActiveActor->mRotation = pActiveActor->mRotation * mTransform;
		}
		if (event.GetY() != m_ClickPos.y)
		{
			double	fRotX	= (event.GetY() - m_ClickPos.y)*2*M_PI/size.GetWidth();

			CVector vAxisX(1, 0, 0);
			if (!pActiveActor->IsAttachedToCameraSpace())
				vAxisX = vAxisX * pCamera->mRotation.Inverse();

			mTransform = CMatrix3x3::Rotation(vAxisX, fRotX);
			pActiveActor->mRotation = pActiveActor->mRotation * mTransform;
		}
	}else
		return false;

	MoveArrowToActiveActorOrigin();

	return true;
}

bool C3DView::OnMouseMoveTransformArrow(wxMouseEvent& event)
{
	if (taAxis == TA_NONE)
	{
		wxASSERT(0);
		return false;
	}
	if (!pActiveActor)
	{
		wxASSERT(0);
		return false;
	}

	if (event.GetY() == m_ClickPos.y)
		return false;

	wxSize size = GetClientSize();

	if (event.LeftIsDown())
	{
                CCamera *pCamera = (CCamera *)m_Scene->GetActor(CScene::PA_CAMERA);
		double fDelta = - double(event.GetY() - m_ClickPos.y)/double(size.GetHeight()) * pCamera->GetFrustumHeight();

		CVector vDelta(0, 0, 0);
		switch (taAxis)
		{
		case TA_X:
			vDelta.x = fDelta;
			break;
		case TA_Y:
			vDelta.y = fDelta;
			break;
		case TA_Z:
			vDelta.z = fDelta;
			break;
		default:
			break;
		}
		
		pActiveActor->Move(vDelta);

		MoveArrowToActiveActorOrigin();
	}
	else if (event.RightIsDown())
	{
		CMatrix3x3	mRot;
		double	fRot = (event.GetY() - m_ClickPos.y)*2*M_PI/size.GetHeight();

		switch (taAxis)
		{
		case TA_X:
			mRot = CMatrix3x3::RotationX(fRot);
			break;
		case TA_Y:
			mRot = CMatrix3x3::RotationY(fRot);
			break;
		case TA_Z:
			mRot = CMatrix3x3::RotationZ(fRot);
			break;
		default:
			break;
		}
		pActiveActor->mRotation = pActiveActor->mRotation * mRot;
	}
	else
		return false;

	return true;
}

bool C3DView::OnMouseMoveTransformCamera(wxMouseEvent& event)
{
	wxSize	size	= GetClientSize();
        CCamera *pCamera = (CCamera *)m_Scene->GetActor(CScene::PA_CAMERA);

	// *** RFD: needed to compute CMatrix3x3 separately for gcc.
	CMatrix3x3	mTransform;
	if ((event.LeftIsDown() && event.RightIsDown()) || (event.MiddleIsDown()))
	{
		double fDeltaX = double(event.GetX() - m_ClickPos.x)/double(size.GetWidth());
		double fDeltaY = double(event.GetY() - m_ClickPos.y)/double(size.GetHeight());

		pCamera->Move(fDeltaX, fDeltaY);
		pCamera->UpdateProjection();
	}
	else if (event.LeftIsDown())
	{
		if (event.GetX() != m_ClickPos.x)
		{
			double	fRotY = (event.GetX() - m_ClickPos.x)*2*M_PI/size.GetWidth();
			mTransform = CMatrix3x3::RotationY(fRotY);
			pCamera->mRotation = pCamera->mRotation * mTransform;
		}
		if (event.GetY() != m_ClickPos.y)
		{
			double	fRotX = -(event.GetY() - m_ClickPos.y)*2*M_PI/size.GetHeight();
			mTransform = CMatrix3x3::RotationX(fRotX);
			pCamera->mRotation = pCamera->mRotation * mTransform;
		}
	}
	else if (event.RightIsDown())
	{
		if (event.GetY() != m_ClickPos.y)
		{
			double	fScale = 1.0f - (event.GetY() - m_ClickPos.y)*0.005f;

			pCamera->Zoom(fScale);
			pCamera->UpdateProjection();
		}
		if (event.GetX() != m_ClickPos.x)
		{
			double	fRot = (event.GetX() - m_ClickPos.x)*2*M_PI/size.GetWidth();
			mTransform = CMatrix3x3::RotationZ(fRot);
			pCamera->mRotation = pCamera->mRotation * mTransform;
		}
	}
	else
		return false;

	return true;
}

void C3DView::OnActorSelectionChange(CActor *pNewClickedActor){
	if (pNewClickedActor == pActiveActor)
		return;
	
	int	i;

	CTransformArrow *pArrows[3];

        pArrows[0] = (CTransformArrow*)m_Scene->GetActor(CScene::PA_MANIP_X);
        pArrows[1] = (CTransformArrow*)m_Scene->GetActor(CScene::PA_MANIP_Y);
        pArrows[2] = (CTransformArrow*)m_Scene->GetActor(CScene::PA_MANIP_Z);

	pActiveActor = pNewClickedActor;

	if (pNewClickedActor){
		MoveArrowToActiveActorOrigin();
		for (i=0; i<3; i++)
			pArrows[i]->Show(true);
	}else{
		for (i=0; i<3; i++)
			pArrows[i]->Show(false);
	}
}

void C3DView::OnMouseDown(wxMouseEvent& event){
//	m_StartingMousePos	= event.GetPosition();
	m_ClickPos = event.GetPosition();
	SetCurrent();
	//this.Timer.Start();
	CActor *pClickedActor = FindClickedActor(m_ClickPos.x, m_ClickPos.y);
	//wxLogDebug("> GL_SELECT took %d ms", this.Timer.GetInterval());
	//this.Timer.Stop();

	if(pClickedActor != pActiveActor){
		if(pClickedActor){
			switch (pClickedActor->GetID()){
			case CScene::PA_MANIP_X:
				taAxis = TA_X;
				break;
			case CScene::PA_MANIP_Y:
				taAxis = TA_Y;
				break;
			case CScene::PA_MANIP_Z:
				taAxis = TA_Z;
				break;
			default:
				taAxis = TA_NONE;
				OnActorSelectionChange(pClickedActor);
				break;
			}
		}else{
			taAxis = TA_NONE;
			OnActorSelectionChange(pClickedActor);
		}
	}

	if(m_mode!=MODE_INTERACT && pClickedActor && pClickedActor->GetClassName()==CMesh::pszClassName){
		// *** TO DO: We currently only allow drawing on a mesh. 
		// Might want to add support for drawing on images.

		GLdouble wx,wy,wz;		// resulting absolute coords
		if(!UnProject(m_ClickPos.x, m_ClickPos.y, &wx, &wy, &wz)){
			wxASSERT(0);
		}

		// The clicked coords are relative to the origin and
		// need to be transformed by the current rotation.
                CActor *pCamera = m_Scene->GetActor(CScene::PA_CAMERA);
		int vIndex = ((CMesh*)pClickedActor)->FindClosestVertex((CVector(wx, wy, wz) 
                                     * pCamera->mRotation.Inverse() 
                                     - pClickedActor->vOrigin)
                                    * pClickedActor->mRotation);
		if (vIndex != -1){
			if(m_mode==MODE_FILL){
				if(m_curRoi!=NULL && m_curRoi->getActor()==pClickedActor){
					// This can take some time...
					SetCursor(wxCursor(wxCURSOR_WAIT));
                                        int numFilled = m_curRoi->fill(vIndex);
                                        ((CServerConsole*)m_pConsole)->Log(wxString::Format(wxString::FromAscii("Flood-filled ROI with %d vertices (%d total boundary points)."), numFilled, m_curRoi->getNumPoints()));
				}else{
					((CServerConsole*)m_pConsole)->Log(_T("No ROI to fill!"));
				}
				// Switching to interact mode should reset the 'wait' cursor that we showed above.
				SwitchMode(MODE_INTERACT);
			}else if(m_mode==MODE_DRAW){
				if(m_curRoi==NULL){
					m_curRoi = new ROI(pClickedActor);
                                }else if(m_curRoi->getActor()!=pClickedActor){
					// *** TO DO: handle multiple ROIs! 
					((CServerConsole*)m_pConsole)->Log(_T("WARNING: beginning new ROI on another actor."));
					m_curRoi->removeAllPoints();
					m_curRoi->setActor(pClickedActor);
				}
				m_curRoi->addConnectedPoint(vIndex);
			}
		}
                ((CServerConsole*)m_pConsole)->Log(wxString::Format(wxString::FromAscii("ROI point: [%0.1f, %0.1f, %0.1f] (vertex = %d)"), wx, wy, wz, vIndex));
	}
	Refresh(FALSE);
}

void C3DView::OnMouseUp(wxMouseEvent& event)
{
	SetCurrent();

	taAxis = TA_NONE;

        m_Scene->siSceneInfo.pPolyBuffer->SortTriangles();

	Refresh(FALSE);
}

void C3DView::OnMouseWheel(wxMouseEvent& event)
{
	SetCurrent();

        CCamera *pCamera = (CCamera *)m_Scene->GetActor(CScene::PA_CAMERA);

#ifdef _WINDOWS
	pCamera->Zoom(1.0f + double(event.GetWheelRotation() / double(event.GetWheelDelta()) * 0.1f));
#else
	pCamera->Zoom(1.0f + double(event.GetWheelRotation() / 80.0f * 0.1f));
#endif
	pCamera->UpdateProjection();

	Refresh(FALSE);
}

void C3DView::OnKeyDown(wxKeyEvent& event)
{
	long evkey = event.GetKeyCode();
	if (evkey == 0) return;
	if(evkey=='D'){
		// Toggle draw mode
		if(m_mode==MODE_DRAW || m_mode==MODE_FILL) 
			SwitchMode(MODE_INTERACT);
		else 
			SwitchMode(MODE_DRAW);
	}else if(m_mode!=MODE_DRAW && m_mode!=MODE_FILL){
		((CServerConsole*)m_pConsole)->Log(_T("Unknown key- Hit 'd' to enter draw mode."));
	}
	if(m_mode==MODE_DRAW || m_mode==MODE_FILL){
		switch(evkey){
		case('D'):	
			// to avoid falling through to default- this case is actually handled above.
			break;
		case(WXK_DELETE):
			if(m_curRoi!=NULL){
				((CServerConsole*)m_pConsole)->Log(_T("Deleting current ROI."));
				//m_curRoi->removeAllPoints();
                                delete m_curRoi;
				m_curRoi = NULL;
				// Fill mode makes no sense when there's no ROI.
				if(m_mode==MODE_FILL) 
					SwitchMode(MODE_DRAW);
				Refresh(FALSE);
			}else{
				((CServerConsole*)m_pConsole)->Log(_T("No ROI to delete!"));
			}
			break;
		case('C'):
			// 'Close' the ROI- connect the first and last points.
			if(m_curRoi!=NULL){
				((CServerConsole*)m_pConsole)->Log(_T("Closing current ROI."));
				m_curRoi->connectCurToFirst();
				Refresh(FALSE);
			}else{
				((CServerConsole*)m_pConsole)->Log(_T("No ROI to close!"));
			}
			break;
		case('U'):
			if(m_curRoi!=NULL){
				((CServerConsole*)m_pConsole)->Log(_T("Removing last point."));
				m_curRoi->removeLastSegment();
				Refresh(FALSE);
			}else{
				((CServerConsole*)m_pConsole)->Log(_T("No ROI to undo!"));
			}
			break;
		case('F'):
			// toggle fill-mode
			if(m_mode==MODE_FILL)
				SwitchMode(MODE_DRAW);
			else if(m_curRoi!=NULL)
				SwitchMode(MODE_FILL);
			else
				((CServerConsole*)m_pConsole)->Log(_T("No ROI to fill!"));
			break;
		default:
			((CServerConsole*)m_pConsole)->Log(_T("\nUnknown key. In draw mode, hot keys are:"));
			((CServerConsole*)m_pConsole)->Log(_T("d: toddle draw mode\nu: undo last point"));
			((CServerConsole*)m_pConsole)->Log(_T("c: close ROI\nf:fill ROI\ndelete: remove all ROI points\n"));
		}
	}
	event.Skip();
}

void C3DView::OnKeyUp( wxKeyEvent& event )
{
        //m_Key = 0;
	event.Skip();
}


bool C3DView::UnProject(int iScreenX, int iScreenY, GLdouble *pX, GLdouble *pY, GLdouble *pZ)
{
	wxASSERT(pX);
	wxASSERT(pY);
	wxASSERT(pZ);

	GLint    viewport[4];
	GLdouble projection[16];
	GLdouble modelview[16];
        GLdouble vx,vy;//,vz;		// mouse position in viewport coordinates
        GLfloat vz;     //RK 12/21/09, glReadPixels does not support double for depth component, yet!

	glGetIntegerv(GL_VIEWPORT, viewport);
	glGetDoublev(GL_PROJECTION_MATRIX, projection);
	glGetDoublev(GL_MODELVIEW_MATRIX, modelview);

	// translate coords: window => viewport
	int ivx = iScreenX;
	int ivy = viewport[3] - iScreenY - 1;
	vx = (GLdouble)ivx;
	vy = (GLdouble)ivy;

        glReadPixels(ivx, ivy, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &vz);    //RK 12/21/09

        return (gluUnProject(vx, vy, (double)vz, modelview, projection, viewport, pX, pY, pZ)!=GL_FALSE);
}

void C3DView::OnLeftDoubleClick(wxMouseEvent& event)
{
	SetCurrent();
	wxPoint	p = event.GetPosition();
	GLdouble wx,wy,wz;		// resulting absolute coords
	CActor *pClickedActor = FindClickedActor(p.x, p.y);
	if (pClickedActor){
		if (pClickedActor->GetClassName() == CText::pszClassName)
			pClickedActor = NULL;	// do not click in Z-transparent object
		else if (pClickedActor->GetClassName() == CTransformArrow::pszClassName)
			pClickedActor = NULL;	// do not click in arrows
	}

        CActor *pCursor = m_Scene->GetActor(CScene::PA_CURSOR);
	wxASSERT(pCursor);
	if (pClickedActor){
		if (!UnProject(p.x, p.y, &wx, &wy, &wz)){
			wxASSERT(0);
			return;
		}
                CActor *pCamera = m_Scene->GetActor(CScene::PA_CAMERA);
		pCursor->vOrigin = CVector(wx, wy, wz) * pCamera->mRotation.Inverse();
		wxLogDebug(wxString::FromAscii("Clicked coord = {%f, %f, %f}\n"), pCursor->vOrigin.x, pCursor->vOrigin.y, pCursor->vOrigin.z);
	}
	((CCursor*)pCursor)->OnSelectActor(pClickedActor);
	Refresh(FALSE);
}

void C3DView::OnRightDoubleClick(wxMouseEvent& event)
{
	SetCurrent();

        CCamera *pCamera = (CCamera *)m_Scene->GetActor(CScene::PA_CAMERA);
	wxASSERT(pCamera);

	pCamera->vOrigin = CVector(0, 0, 0);
	pCamera->mRotation.Identity();

	// set appropriate zoom here
	// ....
	pCamera->UpdateProjection();

        m_Scene->siSceneInfo.pPolyBuffer->UpdateTransform();
        m_Scene->siSceneInfo.pPolyBuffer->SortTriangles();

	Refresh(FALSE);
}

void C3DView::SetupMaterial()
{
	glMateriali(GL_FRONT, GL_SHININESS, 50);
	float fSpec[] = {0.5f, 0.5f, 0.5f, 0.5f};
	glMaterialfv(GL_FRONT, GL_SPECULAR, fSpec);
}

bool C3DView::CommandEnableLighting(CParametersMap &paramsIn, CParametersMap &paramsOut)
{
	SetCurrent();

	if (paramsIn.GetInt(_T("enable"), true))
		glEnable(GL_LIGHTING);
	else
		glDisable(GL_LIGHTING);
	return true;
}

bool C3DView::CommandGetCurRoi(CParametersMap &paramsIn, CParametersMap &paramsOut)
{
	//SetCurrent();
	if(!m_curRoi){
		paramsOut.SetString(_T("error"), _T("No ROI is defined."));
		return false;
	}
        CDoubleArray *pInds = new CDoubleArray;     //RK 12/21/2009, will be freed by CParametersMap::~CParametersMap()
	if(!m_curRoi->getPoints(pInds)){
		paramsOut.SetString(_T("error"), _T("Error copying ROI coords."));
		return false;
	}
	paramsOut.SetArray(_T("vertices"), pInds);
	paramsOut.SetInt(_T("actor"), m_curRoi->getActorNum());
	return true;
}

wxString C3DView::GetNextImageFileName()
{
	char pFileName[20];
	
	int i = iNextScreenShotNumber;
	while (1)
	{
		sprintf(pFileName, "shot%04d.bmp", i);
		FILE *f = fopen(pFileName, "rb");
		if (!f)
			break;
		fclose(f);
		i++;
	}
	iNextScreenShotNumber = i+1;
	return wxString::FromAscii(pFileName);
}

bool C3DView::ProcessCommand(char *pCommand, bool *pRes, CParametersMap &paramsIn, CParametersMap &paramsOut)
{
	SetCurrent();//insure

	if (CCommandHandler::ProcessCommand(pCommand, pRes, paramsIn, paramsOut))
		return true;
	else
	{
		//SetCurrent();
                bool bProcessed = m_Scene->ProcessCommand(pCommand, pRes, paramsIn, paramsOut);
		if (bProcessed)
			Refresh(FALSE);
		return bProcessed;
	}
}

CActor* C3DView::FindClickedActor(int iMouseX, int iMouseY)
{
	SetCurrent();

	wxASSERT(pSelectBuffer);
	glSelectBuffer(iGLNameStackSize * 4, pSelectBuffer);

	int	viewport[4];
	glGetIntegerv(GL_VIEWPORT, viewport);

	glRenderMode(GL_SELECT);

	glMatrixMode(GL_PROJECTION);
	glPushMatrix();

	double	fCurProjMatrix[16];
	glGetDoublev(GL_PROJECTION_MATRIX, fCurProjMatrix);

	glLoadIdentity();
	gluPickMatrix(GLdouble(iMouseX), GLdouble(viewport[3] - iMouseY), 2, 2, viewport);
	glMultMatrixd(fCurProjMatrix);
//	glOrtho(fLeftClip, fRightClip, fBottomClip, fTopClip, fNearClip, fFarClip);

	glMatrixMode(GL_MODELVIEW);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
        m_Scene->Render(true);

	int nHits = glRenderMode(GL_RENDER);

	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);

	int	iActorID = -1;

	GLuint uMinDepth = 0xFFFFFFFF;
	for (int i=0; i<nHits*4; i+=4)
	{
		if (pSelectBuffer[i+1] < uMinDepth)
		{
			uMinDepth = pSelectBuffer[i+1];
			iActorID = pSelectBuffer[i+3];
		}
	}

	if (iActorID == -1)
		return NULL;

        return m_Scene->GetActor(iActorID);
}

bool C3DView::CommandRefresh(CParametersMap &paramsIn, CParametersMap &paramsOut)
{
        CCamera *pCamera = (CCamera *)m_Scene->GetActor(CScene::PA_CAMERA);
	
	pCamera->UpdateProjection();
	Refresh(FALSE);

	return true;
}

bool C3DView::CommandSetSize(CParametersMap &paramsIn, CParametersMap &paramsOut)
{
	int width	= paramsIn.GetInt(_T("width"), 0);
	int height	= paramsIn.GetInt(_T("height"), 0);
	
	if(width>0 && height>0)
	{
		SetSize(width, height);

		wxWindow		*pParentFrame = GetParent();
		//CClientFrame	*pParentFrameCast = wxDynamicCast(pParentFrame, CClientFrame);
		
		if (pParentFrame)
			pParentFrame->Fit();
	}
	else
	{
		paramsOut.AppendString(_T("error"), _T("width and height must be set to integers > 0"));
		return false;
	}

	return true;
}

bool C3DView::CommandSetWindowTitle(CParametersMap &paramsIn, CParametersMap &paramsOut)
{
//	wxString	strTitle;

//	if (paramsIn.GetString(_T("title"), &strTitle))
//	{
//		wxWindow	*pParentFrame = GetParent();
//
//		if (pParentFrame)
//			pParentFrame->SetTitle(strTitle);
//	}
	return true;
}

bool C3DView::CommandScreenShot(CParametersMap &paramsIn, CParametersMap &paramsOut)
{
	SetCurrent();

	int w, h;
	GetClientSize(&w, &h);

	if (w < 1 || h < 1)
	{
		paramsOut.SetString(_T("error"), _T("No client area to save"));
		wxASSERT(0);
		return false;
	}

	// 2003.10.21 RFD: we now return the image data
	unsigned char *pBuffer = new unsigned char[w*h*4];
	unsigned char *pMirrored = new unsigned char[w*h*3];
	if (!pBuffer || !pMirrored)
	{
		paramsOut.SetString(_T("error"), _T("Not enough memory"));
		if (pBuffer)
			delete[] pBuffer;
		if (pMirrored)
			delete[] pMirrored;
		return false;
	}
	
	// 2003.10.21 RFD:
	// Strange intermittent OpenGL bug. The following will give garbled data when 
	// getting GL_RGB format, usually only after the client window has been resized. 
	// Getting the data as GL_RGBA seems to work well every time. We just need to
	// strip away the alpha channel below.
	glReadPixels(0, 0, w, h, GL_RGBA, GL_UNSIGNED_BYTE, (void*)pBuffer);

	int i,j;
	// Remove the alpha channel
	for(i=0; i<w*h; i++){
		pBuffer[i*3] = pBuffer[i*4];
		pBuffer[i*3+1] = pBuffer[i*4+1];
		pBuffer[i*3+2] = pBuffer[i*4+2];
	}

	int iTotalBytes = w*h*3;
	// mirror it by hand because of wxImage strange do-nothing
	for (int iScanline = 0; iScanline < iTotalBytes; iScanline += w*3){
		memmove(pMirrored + iTotalBytes - iScanline - w*3, pBuffer + iScanline, w*3);
	}

	// *** RFD- FIX THIS! We should allow uchar arrays to be returned!
        CDoubleArray *pArray = new CDoubleArray;    //RK 12/21/2009, will be freed by CParametersMap::~CParametersMap()
  pArray->Create(3, w, h, 3);
  //double *pA = pArray->GetPointer();

	for(i=0; i<h; i++){
		for(j=0; j<w; j++){
			pArray->SetValue((double)pMirrored[i*w*3+j*3],j,i,0);
			pArray->SetValue((double)pMirrored[i*w*3+j*3+1],j,i,1);
			pArray->SetValue((double)pMirrored[i*w*3+j*3+2],j,i,2);
		}
	}
	paramsOut.SetArray(_T("rgb"), pArray);

	wxString	strFileName;
	if (!paramsIn.GetString(_T("filename"), &strFileName)){
		strFileName = GetNextImageFileName();
	}

	if(!strFileName.IsSameAs(_T("nosave"), false)){
		wxImage *pImage = new wxImage(w, h, pMirrored, TRUE);
		//pImage->Mirror(TRUE);	//since OpenGL y-axis is bottom-to-top
		// ^ does nothing by some reason
		if (!pImage){
			paramsOut.SetString(_T("error"), _T("Not enough memory"));
			delete[] pBuffer;
			delete[] pMirrored;
			return false;
		}

		if (!pImage->SaveFile(strFileName, wxBITMAP_TYPE_BMP))
			paramsOut.SetString(_T("error"), _T("Cannot write file"));
		delete pImage;
	}

	delete[] pBuffer;
	delete[] pMirrored;

	return true;
}

bool C3DView::SwitchMode(int mode){
	// In windows, be sure to #include wx/msw/wx.rc in your project .rc file.
	// Also, for VC, you need to specify the wxWidgets/include dir specifically
	// for 'resources'. Just specifying it as a C++ include doesn't work.
	switch(mode){
		case MODE_DRAW:
			m_mode = MODE_DRAW;
			SetCursor(wxCursor(wxCURSOR_PENCIL));
                        ((CServerConsole*)m_pConsole)->Log(_T("\nEntering draw mode- click to add points."));
			break;
		case MODE_FILL:
			m_mode = MODE_FILL;
			SetCursor(wxCursor(wxCURSOR_CROSS));
                        ((CServerConsole*)m_pConsole)->Log(_T("\nEntering fill mode- click in region to fill."));
			break;
		case MODE_INTERACT:
			m_mode = MODE_INTERACT;
			SetCursor(wxCursor(wxCURSOR_ARROW));
                        ((CServerConsole*)m_pConsole)->Log(_T("\nEntering interactive mode."));
			break;
		default:
			wxLogDebug(wxString::FromAscii("vertex out of range."));
			return(false);
	}
	return(true);
}
