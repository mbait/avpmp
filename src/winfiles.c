#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>

#include "files.h"

static char *local_dir;
static char *global_dir;

/*
Sets the local and global directories used by the other functions.
Local = ~/.dir, where config and user-installed files are kept.
Global = installdir, where installed data is stored.
*/
int SetGameDirectories(const char *local, const char *global)
{
	local_dir = _strdup(local);
	global_dir = _strdup(global);

	// TODO - create local directory if it doesn't exist
	return 0;
}


#define DIR_SEPARATOR	"\\"

static char *FixFilename(const char *filename, const char *prefix, int force)
{
	char *f, *ptr;
	int flen;
	int plen;
	
	plen = strlen(prefix) + 1;
	flen = strlen(filename) + plen + 1;
	
	f = (char *)malloc(flen);
	strcpy(f, prefix);
	strcat(f, DIR_SEPARATOR);
	strcat(f, filename);
	
	/* only the filename part needs to be modified */
	ptr = &f[plen+1];
	
	while (*ptr) {
		if ((*ptr == '/') || (*ptr == '\\') || (*ptr == ':')) {
			*ptr = DIR_SEPARATOR[0];
		} else if (*ptr == '\r' || *ptr == '\n') {
			*ptr = 0;
			break;
		} else {
			if (force) {
				*ptr = tolower(*ptr);
			}
		}
		ptr++;
	}

	return f;
}

/*
Open a file of type type, with mode mode.

Mode can be:
#define	FILEMODE_READONLY	0x01
#define	FILEMODE_WRITEONLY	0x02
#define	FILEMODE_READWRITE	0x04
#define FILEMODE_APPEND		0x08
Type is (mode = ReadOnly):
#define	FILETYPE_PERM		0x08 // try the global dir only 
#define	FILETYPE_OPTIONAL	0x10 // try the global dir first, then try the local dir
#define	FILETYPE_CONFIG		0x20 // try the local dir only

Type is (mode = WriteOnly or ReadWrite):
FILETYPE_PERM: error
FILETYPE_OPTIONAL: error
FILETYPE_CONFIG: try the local dir only
*/
FILE *OpenGameFile(const char *filename, int mode, int type)
{
	char *rfilename;
	char *openmode;
	FILE *fp;
	
	if ((type != FILETYPE_CONFIG) && (mode != FILEMODE_READONLY)) 
		return NULL;
	
	switch(mode) {
		case FILEMODE_READONLY:
			openmode = "rb";
			break;
		case FILEMODE_WRITEONLY:
			openmode = "wb";
			break;
		case FILEMODE_READWRITE:
			openmode = "w+";
			break;
		case FILEMODE_APPEND:
			openmode = "ab";
			break;
		default:
			return NULL;
	}

	if (type != FILETYPE_CONFIG) {
		rfilename = FixFilename(filename, global_dir, 0);
		
		fp = fopen(rfilename, openmode);
		
		free(rfilename);
		
		if (fp != NULL) {
			return fp;
		}
		
		rfilename = FixFilename(filename, global_dir, 1);
		
		fp = fopen(rfilename, openmode);
		
		free(rfilename);
		
		if (fp != NULL) {
			return fp;
		}
	}
	
	if (type != FILETYPE_PERM) {
		rfilename = FixFilename(filename, local_dir, 0);
		
		fp = fopen(rfilename, openmode);
		
		free(rfilename);
		
		if (fp != NULL) {
			return fp;
		}
		
		rfilename = FixFilename(filename, local_dir, 1);
		
		fp = fopen(rfilename, openmode);
		
		free(rfilename);
		
		return fp;
	}
	
	return NULL;
}

int CloseGameFile(FILE *pfd)
{
	return fclose(pfd);
}

int GetGameFileAttributes(const char *filename, int type)
{
	// TODO
	return 0;
}

int DeleteGameFile(const char *filename)
{
	// TODO
	return 0;
}

int CreateGameDirectory(const char *dirname)
{
	// TODO
	return 0;
}

void *OpenGameDirectory(const char *dirname, const char *pattern, int type)
{
	// TODO
	return NULL;
}

GameDirectoryFile *ScanGameDirectory(void *dir)
{
	// TODO
	return NULL;
}

int CloseGameDirectory(void *dir)
{
	// TODO
	return 0;
}

void InitGameDirectories(char *argv0)
{
	SetGameDirectories("local", ".");
}
