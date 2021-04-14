#include <stdlib.h>
#include <string.h>

char*
bar_strdup(const char* org)
{
    if(org == NULL)
		return NULL;

    char* newstr = malloc(strlen(org)+1);
    char* p;

    if(newstr == NULL)
		return NULL;

    p = newstr;

    while(*org) *p++ = *org++; /* copy the string. */
    return newstr;
}

