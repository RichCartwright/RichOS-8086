// Filesystem Definitions

#ifndef _FSYS_H
#define _FSYS_H

#include <stdint.h>
//	File flags

#define FS_FILE       0
#define FS_DIRECTORY  1
#define FS_INVALID    2

// File
typedef struct _File 
{
	char        Name[256];
	uint32_t    Flags;
	uint32_t    FileLength;
	uint32_t    Id;
	uint32_t    Eof;
	uint32_t    Position;
	uint32_t    CurrentCluster;
} FILE;

typedef FILE * PFILE;

typedef struct _DirectoryEntry 
{
	uint8_t   Filename[8];
	uint8_t   Ext[3];
	uint8_t   Attrib;
	uint8_t   Reserved;
	uint8_t   TimeCreatedMs;
	uint16_t  TimeCreated;
	uint16_t  DateCreated;
	uint16_t  DateLastAccessed;
	uint16_t  FirstClusterHiBytes;
	uint16_t  LastModTime;
	uint16_t  LastModDate;
	uint16_t  FirstCluster;
	uint32_t  FileSize;

} __attribute__((packed)) DirectoryEntry;

typedef DirectoryEntry * pDirectoryEntry;

void 			FsFat12_Initialise();
FILE 			FsFat12_Open(const char* fileName);
FILE 			FsFat12_OpenDirectory(const char* fileName);
unsigned int 	FsFat12_Read(PFILE file, unsigned char* buffer, unsigned int length);
FILE 			FsFat12_OpenSubDirectory(FILE file, const char* fileName);
void 			FsFat12_Close(PFILE file);
void			FsFat12_Dir(const char* path);

#endif
