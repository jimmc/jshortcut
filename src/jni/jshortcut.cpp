/* Copyright 2002 Jim McBeath under GNU GPL v2 */

#include <windows.h>
#include <shlobj.h>
#include <objidl.h>
#include <jni.h>

static char* stringType = "Ljava/lang/String;";
static jclass JShellLinkClass;

// Get a Java string value from a JShellLink object.
static
jstring
JShortcutGetJavaString(
	JNIEnv *env,
	jobject jobj,		// a JShellLink object
	char *fieldName)	// the name of the String field to get
{
	jfieldID fid;
	jstring fieldValue;

	if (!JShellLinkClass) {
		char *className = "net/jimmc/jshortcut/JShellLink";
		JShellLinkClass = env->FindClass(className);
		if (!JShellLinkClass) {
			fprintf(stderr,"Can't find class %s\n",className);
			return NULL;
		}
	}
	fid = env->GetFieldID(JShellLinkClass,fieldName,stringType);
	if (fid==0) {
		//TBD error, throw exception?
		fprintf(stderr,"Can't find field %s\n",fieldName);
		return NULL;
	}
	fieldValue = (jstring)env->GetObjectField(jobj,fid);
	return fieldValue;
}

// Set a Java string value into a JShellLink object.
static
int			// 0 if error, 1 if OK
JShortcutSetJavaString(
	JNIEnv *env,
	jobject jobj,		// a JShellLink object
	char *fieldName,	// the name of the String field to set
	jstring fieldValue)	// the value to set into that field
{
	jfieldID fid;

	if (!JShellLinkClass) {
		char *className = "net/jimmc/jshortcut/JShellLink";
		JShellLinkClass = env->FindClass(className);
		if (!JShellLinkClass) {
			fprintf(stderr,"Can't find class %s\n",className);
			return 0;
		}
	}
	fid = env->GetFieldID(JShellLinkClass,fieldName,stringType);
	if (fid==0) {
		//TBD error, throw exception?
		fprintf(stderr,"Can't find field %s\n",fieldName);
		return 0;
	}
	env->SetObjectField(jobj,fid,fieldValue);
	return 1;
}

// Get the path for one of the special Windows directories such as
// the desktop or the Program Files directory.
static
HRESULT			// status: test using FAILED or SUCCEEDED macro
JShortcutGetDirectory(
	int special,	// one of the CSIDL_* constants
	char* buf)	// Fills this in with the desktop directory
{
	HRESULT h;
	LPITEMIDLIST idList;
	LPMALLOC shellMalloc;

	buf[0] = 0;	// the return value if no data is a blank string
	//Get the requested location
	SHGetMalloc(&shellMalloc);
	SHGetSpecialFolderLocation(NULL, special, &idList);
	h = SHGetPathFromIDList(idList, buf);
	shellMalloc->Free(idList);
	shellMalloc->Release();
	return h;
}

// Get the value of a Registry entry.
static
int			// 0 if OK, nonzero if error
JShortcutGetRegistryValue(
	HKEY root,	// one of the HKEY_* constants
	char* subkey,	// the path within the named root registry
	char* item,	// the final item name
	char* buf,	// Fills this in with the desktop directory
	int bufSize)	// size of buf
{
	LONG status;
	HKEY hkey;
	DWORD type;
	DWORD size;

	status = RegOpenKeyEx(root, subkey, 0, KEY_READ, &hkey);
	if (status!=ERROR_SUCCESS) {	//who thought up this name?
		//failed
		buf[0] = 0;
		return -1;		//no change to buf
	}

	size = bufSize;
	status = RegQueryValueEx(hkey, item, NULL, &type,
			(unsigned char *)buf, &size);
	if (type!=REG_SZ) {
		// We only know how to handle string values
		buf[0] = 0;
		return -1;
	}

	RegCloseKey(hkey);
	return 0;
}

