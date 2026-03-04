#include <vector>
#include <sstream>
#include <filesystem>

#include <bass.h>
#include <bassmidi.h>


#include "../logger.h"
#include "playback.h"
#include "../globals.h"
#include "audio_effects.h"
#include "../render/render.h"






double Playback::Tplay = 0.0;
bool Playback::is_midi_loaded = false;
bool Playback::is_playback_started = false;
bool Playback::playback_ended = false;
u64 Playback::saved_position = 0;
const f64 Playback::seek_amount = 3.0;
f64 Playback::Tscr;


void Playback::LoadDefaultSoundfonts()
{
    
    std::vector<BASS_MIDI_FONT> fontSet;
#ifndef NON_ANDROID
        //std::string default_sf_path = NVFileUtils::GetFilePathA("piano_maganda.sf2", "rb");
        //std::string default_gm_sf_path = NVFileUtils::GetFilePathA("gm_generic.sf2", "rb");
        
        HSOUNDFONT Sf1 = BASS_MIDI_FontInit(DEFAULT_GM_SOUND_FONT_PATH, 0);
        HSOUNDFONT Sf2 = BASS_MIDI_FontInit(DEFAULT_SOUND_FONT_PATH, 0);
        
        fontSet.push_back({Sf2, -1, 0});    // override preset 40 (Violin) in bank 0
        fontSet.push_back({Sf1, -1, 0});
        
        BASS_MIDI_FontSetVolume(Sf1, 0.15);
        BASS_MIDI_FontSetVolume(Sf2, 0.15);
#else
        HSOUNDFONT Sf1 = BASS_MIDI_FontInit(DEFAULT_GM_SOUNDFONT, 0);
        HSOUNDFONT Sf2 = BASS_MIDI_FontInit(DEFAULT_SOUNDFONT, 0);
        
        fontSet.push_back({Sf2, -1, 0});    // override preset 40 (Violin) in bank 0
        fontSet.push_back({Sf1, -1, 0});
        
        BASS_MIDI_FontSetVolume(Sf1, 0.15);
        BASS_MIDI_FontSetVolume(Sf2, 0.15);
        
#endif

    if(!fontSet.empty())
        BASS_MIDI_StreamSetFonts(Playback::main_stream, fontSet.data(), fontSet.size());
}

bool Playback::LoadEnabledSoundfonts(std::vector<UI::SoundfontItem> enabled_soundfonts)
{
    std::vector<BASS_MIDI_FONT> fontSet;
    
    bool is_enabled_sf_available = false;
    
    for(const auto& soundfont : enabled_soundfonts) // Iterate through all enabled soundfonts
    {
        if(soundfont.checked)
        {
            HSOUNDFONT Sf = BASS_MIDI_FontInit(soundfont.label.c_str(), 0);
            if(Sf)
            {
                BASS_MIDI_FontSetVolume(Sf, 0.15);
                BASS_MIDI_FONT font = {Sf, -1, 0}; // Set the soundfont context, preset and bank
                fontSet.push_back(font);
            }
            is_enabled_sf_available = true;
        }
    }
    
    if(!fontSet.empty())
        BASS_MIDI_StreamSetFonts(Playback::main_stream, fontSet.data(), fontSet.size()); // Load soundfonts
    
    return is_enabled_sf_available;
}

void Playback::ReloadSoundfonts()
{
    
    if(!Playback::main_stream || !BASS_ChannelIsActive(Playback::main_stream))
        return; // No active stream to modify
    
    
    // Save current playback position and state
    u64 position = BASS_ChannelGetPosition(Playback::main_stream, BASS_POS_BYTE);
    bool was_playing = !is_paused;
    
    // Get current midi file path
    std::string current_midi = loaded_config.last_midi_path;
    
    // Stop and free the current stream
    BASS_ChannelStop(Playback::main_stream);
    BASS_StreamFree(Playback::main_stream);
    
    // Create new stream with the same MIDI file
    Playback::main_stream = BASS_StreamCreateFile(0, current_midi.c_str(), 0, 0, BASS_SAMPLE_FLOAT);
    BASS_ChannelSetDSP(Playback::main_stream, &dsp_limiter, 0, 0);
    
    //LoadDefaultSoundfonts();
    if(!LoadEnabledSoundfonts(live_soundfont_list))
        LoadDefaultSoundfonts();
    
    // Restore other settings
    BASS_ChannelSetAttribute(Playback::main_stream, BASS_ATTRIB_MIDI_VOICES, loaded_config.bass_voice_count);
    BASS_MIDI_StreamSetFilter(Playback::main_stream, 0, reinterpret_cast<BOOL (*)(HSTREAM, int, BASS_MIDI_EVENT *, BOOL, void *)>(filter), nullptr);
    
    // Restore position
    BASS_ChannelSetPosition(Playback::main_stream, position, BASS_POS_BYTE);
    
    // Resume playback if it was playing before
    if(was_playing)
    {
        BASS_ChannelPlay(Playback::main_stream, FALSE);
        Playback::is_paused = false;
    } 
    else
        Playback::is_paused = true;
    
    
    // Update our playback time
    Playback::Tplay = BASS_ChannelBytes2Seconds(Playback::main_stream, position);
    
    Log::info("", "Soundfonts reloaded");
}

