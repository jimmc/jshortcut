/* Copyright 2002 Jim McBeath under GNU GPL v2 */

#include <windows.h>
#include <shlobj.h>
#include <objidl.h>
#include <jni.h>

//In Win95 and NT, you can read the value of the "Desktop" and "Start Menu"
//strings from the registry key in HKEY_CURRENT_USER:
//Software\MicroSoft\Windows\CurrentVersion\Explorer\Shell Folders

static char* stringType = "Ljava/lang/String;";
static jclass JShellLinkClass;

// Get a Java string value from a JShellLink object.
static jstring
getJavaString(
	JNIEnv *env,
	jobject jobj,		// a JShellLink object
	char *fieldName)	// the name of the String field to get
{
	jfieldID fid;

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
	return (jstring)env->GetObjectField(jobj,fid);
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

	//Get the requested location
	SHGetMalloc(&shellMalloc);
	SHGetSpecialFolderLocation(NULL, special, &idList);
	h = SHGetPathFromIDList(idList, buf);
	shellMalloc->Free(idList);
	shellMalloc->Release();
	return h;
}

// Create a new shell link
static
HRESULT			// status: test using FAILED or SUCCEEDED macro
JShortcutCreate(		// Create a shortcut
	const char *folder,	// The directory in which to create the shortcut
	const char *name,	// Base name of the shortcut
	const char *description,// Description of the shortcut
	const char *path,	// Path to the target of the link
	const char *workingDir	// Working directory for the shortcut
)
{
	char *errStr = NULL;
	HRESULT h;
	IShellLink *shellLink = NULL;
	IPersistFile *persistFile = NULL;
	TCHAR buf[256];
	WORD wName[256];
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

	shellLink->SetPath(path);
	shellLink->SetWorkingDirectory(workingDir);
	shellLink->SetDescription(description);

	//One example elsewhere adds this line:
	//shellLink->SetShowCommand(SW_SHOW);

	//Append the shortcut name to the folder
	buf[0] = 0;
	strcat(buf,folder);
	strcat(buf,"\\");
	strcat(buf,name);	//TBD check for overflow
	strcat(buf,".lnk");	//TBD check for overflow
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
	//TBD - throw exception
	return h;
}

// Create a shell link (shortcut) from Java.
JNIEXPORT jboolean JNICALL
Java_net_jimmc_jshortcut_JShellLink_nCreate(
	JNIEnv *env,
	jobject jobj)	//this
{
	jstring jFolder = getJavaString(env,jobj,"folder");
	jstring jName = getJavaString(env,jobj,"name");
	jstring jDesc = getJavaString(env,jobj,"description");
	jstring jPath = getJavaString(env,jobj,"path");
	jstring jWorkingDir = getJavaString(env,jobj,"workingDirectory");

	const char* folder = jFolder?env->GetStringUTFChars(jFolder,NULL):NULL;
	const char* name = jName?env->GetStringUTFChars(jName,NULL):NULL;
	const char* desc = jDesc?env->GetStringUTFChars(jDesc,NULL):NULL;
	const char* path = jPath?env->GetStringUTFChars(jPath,NULL):NULL;
	const char* workingDir =
		jWorkingDir?env->GetStringUTFChars(jWorkingDir,NULL):NULL;

	HRESULT h = JShortcutCreate(folder,name,desc,path,workingDir);

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

	if (strcmp(which,"desktop")==0) {
		JShortcutGetDirectory(CSIDL_DESKTOP,buf);
	} else if (strcmp(which,"personal")==0) {	// My Documents
		JShortcutGetDirectory(CSIDL_PERSONAL,buf);
//Unfortunately, the CSIDL_PROGRAM_FILES constant was not introduced until
//just after Windows 98, so it doesn't work for many Windows systems.
//Use jRegistryKey and look at
//HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows\CurrentVersion\ProgramFilesDir
//	} else if (strcmp(which,"program_files")==0) {
//		JShortcutGetDirectory(CSIDL_PROGRAM_FILES,buf);
	} else if (strcmp(which,"start_menu")==0) {
		JShortcutGetDirectory(CSIDL_STARTMENU,buf);
	}

	env->ReleaseStringUTFChars(jWhich,which);
	return env->NewStringUTF(buf);
}
