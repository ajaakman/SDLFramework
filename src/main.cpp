#ifdef __EMSCRIPTEN__
	#include <emscripten/emscripten.h>
	#include <functional>

	static void dispatch_main(void* fp)
	{
		std::function<void()>* func = (std::function<void()>*)fp;
		(*func)();
	}
#endif

#include <SDL.h>
#include <vector>

#define SINE_WAVE 0
#define SQUARE_WAVE 1
#define SAW_WAVE 2
#define TRIANGLE_WAVE 3
#define ANALOG_SAW 4
#define NOISE 5

SDL_AudioDeviceID device;

class AudioWaveform // This class contains audio function used by the AudioSynthesizer class.
{
public:
	struct Oscillator
	{
		friend class AudioWaveform;
	private:
		double m_dWaveAmplitude;
		double m_dWaveFrequency;
		unsigned m_nWaveType;
		unsigned m_nSawParts;

		double m_dVibratoFreq;
		double m_dVibratoAmplitude;

		double m_dTremoloFreq;
		double m_dTremoloAmplitude;

		int m_nTune;
		double m_dFineTune;

		// Oscillator frequency. Range int 1.0 - 20000.0
		void SetWaveFrequency(const double& dNewFrequency);

		Oscillator();
		// Passed to the Synthesizer
		double AudioFunction(const double dTime, const double dHertz);
	public:
		// Oscillator amplitude. Range double 0.0 - 1.0
		void SetWaveAmplitude(const double& dNewAmplitude);
		// Select wave type: SINE_WAVE, SQUARE_WAVE, TRIANGLE_WAVE, SAW_WAVE, ANALOG_SAW or NOISE. Optional argument sets number of parts for analog saw waves. Does nothing for other waveforms.
		void SetWaveType(const unsigned int& nNewWave, const unsigned int& nNewSawParts = 50);
		// Vibrato LFO frequency. Range double 0.0 - 100.0
		void SetVibratoFrequency(const double& dNewFrequency);
		// Vibrato amplitude multiplier. Range double 0.0 - 1.0
		void SetVibratoAmplitude(const double& dNewAmplitude);
		// Tremolo LFO frequency. Range double 0.0 - 100.0
		void SetTremoloFrequency(const double& dNewFrequency);
		// Tremolo amplitude multiplier. Range double 0.0 - 1.0
		void SetTremoloAmplitude(const double& dNewAmplitude);
		// Tune OSC. Range int -36 - 36.
		void SetTune(const int& dNewTune);
		// Fine tune OSC. Range double 0.0 - 1.0.
		void SetFineTune(const double& dNewTune);
	};

	struct Note
	{
		friend class AudioWaveform;
	private:
		int m_nNoteID;
		double m_dNoteOnTime;
		double m_dNoteOffTime;
		bool m_bIsNoteActive;

		Note();
	};

	struct Envelope
	{
		friend class AudioWaveform;
	private:
		double m_dAttackTime;
		double m_dDecayTime;
		double m_dSustainAmp;

		double m_dReleaseTime;
		double m_dStartAmp;

		Envelope();

		double ADSREnvelope(const AudioWaveform& wf, const double& dTimeOn, const double& dTimeOff);

	public:
		// Attack time. Range double 0.0 - 50
		void SetAttackTime(const double& dNewTime);
		// Start amplitude multiplier. Range double 0.0 - 1.0
		void SetStartAmplitude(const double& dNewAmplitude);
		// Decay time. Range double 0.0 - 50
		void SetDecayTime(const double& dNewTime);
		// Sustain amplitude multiplier. Range double 0.0 - 1.0
		void SetSusatainAmplitude(const double& dNewAmplitude);
		// Release time. Range double 0.0 - 50
		void SetReleaseTime(const double& dNewTime);
	};

private:

	std::vector<Note> m_Notes;

	double m_dMasterVolume;

public:

	Envelope ADSR;
	Oscillator OSC1;
	Oscillator OSC2;
	Oscillator OSC3;
	// Amplitude multiplier. Range double 0.0 - 1.0
	void SetMasterVolume(const double& dNewAmplitude);

	void NoteTriggered(const int& nKey);
	void NoteReleased(const int& nKey);

	double WaveformFunction();
protected:

	AudioWaveform();

private:

	static double Scale(const int& nNoteID);

	virtual const double& GetSampleTime() const = 0;

};



AudioWaveform::AudioWaveform()
	: m_dMasterVolume(0.02)
{	}

AudioWaveform::Oscillator::Oscillator()
	: m_dWaveAmplitude(0.1), m_dWaveFrequency(444.0), m_nWaveType(1), m_nSawParts(50), m_dVibratoFreq(5.0), m_dVibratoAmplitude(0.003), m_dTremoloFreq(0.1), m_dTremoloAmplitude(0.01), m_nTune(0), m_dFineTune(0.0)
{	}

AudioWaveform::Note::Note()
	: m_nNoteID(0), m_dNoteOnTime(0.0), m_dNoteOffTime(0.0), m_bIsNoteActive(false)
{	}

AudioWaveform::Envelope::Envelope()
	: m_dAttackTime(0.1), m_dDecayTime(0.0), m_dReleaseTime(0.5), m_dSustainAmp(1.0), m_dStartAmp(1.0)
{	}

double AudioWaveform::WaveformFunction()
{	
	double dMasterOut = 0.0;
	for (auto &note : m_Notes)
	{
		bool bNoteFinished = false;
		double dVoice = 0.0;

		double dAmplitude = ADSR.ADSREnvelope(*this, note.m_dNoteOnTime, note.m_dNoteOffTime);
		if (dAmplitude <= 0.0) bNoteFinished = true;

		double dSound = m_dMasterVolume *
			(OSC1.AudioFunction(note.m_dNoteOnTime - GetSampleTime(), AudioWaveform::Scale(note.m_nNoteID + OSC1.m_nTune) + OSC1.m_dFineTune)
				+ OSC2.AudioFunction(note.m_dNoteOnTime - GetSampleTime(), AudioWaveform::Scale(note.m_nNoteID + OSC2.m_nTune) + OSC2.m_dFineTune)
				+ OSC3.AudioFunction(note.m_dNoteOnTime - GetSampleTime(), AudioWaveform::Scale(note.m_nNoteID + OSC3.m_nTune) + OSC3.m_dFineTune));

		dVoice = dAmplitude * dSound;

		dMasterOut += dVoice;

		if (bNoteFinished && note.m_dNoteOffTime > note.m_dNoteOnTime)
			note.m_bIsNoteActive = false;
	}

	for (unsigned int i = 0; i < m_Notes.size(); ++i)
	{
		if (!m_Notes[i].m_bIsNoteActive)
			m_Notes.erase(m_Notes.begin() + i);
	}

	return dMasterOut;
}

double AudioWaveform::Oscillator::AudioFunction(const double dTime, const double dHertz)
{
	double dTremolo = m_dTremoloAmplitude * sin(m_dTremoloFreq * M_PI * 2.0 * dTime);
	double dVibrato = m_dVibratoAmplitude * dHertz * sin(m_dVibratoFreq * M_PI * 2.0 * dTime);
	double dFrequency = dHertz * M_PI * 2.0 * dTime + dVibrato;

	switch (m_nWaveType)
	{
	case SQUARE_WAVE:
		return (m_dWaveAmplitude + dTremolo) * signbit((sin(dFrequency)));
		//return  m_dWaveAmplitude * (sin(dFrequency) > 0.0 ? 1.0 : -1.0);
	case TRIANGLE_WAVE:
		return (m_dWaveAmplitude + dTremolo) * (asin(sin(dFrequency)) * 2.0 / M_PI * 2.0);
	case SAW_WAVE:
		return (m_dWaveAmplitude + dTremolo) * -2 / M_PI * (atan(1.0 / tan(dHertz * dTime * M_PI + dVibrato)));
		//return m_dWaveAmplitude * (((2.0 / PI<double>) * ((dHertz * PI<double> * fmod(dTime, 1.0 / dHertz)) - 1)) );
	case ANALOG_SAW:
	{
		double dOut = 0.0;

		for (int i = 1; i < m_nSawParts; ++i)
			dOut += (sin(i * dFrequency)) / i;

		return (m_dWaveAmplitude + dTremolo) * ((dOut * (2.0 / M_PI)));
	}
	case NOISE:
		return (m_dWaveAmplitude + dTremolo) * ((2.0 * ((double)rand() / (double)RAND_MAX) - 1.0));
	default: // Sine wave.
		return (m_dWaveAmplitude + dTremolo) * (sin(dFrequency));
	}
}

