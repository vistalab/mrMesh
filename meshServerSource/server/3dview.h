#ifndef _3DVIEW_H_
#define _3DVIEW_H_
#include "scene.h"
#include "commandhandler.h"
#include "transformarrow.h"
#include <vector>
#include <list>
using namespace std;

struct roiPoint{
	unsigned char origColor[4];
	int vertex;
	bool userDefined;
};

typedef list<roiPoint> pointList;

class ROI;
class ROI{
 public:
	ROI(CActor *roiActor);
	~ROI();

	bool addPoint(int vertex, bool userDefined);
	void removePoint();
	void removeAllPoints();
	void addConnectedPoint(int vertex);
	void connectCurToFirst();
	int fill(int seedVert);
	void removeLastSegment();
	int fillPatch(int initialID);

	bool getPoints(CDoubleArray *inds);
	vector<int> *getPoints();
	int  getCurVertex(){ if(!points.empty()) return(points.back().vertex); else return(-1); }

	bool setActor(CActor *roiActor);
	CActor *getActor(){ return(actor); };
	int  getActorNum(){ if(actor) return(actor->GetID()); else return(-1); }
	int  getNumPoints(){ return(points.size()); };			
																								 
	vtkPolyData	*polyData;

 private:
	CActor *actor;
	pointList points;
};

class C3DView : public wxGLCanvas, CCommandHandler
{
 public:
	C3DView(wxWindow* parent, const wxPoint& pos, const wxSize& size, wxFrame* pConsole);
	virtual ~C3DView();

 private:
        // Scene rendering
        void OnPaint(wxPaintEvent& event);
	// Updates viewport size
	void OnSize(wxSizeEvent& event);
	// Does nothing. Useful on Windows.
	void OnEraseBackground(wxEraseEvent& event);
	// Rotate & move around actors and camera
	void OnMouseMove(wxMouseEvent& event);
	// Select actor
	void OnMouseDown(wxMouseEvent& event);
	// End of rotating and moving
	void OnMouseUp(wxMouseEvent& event);
	// Used to process hot-keys
	void OnKeyDown(wxKeyEvent& event);
	void OnKeyUp(wxKeyEvent& event);
	// Selects point on mesh surface (moves PA_CURSOR to clicked point)
	void OnLeftDoubleClick(wxMouseEvent& event);
	// Resets camera position
	void OnRightDoubleClick(wxMouseEvent& event);
	// Zoom
	void OnMouseWheel(wxMouseEvent& event);

 private:
	// Initial setup
	void InitGL();
	// Retrieve next numeric filename for screen shot (like shot0012.bmp)
	wxString GetNextImageFileName();

	// Sets GL material properties
	void SetupMaterial();

	// Find point in 3d-space by x,y coords of viewport
	bool UnProject(int vx, int vy, GLdouble *pX, GLdouble *pY, GLdouble *pZ);

	// Returns selected scene actor
	CActor*	FindClickedActor(int iMouseX, int iMouseY);

	// Changes pActiveActor and shows/hides manipulation arrows
	void	OnActorSelectionChange(CActor *pNewClickedActor);

	// Switches modes
	bool SwitchMode(int mode);

 private:
	// Pointer to the console who owns us
	wxFrame *m_pConsole;

	// true if InitGL was called for this view
	bool	bInitialized;

	// point at screen - used to calculate rotations angles and movement distance
	wxPoint		m_ClickPos;
	// point at which mouse move was started - used to perform selection
	//wxPoint		m_StartingMousePos;

	// Max depth of OpenGL select buffer
	int		iGLNameStackSize;

	// GL_SELECT rendering mode info
	GLuint	*pSelectBuffer;

	// Screenshot counter to prevent unnecessary opening of files
	int	iNextScreenShotNumber;
	
	// Flag for mesh ROI drawing mode 
	int m_mode;
	static const int MODE_INTERACT = 0;
	static const int MODE_DRAW = 1;
	static const int MODE_FILL = 2;

	// ROI struct
	ROI *m_curRoi;
 private:
        CScene	*m_Scene;       //RK 12/21/09: made a pointer, the static version crashed because CCursor constructor
                                //tried to access glGenList before InitGL
        void InitScene();       //RK 12/21/09: added to initialize m_Scene
	
	// Actor clicked with mouse that user is operating on
	CActor	*pActiveActor;

	// keep track of mouse operation
	enum	transform_axis
		{
			TA_NONE,
			TA_X,
			TA_Y,
			TA_Z
		}taAxis;

 private:
	//@{
	// Called by OnMouseMove.
	// @return true if object position was changed
	bool OnMouseMoveTransformActor(wxMouseEvent& event);
	bool OnMouseMoveTransformArrow(wxMouseEvent& event);
	bool OnMouseMoveTransformCamera(wxMouseEvent& event);
	//@}
	void MoveArrowToActiveActorOrigin();

 public:
	// Processes command or redirects for processing to CScene and its actors
	virtual	bool ProcessCommand(char *pCommand, bool *pRes, CParametersMap &paramsIn, CParametersMap &paramsOut);

 private:
	DECLARE_COMMAND_HANDLER(CommandScreenShot);
	DECLARE_COMMAND_HANDLER(CommandEnableLighting);
	DECLARE_COMMAND_HANDLER(CommandRefresh);
	DECLARE_COMMAND_HANDLER(CommandSetSize);
	DECLARE_COMMAND_HANDLER(CommandSetWindowTitle);
	DECLARE_COMMAND_HANDLER(CommandGetCurRoi);
 private:
        DECLARE_EVENT_TABLE()
        DECLARE_HANDLER_TABLE()
};

#endif //_3DVIEW_H_
