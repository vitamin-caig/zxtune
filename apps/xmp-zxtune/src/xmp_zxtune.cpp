/*
XMPlay ZXTune input plugin by djdron (C) 2015

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif//_CRT_SECURE_NO_WARNINGS

#define NOMINMAX
#include <xmpin.h>

//common includes
#include <contract.h>
#include <cycle_buffer.h>
#include <error_tools.h>
#include <progress_callback.h>
//library includes
#include <binary/container.h>
#include <binary/container_factories.h>
#include <time/stamp.h>
#include <core/core_parameters.h>
#include <core/module_open.h>
#include <core/module_holder.h>
#include <core/module_player.h>
#include <core/module_detect.h>
#include <core/module_attrs.h>
#include <core/data_location.h>
#include <parameters/container.h>
#include <sound/sound_parameters.h>
#include <platform/version/api.h>

//std includes
#include <vector>

//boost
#include <boost/bind.hpp>
#include <boost/make_shared.hpp>
#include <boost/static_assert.hpp>
#include <boost/range/end.hpp>
#include <boost/type_traits/is_signed.hpp>
#include <boost/lexical_cast.hpp>

#include "player.h"

static XMPFUNC_IN*		xmpfin;
static XMPFUNC_MISC*	xmpfmisc;
static XMPFUNC_FILE*	xmpffile;
static XMPFUNC_TEXT*	xmpftext;

static const char* file_types[] =
{
	// AY/YM
	"as0", "asc", "ay", "ayc", "gtr", "$c", "logo1", "psg", "pt1", "pt2", "pt3", "st1", "st3", "stc", "stp", "ts", "vtx", "ym", "ftc", "psc", "sqt",
	// dac
	"pdt", "chi", "str", "dst", "sqd", "et1", "dmm", "669", "amf", "dbm", "dmf", "dtm", "dtt", "emod", "far", "fnk", "gdm", "gtk", "mod", "mtn", "imf", "ims", "it", "liq", "psm", "mdl", "med", "mtm", "okt", "pt36", "ptm", "rtm", "s3m", "sfx", "stim", "stm", "stx", "tcb", "ult", "xm",
	// fm
	"tfc", "tfd", "tf0", "tfe",
	// Sam Coupe
	"cop",
	// C64
	"sid",
	// SNES
	"spc",
	// arch
	"hrp", "scl", "szx", "trd", "cc3", "dsq", "esv", "fdi", "gam", "gamplus", "$m", "$b", "hrm", "bin", "p", "lzs", "msp", "pcd", "td0", "tlz", "tlzp", "trs",
	// end
	NULL
};

enum
{
	raw_bits_per_sample = 16,
	raw_channels = 2,
	raw_sample_rate = 44100,

	raw_bytes_per_sample = raw_bits_per_sample / 8,
	raw_total_sample_width = raw_bytes_per_sample * raw_channels,
};

class xmp_zxtune
{
public:
	bool open(const char*, XMPFILE file, bool info_only = false)
	{
		DWORD size = xmpffile->GetSize(file);
		std::auto_ptr<Dump> buf(new Dump(size));
		xmpffile->Read(file, &buf->front(), size);
		input_file = Binary::CreateContainer(buf);
		if(!input_file)
			return false;
		ParseModules();
		if(!info_only)
			return seek(0);
		return input_modules.size() > 0;
	}
	void ParseModules()
	{
		struct ModuleDetector : public Module::DetectCallback
		{
			ModuleDetector(Modules* _mods, double* _length) : modules(_mods), length(_length) {}
			double FrameDuration(Parameters::Accessor::Ptr props) const
			{
				Parameters::IntType frameDuration = Parameters::ZXTune::Sound::FRAMEDURATION_DEFAULT;
				props->FindValue(Parameters::ZXTune::Sound::FRAMEDURATION, frameDuration);
				return double(frameDuration) / Time::Microseconds::PER_SECOND;
			}
		    virtual void ProcessModule(ZXTune::DataLocation::Ptr location, ZXTune::Plugin::Ptr, Module::Holder::Ptr holder) const
			{
				ModuleDesc m;
				m.module = holder;
				m.subname = location->GetPath()->AsString();
				Module::Information::Ptr mi = m.module->GetModuleInformation();
				Parameters::Accessor::Ptr props = m.module->GetModuleProperties();

				m.start = *length;
				m.length = mi->FramesCount() * FrameDuration(props);
				*length += m.length;
				Parameters::IntType size;
				if(props->FindValue(Module::ATTR_SIZE, size))
					m.bitrate = (int)(size*8/m.length);

				props->FindValue(Module::ATTR_TYPE, m.codec);
				props->FindValue(Module::ATTR_AUTHOR, m.author);
				props->FindValue(Module::ATTR_TITLE, m.title);
				if(m.title.empty())
					m.title = m.subname;
				props->FindValue(Module::ATTR_COMMENT, m.comment);
				props->FindValue(Module::ATTR_PROGRAM, m.program);
				modules->push_back(m);
			}
			virtual Log::ProgressCallback* GetProgress() const { return NULL; }
			virtual Parameters::Accessor::Ptr GetPluginsParameters() const { return Parameters::Container::Create(); }
			Modules* modules;
			double* length;
		};

		ModuleDetector md(&input_modules, &length);
		Module::Detect(ZXTune::CreateLocation(input_file), md);
		if(input_modules.empty())
		{
			input_file.reset();
		}
	}
	void close()
	{
		if(input_file)
		{
			input_player.reset();
			input_modules.clear();
			input_module.reset();
			input_file.reset();
		}
	}

	DWORD process(float* buffer, DWORD count)
	{
		assert(count%2 == 0);
		if(!input_player)
			return 0;
		std::vector<Sound::Sample> buf;
		buf.resize(count/2);
		size_t samples = input_player->RenderSound(&buf.front(), buf.size());
		float scale = 1.0f/(1 << (Sound::Sample::BITS - 1));
		auto end = buf.begin() + samples;
		for(auto i = buf.begin(); i != end; ++i)
		{
			*buffer++ = scale * i->Left();
			*buffer++ = scale * i->Right();
			count -= 2;
		}
		return samples*2;
	}
	bool seek(int subsong)
	{
		if(subsong >= 0 && subsong < (int)input_modules.size())
		{
			input_module = input_modules[subsong].module;
			input_player = PlayerWrapper::Create(input_module);
			if(!input_player)
			{
				input_module.reset();
				return false;
			}
			return true;
		}
		else
		{
			input_module.reset();
			input_player.reset();
			return false;
		}
	}
	bool seek(double seconds)
	{
		size_t s = (size_t)(seconds*raw_sample_rate);
		input_player->Seek(s);
		return true;
	}

	struct ModuleDesc
	{
		ModuleDesc() : length(0.0), bitrate(0) {}
		Module::Holder::Ptr module;
		double start;
		double length;
		int bitrate;
		std::string subname;
		std::string codec;
		std::string author;
		std::string title;
		std::string comment;
		std::string program;
	};

	const ModuleDesc& current_desc() const
	{
		for(auto m = input_modules.begin(); m != input_modules.end(); ++m)
		{
			if(input_module == m->module)
			{
				return *m;
			}
		}
		assert(false);
		return *(const ModuleDesc*)NULL;
	}

public:
	xmp_zxtune() : length(0.0), single_subsong(false) {}
	~xmp_zxtune() { close(); }

	Binary::Container::Ptr	input_file;
	typedef std::vector<ModuleDesc> Modules;
	Modules					input_modules;
	Module::Holder::Ptr		input_module;
	PlayerWrapper::Ptr		input_player;
	double length;
	bool single_subsong;
};

static xmp_zxtune* zxtune = NULL;

static BOOL WINAPI ZXTUNE_CheckFile(const char* filename, XMPFILE file)
{
	const char* fileext = strrchr(filename, '.');
	if(!fileext)
		return false;
	++fileext;
	for(const char** ft = file_types; *ft; ++ft)
	{
		if(_stricmp(fileext, *ft) == 0)
			return true;
	}
	return false;
}

// get the tags as a series of NULL-terminated names and values
static const std::string AddTag(const std::string& name, const std::string& value)
{
	assert(!name.empty());
	std::string res;
	if(value.empty())
		return res;
	res += name;
	res += '\0';
	res += value;
	res += '\0';
	return res;
}
static std::string AddTags(const xmp_zxtune::ModuleDesc& m)
{
	std::string tags;
	tags += AddTag("filetype",	m.codec);
	tags += AddTag("title",		m.title);
	tags += AddTag("artist",	m.author);
	tags += AddTag("comment",	m.comment);
	tags += '\0';
	return tags;
}
static std::string AddTags(xmp_zxtune* zxtune)
{
	std::string tags;
	for(auto m = zxtune->input_modules.begin(); m != zxtune->input_modules.end(); ++m)
	{
		tags += AddTags(*m);
	}
	assert(tags.length());
	return tags;
}
static char* xmp_string(const std::string& str)
{
	char* t = (char*)xmpfmisc->Alloc(str.length());
	if(!t)
		return NULL;
	memcpy(t, str.data(), str.length());
	return t;
}

// get file info
// return: the number of subsongs
static DWORD WINAPI ZXTUNE_GetFileInfo(const char*, XMPFILE file, float **length, char **tags)
{
	xmp_zxtune* zxtune = new xmp_zxtune;
	DWORD subsongs = 0;
	if(zxtune->open(NULL, file, true))
	{
		subsongs = zxtune->input_modules.size();
		assert(subsongs > 0);
		if(length)
		{
			float* l = (float*)xmpfmisc->Alloc(sizeof(float)*subsongs); // allocate array for length(s)
			*length = l;
			for(auto m = zxtune->input_modules.begin(); m != zxtune->input_modules.end(); ++m)
			{
				*l++ = (float)m->length;
			}
		}
		if(tags)
		{
			*tags = xmp_string(AddTags(zxtune));
		}
	}
	delete zxtune;
	return subsongs;
}
static DWORD WINAPI ZXTUNE_GetSubSongs(float *length) // get number (and total length) of sub-songs (OPTIONAL)
{
	*length = (float)zxtune->length;
	return zxtune->input_modules.size();
}

// close the file
static void WINAPI ZXTUNE_Close()
{
	delete zxtune;
	zxtune = NULL;
}

static void UpdateTrackInfo()
{
	const xmp_zxtune::ModuleDesc& md = zxtune->current_desc();
	xmpfin->SetLength((float)md.length, TRUE); // set length
	xmpfin->UpdateTitle(NULL); // refresh
}

// open a file for playback
// return:  0=failed, 1=success, 2=success and XMPlay can close the file
static DWORD WINAPI ZXTUNE_Open(const char*, XMPFILE file)
{
	assert(!zxtune);
	zxtune = new xmp_zxtune;
	if(zxtune->open(NULL, file))
	{
		UpdateTrackInfo();
		return 2;
	}
	ZXTUNE_Close();
	return 0;
}

// get the tags
// return NULL to delay the title update when there are no tags (use UpdateTitle to request update when ready)
static char* WINAPI ZXTUNE_GetTags()
{
	return xmp_string(AddTags(zxtune->current_desc()));
}

// set the sample format (in=user chosen format, out=file format if different)
static void WINAPI ZXTUNE_SetFormat(XMPFORMAT* form)
{
	form->res = raw_bytes_per_sample;
	form->chan = raw_channels;
	form->rate = raw_sample_rate;
}

// get the main panel info text
static void WINAPI ZXTUNE_GetInfoText(char* format, char* length)
{
	if(format)
	{
		// format details...
		const xmp_zxtune::ModuleDesc& md = zxtune->current_desc();
		strcpy(format, md.title.c_str());
	}
}

static std::string seconds_to_string(double _time)
{
	int time = int(_time);
	int seconds = time % 60;
	int minutes = (time/60) % 60;
	int hours = time/(60*60);
	std::string res;
	if(hours > 0)
	{
		res += boost::lexical_cast<std::string>(hours);
		res += ":";
	}
	res += (boost::format("%02d:%02d") % minutes % seconds).str();
	return res;
}

// get text for "General" info window
// separate headings and values with a tab (\t), end each line with a carriage-return (\r)
static void WINAPI ZXTUNE_GetGeneralInfo(char* buf)
{
	const xmp_zxtune::ModuleDesc& md = zxtune->current_desc();
	if(!zxtune->single_subsong && zxtune->input_modules.size() > 1)
	{
		buf += sprintf(buf, "Subsongs\t%d\r", zxtune->input_modules.size());
	}
	if(!md.subname.empty())
		buf += sprintf(buf, "Subname\t%s\r", md.subname.c_str());
	if(!md.comment.empty())
		buf += sprintf(buf, "Comment\t%s\r", md.comment.c_str());
	if(!md.program.empty())
		buf += sprintf(buf, "Program\t%s\r", md.program.c_str());
	buf += sprintf(buf, "Format\t%s\r", md.codec.c_str());
	buf += sprintf(buf, "Bit rate\t%d bps\r", md.bitrate);
	buf += sprintf(buf, "Sample rate\t%d hz\r", raw_sample_rate);
	buf += sprintf(buf, "Channels\t%d\r", raw_channels);
	buf += sprintf(buf, "Resolution\t%u bit\r", raw_bits_per_sample);
}

// get text for "Message" info window
// separate tag names and values with a tab (\t), and end each line with a carriage-return (\r)
static void WINAPI ZXTUNE_GetMessage(char* buf)
{
	if(!zxtune->single_subsong && zxtune->input_modules.size() > 1)
	{
		buf += sprintf(buf, "Subsongs");
		for(auto m = zxtune->input_modules.begin(); m != zxtune->input_modules.end(); ++m)
		{
			buf += sprintf(buf, "\t%c%d. %s %s\r", m->module == zxtune->input_module ? '>' : ' ', m - zxtune->input_modules.begin() + 1, seconds_to_string(m->length).c_str(), m->title.c_str());
		}
	}
}

// get some sample data, always floating-point
// count=number of floats to write (not bytes or samples)
// return number of floats written. if it's less than requested, playback is ended...
// so wait for more if there is more to come (use CheckCancel function to check if user wants to cancel)
static DWORD WINAPI ZXTUNE_Process(float* buffer, DWORD count)
{
	return zxtune->process(buffer, count);
}

// Get the seeking granularity in seconds
static double WINAPI ZXTUNE_GetGranularity()
{
	return 0.001; // seek in millisecond units
}

// Seek to a position (in granularity units)
// return the new position in seconds (-1 = failed)
static double WINAPI ZXTUNE_SetPosition(DWORD pos)
{
	if(pos&XMPIN_POS_SUBSONG)
	{
		zxtune->single_subsong = (pos&XMPIN_POS_SUBSONG1) != 0;
		zxtune->seek(LOWORD(pos)); // seek to beginning of subsong
		UpdateTrackInfo();
		return 0.0;
	}
	double pos_sec = pos*ZXTUNE_GetGranularity();
	if(zxtune->seek(pos_sec))
		return pos_sec;
	return -1.0;
}

static const char* about_short = "ZXTune Player (rev.1)";
static const char* about_text = \
"ZXTune Player (C) 2008 - 2015 by Vitamin/CAIG.\n"
"based on r3215 feb 25 2015\n"
"XMPlay plugin by djdron (C) 2015.\n\n"

"Used source codes from:\n"
"AYEmul from S.Bulba\n"
"AYFly from Ander\n"
"xPlugins from elf/2\n"
"Pusher from Himik/ZxZ\n\n"

"Used 3rdparty libraries:\n"
"boost C++ library\n"
"zlib from Jean-loup Gailly and Mark Adler\n"
"z80ex from Boo-boo\n"
"lhasa from Simon Howard\n"
"libxmp from Claudio Matsuoka\n"
"libsidplayfp from Simon White, Antti Lankila and Leandro Nini\n"
"snes_spc from Shay Green\n";

static void WINAPI ZXTUNE_About(HWND win)
{
	MessageBox(win, about_text, about_short, MB_ICONINFORMATION | MB_OK);
}

static std::string ft_reg;
static struct tFT_Init
{
	tFT_Init()
	{
		ft_reg = "ZXTune";
		ft_reg += '\0';
		bool first = true;
		for(const char** ft = file_types; *ft; ++ft)
		{
			if(!first)
				ft_reg += "/";
			ft_reg += *ft;
			first = false;
		}
	}
} ft_init;

// plugin interface
static XMPIN xmpin =
{
	0, // only using XMPFILE
	about_short,
	ft_reg.c_str(),
	ZXTUNE_About,
	NULL,
	ZXTUNE_CheckFile,
	ZXTUNE_GetFileInfo,
	ZXTUNE_Open,
	ZXTUNE_Close,
	NULL,
	ZXTUNE_SetFormat,
	ZXTUNE_GetTags,
	ZXTUNE_GetInfoText,
	ZXTUNE_GetGeneralInfo,
	ZXTUNE_GetMessage,
	ZXTUNE_SetPosition,
	ZXTUNE_GetGranularity,
	NULL, // GetBuffering only applies when using your own file routines
	ZXTUNE_Process,
	NULL, // WriteFile, see GetBuffering
	NULL, // don't have any "Samples"
	ZXTUNE_GetSubSongs,
	NULL,
	NULL, // GetDownloaded, see GetBuffering
	// no built-in vis
};

// get the plugin's XMPIN interface
XMPIN* WINAPI XMPIN_GetInterface(DWORD face, InterfaceProc faceproc)
{
	if (face!=XMPIN_FACE) { // unsupported version
		static int shownerror=0;
		if (face<XMPIN_FACE && !shownerror) {
			MessageBox(0,"The XMP-ZXTune plugin requires XMPlay 3.8 or above",0,MB_ICONEXCLAMATION);
			shownerror=1;
		}
		return NULL;
	}
	xmpfin=(XMPFUNC_IN*)faceproc(XMPFUNC_IN_FACE);
	xmpfmisc=(XMPFUNC_MISC*)faceproc(XMPFUNC_MISC_FACE);
	xmpffile=(XMPFUNC_FILE*)faceproc(XMPFUNC_FILE_FACE);
	xmpftext=(XMPFUNC_TEXT*)faceproc(XMPFUNC_TEXT_FACE);
	return &xmpin;
}

BOOL WINAPI DllMain(HINSTANCE hDLL, DWORD reason, LPVOID reserved)
{
	switch (reason) {
		case DLL_PROCESS_ATTACH:
			DisableThreadLibraryCalls(hDLL);
			break;
	}
	return TRUE;
}