double AudioWaveform::Envelope::ADSREnvelope(const AudioWaveform& wf, const double& dTriggerOnTime, const double& dTriggerOffTime)
{
	double dAmplitude = 0.0;
	double dReleaseAmplitude = 0.0;

	if (dTriggerOnTime > dTriggerOffTime)
	{
		double dLifeTime = wf.GetSampleTime() - dTriggerOnTime;
		// Attack
		if (dLifeTime <= m_dAttackTime)
			dAmplitude = (dLifeTime / m_dAttackTime) * m_dStartAmp;
		// Decay
		if (dLifeTime > m_dAttackTime && dLifeTime <= (m_dAttackTime + m_dDecayTime))
			dAmplitude = ((dLifeTime - m_dAttackTime) / m_dDecayTime) * (m_dSustainAmp - m_dStartAmp) + m_dStartAmp;
		// Sustian 
		if (dLifeTime > (m_dAttackTime + m_dDecayTime))
			dAmplitude = m_dSustainAmp;
	}
	else
	{
		//Release
		double dLifeTime = dTriggerOffTime - dTriggerOnTime;

		if (dLifeTime <= m_dAttackTime)
			dReleaseAmplitude = (dLifeTime / m_dAttackTime) * m_dStartAmp;

		if (dLifeTime > m_dAttackTime && dLifeTime <= (m_dAttackTime + m_dDecayTime))
			dReleaseAmplitude = ((dLifeTime - m_dAttackTime) / m_dDecayTime) * (m_dSustainAmp - m_dStartAmp) + m_dStartAmp;

		if (dLifeTime > (m_dAttackTime + m_dDecayTime))
			dReleaseAmplitude = m_dSustainAmp;

		dAmplitude = ((wf.GetSampleTime() - dTriggerOffTime) / m_dReleaseTime) * (0.0 - dReleaseAmplitude) + dReleaseAmplitude;
	}

	if (dAmplitude <= 0.0001)
		dAmplitude = 0;

	return dAmplitude;
}

void AudioWaveform::NoteTriggered(const int& nKey)
{
	SDL_LockAudioDevice(device);
	bool bIsKeyActive = false;

	for (unsigned int i = 0; i < m_Notes.size(); i++)
	{
		if (m_Notes[i].m_nNoteID == nKey)
			bIsKeyActive = true;
	}

	if (bIsKeyActive)
	{
		for (unsigned int i = 0; i < m_Notes.size(); i++)
		{
			if (m_Notes[i].m_nNoteID == nKey)
				m_Notes[i].m_dNoteOnTime = GetSampleTime();
		}
	}
	else
	{
		Note note;
		note.m_nNoteID = nKey;
		note.m_dNoteOnTime = GetSampleTime();
		note.m_bIsNoteActive = true;
		m_Notes.push_back(note);
	}
	SDL_UnlockAudioDevice(device);
}

void AudioWaveform::NoteReleased(const int& nKey)
{
	SDL_LockAudioDevice(device);
	for (unsigned int i = 0; i < m_Notes.size(); i++)
	{
		if (m_Notes[i].m_nNoteID == nKey)
			m_Notes[i].m_dNoteOffTime = GetSampleTime();
	}
	SDL_UnlockAudioDevice(device);
}

double AudioWaveform::Scale(const int& nNoteID)
{
	return 261.63 * pow(1.0594630943592952645618252949463, nNoteID);
}

void AudioWaveform::SetMasterVolume(const double& dNewAmplitude)
{
	SDL_LockAudioDevice(device);
	if (dNewAmplitude > 1.0)
		m_dMasterVolume = 1.0;
	else if (dNewAmplitude < 0.0)
		m_dMasterVolume = 0.0;
	else
		m_dMasterVolume = dNewAmplitude;
	SDL_UnlockAudioDevice(device);
}

