#include <filesystem.h>
#include <console.h>
#include <bpb.h>
#include <floppydisk.h>
#include <string.h> 

uint32_t rootOffset;
uint32_t numOfDirs; 
uint16_t reservedSectors; 
uint32_t rootSize;

//Counters (for "dir")
unsigned int fileCount 	= 0; 
unsigned int dirCount 	= 0;
unsigned int byteCount 	= 0;	
	
void FsFat12_Initialise()
{
	//Saving relevant information concering root
	//Got help from here http://www.maverick-os.dk/FileSystemFormats/FAT16_FileSystem.html (at the bottom)
	pBootSector boot;	
	unsigned char* bootBuffer;	
	//memcpy(bootBuffer, (unsigned char*)FloppyDriveReadSector(0), sizeof(BootSector));	
	bootBuffer = (unsigned char*)FloppyDriveReadSector(0);
	boot = (pBootSector)bootBuffer; 
	
	numOfDirs 		= boot->Bpb.NumDirEntries;
	rootSize		= (boot->Bpb.NumDirEntries * 32) / boot->Bpb.BytesPerSector;
	reservedSectors = boot->Bpb.ReservedSectors;
	rootOffset		= (boot->Bpb.NumberOfFats * boot->Bpb.SectorsPerFat) + reservedSectors;
}

char* ConvertToDOSNaming(const char* fileName, unsigned char* buffer, unsigned int length)
{
	int counter = 0; 
	int dotPosition = 0;
	memset(buffer, ' ', 12);
	strcpy(buffer, fileName);
	
	//Catch the dots before they got murdered
	if(strncmp(buffer, ".", 1) == 0 || strncmp(buffer, "..", 2) == 0)
	{
		memset(buffer + strlen(buffer), ' ', 12 - strlen(buffer));
		return buffer; 
	}
	
	char* returnName = toupper(buffer);	
	char searchName[12];
	memset(searchName, 0, 12);
	
	while(counter <= 8)
	{
		if (returnName[counter] == '.' || returnName[counter] == '\0')
		{
			dotPosition = counter;
			counter++;
			break;
		}
		else
		{
			searchName[counter] = returnName[counter];
		}
		counter++;
	}	
	
	int extentionCount = 8;
	while(extentionCount < 11)  //Now we need to check for the 3 char file extention
	{
		if (returnName[counter] != '\0')
		{
			searchName[extentionCount] = returnName[counter];
		}
		else
		{
			searchName[extentionCount] = ' ';
		}
		counter++;
		extentionCount++;
	}
	
	counter = dotPosition;	
	while (counter < 8)
	{
		searchName[counter] = ' '; //bulk the spaces
		counter++;
	}
	
	strcpy(returnName, searchName);
	
	return returnName; 
}


FILE FsFat12_Open(const char* fileName)
{
	FILE currentDir;
	FILE nullFile;	//Create a false file just to return if there is some kind of error
	char* filePath = (char*) fileName;
	char* path = 0;
	nullFile.Flags = FS_INVALID; //Set the flag to invalid as explained in assignement brief

	//First we need to find out if we are look in root, this can be done by looking for backslashes '\\' 
	path = strchr(filePath, '\\'); //Returns a point of the first backslash (if any)
	
	if(path == NULL) //We dont need to go anywhere
	{		
		currentDir = FsFat12_OpenDirectory(filePath);
		
		if(currentDir.Flags == FS_FILE) //We only care about files if there are not backslashes
		{
			return currentDir; //We have found a file, that was easy
		}
		
		FILE nullFile;	//Create a false file just to return, returning NULL doesn't work because a FILE can't be '0'
		nullFile.Flags = FS_INVALID; //Set the flag to invalid as explained in assignement brief
		return nullFile; 
	}
	
	path++;
	int level = 0; //How many levels we are in - (ie. 0 = root, 1 = 1 dir in) 
	
	//Now we want to loop though the whole path dealing with sub-dirs and finally the normal dir (file in this case)
	while(path)
	{
		
		char nextEntry[11];
	
		int len = 0; 
		for (len = 0; len < 11; len++)
		{
			if(path[len] == '\\' || path[len] == '\0')
			{
				break; 
			}
			
			nextEntry[len] = path[len];
		}
		nextEntry[len] = '\0';
		
		if(level == 0) //We haven't traversed anything yet, we are still in root
		{				//So we need to deal with that first
			//We want to keep the position of the path pointer
			currentDir = FsFat12_OpenDirectory(nextEntry);
		}
		else //We're dealing with a sub-dir
		{
			currentDir = FsFat12_OpenSubDirectory(currentDir, nextEntry); //Pass in a reference and a name to find
		}

		if(currentDir.Flags == FS_INVALID)
		{
			//Same as above, make a false return
			return nullFile; 
		}
		if(currentDir.Flags == FS_FILE)
		{
			return currentDir; //WE DID IT
		}		
		//Then we can assume its another directory
		
		path = strchr(path+1, '\\'); //Check for another backslash
		if (path != NULL)
		{
			//There is one
			path++, level++; //we've gone up a level
		}
	}
	//No more backslashes and no return something has messed up or its a directory! 
	return currentDir; 
}