// Create a new shell link
static
HRESULT			// status: test using FAILED or SUCCEEDED macro
JShortcutCreate(		// Create a shortcut
	const char *folder,	// The directory in which to create the shortcut
	const char *name,	// Base name of the shortcut
	const char *description,// Description of the shortcut
	const char *path,	// Path to the target of the link
	const char *workingDir,	// Working directory for the shortcut
	const char *iconLoc	// Path to icon
)
{
	char *errStr = NULL;
	HRESULT h;
	IShellLink *shellLink = NULL;
	IPersistFile *persistFile = NULL;
	TCHAR buf[MAX_PATH+1];
	WORD wName[MAX_PATH+1];
	int id;

	// Initialize the COM library
	h = CoInitialize(NULL);
	if (FAILED(h)) {
		errStr = "Failed to initialize COM library";
		goto err;
	}

	h = CoCreateInstance( CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER,
			IID_IShellLink, (PVOID*)&shellLink );
	if (FAILED(h)) {
		errStr = "Failed to create IShellLink";
		goto err;
	}

	h = shellLink->QueryInterface(IID_IPersistFile, (PVOID*)&persistFile);
	if (FAILED(h)) {
		errStr = "Failed to get IPersistFile";
		goto err;
	}

	if (description!=NULL)
		shellLink->SetDescription(description);
	if (path!=NULL)
		shellLink->SetPath(path);
	if (workingDir!=NULL)
		shellLink->SetWorkingDirectory(workingDir);
	if (iconLoc!=NULL)
		shellLink->SetIconLocation(iconLoc);

	//One example elsewhere adds this line:
	//shellLink->SetShowCommand(SW_SHOW);

	//Append the shortcut name to the folder
	buf[0] = 0;
	if (strlen(folder)+strlen(name)+6 > sizeof(buf)) {
		errStr = "Folder+name is too long";
		goto err;
	}
	strcat(buf,folder);
	strcat(buf,"\\");
	strcat(buf,name);
	strcat(buf,".lnk");
	MultiByteToWideChar(CP_ACP,0,buf,-1,wName,MAX_PATH);

	//Save the shortcut to disk
	h = persistFile->Save(wName, TRUE);
	if (FAILED(h)) {
		errStr = "Failed to save shortcut";
		goto err;
	}

#if 0
	//Now use the icon from shell32.dll for the shortcut
	GetSystemDirectory(buf,sizeof(buf));
	strcat(buf,"\\shell32.dll");	//TBD check for overflow

	h = shellLink->SetIconLocation(buf,1);
	if (FAILED(h)) {
		errStr = "Failed to set icon";
		goto err;
	}

	h = shellLink->GetIconLocation(buf,sizeof(buf),&id);
	if (FAILED(h)) {
		errStr = "Failed to verify icon";
		goto err;
	}

	//Save the shelllink changes into the file
	h = persistFile->Save(wName,TRUE);
	if (FAILED(h)) {
		errStr = "Failed to save icon";
		goto err;
	}
#endif

	persistFile->Release();
	shellLink->Release();
	CoUninitialize();
	return h;

err:
	if (persistFile!=NULL)
		persistFile->Release();
	if (shellLink!=NULL)
		shellLink->Release();
	CoUninitialize();
	fprintf(stderr,"Error: %s\n",errStr);
	//TBD - throw exception with errStr
	return h;
}

