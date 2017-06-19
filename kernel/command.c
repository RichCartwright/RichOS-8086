#include <command.h>
#include <console.h>
#include <keyboard.h>
#include <hal.h>
#include <stdint.h>
#include <ctype.h>
#include <string.h>
#include <floppydisk.h>
#include <filesystem.h>
#include "physicalmemorymanager.h"

int cursorX; 
int cursorY;
char* promptMessage = "CMD";
char CurrentDirectory[100];

void ReadFromDisk(int sector)
{
	uint8_t* result = FloppyDriveReadSector(sector); 	//Get the result from floppy 
	
	ConsoleWriteString("\n\r");
	
	//512 bytes to a sector (reference - Lecture slides)
	for (int i = 0; i < 512; i++)
	{
		if(result[i] != 0)
		{
			ConsoleWriteString("0x");
			ConsoleWriteInt(result[i], 16);					//Display in hex
			ConsoleWriteString(" ");
		}
	}
}

void KeyboardInput(char* buffer)
{	
	char key;	
	int i = 0; 											//Keeping track of position in char array

	while(*buffer)
	{
		bool addToArray = true; 						//We need this for backspaces - We don't to add certain keypresses in the buffer

		keycode keyInputted = KeyboardGetCharacter();	//Wait till we get input
		
		if (keyInputted == KEY_RETURN)					//Break the loop if RET is hit, 
		{	
			if (i != 0)
			{
				buffer[i] = '\0';							
			}		
			return;
		}
		
		if (keyInputted == KEY_BACKSPACE)
		{
			addToArray = false;

			if(i > 0)									//We only want to delete the char* not other stuff
			{	
				/* I tried to get this working using the actual keycode print (it seems to step it back) 
					but this was the only way I could get it working */
				
				ConsoleGetXY(&cursorX, &cursorY); 		//First get the cursor position
				ConsoleGotoXY(--cursorX, cursorY);		//Go back a char
				ConsoleWriteCharacter(' ');				//Clear the char on screen
				ConsoleGotoXY(cursorX, cursorY);		//put it back
				i--;				
			}
		}
		
		if(addToArray)
		{
			key = KeyboardConvertKeyToASCII(keyInputted);	//Get the input in ASCII, save it as a single char
			ConsoleWriteCharacter(key);						//Write it for the user
			buffer[i++] = key; 								//Add it to the buffer 
		}
	}
}