//We need to handle directories and subdirectories differently, so its easier just to make classes to save repeated code
FILE FsFat12_OpenDirectory(const char* fileName)
{	
	char fileNameConv[11];
	ConvertToDOSNaming(fileName, fileNameConv, 11);
	
	unsigned char* floppyBuffer; 
	FILE newFile;
	pDirectoryEntry dir;
	
	for (int sec = 0; sec < 14; sec++) //14 sectors
	{		
 		floppyBuffer = (unsigned char*)FloppyDriveReadSector(rootOffset + sec);
		dir = (pDirectoryEntry)floppyBuffer;
		for (int i = 0; i < 16; i++) //16 dirs per sector
		{
			char name[11];
			memcpy(name, dir->Filename, 11);
			name[11] = '\0';

			if(strncmp(name, fileNameConv, 11) == 0) // We got a hit!
			{
				strcpy(newFile.Name, fileName);
				
				newFile.CurrentCluster 	= dir->FirstCluster; 
				newFile.Position 		= 0; 
				newFile.Eof 			= 0; 
				newFile.FileLength 		= dir->FileSize;
				//File attribute masks found here & info for the loops - 
				//http://www.disc.ua.es/~gil/FAT12Description.pdf
				if (dir->Attrib == 0x10) 
				{
					newFile.Flags = FS_DIRECTORY; 
					return newFile; 

				}
				else
				{
					newFile.Flags = FS_FILE;
					return newFile; 
				}
				
				break; //If its not a file or directory for some reason, we dont want it. This is more of a sanity check
			}
			dir++;
		}
	}
	//Search Failed
	newFile.Flags = FS_INVALID;
	return newFile;
}

uint16_t bytesRead = 0;	

FILE FsFat12_OpenSubDirectory(FILE file, const char* fileName)
{		
	char fileNameConv[11];		
	ConvertToDOSNaming(fileName, fileNameConv, 11);	
	FILE newFile; 	
	
	while(file.Eof != 1) //Read the current dir
	{						
		unsigned char buffer[512]; //one sector is the most it can be			
		FsFat12_Read(&file, buffer, 512);
		pDirectoryEntry subDir = (pDirectoryEntry)buffer;	
		
		//Same as above except we dont need the sector loop anymore!
		for (int i = 0; i < 16; i++) //16 dirs per file
		{			
			char name[11];			
			memcpy(&name, &subDir->Filename, 11);

			if(strncmp(name, fileNameConv, 11) == 0) // We got a hit!
			{
 				if(subDir->Filename[0] == (char)0xE5)
				{	
					break;			
				}

				strcpy(newFile.Name, fileName);
				if(strncmp(name, "..          ", 11) == 0 && subDir->FirstCluster == 0)
				{
					newFile.CurrentCluster = rootOffset; 
				}
				else
				{
					newFile.CurrentCluster = subDir->FirstCluster;
				}

				newFile.Position = 0; 
				newFile.Eof = 0; 
				newFile.FileLength = subDir->FileSize;
				//File attribute masks found here & info for the loops - 
				//http://www.disc.ua.es/~gil/FAT12Description.pdf
				if (subDir->Attrib == 0x10) 
				{
					newFile.Flags = FS_DIRECTORY; 
					return newFile;
				}
				else
				{					
					newFile.Flags = FS_FILE;
					return newFile; 
				}
				
			}
			subDir++;
		}			
	} 
	newFile.Flags = FS_INVALID;
	return newFile; 
}


unsigned int FsFat12_Read(PFILE file, unsigned char* buffer, unsigned int length)
{	
	unsigned int totalBytesRead = 0;
	unsigned char FAT_TABLE[1024];

	if(file)
	{				
		//Ref:page 4 http://www.disc.ua.es/~gil/FAT12Description.pdf 		
		//Load the buffer with the first result
		memcpy(buffer, (unsigned char*) FloppyDriveReadSector((32 + file->CurrentCluster - 2) + reservedSectors), 512); //Give the buffer the full result for parsing
		//Got help with the variables and bit shifting from http://wiki.osdev.org/FAT && with help from the assembly file
		unsigned int offset = file->CurrentCluster + (file->CurrentCluster / 2); // * 1.5
		unsigned int fatSector = reservedSectors + (offset / 512);
		unsigned int entryOffset = offset % 512;
		
		//Load the FAT up 
		memcpy(FAT_TABLE, (unsigned char*)FloppyDriveReadSector(fatSector), 512); //Put the first result in the first half of the FAT
		memcpy(FAT_TABLE + 512, (unsigned char*)FloppyDriveReadSector(fatSector + 1), 512); //Now the second half of the FAT
		
		uint16_t nextCluster = *(uint16_t*)&FAT_TABLE[entryOffset];
		if (file->CurrentCluster & 0x0001)
		{
			nextCluster = nextCluster >> 4; //High 
		}
		else
		{
			nextCluster = nextCluster & 0x0FFF; //Low 
		}
		
		totalBytesRead = 64; 
		
		if (nextCluster >= 0xFF8) //between 0xff8 & 0xfff indicates last cluster in chain
		{
			FsFat12_Close(file);
			return totalBytesRead; 
		}
		
		if (nextCluster == 0x00) //Unused
		{
			FsFat12_Close(file);
			return totalBytesRead; 
		}
		
		memcpy(&file->CurrentCluster, &nextCluster, sizeof(unsigned short));
	}
}