// Read an existing shell link
static
HRESULT			// status: test using FAILED or SUCCEEDED macro
JShortcutRead(		// Read a shortcut
	const char *folder,	// The directory in which to create the shortcut
	const char *name,	// Base name of the shortcut
	char *description,	// RETURN Description of the shortcut
	const int descriptionSize,
	char *path,		// RETURN Path to the target of the link
	const int pathSize,
	char *workingDir,	// RETURN Working directory for the shortcut
	const int workingDirSize,
	char *iconLoc,		// RETURN Icon location
	cont int iconLocSize
		//All returned chars are written into the caller's buffers
)
{
	char *errStr = NULL;
	HRESULT h;
	IShellLink *shellLink = NULL;
	IPersistFile *persistFile = NULL;
	TCHAR buf[MAX_PATH+1];
	WORD wName[MAX_PATH+1];
	int id;

	// Initialize the COM library
	h = CoInitialize(NULL);
	if (FAILED(h)) {
		errStr = "Failed to initialize COM library";
		goto err;
	}

	h = CoCreateInstance( CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER,
			IID_IShellLink, (PVOID*)&shellLink );
	if (FAILED(h)) {
		errStr = "Failed to create IShellLink";
		goto err;
	}

	h = shellLink->QueryInterface(IID_IPersistFile, (PVOID*)&persistFile);
	if (FAILED(h)) {
		errStr = "Failed to get IPersistFile";
		goto err;
	}

	//Append the shortcut name to the folder
	buf[0] = 0;
	if (strlen(folder)+strlen(name)+6 > sizeof(buf)) {
		errStr = "Folder+name is too long";
		goto err;
	}
	strcat(buf,folder);
	strcat(buf,"\\");
	strcat(buf,name);
	strcat(buf,".lnk");
	MultiByteToWideChar(CP_ACP,0,buf,-1,wName,MAX_PATH);

//TBD - figure out how to Load a shortcut (IPersistFile)

	//Load the shortcut From disk
	h = persistFile->Load(wName, 0);
	if (FAILED(h)) {
		errStr = "Failed to load shortcut";
		goto err;
	}

	//Read the field values
	h = shellLink->GetDescription(desc,descSize);
	if (FAILED(h)) {
		errStr = "Failed to read description";
		goto err;
	}

	h = shellLink->GetPath(path,pathSize,NULL,SLGP_UNCPRIORITY);
	if (FAILED(h)) {
		errStr = "Failed to read path";
		goto err;
	}

	h = shellLink->GetWorkingDirectory(workingDir,workingDirSize);
	if (FAILED(h)) {
		errStr = "Failed to read working directory";
		goto err;
	}

	h = shellLink->GetIconLocation(iconLoc,iconLocSize,&id);
	if (FAILED(h)) {
		errStr = "Failed to read icon";
		goto err;
	}

	persistFile->Release();
	shellLink->Release();
	CoUninitialize();
	return h;

err:
	if (persistFile!=NULL)
		persistFile->Release();
	if (shellLink!=NULL)
		shellLink->Release();
	CoUninitialize();
	fprintf(stderr,"Error: %s\n",errStr);
	//TBD - throw exception with errStr
	return h;
}

// Create a shell link (shortcut) from Java.
JNIEXPORT jboolean JNICALL
Java_net_jimmc_jshortcut_JShellLink_nCreate(
	JNIEnv *env,
	jobject jobj)	//this
{
	jstring jFolder, jName, jDesc, jPath, jWorkingDir, jIconLoc;
	const char *folder, *name, *desc, *path, *workingDir, *iconLoc;

	//If we don't clear JShellLinkClass between calls, we get an
	//EXCEPTION_ACCESS_VIOLATION when we call GetFieldID on the
	//second call to nCreate.
	JShellLinkClass = NULL;

	jFolder = JShortcutGetJavaString(env,jobj,"folder");
	jName = JShortcutGetJavaString(env,jobj,"name");
	jDesc = JShortcutGetJavaString(env,jobj,"description");
	jPath = JShortcutGetJavaString(env,jobj,"path");
	jWorkingDir = JShortcutGetJavaString(env,jobj,"workingDirectory");
	jIconLoc = JShortcutGetJavaString(env,jobj,"iconLocation");

	if (jFolder==NULL || jName==NULL)
		return false;		//error, incompletely specified

	folder = jFolder?env->GetStringUTFChars(jFolder,NULL):NULL;
	name = jName?env->GetStringUTFChars(jName,NULL):NULL;
	desc = jDesc?env->GetStringUTFChars(jDesc,NULL):NULL;
	path = jPath?env->GetStringUTFChars(jPath,NULL):NULL;
	workingDir = jWorkingDir?env->GetStringUTFChars(jWorkingDir,NULL):NULL;
	iconLoc = jIconLoc?env->GetStringUTFChars(jIconLoc,NULL):NULL;

	HRESULT h = JShortcutCreate(folder,name,desc,path,workingDir,iconLoc);

	if (folder)
		env->ReleaseStringUTFChars(jFolder,folder);
	if (name)
		env->ReleaseStringUTFChars(jName,name);
	if (desc)
		env->ReleaseStringUTFChars(jDesc,desc);
	if (path)
		env->ReleaseStringUTFChars(jPath,path);
	if (workingDir)
		env->ReleaseStringUTFChars(jWorkingDir,workingDir);
	if (iconLoc)
		env->ReleaseStringUTFChars(jIconLoc,iconLoc);

	return SUCCEEDED(h);
}

