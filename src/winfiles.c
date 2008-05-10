#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>
#include <shlobj.h>
#include <direct.h>
#include <assert.h>

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

	if( GetFileAttributes( local_dir ) == INVALID_FILE_ATTRIBUTES ) {
		_mkdir( local_dir );
	}

	return 0;
}


#define DIR_SEPARATOR	"\\"

static char *FixFilename(const char *filename, const char *prefix, int force)
{
	char *f, *ptr;
	size_t flen;
	size_t plen;
	
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

static char* GetLocalDirectory(void)
{
	char folderPath[2 * MAX_PATH + 10];
	char* localdir;

	const char* homedrive;
	const char* homepath;
	char* homedir;

	homedir = NULL;

	/*
	  TODO - should check that the directory is actually usable.
     */

	/*
	   1. Check registry (not currently implemented)
	 */

	/*
	   2. CSIDL_LOCAL_APPDATA with SHGetFolderPath
	 */
	if( homedir == NULL ) {
		if( SUCCEEDED( SHGetFolderPath( NULL, CSIDL_LOCAL_APPDATA,
			NULL, SHGFP_TYPE_CURRENT, &folderPath[0] ) ) ) {

			homedir = _strdup( folderPath );
		}
	}

	/*
	   3. CSIDL_APPDATA with SHGetFolderPath
     */
	if( homedir == NULL ) {
		if( SUCCEEDED( SHGetFolderPath( NULL, CSIDL_APPDATA,
			NULL, SHGFP_TYPE_CURRENT, &folderPath[0] ) ) ) {

			homedir = _strdup( folderPath );
		}
	}

	/*
	   4. HOMEDRIVE+HOMEPATH
     */

	if( homedir == NULL ) {
		homedrive = getenv("HOMEDRIVE");
		homepath  = getenv("HOMEPATH");

		if( homedrive == NULL ) {
			homedrive = "";
		}

		if( homepath != NULL ) {

			homedir = (unsigned char*)malloc(strlen(homedrive)+strlen(homepath)+1);
			
			strcpy(homedir, homedrive);
			strcat(homedir, homepath);
		}
	}

	/*
	   5. HOME
     
     */
	if( homedir == NULL ) {
		homepath = getenv("HOME");

		if( homepath != NULL ) {
			homedir = _strdup(homepath);
		}
	}

	/* 
	  6. CWD
     */
	if( homedir == NULL ) {
		homedir = _strdup(".");
	}

	localdir = (unsigned char*)malloc(strlen(homedir) + 10);
	strcpy(localdir, homedir);
	strcat(localdir, "\\AvPLinux"); // temp name, maybe

	free(homedir);

	return localdir;
}

static const char* GetGlobalDirectory(void)
{
	/*
	   TODO
     */
	return _strdup(".");
}

/*
  Game-specific helper function.
 */
static int try_game_directory(const char *dir, const char *file)
{
	char tmppath[MAX_PATH];
	DWORD retr;

	strncpy(tmppath, dir, MAX_PATH-32);
	tmppath[MAX_PATH-32] = 0;
	strcat(tmppath, file);
	
	retr = GetFileAttributes(tmppath);

	if( retr == INVALID_FILE_ATTRIBUTES ) {
		return 0;
	}

	/*
	  TODO - expand this check to check for read access
     */
	return 1;
}

/*
  Game-specific helper function.
 */
static int check_game_directory(const char *dir)
{
	if (!dir || !*dir) {
		return 0;
	}
	
	if (!try_game_directory(dir, "\\avp_huds")) {
		return 0;
	}
	
	if (!try_game_directory(dir, "\\avp_huds\\alien.rif")) {
		return 0;
	}
	
	if (!try_game_directory(dir, "\\avp_rifs")) {
		return 0;
	}
	
	if (!try_game_directory(dir, "\\avp_rifs\\temple.rif")) {
		return 0;
	}
	
	if (!try_game_directory(dir, "\\fastfile")) {
		return 0;
	}
	
	if (!try_game_directory(dir, "\\fastfile\\ffinfo.txt")) {
		return 0;
	}
	
	return 1;
}

/*
  Game-specific initialization
 */
void InitGameDirectories(char *argv0)
{
	extern char *SecondTex_Directory;
	extern char *SecondSoundDir;

	const char* localdir;
	const char* globaldir;

	SecondTex_Directory = "graphics\\";
	SecondSoundDir = "sound\\";

	localdir  = GetLocalDirectory();
	globaldir = GetGlobalDirectory();

	assert(localdir != NULL);
	assert(globaldir != NULL);
	
	/* last chance sanity check */
	if (!check_game_directory(globaldir)) {
		fprintf(stderr, "Unable to find the AvP gamedata.\n");
		fprintf(stderr, "The directory last examined was: %s\n", globaldir);
		fprintf(stderr, "Has the game been installed and\n");
		fprintf(stderr, "are all game files lowercase?\n");
		exit(EXIT_FAILURE);
	}

	SetGameDirectories(localdir, globaldir);
}
