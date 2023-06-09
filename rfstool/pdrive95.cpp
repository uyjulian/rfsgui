#include "precomp.h"
#include "pdrive95.h"

// RAW functions
LPFNResetDisk m_ResetDisk = NULL;
LPFNReadPhysicalSector m_ReadPhysicalSector = NULL;
LPFNWritePhysicalSector m_WritePhysicalSector = NULL;
LPFNReadDiskGeometry m_ReadDiskGeometry = NULL;
LPFNEI13GetDriveParameters m_EI13GetDriveParameters = NULL;
LPFNEI13ReadSector m_EI13ReadSector = NULL;
LPFNEI13WriteSector m_EI13WriteSector = NULL;

P9xPhysicalDrive::P9xPhysicalDrive()
{
}

P9xPhysicalDrive::~P9xPhysicalDrive()
{
}

BOOL P9xPhysicalDrive::ReadPartitionInfoRecursive(DWORD dwSector,INT64 TotalOffset )
{
	BYTE mbr[512];
	INT64 OffsetInBytes = dwSector;
	OffsetInBytes *= 512;


	if( ReadAbsolute(mbr,sizeof(mbr),OffsetInBytes) )
	{
		MBR* pMBR = (MBR*) &(mbr[446]);
		if( pMBR->wSignature != 0xaa55 )
			return FALSE;

		INT64 ScheissOffset = 0;

		for( int i = 0; i < 4; i++ )
		{
			PARTITIONINFO* source = &(pMBR->pi[i]);

			if( !source->SIZE && !source->LBA )
				continue;

			PARTITION_INFORMATION pi;
			ZeroMemory(&pi,sizeof(pi));

			pi.PartitionLength.QuadPart = source->SIZE;
			pi.PartitionLength.QuadPart *= 512L;
			pi.PartitionType = source->Type;

			if( i == 0 )
			{
				pi.StartingOffset.QuadPart = source->LBA;
				pi.StartingOffset.QuadPart *= 512L;
				pi.StartingOffset.QuadPart += TotalOffset;
				ScheissOffset = pi.StartingOffset.QuadPart;
			}
			else
			{
				pi.StartingOffset.QuadPart = ScheissOffset;
			}
			ScheissOffset += pi.PartitionLength.QuadPart;

			P9xPartitionInfo* p9pi = new P9xPartitionInfo(&pi);
			m_PartitionInfo.AddTail( p9pi );
			if( pi.PartitionType == PARTITION_TYPE_EXTENDED )
			{
				if( !ReadPartitionInfoRecursive(dwSector + source->LBA,pi.StartingOffset.QuadPart) )
				{
					p9pi->m_pi.StartingOffset.QuadPart += 63*512;
				}
			}
			
		}
	}
	return TRUE;
}

