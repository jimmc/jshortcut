/* Copyright 2002-2003 Jim McBeath under GNU GPL v2 */

#include <windows.h>
#include <shlobj.h>
#include <objidl.h>
#include <jni.h>

//Define this to use UTF-8 encoding of native strings.  If the native encoding
//may not be UTF-8, leave this undefined to use the native encoding.
//#define USE_UTF_8

static char* stringType = "Ljava/lang/String;";
static char* intType = "I";
static jclass JShellLinkClass;
static jclass StringClass;	//java.lang.String class
static jmethodID StringClassGetBytes;
static jmethodID StringClassBytesConstructor;

static
void
JShortcutClearClasses() {
	//If we don't clear JShellLinkClass between calls, we get an
	//EXCEPTION_ACCESS_VIOLATION when we call GetFieldID on the
	//second call to nSave.
	JShellLinkClass = NULL;
	StringClass = NULL;
	StringClassGetBytes = NULL;
	StringClassBytesConstructor = NULL;
}

// Given a Java string, convert it to a native string
static
char*
JShortcutJavaStringToNative(
	JNIEnv *env,
	jstring jstr,
	jobject *jfreeobjp)		//RETURN pointer to use for freeing
{
#ifdef USE_UTF_8
// This is the easy way, using UTF-8 strings.  Unfortunately, it doesn't
// work if the native encoding is something other than UTF-8.
	char *str;

	str = jstr?(char*)env->GetStringUTFChars(jstr,NULL):NULL;
	*jfreeobjp = jstr;
	return str;
#else
// Use the String class's getByte() method to convert from a Java
// string to an array of bytes using the platform's native encoding.
	char *str;
	jbyteArray arr;
	int len;

	if (!jstr)
		return NULL;
	if (!StringClass) {
		char *className = "java/lang/String";
		StringClass = env->FindClass(className);
		if (!StringClass) {
			fprintf(stderr,"Can't find class %s\n",className);
			return NULL;
		}
	}
	if (!StringClassGetBytes) {
		StringClassGetBytes = env->GetMethodID(StringClass,
				"getBytes","()[B");
		if (!StringClassGetBytes) {
			fprintf(stderr,"Can't find method String.getBytes()\n");
			return NULL;
		}
	}
	arr = (jbyteArray)(env->CallObjectMethod(jstr,StringClassGetBytes));
	len = env->GetArrayLength(arr);
	str = (char*)malloc(len+1);
	(env->GetByteArrayRegion(arr,0,len,(signed char*)str));
	str[len] = 0;	//null-terminate the string
	*jfreeobjp = arr;
	return str;
#endif
}

// Release a native string
static
void
JShortcutReleaseNativeString(
	JNIEnv *env,
	jobject jobj,
	const char *str)
{
	if (!str)
		return;
#ifdef USE_UTF_8
	env->ReleaseStringUTFChars((jstring)jobj,str);
#else
	free((void*)str);
#endif
}

static
jstring
JShortcutNativeStringToJava(
	JNIEnv *env,
	char *str)
{
#ifdef USE_UTF_8
	jstring jstr;

	jstr = env->NewStringUTF(str);
	return jstr;
#else
	jbyteArray arr;
	jstring jstr;

	if (!str) {
		return env->NewStringUTF("");	//empty string
	}
	arr = env->NewByteArray(strlen(str));
	env->SetByteArrayRegion(arr,0,strlen(str),(signed char*)str);

	// now use String(byte[]) constructor
	if (!StringClass) {
		char *className = "java/lang/String";
		StringClass = env->FindClass(className);
		if (!StringClass) {
			fprintf(stderr,"Can't find class %s\n",className);
			return NULL;
		}
	}
	if (!StringClassBytesConstructor) {
		StringClassBytesConstructor = env->GetMethodID(StringClass,
				"<init>","([B)V");
		if (!StringClassBytesConstructor) {
			fprintf(stderr,
				"Can't find constructor String(byte[])\n");
			return NULL;
		}
	}
	jstr = (jstring)(env->NewObject(
			StringClass,StringClassBytesConstructor,arr));
	return jstr;
#endif
}

// Get a native string value from a JShellLink object String field.
static
char*
JShortcutGetNativeString(
	JNIEnv *env,
	jobject jobj,		// a JShellLink object
	char *fieldName,	// the name of the String field to get
	jobject *jfreeobjp)	// RETURN the associated jobject for freeing
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
	return JShortcutJavaStringToNative(env,fieldValue,jfreeobjp);
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

