/* Copyright 2002 Jim McBeath under GNU GPL v2 */

package net.jimmc.jshortcut;

import java.io.File;
import java.io.FileOutputStream;
import java.io.InputStream;
import java.io.OutputStream;

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
    String folder;	//accessed from native code by name

    // The base name of this shortcut within the folder.
    // @see #getName
    String name;	//accessed from native code by name

    // The description of this shortcut.
    // @see #getDescription
    String description;	//accessed from native code by name

    // The path or target of this shortcut.
    // @see #getPath
    String path;	//accessed from native code by name

    // The arguments (after path)
    // @see #getArguments
    String arguments;	//accessed from native code by name

    // The working directory for the shortcut.
    String workingDirectory;	//accessed from native code by name

    // The location of the icon for the shortcut.
    String iconLocation;	//accessed from native code by name

    // The index of the icon within the specified location.
    int iconIndex;		//accessed from native code by name

    // Load our native library from PATH or CLASSPATH when this class is loaded.
    static {
	// The CLASSPATH searching code below was written by Jim McBeath
	// and contributed to the jRegistryKey project,
	// after which it was modified and used here.
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
            try {
            ClassLoader cl = JShellLink.class.getClassLoader();

            String libname = "jshortcut_" + System.getProperty("os.arch") + ".dll";

            InputStream in = cl.getResourceAsStream(libname);
            if (in == null)
                throw new Exception("libname: jshortcut.dll not found");
            File tmplib = File.createTempFile("jshortcut-",".dll");
            tmplib.deleteOnExit();
            OutputStream out = new FileOutputStream(tmplib);
            byte[] buf = new byte[1024];
            for (int len; (len = in.read(buf)) != -1;)
                out.write(buf, 0, len);
            in.close();
            out.close();

            System.load(tmplib.getAbsolutePath());

            foundIt = Boolean.TRUE;

            System.out.println("jshortcut.dll loaded via tmp generated pathname: " + tmplib.getAbsolutePath());
            
            } catch (Exception e) {
              foundIt = Boolean.FALSE;
            }
        }

	     if (!foundIt) {
		 // we did not find it in CLASSPATH
             System.out.println("failed loading jshortcut.dll");

		 throw ex;	// throw the can't-find-library exception
	     }
	}
    }

    /** Get the requested directory.
     * @param dirtype One of the following special strings:
     * <ul>
     * <li>common_desktopdirectory -
     *     the All Users desktop folder (not on earlier Windows)
     * <li>common_programs - the All Users programs (not on earlier Windows)
     * <li>desktop - the Desktop
     * <li>personal - the My Documents folder
     * <li>programs - the Start Menu/Programs folder
     * <li>program_files - the Program Files folder
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

    /** Create a JShellLink object for a specified location.
     */
    public JShellLink(String folder, String name) {
    	this();
	setFolder(folder);
	setName(name);
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

    /** Set the path for this shortcut.
     * If the working directory is null, this call also sets the
     * working directory to the parent of the given path.
     */
    public void setPath(String path) {
        this.path = path;
	if (workingDirectory==null) {
	    String parent = (new File(path)).getParent();
	    setWorkingDirectory(parent);
	}
    }

    /** Get the path for this shortcut. */
    public String getPath() {
        return path;
    }

    /** Set the arguments for this shortcut. */
    public void setArguments(String arguments) {
        this.arguments = arguments;
    }

    /** Get the arguments for this shortcut. */
    public String getArguments() {
        return arguments;
    }

    /** Set the working directory for this shortcut. */
    public void setWorkingDirectory(String workingDirectory) {
        this.workingDirectory = workingDirectory;
    }

    /** Get the working directory for this shortcut. */
    public String getWorkingDirectory() {
        return workingDirectory;
    }

    /** Set the icon location for this shortcut. */
    public void setIconLocation(String iconLocation) {
        this.iconLocation = iconLocation;
    }

    /** Get the icon location for this shortcut. */
    public String getIconLocation() {
        return iconLocation;
    }

    /** Set the icon index for this shortcut. */
    public void setIconIndex(int iconIndex) {
        this.iconIndex = iconIndex;
    }

    /** Get the icon index for this shortcut. */
    public int getIconIndex() {
        return iconIndex;
    }

    /** Load a shortcut from disk.
     */
    public void load() {
        if (!nLoad()) {
	    throw new RuntimeException("Failed to load ShellLink");
	    	//TBD - better error info
	}
    }

    /** Write out this shortcut to disk. */
    public void save() {
        if (!nSave()) {
	    throw new RuntimeException("Failed to save ShellLink");
	    	//TBD - better error info
	}
    }

  //Native methods

    /** Load a shortcut.
     * The native code reads the following variables from this object:
     * folder, name.
     * The native code fills in the following variables in this object:
     * description, path, workingDirectory, iconLocation, iconIndex.
     */
    private native boolean nLoad();

    /** Save a shortcut.
     * The native code reads the following variables from this object:
     * folder, name, description, path, workingDirectory,
     * iconLocation, iconIndex.
     */
    private native boolean nSave();

    /** Get the location of a special directory.
     */
    private static native String nGetDirectory(String dirtype);

  //End native methods

    public static void main(String argv[] )
    {
        JShellLink link = new JShellLink();        

        System.out.println(JShellLink.getDirectory("desktop"));

        System.out.println("success");
    }
}
