function newMsh = colorMesh(msh,modDepth)
%Color a VTK mesh according to its local curvature
%
%   newMsh = colorMesh(msh,[modDepth])
%
% The modDepth specifies how much contrast is included around the mean gray
% value of the surface.  The surface mean is 127.5 (digital values).  The
% mod_depth expands the curvature to 
% 
%    ((2*curvature>0) - 1) * mod_depth * 128 + 127.5.  
%
% The msh can either be a file name to a white matter classification
% file or one of our VTK mesh data structures (see meshGet, meshSet)
%
% If modDepth is not sent in, the mshGet(msh,'mod_depth') is checked.  If
% that field is empty, then the default is 0.20.
%
% Example:
%  fName ='X:\anatomy\nakadomari\left\20050901_fixV1\left.Class';
%  msh = buildMesh(fName);
%  msh = smoothMesh(msh);
%  msh = colorMesh(msh);
%  visualizeMesh(msh);
%
% Author GB

if ieNotDefined('msh'), error('This function needs a mesh input'); end

if isstr(msh), msh = buildMesh(msh); end

if ieNotDefined('modDepth')
    modDepth = meshGet(msh,'mod_depth');
    if isempty(modDepth), modDepth = 0.20;  end
end
msh = meshSet(msh,'mod_depth',modDepth);

fprintf('Coloring mesh using curvature ...');
coloredMsh = curvature(msh);
fprintf('done\n');
newMsh = msh;
newMsh = meshSet(newMsh,'vertices',meshGet(coloredMsh,'vertices'));
newMsh = meshSet(newMsh,'colors',meshGet(coloredMsh,'colors'));
newMsh = meshSet(newMsh,'normals',meshGet(coloredMsh,'normals'));
newMsh = meshSet(newMsh,'triangles',meshGet(coloredMsh,'triangles'));
newMsh = meshSet(newMsh,'curvature',meshGet(coloredMsh,'curvature'));

curvColorIntensity = 128*meshGet(newMsh,'mod_depth'); % mesh.curvature_mod_depth;
monochrome = uint8(round((double(newMsh.curvature>0)*2-1)*curvColorIntensity+127.5));

colors = meshGet(newMsh,'colors');
colors(1:3,:) = repmat(monochrome,[3 1]);
newMsh = meshSet(newMsh,'colors',colors);

return;