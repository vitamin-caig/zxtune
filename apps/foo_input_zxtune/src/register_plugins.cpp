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

#include <core/plugins/containers/plugins.h>
#include <core/plugins/containers/container_supp_common.h>
#include <core/plugin_attrs.h>
#include <formats/archived/decoders.h>

namespace ZXTune
{

void RegisterContainerPlugins(ArchivePluginsRegistrator& r)
{
	using namespace Formats::Archived;
	//process raw container first
	RegisterRawContainer(r);

//	RegisterArchiveContainers(registrator);
//	removed rar, zip, lha archives compared to standard RegisterArchiveContainers()
	r.RegisterPlugin(CreateContainerPlugin("TRD",     CAP_STOR_MULTITRACK | CAP_STOR_PLAIN,	CreateTRDDecoder()));
	r.RegisterPlugin(CreateContainerPlugin("SCL",     CAP_STOR_MULTITRACK | CAP_STOR_PLAIN,	CreateSCLDecoder()));
	r.RegisterPlugin(CreateContainerPlugin("HRIP",    CAP_STOR_MULTITRACK,	CreateHripDecoder()));
	r.RegisterPlugin(CreateContainerPlugin("ZXZIP",   CAP_STOR_MULTITRACK,	CreateZXZipDecoder()));
	r.RegisterPlugin(CreateContainerPlugin("ZXSTATE", CAP_STOR_MULTITRACK,	CreateZXStateDecoder()));

	//process containers last
	RegisterAyContainer(r);
	RegisterSidContainer(r);
//	RegisterZdataContainer(r);
}

}
//namespace ZXTune