// Read a shell link (shortcut) from Java.
JNIEXPORT jboolean JNICALL
Java_net_jimmc_jshortcut_JShellLink_nRead(
	JNIEnv *env,
	jobject jobj)	//this
{
	jstring jFolder, jName, jDesc, jPath, jWorkingDir;
	const char *folder, *name;
	char desc[MAX_PATH+1];
	char path[MAX_PATH+1];
	char workingDir[MAX_PATH+1];
	char iconLoc[MAX_PATH+1];

	//See comment in Java_net_jimmc_jshortcut_JShellLink_nCreate
	JShellLinkClass = NULL;

	jFolder = JShortcutGetJavaString(env,jobj,"folder");
	jName = JShortcutGetJavaString(env,jobj,"name");

	if (jFolder==NULL || jName==NULL)
		return false;		//error, incompletely specified

	folder = jFolder?env->GetStringUTFChars(jFolder,NULL):NULL;
	name = jName?env->GetStringUTFChars(jName,NULL):NULL;

	HRESULT h = JShortcutRead(folder,name,
			desc,sizeof(desc),path,sizeof(path),
			workingDir,sizeof(workingDir),iconLoc,sizeof(iconLoc));

	//TBD - if the shortcut does not exist, what should we do?

	if (folder)
		env->ReleaseStringUTFChars(jFolder,folder);
	if (name)
		env->ReleaseStringUTFChars(jName,name);

	jDesc = env->NewStringUTF(desc);
	jPath = env->NewStringUTF(path);
	jWorkingDir = env->NewStringUTF(workingDir);
	jIconLoc = env->NewStringUTF(iconLoc);

	JShortcutSetJavaString(env,jobj,"description",jDesc);
	JShortcutSetJavaString(env,jobj,"path",jPath);
	JShortcutSetJavaString(env,jobj,"workingDirectory",jWorkingDir);
	JShortcutSetJavaString(env,jobj,"iconLocation",jIconLoc);

	return SUCCEEDED(h);
}

// Get the path to a Windows special directory from Java
JNIEXPORT jstring JNICALL
Java_net_jimmc_jshortcut_JShellLink_nGetDirectory(
	JNIEnv *env,
	jclass *jcl,    // static method
	jstring jWhich)     // keyword for special location, must be lower case
{
    	char buf[MAX_PATH+1];
	const char* which = env->GetStringUTFChars(jWhich,NULL);

	//If not defined, return blank string.
	buf[0] = 0;

	//Start by looking for specials which we can get
	//using SHGetSpecialFolderLocation.
	if (strcmp(which,"desktop")==0) {
		JShortcutGetDirectory(CSIDL_DESKTOP,buf);
	} else if (strcmp(which,"personal")==0) {	// My Documents
		JShortcutGetDirectory(CSIDL_PERSONAL,buf);
	} else if (strcmp(which,"programs")==0) {	// StartMenu/Programs
		JShortcutGetDirectory(CSIDL_PROGRAMS,buf);
//The CSIDL_PROGRAM_FILES constant was not introduced until
//just after Windows 98, so it doesn't work for many Windows systems.
//See below for code to look in the Registry instead.
//	} else if (strcmp(which,"program_files")==0) {
//		JShortcutGetDirectory(CSIDL_PROGRAM_FILES,buf);
//CSIDL_STARTMENU returns garbage on NT; use CSIDL_PROGRAMS instead.
//	} else if (strcmp(which,"start_menu")==0) {
//		JShortcutGetDirectory(CSIDL_STARTMENU,buf);
	}

	//Look for specials we can get from the Registry.
	else if (strcmp(which,"program_files")==0) {
		JShortcutGetRegistryValue(HKEY_LOCAL_MACHINE,
			"SOFTWARE\\Microsoft\\Windows\\CurrentVersion",
			"ProgramFilesDir",buf,sizeof(buf));
	}

//In Win95 and NT, you can read the value of the "Desktop" and "Start Menu"
//strings from the registry key in HKEY_CURRENT_USER:
//Software\MicroSoft\Windows\CurrentVersion\Explorer\Shell Folders

	env->ReleaseStringUTFChars(jWhich,which);
	return env->NewStringUTF(buf);
}
