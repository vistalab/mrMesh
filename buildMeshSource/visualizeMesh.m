function [msh , lights] = visualizeMesh(msh,id);
% Visualize a VTK mesh using mrMesh
%
%  [msh] = visualizeMesh(msh,id);
% 
% oldMsh is input and the mesh is displayed in mrMesh window number id.
% The changed mesh, which has a lot of new structures, can be returned.
%
% Example :
%
%  fName ='X:\anatomy\nakadomari\left\20050901_fixV1\left.Class';
%  msh = buildMesh(fName);
%  msh = smoothMesh(msh);
%  visualizeMesh(msh);
%

meshName = '';
backColor = [1,1,1];  

if ieNotDefined('msh'), error('The mesh is required.'); end
if ieNotDefined('mmPerVox'), mmPerVox = [1 1 1]; end
if ieNotDefined('host'), host = 'localhost'; end
if ieNotDefined('id'), id = -1; end

% Set initial parameters for the mesh.
if isempty(meshGet(msh,'host')), msh = meshSet(msh,'host',host); end
if isempty(meshGet(msh,'id')),   msh = meshSet(msh,'id',id); end
if isempty(meshGet(msh,'name')), msh = meshSet(msh,'name',meshName); end

% If the window is already open, no harm is done.
if mrmCheckServer, mrMesh(meshGet(msh,'host'),meshGet(msh,'id'),'close'); end
msh = mrmInitHostWindow(msh); 

% Initializes the mesh
[msh, lights] = mrmInitMesh(msh,backColor);

% Adds an actor
% msh = mrmSet(msh,'addactor');

% Updates data
% msh = meshSet(msh,'vertices',meshGet(oldMsh,'vertices'));
% msh = meshSet(msh,'colors',meshGet(oldMsh,'colors'));
% msh = meshSet(msh,'normals',meshGet(oldMsh,'normals'));
% msh = meshSet(msh,'triangles',meshGet(oldMsh,'triangles'));

% Sets the origin
% msh = mrmSet(msh,'origin',-mean(meshGet(msh,'vertices')'));

% msh = meshSet(msh,'camera_space',0);
% msh = meshSet(msh,'rotation',eye(3));

% Loads the mesh to the viewer
% mrmSet(msh,'data');

return;



