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
#include "Settings.h"

// some gcc fixes
#if OGRE_PLATFORM == OGRE_PLATFORM_LINUX
#pragma GCC diagnostic ignored "-Wfloat-equal"
#endif // OGRE_PLATFORM_LINUX

using namespace Ogre;

const float SoundManager::MAX_DISTANCE       = 500.0f;
const float SoundManager::ROLLOFF_FACTOR     = 1.0f;
const float SoundManager::REFERENCE_DISTANCE = 7.5f;

SoundManager::SoundManager() :
	  m_sound_device(NULL)
	, m_sound_context(NULL)
	, m_audio_sources_in_use_count(0)
	, m_audio_buffers_in_use_count(0)
	, m_hardware_sources_num(0)
	, m_hardware_sources_in_use_count(0)
	, master_volume(1.0f)
{
	if (SSETTING("3D Sound renderer") == "No sound") return;

    char str[256];
	if (SSETTING("3D Sound renderer") == "Default") str[0]=0;
	else sprintf(str, "DirectSound Software on %s", SSETTING("3D Sound renderer").c_str());

	LOG("Opening Device: '"+String(str)+"'");

	master_volume = FSETTING("Sound Volume") / 100.0f;

	for (int i=0; i<MAX_HARDWARE_SOURCES; i++) m_hardware_sources_map[i]=-1;

	// We loop alcOpenDevice() because there is a potential race condition with the asynchronous DSound enumeration callback
	for (int i=0; i<100; i++)
	{
		m_sound_device=alcOpenDevice(str);
		if (m_sound_device)
			break;
		else
		{
			ALint error=alGetError();
			if (error != ALC_INVALID_VALUE) {LOG("Could not create OpenAL device, error code: "+TOSTRING(error));return;}
		}
	}
	if (!m_sound_device)
	{
		// Last ditch attempt with the default sound device, in case the user has a messed-up config file
		m_sound_device=alcOpenDevice("");
		if (!m_sound_device) {LOG("Could not create OpenAL device after many tries.");return;}
		else LOG("Warning: invalid sound device configuration, I will use the default sound source. Run configurator!");
	}
	m_sound_context=alcCreateContext(m_sound_device, NULL);
	if (!m_sound_context)
	{
		ALint error;
		error=alGetError();
		LOG("Could not create OpenAL context, error code: "+TOSTRING(error));
		alcCloseDevice(m_sound_device);
		m_sound_device=NULL;
		return;
	}
	alcMakeContextCurrent(m_sound_context);
	// Generate the AL sources
	for (m_hardware_sources_num=0; m_hardware_sources_num<MAX_HARDWARE_SOURCES; m_hardware_sources_num++)
	{
		alGetError();
		alGenSources(1, &m_hardware_sources[m_hardware_sources_num]);
		if (alGetError() != AL_NO_ERROR)
			break;
		alSourcef(m_hardware_sources[m_hardware_sources_num], AL_REFERENCE_DISTANCE, REFERENCE_DISTANCE);
		alSourcef(m_hardware_sources[m_hardware_sources_num], AL_ROLLOFF_FACTOR, ROLLOFF_FACTOR);
		alSourcef(m_hardware_sources[m_hardware_sources_num], AL_MAX_DISTANCE, MAX_DISTANCE);
	}
	alDopplerFactor(1.0);
	alDopplerVelocity(343.0f);
}

SoundManager::~SoundManager()
{
	// Delete the sources and buffers
	alDeleteSources(MAX_HARDWARE_SOURCES, m_hardware_sources);
    alDeleteBuffers(MAX_AUDIO_BUFFERS, m_audio_buffers);

	// Destroy the sound context and device
    m_sound_context = alcGetCurrentContext();
    m_sound_device = alcGetContextsDevice(m_sound_context);
    alcMakeContextCurrent(NULL);
    alcDestroyContext(m_sound_context);
    if (m_sound_device)
        alcCloseDevice(m_sound_device);

	LOG("SoundManager destroyed.");
}

void SoundManager::setCamera(Ogre::Vector3 position, Ogre::Vector3 direction, Ogre::Vector3 up, Ogre::Vector3 velocity)
{
	if (!m_sound_device) return;
	camera_position = position;
	recomputeAllSources();
	
	float orientation[6];
	//dir
	orientation[0] = direction.x;
	orientation[1] = direction.y;
	orientation[2] = direction.z;
	//up
	orientation[3] = up.x;
	orientation[4] = up.y;
	orientation[5] = up.z;

	alListener3f(AL_POSITION, position.x, position.y, position.z);
	alListener3f(AL_VELOCITY, velocity.x, velocity.y, velocity.z);
	alListenerfv(AL_ORIENTATION, orientation);
}

