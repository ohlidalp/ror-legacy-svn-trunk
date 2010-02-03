/*
This file is part of Rigs of Rods
Copyright 2007 Pierre-Michel Ricordel
This source code must not be used or published without author's consent.
Contact: pierre-michel@ricordel.org
*/

#include "truckeditor.h"

#if OGRE_PLATFORM == OGRE_PLATFORM_LINUX
#include <dirent.h>
#endif
#if OGRE_PLATFORM == OGRE_PLATFORM_APPLE
#include <dirent.h>
#endif

SceneNode *mainPlaneX;
SceneNode *secPlaneX;
SceneNode *mainPlaneY;
SceneNode *secPlaneY;
SceneNode *mainPlaneZ;
SceneNode *secPlaneZ;
SceneNode *cursor3D;
float scale[4];

class View0Listener : public RenderTargetListener
{
private:
public:
    void preRenderTargetUpdate(const RenderTargetEvent& evt)
    {
		mainPlaneX->setVisible(true);
		secPlaneX->setVisible(scale[0]<5);
		mainPlaneY->setVisible(false);
		secPlaneY->setVisible(false);
		mainPlaneZ->setVisible(false);
		secPlaneZ->setVisible(false);
    }
    void postRenderTargetUpdate(const RenderTargetEvent& evt)
    {
    }

};

View0Listener viewlist0;

class View1Listener : public RenderTargetListener
{
private:
public:
    void preRenderTargetUpdate(const RenderTargetEvent& evt)
    {
		mainPlaneX->setVisible(false);
		secPlaneX->setVisible(false);
		mainPlaneY->setVisible(true);
		secPlaneY->setVisible(scale[1]<5);
		mainPlaneZ->setVisible(false);
		secPlaneZ->setVisible(false);
    }
    void postRenderTargetUpdate(const RenderTargetEvent& evt)
    {
    }

};

View1Listener viewlist1;

class View2Listener : public RenderTargetListener
{
private:
public:
    void preRenderTargetUpdate(const RenderTargetEvent& evt)
    {
		mainPlaneX->setVisible(false);
		secPlaneX->setVisible(false);
		mainPlaneY->setVisible(false);
		secPlaneY->setVisible(false);
		mainPlaneZ->setVisible(true);
		secPlaneZ->setVisible(scale[2]<5);
    }
    void postRenderTargetUpdate(const RenderTargetEvent& evt)
    {
    }

};

View2Listener viewlist2;

class View3Listener : public RenderTargetListener
{
private:
public:
    void preRenderTargetUpdate(const RenderTargetEvent& evt)
    {
		mainPlaneX->setVisible(false);
		secPlaneX->setVisible(false);
		mainPlaneY->setVisible(true);
		secPlaneY->setVisible(false);
		mainPlaneZ->setVisible(false);
		secPlaneZ->setVisible(false);
    }
    void postRenderTargetUpdate(const RenderTargetEvent& evt)
    {
    }

};

View3Listener viewlist3;


