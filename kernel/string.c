// Implementation of string and memory functions

#include <string.h>

// Finds the first occurance of a char and returns position
int contains(const char* source, char c)
{
	unsigned int i = 0;
	for(i = 0; i < strlen(source); i++)
	{
		if(source[i] == c)
		{
			return i;
		}
	}
	return -1;
}

// Compare two strings
int strcmp(const char* str1, const char* str2) 
{
	for ( ; *str1 == *str2; str1++, str2++)
	{
		if (*str1 == '\0')
		{
			return 0;
		}
	}
    return ((*(unsigned char *)str1 < *(unsigned char *)str2) ? -1 : +1);
}

// Compare two string up to a length - https://opensource.apple.com/source/Libc/Libc-167/gen.subproj/ppc.subproj/strncmp.c
int strncmp(const char *s1, const char *s2, size_t n)
{
    for ( ; n > 0; s1++, s2++, --n)
	{
		if (*s1 != *s2)
		{
			return ((*(unsigned char *)s1 < *(unsigned char *)s2) ? -1 : +1);
		}
		else if (*s1 == '\0')
		{
			return 0;
		}
	}
    return 0;
}

// Copy string source to destination

char * strcpy(char * destination, const char * source)
{
    char * destinationTemp = destination;
    while (*destination++ = *source++);
    return destinationTemp;
}

// Safe version of strcpy

errno_t strcpy_s(char *destination, size_t numberOfElements, const char *source)
{
	if (destination == NULL || source == NULL)
	{
		return EINVAL;
	}
	if (numberOfElements == 0)
	{
		*destination = '\0';
		return ERANGE;
	}
	char * sourceTemp = (char *)source;
	char * destinationTemp = destination;
	char charSource;
	while (numberOfElements--)
	{
		charSource = *sourceTemp++;
		*destinationTemp = charSource;
		if (charSource == 0)
		{
			// Copy succeeded
			return 0;
		}
	}
	// We haven't reached the end of the string
	*destination = '\0';
	return ERANGE;
}


// Return length of string

size_t strlen(const char* source) 
{
	size_t len = 0;
	while (source[len] != 0)
	{
		len++;
	}
	return len;
}

// Copy count bytes from src to dest

void * memcpy(void * destination, const void * source, size_t count)
{
    const char *sp = (const char *)source;
    char *dp = (char *)destination;
    while (count != 0)
	{
		*dp++ = *sp++;
		count--;
	}
    return destination;
}

// Safe verion of memcpy

errno_t memcpy_s(void *destination, size_t destinationSize, const void *source, size_t count)
{
	if (destination == NULL)
	{
		return EINVAL;
	}
	if (source == NULL)
	{
		memset((void *)destination, '\0', destinationSize);
		return EINVAL;
	}
	if (destinationSize < count == 0)
	{
		memset((void *)destination, '\0', destinationSize);
		return ERANGE;
	}
	char * sp = (char *)source;
	char * dp = (char *)destination;
	while (count--)
	{
		*dp++ = *sp++;
	}
	return 0;
}

// Set count bytes of destination to val

void * memset(void *destination, char val, size_t count)
{
    unsigned char * temp = (unsigned char *)destination;
	while (count != 0)
	{
		temp[--count] = val;
		
	}
	return destination;
}

// Set count bytes of destination to val

unsigned short * memsetw(unsigned short * destination, unsigned short val, size_t count)
{
    unsigned short * temp = (unsigned short *)destination;
    while(count != 0)
	{
		*temp++ = val;
		count--;
	}
    return destination;
}

//MAKES EVERYTHING UPPERCASE
char* toupper(char* source)
{
	int counter = 0; 
	char single;
	char buffer[1000];
	
	while(source[counter] != '\0')
	{
		single = source[counter];
		if (single >= 'A' && single <= 'Z')
		{
			source[counter] = source[counter]; //Keep the same
		}
		else if(single >= 'a' && single <= 'z')
		{
			source[counter] = source[counter] - 32;
		}
		counter ++;
	}
	return source; 
}

//returns a pointer to the nearest occurance of "c" - https://opensource.apple.com/source/Libc/Libc-167/string.subproj/strchr.c
char* strchr(const char *s, int c)
{	
	while (*s != (char)c)
		if (!*s++)
			return 0;
    return (char *)s;
}