void AudioWaveform::Oscillator::SetWaveFrequency(const double& dNewFrequency)
{
	SDL_LockAudioDevice(device);
	if (dNewFrequency < 1.0)
		m_dWaveFrequency = 1.0;
	else if (dNewFrequency > 20000.0)
		m_dWaveFrequency = 20000.0;
	else
		m_dWaveFrequency = dNewFrequency;
	SDL_UnlockAudioDevice(device);
}

void AudioWaveform::Oscillator::SetWaveAmplitude(const double& dNewAmplitude)
{
	SDL_LockAudioDevice(device);
	if (dNewAmplitude < 0.0)
		m_dWaveAmplitude = 0.0;
	else if (dNewAmplitude > 1.0)
		m_dWaveAmplitude = 1.0;
	else
		m_dWaveAmplitude = dNewAmplitude;
	SDL_UnlockAudioDevice(device);
}

void AudioWaveform::Oscillator::SetWaveType(const unsigned int& nNewWave, const unsigned int& nNewSawParts)
{
	SDL_LockAudioDevice(device);
	switch (nNewWave)
	{
	case 0: m_nWaveType = SINE_WAVE; break;
	case 1: m_nWaveType = SQUARE_WAVE; break;
	case 2: m_nWaveType = SAW_WAVE; break;
	case 3: m_nWaveType = TRIANGLE_WAVE; break;
	case 4:
	{
		m_nWaveType = ANALOG_SAW;
		if (nNewSawParts > 100)
			m_nSawParts = 100;
		else if (nNewSawParts < 2)
			m_nSawParts = 2;
		else
			m_nSawParts = nNewSawParts;
		break;
	}
	case 5: m_nWaveType = NOISE; break;
	default: m_nWaveType = SINE_WAVE;
	}
	SDL_UnlockAudioDevice(device);
}

void AudioWaveform::Oscillator::SetVibratoFrequency(const double& dNewFrequency)
{
	SDL_LockAudioDevice(device);
	if (dNewFrequency < 0.0)
		m_dVibratoFreq = 0.0;
	else if (dNewFrequency > 100.0)
		m_dVibratoFreq = 100.0;
	else
		m_dVibratoFreq = dNewFrequency;
	SDL_UnlockAudioDevice(device);
}

void AudioWaveform::Oscillator::SetVibratoAmplitude(const double& dNewAmplitude)
{
	SDL_LockAudioDevice(device);
	if (dNewAmplitude < 0.0)
		m_dVibratoAmplitude = 0.0;
	else if (dNewAmplitude > 1.0)
		m_dVibratoAmplitude = 1.0;
	else
		m_dVibratoAmplitude = dNewAmplitude;
	SDL_UnlockAudioDevice(device);
}

void AudioWaveform::Oscillator::SetTremoloFrequency(const double& dNewFrequency)
{
	SDL_LockAudioDevice(device);
	if (dNewFrequency < 0.0)
		m_dTremoloFreq = 0.0;
	else if (dNewFrequency > 100.0)
		m_dTremoloFreq = 100.0;
	else
		m_dTremoloFreq = dNewFrequency;
	SDL_UnlockAudioDevice(device);
}

void AudioWaveform::Oscillator::SetTremoloAmplitude(const double& dNewAmplitude)
{
	SDL_LockAudioDevice(device);
	if (dNewAmplitude < 0.0)
		m_dTremoloAmplitude = 0.0;
	else if (dNewAmplitude > 1.0)
		m_dTremoloAmplitude = 1.0;
	else
		m_dTremoloAmplitude = dNewAmplitude;
	SDL_UnlockAudioDevice(device);
}

void AudioWaveform::Oscillator::SetTune(const int& dNewTune)
{
	SDL_LockAudioDevice(device);
	if (dNewTune < -36)
		m_nTune = -36;
	else if (dNewTune > 36)
		m_nTune = 36;
	else
		m_nTune = dNewTune;
	SDL_UnlockAudioDevice(device);
}

void AudioWaveform::Oscillator::SetFineTune(const double& dNewTune)
{
	SDL_LockAudioDevice(device);
	if (dNewTune < -1.0)
		m_dFineTune = -1.0;
	else if (dNewTune > 1.0)
		m_dFineTune = 1.0;
	else
		m_dFineTune = dNewTune;
	SDL_UnlockAudioDevice(device);
}

