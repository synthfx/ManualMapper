#include "Utils.hpp"

namespace Utils
{
	std::string ToMultiByte( const std::wstring& str )
	{
		int size = WideCharToMultiByte( CP_UTF8, 0, &str[ 0 ], str.size( ), nullptr, 0, nullptr, nullptr );
		std::string ret( size, 0 );
		WideCharToMultiByte( CP_UTF8, 0, &str[ 0 ], str.size( ), &ret[ 0 ], size, nullptr, nullptr );
		return ret;
	}

	std::string ResolveRelativePath( const std::string& dll )
	{
		char path[ MAX_PATH ];
		GetModuleFileNameA( GetModuleHandleA( NULL ), path, MAX_PATH );

		std::string str( path );
		return str.substr( 0, str.rfind( "\\" ) + 1 ) + dll;
	}

	bool IsProcessActive( std::string& process_name )
	{
		if ( process_name.rfind( ".exe" ) == std::string::npos )
			process_name.append( ".exe" );

		HANDLE snapshot = CreateToolhelp32Snapshot( TH32CS_SNAPPROCESS, 0 );

		if ( snapshot == INVALID_HANDLE_VALUE )
			return false;

		PROCESSENTRY32 pe32;
		pe32.dwSize = sizeof( PROCESSENTRY32 );

		if ( Process32First( snapshot, &pe32 ) )
		{
			std::string converted = ToMultiByte( pe32.szExeFile );

			if ( process_name.compare( converted ) == 0 )
			{
				CloseHandle( snapshot );
				return true;
			}
		}

		while ( Process32Next( snapshot, &pe32 ) )
		{
			std::string converted = ToMultiByte( pe32.szExeFile );

			if ( process_name.compare( converted ) == 0 )
			{
				CloseHandle( snapshot );
				return true;
			}
		}

		return false;
	}

	unsigned long GetProcessID( const std::string& process_name )
	{
		HANDLE snapshot = CreateToolhelp32Snapshot( TH32CS_SNAPPROCESS, 0 );

		if ( snapshot == INVALID_HANDLE_VALUE )
			return false;

		PROCESSENTRY32 pe32;
		pe32.dwSize = sizeof( PROCESSENTRY32 );

		if ( Process32First( snapshot, &pe32 ) )
		{
			std::string converted = ToMultiByte( pe32.szExeFile );

			if ( process_name.compare( converted ) == 0 )
			{
				CloseHandle( snapshot );
				return pe32.th32ProcessID;
			}
		}

		while ( Process32Next( snapshot, &pe32 ) )
		{
			std::string converted = ToMultiByte( pe32.szExeFile );

			if ( process_name.compare( converted ) == 0 )
			{
				CloseHandle( snapshot );
				return pe32.th32ProcessID;
			}
		}

		return false;
	}

	bool FileExists( const std::string& file_path )
	{
		FILE* file;
		if ( !fopen_s( &file, file_path.c_str( ), "r" ) )
			return true;
		else
			return false;
	}
}