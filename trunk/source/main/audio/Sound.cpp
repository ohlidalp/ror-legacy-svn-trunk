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

#ifdef USE_OPENAL

#include "Sound.h"
#include "SoundManager.h"

using namespace Ogre;

Sound::Sound(ALuint _buffer, SoundManager *_sound_mgr, int _source_index) : 
	  buffer(_buffer)
	, sound_mgr(_sound_mgr)
	, source_index(_source_index)
	, hardware_index(-1)
	, gain(1)
	, pitch(1)
	, loop(false)
	, enabled(true)
	, should_play(false)
	, position(Vector3::ZERO)
	, velocity(Vector3::ZERO)
{
}

void Sound::computeAudibility(Vector3 pos)
{
	// Disable sound?
	if (!enabled)
	{
		audibility = 0;
		return;
	}

	// First check if the sound is finished!
	if (!loop && should_play && hardware_index!=-1)
	{
		int value;
		alGetSourcei((ALuint)sound_mgr->getHardwareSource(hardware_index), AL_SOURCE_STATE, &value);
		if (value != AL_PLAYING)
			should_play = false;
	}
	
	// should it play at all?
	if (!should_play || gain == 0.0)
	{
		audibility = 0.0;
		return;
	}

	float distance = (pos - position).length();
	
	if (distance > sound_mgr->MAX_DISTANCE)
	{
		audibility = 0.0;
	} else if (distance < sound_mgr->REFERENCE_DISTANCE)
	{
		audibility = gain;
	} else
	{
		audibility = gain * (sound_mgr->REFERENCE_DISTANCE / (sound_mgr->REFERENCE_DISTANCE + (sound_mgr->ROLLOFF_FACTOR * (distance - sound_mgr->REFERENCE_DISTANCE))));
	}
}

bool Sound::isPlaying()
{
	if (hardware_index != -1)
	{
		int value;
		alGetSourcei((ALuint)sound_mgr->getHardwareSource(hardware_index), AL_SOURCE_STATE, &value);
		return (value==AL_PLAYING);
	}
	return false;
}

void Sound::setEnabled(bool e)
{
	enabled = e;
	sound_mgr->recomputeSource(source_index, REASON_PLAY, 0, NULL);
}

bool Sound::getEnabled()
{
	return enabled;
}

void Sound::play()
{
	should_play=true;
	sound_mgr->recomputeSource(source_index, REASON_PLAY, 0, NULL);
}

void Sound::stop()
{
	should_play=false;
	sound_mgr->recomputeSource(source_index, REASON_STOP, 0, NULL);
}

void Sound::setGain(float gain)
{
	this->gain=gain;
	sound_mgr->recomputeSource(source_index, REASON_GAIN, gain, NULL);
}

void Sound::setLoop(bool loop)
{
	this->loop=loop;
	sound_mgr->recomputeSource(source_index, REASON_LOOP, (loop)?1.0:0.0, NULL);
}

void Sound::setPitch(float pitch)
{
	this->pitch=pitch;
	sound_mgr->recomputeSource(source_index, REASON_PTCH, pitch, NULL);
}

void Sound::setPosition(Ogre::Vector3 pos)
{
	this->position=pos;
	sound_mgr->recomputeSource(source_index, REASON_POSN, 0, &pos);
}

void Sound::setVelocity(Ogre::Vector3 vel)
{
	this->velocity=vel;
	sound_mgr->recomputeSource(source_index, REASON_VLCT, 0, &vel);
}

#endif // USE_OPENAL
