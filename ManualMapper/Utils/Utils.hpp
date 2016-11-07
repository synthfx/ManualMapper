#pragma once

#include <Windows.h>
#include <TlHelp32.h>
#include <string>

namespace Utils
{
	std::string ToMultiByte( const std::wstring& str );
	std::string ResolveRelativePath( const std::string& path );

	bool IsProcessActive( std::string& process_name );
	unsigned long GetProcessID( const std::string& process_name );

	bool FileExists( const std::string& file );
}