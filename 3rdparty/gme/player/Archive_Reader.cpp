#include "Archive_Reader.h"

#ifdef RARDLL

#include <string.h>

static int CALLBACK call_rar( UINT msg, LPARAM UserData, LPARAM P1, LPARAM P2 )
{
	uint8_t **bp = (uint8_t **)UserData;
	uint8_t *addr = (uint8_t *)P1;
	memcpy( *bp, addr, P2 );
	*bp += P2;
	(void) msg;
	return 0;
}

blargg_err_t Rar_Reader::restart( RAROpenArchiveData* data )
{
	if ( rar )
		close();
	rar = RAROpenArchive( data );
	if ( !rar )
		return "Failed to instantiate RAR handle";
	RARSetCallback( rar, call_rar, (LPARAM)&bp );
	return nullptr;
}

blargg_err_t Rar_Reader::open( const char* path, bool skip )
{
	blargg_err_t err;
	RAROpenArchiveData data;
	memset( &data, 0, sizeof data );
	data.ArcName = (char *)path;

	// determine space needed for the unpacked size and file count.
	data.OpenMode = RAR_OM_LIST;
	if ( (err = restart( &data )) )
		return err;
	while ( RARReadHeader( rar, &head ) == ERAR_SUCCESS )
	{
		RARProcessFile( rar, RAR_SKIP, nullptr, nullptr );
		count_++, size_ += head.UnpSize;
	}

	// prepare for extraction
	data.OpenMode = skip ? RAR_OM_LIST : RAR_OM_EXTRACT;
	return restart( &data );
}

blargg_err_t Rar_Reader::read( void* p )
{
	bp = p;
	if ( RARProcessFile( rar, -1, nullptr, nullptr ) != ERAR_SUCCESS )
		return "Error processing RAR file";
	return nullptr;
}

#endif // RARDLL
