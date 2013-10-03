function buildMex(files)
%Makefile to compile the files needed to create a mesh
%
% buildMex([files])
%
% The three files you have to compile to create a mesh are:
%   - build_mesh.cpp
%   - smooth_mesh.cpp
%   - curvature.cpp
%
% If you don't specify any argument, this routine compile the three mex
% files by default
%
% Examples:
%   buildMex
%   buildMex({'build_mesh.cpp','smooth_mesh.cpp'});
%

if ispc
    depth = '..\..\';
    includePath = ['-I' depth 'VISTAPACK\VTK\include\Cygwin'];
    libraryPath = [depth 'VISTAPACK\VTK\lib\Cygwin'];
    
    % This flag enables not to display the long warning concerning VTK
    % deprecated header includes 
    flags = 'COMPFLAGS#"$COMPFLAGS -Wno-deprecated"';
else
    depth = '../../';
    includePath = ['-I' depth 'VISTAPACK/VTK/include/Linux'];
    libraryPath = [depth 'VISTAPACK/VTK/lib/Linux'];
    
    % This flag enables not to display the long warning concerning VTK
    % deprecated header includes 
    flags = 'CXXFLAGS=''$CXXFLAGS -Wno-deprecated''';
end

if ieNotDefined('files')
    files = {'build_mesh.cpp','smooth_mesh.cpp','curvature.cpp'};
elseif isstr(files)
    files = {files};
end

libraries = {'libvtkCommon.a','libvtkFiltering.a','libvtkGraphics.a'};

for i = 1:length(files)
    command = 'mex ';
    command = [command files{i} ' ' flags ' ' includePath ' '];

    for j = 1:length(libraries)
        command = [command libraryPath filesep libraries{j} ' '];
    end

    % You MUST put the names of the libraries twice because of circular
    % dependances
    
    for j = 1:length(libraries)
        command = [command libraryPath filesep libraries{j} ' '];
    end
    
    fprintf(['Compiling ' files{i} '...\n'])
    eval(command);
end

fprintf('\n');
fprintf([num2str(length(files)) ' files compiled successfully.\n']);
fprintf(['Copying the dll files to the directory VISTASOFT/Anatomy/mrMesh\n']);

vistaSoftPath = [depth 'VISTASOFT/Anatomy/mrMesh'];

if ispc, copyfile('*.dll',vistaSoftPath);
else copyfile('*.mexglx',vistaSoftPath);
end
        
return;
