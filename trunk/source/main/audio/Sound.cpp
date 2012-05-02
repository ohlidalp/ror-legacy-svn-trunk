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

Sound::Sound(ALuint _buffer, SoundManager *soundManager, int sourceIndex) :
	  buffer(_buffer)
	, soundManager(soundManager)
	, sourceIndex(sourceIndex)
	, audibility(0.0f)
	, enabled(true)
	, gain(0.0f)
	, hardware_index(-1)
	, loop(false)
	, pitch(1.0f)
	, position(Vector3::ZERO)
	, should_play(false)
	, velocity(Vector3::ZERO)
{
}

void Sound::computeAudibility(Vector3 pos)
{
	// Disable sound?
	if (!enabled)
	{
		audibility = 0.0f;
		return;
	}

	// First check if the sound is finished!
	if (!loop && should_play && hardware_index!=-1)
	{
		int value = 0;
		alGetSourcei((ALuint)soundManager->getHardwareSource(hardware_index), AL_SOURCE_STATE, &value);
		if (value != AL_PLAYING)
		{
			should_play = false;
		}
	}
	
	// Should it play at all?
	if (!should_play || gain == 0.0f)
	{
		audibility = 0.0f;
		return;
	}

	float distance = (pos - position).length();
	
	if (distance > soundManager->MAX_DISTANCE)
	{
		audibility = 0.0f;
	} else if (distance < soundManager->REFERENCE_DISTANCE)
	{
		audibility = gain;
	} else
	{
		audibility = gain * (soundManager->REFERENCE_DISTANCE / (soundManager->REFERENCE_DISTANCE + (soundManager->ROLLOFF_FACTOR * (distance - soundManager->REFERENCE_DISTANCE))));
	}
}

bool Sound::isPlaying()
{
	if (hardware_index != -1)
	{
		int value = 0;
		alGetSourcei((ALuint)soundManager->getHardwareSource(hardware_index), AL_SOURCE_STATE, &value);
		return (value == AL_PLAYING);
	}
	return false;
}

void Sound::setEnabled(bool e)
{
	enabled = e;
	soundManager->recomputeSource(sourceIndex, REASON_PLAY, 0, NULL);
}

bool Sound::getEnabled()
{
	return enabled;
}

void Sound::play()
{
	should_play = true;
	soundManager->recomputeSource(sourceIndex, REASON_PLAY, 0, NULL);
}

void Sound::stop()
{
	should_play = false;
	soundManager->recomputeSource(sourceIndex, REASON_STOP, 0, NULL);
}

void Sound::setGain(float gain)
{
	this->gain = gain;
	soundManager->recomputeSource(sourceIndex, REASON_GAIN, gain, NULL);
}

void Sound::setLoop(bool loop)
{
	this->loop = loop;
	soundManager->recomputeSource(sourceIndex, REASON_LOOP, (loop) ? 1.0f : 0.0f, NULL);
}

void Sound::setPitch(float pitch)
{
	this->pitch = pitch;
	soundManager->recomputeSource(sourceIndex, REASON_PTCH, pitch, NULL);
}

void Sound::setPosition(Ogre::Vector3 pos)
{
	this->position = pos;
	soundManager->recomputeSource(sourceIndex, REASON_POSN, 0, &pos);
}

void Sound::setVelocity(Ogre::Vector3 vel)
{
	this->velocity = vel;
	soundManager->recomputeSource(sourceIndex, REASON_VLCT, 0, &vel);
}

#endif // USE_OPENAL
