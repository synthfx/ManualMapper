#pragma once

#include <Windows.h>
#include <string>

using tGetModuleHandle = decltype( &GetModuleHandleA );

struct info
{
	byte* imagebase;
	tGetModuleHandle getmodulehandle;
};

class Mapper
{
public:
	Mapper( unsigned long process_id, const std::string library_path );
	bool MapDLL( );

public:
	static uintptr_t ResolveImport( const std::string& exporting_dll, const std::string& exported_function_name, info* pinfo );

private:
	unsigned long m_PID;
	std::string m_LibraryPath;
};