bool ProcessCommand(char* cmd)
{	
	if(strlen(cmd) == 0)
	{
		return 1;	//No command - Just return
	}
	
	toupper(cmd); //Turn the command to upper case, this means user input isnt case sensitive!
	
	if(contains(cmd, ' ') == -1) //No spaces in the command
	{
		if(strcmp(cmd, "DIR") == 0) //No argument for DIR, that means use the current directory
		{
			if(strcmp(CurrentDirectory, "") != 0)
			{
				FsFat12_Dir(CurrentDirectory);
			}
			else
			{
				FsFat12_Dir(NULL);
			}
			return 1; 
		}
		
		//First check for single word commands
		if(strcmp(cmd, "CLS") == 0)
		{
			ConsoleClearScreen(0x0F);							//Simply clear the screen & return
			return 1;
		}
		
		if(strcmp(cmd, "EXIT") == 0)
		{
			ConsoleWriteString("\n\n\n\n\rSystem is shutting down - Please Wait...");
			return 0;											//return a false to break the loop
		}
		
		if(strcmp(cmd, "MEMUSE") == 0)
		{
			ConsoleWriteString("\nMemory Size:      ");
			ConsoleWriteInt(PMM_GetAvailableMemorySize(), 10);
			ConsoleWriteString("bytes");
			
			ConsoleWriteString("\nUsed Memory:      ");
			ConsoleWriteInt(PMM_GetUsedBlockCount() * 4, 10);
			ConsoleWriteString("bytes");
			
			ConsoleWriteString("\nAvailable Memory: ");
			ConsoleWriteInt(PMM_GetFreeBlockCount() * 4, 10);
			ConsoleWriteString("bytes\n");
			
			return 1;
			
		}	
		
		if(strcmp(cmd, "PWD") == 0)
		{
			ConsoleWriteString("\nCurrent Directory: ");
			if(strcmp(CurrentDirectory, "") == 0)
			{
				ConsoleWriteString("\\");
			}
			else
			{
				ConsoleWriteString((char*)CurrentDirectory);
			}
			ConsoleWriteCharacter('\n');
			return 1; 
		}
		
		//Its either invalid or has an arguement
		if(strcmp(cmd, "HELP") == 0)
		{
			ConsoleWriteString("\nhelp                     : You're lookin' at it");
			ConsoleWriteString("\ncls                      : Clear Screen");
			ConsoleWriteString("\npwd                      : Display current directory");
			ConsoleWriteString("\nmemuse			 : Show system memory information");
			ConsoleWriteString("\nprompt /\"var\"            : Change prompt message");
			ConsoleWriteString("\nreaddisk /\"sector no.\"   : Display contents of chosen floppy sector");
			ConsoleWriteString("\nread \\\"filepath\"         : Read file");
			ConsoleWriteString("\ncd \\\"filepath\" or dir    : Change working directory");
			ConsoleWriteString("\n");

			return 1;
		}
	}
	else
	{
		//Get the position of the space
		int spacePos = contains(cmd, ' ');	
		
		char commandFirst[10];
		memset(commandFirst, 0, 10);		
		//Get the base command
		for(int i = 0; i < spacePos; i++)
		{
			commandFirst[i] = cmd[i];
		}
		
		char argument[50]; //50 seems reasonable for a path...
		memset(argument, 0, 50);		
		//Get the rest of the command
		for(int i = spacePos + 1, j = 0; i < strlen(cmd); i++, j++)
		{
			if(cmd != '\0')
			{
				argument[j] = cmd[i];
			}
		}
	
		// ###### Read file with path ######
		if(strcmp(commandFirst, "READ") == 0)
		{
			FILE newFile;
			unsigned int bytesRead = 0;
			
			if(strcmp(CurrentDirectory, "") == 0) 	//We are CD'd in root
			{
				newFile = FsFat12_Open(argument);
			}
			else
			{					
				char argumentBuffer[100];
				memset(argumentBuffer, 0, sizeof(argumentBuffer));
				strcpy(argumentBuffer, CurrentDirectory);
				argumentBuffer[strlen(CurrentDirectory)] = '\\';	
				
				for(int i = strlen(argumentBuffer), j = 0; 
						i < strlen(CurrentDirectory) + strlen(argument) + 1; 
						i++, j++)
				{
					argumentBuffer[i] = argument[j];
				}
				newFile = FsFat12_Open(argumentBuffer);
			}
			
			if(newFile.Flags == FS_INVALID)
			{
				ConsoleWriteString("\nNo file found.");
				return 1;
			}
			else
			{
				if(newFile.Flags == FS_FILE)
				{
					ConsoleWriteString("\n\nViewing: ");
					ConsoleWriteString(newFile.Name);
					ConsoleWriteString("\n--------------------------------------------------\n");
					while(newFile.Eof != 1)
					{			
						unsigned char buffer[512];
						bytesRead += FsFat12_Read(&newFile, buffer, 512);
						for(int i = 0; i < 512; i++)
						{
							ConsoleWriteCharacter(buffer[i]);
						}
					}
					ConsoleWriteString("\n################### EOF ");
					//ConsoleWriteInt(bytesRead, 10);
					//ConsoleWriteString(" bytes read");
					ConsoleWriteString(" ###################\n");
				}
				else
				{
					ConsoleWriteString("\nCannot read directories! Type \"help\" for assistance\n");
				}
			}
			
			return 1; 
		} 
		
		// ###### Print directory contents with argument ######
		if(strcmp(commandFirst, "DIR") == 0)
		{
			FsFat12_Dir(argument);
			
			return 1;
		}
		
		// ###### Change prompt ######
		if(strcmp(commandFirst, "PROMPT") == 0)
		{
			char* arg = strchr(argument, '/');	//Easier than messing around with shifting arrays 
			arg++; 								
			strcpy(promptMessage, arg);			//Change the prompt message
			return 1;
		}
		
		// ###### Read disk ######
		if(strcmp(commandFirst, "READDISK") == 0)
		{				
			int num = 0; 										//Instanciate the result
			for (int i = 1; argument[i] != '\0'; ++i)			//Itterate though the char* of ints
			{													//i = 1 because we want to skip the forward slash!			
				if(argument[i] < '0' || argument[i] > '9')		//Make sure they're numbers
				{
					ConsoleWriteString("\n\n\rInvalid argument - Integers only"); //Check that we are dealing with numbers each itteration
					return 1;
				}
				
				num = num * 10 + argument[i] - '0'; //Add the old number, multiply it and add the ASCII minus ASCII '0' (48)
			}
			
			ReadFromDisk(num);
			return 1;
		}
		
		// ###### CD ######
		if(strcmp(commandFirst, "CD") == 0)
		{
			FILE newFile;

			//CD to root! 
			if(argument[0] == '\\' && argument[1] == '\0') //Really hacky but it works!
			{
				memset(CurrentDirectory, 0, sizeof(CurrentDirectory)); 
				ConsoleWriteString("\nCurrent directory changed to ");
				ConsoleWriteString("\\");
				ConsoleWriteCharacter('\n');
				return 1;
			}			
			
			char argumentBuffer[100];
			memset(argumentBuffer, 0, sizeof(argumentBuffer));
			if(strcmp(CurrentDirectory, "") != 0)
			{
				argumentBuffer[strlen(CurrentDirectory)] = '\\';
				memcpy(argumentBuffer, CurrentDirectory, strlen(CurrentDirectory));
			}
			else
			{
				argumentBuffer[0] = '\\';
				strcpy(argumentBuffer+1, CurrentDirectory);
			}
			
			for(int i = strlen(argumentBuffer), j = 0; 
					i < strlen(CurrentDirectory) + strlen(argument) + 1; 
					i++, j++)
			{
				argumentBuffer[i] = argument[j];
			}
			
			ConsoleWriteCharacter('\n');
			newFile = FsFat12_Open(argumentBuffer);
			//ConsoleWriteString(argumentBuffer);
			//It's failed first time, try with the argument on its own (incase of non-root CD)
			if(newFile.Flags != FS_DIRECTORY)
			{
				newFile = FsFat12_Open(argument);
				if(newFile.Flags == FS_DIRECTORY)
				{
					memset(CurrentDirectory, 0, sizeof(CurrentDirectory));
					strcpy(CurrentDirectory, argument); 
					ConsoleWriteString("\nCurrent directory changed to ");
					ConsoleWriteString(argument);
					ConsoleWriteCharacter('\n');
					return 1; 
				}
				else
				{
					ConsoleWriteString("\nNo valid directory found!\n");
					return 1; 
				}
			}
			else
			{
				memset(CurrentDirectory, 0, sizeof(CurrentDirectory));
				strcpy(CurrentDirectory, argumentBuffer); 
				ConsoleWriteString("\nCurrent directory changed to ");
				ConsoleWriteString(argumentBuffer);
				ConsoleWriteCharacter('\n');
			}
			return 1; 
		}
	}

	//Not returned yet? It hasn't found a command. Give the sad news and return
	ConsoleWriteString("\nUnknown Command - Type /help for list of available commands\n");
	return 1;

}

void Run() 
{
	ConsoleGetXY(&cursorX, &cursorY); //First get the cursor position and save it 
	char buffer[100];
	memset(CurrentDirectory, 0, sizeof(CurrentDirectory));
	
	while(1)
	{
		ConsoleWriteString("\n");
		ConsoleWriteString(promptMessage);
		ConsoleWriteString("> ");
		KeyboardInput(buffer);	
		
		if(!ProcessCommand(buffer)) //If we get a false returned - We need to get out of here
		{
			break;
		}
	}
}