bool compareByAudibility(std::pair<int, float> a, std::pair<int, float> b)
{		
	return a.second > b.second;
}

// Called when the camera moves
void SoundManager::recomputeAllSources()
{
	
	if (!m_sound_device) return;
	// Mutex is supposed to be already taken
	for (int i=0; i<m_audio_sources_in_use_count; i++)
	{
		m_audio_sources[i]->computeAudibility(camera_position);
		m_audio_sources_most_audible[i].first = i;
		m_audio_sources_most_audible[i].second = m_audio_sources[i]->audibility;
	}
	// Sort first 'num_hardware_sources' sources by audibility
	// See: https://en.wikipedia.org/wiki/Selection_algorithm
	if ((m_audio_sources_in_use_count-1) > m_hardware_sources_num)
		std::nth_element(m_audio_sources_most_audible, m_audio_sources_most_audible+m_hardware_sources_num, m_audio_sources_most_audible+m_audio_sources_in_use_count-1, compareByAudibility);
	// Retire out of range sources first
	for (int i=0; i<m_audio_sources_in_use_count; i++)
	{
		if (m_audio_sources[m_audio_sources_most_audible[i].first]->hardware_index != -1 && (i >= m_hardware_sources_num || m_audio_sources_most_audible[i].second == 0))
			retire(m_audio_sources_most_audible[i].first);
	}
	// Assign new sources
	for (int i=0; i<std::min(m_audio_sources_in_use_count, m_hardware_sources_num); i++)
	{
		if (m_audio_sources[m_audio_sources_most_audible[i].first]->hardware_index == -1 && m_audio_sources_most_audible[i].second > 0)
		{
			for (int j=0; j<m_hardware_sources_num; j++)
			{
				if (m_hardware_sources_map[j] == -1)
				{
					assign(m_audio_sources_most_audible[i].first, j);
					break;
				}
			}
		}
	}
}

void SoundManager::recomputeSource(int source_index, int reason, float vfl, Vector3 *vvec)
{
	if (!m_sound_device) return;

	m_audio_sources[source_index]->computeAudibility(camera_position);

	if (m_audio_sources[source_index]->audibility == 0.0)
	{
		if (m_audio_sources[source_index]->hardware_index != -1) // Retire the source if it is currently assigned
			retire(source_index);
	}
	else
	{
		// This is a potentially audible m_audio_sources[source_index]
		if (m_audio_sources[source_index]->hardware_index != -1)
		{
			// m_audio_sources[source_index] already playing
			// Update the AL settings
			switch (reason)
			{
				case Sound::REASON_PLAY: alSourcePlay(m_hardware_sources[m_audio_sources[source_index]->hardware_index]);break;
				case Sound::REASON_STOP: alSourceStop(m_hardware_sources[m_audio_sources[source_index]->hardware_index]);break;
				case Sound::REASON_GAIN: alSourcef(m_hardware_sources[m_audio_sources[source_index]->hardware_index], AL_GAIN, vfl*master_volume);break;
				case Sound::REASON_LOOP: alSourcei(m_hardware_sources[m_audio_sources[source_index]->hardware_index], AL_LOOPING, (vfl>0.5)?AL_TRUE:AL_FALSE);break;
				case Sound::REASON_PTCH: alSourcef(m_hardware_sources[m_audio_sources[source_index]->hardware_index], AL_PITCH, vfl);break;
				case Sound::REASON_POSN: alSource3f(m_hardware_sources[m_audio_sources[source_index]->hardware_index], AL_POSITION, vvec->x,vvec->y,vvec->z);break;
				case Sound::REASON_VLCT: alSource3f(m_hardware_sources[m_audio_sources[source_index]->hardware_index], AL_VELOCITY, vvec->x,vvec->y,vvec->z);break;
				default: break;
			}
		}
		else
		{
			// Try to make it play by the hardware
			// Check if there is one free m_audio_sources[source_index] in the pool
			if (m_hardware_sources_in_use_count < m_hardware_sources_num)
			{
				for (int i=0; i<m_hardware_sources_num; i++)
					if (m_hardware_sources_map[i] == -1) {assign(source_index, i);break;}
			}
			else
			{
				// Now, compute who is the faintest
				// Note: we know the table m_hardware_sources_map is full!
				float fv=1.0;
				int al_faintest=0;
				for (int i=0; i<m_hardware_sources_num; i++)
				{
					if (m_hardware_sources_map[i] >= 0 && m_audio_sources[m_hardware_sources_map[i]]->audibility < fv)
					{
						fv=m_audio_sources[m_hardware_sources_map[i]]->audibility;
						al_faintest=i;
					}
					}
				// Find if the the faintest m_audio_sources[source_index] is louder or not
				if (fv < m_audio_sources[source_index]->audibility)
				{
					// This new m_audio_sources[source_index] is louder than the faintest!
					retire(m_hardware_sources_map[al_faintest]);
					assign(source_index, al_faintest);
				}
				// Else this m_audio_sources[source_index] is too faint, we don't play it!
			}
		}
	}
}
	
