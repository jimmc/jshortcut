/* Copyright 2002 Jim McBeath under GNU GPL v2 */

package net.jimmc.jshortcut;

import java.io.File;

/** Provide access to shortcuts (shell links) from Java.
 *
 * The native library (jshortcut.dll) is loaded when JShellLink is first
 * loaded.
 * By default, JShellLink first looks for the native library in the PATH,
 * using System.loadLibrary.
 * If the native library is not found in the PATH,
 * JShellLink then looks through each directory in the CLASSPATH
 * (as determined by the value of the system property java.class.path).
 * If an entry in the CLASSPATH is a jar file,
 * then JShellLink looks for the native library
 * in the directory containing that jar file.
 * The application can override this behavior and force JShellLink to look
 * for the native library in a specific directory by setting the system
 * property JSHORTCUT_HOME to point to that directory.
 * This property must be set before the JShellLink class is loaded.
 * This makes it possible to use this library from a self-extracting jar file.
 */
public class JShellLink {
    /// The folder in which this shortcut is found on disk.
    // @see #getFolder
    String folder;

    // The base name of this shortcut within the folder.
    // @see #getName
    String name;

    // The description of this shortcut.
    // @see #getDescription
    String description;

    // The path or target of this shortcut.
    // @see #getPath
    String path;

    // Load our native library from PATH or CLASSPATH when this class is loaded.
    static {
	// The CLASSPATH searching code below was written by Jim McBeath
	// and contributed to the jRegistryKey project,
	// after which it was also used here.
	try {
	    String appDir = System.getProperty("JSHORTCUT_HOME");
	    	// allow application to specify our JNI location
	    if (appDir!=null) {
	        // application told us to look in $JSHORTCUT_HOME for our dll
		File f = new File(appDir,"jshortcut.dll");
		String path = f.getAbsolutePath();
		System.load(path);	// load JNI code
	    } else {
	        // No specified location for DLL, look through PATH
	        System.loadLibrary("jshortcut");
	    }
	} catch (UnsatisfiedLinkError ex) {
	    // didn't find it in our PATH, look for it in CLASSPATH
	    String cp = System.getProperty("java.class.path");
	    boolean foundIt = false;
	    while (cp.length() > 0) {
		int x = cp.indexOf(File.pathSeparator);
		String dir;
		if (x >= 0) {
		    dir = cp.substring(0,x);
		    cp = cp.substring(x+1);
		} else {
		    dir = cp;
		    cp = "";
		}
		if (dir.length()>4 &&
		    dir.substring(dir.length()-4).toLowerCase().equals(".jar")){
		    // If the classpath contains a jar file,
		    // then we look in the directory
		    // containing the jar file.
		    x = dir.lastIndexOf(File.separator);
		    if (x>0)
			dir = dir.substring(0,x);
		    else
			dir = ".";
		}
		File f = new File(dir,"jshortcut.dll");
		if (f.exists()) {
		    String path = f.getAbsolutePath();
		    System.load(path);	// load JNI code
		    foundIt = true;
		    break;
		}
	     }
	     if (!foundIt) {
		 // we did not find it in CLASSPATH
		 throw ex;	// throw the can't-find-library exception
	     }
	}
    }

    /** Get the requested directory.
     * @param dirtype One of the following special strings:
     * <ul>
     * <li>desktop - the Desktop
     * <li>personal - the My Documents folder
     * <li>program_files - the Program Files folder
     * <li>start_menu - the Start Menu
     * </ul>
     * @return The location of the special folder.
     */
    public static String getDirectory(String dirtype) {
        return nGetDirectory(dirtype.toLowerCase());
    }

    /** Create a JShellLink object with no values filled in.
     */
    public JShellLink() {
    }

    /** Create a JShellLink object with some values.
     */
    public JShellLink(String folder, String name, String description,
    			String path) {
    	this();
	setFolder(folder);
	setName(name);
	setDescription(description);
	setPath(path);
    }

    /** Set the folder for this shortcut. */
    public void setFolder(String folder) {
        this.folder = folder;
    }

    /** Get the folder for this shortcut. */
    public String getFolder() {
        return folder;
    }

    /** Set the shortcut base name for this shortcut. */
    public void setName(String name) {
        this.name = name;
    }

    /** Get the base name for this shortcut. */
    public String getName() {
        return name;
    }

    /** Set the description for this shortcut. */
    public void setDescription(String description) {
        this.description = description;
    }

    /** Get the description for this shortcut. */
    public String getDescription() {
        return description;
    }

    /** Set the path for this shortcut. */
    public void setPath(String path) {
        this.path = path;
    }

    /** Get the path for this shortcut. */
    public String getPath() {
        return path;
    }

    /** Write out this shortcut to disk. */
    public void save() {
        nCreate(folder,name,description,path);
    }

  //Native methods

    /** Create a shortcut.
     * @param folder The location of the shortcut.
     * @param name The base name of the shortcut.
     * @param description The description of the shortcut.
     * @param path The target of the shortcut.
     */
    private native boolean nCreate(String folder, String name,
		    String description, String path);

    /** Get the location of a special directory.
     */
    private static native String nGetDirectory(String dirtype);

  //End native methods
}