void AudioWaveform::Envelope::SetAttackTime(const double& dNewTime)
{
	SDL_LockAudioDevice(device);
	if (dNewTime < 0.0)
		m_dAttackTime = 0.0;
	else if (dNewTime > 5.0)
		m_dAttackTime = 5.0;
	else
		m_dAttackTime = dNewTime;
	SDL_UnlockAudioDevice(device);
}

void AudioWaveform::Envelope::SetStartAmplitude(const double& dNewAmplitude)
{
	SDL_LockAudioDevice(device);
	if (dNewAmplitude < 0.0)
		m_dStartAmp = 0.0;
	else if (dNewAmplitude > 1.0)
		m_dStartAmp = 1.0;
	else
		m_dStartAmp = dNewAmplitude;
	SDL_UnlockAudioDevice(device);
}

void AudioWaveform::Envelope::SetDecayTime(const double& dNewTime)
{
	SDL_LockAudioDevice(device);
	if (dNewTime < 0.0)
		m_dDecayTime = 0.0;
	else if (dNewTime > 5.0)
		m_dDecayTime = 5.0;
	else
		m_dDecayTime = dNewTime;
	SDL_UnlockAudioDevice(device);
}

void AudioWaveform::Envelope::SetSusatainAmplitude(const double& dNewAmplitude)
{
	SDL_LockAudioDevice(device);
	if (dNewAmplitude < 0.0)
		m_dSustainAmp = 0.0;
	else if (dNewAmplitude > 1.0)
		m_dSustainAmp = 1.0;
	else
		m_dSustainAmp = dNewAmplitude;
	SDL_UnlockAudioDevice(device);
}

void AudioWaveform::Envelope::SetReleaseTime(const double& dNewTime)
{
	SDL_LockAudioDevice(device);

	if (dNewTime < 0.0)
		m_dReleaseTime = 0.0;
	else if (dNewTime > 5.0)
		m_dReleaseTime = 5.0;
	else
		m_dReleaseTime = dNewTime;

	SDL_UnlockAudioDevice(device);
}

struct AudioData : public AudioWaveform
{
	AudioData()
	{
		SetMasterVolume(0.1);	

		ADSR.SetAttackTime(0.05);
		ADSR.SetDecayTime(1.0);
		ADSR.SetReleaseTime(0.7);
		ADSR.SetStartAmplitude(0.7);
		ADSR.SetSusatainAmplitude(0.7);

		OSC1.SetWaveAmplitude(0.9);
		OSC2.SetWaveAmplitude(0.8);
		OSC3.SetWaveAmplitude(1.0);

		OSC1.SetWaveType(SAW_WAVE, 0);
		OSC2.SetWaveType(SQUARE_WAVE, 0);
		OSC3.SetWaveType(TRIANGLE_WAVE, 0);

		OSC1.SetTune(-12);
		OSC2.SetTune(0);
		OSC3.SetTune(12);

		OSC1.SetFineTune(0.0);
		OSC2.SetFineTune(-0.1);
		OSC3.SetFineTune(0.1);

		OSC1.SetTremoloAmplitude(0.003);
		OSC1.SetTremoloFrequency(5.0);
		OSC2.SetTremoloAmplitude(0.003);
		OSC2.SetTremoloFrequency(5.0);
		OSC3.SetTremoloAmplitude(0.003);
		OSC3.SetTremoloFrequency(5.0);

		OSC1.SetVibratoAmplitude(0.003);
		OSC1.SetVibratoFrequency(5.0);
		OSC2.SetVibratoAmplitude(0.003);
		OSC2.SetVibratoFrequency(5.0);
		OSC3.SetVibratoAmplitude(0.003);
		OSC3.SetVibratoFrequency(5.0);
	}

	double m_dSampleTime = 0.0;
	inline const double& GetSampleTime() const override { return m_dSampleTime; }
};

void MyAudioCallback(void* userdata, Uint8* stream, int streamLength) // streamLength = samples * channels * bitdepth/8
{
	AudioData* audio = static_cast<AudioData*>(userdata);

	for (Uint32 i = 0; i < streamLength / 2; ++i)
	{
		((Sint16*)stream)[i] = 0;

		((Sint16*)stream)[i] += Sint16(audio->WaveformFunction() * 32767);
			
		audio->m_dSampleTime += 1.0 / 41000.0;
	}		
}


