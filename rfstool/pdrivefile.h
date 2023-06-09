#ifndef PSimulatedDriveFromBackupFile_H
#define PSimulatedDriveFromBackupFile_H

#include "pdrive95.h"
#include "physicaldrive.h"
#include "reiserfs.h"

class PSimulatedDriveFromBackupFile : public IPhysicalDrive
    {
    public:
        PSimulatedDriveFromBackupFile(LPCSTR lpszFilename);
        virtual ~PSimulatedDriveFromBackupFile();
        virtual BOOL Open( int iDrive );
		virtual void Close();
        virtual BOOL GetDriveGeometry( DISK_GEOMETRY* lpDG );
        virtual BOOL GetDriveLayout( LPBYTE lpbMemory, DWORD dwSize );
        virtual BOOL ReadAbsolute( LPBYTE lpbMemory, DWORD dwSize, INT64 OffsetInBytes );

#ifdef SUPPORT_WINDOWS_XP_PARTITIONS
        // shit for Windows XP
        virtual BOOL GetDriveGeometryEx( DISK_GEOMETRY_EX* lpDG, DWORD dwSize );
        virtual BOOL GetDriveLayoutEx( LPBYTE lpbMemory, DWORD dwSize );
#endif
        PString m_strFilename;
        ReiserFsMetafile m_Metafile;
    };

#endif // PSimulatedDriveFromBackupFile_H