BOOL P9xPhysicalDrive::Open( int iDrive )
{
	if( m_ResetDisk == NULL )
	{
		HINSTANCE hLibrary = (HINSTANCE)LoadLibrary( "RAWIO32.DLL" );
		if( !hLibrary )
		{
			printf("ERROR %s, unable to load RAWIO32.DLL.\n", (LPCSTR) GetLastErrorString() );
			return FALSE;
		}

	#define GETPROC(__NAME__) \
		m_##__NAME__ = (LPFN##__NAME__) GetProcAddress(hLibrary,#__NAME__); \
		if( !m_##__NAME__ ) { \
			printf("ERROR, missing export " #__NAME__ " IN DISKIO32.DLL\n" ); \
			return FALSE; \
		}

		GETPROC( ResetDisk )
		GETPROC( ReadPhysicalSector )
		GETPROC( WritePhysicalSector )
		GETPROC( ReadDiskGeometry )
		GETPROC( EI13GetDriveParameters )
		GETPROC( EI13ReadSector )
		GETPROC( EI13WriteSector )
	}
	
	//printf("About to get geometry\n");
	m_bDriveNumber = (BYTE)(128 + iDrive);

	DISK_GEOMETRY dg;
	if( GetDriveGeometry(&dg) )
	{
		//printf("Cylinders = %I64d\n", dg.Cylinders );
		//printf("TracksPerCylinder = %d\n", dg.TracksPerCylinder );
		//printf("SectorsPerTrack = %d\n", dg.SectorsPerTrack );
		//printf("BytesPerSector = %d\n", dg.BytesPerSector );

		INT64 TotalSize = dg.Cylinders.QuadPart;
		TotalSize *= dg.TracksPerCylinder;
		TotalSize *= dg.SectorsPerTrack;
		TotalSize *= dg.BytesPerSector;
		//printf( "Total Size In Bytes = %I64d\n", TotalSize );
		TotalSize /= 1024L;
		TotalSize /= 1024L;
		//printf( "Total Size In Megabytes = %I64d\n", TotalSize );

		ReadPartitionInfoRecursive(0,0);
		/*int index = 0;
		ENUMERATE( &m_PartitionInfo, P9xPartitionInfo, pI )
		{
			printf("PARTITION %d: --------------------\n", index++ );
			printf("    StartingOffset = %" FMT_QWORD "\n", pI->m_pi.StartingOffset.QuadPart );
			printf("    PartitionLength = %" FMT_QWORD " (%.2f MB)\n", pI->m_pi.PartitionLength.QuadPart, PBytesInMBytes(pI->m_pi.PartitionLength.QuadPart) );
			printf("    PartitionType = %ld\n", (long) pI->m_pi.PartitionType );	
		}*/
		return TRUE;
	}
	return FALSE;
}

void P9xPhysicalDrive::Close()
{
}

BOOL P9xPhysicalDrive::GetDriveGeometry( DISK_GEOMETRY* lpDG )
{
	lpDG->MediaType = Unknown;
	lpDG->BytesPerSector = 512;

	ExtDriveInfo edi;
	ZeroMemory(&edi,sizeof(edi));
	edi.drive = m_bDriveNumber;

	if( m_EI13GetDriveParameters(&edi) > 0 )
	{
		lpDG->Cylinders.QuadPart = *(INT64*)&(edi.sectorsLo);
		lpDG->TracksPerCylinder = 1; //edi.heads;
		lpDG->SectorsPerTrack = 1; //edi.cylinders;
		return TRUE;
	}

	SectorInfo si;
	ZeroMemory(&si,sizeof(si));
	si.bDrive = m_bDriveNumber;

	if( m_ReadDiskGeometry(&si) > 0 )
	{
		lpDG->Cylinders.QuadPart = si.wCylinder;
		lpDG->TracksPerCylinder = si.bHead;
		lpDG->SectorsPerTrack = si.bSector;
		return TRUE;
	}
	return FALSE;
}

BOOL P9xPhysicalDrive::ReadAbsolute( LPBYTE lpbMemory, DWORD dwSize, INT64 OffsetInBytes )
{
	BlockInfo bi;
	bi.drive = m_bDriveNumber;
	OffsetInBytes /= 512;
	*((INT64*)&(bi.scheiss[0])) = OffsetInBytes;
	bi.count = (WORD) (dwSize / 512);

	if( m_EI13ReadSector (&bi, lpbMemory, dwSize) > 0 )
		return TRUE;

// ***** NOT SUPPORTED *******
//struct SectorInfo
//{
//	 BYTE bDrive;
//	 WORD wCylinder;
//	 BYTE bHead;
//	 BYTE bSector;
//	 BYTE bCount;
//};
//
//	if( ReadPhysicalSector(&bi, lpbMemory, dwSize) > 0 )
//		return TRUE;

	return FALSE;
}

#ifdef SUPPORT_WINDOWS_XP_PARTITIONS
BOOL P9xPhysicalDrive::GetDriveGeometryEx( DISK_GEOMETRY_EX* lpDG, DWORD dwSize )
{
	return FALSE;
}

BOOL P9xPhysicalDrive::GetDriveLayoutEx( LPBYTE lpbMemory, DWORD dwSize )
{
	return FALSE;
}
#endif

BOOL P9xPhysicalDrive::GetDriveLayout( LPBYTE lpbMemory, DWORD dwSize )
{
	DWORD dwBytesRequired = sizeof(DRIVE_LAYOUT_INFORMATION) + sizeof(PARTITION_INFORMATION)*(m_PartitionInfo.m_lCount-1);
	
	if( dwSize < dwBytesRequired )
		return FALSE;

	PDRIVE_LAYOUT_INFORMATION pli = (PDRIVE_LAYOUT_INFORMATION) lpbMemory;
	pli->PartitionCount = m_PartitionInfo.m_lCount;
	pli->Signature = 0;
	int index = 0;
	ENUMERATE( &m_PartitionInfo, P9xPartitionInfo, pI )
	{
		pli->PartitionEntry[index++] = pI->m_pi;
	}
	return TRUE;
}