int main(int argc, char* args[])
{
	SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Running...\n");
	SDL_Window* window = nullptr;
	SDL_Surface* surface = nullptr;

	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0)
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
	else
	{
		if (!SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1"))
		{
			SDL_Log("Warning: Linear texture filtering not enabled!");
		}
#ifdef __ANDROID__
		SDL_DisplayMode displayMode;
		if (SDL_GetCurrentDisplayMode(0, &displayMode) == 0)		
			window = SDL_CreateWindow("SDL Framework", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, displayMode.w, displayMode.h, SDL_WINDOW_SHOWN);
		else
			SDL_LogError(SDL_LOG_CATEGORY_VIDEO, "Could not get display mode for video display #%d: %s", 0, SDL_GetError());
#else
		window = SDL_CreateWindow("SDL Framework", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 1024, 576, SDL_WINDOW_SHOWN);
#endif
		if (window == nullptr)
			SDL_LogError(SDL_LOG_CATEGORY_VIDEO, "Window could not be created! SDL_Error: %s\n", SDL_GetError());
		else
		{
            SDL_Renderer* gRenderer = SDL_CreateRenderer( window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC );
            if( gRenderer == NULL )
                SDL_LogError(SDL_LOG_CATEGORY_VIDEO, "Renderer could not be created! SDL Error: %s\n", SDL_GetError() );
			else      
				SDL_SetRenderDrawColor(gRenderer, 0xFF, 0xFF, 0x00, 0xFF);

			SDL_AudioSpec spec;
			//SDL_AudioDeviceID device;
			AudioData audioData;

			SDL_memset(&spec, 0, sizeof(spec));

			spec.userdata = &audioData;
			spec.channels = 1;
			spec.freq = 44100;
			spec.format = AUDIO_S16SYS;
			spec.samples = 2048;
			spec.callback = MyAudioCallback;

			device = SDL_OpenAudioDevice(NULL, 0, &spec, NULL, 0);

			if (device == 0)
				SDL_LogError(SDL_LOG_CATEGORY_AUDIO, "Could not open audio device %s\n", SDL_GetError());

			SDL_PauseAudioDevice(device, 0);

			SDL_Rect fillRect = { 1024 / 4, 576 / 4, 1024 / 2, 576 / 2 };

			bool quit = false;

			SDL_Event e;
#ifdef __EMSCRIPTEN__
			std::function<void()> mainLoop = [&]() {
#else
			while (!quit) {
#endif
				SDL_SetRenderDrawColor(gRenderer, 0xFF, 0x00, 0x00, 0xFF);
				SDL_RenderClear(gRenderer);
				SDL_SetRenderDrawColor(gRenderer, 0xFF, 0xFF, 0x00, 0xFF);
				while (SDL_PollEvent(&e) != 0)
				{
					if (e.type == SDL_QUIT)
						quit = true;
#ifdef __ANDROID__
					if (e.type == SDL_FINGERDOWN)
#else
					if (e.type == SDL_MOUSEBUTTONDOWN)					
						if (e.button.button == SDL_BUTTON_LEFT)
#endif						
						{
							SDL_SetRenderDrawColor(gRenderer, 0xFF, 0x00, 0xFF, 0xFF);
							audioData.NoteTriggered(1);
							audioData.NoteTriggered(5);
						}
#ifdef __ANDROID__
					if (e.type == SDL_FINGERUP)
#else
					if (e.type == SDL_MOUSEBUTTONUP)
						if (e.button.button == SDL_BUTTON_LEFT)
#endif						
						{
							SDL_SetRenderDrawColor(gRenderer, 0xFF, 0xFF, 0x00, 0xFF);
							audioData.NoteReleased(1);
							audioData.NoteReleased(5);
						}
				}

				SDL_RenderFillRect(gRenderer, &fillRect);				
				SDL_RenderPresent(gRenderer);

			}
#ifdef __EMSCRIPTEN__
			; emscripten_set_main_loop_arg(dispatch_main, &mainLoop, 0, 1);
#endif
		}
	}

	SDL_DestroyWindow(window);
	SDL_Quit();

	SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Exiting...\n");
	return 0;
}