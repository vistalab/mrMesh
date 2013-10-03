function newMsh = formatMesh(msh)
%
%    newMsh = formatMesh(msh)
%
% GB 02/13/05
%
% Updates the mesh mesh to a new format directly readable by the mex files
% Then we will be able to remove the field data of all meshes
%

newMsh = [];

fields = {'name','host','id','actor','lights','origin',...
    'initialvertices','vertices','triangles','colors','normals','curvature',...
    'ngraylayers','vertexGrayMap','fibers',...
    'smooth_sinc_method','smooth_relaxation','smooth_iterations','mod_depth'};

for ii = 1:length(fields)
    val = eval(['meshGet(msh,''' fields{ii} ''');']);
    if ~isempty(val)
        eval(['newMsh = meshSet(newMsh,''' fields{ii} ''',val);']);
    end
end