void Playback::updateBassVoiceCount(int voiceCount)
{
    if(Playback::main_stream && BASS_ChannelIsActive(Playback::main_stream))
    {
        // Update the voice count for the current stream
        BASS_ChannelSetAttribute(Playback::main_stream, BASS_ATTRIB_MIDI_VOICES, voiceCount);
        
        // Update the configuration
        loaded_config.bass_voice_count = voiceCount;
        
        Log::info("Voice count updated to: %d");
    }
}

void Playback::loadMidiFile(const std::string& midi_path)
{
    SDL_LockMutex(bass_mutex);
    
    // Make sure the main window is closed before loading a new midi
    if(UI::main_gui_window)
        UI::main_gui_window = false;
    
    
    if(!std::filesystem::exists(midi_path))
    {
        Log::error("MIDI File does not exists ! '%s'", midi_path.c_str());
        SDL_UnlockMutex(bass_mutex);
        return;
    }
    
    // Parse new MIDI file
    
    if(!Midi_ctx.start_parse(midi_path.c_str()))
    {
        std::ostringstream temp_msg;
        temp_msg << "Failed to load '" << midi_path << "'";
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "MIDI Loading Error!!!!!", temp_msg.str().c_str(), nullptr);
        return;
    }

    
    // - - - - [Create new MIDI stream] - - - -
    
    // Honestly idfk which one is better
    //Playback::main_stream = BASS_StreamCreateFile(0, midi_path.c_str(), 0, 0, BASS_SAMPLE_FLOAT | BASS_STREAM_PRESCAN | BASS_STREAM_DECODE); // On heavy load it caues longer sutters
    Playback::main_stream = BASS_MIDI_StreamCreateFile(FALSE, midi_path.c_str(), 0, 0, BASS_SAMPLE_FLOAT, 1); // While this 1 causes many short stutters. Absolute garbage
    BASS_ChannelSetDSP(Playback::main_stream, &dsp_limiter, 0, 0);
    
    if(!LoadEnabledSoundfonts(live_soundfont_list))
        LoadDefaultSoundfonts();
    
    BASS_ChannelSetAttribute(Playback::main_stream, BASS_ATTRIB_MIDI_VOICES, loaded_config.bass_voice_count);
    if(live_conf.vel_filter == true)
        BASS_MIDI_StreamSetFilter(Playback::main_stream, 0, reinterpret_cast<BOOL (*)(HSTREAM, int, BASS_MIDI_EVENT *, BOOL, void *)>(filter), nullptr);
    
    is_midi_loaded = true;
    
    //sleep(1); // Sleep 1 second before starting playback
    
    // Start playback
    BASS_ChannelPlay(Playback::main_stream, 1);
    
    // Reset playback variables
    Tplay = 0.0;
    is_paused = false;
    playback_ended = false; // Allow the playback to start with the audio playback
    
    // Update current midi path
    loaded_config.last_midi_path = midi_path;
    
    SDL_UnlockMutex(bass_mutex);
}

void Playback::CloseMidi()
{
    // Stop current playback and free resources
    if(Playback::main_stream)
    {
        BASS_ChannelStop(Playback::main_stream);
        BASS_StreamFree(Playback::main_stream);
    }
    
    // Reset note lists
    for(int i = 0; i < 128; ++i)
    {
        Midi_ctx.Note_list[i].clear();
        std::list<NVnote>().swap(Midi_ctx.Note_list[i]);  // Shrink the capacity so the visualization performace is the same when playing new midis
    }
    
    
    // Clear previous channel / track colors
    RenderWin->ClearTrackChannelColors();
    is_midi_loaded = false;
}

