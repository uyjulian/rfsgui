#ifndef PNtPhysicalDrive_H
#define PNtPhysicalDrive_H

#include "ntdiskspec.h"

class IPhysicalDrive
    {
    public:
        virtual BOOL Open( int iDrive ) = 0;
		virtual void Close() = 0;
        virtual BOOL GetDriveGeometry( DISK_GEOMETRY* lpDG ) = 0;
        virtual BOOL GetDriveLayout( LPBYTE lpbMemory, DWORD dwSize ) = 0;
        virtual BOOL ReadAbsolute( LPBYTE lpbMemory, DWORD dwSize, INT64 Sector ) = 0;
		virtual void DumpDriveInfo( LPCSTR lpszDrive );

#ifdef SUPPORT_WINDOWS_XP_PARTITIONS
        // shit for Windows XP
        virtual BOOL GetDriveGeometryEx( DISK_GEOMETRY_EX* lpDG, DWORD dwSize ) = 0;
        virtual BOOL GetDriveLayoutEx( LPBYTE lpbMemory, DWORD dwSize ) = 0;
#endif
    };

IPhysicalDrive* CreatePhysicalDriveInstance();

#endif // PNtPhysicalDrive_H