void SoundManager::assign(int source_index, int hardware_index)
{
	if (!m_sound_device) return;

	m_audio_sources[source_index]->hardware_index=hardware_index;
	m_hardware_sources_map[hardware_index]=source_index;

	// The hardware source is supposed to be stopped!
	alSourcei(m_hardware_sources[hardware_index], AL_BUFFER, m_audio_sources[source_index]->buffer);
	alSourcef(m_hardware_sources[hardware_index], AL_GAIN, m_audio_sources[source_index]->gain*master_volume);
	alSourcei(m_hardware_sources[hardware_index], AL_LOOPING, (m_audio_sources[source_index]->loop)?AL_TRUE:AL_FALSE);
	alSourcef(m_hardware_sources[hardware_index], AL_PITCH, m_audio_sources[source_index]->pitch);
	alSource3f(m_hardware_sources[hardware_index], AL_POSITION, m_audio_sources[source_index]->position.x,m_audio_sources[source_index]->position.y,m_audio_sources[source_index]->position.z);
	alSource3f(m_hardware_sources[hardware_index], AL_VELOCITY, m_audio_sources[source_index]->velocity.x,m_audio_sources[source_index]->velocity.y,m_audio_sources[source_index]->velocity.z);
	if (m_audio_sources[source_index]->should_play)
		alSourcePlay(m_hardware_sources[hardware_index]);
	
	m_hardware_sources_in_use_count++;
}

void SoundManager::retire(int source_index)
{
	if (!m_sound_device) return;

	if (m_audio_sources[source_index]->hardware_index == -1) return;
	alSourceStop(m_hardware_sources[m_audio_sources[source_index]->hardware_index]);
	m_hardware_sources_map[m_audio_sources[source_index]->hardware_index]=-1;
	m_audio_sources[source_index]->hardware_index=-1;
	m_hardware_sources_in_use_count--;
}

void SoundManager::pauseAllSounds()
{
	if (!m_sound_device) return;
	alListenerf(AL_GAIN, 0.0);
}

void SoundManager::resumeAllSounds()
{
	if (!m_sound_device) return;
	alListenerf(AL_GAIN, master_volume);
}

void SoundManager::setMasterVolume(float v)
{
	if (!m_sound_device) return;
	master_volume = v;
	alListenerf(AL_GAIN, master_volume);
}

Sound* SoundManager::createSound(String filename)
{
	if (!m_sound_device) return NULL;
	ALuint buffer=0;

	// Is the file already loaded?
	for (int i=0; i<m_audio_buffers_in_use_count; i++)
	{
		if (filename == m_audio_buffer_file_name[i])
		{
			buffer=m_audio_buffers[i];
			break;
		}
	}
	if (!buffer)
	{
		// Load the file
		if (m_audio_buffers_in_use_count >= MAX_AUDIO_BUFFERS)
			return NULL;

		alGenBuffers(1, &m_audio_buffers[m_audio_buffers_in_use_count]);
		if (loadWAVFile(filename, m_audio_buffers[m_audio_buffers_in_use_count]))
		{
			// There was an error!
			alDeleteBuffers(1, &m_audio_buffers[m_audio_buffers_in_use_count]);
			m_audio_buffer_file_name[m_audio_buffers_in_use_count]=String("");
			return NULL;
		}
		buffer=m_audio_buffers[m_audio_buffers_in_use_count];
		m_audio_buffer_file_name[m_audio_buffers_in_use_count]=filename;
	}
	if (m_audio_buffers_in_use_count >= MAX_AUDIO_SOURCES) return NULL;

	m_audio_sources[m_audio_buffers_in_use_count]=new Sound(buffer, this, m_audio_buffers_in_use_count);

	return m_audio_sources[m_audio_buffers_in_use_count++];
}

