/*
This source file is part of Rigs of Rods
Copyright 2005-2012 Pierre-Michel Ricordel
Copyright 2007-2012 Thomas Fischer

For more information, see http://www.rigsofrods.com/

Rigs of Rods is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License version 3, as
published by the Free Software Foundation.

Rigs of Rods is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Rigs of Rods.  If not, see <http://www.gnu.org/licenses/>.
*/
#include "envmap.h"

#include "Beam.h"
#include "Settings.h"
#include "SkyManager.h"

using namespace Ogre;

Envmap::Envmap(SceneManager *mSceneMgr, RenderWindow *mWindow, Camera *incam, bool dynamic) :
	  debug_sn(0)
	, isDynamic(dynamic)
	, inited(false)
	, round(0)
{
	texture = TextureManager::getSingleton().createManual("EnvironmentTexture",
		ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME, TEX_TYPE_CUBE_MAP, 256, 256, 0,
		PF_R8G8B8, TU_RENDERTARGET);

	for (int face = 0; face < 6; face++)
	{
		rt[face] = texture->getBuffer(face)->getRenderTarget();
		// rt->addListener(this);

		char name[256];
		sprintf(name, "EnvironmentCamera-%i\n", face);
		camera[face] = mSceneMgr->createCamera(name);
		camera[face]->setAspectRatio(1.0);
		camera[face]->setProjectionType(PT_PERSPECTIVE);
		camera[face]->setFOVy(Degree(90));
		camera[face]->setNearClipDistance(0.1); //why this does not work, I wonder.
		camera[face]->setFarClipDistance(incam->getFarClipDistance());
		//ent->getParentSceneNode()->attachObject(camera[face]);

		Viewport *v = rt[face]->addViewport(camera[face]);
		v->setOverlaysEnabled(false);
		v->setClearEveryFrame(true);
		v->setBackgroundColour(incam->getViewport()->getBackgroundColour());
		rt[face]->setAutoUpdated(false);

		switch (face)
		{
		case 0:
			camera[face]->setDirection(-1, 0, 0);   // <-- should be +X
			break;
		case 1:
			camera[face]->setDirection(1, 0, 0);   // <-- should be -X
			break;
		case 2:
			camera[face]->setDirection(0, 0, 1);
			camera[face]->pitch(Degree(90));
			break;
		case 3:
			camera[face]->setDirection(0, 0, 1);
			camera[face]->pitch(Degree(-90));
			break;
		case 4:
			camera[face]->setDirection(0, 0, 1);
			break;
		case 5:
			camera[face]->setDirection(0, 0, -1);
			break;
		}
	}

	bool debug = BSETTING("EnvMapDebug", false);
	if(debug)
	{
		// create fancy mesh for debugging the envmap
		Overlay *overlay = OverlayManager::getSingleton().create("EnvMapDebugOverlay");
		if(overlay)
		{
			Vector3 position = Vector3::ZERO;
			Vector3 scale = Vector3(1,1,1);
			
			MeshPtr mesh = MeshManager::getSingletonPtr()->createManual("cubeMapDebug", ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
			// create sub mesh
			SubMesh *sub = mesh->createSubMesh();

			// Initialize render operation
			sub->operationType = RenderOperation::OT_TRIANGLE_LIST;
			//
			sub->useSharedVertices = true;
			mesh->sharedVertexData = new VertexData;
			sub->indexData = new IndexData;

			// Create vertex declaration
			size_t offset = 0;
			VertexDeclaration* vertexDeclaration = mesh->sharedVertexData->vertexDeclaration;
			vertexDeclaration->addElement(0, offset, VET_FLOAT3, VES_POSITION);
			offset += VertexElement::getTypeSize(VET_FLOAT3);
			vertexDeclaration->addElement(0, offset, VET_FLOAT3, VES_TEXTURE_COORDINATES);
			offset += VertexElement::getTypeSize(VET_FLOAT3);

			// Create and bind vertex buffer
			mesh->sharedVertexData->vertexCount = 14;
			HardwareVertexBufferSharedPtr vertexBuffer =
			  HardwareBufferManager::getSingleton().createVertexBuffer(
				 mesh->sharedVertexData->vertexDeclaration->getVertexSize(0),
				 mesh->sharedVertexData->vertexCount,
				 HardwareBuffer::HBU_STATIC_WRITE_ONLY);
			mesh->sharedVertexData->vertexBufferBinding->setBinding(0, vertexBuffer);

			// Vertex data
			static const float vertexData[] = {
			//   Position      Texture coordinates      // Index
			  0.0, 2.0,      -1.0,  1.0,  1.0,      //  0
			  0.0, 1.0,      -1.0, -1.0,  1.0,      //  1
			  1.0, 2.0,      -1.0,  1.0, -1.0,      //  2
			  1.0, 1.0,      -1.0, -1.0, -1.0,      //  3
			  2.0, 2.0,       1.0,  1.0, -1.0,      //  4
			  2.0, 1.0,       1.0, -1.0, -1.0,      //  5
			  3.0, 2.0,       1.0,  1.0,  1.0,      //  6
			  3.0, 1.0,       1.0, -1.0,  1.0,      //  7
			  4.0, 2.0,      -1.0,  1.0,  1.0,      //  8
			  4.0, 1.0,      -1.0, -1.0,  1.0,      //  9
			  1.0, 3.0,      -1.0,  1.0,  1.0,      // 10
			  2.0, 3.0,       1.0,  1.0,  1.0,      // 11
			  1.0, 0.0,      -1.0, -1.0,  1.0,      // 12
			  2.0, 0.0,       1.0, -1.0,  1.0,      // 13
			};

			// Fill vertex buffer
			float *pData = static_cast<float*>(vertexBuffer->lock(HardwareBuffer::HBL_DISCARD));
			for (size_t vertex = 0, i = 0; vertex < mesh->sharedVertexData->vertexCount; ++vertex)
			{
			  // Position
			  *pData++ = position.x + scale.x * vertexData[i++];
			  *pData++ = position.y + scale.y * vertexData[i++];
			  *pData++ = 0.0;

			  // Texture coordinates
			  *pData++ = vertexData[i++];
			  *pData++ = vertexData[i++];
			  *pData++ = vertexData[i++];
			}
			vertexBuffer->unlock();

			// Create index buffer
			sub->indexData->indexCount = 36;
			HardwareIndexBufferSharedPtr indexBuffer =
			  HardwareBufferManager::getSingleton().createIndexBuffer(
				 HardwareIndexBuffer::IT_16BIT,
				 sub->indexData->indexCount,
				 HardwareBuffer::HBU_STATIC_WRITE_ONLY);
			sub->indexData->indexBuffer = indexBuffer;

			// Index data
			static const uint16 indexData[] = {
			//   Indices         // Face
			   0,  1,  2,      //  0
			   2,  1,  3,      //  1
			   2,  3,  4,      //  2
			   4,  3,  5,      //  3
			   4,  5,  6,      //  4
			   6,  5,  7,      //  5
			   6,  7,  8,      //  6
			   8,  7,  9,      //  7
			  10,  2, 11,      //  8
			  11,  2,  4,      //  9
			   3, 12,  5,      // 10
			   5, 12, 13,      // 11
			};

			// Fill index buffer
			indexBuffer->writeData(0, indexBuffer->getSizeInBytes(), indexData, true);
			indexBuffer->unlock();

			mesh->_setBounds(AxisAlignedBox::BOX_INFINITE);
			mesh->_setBoundingSphereRadius(10);
			mesh->load();

			Entity *e = mSceneMgr->createEntity(mesh->getName());
			e->setCastShadows(false);
			e->setRenderQueueGroup(RENDER_QUEUE_OVERLAY-1);
			e->setVisible(true);

			e->setMaterialName("tracks/EnvMapDebug");
			debug_sn = new SceneNode(mSceneMgr);
			debug_sn->attachObject(e);
			debug_sn->setPosition(Vector3(0, 0, -5));
			debug_sn->setFixedYawAxis(true, Vector3::UNIT_Y);
			debug_sn->setVisible(true);
			debug_sn->_update(true, true);
			debug_sn->_updateBounds();
			overlay->add3D(debug_sn);
			overlay->show();
			
			// example update
			forceUpdate(Vector3::ZERO);
		}
	}
}

void Envmap::update(Vector3 center, Beam *beam)
{
	if (!isDynamic)
	{
		if (inited) return;
		else
		{
			//capture all images at once
			for (int i=0; i<6; i++)
			{
				camera[i]->setPosition(center);
				rt[i]->update();
			}
			inited=true;
		}
	}
	for (int i=0; i<6; i++) camera[i]->setPosition(center);
	
	// try to hide all flexbodies and cabs prior render, and then show them again after done
	// but only if they are visible ...
	bool togglemeshes = (beam)?(beam->meshesVisible):false;
	if(beam && togglemeshes)
		beam->setMeshVisibility(false);

	// caelum needs to know that we changed the cameras
#ifdef USE_CAELUM
	if(SkyManager::getSingletonPtr())
		SkyManager::getSingleton().notifyCameraChanged(camera[round]);
#endif // USE_CAELUM
	rt[round]->update();

	if(beam && togglemeshes)
		beam->setMeshVisibility(true);
	round++;
	if (round >= 6)
		round = 0;
}

void Envmap::forceUpdate(Vector3 center)
{
	if (isDynamic) return;
	//capture all images at once
	for (int i=0; i<6; i++)
	{
		camera[i]->setPosition(center);
		rt[i]->update();
	}
	inited=true;
}

void Envmap::prepareShutdown()
{
}