//Scans and prints the chosen dir
void FsFat12_ScanDir(pDirectoryEntry dir)
{
	//Date
	unsigned int year;
	unsigned int month; 
	unsigned int day; 
	//Time
	unsigned int hour; 
	unsigned int min;
	
	for (int i = 0; i < 16; i++) //16 dirs per sector
	{
		if(dir->Attrib == 0x10 || dir->Attrib == 0x20 && dir->Filename[0] != 0xe5)
		{
			char name[11];
			memcpy(name, dir->Filename, 11);
			name[11] = '\0';
			
			//Write the filename and extension
			ConsoleWriteString(name);
			if(dir->Attrib == 0x10)
			{
				dirCount++;
				ConsoleWriteString("<DIR>");
				ConsoleWriteCharacter('\t');
			}
			else
			{
				fileCount++;
				ConsoleWriteCharacter('\t');
				ConsoleWriteInt(dir->FileSize, 10);
				byteCount += dir->FileSize;
				ConsoleWriteCharacter('\t');
			}
			
			//For the day, we want bits 11 to 16
			day = ((dir->DateCreated) & 0x1f);
			ConsoleWriteInt(day, 10);
			ConsoleWriteCharacter('-');
			//Month is 7 to 11
			month = ((dir->DateCreated >> 5) & 0x0f);
			ConsoleWriteInt(month, 10);
			ConsoleWriteCharacter('-');
			//year is the first 7
			year = (dir->DateCreated >> 9) + 1980;
			ConsoleWriteInt(year, 10);	
			
			ConsoleWriteCharacter('\t');

			//Do the hour first 5 bits in a 16 bit int
			hour = (dir->TimeCreated >> 11);
			ConsoleWriteInt(hour, 10);
			ConsoleWriteCharacter(':');
			min = ((dir->TimeCreated >> 5) & 0x3f);
			ConsoleWriteInt(min, 10);
			ConsoleWriteCharacter('\n');

		}
		
		dir++;
	}
	return;
}

void FsFat12_Dir(const char* path)
{		
	pDirectoryEntry dir;
	unsigned char* floppyBuffer; 

	if(path)
	{
		FILE folder = FsFat12_Open(path); //Get the directory!
		if(folder.Flags == FS_DIRECTORY)
		{
			ConsoleWriteCharacter('\n');

			while(folder.Eof != 1)
			{						
				unsigned char buffer[512];		
				FsFat12_Read(&folder, buffer, 512);
				dir = (pDirectoryEntry)buffer;	
				FsFat12_ScanDir(dir);
			}	
		}
		else
		{
			ConsoleWriteString("\nYou can only use \"dir\" on directories!\n");
			return;
		}
	}
	else
	{
		//We need to do sort root!		
		ConsoleWriteCharacter('\n');
		for (int sec = 0; sec < 14; sec++) //14 sectors
		{		
			floppyBuffer = (unsigned char*)FloppyDriveReadSector(rootOffset + sec);
			dir = (pDirectoryEntry)floppyBuffer;
			FsFat12_ScanDir(dir);
		}
			
	}
	
	//Print directory info MSDOS style
	ConsoleWriteCharacter('\t');
	ConsoleWriteInt(fileCount, 10);
	ConsoleWriteString(" file(s)\t");
	ConsoleWriteInt(byteCount, 10);
	ConsoleWriteString(" bytes");
	ConsoleWriteCharacter('\n');
	ConsoleWriteCharacter('\t');
	ConsoleWriteInt(dirCount, 10);
	ConsoleWriteString(" dir(s)");
	ConsoleWriteCharacter('\n');
	//Reset the counters before getting outa here! 
	fileCount 	= 0; 
	dirCount 	= 0;
	byteCount 	= 0;
}

void FsFat12_Close(PFILE file)
{
	if (file)
	{
		file->Eof = 1;
	}
}