TruckEditor::TruckEditor(Overlay *screen, int width, int height)
	{
		free_openfile=0;
		openfile_offset=0;
		dirty=true;
		snapgrid=0.1;
		dragAction=DRAG_NONE;
		lastActive=0;
		lastclick=-1;
		currentName[0]=0;
		mouseX=width/2;
		mouseY=height/2;
		screenWidth=width;
		screenHeight=height;
		screenOverlay=screen;
		modalOverlay=screen;
		sceneman=Root::getSingleton().createSceneManager(ST_GENERIC);
		//some lighting
        sceneman->setAmbientLight(ColourValue(1.0,1.0,1.0));
		sceneman->setShadowTechnique(SHADOWTYPE_NONE);
        // Create a light
        Light* l = sceneman->createLight("Light");
		l->setType(Light::LT_DIRECTIONAL);
		l->setDirection(-1, -1, -1);
		l->setCastShadows(false);
		//testing with a cube
		/*
		Entity *ec = sceneman->createEntity("test", "kwhale.mesh");
		SceneNode *cnode = sceneman->getRootSceneNode()->createChildSceneNode();
		cnode->attachObject(ec);
		cnode->setPosition(0,0,0);
		cnode->setScale(2.0, 2.0, 2.0);
		*/
		//creating the RTT
		int c;
		char name[256];
		for (c=0; c<4; c++)
		{
			sprintf(name, "editor_%i", c);
			rttTexes[c]=Root::getSingleton().getRenderSystem()->createRenderTexture( name, 512, 512);
			{
				rttTexes[c]->setAutoUpdated(false);
				
				//determine the real screen size
				char tname[256];
				sprintf(tname, "editor/editorpanel%i", c);
				BorderPanelOverlayElement *bpoe=(BorderPanelOverlayElement*)(OverlayManager::getSingleton().getOverlayElement(tname));
				float rwidth=(bpoe->getWidth()-bpoe->getLeftBorderSize()-bpoe->getRightBorderSize())*(float)width;
				float rheight=(bpoe->getHeight()-bpoe->getTopBorderSize()-bpoe->getBottomBorderSize())*(float)height;

				cams[c]=sceneman->createCamera(name);
				cams[c]->setNearClipDistance(1.0);
				cams[c]->setFarClipDistance(100.0);
				cams[c]->setFOVy(Degree(50));
				cams[c]->setAspectRatio(rwidth/rheight);
				scale[c]=3.0;
				screenscale[c]=rheight/2.0;
				screencenterx[c]=(bpoe->getLeft()+bpoe->getWidth()/2.0)*(float)width;
				screencentery[c]=(bpoe->getTop()+bpoe->getHeight()/2.0)*(float)height;
				posx[c]=0;
				posy[c]=0;
				if (c!=3)
				{
					cams[c]->setProjectionType(PT_ORTHOGRAPHIC);
					cams[c]->setFOVy(Radian(2.0*atan2(scale[c], 1.0f)));
				}

				Viewport *v = rttTexes[c]->addViewport(cams[c]);
				v->setClearEveryFrame( true );
				v->setBackgroundColour(ColourValue(0.4, 0.4, 0.4));

				MaterialPtr mat = MaterialManager::getSingleton().getByName(name);
				mat->getTechnique(0)->getPass(0)->getTextureUnitState(0)->setTextureName(name);
				v->setOverlaysEnabled(false);

			}
		}
		cams[0]->setPosition(50, 0, 0);
		cams[1]->setPosition(0, 50, 0);
		cams[2]->setPosition(0, 0, 50);
		cams[3]->setPosition(10, 10, 10);
//		for (c=0; c<4; c++) cams[c]->lookAt(Vector3(0,0,0));
		for (c=0; c<4; c++) if (c!=1) cams[c]->setDirection(-cams[c]->getPosition());
		cams[1]->pitch(Degree(-90));

        mouseElement = OverlayManager::getSingleton().getOverlayElement("mouse/pointer");
        helpTextOE = (TextAreaOverlayElement*)OverlayManager::getSingleton().getOverlayElement("editor/HelpText");
		openOverlay=OverlayManager::getSingleton().getByName("tracks/TruckEditorOpenOverlay");
		nodeContextOverlay=OverlayManager::getSingleton().getByName("tracks/NodeContextOverlay");
		beamContextOverlay=OverlayManager::getSingleton().getByName("tracks/BeamContextOverlay");
		contextOverlay=0;
		createGrid();
		rttTexes[0]->addListener(&viewlist0);
		rttTexes[1]->addListener(&viewlist1);
		rttTexes[2]->addListener(&viewlist2);
		rttTexes[3]->addListener(&viewlist3);
		truck=new TruckEditorElement(sceneman, snapgrid);
		setTool(TOOL_MOVE);
		//selection tool
		selectionOverlay=OverlayManager::getSingleton().getByName("tracks/MouseSelectionOverlay");
        selectionElement = OverlayManager::getSingleton().getOverlayElement("mouse/selection");


		/*
		beamsChain=sceneman->createBillboardChain("BeamsChain");
		SceneNode *bnode = sceneman->getRootSceneNode()->createChildSceneNode();
		bnode->attachObject(beamsChain);
		bnode->setPosition(0,0,0);
		bnode->setVisible(true);
		*/
	};

	void TruckEditor::createGrid()
	{
		//create the planes
		int i;
		int counter;
		//X
		DynamicLines *dlxr1=new DynamicLines("editor/basegrid",RenderOperation::OT_LINE_LIST);
		dlxr1->addPoint(Vector3(0, 0, -50));
		dlxr1->addPoint(Vector3(0, 0, 50));
		dlxr1->update();
		dlxr1->setRenderQueueGroup(RENDER_QUEUE_2);
		DynamicLines *dlxr2=new DynamicLines("editor/basegrid",RenderOperation::OT_LINE_LIST);
		dlxr2->addPoint(Vector3(0, -50, 0));
		dlxr2->addPoint(Vector3(0, 50, 0));
		dlxr2->update();
		dlxr2->setRenderQueueGroup(RENDER_QUEUE_2);
		baseNode=sceneman->getRootSceneNode()->createChildSceneNode();
		baseNode->attachObject(dlxr1);
		baseNode->attachObject(dlxr2);
		baseNode->setPosition(0,0,0);

		DynamicLines *dlx=new DynamicLines("editor/maingrid",RenderOperation::OT_LINE_LIST);
		counter=0;
		for (i=-50; i<=50; i++)
		{
			if (i==0) continue;
			if (counter%2)
			{
				dlx->addPoint(Vector3(0, i, -50));
				dlx->addPoint(Vector3(0, i, 50));
			}
			else
			{
				dlx->addPoint(Vector3(0, i, -50));
				dlx->addPoint(Vector3(0, i, 50));
			}
			counter++;
		}
		counter=0;
		for (i=-50; i<=50; i++)
		{
			if (i==0) continue;
			if (counter%2)
			{
				dlx->addPoint(Vector3(0, -50, i));
				dlx->addPoint(Vector3(0, 50, i));
			}
			else
			{
				dlx->addPoint(Vector3(0, 50, i));
				dlx->addPoint(Vector3(0, -50, i));
			}
			counter++;
		}
		dlx->update();
		dlx->setRenderQueueGroup(RENDER_QUEUE_1);
		mainPlaneX=sceneman->getRootSceneNode()->createChildSceneNode();
		mainPlaneX->attachObject(dlx);
		mainPlaneX->setPosition(0,0,0);

		DynamicLines *dlxs=new DynamicLines("editor/secgrid",RenderOperation::OT_LINE_LIST);
		counter=0;
		for (i=-500; i<=500; i++)
		{
			if (i%10==0) continue;
			if (counter%2)
			{
				dlxs->addPoint(Vector3(0, (float)i/10.0, -50));
				dlxs->addPoint(Vector3(0, (float)i/10.0, 50));
			}
			else
			{
				dlxs->addPoint(Vector3(0, (float)i/10.0, -50));
				dlxs->addPoint(Vector3(0, (float)i/10.0, 50));
			}
			counter++;
		}
		counter=0;
		for (i=-500; i<=500; i++)
		{
			if (i%10==0) continue;
			if (counter%2)
			{
				dlxs->addPoint(Vector3(0, -50, (float)i/10.0));
				dlxs->addPoint(Vector3(0, 50, (float)i/10.0));
			}
			else
			{
				dlxs->addPoint(Vector3(0, 50, (float)i/10.0));
				dlxs->addPoint(Vector3(0, -50, (float)i/10.0));
			}
			counter++;
		}
		dlxs->update();
		dlxs->setRenderQueueGroup(RENDER_QUEUE_BACKGROUND);
		secPlaneX=sceneman->getRootSceneNode()->createChildSceneNode();
		secPlaneX->attachObject(dlxs);
		secPlaneX->setPosition(0,0,0);

		//Y
		DynamicLines *dlyr1=new DynamicLines("editor/basegrid",RenderOperation::OT_LINE_LIST);
		dlyr1->addPoint(Vector3(-50, 0, 0));
		dlyr1->addPoint(Vector3(50, 0, 0));
		dlyr1->update();
		dlyr1->setRenderQueueGroup(RENDER_QUEUE_2);
		baseNode->attachObject(dlyr1);

		DynamicLines *dly=new DynamicLines("editor/maingrid",RenderOperation::OT_LINE_LIST);
		counter=0;
		for (i=-50; i<=50; i++)
		{
			if (i==0) continue;
			if (counter%2)
			{
				dly->addPoint(Vector3(i, 0, -50));
				dly->addPoint(Vector3(i, 0, 50));
			}
			else
			{
				dly->addPoint(Vector3(i, 0, -50));
				dly->addPoint(Vector3(i, 0, 50));
			}
			counter++;
		}
		counter=0;
		for (i=-50; i<=50; i++)
		{
			if (i==0) continue;
			if (counter%2)
			{
				dly->addPoint(Vector3(-50, 0, i));
				dly->addPoint(Vector3(50, 0, i));
			}
			else
			{
				dly->addPoint(Vector3(50, 0, i));
				dly->addPoint(Vector3(-50, 0, i));
			}
			counter++;
		}
		dly->update();
		dly->setRenderQueueGroup(RENDER_QUEUE_1);
		mainPlaneY=sceneman->getRootSceneNode()->createChildSceneNode();
		mainPlaneY->attachObject(dly);
		mainPlaneY->setPosition(0,0,0);

		DynamicLines *dlys=new DynamicLines("editor/secgrid",RenderOperation::OT_LINE_LIST);
		counter=0;
		for (i=-500; i<=500; i++)
		{
			if (i%10==0) continue;
			if (counter%2)
			{
				dlys->addPoint(Vector3((float)i/10.0, 0, -50));
				dlys->addPoint(Vector3((float)i/10.0, 0, 50));
			}
			else
			{
				dlys->addPoint(Vector3((float)i/10.0, 0, -50));
				dlys->addPoint(Vector3((float)i/10.0, 0, 50));
			}
			counter++;
		}
		counter=0;
		for (i=-500; i<=500; i++)
		{
			if (i%10==0) continue;
			if (counter%2)
			{
				dlys->addPoint(Vector3(-50, 0, (float)i/10.0));
				dlys->addPoint(Vector3(50, 0, (float)i/10.0));
			}
			else
			{
				dlys->addPoint(Vector3(50, 0, (float)i/10.0));
				dlys->addPoint(Vector3(-50, 0, (float)i/10.0));
			}
			counter++;
		}
		dlys->update();
		dlys->setRenderQueueGroup(RENDER_QUEUE_BACKGROUND);
		secPlaneY=sceneman->getRootSceneNode()->createChildSceneNode();
		secPlaneY->attachObject(dlys);
		secPlaneY->setPosition(0,0,0);


		//Z

		DynamicLines *dlz=new DynamicLines("editor/maingrid",RenderOperation::OT_LINE_LIST);
		counter=0;
		for (i=-50; i<=50; i++)
		{
			if (i==0) continue;
			if (counter%2)
			{
				dlz->addPoint(Vector3(i, -50, 0));
				dlz->addPoint(Vector3(i, 50, 0));
			}
			else
			{
				dlz->addPoint(Vector3(i, -50, 0));
				dlz->addPoint(Vector3(i, 50, 0));
			}
			counter++;
		}
		counter=0;
		for (i=-50; i<=50; i++)
		{
			if (i==0) continue;
			if (counter%2)
			{
				dlz->addPoint(Vector3(-50, i, 0));
				dlz->addPoint(Vector3(50, i, 0));
			}
			else
			{
				dlz->addPoint(Vector3(50, i, 0));
				dlz->addPoint(Vector3(-50, i, 0));
			}
			counter++;
		}
		dlz->update();
		dlz->setRenderQueueGroup(RENDER_QUEUE_1);
		mainPlaneZ=sceneman->getRootSceneNode()->createChildSceneNode();
		mainPlaneZ->attachObject(dlz);
		mainPlaneZ->setPosition(0,0,0);

		DynamicLines *dlzs=new DynamicLines("editor/secgrid",RenderOperation::OT_LINE_LIST);
		counter=0;
		for (i=-500; i<=500; i++)
		{
			if (i%10==0) continue;
			if (counter%2)
			{
				dlzs->addPoint(Vector3((float)i/10.0, -50, 0));
				dlzs->addPoint(Vector3((float)i/10.0, 50, 0));
			}
			else
			{
				dlzs->addPoint(Vector3((float)i/10.0, -50, 0));
				dlzs->addPoint(Vector3((float)i/10.0, 50, 0));
			}
			counter++;
		}
		counter=0;
		for (i=-500; i<=500; i++)
		{
			if (i%10==0) continue;
			if (counter%2)
			{
				dlzs->addPoint(Vector3(-50, (float)i/10.0, 0));
				dlzs->addPoint(Vector3(50, (float)i/10.0, 0));
			}
			else
			{
				dlzs->addPoint(Vector3(50, (float)i/10.0, 0));
				dlzs->addPoint(Vector3(-50, (float)i/10.0, 0));
			}
			counter++;
		}
		dlzs->update();
		dlzs->setRenderQueueGroup(RENDER_QUEUE_BACKGROUND);
		secPlaneZ=sceneman->getRootSceneNode()->createChildSceneNode();
		secPlaneZ->attachObject(dlzs);
		secPlaneZ->setPosition(0,0,0);

		//cursor
		DynamicLines *cx=new DynamicLines("editor/cursor3D",RenderOperation::OT_LINE_LIST);
		cx->addPoint(Vector3(0, 0, -50));
		cx->addPoint(Vector3(0, 0, 50));
		cx->update();
		cx->setRenderQueueGroup(RENDER_QUEUE_9);
		DynamicLines *cy=new DynamicLines("editor/cursor3D",RenderOperation::OT_LINE_LIST);
		cy->addPoint(Vector3(0, -50, 0));
		cy->addPoint(Vector3(0, 50, 0));
		cy->update();
		cy->setRenderQueueGroup(RENDER_QUEUE_9);
		DynamicLines *cz=new DynamicLines("editor/cursor3D",RenderOperation::OT_LINE_LIST);
		cz->addPoint(Vector3(-50, 0, 0));
		cz->addPoint(Vector3(50, 0, 0));
		cz->setRenderQueueGroup(RENDER_QUEUE_9);
		cz->update();
		cursor3D=sceneman->getRootSceneNode()->createChildSceneNode();
		cursor3D->attachObject(cx);
		cursor3D->attachObject(cy);
		cursor3D->attachObject(cz);
		cursor3D->setPosition(0,0,0);

	}

