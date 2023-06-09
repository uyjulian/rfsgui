#ifndef pdrive95_H
#define pdrive95_H

#include "ntdiskspec.h"

#define PARTITION_TYPE_EXTENDED 5

typedef struct {
   BYTE DH; // Bit 7 is the active partition flag, bits 6-0 are zero.
   BYTE StartS;	// Starting CHS in INT 13 call format.
   BYTE StartH;
   BYTE StartC;
   BYTE Type; // Partition type byte.
   BYTE StopS;	// Starting CHS in INT 13 call format.
   BYTE StopH;
   BYTE StopC;
   DWORD LBA; // Starting LBA.
   DWORD SIZE; //  Size in sectors.
} PARTITIONINFO, *LPPARTITIONINFO;

typedef struct
{
	PARTITIONINFO pi[4];
	WORD wSignature;
} MBR;


class P9xPartitionInfo : public PNode
	{
	public:
		P9xPartitionInfo(PARTITION_INFORMATION* pi)
			:	m_pi(*pi)
		{
		}

		PARTITION_INFORMATION m_pi;
	};

#ifdef _WIN32
#include "..\RAWIO32\RAWIO32.h"
#endif
#include "physicaldrive.h"

class P9xPhysicalDrive : public IPhysicalDrive
    {
    public:
        P9xPhysicalDrive();
        virtual ~P9xPhysicalDrive();

        // path must look like this: "\\.\physicaldrive0" (of course, \ maps to \\, and \\ to \\\\)
        virtual BOOL Open( int iDrive );
		virtual void Close();
        virtual BOOL GetDriveGeometry( DISK_GEOMETRY* lpDG );
        virtual BOOL GetDriveLayout( LPBYTE lpbMemory, DWORD dwSize );
        virtual BOOL ReadAbsolute( LPBYTE lpbMemory, DWORD dwSize, INT64 OffsetInBytes );

		virtual BOOL ReadPartitionInfoRecursive(DWORD dwSector,INT64 TotalOffset);
    
#ifdef SUPPORT_WINDOWS_XP_PARTITIONS
        // shit for Windows XP
        virtual BOOL GetDriveGeometryEx( DISK_GEOMETRY_EX* lpDG, DWORD dwSize );
        virtual BOOL GetDriveLayoutEx( LPBYTE lpbMemory, DWORD dwSize );
#endif
#ifdef _WIN32
        HANDLE m_hDevice;
#endif    
		BYTE m_bDriveNumber;
		PList m_PartitionInfo;
    };

#endif // pdrive95_H
