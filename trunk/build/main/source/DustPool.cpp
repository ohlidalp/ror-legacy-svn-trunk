/*
This source file is part of Rigs of Rods
Copyright 2005,2006,2007,2008,2009 Pierre-Michel Ricordel
Copyright 2007,2008,2009 Thomas Fischer

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
#include "DustPool.h"
#include "water.h"
#include "groundmodel.h"

#if OGRE_PLATFORM == OGRE_PLATFORM_LINUX
#pragma GCC diagnostic ignored "-Wfloat-equal"
#endif //OGRE_PLATFORM_LINUX

DustPool::DustPool(String dname, int dsize, float minvelo, float maxvelo, float fadeco, float timedelta, float velofac, float ttl, SceneNode *parent, SceneManager *smgr, Water *mw) : w(mw), smgr(smgr), dustAtomsVec(), parentNode(parent), dname(dname), size(dsize), allocated(0), minvelo(minvelo), maxvelo(maxvelo), fadeco(fadeco), timedelta(timedelta), velofac(velofac), ttl(ttl)
{
	//dustAtomsVec.resize(size);
	for(int i=0; i<size;i++)
	{
		// per class values
		String dename = "Dust " + dname + StringConverter::toString(i);
		dustatom_t da = dustatom_t();
		da.node = parent->createChildSceneNode();
		da.ps = smgr->createParticleSystem(dename, dname);
		da.node->attachObject(da.ps);
		da.ps->setCastShadows(false);
		da.ps->getEmitter(0)->setEnabled(false);
		dustAtomsVec.push_back(da);
	}
}

DustPool::~DustPool()
{
	for (int i=0; i<size; i++)
	{
		if(dustAtomsVec[i].ps)
		{
			smgr->destroyParticleSystem(dustAtomsVec[i].ps);
			dustAtomsVec[i].ps = 0;
		}
		if(dustAtomsVec[i].node)
		{
			dustAtomsVec[i].node->removeAndDestroyAllChildren();
			//dustAtomsVec[i] = 0;
		}
	}

}

void DustPool::setVisible(bool s)
{
	for (int i=0; i<size; i++)
	{
		dustAtomsVec[i].ps->setVisible(s);
	}
}

void DustPool::alloc(Vector3 pos, Vector3 vel, ColourValue col, float time)
{
	if(allocated == size) return; // discard then
	if(vel.length() < minvelo || vel.length() > maxvelo) return;

	dustAtomsVec[allocated].colour = col;
	dustAtomsVec[allocated].velocity = vel;
	dustAtomsVec[allocated].position = pos;
	if(time > 0)
		dustAtomsVec[allocated].rate = timedelta - time;
	allocated++;
}

void DustPool::update(float gspeed)
{
	gspeed=fabs(gspeed);
	// walk the queue
	for(int i=0; i<allocated; i++)
	{
		dustatom_t *da = &dustAtomsVec[i];

		ParticleEmitter *emit = da->ps->getEmitter(0);
		Vector3 ndir = da->velocity;
		ndir.y = 0;
		ndir = ndir * velofac;
		// XXX: whats this for?!
		//if (ndir.y<0) ndir.y=-ndir.y;
		Real vel = ndir.length();
		//ndir.y+=vel/5.5;
		if(vel == 0) vel += 0.0001; // fix division thorugh zero
		ndir = ndir / vel;
		emit->setEnabled(true);
		da->node->setPosition(da->position);
		emit->setDirection(ndir);
		emit->setParticleVelocity(vel);
//			emit->setColour(ColourValue(0.65, 0.55, 0.53,(vel+(gspeed/10.0))*0.05));
		ColourValue col = da->colour;
		if(fadeco >= 0)
			col.a = (((vel+(gspeed/10.0))*0.05) * fadeco);
		emit->setColour(col);
		// TODO: FIX below
		emit->setTimeToLive((vel+(gspeed/10.0)) * ttl);

	}
	for (int i=allocated; i<size; i++)
		dustAtomsVec[i].ps->getEmitter(0)->setEnabled(false);
	allocated=0;
}
