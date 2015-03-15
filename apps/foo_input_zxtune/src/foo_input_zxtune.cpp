/*
ZXTune foobar2000 decoder component by djdron (C) 2013 - 2014

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

#define UNICODE
#include <foobar2000.h>
#undef UNICODE
#undef min

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

#include "player.h"

// Declaration of your component's version information
// Since foobar2000 v1.0 having at least one of these in your DLL is mandatory to let the troubleshooter tell different versions of your component apart.
// Note that it is possible to declare multiple components within one DLL, but it's strongly recommended to keep only one declaration per DLL.
// As for 1.1, the version numbers are used by the component update finder to find updates; for that to work, you must have ONLY ONE declaration per DLL. If there are multiple declarations, the component is assumed to be outdated and a version number of "0" is assumed, to overwrite the component with whatever is currently on the site assuming that it comes with proper version numbers.
DECLARE_COMPONENT_VERSION("ZX Tune Player", "0.0.4",
"ZX Tune Player (C) 2008 - 2015 by Vitamin/CAIG.\n"
"based on r3215 feb 25 2015\n"
"foobar2000 plugin by djdron (C) 2013 - 2015.\n\n"

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
"snes_spc from Shay Green\n"
);

// This will prevent users from renaming your component around (important for proper troubleshooter behaviors) or loading multiple instances of it.
VALIDATE_COMPONENT_FILENAME("foo_input_zxtune.dll");

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

static const char* ZXTUNE_SUBNAME = "ZXTUNE_SUBNAME";

enum
{
	raw_bits_per_sample = 16,
	raw_channels = 2,
	raw_sample_rate = 44100,

	raw_bytes_per_sample = raw_bits_per_sample / 8,
	raw_total_sample_width = raw_bytes_per_sample * raw_channels,
};

// No inheritance. Our methods get called over input framework templates. See input_singletrack_impl for descriptions of what each method does.
class input_zxtune
{
public:
	void open(service_ptr_t<file> p_filehint, const char * p_path,t_input_open_reason p_reason,abort_callback & p_abort) {
		if (p_reason == input_open_info_write)
			throw exception_io_unsupported_format();//our input does not support retagging.
		m_file_path = p_path;
		m_file = p_filehint;//p_filehint may be null, hence next line
		input_open_file_helper(m_file,p_path,p_reason,p_abort);//if m_file is null, opens file with appropriate privileges for our operation (read/write for writing tags, read-only otherwise).
		t_size size = (t_size)m_file->get_size(p_abort);
		m_buffer.set_size(size);
		m_file->read(m_buffer.get_ptr(), size, p_abort);
		input_file = Binary::CreateContainer(m_buffer.get_ptr(), size);
		if(!input_file)
			throw exception_io_unsupported_format();
		if(p_reason == input_open_info_read)
			ParseModules(p_abort);
	}
	void ParseModules(abort_callback& p_abort)
	{
		struct ModuleDetector : public Module::DetectCallback
		{
			ModuleDetector(Modules* _mods) : modules(_mods) {}
		    virtual void ProcessModule(ZXTune::DataLocation::Ptr location, ZXTune::Plugin::Ptr, Module::Holder::Ptr holder) const
			{
				ModuleDesc m;
				m.module = holder;
				m.subname = location->GetPath()->AsString();
				modules->push_back(m);
			}
			virtual Log::ProgressCallback* GetProgress() const { return NULL; }
			virtual Parameters::Accessor::Ptr GetPluginsParameters() const { return Parameters::Container::Create(); }
			Modules* modules;
		};

		ModuleDetector md(&input_modules);
		Module::Detect(ZXTune::CreateLocation(input_file), md);
		if(input_modules.empty())
		{
			input_file.reset();
			throw exception_io_unsupported_format();
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
	unsigned get_subsong_count() { return input_modules.size(); }
	t_uint32 get_subsong(unsigned p_index) { return p_index + 1; }

	std::string SubName(t_uint32 p_subsong)
	{
		metadb_handle_ptr h;
		meta_db->handle_create(h, playable_location_impl(m_file_path.c_str(), p_subsong));
		file_info_impl fi;
		h->get_info(fi);
		const char* subname = fi.meta_get(ZXTUNE_SUBNAME, 0);
		if(subname)
			return subname;
		return std::string();
	}

	double FrameDuration(Parameters::Accessor::Ptr props) const
	{
		Parameters::IntType frameDuration = Parameters::ZXTune::Sound::FRAMEDURATION_DEFAULT;
		props->FindValue(Parameters::ZXTune::Sound::FRAMEDURATION, frameDuration);
		return double(frameDuration) / Time::Microseconds::PER_SECOND;
	}

	void get_info(t_uint32 p_subsong, file_info & p_info,abort_callback & p_abort)
	{
		Module::Information::Ptr mi;
		Parameters::Accessor::Ptr props;
		std::string subname;
		if(!input_modules.empty())
		{
			if(p_subsong > input_modules.size())
				throw exception_io_unsupported_format();
			mi = input_modules[p_subsong - 1].module->GetModuleInformation();
			props = input_modules[p_subsong - 1].module->GetModuleProperties();
			if(!mi)
				throw exception_io_unsupported_format();
			subname = input_modules[p_subsong - 1].subname;
			if(!subname.empty())
				p_info.meta_set(ZXTUNE_SUBNAME, subname.c_str());
		}
		else
		{
			subname = SubName(p_subsong);
			Module::Holder::Ptr m = Module::Open(ZXTune::OpenLocation(Parameters::Container::Create(), input_file, subname));
			if(!m)
				throw exception_io_unsupported_format();
			mi = m->GetModuleInformation();
			props = m->GetModuleProperties();
			if(!mi)
				throw exception_io_unsupported_format();
		}

		double len = mi->FramesCount() * FrameDuration(props);
		p_info.set_length(len);
		Parameters::IntType size;
		if(props->FindValue(Module::ATTR_SIZE, size))
			p_info.info_calculate_bitrate(size, len);

		//note that the values below should be based on contents of the file itself, NOT on user-configurable variables for an example. To report info that changes independently from file contents, use get_dynamic_info/get_dynamic_info_track instead.
		p_info.info_set_int("samplerate", raw_sample_rate);
		p_info.info_set_int("channels", raw_channels);
		p_info.info_set_int("bitspersample", raw_bits_per_sample);
		p_info.info_set("encoding", "synthesized");
		String type;
		if(props->FindValue(Module::ATTR_TYPE, type))
			p_info.info_set("codec", type.c_str());
		String author;
		if(props->FindValue(Module::ATTR_AUTHOR, author) && !author.empty())
			p_info.meta_set("ARTIST", author.c_str());
		String title;
		if(props->FindValue(Module::ATTR_TITLE, title) && !title.empty())
			p_info.meta_set("TITLE", title.c_str());
		else if(subname.length())
			p_info.meta_set("TITLE", subname.c_str());
		String comment;
		if(props->FindValue(Module::ATTR_COMMENT, comment) && !comment.empty())
			p_info.meta_set("COMMENT", comment.c_str());
		String program;
		if(props->FindValue(Module::ATTR_PROGRAM, program) && !program.empty())
			p_info.meta_set("PERFORMER", program.c_str());
		if(input_modules.size() > 1)
		{
			p_info.meta_set("TRACKNUMBER", pfc::format_uint(p_subsong));
			p_info.meta_set("TOTALTRACKS", pfc::format_uint(input_modules.size()));
		}
	}
	t_filestats get_file_stats(abort_callback & p_abort) {return m_file->get_stats(p_abort);}

	void decode_initialize(t_uint32 p_subsong, unsigned p_flags, abort_callback & p_abort)
	{
		std::string subname = SubName(p_subsong);
		input_module = Module::Open(ZXTune::OpenLocation(Parameters::Container::Create(), input_file, subname));
		if(!input_module)
			throw exception_io_unsupported_format(); 

		Module::Information::Ptr mi = input_module->GetModuleInformation();
		if(!mi)
			throw exception_io_unsupported_format(); 
		input_player = PlayerWrapper::Create(input_module);
		if(!input_player)
			throw exception_io_unsupported_format(); 
	}
	bool decode_run(audio_chunk & p_chunk,abort_callback & p_abort)
	{
		enum { deltaread = 1024 };
		m_buffer.set_size(deltaread * raw_total_sample_width);
		t_size deltaread_done = input_player->RenderSound(reinterpret_cast<Sound::Sample*>(m_buffer.get_ptr()), deltaread);
		if(deltaread_done == 0) return false;//EOF
		p_chunk.set_data_fixedpoint(m_buffer.get_ptr(),deltaread_done * raw_total_sample_width,raw_sample_rate,raw_channels,raw_bits_per_sample,audio_chunk::g_guess_channel_config(raw_channels));
		return deltaread_done == deltaread; // EOF when deltaread_done != deltaread
	}
	void decode_seek(double p_seconds,abort_callback & p_abort)
	{
		// IMPORTANT: convert time to sample offset with proper rounding! audio_math::time_to_samples does this properly for you.
		t_uint64 s = audio_math::time_to_samples(p_seconds, raw_sample_rate);
		Module::Information::Ptr mi = input_module->GetModuleInformation();
		Parameters::Accessor::Ptr props = input_module->GetModuleProperties();
		t_uint64 max_s = audio_math::time_to_samples(mi->FramesCount() * FrameDuration(props), raw_sample_rate);
		if(s > max_s)
			s = max_s;
		input_player->Seek((size_t)s);
	}
	bool decode_can_seek() { return true; }
	bool decode_get_dynamic_info(file_info & p_out, double & p_timestamp_delta) { return false; } // deals with dynamic information such as VBR bitrates
	bool decode_get_dynamic_info_track(file_info & p_out, double & p_timestamp_delta) { return false; } // deals with dynamic information such as track changes in live streams
	void decode_on_idle(abort_callback & p_abort) {}

	void retag_set_info(t_uint32 p_subsong, const file_info & p_info,abort_callback & p_abort) { throw exception_io_unsupported_format(); }
	void retag_commit(abort_callback & p_abort) {}
	
	static bool g_is_our_content_type(const char * p_content_type) { return false; } // match against supported mime types here
	static bool g_is_our_path(const char * p_path,const char * p_extension)
	{
		for(const char** ft = file_types; *ft; ++ft)
		{
			if(stricmp_utf8(p_extension, *ft) == 0)
				return true;
		}
		return false;
	}

public:
	pfc::array_t<t_uint8> m_buffer;

	input_zxtune() {}
	~input_zxtune() { close(); }

	service_ptr_t<file>		m_file;
	std::string				m_file_path;
	Binary::Container::Ptr	input_file;

	struct ModuleDesc
	{
		Module::Holder::Ptr module;
		std::string subname;
	};
	typedef std::vector<ModuleDesc> Modules;
	Modules					input_modules;
	Module::Holder::Ptr		input_module;
	PlayerWrapper::Ptr		input_player;

	static_api_ptr_t<metadb> meta_db;
};

static input_factory_t<input_zxtune> g_input_zxtune_factory;

static std::string ft_reg;
static struct tFT_Init
{
	tFT_Init()
	{
		for(const char** ft = file_types; *ft; ++ft)
		{
			if(!ft_reg.empty())
				ft_reg += ";";
			ft_reg += "*.";
			ft_reg += *ft;
		}
	}
} ft_init;

DECLARE_FILE_TYPE("ZX Tune Audio Files", ft_reg.c_str());
