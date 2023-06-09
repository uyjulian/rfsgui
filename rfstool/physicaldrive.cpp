#include "precomp.h"
#include "physicaldrive.h"
#include "assert.h"
#include "reiserfs.h"
#ifdef _WIN32
#include "pdrive95.h"
#include "pdrivent.h"
#else
#include "pdriveposix.h"
#endif

#ifdef _WIN32
typedef enum
{
	IS_WINDOWS_NT,
	IS_WINDOWS_2000,
	IS_WINDOWS_XP,
	IS_WINDOWS_95,
	IS_WINDOWS_98,
	IS_WINDOWS_98SE,
	IS_WINDOWS_ME,
	IS_WINDOWS_UNKNOWN,
} WINDOWS_VERSION;

WINDOWS_VERSION RefreshWindowsVersion()
{
	OSVERSIONINFOEX osvi;
	BOOL bOsVersionInfoEx;

	memset(&osvi, 0, sizeof(OSVERSIONINFOEX));
	osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
	if( !(bOsVersionInfoEx = GetVersionEx( (OSVERSIONINFO*) &osvi)) )
	{
		osvi.dwOSVersionInfoSize = sizeof (OSVERSIONINFO);
		if( !GetVersionEx( (OSVERSIONINFO*) &osvi) ) 
			return IS_WINDOWS_UNKNOWN;
	}
	switch (osvi.dwPlatformId)
	{
	case VER_PLATFORM_WIN32_NT:
		if( osvi.dwMajorVersion <= 4 )
			return IS_WINDOWS_NT;

		if( osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 0 )
			return IS_WINDOWS_2000;

		 if ( osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 1 )
			return IS_WINDOWS_XP;

		return IS_WINDOWS_NT;

	case VER_PLATFORM_WIN32_WINDOWS:
		if(osvi.dwMajorVersion == 4 && osvi.dwMinorVersion == 0)
			return IS_WINDOWS_95;

		if(osvi.dwMajorVersion == 4 && osvi.dwMinorVersion == 10)
		{
			if( osvi.szCSDVersion[1] == 'A' )
				return IS_WINDOWS_98SE;
			return IS_WINDOWS_98;
		}
		if (osvi.dwMajorVersion == 4 && osvi.dwMinorVersion == 90)
			return IS_WINDOWS_ME;

		return IS_WINDOWS_95;
	}
	return IS_WINDOWS_UNKNOWN;
}

static WINDOWS_VERSION wv;

IPhysicalDrive* CreatePhysicalDriveInstance()
{
	wv = RefreshWindowsVersion();
	if( (wv == IS_WINDOWS_NT) || (wv == IS_WINDOWS_2000) || (wv == IS_WINDOWS_XP) )
	{
		return new PNtPhysicalDrive();
	}
	else 
	{
		return new P9xPhysicalDrive();
	}
}
#else
IPhysicalDrive* CreatePhysicalDriveInstance()
{
	return new PPosixPhysicalDrive();
}
#endif


