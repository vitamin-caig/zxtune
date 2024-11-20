#include "blargg_common.h"

class Archive_Reader {
protected:
	int count_;
	long size_;
public:
	Archive_Reader() : count_( 0 ), size_( 0L ) { }
	int count() const { return count_; }
	long size() const { return size_; }
public:
	virtual blargg_err_t open( const char* path, bool skip = false ) = 0;
	virtual blargg_err_t read( void* ) = 0;

	virtual const char* entry_name() const = 0;
	virtual long entry_size() const = 0;
	virtual bool next_entry() = 0;
	virtual void close() { }
	virtual ~Archive_Reader() { }
};

#ifdef RARDLL

#ifndef _WIN32
# define PASCAL
# define CALLBACK
# define UINT unsigned int
# define LONG long
# define HANDLE void *
# define LPARAM intptr_t
#else
# ifndef WIN32_LEAN_AND_MEAN
#  define WIN32_LEAN_AND_MEAN
# endif
# include <windows.h>
#endif

#if defined RAR_HDR_UNRAR_H
# include <unrar.h>
#elif defined RAR_HDR_DLL_HPP
# include <dll.hpp>
#endif

#ifndef ERAR_SUCCESS
# define ERAR_SUCCESS 0
#endif

class Rar_Reader : public Archive_Reader {
	RARHeaderData head;
	void* rar = nullptr;
	void* bp = nullptr;
	blargg_err_t restart( RAROpenArchiveData* );
public:
	blargg_err_t open( const char* path, bool skip );
	blargg_err_t read( void* );

	const char* entry_name() const { return head.FileName; }
	long entry_size() const { return head.UnpSize; }
	bool next_entry() { return RARReadHeader( rar, &head ) == ERAR_SUCCESS; }
	void close() { RARCloseArchive( rar ); rar = nullptr; }
	~Rar_Reader() { close(); }
};

#endif // RARDLL