void Playback::seek_playback(f64 seconds)
{
    if(!is_midi_loaded)
        return;
    
    u64 current_byte_pos = BASS_ChannelGetPosition(Playback::main_stream, BASS_POS_BYTE);
    f64 current_time = BASS_ChannelBytes2Seconds(Playback::main_stream, current_byte_pos);
    f64 new_time = current_time + seconds;
    
    // Ensure we don't seek before the beginning
    if(new_time < 0)
        new_time = 0;
    
    // Convert back to bytes and set position
    u64 new_pos = BASS_ChannelSeconds2Bytes(Playback::main_stream, new_time);
    BASS_ChannelSetPosition(Playback::main_stream, new_pos, BASS_POS_BYTE);
    
    // Update our playback time
    Tplay = BASS_ChannelBytes2Seconds(Playback::main_stream, new_pos);
    
    // When seeking backwards, reload the note data
    if(seconds < 0)
    {
        // Clear the note list
        for(int i = 0; i < 128; ++i)
            Midi_ctx.Note_list[i].clear();
        
        
        // Seek the list to the new position and update
        Midi_ctx.list_seek(Tplay);
        Midi_ctx.update_to(Tplay + Tscr);
    }
    
    if(playback_ended) 
	{
		// If at the end and trying to seek back, restart playback from desired position
		f64 new_time = Tplay + (-seek_amount);
		if(new_time < 0)
		    new_time = 0;
		
		u64 new_pos = BASS_ChannelSeconds2Bytes(Playback::main_stream, new_time);
		BASS_ChannelSetPosition(Playback::main_stream, new_pos, BASS_POS_BYTE);
		BASS_ChannelPlay(Playback::main_stream, FALSE);
		
		// Reset visualization properly
		for(int i = 0; i < 128; ++i) 
			Midi_ctx.Note_list[i].clear();
		
		
		// Update time and reset flags
		Tplay = new_time;
		Midi_ctx.list_seek(Tplay);
		playback_ended = false;
		is_paused = false;
	}
}

void Playback::pause()
{
    if(!is_midi_loaded)
        return;
    
    if(is_paused)
    {
        // Resume playback
        BASS_ChannelPlay(main_stream, false); // false means don't restart from beginning
        is_paused = false;
    } 
    else
    {
        // Pause playback
        saved_position = BASS_ChannelGetPosition(main_stream, BASS_POS_BYTE);
        BASS_ChannelPause(main_stream);
        is_paused = true;
    }
    
    if(playback_ended)
	{
		// If at the end and we press space, restart from beginning
		ReloadSoundfonts(); // Useful for when chaning soundfonts after playback ended
		BASS_ChannelSetPosition(main_stream, 0, BASS_POS_BYTE);
		BASS_ChannelPlay(main_stream, FALSE);
		Tplay = 0.0;
		playback_ended = false;
		is_paused = false;
		
		// Reset visualization state
		for(int i = 0; i < 128; ++i)
			Midi_ctx.Note_list[i].clear();
		
		Midi_ctx.list_seek(0);
	}
}

std::vector<Playback::AudioDevice> Playback::GetAudioOutputs()
{
    std::vector<Playback::AudioDevice> res;
    BASS_DEVICEINFO dev_info;
    int deviceIndex = 0;
    
    while(BASS_GetDeviceInfo(deviceIndex, &dev_info))
    {
        deviceIndex++;
        res.push_back({deviceIndex, dev_info.name, dev_info.driver, (dev_info.flags & BASS_DEVICE_DEFAULT) ? true : false, (dev_info.flags & BASS_DEVICE_ENABLED) ? true : false});
    }
   
    return res;
}

void Playback::bassErrorHandler()
{
    int error_code = BASS_ErrorGetCode();
    std::ostringstream msg_str;
        
    switch(error_code)
    {
        case BASS_ERROR_DEVICE:
            msg_str << "Device index is invalid\n";
            break;
        case BASS_ERROR_ALREADY:
            msg_str << "BASS Already initialized\n";
            break;
        case BASS_ERROR_DRIVER:
            msg_str << "Unavailable device driver\n";
            break;
        case BASS_ERROR_FORMAT:
            msg_str << "Unsupported format by device\n";
            break;
        case BASS_ERROR_MEM:
            msg_str << "Insufficient memory\n";
            break;
        case BASS_ERROR_NO3D:
            msg_str << "Failed to initialize 3D support\n";
            break;
        case BASS_ERROR_UNKNOWN:
            msg_str << "Unknown error occured!!!\n";
            break;
        default:
            msg_str << "Unhandled error code!!!\n";
            break;
    }
    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Bass Init Error", msg_str.str().c_str(), NULL);
}