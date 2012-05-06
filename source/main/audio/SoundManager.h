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
#ifndef __SoundManager_H_
#define __SoundManager_H_

#include "RoRPrerequisites.h"

#include "Ogre.h"
#include <AL/al.h>
#include <AL/alc.h>

class SoundManager
{
	friend class Sound;

public:
	static SoundManager* mSoundManager;

    SoundManager();
	~SoundManager();

	Sound* createSound(Ogre::String filename);

	void setCamera(Ogre::Vector3 position, Ogre::Vector3 direction, Ogre::Vector3 up, Ogre::Vector3 velocity);
	void pauseAllSounds();
	void resumeAllSounds();
	void setMasterVolume(float v);

	int getNumHardwareSources() { return m_hardware_sources_num; }

	static const float MAX_DISTANCE;
	static const float ROLLOFF_FACTOR;
	static const float REFERENCE_DISTANCE;
	static const unsigned int MAX_HARDWARE_SOURCES = 32;
	static const unsigned int MAX_AUDIO_BUFFERS = 8192;

private:
	void recomputeAllSources();
	void recomputeSource(int source_index, int reason, float vfl, Ogre::Vector3 *vvec);
	ALuint getHardwareSource(int hardware_index) { return m_hardware_sources[hardware_index]; };

	void assign(int source_index, int hardware_index);
	void retire(int source_index);

	bool loadWAVFile(Ogre::String filename, ALuint buffer);

	// active audio sources (hardware sources)
	int    m_hardware_sources_num;                       // total number of available hardware sources < MAX_HARDWARE_SOURCES
	int    m_hardware_sources_in_use_count;
	int    m_hardware_sources_map[MAX_HARDWARE_SOURCES]; // stores the hardware index for each source. -1 = unmapped
	ALuint m_hardware_sources[MAX_HARDWARE_SOURCES];     // this buffer contains valid AL handles up to m_hardware_sources_num

	// audio sources
	int    m_audio_sources_in_use_count;
	Sound* m_audio_sources[MAX_AUDIO_BUFFERS];
	// helper for calculating the most audible sources
	std::pair<int, float> m_audio_sources_most_audible[MAX_AUDIO_BUFFERS];
	
	// audio buffers: Array of AL buffers and filenames
	int          m_audio_buffers_in_use_count;
	ALuint       m_audio_buffers[MAX_AUDIO_BUFFERS];
	Ogre::String m_audio_buffer_file_name[MAX_AUDIO_BUFFERS];

	Ogre::Vector3 camera_position;
	ALCdevice*    m_sound_device;
	ALCcontext*   m_sound_context;

	float m_master_volume;
};

#endif // __SoundManager_H_
#endif // USE_OPENAL