bool SoundManager::loadWAVFile(String filename, ALuint buffer)
{
	if (!m_sound_device) return true;
	LOG("Loading WAV file "+filename);
	
	// Create the Stream
	ResourceGroupManager *rgm=ResourceGroupManager::getSingletonPtr();
	String group=rgm->findGroupContainingResource(filename);
	DataStreamPtr stream=rgm->openResource(filename, group);

	// Load RIFF/WAVE
	char magic[5];
	magic[4]=0;
	unsigned int   lbuf; // uint32_t
	unsigned short sbuf; // uint16_t

	// Check magic
	if (stream->read(magic, 4) != 4) {LOG("Could not read file "+filename);return true;}
	if (String(magic) != String("RIFF")) {LOG("Invalid WAV file (no RIFF): "+filename);return true;}
	// Skip 4 bytes (magic)
	stream->skip(4);
	// Check file format
	if (stream->read(magic, 4) != 4) {LOG("Could not read file "+filename);return true;}
	if (String(magic) != String("WAVE")) {LOG("Invalid WAV file (no WAVE): "+filename);return true;}
	// Check 'fmt ' sub chunk (1)
	if (stream->read(magic, 4) != 4) {LOG("Could not read file "+filename);return true;}
	if (String(magic) != String("fmt ")) {LOG("Invalid WAV file (no fmt): "+filename);return true;}
	// Read (1)'s size
	if (stream->read(&lbuf, 4) != 4) {LOG("Could not read file "+filename);return true;}
	unsigned long subChunk1Size=lbuf;
	if (subChunk1Size<16) {LOG("Invalid WAV file (invalid subChunk1Size): "+filename);return true;}
	// Check PCM audio format
	if (stream->read(&sbuf, 2) != 2) {LOG("Could not read file "+filename);return true;}
	unsigned short audioFormat=sbuf;
	if (audioFormat != 1) {LOG("Invalid WAV file (invalid audioformat "+TOSTRING(audioFormat)+"): "+filename);return true;}
	// Read number of channels
	if (stream->read(&sbuf, 2) != 2) {LOG("Could not read file "+filename);return true;}
	unsigned short channels=sbuf;
	// Read frequency (sample rate)
	if (stream->read(&lbuf, 4) != 4) {LOG("Could not read file "+filename);return true;}
	unsigned long freq=lbuf;
	// Skip 6 bytes (Byte rate (4), Block align (2))
	stream->skip(6);
	// Read bits per sample
	if (stream->read(&sbuf, 2) != 2) {LOG("Could not read file "+filename);return true;}
	unsigned short bps=sbuf;
	// Check 'data' sub chunk (2)
	if (stream->read(magic, 4) != 4) {LOG("Could not read file "+filename);return true;}
	if (String(magic) != String("data") && String(magic) != String("fact")) {LOG("Invalid WAV file (no data/fact): "+filename);return true;}
	// Fact is an option section we don't need to worry about
	if (String(magic)==String("fact"))
	{
		stream->skip(8);
		// Now we shoudl hit the data chunk
		if (stream->read(magic, 4) != 4) {LOG("Could not read file "+filename);return true;}
		if (String(magic) != String("data")) {LOG("Invalid WAV file (no data): "+filename);return true;}
	}
	// The next four bytes are the remaining size of the file
	if (stream->read(&lbuf, 4) != 4) {LOG("Could not read file "+filename);return true;}
	
	unsigned long dataSize=lbuf;
	int format=0;

	if (channels == 1 && bps == 8)
		format=AL_FORMAT_MONO8;
	else if (channels == 1 && bps == 16)
		format=AL_FORMAT_MONO16;
	else if (channels == 2 && bps == 8)
		format=AL_FORMAT_STEREO16;
	else if (channels == 2 && bps == 16)
		format=AL_FORMAT_STEREO16;
	else
	{
		LOG("Invalid WAV file (wrong channels/bps): "+filename);
		return true;
	}

	if (channels != 1) LOG("Invalid WAV file: the file needs to be mono, and nothing else. Will try to continue anyways ...");

	//okay, creating buffer
	void* bdata=malloc(dataSize);
	if (!bdata) {LOG("Memory error reading file "+filename);return true;}
	if (stream->read(bdata, dataSize) != dataSize) {LOG("Could not read file "+filename);return true;}

	//LOG("alBufferData: format "+TOSTRING(format)+" size "+TOSTRING(dataSize)+" freq "+TOSTRING(freq));
	alGetError(); // Reset errors
	ALint error;
	alBufferData(buffer, format, bdata, dataSize, freq);
	error=alGetError();

	free(bdata);
	// Stream will be closed by itself

	if (error != AL_NO_ERROR) {LOG("OpenAL error while loading buffer for "+filename+" : "+TOSTRING(error));return true;}

	return false;
}

#endif // USE_OPENAL
