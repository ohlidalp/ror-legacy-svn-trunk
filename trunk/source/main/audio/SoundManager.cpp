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


bool _checkALErrors(const char *filename, int linenum)
{
	int err = alGetError();
	if(err != AL_NO_ERROR)
	{
		char buf[4096]="";
		sprintf(buf, "OpenAL Error: %s (0x%x), @ %s:%d\n", alGetString(err), err, filename, linenum);
		LOG(buf);
		return true;
	}
	return false;
}
#define hasALErrors() _checkALErrors(__FILE__, __LINE__)

using namespace Ogre;

const float SoundManager::MAX_DISTANCE       = 500.0f;
const float SoundManager::ROLLOFF_FACTOR     = 1.0f;
const float SoundManager::REFERENCE_DISTANCE = 7.5f;

SoundManager::SoundManager() :
	  m_audio_buffers_in_use_count(0)
	, m_audio_sources_in_use_count(0)
	, m_hardware_sources_in_use_count(0)
	, m_hardware_sources_num(0)
	, m_sound_context(NULL)
	, m_sound_device(NULL)
{
	String audio_device = SSETTING("AudioDevice", "Default");

	if (audio_device == "No Output") return;

	LOG("Opening Device: '" + audio_device + "'");
	
	const char *alDeviceString = audio_device.c_str();
	if(audio_device == "Default") alDeviceString = NULL;
	m_sound_device = alcOpenDevice(alDeviceString);
	if(!m_sound_device)
	{
		hasALErrors();
		return;
	}

	m_sound_context = alcCreateContext(m_sound_device, NULL);

	if (!m_sound_context)
	{
		ALint error= alGetError();
		LOG("SoundManager: Could not create OpenAL context, error code: " + TOSTRING(error));
		alcCloseDevice(m_sound_device);
		m_sound_device = NULL;
		return;
	}

	const char *tmp = alGetString(AL_VENDOR);
	if(tmp) LOG("OpenAL vendor is: " + String(tmp));

	tmp = alGetString(AL_VERSION);
	if(tmp) LOG("OpenAL version is: " + String(tmp));

	tmp = alGetString(AL_RENDERER);
	if(tmp) LOG("OpenAL renderer is: " + String(tmp));

	tmp = alGetString(AL_EXTENSIONS);
	if(tmp) LOG("OpenAL extensions are: " + String(tmp));

	tmp = alcGetString(m_sound_device, ALC_DEVICE_SPECIFIER);
	if(tmp) LOG("OpenAL device is: " + String(tmp));

	tmp = alcGetString(m_sound_device, ALC_EXTENSIONS);
	if(tmp) LOG("OpenAL ALC extensions are: " + String(tmp));


	alcMakeContextCurrent(m_sound_context);

	// generate the AL sources
	for (m_hardware_sources_num=0; m_hardware_sources_num < MAX_HARDWARE_SOURCES; m_hardware_sources_num++)
	{
		alGetError();
		alGenSources(1, &m_hardware_sources[m_hardware_sources_num]);
		if (alGetError() != AL_NO_ERROR) break;
		alSourcef(m_hardware_sources[m_hardware_sources_num], AL_REFERENCE_DISTANCE, REFERENCE_DISTANCE);
		alSourcef(m_hardware_sources[m_hardware_sources_num], AL_ROLLOFF_FACTOR, ROLLOFF_FACTOR);
		alSourcef(m_hardware_sources[m_hardware_sources_num], AL_MAX_DISTANCE, MAX_DISTANCE);
	}

	alDopplerFactor(1.0f);
	alDopplerVelocity(343.0f);

	for (int i=0; i < MAX_HARDWARE_SOURCES; i++)
	{
		m_hardware_sources_map[i] = -1;
	}

	m_master_volume = FSETTING("Sound Volume", 100.0f) / 100.0f;
}

SoundManager::~SoundManager()
{
	// delete the sources and buffers
	alDeleteSources(MAX_HARDWARE_SOURCES, m_hardware_sources);
    alDeleteBuffers(MAX_AUDIO_BUFFERS, m_audio_buffers);

	// destroy the sound context and device
    m_sound_context = alcGetCurrentContext();
    m_sound_device  = alcGetContextsDevice(m_sound_context);
    alcMakeContextCurrent(NULL);
    alcDestroyContext(m_sound_context);
    if (m_sound_device)
	{
        alcCloseDevice(m_sound_device);
	}
	LOG("SoundManager destroyed.");
}