int TruckEditor::updateOpenFiles()
{
   free_openfile=0;
   openfile_offset=0;
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
   WIN32_FIND_DATA FindFileData;
   HANDLE hFind = INVALID_HANDLE_VALUE;
   char DirSpec[MAX_PATH];  // directory specification
   DWORD dwError;

   strcpy(DirSpec, "data\\trucks\\*.truck");

   hFind = FindFirstFile(DirSpec, &FindFileData);

   if (hFind == INVALID_HANDLE_VALUE) 
   {
      //printf ("Invalid file handle. Error is %u\n", GetLastError());
      return (-1);
   } 
   else 
   {
//      printf ("First file name is %s\n", FindFileData.cFileName);
	   strcpy(openfiles[free_openfile], FindFileData.cFileName);
	   free_openfile++;
      while (FindNextFile(hFind, &FindFileData) != 0) 
      {
//         printf ("Next file name is %s\n", FindFileData.cFileName);
	   strcpy(openfiles[free_openfile], FindFileData.cFileName);
	   free_openfile++;
      }
      dwError = GetLastError();
      FindClose(hFind);
      if (dwError != ERROR_NO_MORE_FILES) 
      {
//         printf ("FindNextFile error. Error is %u\n", dwError);
         return (-1);
      }
   }
   //now for the loads
   strcpy(DirSpec, "data\\trucks\\*.load");

   hFind = FindFirstFile(DirSpec, &FindFileData);

   if (hFind == INVALID_HANDLE_VALUE) 
   {
      //printf ("Invalid file handle. Error is %u\n", GetLastError());
      return (-1);
   } 
   else 
   {
//      printf ("First file name is %s\n", FindFileData.cFileName);
	   strcpy(openfiles[free_openfile], FindFileData.cFileName);
	   free_openfile++;
      while (FindNextFile(hFind, &FindFileData) != 0) 
      {
//         printf ("Next file name is %s\n", FindFileData.cFileName);
	   strcpy(openfiles[free_openfile], FindFileData.cFileName);
	   free_openfile++;
      }
      dwError = GetLastError();
      FindClose(hFind);
      if (dwError != ERROR_NO_MORE_FILES) 
      {
//         printf ("FindNextFile error. Error is %u\n", dwError);
         return (-1);
      }
   }
#endif
#if OGRE_PLATFORM == OGRE_PLATFORM_LINUX
   char spec[256];
   {
	DIR *dirp;
//   printf("Scanning %s\n", spec);
   strcpy(spec, "data\\trucks\\*.truck");
	char prespec[256];
	strcpy(prespec, spec);
   char* strpt=prespec;
   while (*strpt!='*') strpt++;
	*strpt=0;
   strpt=prespec;
   while (*strpt!=0) {if (*strpt=='\\') *strpt='/';strpt++;};
  // printf("Prespec %s\n", prespec);
	dirp = opendir(prespec);
	struct dirent *dp;
	if (dirp != NULL)
	{
		while ((dp = readdir(dirp)) != NULL)
		{
//   printf(" Found %s\n", dp->d_name);
			char* pt1;
			char* pt2;
			pt1=dp->d_name+strlen(dp->d_name);
			pt2=spec+strlen(spec);
			bool equal=true;
			while (*pt2!='.') {equal=equal && (*pt1==*pt2);pt1--; pt2--;};
			if (equal)
				{
				   strcpy(openfiles[free_openfile], dp->d_name);
				   free_openfile++;
				};
		}
		(void)closedir(dirp);
    }
	}
	//and for the loads
   {
	DIR *dirp;
//   printf("Scanning %s\n", spec);
   strcpy(spec, "data\\trucks\\*.load");
	char prespec[256];
	strcpy(prespec, spec);
   char* strpt=prespec;
   while (*strpt!='*') strpt++;
	*strpt=0;
   strpt=prespec;
   while (*strpt!=0) {if (*strpt=='\\') *strpt='/';strpt++;};
  // printf("Prespec %s\n", prespec);
	dirp = opendir(prespec);
	struct dirent *dp;
	if (dirp != NULL)
	{
		while ((dp = readdir(dirp)) != NULL)
		{
//   printf(" Found %s\n", dp->d_name);
			char* pt1;
			char* pt2;
			pt1=dp->d_name+strlen(dp->d_name);
			pt2=spec+strlen(spec);
			bool equal=true;
			while (*pt2!='.') {equal=equal && (*pt1==*pt2);pt1--; pt2--;};
			if (equal)
				{
				   strcpy(openfiles[free_openfile], dp->d_name);
				   free_openfile++;
				};
		}
		(void)closedir(dirp);
    }
}
#endif   
#if OGRE_PLATFORM == OGRE_PLATFORM_APPLE
   char spec[256];
   {
	DIR *dirp;
//   printf("Scanning %s\n", spec);
   strcpy(spec, "data\\trucks\\*.truck");
	char prespec[256];
	strcpy(prespec, spec);
   char* strpt=prespec;
   while (*strpt!='*') strpt++;
	*strpt=0;
   strpt=prespec;
   while (*strpt!=0) {if (*strpt=='\\') *strpt='/';strpt++;};
  // printf("Prespec %s\n", prespec);
	dirp = opendir(prespec);
	struct dirent *dp;
	if (dirp != NULL)
	{
		while ((dp = readdir(dirp)) != NULL)
		{
//   printf(" Found %s\n", dp->d_name);
			char* pt1;
			char* pt2;
			pt1=dp->d_name+strlen(dp->d_name);
			pt2=spec+strlen(spec);
			bool equal=true;
			while (*pt2!='.') {equal=equal && (*pt1==*pt2);pt1--; pt2--;};
			if (equal)
				{
				   strcpy(openfiles[free_openfile], dp->d_name);
				   free_openfile++;
				};
		}
		(void)closedir(dirp);
    }
	}
	//and for the loads
   {
	DIR *dirp;
//   printf("Scanning %s\n", spec);
   strcpy(spec, "data\\trucks\\*.load");
	char prespec[256];
	strcpy(prespec, spec);
   char* strpt=prespec;
   while (*strpt!='*') strpt++;
	*strpt=0;
   strpt=prespec;
   while (*strpt!=0) {if (*strpt=='\\') *strpt='/';strpt++;};
  // printf("Prespec %s\n", prespec);
	dirp = opendir(prespec);
	struct dirent *dp;
	if (dirp != NULL)
	{
		while ((dp = readdir(dirp)) != NULL)
		{
//   printf(" Found %s\n", dp->d_name);
			char* pt1;
			char* pt2;
			pt1=dp->d_name+strlen(dp->d_name);
			pt2=spec+strlen(spec);
			bool equal=true;
			while (*pt2!='.') {equal=equal && (*pt1==*pt2);pt1--; pt2--;};
			if (equal)
				{
				   strcpy(openfiles[free_openfile], dp->d_name);
				   free_openfile++;
				};
		}
		(void)closedir(dirp);
    }
}
#endif   
   updateOpenFilesVisu();
   return (0);
	}

	void TruckEditor::updateOpenFilesVisu()
	{
		char oname[256];
		int i;
		for (i=0; i<14; i++)
		{
			sprintf(oname, "activetext/OpenFile%.2iText",i);
			if (i+openfile_offset<free_openfile)
				OverlayManager::getSingleton().getOverlayElement(oname)->setCaption(openfiles[i+openfile_offset]);
			else
				OverlayManager::getSingleton().getOverlayElement(oname)->setCaption("");
		}
	}

	void TruckEditor::showOpen(bool s)
	{
		if (s)
		{
			openOverlay->show();
			updateOpenFiles();
			modalOverlay=openOverlay;
		}
		else
		{
			openOverlay->hide();
			modalOverlay=screenOverlay;
		}
	}

	void TruckEditor::selectScroll(int v)
	{
		if (v==-1 && openfile_offset>0) openfile_offset--;
		if (v==1 && free_openfile-openfile_offset>12) openfile_offset++;
		updateOpenFilesVisu();
	}

	void TruckEditor::selectOpen(char *ename)
	{
		int i;
		char name[256];
		//clear all
		for (i=0; i<14; i++)
		{
			sprintf(name, "active/OpenFile%.2iPanel", i);
			OverlayManager::getSingleton().getOverlayElement(name)->setMaterialName("gui/background");
		}
		//check if valid name
		ename[10]=0;
		sprintf(name, "activetext/%sText", ename);
		if (OverlayManager::getSingleton().getOverlayElement(name)->getCaption().length()>0)
		{
			//select
			sprintf(name, "active/%sPanel", ename);
			OverlayManager::getSingleton().getOverlayElement(name)->setMaterialName("gui/selected");
			lastActive=0;
		}
	}

	char *TruckEditor::getLoadPointedTruck()
	{
		char name[256];
		int i;
		for (i=0; i<14; i++)
		{
			sprintf(name, "active/OpenFile%.2iPanel", i);
			if (!strcmp(OverlayManager::getSingleton().getOverlayElement(name)->getMaterialName().c_str(), "gui/selected"))
			{
				sprintf(name, "activetext/OpenFile%.2iText", i);
				strcpy(currentName, OverlayManager::getSingleton().getOverlayElement(name)->getCaption().asUTF8_c_str());
				return currentName;
			}
		}
		return 0;
	}

	void TruckEditor::showSave()
	{
		truck->save("generated.truck");
	}

	void TruckEditor::showEngine()
	{
	}

	bool TruckEditor::mouseClick(int x, int y, int button, OIS::Keyboard *modifier)
	{
		OverlayElement *element=modalOverlay->findElementAt((float)x/(float)screenWidth,(float)y/(float)screenHeight);
		if (element)
		{
			char name[256];
			char *ename=name;
			strcpy(name,element->getName().c_str());
			while (*ename!='/' && *ename!=0) ename++;
			if (*ename) ename++;
			if (!strncmp(ename, "MenuNew", 7)) clearAll();
			if (!strncmp(ename, "MenuOpen", 8)) showOpen(true);
			if (!strncmp(ename, "MenuSave", 8)) showSave();
			if (!strncmp(ename, "MenuEngine", 10)) showEngine();
			if (!strncmp(ename, "MenuExit", 8)) {setTool(TOOL_MOVE);return false;};
			if (!strncmp(ename, "OpenCancel", 10)) showOpen(false);
			if (!strncmp(ename, "OpenOpen", 8)) {showOpen(false);loadTruck(getLoadPointedTruck());}
			if (!strncmp(ename, "OpenFile", 8)) selectOpen(ename);
			if (!strncmp(ename, "OpenScrollUp",12)) selectScroll(-1);
			if (!strncmp(ename, "OpenScrollDown",14)) selectScroll(1);
			if (!strncmp(ename, "EditMove", 8)) setTool(TOOL_MOVE);
			if (!strncmp(ename, "EditSelectNode", 14)) setTool(TOOL_SELECT);
			if (!strncmp(ename, "EditSelectBeam", 14)) setTool(TOOL_SELECTBEAM);
			if (!strncmp(ename, "EditBuild", 9)) setTool(TOOL_BUILD);
			if (!strncmp(ename, "editorpanel0", 12)) clickView(0, x,y,button,modifier);
			if (!strncmp(ename, "editorpanel1", 12)) clickView(1, x,y,button,modifier);
			if (!strncmp(ename, "editorpanel2", 12)) clickView(2, x,y,button,modifier);
			if (!strncmp(ename, "editorpanel3", 12)) clickView(3, x,y,button,modifier);
		}
		//process also the context overlay
		if (contextOverlay)
		{
			OverlayElement *element=contextOverlay->findElementAt((float)x/(float)screenWidth,(float)y/(float)screenHeight);
			if (element)
			{
				char name[256];
				char *ename=name;
				strcpy(name,element->getName().c_str());
				while (*ename!='/' && *ename!=0) ename++;
				if (*ename) ename++;
				if (!strncmp(name, "spin",4))
				{
					//we hit a spincontrol
					OverlayElement *parent=element->getParent();
					if (parent->getTypeName()=="SpinControl")
					{
						((SpinControl*)parent)->clickEvent(name);
					}
					//update the spins as necessary : nodecontext
					if (((SpinControl*)OverlayManager::getSingleton().getOverlayElement("spin/tiemaxlen"))->getSpinEnable())
						truck->setSelectedNodesTieLength(((SpinControl*)OverlayManager::getSingleton().getOverlayElement("spin/tiemaxlen"))->getSpinValue());
					if (((SpinControl*)OverlayManager::getSingleton().getOverlayElement("spin/tierate"))->getSpinEnable())
						truck->setSelectedNodesTieRate(((SpinControl*)OverlayManager::getSingleton().getOverlayElement("spin/tierate"))->getSpinValue());
					if (((SpinControl*)OverlayManager::getSingleton().getOverlayElement("spin/tiestop"))->getSpinEnable())
						truck->setSelectedNodesTieStop(((SpinControl*)OverlayManager::getSingleton().getOverlayElement("spin/tiestop"))->getSpinValue()/100.0);
					//update the spins as necessary : beamcontext
					if (((SpinControl*)OverlayManager::getSingleton().getOverlayElement("spin/beamhydrorate"))->getSpinEnable())
						truck->setSelectedBeamsHydroRate(((SpinControl*)OverlayManager::getSingleton().getOverlayElement("spin/beamhydrorate"))->getSpinValue()/100.0);
					if (((SpinControl*)OverlayManager::getSingleton().getOverlayElement("spin/beamshockk"))->getSpinEnable())
						truck->setSelectedBeamsShockK(((SpinControl*)OverlayManager::getSingleton().getOverlayElement("spin/beamshockk"))->getSpinValue()*1000.0);
					if (((SpinControl*)OverlayManager::getSingleton().getOverlayElement("spin/beamshockd"))->getSpinEnable())
						truck->setSelectedBeamsShockD(((SpinControl*)OverlayManager::getSingleton().getOverlayElement("spin/beamshockd"))->getSpinValue()*1000.0);
					if (((SpinControl*)OverlayManager::getSingleton().getOverlayElement("spin/beamshockshort"))->getSpinEnable())
						truck->setSelectedBeamsShockShort(((SpinControl*)OverlayManager::getSingleton().getOverlayElement("spin/beamshockshort"))->getSpinValue()/100.0);
					if (((SpinControl*)OverlayManager::getSingleton().getOverlayElement("spin/beamshocklong"))->getSpinEnable())
						truck->setSelectedBeamsShockLong(((SpinControl*)OverlayManager::getSingleton().getOverlayElement("spin/beamshocklong"))->getSpinValue()/100.0);
					if (((SpinControl*)OverlayManager::getSingleton().getOverlayElement("spin/beamshockprecomp"))->getSpinEnable())
						truck->setSelectedBeamsShockPrecomp(((SpinControl*)OverlayManager::getSingleton().getOverlayElement("spin/beamshockprecomp"))->getSpinValue()/100.0);
					if (((SpinControl*)OverlayManager::getSingleton().getOverlayElement("spin/beamcommandrate"))->getSpinEnable())
						truck->setSelectedBeamsCommandRate(((SpinControl*)OverlayManager::getSingleton().getOverlayElement("spin/beamcommandrate"))->getSpinValue()/100.0);
					if (((SpinControl*)OverlayManager::getSingleton().getOverlayElement("spin/beamcommandshortbound"))->getSpinEnable())
						truck->setSelectedBeamsCommandShortBound(((SpinControl*)OverlayManager::getSingleton().getOverlayElement("spin/beamcommandshortbound"))->getSpinValue()/100.0);
					if (((SpinControl*)OverlayManager::getSingleton().getOverlayElement("spin/beamcommandshortkey"))->getSpinEnable())
						truck->setSelectedBeamsCommandShortKey(((SpinControl*)OverlayManager::getSingleton().getOverlayElement("spin/beamcommandshortkey"))->getSpinValue());
					if (((SpinControl*)OverlayManager::getSingleton().getOverlayElement("spin/beamcommandlongbound"))->getSpinEnable())
						truck->setSelectedBeamsCommandLongBound(((SpinControl*)OverlayManager::getSingleton().getOverlayElement("spin/beamcommandlongbound"))->getSpinValue()/100.0);
					if (((SpinControl*)OverlayManager::getSingleton().getOverlayElement("spin/beamcommandlongkey"))->getSpinEnable())
						truck->setSelectedBeamsCommandLongKey(((SpinControl*)OverlayManager::getSingleton().getOverlayElement("spin/beamcommandlongkey"))->getSpinValue());

				}
				else
				{
					//nodes modes
					if (!strncmp(ename, "editnoderadio", 13)) truck->setSelectedNodesMode(ename[13]);
					if (!strncmp(ename, "editnodecheckfixed", 18)) truck->setSelectedNodesFixed(!truck->getSelectedNodesFixed());
					if (!strncmp(ename, "editnodecheckcontacter", 22)) truck->setSelectedNodesContacter(!truck->getSelectedNodesContacter());
					if (!strncmp(ename, "editnodecheckropable", 20)) truck->setSelectedNodesRopable(!truck->getSelectedNodesRopable());
					if (!strncmp(ename, "editnodechecktie", 16)) truck->setSelectedNodesTie(!truck->getSelectedNodesTie());
					//beams modes
					if (!strncmp(ename, "editbeamradiostructural",23)) truck->setSelectedBeamsMode(EBEAM_STRUCTURAL);
					if (!strncmp(ename, "editbeamradiorope",17)) truck->setSelectedBeamsMode(EBEAM_ROPE);
					if (!strncmp(ename, "editbeamradiohydro",18)) truck->setSelectedBeamsMode(EBEAM_HYDRO);
					if (!strncmp(ename, "editbeamradioshock",18)) truck->setSelectedBeamsMode(EBEAM_SHOCK);
					if (!strncmp(ename, "editbeamradiocommand",20)) truck->setSelectedBeamsMode(EBEAM_COMMAND);
					//beams checks
					if (!strncmp(ename, "editbeamstructuralinvisible",27) && truck->getSelectedBeamsMode()==EBEAM_STRUCTURAL) truck->setSelectedBeamsStructuralVisible(!truck->getSelectedBeamsStructuralVisible());

					updateContextOverlay();
				}
			}
		}
		return true;
	}

	void TruckEditor::updateContextOverlay()
	{
		if (contextOverlay==nodeContextOverlay)
		{
			//nodes
			setRadioBitmap("radio/editnoderadion", truck->getSelectedNodesMode('n'));
			setRadioBitmap("radio/editnoderadiol", truck->getSelectedNodesMode('l'));
			setRadioBitmap("radio/editnoderadiof", truck->getSelectedNodesMode('f'));
			setRadioBitmap("radio/editnoderadiox", truck->getSelectedNodesMode('x'));
			setRadioBitmap("radio/editnoderadioy", truck->getSelectedNodesMode('y'));
			setRadioBitmap("radio/editnoderadioc", truck->getSelectedNodesMode('c'));
			setRadioBitmap("radio/editnoderadioh", truck->getSelectedNodesMode('h'));
			setRadioBitmap("radio/editnoderadioe", truck->getSelectedNodesMode('e'));
			setRadioBitmap("radio/editnoderadiob", truck->getSelectedNodesMode('b'));
			setCheckBitmap("check/editnodecheckfixed", truck->getSelectedNodesFixed());
			setCheckBitmap("check/editnodecheckcontacter", truck->getSelectedNodesContacter());
			setCheckBitmap("check/editnodecheckropable", truck->getSelectedNodesRopable());
			setCheckBitmap("check/editnodechecktie", truck->getSelectedNodesTie());
			//spins
			if (truck->getSelectedNodesTie()==SEL_TRUE && truck->numSelected()==1)
			{
				((SpinControl*)OverlayManager::getSingleton().getOverlayElement("spin/tiemaxlen"))->setSpinValue(truck->getSelectedNodesTieLength());
				((SpinControl*)OverlayManager::getSingleton().getOverlayElement("spin/tiemaxlen"))->setSpinEnable(true);
				((SpinControl*)OverlayManager::getSingleton().getOverlayElement("spin/tierate"))->setSpinValue(truck->getSelectedNodesTieRate());
				((SpinControl*)OverlayManager::getSingleton().getOverlayElement("spin/tierate"))->setSpinEnable(true);
				((SpinControl*)OverlayManager::getSingleton().getOverlayElement("spin/tiestop"))->setSpinValue(truck->getSelectedNodesTieStop()*100.0);
				((SpinControl*)OverlayManager::getSingleton().getOverlayElement("spin/tiestop"))->setSpinEnable(true);
			}else
			{
				((SpinControl*)OverlayManager::getSingleton().getOverlayElement("spin/tiemaxlen"))->setSpinEnable(false);
				((SpinControl*)OverlayManager::getSingleton().getOverlayElement("spin/tierate"))->setSpinEnable(false);
				((SpinControl*)OverlayManager::getSingleton().getOverlayElement("spin/tiestop"))->setSpinEnable(false);
			}
		}
		if (contextOverlay==beamContextOverlay)
		{
			setRadioBitmap("radio/editbeamradiostructural", SEL_FALSE);
			setCheckBitmap("check/editbeamstructuralinvisible", SEL_FALSE);

			setRadioBitmap("radio/editbeamradiorope", SEL_FALSE);

			setRadioBitmap("radio/editbeamradiohydro", SEL_FALSE);
			((SpinControl*)OverlayManager::getSingleton().getOverlayElement("spin/beamhydrorate"))->setSpinEnable(false);

			setRadioBitmap("radio/editbeamradioshock", SEL_FALSE);
			((SpinControl*)OverlayManager::getSingleton().getOverlayElement("spin/beamshockk"))->setSpinEnable(false);
			((SpinControl*)OverlayManager::getSingleton().getOverlayElement("spin/beamshockd"))->setSpinEnable(false);
			((SpinControl*)OverlayManager::getSingleton().getOverlayElement("spin/beamshockshort"))->setSpinEnable(false);
			((SpinControl*)OverlayManager::getSingleton().getOverlayElement("spin/beamshocklong"))->setSpinEnable(false);
			((SpinControl*)OverlayManager::getSingleton().getOverlayElement("spin/beamshockprecomp"))->setSpinEnable(false);

			setRadioBitmap("radio/editbeamradiocommand", SEL_FALSE);
			setCheckBitmap("check/editbeamcommandinvisible", SEL_FALSE);
			((SpinControl*)OverlayManager::getSingleton().getOverlayElement("spin/beamcommandrate"))->setSpinEnable(false);
			((SpinControl*)OverlayManager::getSingleton().getOverlayElement("spin/beamcommandshortbound"))->setSpinEnable(false);
			((SpinControl*)OverlayManager::getSingleton().getOverlayElement("spin/beamcommandshortkey"))->setSpinEnable(false);
			((SpinControl*)OverlayManager::getSingleton().getOverlayElement("spin/beamcommandlongbound"))->setSpinEnable(false);
			((SpinControl*)OverlayManager::getSingleton().getOverlayElement("spin/beamcommandlongkey"))->setSpinEnable(false);

			int radiomode=truck->getSelectedBeamsMode();
			if (truck->numBeamSelected()>=1)
			{
				switch (radiomode)
				{
				case EBEAM_STRUCTURAL:
					setRadioBitmap("radio/editbeamradiostructural", SEL_TRUE);
					setCheckBitmap("check/editbeamstructuralinvisible", truck->getSelectedBeamsStructuralVisible());
					break;
				case EBEAM_ROPE:
					setRadioBitmap("radio/editbeamradiorope", SEL_TRUE);
					break;
				case EBEAM_HYDRO:
					setRadioBitmap("radio/editbeamradiohydro", SEL_TRUE);
					((SpinControl*)OverlayManager::getSingleton().getOverlayElement("spin/beamhydrorate"))->setSpinEnable(true);
					break;
				case EBEAM_SHOCK:
					setRadioBitmap("radio/editbeamradioshock", SEL_TRUE);
					((SpinControl*)OverlayManager::getSingleton().getOverlayElement("spin/beamshockk"))->setSpinEnable(true);
					((SpinControl*)OverlayManager::getSingleton().getOverlayElement("spin/beamshockk"))->setSpinValue(truck->getSelectedBeamsShockK()/1000.0);
					((SpinControl*)OverlayManager::getSingleton().getOverlayElement("spin/beamshockd"))->setSpinEnable(true);
					((SpinControl*)OverlayManager::getSingleton().getOverlayElement("spin/beamshockd"))->setSpinValue(truck->getSelectedBeamsShockD()/1000.0);
					((SpinControl*)OverlayManager::getSingleton().getOverlayElement("spin/beamshockshort"))->setSpinEnable(true);
					((SpinControl*)OverlayManager::getSingleton().getOverlayElement("spin/beamshockshort"))->setSpinValue(truck->getSelectedBeamsShockShort()*100.0);
					((SpinControl*)OverlayManager::getSingleton().getOverlayElement("spin/beamshocklong"))->setSpinEnable(true);
					((SpinControl*)OverlayManager::getSingleton().getOverlayElement("spin/beamshocklong"))->setSpinValue(truck->getSelectedBeamsShockLong()*100.0);
					((SpinControl*)OverlayManager::getSingleton().getOverlayElement("spin/beamshockprecomp"))->setSpinEnable(true);
					((SpinControl*)OverlayManager::getSingleton().getOverlayElement("spin/beamshockprecomp"))->setSpinValue(truck->getSelectedBeamsShockPrecomp()*100.0);
					break;
				case EBEAM_COMMAND:
					setRadioBitmap("radio/editbeamradiocommand", SEL_TRUE);
					//setCheckBitmap("check/editbeamcommandinvisible", truck->getSelectedBeamsCommandVisible());
					((SpinControl*)OverlayManager::getSingleton().getOverlayElement("spin/beamcommandrate"))->setSpinEnable(true);
					((SpinControl*)OverlayManager::getSingleton().getOverlayElement("spin/beamcommandrate"))->setSpinValue(truck->getSelectedBeamsCommandRate());
					((SpinControl*)OverlayManager::getSingleton().getOverlayElement("spin/beamcommandshortbound"))->setSpinEnable(true);
					((SpinControl*)OverlayManager::getSingleton().getOverlayElement("spin/beamcommandshortbound"))->setSpinValue(truck->getSelectedBeamsCommandShortBound()*100.0);
					((SpinControl*)OverlayManager::getSingleton().getOverlayElement("spin/beamcommandshortkey"))->setSpinEnable(true);
					((SpinControl*)OverlayManager::getSingleton().getOverlayElement("spin/beamcommandshortkey"))->setSpinValue(truck->getSelectedBeamsCommandShortKey());
					((SpinControl*)OverlayManager::getSingleton().getOverlayElement("spin/beamcommandlongbound"))->setSpinEnable(true);
					((SpinControl*)OverlayManager::getSingleton().getOverlayElement("spin/beamcommandlongbound"))->setSpinValue(truck->getSelectedBeamsCommandLongBound()*100.0);
					((SpinControl*)OverlayManager::getSingleton().getOverlayElement("spin/beamcommandlongkey"))->setSpinEnable(true);
					((SpinControl*)OverlayManager::getSingleton().getOverlayElement("spin/beamcommandlongkey"))->setSpinValue(truck->getSelectedBeamsCommandLongKey());
					break;
				case EBEAM_MIXED:
					setRadioBitmap("radio/editbeamradiostructural", SEL_MIXED);
					setRadioBitmap("radio/editbeamradiorope", SEL_MIXED);
					setRadioBitmap("radio/editbeamradiohydro", SEL_MIXED);
					setRadioBitmap("radio/editbeamradioshock", SEL_MIXED);
					setRadioBitmap("radio/editbeamradiocommand", SEL_MIXED);
					break;
				}
			}
		}
	}

	void TruckEditor::setRadioBitmap(char *name, int value)
	{
		switch (value)
		{
		case SEL_FALSE: OverlayManager::getSingleton().getOverlayElement(name)->setMaterialName("gui/radiounchecked");break;
		case SEL_TRUE: OverlayManager::getSingleton().getOverlayElement(name)->setMaterialName("gui/radiochecked");break;
		case SEL_MIXED: OverlayManager::getSingleton().getOverlayElement(name)->setMaterialName("gui/radiosemichecked");break;
		}
	}

	void TruckEditor::setCheckBitmap(char *name, int value)
	{
		switch (value)
		{
		case SEL_FALSE: OverlayManager::getSingleton().getOverlayElement(name)->setMaterialName("gui/checkunchecked");break;
		case SEL_TRUE: OverlayManager::getSingleton().getOverlayElement(name)->setMaterialName("gui/checkchecked");break;
		case SEL_MIXED: OverlayManager::getSingleton().getOverlayElement(name)->setMaterialName("gui/checksemichecked");break;
		}
	}

	void TruckEditor::setTool(int ttype)
	{
		OverlayManager::getSingleton().getOverlayElement("active/EditMoveIconPanel")->setMaterialName("gui/background");
		OverlayManager::getSingleton().getOverlayElement("active/EditSelectNodeIconPanel")->setMaterialName("gui/background");
		OverlayManager::getSingleton().getOverlayElement("active/EditSelectBeamIconPanel")->setMaterialName("gui/background");
		OverlayManager::getSingleton().getOverlayElement("active/EditBuildIconPanel")->setMaterialName("gui/background");
		setContext(CTX_NONE);
		switch(ttype)
		{
		case TOOL_MOVE: OverlayManager::getSingleton().getOverlayElement("active/EditMoveIconPanel")->setMaterialName("gui/selected");break;
		case TOOL_SELECT: OverlayManager::getSingleton().getOverlayElement("active/EditSelectNodeIconPanel")->setMaterialName("gui/selected");setContext(CTX_NODE);break;
		case TOOL_SELECTBEAM: OverlayManager::getSingleton().getOverlayElement("active/EditSelectBeamIconPanel")->setMaterialName("gui/selected");setContext(CTX_BEAM);break;
		case TOOL_BUILD: OverlayManager::getSingleton().getOverlayElement("active/EditBuildIconPanel")->setMaterialName("gui/selected");break;
		}
		lastActive=0;
		toolMode=ttype;
	}

	void TruckEditor::setContext(int ctx)
	{
		if (contextOverlay) contextOverlay->hide();
		switch (ctx)
		{
		case CTX_NONE: contextOverlay=0;break;
		case CTX_NODE: contextOverlay=nodeContextOverlay;truck->switchSelectionMode(SELECTMODE_NODE);updateContextOverlay();break;
		case CTX_BEAM: contextOverlay=beamContextOverlay;truck->switchSelectionMode(SELECTMODE_BEAM);updateContextOverlay();break;
		}
		if (contextOverlay) contextOverlay->show();
	}

	void TruckEditor::mouseZoom(int x, int y, int zoom)
	{
		OverlayElement *element=modalOverlay->findElementAt((float)x/(float)screenWidth,(float)y/(float)screenHeight);
		if (element)
		{
			char name[256];
			char *ename=name;
			strcpy(name,element->getName().c_str());
			while (*ename!='/' && *ename!=0) ename++;
			if (*ename) ename++;
			if (!strncmp(ename, "editorpanel0", 12)) zoomView(0, zoom);
			if (!strncmp(ename, "editorpanel1", 12)) zoomView(1, zoom);
			if (!strncmp(ename, "editorpanel2", 12)) zoomView(2, zoom);
			if (!strncmp(ename, "editorpanel3", 12)) zoomView(3, zoom);
		}
	}

	void TruckEditor::mouseHover(int x, int y)
	{
		OverlayElement *element=modalOverlay->findElementAt((float)x/(float)screenWidth,(float)y/(float)screenHeight);
		if (element)
		{
			char name[256];
			strcpy(name,element->getName().c_str());
			if (!strncmp(name, "active", 6))
			{
				//this is an active region
				OverlayElement *toactivate=element;
				if (!strncmp(name, "activetext", 10))
				{
					toactivate=element->getParent();
				}
				if (strcmp("gui/selected",toactivate->getMaterialName().c_str()))
				{
					toactivate->setMaterialName("gui/active");
					if (lastActive && lastActive!=toactivate) lastActive->setMaterialName("gui/background");
					lastActive=toactivate;
					//setHelpMessage(toactivate->getParameter("helptext").c_str());
				}
				else if (lastActive) {lastActive->setMaterialName("gui/background");lastActive=0;};
			} else {
				if (lastActive) {lastActive->setMaterialName("gui/background");lastActive=0;};
			}
			//write coordinates
			char *ename=name;
			strcpy(name,element->getName().c_str());
			while (*ename!='/' && *ename!=0) ename++;
			if (*ename) ename++;
			if (!strncmp(ename, "editorpanel0", 12)) showInfos(0, x, y);
			if (!strncmp(ename, "editorpanel1", 12)) showInfos(1, x, y);
			if (!strncmp(ename, "editorpanel2", 12)) showInfos(2, x, y);
		//	if (!strncmp(ename, "editorpanel3", 12)) showInfos(3, x, y);
		}
	}

	void TruckEditor::setHelpMessage(const char* msg)
	{
		helpTextOE->setCaption(msg);
	}

	bool TruckEditor::processInput(OIS::Mouse *mInputReader, OIS::Keyboard *modifier)
	{
		bool ret=true;
		//the mouse stuff
		const OIS::MouseState mstate=mInputReader->getMouseState();
		mouseX+=mstate.X.rel;
		if (mouseX<0) mouseX=0;
		if (mouseX>screenWidth-1) mouseX=screenWidth-1;
		mouseY+=mstate.Y.rel;
		if (mouseY<0) mouseY=0;
		if (mouseY>screenHeight-1) mouseY=screenHeight-1;
		int dZ=mstate.Z.rel;
		//position
		mouseElement->setPosition(mouseX, mouseY);
		//action
		int click=-1;
		if (mstate.buttonDown(OIS::MB_Left)) click=0;
		if (mstate.buttonDown(OIS::MB_Right)) click=1;
		if (click==-1) 
		{
			if (dragAction!=DRAG_NONE) mouseDragFinish(mouseX, mouseY, click, modifier);
			dragAction=DRAG_NONE;
		}
		//drag
		if (click!=-1 && lastclick==click) mouseDrag(mouseX-lastmX, mouseY-lastmY, click, mouseX, mouseY);
		else
		{
			if (click!=-1) ret=mouseClick(mouseX, mouseY, click,modifier);
		}
		if (dZ!=0) mouseZoom(mouseX, mouseY, dZ);
		mouseHover(mouseX, mouseY);
		//keys
		if (modifier->isKeyDown(OIS::KC_DELETE)) 
		{
			if (toolMode==TOOL_SELECT) truck->deleteSelectedNodes();
			if (toolMode==TOOL_SELECTBEAM) truck->deleteSelectedBeams();
		}
		if (modifier->isKeyDown(OIS::KC_PGUP))
		{
			mouseZoom(mouseX, mouseY, 60);
		}
		if (modifier->isKeyDown(OIS::KC_PGDOWN))
		{
			mouseZoom(mouseX, mouseY, -60);
		}
		
		//refresh
		if (truck->dirty)
		{
			refresh();
			truck->dirty=false;
		}
		if (mouseX!=lastmX || mouseY!=lastmY || dZ!=0 || click!=-1) dirty=true;
		lastmX=mouseX;
		lastmY=mouseY;
		lastclick=click;
		return ret;
	}

	void TruckEditor::clearAll()
	{
		truck->clear(true);
	}

	void TruckEditor::loadTruck(char* truckname)
	{
		if (!truckname) return;

		// fixed unused variable r
		//int r=truck->load(truckname);
		truck->load(truckname);
		
//		char debug[256];
//		sprintf(debug, "Result:%i Nodes:%i Beams:%i", r, truck->free_node, truck->free_beam);
//		setHelpMessage(debug);
	}

	void TruckEditor::refresh()
	{
		int i;
		for (i=0; i<4; i++) rttTexes[i]->update();
		dirty=true;
	}

	void TruckEditor::zoomView(int c, int zoom)
	{
		scale[c]*=(1.0+(float)zoom/1200.0);
		if (c!=4) cams[c]->setFOVy(Radian(2.0*atan2(scale[c], 1.0f)));
		refresh();
	}

	void TruckEditor::clickView(int c, int x, int y, int button, OIS::Keyboard *modifier)
	{
		if (toolMode==TOOL_BUILD)
		{
			//this is a node creation
			truck->appendNode(convertMouse3World(c, Vector2(x,y), true), c);

			dragContext=c;
		}
		if (toolMode==TOOL_SELECT || toolMode==TOOL_SELECTBEAM)
		{
			if (toolMode==TOOL_SELECT) dragAction=DRAG_SELECT;
			if (toolMode==TOOL_SELECTBEAM) dragAction=DRAG_SELECTBEAM;
			Vector3 pos=convertMouse3World(c, Vector2(x,y), false);
			selectstart=pos;
			dragContext=c;
			selectionBoxBegin(x,y);
		}
		if (toolMode==TOOL_MOVE)
		{
			dragAction=DRAG_VIEW;

			Vector3 pos=convertMouse3World(c, Vector2(x,y), true);
			if ((c==1 || c==2) && pos.x==cursor3D->getPosition().x) dragAction=DRAG_CURSOR_X;
			if ((c==0 || c==2) && pos.y==cursor3D->getPosition().y) dragAction=DRAG_CURSOR_Y;
			if ((c==0 || c==1) && pos.z==cursor3D->getPosition().z) dragAction=DRAG_CURSOR_Z;

			if (truck->isDraggable(pos, c, snapgrid/2.0)) {dragstart=pos; dragAction=DRAG_SELNODES;};

			dragContext=c;
		}
	}

	Vector2 TruckEditor::convertMouse2World(int c, Vector2 mousepos)
	{
		return Vector2(posx[c]-scale[c]*(mousepos.x-screencenterx[c])/screenscale[c], posy[c]-scale[c]*(mousepos.y-screencentery[c])/screenscale[c]);
	}

	Vector3 TruckEditor::convertMouse3World(int c, Vector2 mousepos, bool snap)
	{
		Vector2 world2=convertMouse2World(c, mousepos);
		Vector3 world3;
		switch (c)
		{
		case 0: world3=Vector3(cursor3D->getPosition().x, world2.y, world2.x);break;
		case 1: world3=Vector3(-world2.x, cursor3D->getPosition().y, -world2.y);break;
		case 2: world3=Vector3(-world2.x, world2.y, cursor3D->getPosition().z);break;
		}
		if (snap)
		{
			world3.x=(float)round(world3.x/snapgrid)*snapgrid;
			world3.y=(float)round(world3.y/snapgrid)*snapgrid;
			world3.z=(float)round(world3.z/snapgrid)*snapgrid;
		}
		return world3;
	}

	void TruckEditor::showInfos(int c, int x, int y)
	{
		Vector3 world3=convertMouse3World(c, Vector2((float)x,(float)y), true);
		char mesg[256];
		sprintf(mesg, "X=%f Y=%f Z=%f", world3.x, world3.y, world3.z);
		if (toolMode==TOOL_SELECT && truck->numSelected()>0)
		{
			//show selected
			strcat(mesg," Selected:");
			int nb=truck->numSelected();
			if (nb>10) nb=10;
			int i;
			for (i=0; i<nb; i++) sprintf(mesg, "%s %i", mesg, truck->selected_nodes[i]);
			if (truck->numSelected()>10) strcat(mesg, " ...");
		}
		setHelpMessage(mesg);
	}

	void TruckEditor::mouseDrag(int dx, int dy, int button, int x, int y)
	{
		int c=dragContext;
		if (dragAction==DRAG_VIEW)
		{
			posx[c]+=(float)dx*scale[c]/screenscale[c];
			posy[c]+=(float)dy*scale[c]/screenscale[c];
			switch (c)
			{
			case 0: cams[0]->setPosition(Vector3(50, posy[0], posx[0]));break;
			case 1: cams[1]->setPosition(Vector3(-posx[1], 50, -posy[1]));break;
			case 2: cams[2]->setPosition(Vector3(-posx[2], posy[2], 50));break;
			}
		}
		if (dragAction==DRAG_CURSOR_X)
		{
			Vector3 pos=convertMouse3World(c, Vector2(x,y),true);
			Vector3 cpos=cursor3D->getPosition();
			cpos.x=pos.x;
			cursor3D->setPosition(cpos);
		}
		if (dragAction==DRAG_CURSOR_Y)
		{
			Vector3 pos=convertMouse3World(c, Vector2(x,y),true);
			Vector3 cpos=cursor3D->getPosition();
			cpos.y=pos.y;
			cursor3D->setPosition(cpos);
		}
		if (dragAction==DRAG_CURSOR_Z)
		{
			Vector3 pos=convertMouse3World(c, Vector2(x,y),true);
			Vector3 cpos=cursor3D->getPosition();
			cpos.z=pos.z;
			cursor3D->setPosition(cpos);
		}
		if (dragAction==DRAG_SELECT || dragAction==DRAG_SELECTBEAM)
		{
			selectionBoxUpdate(x,y);
		}

		if (dragAction==DRAG_SELNODES)
		{
			Vector3 pos=convertMouse3World(c, Vector2(x,y),true);
			truck->moveSelected(pos-dragstart);
			dragstart=pos;
		}

		refresh();
	}

	void TruckEditor::mouseDragFinish(int x,int y, int button, OIS::Keyboard *modifier)
	{
		int c=dragContext;
		if (dragAction==DRAG_SELECT || dragAction==DRAG_SELECTBEAM)
		{
			int type=SELECT_NEW;
			if (modifier->isKeyDown(OIS::KC_LSHIFT)||modifier->isKeyDown(OIS::KC_RSHIFT)) type=SELECT_ADD;
			else if (modifier->isKeyDown(OIS::KC_LCONTROL)||modifier->isKeyDown(OIS::KC_RCONTROL)) type=SELECT_DEL;
			Vector3 end=convertMouse3World(c, Vector2(x,y),false);
			switch (c)
			{
			case 0: selectstart.x=-5000; end.x=5000;break;
			case 1: selectstart.y=-5000; end.y=5000;break;
			case 2: selectstart.z=-5000; end.z=5000;break;
			}
			if (selectstart.x>end.x){float t=selectstart.x; selectstart.x=end.x; end.x=t;};
			if (selectstart.y>end.y){float t=selectstart.y; selectstart.y=end.y; end.y=t;};
			if (selectstart.z>end.z){float t=selectstart.z; selectstart.z=end.z; end.z=t;};
			if (dragAction==DRAG_SELECT) truck->selectNodes(selectstart, end, type);
			if (dragAction==DRAG_SELECTBEAM) truck->selectBeams(selectstart, end, type);
			selectionBoxEnd();
			updateContextOverlay();
		}
	}

	void TruckEditor::selectionBoxBegin(int x, int y)
	{
		selectionBoxX=x;
		selectionBoxY=y;
		selectionBoxUpdate(x,y);
		selectionOverlay->show();
	}
	void TruckEditor::selectionBoxUpdate(int x, int y)
	{
		int minx, maxx, miny, maxy;
		if (x<selectionBoxX) {minx=x; maxx=selectionBoxX;} else {maxx=x; minx=selectionBoxX;};
		if (y<selectionBoxY) {miny=y; maxy=selectionBoxY;} else {maxy=y; miny=selectionBoxY;};
		selectionElement->setPosition(minx,miny);
		selectionElement->setDimensions(maxx-minx, maxy-miny);
	}
	void TruckEditor::selectionBoxEnd()
	{
		selectionOverlay->hide();
	}

	TruckEditor::~TruckEditor()
	{
	}
