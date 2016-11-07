#include "Mapper.hpp"

Mapper::Mapper( unsigned long process_id, const std::string library_path )
	: m_PID( process_id ), m_LibraryPath( library_path )
{

}

void ExecuteMe( info* pinfo )
{
	IMAGE_DOS_HEADER* dos_header = reinterpret_cast< IMAGE_DOS_HEADER* >( pinfo->imagebase );
	IMAGE_NT_HEADERS* nt_header = reinterpret_cast< IMAGE_NT_HEADERS* >( pinfo->imagebase + dos_header->e_lfanew );
	IMAGE_BASE_RELOCATION* base_reloc = reinterpret_cast< IMAGE_BASE_RELOCATION* >( pinfo->imagebase + nt_header->OptionalHeader.DataDirectory[ IMAGE_DIRECTORY_ENTRY_BASERELOC ].VirtualAddress );

	unsigned long diff = reinterpret_cast< unsigned long >( pinfo->imagebase ) - nt_header->OptionalHeader.ImageBase;

	if ( diff != 0 )
	{
		while ( base_reloc->VirtualAddress && base_reloc->SizeOfBlock > sizeof( IMAGE_BASE_RELOCATION ) )
		{
			uint16_t* relocation_offsets = reinterpret_cast< uint16_t* >( base_reloc + 1 );

			for ( unsigned int i = 0; i < ( base_reloc->SizeOfBlock - sizeof( IMAGE_BASE_RELOCATION ) ) / sizeof( uint16_t ); i++ )
			{
				uintptr_t relocation_ptr = ( unsigned long )pinfo->imagebase + base_reloc->VirtualAddress + ( relocation_offsets[ i ] & 0xFFF );
				*reinterpret_cast< uintptr_t* >( relocation_ptr ) += diff;
			}

			base_reloc += base_reloc->SizeOfBlock;
		}
	}

	IMAGE_IMPORT_DESCRIPTOR* import = reinterpret_cast< IMAGE_IMPORT_DESCRIPTOR* >( pinfo->imagebase + nt_header->OptionalHeader.DataDirectory[ IMAGE_DIRECTORY_ENTRY_IMPORT ].VirtualAddress );
	while ( import->Characteristics )
	{
		const char* dll = reinterpret_cast< const char* >( pinfo->imagebase + import->Name );

		IMAGE_THUNK_DATA* import_lookup_table = reinterpret_cast< IMAGE_THUNK_DATA* >( pinfo->imagebase + import->OriginalFirstThunk );
		IMAGE_THUNK_DATA* iat = ( IMAGE_THUNK_DATA* )( pinfo->imagebase + import->FirstThunk );
		while ( import_lookup_table->u1.AddressOfData )
		{
			IMAGE_IMPORT_BY_NAME* import_by_name = reinterpret_cast< IMAGE_IMPORT_BY_NAME* >( pinfo->imagebase + import_lookup_table->u1.AddressOfData );
			uintptr_t iat_addr = Mapper::ResolveImport( dll, import_by_name->Name, pinfo );

			iat->u1.Function = iat_addr;

			import_lookup_table++;
			iat++;
		}


		import++;
	}
}

void ExecuteMeEnd( )
{

}

