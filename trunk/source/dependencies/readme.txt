Here are ALL* dependencies CURRENTLY USED by RoR.exe, Configurator.exe and Updater.exe

*With the exception of 
-wxWidgets

Windows build

The RoRdeps.sln is supposed to be able to build all the dependencies in this directory, with the exception of:
-pthread (using prebuild dlls for extra safety)

Procedure to add a dependency: 
-add the project to the solution
-if the project depends on another one (e.g. OgreMain) set the dependency in the solution (right-click on solution->Projects Dependencies...)
-if the project generates a dll that needs to be in the runtime directory, make the project "postbuild" depend on it, and add in the "postbuild" project properties->Configuration Properties->Build Events->Post-Build Events an extra command line "copy <source dll> ..\..\bin\release\windows"

ONLY RELEASE is supported currently!



