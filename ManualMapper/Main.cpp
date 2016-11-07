#include "Console.hpp"
#include "Utils.hpp"
#include "Mapper.hpp"

#include <Shlwapi.h>
#pragma comment( lib, "Shlwapi.lib" )

int main( )
{
	SetConsoleTitleA( "Manual Mapper" );

	std::string process, dll;
	unsigned long pid;

	Console::GreenText( "> Process: " );
	std::cin >> process;

	if ( !Utils::IsProcessActive( process ) )
	{
		Console::Error( "Process not found!\n" );
		Sleep( 1500 );
		TerminateProcess( GetCurrentProcess( ), 0 );
	}

	pid = Utils::GetProcessID( process );
	Console::Information( "Process '" ) << process << "' with PID '" << pid << "'\n\n";

	Console::GreenText( "> DLL: " );
	std::cin >> dll;

	if ( PathIsRelativeA( dll.c_str( ) ) )
	{
		Console::Information( "Library file path is relative to current folder?\n\n" );
		dll = Utils::ResolveRelativePath( dll );
	}

	if ( dll.rfind( ".dll" ) == std::string::npos )
		dll.append( ".dll" );

	if ( !Utils::FileExists( dll ) )
		Console::Error( "Specified file path does not relate to an existant library\n" );

	Mapper mapper( pid, dll );
	mapper.MapDLL( );
}