bool Mapper::MapDLL( )
{
	HANDLE process_handle = OpenProcess( PROCESS_ALL_ACCESS, FALSE, m_PID );

	if ( process_handle == NULL )
		return false;

	HANDLE dll_handle = CreateFileA( m_LibraryPath.c_str( ), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, 0, 0 );

	if ( dll_handle == INVALID_HANDLE_VALUE )
	{
		CloseHandle( process_handle );
		return false;
	}

	unsigned long dll_size = GetFileSize( dll_handle, nullptr );

	if ( dll_size <= 0 )
	{
		CloseHandle( process_handle );
		CloseHandle( dll_handle );
		return false;
	}

	byte* dll_image = reinterpret_cast< byte* >( VirtualAllocEx( process_handle, nullptr, dll_size, MEM_RESERVE | MEM_COMMIT, PAGE_EXECUTE_READWRITE ) );
	
	if ( dll_image == NULL )
	{
		VirtualFreeEx( process_handle, dll_image, 0, MEM_RELEASE );
		CloseHandle( process_handle );
		CloseHandle( dll_handle );
		return false;
	}

	byte* dll_file = new byte[ dll_size ];
	unsigned long bytes_read;

	if ( ReadFile( dll_handle, dll_file, dll_size, &bytes_read, nullptr ) == FALSE )
	{
		VirtualFreeEx( process_handle, dll_image, 0, MEM_RELEASE );
		CloseHandle( process_handle );
		CloseHandle( dll_handle );
		return false;
	}

	IMAGE_DOS_HEADER* dos_header = reinterpret_cast< IMAGE_DOS_HEADER* >( dll_file );
	IMAGE_NT_HEADERS* nt_header = reinterpret_cast< IMAGE_NT_HEADERS* >( dll_file + dos_header->e_lfanew );

	WriteProcessMemory( process_handle, dll_image, dll_file, nt_header->OptionalHeader.SizeOfHeaders, nullptr );

	IMAGE_SECTION_HEADER* section = reinterpret_cast< IMAGE_SECTION_HEADER* >( nt_header + 1 );
	for ( int i = 0; i < nt_header->FileHeader.NumberOfSections; i++ )
	{
		WriteProcessMemory( process_handle, dll_image + section->VirtualAddress, dll_file + section->PointerToRawData, section->SizeOfRawData, nullptr );
		section++;
	}

	byte* ptr = ( byte* )GetProcAddress( GetModuleHandleA( "kernel32.dll" ), "GetModuleHandleA" );

	unsigned long loader_size = reinterpret_cast< byte* >( ExecuteMeEnd ) - reinterpret_cast< byte* >( ExecuteMe );
	byte* loader = reinterpret_cast< byte* >( VirtualAllocEx( process_handle, nullptr, loader_size + sizeof( byte* ) * 2, MEM_RESERVE | MEM_COMMIT, PAGE_EXECUTE_READWRITE ) );
	WriteProcessMemory( process_handle, loader, ExecuteMe, loader_size, nullptr );
	WriteProcessMemory( process_handle, loader + loader_size, &dll_image, sizeof( byte* ), nullptr );
	WriteProcessMemory( process_handle, loader + loader_size + sizeof( byte* ), &ptr, sizeof( byte ), nullptr );

	CreateRemoteThread( process_handle, nullptr, 0, reinterpret_cast< LPTHREAD_START_ROUTINE >( loader ), loader + loader_size, 0, nullptr );

	CloseHandle( process_handle );
	CloseHandle( dll_handle );

	return true;
}

uintptr_t Mapper::ResolveImport( const std::string& exporting_dll, const std::string& exported_function_name, info* pinfo )
{
	IMAGE_DOS_HEADER* dos_header = ( IMAGE_DOS_HEADER* )pinfo->getmodulehandle( exporting_dll.c_str( ) );
	byte* imagebase = ( byte* )dos_header;

	IMAGE_NT_HEADERS* nt_header = ( IMAGE_NT_HEADERS* )( imagebase + dos_header->e_lfanew );
	IMAGE_EXPORT_DIRECTORY* export_directory = ( IMAGE_EXPORT_DIRECTORY* )( imagebase + nt_header->OptionalHeader.DataDirectory[ IMAGE_DIRECTORY_ENTRY_EXPORT ].VirtualAddress );

	uintptr_t* address_table = ( uintptr_t* )( imagebase + export_directory->AddressOfFunctions );
	uintptr_t* name_table = ( uintptr_t* )( imagebase + export_directory->AddressOfNames );
	uint16_t* ordinal_table = ( uint16_t* )( imagebase + export_directory->AddressOfNameOrdinals );

	size_t i;
	for ( i = 0; i < export_directory->NumberOfNames; i++ )
	{
		const char* cur_name = ( const char* )( imagebase + name_table[ i ] );
		if ( exported_function_name.compare( cur_name ) == 0 )
			break;
	}

	return address_table[ ordinal_table[ i ] ];
}