// Get a Java int value from a JShellLink object.
static
jint
JShortcutGetJavaInt(
	JNIEnv *env,
	jobject jobj,		// a JShellLink object
	char *fieldName)	// the name of the String field to get
{
	jfieldID fid;
	jint fieldValue;

	if (!JShellLinkClass) {
		char *className = "net/jimmc/jshortcut/JShellLink";
		JShellLinkClass = env->FindClass(className);
		if (!JShellLinkClass) {
			fprintf(stderr,"Can't find class %s\n",className);
			return NULL;
		}
	}
	fid = env->GetFieldID(JShellLinkClass,fieldName,intType);
	if (fid==0) {
		//TBD error, throw exception?
		fprintf(stderr,"Can't find field %s\n",fieldName);
		return NULL;
	}
	fieldValue = env->GetIntField(jobj,fid);
	return fieldValue;
}

// Set a Java int value into a JShellLink object.
static
int			// 0 if error, 1 if OK
JShortcutSetJavaInt(
	JNIEnv *env,
	jobject jobj,		// a JShellLink object
	char *fieldName,	// the name of the String field to set
	jint fieldValue)	// the value to set into that field
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
	fid = env->GetFieldID(JShellLinkClass,fieldName,intType);
	if (fid==0) {
		//TBD error, throw exception?
		fprintf(stderr,"Can't find field %s\n",fieldName);
		return 0;
	}
	env->SetIntField(jobj,fid,fieldValue);
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