void IPhysicalDrive::DumpDriveInfo( LPCSTR lpszDrive )
{
    DISK_GEOMETRY dg;
    BYTE bLayout[20240];

	printf("\n----- Info on %s ------\n\n", lpszDrive );

	DWORD BytesPerSector = 512;
    
#ifdef SUPPORT_WINDOWS_XP_PARTITIONS
    if( GetDriveGeometryEx( (DISK_GEOMETRY_EX*) bLayout, sizeof(bLayout) ) )
    {
        DISK_GEOMETRY_EX* pDG = (DISK_GEOMETRY_EX*) bLayout;
		printf("Drive uses XP-style DISK_GEOMETRY_EX:\n" );
		printf("    Cylinders = %" FMT_QWORD "\n",  pDG->Geometry.Cylinders.QuadPart );
		printf("    MediaType = %hd\n", (USHORT) pDG->Geometry.MediaType );
		printf("    TracksPerCylinder = %hd\n", pDG->Geometry.TracksPerCylinder );
		printf("    SectorsPerTrack = %hd\n", pDG->Geometry.SectorsPerTrack );
		printf("    BytesPerSector = %hd\n", pDG->Geometry.BytesPerSector );
		BytesPerSector = pDG->Geometry.BytesPerSector;
		printf("    DiskSize = %" FMT_QWORD "\n", pDG->DiskSize.QuadPart );
    }
    else
#endif
    if( GetDriveGeometry(&dg) )
    {
		printf("Drive uses DISK_GEOMETRY:\n" );
		printf("    Cylinders = %" FMT_QWORD "\n",  dg.Cylinders.QuadPart );
		printf("    MediaType = %hd\n", (USHORT) dg.MediaType );
		printf("    TracksPerCylinder = %hd\n", dg.TracksPerCylinder );
		printf("    SectorsPerTrack = %hd\n", dg.SectorsPerTrack );
		printf("    BytesPerSector = %hd\n", dg.BytesPerSector );
		INT64 DiskSize = dg.BytesPerSector;
		DiskSize *= dg.SectorsPerTrack;
		DiskSize *= dg.TracksPerCylinder;
		DiskSize *= dg.Cylinders.QuadPart;
		BytesPerSector = dg.BytesPerSector;
		printf("    DiskSize (calculated) = %" FMT_QWORD "\n", DiskSize );
    }
    else
    {
        printf("ERROR %s, unable to get drive geometry\n", (LPCSTR) GetLastErrorString() ); 
        return;
    }
	printf("\n");

    BYTE* bMemory = new BYTE[BytesPerSector*2];
	assert(bMemory != NULL);
	memset( bMemory, 0, BytesPerSector*2 );
    
    memset( bLayout, 0, sizeof(bLayout) );

#ifdef SUPPORT_WINDOWS_XP_PARTITIONS
	WCHAR wcGuid[256];
    if( GetDriveLayoutEx(bLayout, sizeof(bLayout)) )
    {
        PDRIVE_LAYOUT_INFORMATION_EX pLI = (PDRIVE_LAYOUT_INFORMATION_EX)bLayout;

		printf("Got DRIVE_LAYOUT_INFORMATION_EX\n\n" );
		printf("    PartitionCount = %d\n", pLI->PartitionCount );

		if( pLI->PartitionStyle == PARTITION_STYLE_GPT )
		{
			printf("    PartitionStyle = %d (GPT)\n", pLI->PartitionStyle );

			StringFromGUID2(pLI->Gpt.DiskId, wcGuid, 256);
			printf("    DiskId = %ls\n",wcGuid );
			printf("    StartingUsableOffset = %" FMT_QWORD "\n", pLI->Gpt.StartingUsableOffset.QuadPart );
			printf("    UsableLength = %" FMT_QWORD "\n", pLI->Gpt.UsableLength.QuadPart );
			printf("    MaxPartitionCount = %d\n", pLI->Gpt.MaxPartitionCount );
		}
		else if( pLI->PartitionStyle == PARTITION_STYLE_MBR )
		{
			printf("    PartitionStyle = %d (MBR)\n", pLI->PartitionStyle );
			printf("    Signature = 0x%08lx\n", pLI->Mbr.Signature );
		}
		else
		{
			printf("    PartitionStyle = %d (RAW)\n", pLI->PartitionStyle );
		}

        for( unsigned long ulPartition = 0; ulPartition < pLI->PartitionCount; ulPartition++ )
        {
			PARTITION_INFORMATION_EX& p = pLI->PartitionEntry[ulPartition];	

			if( !p.StartingOffset.QuadPart && !p.PartitionLength.QuadPart )
				continue;

			printf("\n    --- PARTITION %d ---\n\n", ulPartition );

			printf("    PartitionStart = %" FMT_QWORD "\n", p.StartingOffset.QuadPart );
			printf("    PartitionLength = %" FMT_QWORD " (%.2f MB)\n", p.PartitionLength.QuadPart, PBytesInMBytes(p.PartitionLength.QuadPart) );

    		if( pLI->PartitionStyle == PARTITION_STYLE_GPT )
			{
				StringFromGUID2(p.Gpt.PartitionType, wcGuid, 256);
				printf("    PartitionType = %ls\n", wcGuid );
				StringFromGUID2(p.Gpt.PartitionId, wcGuid, 256);
				printf("    PartitionId = %ls\n", wcGuid );
				printf("    Attributes = %" FMT_QWORD "\n", (DWORD) p.Gpt.Attributes );
				printf("    Name = %ls\n", p.Gpt.Name );
			}
			else if( pLI->PartitionStyle == PARTITION_STYLE_MBR )
			{
				printf("    PartitionType = %d\n", (DWORD) p.Mbr.PartitionType );
			}

			INT64 diskoffset = (p.StartingOffset.QuadPart + REISERFS_DISK_OFFSET_IN_BYTES);
			if( !ReadAbsolute(bMemory, BytesPerSector, diskoffset ) )
			{
				printf("Warning %s, unable to read %d bytes from offset %" FMT_QWORD "\n", (LPCSTR) GetLastErrorString(), BytesPerSector, diskoffset ); 
			}
			else
			{
				printf("    Read %d bytes at offset %" FMT_QWORD "\n", (DWORD) BytesPerSector, diskoffset );
				LPREISERFS_SUPER_BLOCK p = (LPREISERFS_SUPER_BLOCK) bMemory;

				if( stricmp(p->s_magic,REISERFS_SUPER_MAGIC_STRING) )
				{
					if( stricmp(p->s_magic,REISER2FS_SUPER_MAGIC_STRING) )
					{
						printf("    Doesn't seem to be a ReiserFS partition (%08lx %08lx %08lx)\n",
							*(LPDWORD)&(p->s_magic[0]), *(LPDWORD)&(p->s_magic[4]), *(LPDWORD)&(p->s_magic[8]) ); 
					}
					else
					{
						printf( "    Is ReiserFS partition type 2 (%08lx %08lx %08lx)\n",
							*(LPDWORD)&(p->s_magic[0]), *(LPDWORD)&(p->s_magic[4]), *(LPDWORD)&(p->s_magic[8]) ); 
					}
				}
				else
				{
					printf( "    Is ReiserFS partition type 1 (%08lx %08lx %08lx)\n",
						*(LPDWORD)&(p->s_magic[0]), *(LPDWORD)&(p->s_magic[4]), *(LPDWORD)&(p->s_magic[8]) ); 
				}
			}

        }
    }
    else 
#endif
    if( GetDriveLayout(bLayout, sizeof(bLayout)) )
    {
        PDRIVE_LAYOUT_INFORMATION pLI = (PDRIVE_LAYOUT_INFORMATION)bLayout;

		printf("Got DRIVE_LAYOUT_INFORMATION\n\n" );
		printf("    PartitionCount = %d\n", pLI->PartitionCount );
		printf("    Signature = 0x%08lx\n", pLI->Signature );

        for( unsigned long ulPartition = 0; ulPartition < pLI->PartitionCount; ulPartition++ )
        {
			PARTITION_INFORMATION& p = pLI->PartitionEntry[ulPartition];

			if( !p.StartingOffset.QuadPart && !p.PartitionLength.QuadPart )
				continue;

			printf("\n    --- PARTITION %d ---\n\n", ulPartition );
			printf("    StartingOffset = %" FMT_QWORD "\n", p.StartingOffset.QuadPart );
			printf("    PartitionLength = %" FMT_QWORD " (%.2f MB)\n", p.PartitionLength.QuadPart, PBytesInMBytes(p.PartitionLength.QuadPart) );
			printf("    PartitionNumber = %ld\n", p.PartitionNumber );
			printf("    PartitionType = %ld\n", (long) p.PartitionType );

			INT64 diskoffset = (p.StartingOffset.QuadPart + REISERFS_DISK_OFFSET_IN_BYTES);
			if( !ReadAbsolute(bMemory, BytesPerSector, diskoffset ) )
			{
				printf("Warning %s, unable to read %d bytes from offset %" FMT_QWORD "\n", (LPCSTR) GetLastErrorString(), BytesPerSector, diskoffset ); 
			}
			else
			{
				diskoffset /= BytesPerSector;

				printf("    Read %d bytes at offset %" FMT_QWORD "\n", (DWORD) BytesPerSector, diskoffset );
				LPREISERFS_SUPER_BLOCK p = (LPREISERFS_SUPER_BLOCK) bMemory;

				if( stricmp(p->s_magic,REISERFS_SUPER_MAGIC_STRING) )
				{
					if( stricmp(p->s_magic,REISER2FS_SUPER_MAGIC_STRING) )
					{
						printf("    Doesn't seem to be a ReiserFS partition (%08lx %08lx %08lx)\n",
							*(LPDWORD)&(p->s_magic[0]), *(LPDWORD)&(p->s_magic[4]), *(LPDWORD)&(p->s_magic[8]) ); 
					}
					else
					{
						printf( "    Is ReiserFS partition type 2 (%08lx %08lx %08lx)\n",
							*(LPDWORD)&(p->s_magic[0]), *(LPDWORD)&(p->s_magic[4]), *(LPDWORD)&(p->s_magic[8]) ); 
					}
				}
				else
				{
					printf( "    Is ReiserFS partition type 1 (%08lx %08lx %08lx)\n",
						*(LPDWORD)&(p->s_magic[0]), *(LPDWORD)&(p->s_magic[4]), *(LPDWORD)&(p->s_magic[8]) ); 
				}
			}

        }


    }
    else
    {
        printf("ERROR %s, unable to get drive layout\n", (LPCSTR) GetLastErrorString() ); 
        return;
    }
}