void SoundManager::setCamera(Ogre::Vector3 position, Ogre::Vector3 direction, Ogre::Vector3 up, Ogre::Vector3 velocity)
{
	if (!m_sound_device) return;
	camera_position = position;
	recomputeAllSources();
	
	float orientation[6];
	// direction
	orientation[0] = direction.x;
	orientation[1] = direction.y;
	orientation[2] = direction.z;
	// up
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

// called when the camera moves
void SoundManager::recomputeAllSources()
{
	if (!m_sound_device) return;

	for (int i=0; i < m_audio_sources_in_use_count; i++)
	{
		m_audio_sources[i]->computeAudibility(camera_position);
		m_audio_sources_most_audible[i].first = i;
		m_audio_sources_most_audible[i].second = m_audio_sources[i]->audibility;
	}
	// sort first 'num_hardware_sources' sources by audibility
	// see: https://en.wikipedia.org/wiki/Selection_algorithm
	if ((m_audio_sources_in_use_count - 1) > m_hardware_sources_num)
	{
		std::nth_element(m_audio_sources_most_audible, m_audio_sources_most_audible+m_hardware_sources_num, m_audio_sources_most_audible+m_audio_sources_in_use_count-1, compareByAudibility);
	}
	// retire out of range sources first
	for (int i=0; i < m_audio_sources_in_use_count; i++)
	{
		if (m_audio_sources[m_audio_sources_most_audible[i].first]->hardware_index != -1 && (i >= m_hardware_sources_num || m_audio_sources_most_audible[i].second == 0))
			retire(m_audio_sources_most_audible[i].first);
	}
	// assign new sources
	for (int i=0; i < std::min(m_audio_sources_in_use_count, m_hardware_sources_num); i++)
	{
		if (m_audio_sources[m_audio_sources_most_audible[i].first]->hardware_index == -1 && m_audio_sources_most_audible[i].second > 0)
		{
			for (int j=0; j < m_hardware_sources_num; j++)
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

	if (m_audio_sources[source_index]->audibility == 0.0f)
	{
		if (m_audio_sources[source_index]->hardware_index != -1)
		{
			// retire the source if it is currently assigned
			retire(source_index);
		}
	} else
	{
		// this is a potentially audible m_audio_sources[source_index]
		if (m_audio_sources[source_index]->hardware_index != -1)
		{
			// m_audio_sources[source_index] already playing
			// update the AL settings
			switch (reason)
			{
				case Sound::REASON_PLAY: alSourcePlay(m_hardware_sources[m_audio_sources[source_index]->hardware_index]); break;
				case Sound::REASON_STOP: alSourceStop(m_hardware_sources[m_audio_sources[source_index]->hardware_index]); break;
				case Sound::REASON_GAIN: alSourcef(m_hardware_sources[m_audio_sources[source_index]->hardware_index], AL_GAIN, vfl * m_master_volume); break;
				case Sound::REASON_LOOP: alSourcei(m_hardware_sources[m_audio_sources[source_index]->hardware_index], AL_LOOPING, (vfl > 0.5) ? AL_TRUE : AL_FALSE); break;
				case Sound::REASON_PTCH: alSourcef(m_hardware_sources[m_audio_sources[source_index]->hardware_index], AL_PITCH, vfl); break;
				case Sound::REASON_POSN: alSource3f(m_hardware_sources[m_audio_sources[source_index]->hardware_index], AL_POSITION, vvec->x, vvec->y, vvec->z); break;
				case Sound::REASON_VLCT: alSource3f(m_hardware_sources[m_audio_sources[source_index]->hardware_index], AL_VELOCITY, vvec->x, vvec->y, vvec->z); break;
				default: break;
			}
		} else
		{
			// try to make it play by the hardware
			// check if there is one free m_audio_sources[source_index] in the pool
			if (m_hardware_sources_in_use_count < m_hardware_sources_num)
			{
				for (int i=0; i < m_hardware_sources_num; i++)
				{
					if (m_hardware_sources_map[i] == -1)
					{
						assign(source_index, i);
						break;
					}
				}
			} else
			{
				// now, compute who is the faintest
				// note: we know the table m_hardware_sources_map is full!
				float fv = 1.0f;
				int al_faintest = 0;
				for (int i=0; i < m_hardware_sources_num; i++)
				{
					if (m_hardware_sources_map[i] >= 0 && m_audio_sources[m_hardware_sources_map[i]]->audibility < fv)
					{
						fv = m_audio_sources[m_hardware_sources_map[i]]->audibility;
						al_faintest = i;
					}
				}
				// check to ensure that the sound is louder than the faintest sound currently playing
				if (fv < m_audio_sources[source_index]->audibility)
				{
					// this new m_audio_sources[source_index] is louder than the faintest!
					retire(m_hardware_sources_map[al_faintest]);
					assign(source_index, al_faintest);
				}
				// else this m_audio_sources[source_index] is too faint, we don't play it!
			}
		}
	}
}
	
void SoundManager::assign(int source_index, int hardware_index)
{
	if (!m_sound_device) return;
	m_audio_sources[source_index]->hardware_index = hardware_index;
	m_hardware_sources_map[hardware_index] = source_index;

	// the hardware source is supposed to be stopped!
	alSourcei(m_hardware_sources[hardware_index], AL_BUFFER, m_audio_sources[source_index]->buffer);
	alSourcef(m_hardware_sources[hardware_index], AL_GAIN, m_audio_sources[source_index]->gain*m_master_volume);
	alSourcei(m_hardware_sources[hardware_index], AL_LOOPING, (m_audio_sources[source_index]->loop)?AL_TRUE:AL_FALSE);
	alSourcef(m_hardware_sources[hardware_index], AL_PITCH, m_audio_sources[source_index]->pitch);
	alSource3f(m_hardware_sources[hardware_index], AL_POSITION, m_audio_sources[source_index]->position.x,m_audio_sources[source_index]->position.y,m_audio_sources[source_index]->position.z);
	alSource3f(m_hardware_sources[hardware_index], AL_VELOCITY, m_audio_sources[source_index]->velocity.x,m_audio_sources[source_index]->velocity.y,m_audio_sources[source_index]->velocity.z);
	if (m_audio_sources[source_index]->should_play)
	{
		alSourcePlay(m_hardware_sources[hardware_index]);
	}
	m_hardware_sources_in_use_count++;
}

void SoundManager::retire(int source_index)
{
	if (!m_sound_device) return;
	if (m_audio_sources[source_index]->hardware_index == -1) return;
	alSourceStop(m_hardware_sources[m_audio_sources[source_index]->hardware_index]);
	m_hardware_sources_map[m_audio_sources[source_index]->hardware_index] = -1;
	m_audio_sources[source_index]->hardware_index = -1;
	m_hardware_sources_in_use_count--;
}

void SoundManager::pauseAllSounds()
{
	if (!m_sound_device) return;
	// no mutex needed
	alListenerf(AL_GAIN, 0.0f);
}

void SoundManager::resumeAllSounds()
{
	if (!m_sound_device) return;
	// no mutex needed
	alListenerf(AL_GAIN, m_master_volume);
}

void SoundManager::setMasterVolume(float v)
{
	if (!m_sound_device) return;
	// no mutex needed
	m_master_volume = v;
	alListenerf(AL_GAIN, m_master_volume);
}

Sound* SoundManager::createSound(String filename)
{
	if (!m_sound_device) return NULL;

	if (m_audio_buffers_in_use_count >= MAX_AUDIO_BUFFERS)
	{
		LOG("SoundManager: Reached MAX_AUDIO_BUFFERS limit (" + TOSTRING(MAX_AUDIO_BUFFERS) + ")");
		return NULL;
	}

	ALuint buffer = 0;

	// is the file already loaded?
	for (int i=0; i < m_audio_buffers_in_use_count; i++)
	{
		if (filename == m_audio_buffer_file_name[i])
		{
			buffer = m_audio_buffers[i];
			break;
		}
	}

	if (!buffer)
	{
		// load the file
		alGenBuffers(1, &m_audio_buffers[m_audio_buffers_in_use_count]);
		if (loadWAVFile(filename, m_audio_buffers[m_audio_buffers_in_use_count]))
		{
			// there was an error!
			alDeleteBuffers(1, &m_audio_buffers[m_audio_buffers_in_use_count]);
			m_audio_buffer_file_name[m_audio_buffers_in_use_count] = "";
			return NULL;
		}
		buffer = m_audio_buffers[m_audio_buffers_in_use_count];
		m_audio_buffer_file_name[m_audio_buffers_in_use_count] = filename;
	}

	m_audio_sources[m_audio_buffers_in_use_count] = new Sound(buffer, this, m_audio_buffers_in_use_count);

	return m_audio_sources[m_audio_buffers_in_use_count++];
}

bool SoundManager::loadWAVFile(String filename, ALuint buffer)
{
	if (!m_sound_device) return true;
	LOG("Loading WAV file "+filename);
	
	// create the Stream
	ResourceGroupManager *rgm=ResourceGroupManager::getSingletonPtr();
	String group=rgm->findGroupContainingResource(filename);
	DataStreamPtr stream=rgm->openResource(filename, group);

	// load RIFF/WAVE
	char magic[5];
	magic[4]=0;
	unsigned int   lbuf; // uint32_t
	unsigned short sbuf; // uint16_t

	// check magic
	if (stream->read(magic, 4) != 4) {LOG("Could not read file "+filename);return true;}
	if (String(magic) != String("RIFF")) {LOG("Invalid WAV file (no RIFF): "+filename);return true;}
	// skip 4 bytes (magic)
	stream->skip(4);
	// check file format
	if (stream->read(magic, 4) != 4) {LOG("Could not read file "+filename);return true;}
	if (String(magic) != String("WAVE")) {LOG("Invalid WAV file (no WAVE): "+filename);return true;}
	// check 'fmt ' sub chunk (1)
	if (stream->read(magic, 4) != 4) {LOG("Could not read file "+filename);return true;}
	if (String(magic) != String("fmt ")) {LOG("Invalid WAV file (no fmt): "+filename);return true;}
	// read (1)'s size
	if (stream->read(&lbuf, 4) != 4) {LOG("Could not read file "+filename);return true;}
	unsigned long subChunk1Size=lbuf;
	if (subChunk1Size<16) {LOG("Invalid WAV file (invalid subChunk1Size): "+filename);return true;}
	// check PCM audio format
	if (stream->read(&sbuf, 2) != 2) {LOG("Could not read file "+filename);return true;}
	unsigned short audioFormat=sbuf;
	if (audioFormat != 1) {LOG("Invalid WAV file (invalid audioformat "+TOSTRING(audioFormat)+"): "+filename);return true;}
	// read number of channels
	if (stream->read(&sbuf, 2) != 2) {LOG("Could not read file "+filename);return true;}
	unsigned short channels=sbuf;
	// read frequency (sample rate)
	if (stream->read(&lbuf, 4) != 4) {LOG("Could not read file "+filename);return true;}
	unsigned long freq=lbuf;
	// skip 6 bytes (Byte rate (4), Block align (2))
	stream->skip(6);
	// read bits per sample
	if (stream->read(&sbuf, 2) != 2) {LOG("Could not read file "+filename);return true;}
	unsigned short bps=sbuf;
	// check 'data' sub chunk (2)
	if (stream->read(magic, 4) != 4) {LOG("Could not read file "+filename);return true;}
	if (String(magic) != String("data") && String(magic) != String("fact")) {LOG("Invalid WAV file (no data/fact): "+filename);return true;}
	// fact is an option section we don't need to worry about
	if (String(magic)==String("fact"))
	{
		stream->skip(8);
		// now we should hit the data chunk
		if (stream->read(magic, 4) != 4) {LOG("Could not read file "+filename);return true;}
		if (String(magic) != String("data")) {LOG("Invalid WAV file (no data): "+filename);return true;}
	}
	// the next four bytes are the remaining size of the file
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

	// ok, creating buffer
	void* bdata=malloc(dataSize);
	if (!bdata) {LOG("Memory error reading file "+filename);return true;}
	if (stream->read(bdata, dataSize) != dataSize) {LOG("Could not read file "+filename);return true;}

	//LOG("alBufferData: format "+TOSTRING(format)+" size "+TOSTRING(dataSize)+" freq "+TOSTRING(freq));
	alGetError(); // Reset errors
	ALint error;
	alBufferData(buffer, format, bdata, dataSize, freq);
	error=alGetError();

	free(bdata);
	// stream will be closed by itself

	if (error != AL_NO_ERROR) {LOG("OpenAL error while loading buffer for "+filename+" : "+TOSTRING(error));return true;}

	return false;
}

#endif // USE_OPENAL
