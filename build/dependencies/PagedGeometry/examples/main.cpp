/*-----------------------------------------------------------------------------
WARNING!!! main.cpp, Appliation.h, and Appliation.cpp are TEMPORARY source files
that demonstrate PagedGeometry. They are extremely messy, unstructured, and they
should NOT be taken as an example. This is the beta version of PagedGeometry 1.0
and does not include the examples and tutorials that the final version will have.
-----------------------------------------------------------------------------*/

#include "OgreException.h"
#include "Application.h"


#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
#define WIN32_LEAN_AND_MEAN
#include "windows.h"
#endif

#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
INT WINAPI WinMain( HINSTANCE hInst, HINSTANCE, LPSTR strCmdLine, INT )
#else
int main(int argc, char *argv[])
#endif
{
	//Create the application object
	Application a;
	app = &a;
	
	try {
		//Start the application
		app->go();
	} catch( Ogre::Exception& e ) {
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
		MessageBox(NULL, e.getFullDescription().c_str(), "Unhandled Exception", MB_OK);
#else
		//Catch any exceptions and display them
		std::cerr << "An exception has occurred: " <<
			e.getFullDescription().c_str() << std::endl;
#endif
	}

    return 0;
}