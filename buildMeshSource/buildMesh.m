function msh = buildMesh(voxels,mmPerVox)
%Build a VTK mesh from a class file
%  
%   msh = buildMesh(voxels,[mmPerVox])
%
% voxels can either by the name of a white matter file or it can be a 3D
% volume from a white matter file.
%
% 
% Examples:
%   fName ='X:\anatomy\nakadomari\left\20050901_fixV1\left.Class';
%   msh = buildMesh(fName);
%   visualizeMesh(msh);
%
% See also:  smoothMesh, colorMesh, visualizeMesh
%
% Author: GB

if ieNotDefined('voxels')
    error('This function needs input voxels or white matter class file name.');
end
if ieNotDefined('mmPerVox'), mmPerVox = [1 1 1]; end

if isstr(voxels)
    fprintf('Loading white matter voxels...\n');
    output = readClassFile(voxels);
    voxels = uint8(output.data == output.type.white);
end

fprintf('Building mesh...')
msh = build_mesh(voxels,mmPerVox);

% Set the mesh origin, by default, to the center of the object.
vertices = meshGet(msh,'vertices');
msh = meshSet(msh,'origin',-mean(vertices'));

fprintf('done. \n');

return;