// Save a new shell link
static
HRESULT			// status: test using FAILED or SUCCEEDED macro
JShortcutSave(		// Save a shortcut
	const char *folder,	// The directory in which to create the shortcut
	const char *name,	// Base name of the shortcut
	const char *description,// Description of the shortcut
	const char *path,	// Path to the target of the link
	const char *args,	// Arguments for the target of the link
	const char *workingDir,	// Working directory for the shortcut
	const char *iconLoc,	// Path to file containing icon
	const int iconIndex	// Index of icon within the icon file
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

	// Load the file if it exists, to get the values for anything
	// that we do not set.  Ignore errors, such as if it does not exist.
	h = persistFile->Load(wName, 0);

	// Set the fields for which the application has set a value
	if (description!=NULL)
		shellLink->SetDescription(description);
	if (path!=NULL)
		shellLink->SetPath(path);
	if (args!=NULL)
		shellLink->SetArguments(args);
	if (workingDir!=NULL)
		shellLink->SetWorkingDirectory(workingDir);
	if (iconLoc!=NULL)
		shellLink->SetIconLocation(iconLoc,iconIndex);

	//One example elsewhere adds this line:
	//shellLink->SetShowCommand(SW_SHOW);

	//Save the shortcut to disk
	h = persistFile->Save(wName, TRUE);
	if (FAILED(h)) {
		errStr = "Failed to save shortcut";
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

// Load an existing shell link
static
HRESULT			// status: test using FAILED or SUCCEEDED macro
JShortcutLoad(		// Load a shortcut
	const char *folder,	// The directory in which to create the shortcut
	const char *name,	// Base name of the shortcut
	char *description,	// RETURN Description of the shortcut
	const int descriptionSize,
	char *path,		// RETURN Path to the target of the link
	const int pathSize,
	char *args,		// RETURN Path to the target of the link
	const int argsSize,
	char *workingDir,	// RETURN Working directory for the shortcut
	const int workingDirSize,
	char *iconLoc,		// RETURN Icon location
	const int iconLocSize,
	int *iconIndex		// RETURN Icon index
		//All returned chars are written into the caller's buffers
)
{
	char *errStr = NULL;
	HRESULT h;
	IShellLink *shellLink = NULL;
	IPersistFile *persistFile = NULL;
	TCHAR buf[MAX_PATH+1];
	WORD wName[MAX_PATH+1];

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

	//Load the shortcut From disk
	h = persistFile->Load(wName, 0);
	if (FAILED(h)) {
		errStr = "Failed to load shortcut";
		goto err;
	}

	//Read the field values
	h = shellLink->GetDescription(description,descriptionSize);
	if (FAILED(h)) {
		errStr = "Failed to read description";
		goto err;
	}

	h = shellLink->GetPath(path,pathSize,NULL,SLGP_UNCPRIORITY);
	if (FAILED(h)) {
		errStr = "Failed to read path";
		goto err;
	}

	h = shellLink->GetArguments(args, argsSize);
	if (FAILED(h)) {
		errStr = "Failed to read arguments";
		goto err;
	}

	h = shellLink->GetWorkingDirectory(workingDir,workingDirSize);
	if (FAILED(h)) {
		errStr = "Failed to read working directory";
		goto err;
	}

	h = shellLink->GetIconLocation(iconLoc,iconLocSize,iconIndex);
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

// Save a shell link (shortcut) from Java.
JNIEXPORT jboolean JNICALL
Java_net_jimmc_jshortcut_JShellLink_nSave(
	JNIEnv *env,
	jobject jobj)	//this
{
	jobject jFolder, jName, jDesc, jPath, jArgs, jWorkingDir, jIconLoc;
	jint iconIndex;
	const char *folder, *name, *desc, *path, *args, *workingDir, *iconLoc;

	JShortcutClearClasses();

	folder = JShortcutGetNativeString(env,jobj,"folder",&jFolder);
	name = JShortcutGetNativeString(env,jobj,"name",&jName);
	desc = JShortcutGetNativeString(env,jobj,"description",&jDesc);
	path = JShortcutGetNativeString(env,jobj,"path",&jPath);
	args = JShortcutGetNativeString(env,jobj,"arguments",&jArgs);
	workingDir = JShortcutGetNativeString(env,jobj,"workingDirectory",
							&jWorkingDir);
	iconLoc = JShortcutGetNativeString(env,jobj,"iconLocation",&jIconLoc);
	iconIndex = JShortcutGetJavaInt(env,jobj,"iconIndex");

	if (folder==NULL || name==NULL)
		return false;		//error, incompletely specified


	HRESULT h = JShortcutSave(folder,name,desc,path,args,workingDir,
			iconLoc,iconIndex);

	JShortcutReleaseNativeString(env,jFolder,folder);
	JShortcutReleaseNativeString(env,jName,name);
	JShortcutReleaseNativeString(env,jDesc,desc);
	JShortcutReleaseNativeString(env,jPath,path);
	JShortcutReleaseNativeString(env,jArgs,args);
	JShortcutReleaseNativeString(env,jWorkingDir,workingDir);
	JShortcutReleaseNativeString(env,jIconLoc,iconLoc);

	return SUCCEEDED(h);
}

// Load a shell link (shortcut) from Java.
JNIEXPORT jboolean JNICALL
Java_net_jimmc_jshortcut_JShellLink_nLoad(
	JNIEnv *env,
	jobject jobj)	//this
{
	jobject jFolder, jName;
	jstring jDesc, jPath, jArgs, jWorkingDir, jIconLoc;
	const char *folder, *name;
	char desc[MAX_PATH+1];
	char path[MAX_PATH+1];
	char args[MAX_PATH+1];
	char workingDir[MAX_PATH+1];
	char iconLoc[MAX_PATH+1];
	int iconIndex;

	JShortcutClearClasses();

	folder = JShortcutGetNativeString(env,jobj,"folder",&jFolder);
	name = JShortcutGetNativeString(env,jobj,"name",&jName);

	if (folder==NULL || name==NULL)
		return false;		//error, incompletely specified

	HRESULT h = JShortcutLoad(folder,name,
			desc,sizeof(desc),
			path,sizeof(path),
			args,sizeof(args),
			workingDir,sizeof(workingDir),
			iconLoc,sizeof(iconLoc),&iconIndex);

	//TBD - if the shortcut does not exist, what should we do?

	JShortcutReleaseNativeString(env,jFolder,folder);
	JShortcutReleaseNativeString(env,jName,name);

	jDesc = JShortcutNativeStringToJava(env,desc);
	jPath = JShortcutNativeStringToJava(env,path);
	jArgs = JShortcutNativeStringToJava(env,args);
	jWorkingDir = JShortcutNativeStringToJava(env,workingDir);
	jIconLoc = JShortcutNativeStringToJava(env,iconLoc);

	JShortcutSetJavaString(env,jobj,"description",jDesc);
	JShortcutSetJavaString(env,jobj,"path",jPath);
	JShortcutSetJavaString(env,jobj,"arguments",jArgs);
	JShortcutSetJavaString(env,jobj,"workingDirectory",jWorkingDir);
	JShortcutSetJavaString(env,jobj,"iconLocation",jIconLoc);
	JShortcutSetJavaInt(env,jobj,"iconIndex",iconIndex);

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
	jstring jstr;

	JShortcutClearClasses();

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
	jstr = JShortcutNativeStringToJava(env,buf);
	return jstr;
}
