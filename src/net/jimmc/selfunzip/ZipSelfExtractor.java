// From http://www.javaworld.com/javaworld/javatips/jw-javatip120.html
// Modified by jimmc for JShortcut

/* ZipSelfExtractor.java */
/* Author: Z.S. Jin
   Updates: John D. Mitchell */

package net.jimmc.selfunzip;

import net.jimmc.jshortcut.JShellLink;

import java.io.*;
import java.net.*;
import javax.swing.*;
import java.util.zip.*;
import java.util.*;
import java.text.*;

public class ZipSelfExtractor extends JFrame
{
    private String myClassName;
    private String jsClassName;
    static String MANIFEST = "META-INF/MANIFEST.MF";
    private File tmpDllFile;

    public static void main(String[] args)
    {
	ZipSelfExtractor zse = new ZipSelfExtractor();
	String jarFileName = zse.getJarFileName();
	zse.extract(jarFileName);
	System.exit(0);
    }

    ZipSelfExtractor()
    {
    }

    private String getJarFileName()
    {
	jsClassName = "net/jimmc/jshortcut/JShellLink.class";
	myClassName = this.getClass().getName().replace('.','/') + ".class";
	URL urlJar =
		this.getClass().getClassLoader().getSystemResource(myClassName);
	String urlStr = urlJar.toString();
	int from = "jar:file:".length();
	int to = urlStr.indexOf("!/");
	return urlStr.substring(from, to);
    }

    //True if we are running on Windows
    private boolean isWindows() {
    	return (File.separatorChar=='\\');
    }

    //Extract the program name and the version number from the file name
    private String getProgramAndVersion(String filename) {
        int xx = filename.lastIndexOf('.');
	if (xx>0) {
		String ext = filename.substring(xx+1).toLowerCase();
		if (ext.equals("jar"))
			filename = filename.substring(0,xx);
	}
    	int dx = filename.indexOf('-');
	if (dx<0)
		dx = filename.indexOf('_');	//on CD, we use _ instead of -
	if (dx<0)
		return filename;	//can't figure it out
	String prog = filename.substring(0,dx);
	if (prog.equalsIgnoreCase("jshortcut") || prog.equalsIgnoreCase("JS"))
		prog = "JShortcut";
	String ver = filename.substring(dx+1).replace('_','.');
	return prog+" "+ver;
    }

    //Extract the version number suffix from the file name.
    //The string includes underscore rather than dot.
    private String getVersionSuffix(String filename) {
	int vx = filename.indexOf("-");
	if (vx<0)
	    vx = filename.indexOf("_");
	if (vx<0)
	    return "X_X_X";		//can't figure it out
	String version = filename.substring(vx+1);
	vx = version.lastIndexOf(".");
	if (vx>0)
	    version= version.substring(0,vx);
	return version;
    }

    //Get the directory in which to unpack our file
    //@param filename The name of the JAR file from which we are unpacking.
    File getInstallDir(String filename) {
	String programAndVersion = getProgramAndVersion(filename);
	String defaultInstallDir;
	if (isWindows()) {
	    defaultInstallDir = JShellLink.getDirectory("program_files");
	} else {
	    defaultInstallDir = System.getProperty("user.home");
	}

	String versionSuffix = getVersionSuffix(filename);
	File dfltFile = new File(defaultInstallDir);
	String msg = "This installer will create the directory\n"+
		"   jshortcut-"+versionSuffix+"\n"+
		"within the install directory you select.\n"+
		" \n"+
		"The recommended install directory is\n"+
	    	"   "+defaultInstallDir+"\n";

	if (dfltFile.exists()) {
	    msg += " \n"+
	        "Would you like to install into that directory now?";
	} else {
	    msg += "which does not exist.\n"+
	        " \n"+
		"Would you like to create that directory and install into it now?";
	}
	String title = "Installing "+programAndVersion;
	switch (JOptionPane.showConfirmDialog(this,msg,title,
			JOptionPane.YES_NO_CANCEL_OPTION)) {
	case JOptionPane.YES_OPTION:
	    if (dfltFile.exists())
	        return dfltFile;	//already there, use it
	    if (!dfltFile.mkdirs()) {
		msg = "Unable to create directory\n"+
			defaultInstallDir;
		title="Error Creating Directory";
		JOptionPane.showMessageDialog(this,msg,title,
			JOptionPane.ERROR_MESSAGE);
		break;	//let user choose some other directory
	    }
	    return dfltFile;	//use the default install dir
	case JOptionPane.NO_OPTION:
	    break;		//let user choose a directory
	case JOptionPane.CANCEL_OPTION:
	    return null;
	}

	JFileChooser fc = new JFileChooser();
        fc.setCurrentDirectory(new File(defaultInstallDir));
        fc.setDialogType(JFileChooser.OPEN_DIALOG);
        fc.setDialogTitle("Select destination directory for extracting " +
		programAndVersion);
        fc.setMultiSelectionEnabled(false);

	fc.setFileSelectionMode(JFileChooser.DIRECTORIES_ONLY);

        if (fc.showDialog(ZipSelfExtractor.this, "Select")
	    != JFileChooser.APPROVE_OPTION)
        {
            return null;	//user cancelled
        }

        return fc.getSelectedFile();
    }

    /** Set up the native library we use during installation.
     * This involves the following steps:
     * <ol>
     * <li>Get the tmp dir location from property java.io.tmpdir
     * <li>Extract jshortcut.dll into that directory
     * <li>Set JSHORTCUT_HOME to that directory
     * <li>Instantiate a JShellLink to load the library
     * </ol>
     */
    void setupNativeLibrary(String filename, ZipFile zf)
    		throws IOException, FileNotFoundException {
        String tmpDir = System.getProperty("java.io.tmpdir");
	if (tmpDir==null) {
	    //TBD - error, no tmp directory available
	}

	//Open the entry for our native library
	String versionSuffix = getVersionSuffix(filename);
	String dllName = "jshortcut-"+versionSuffix+"/jshortcut.dll";
	ZipEntry ze = zf.getEntry(dllName);
	InputStream in = zf.getInputStream(ze);
	tmpDllFile = new File(tmpDir, "jshortcut.dll");
	FileOutputStream out = new FileOutputStream(tmpDllFile);

	//Copy the DLL from the zip file to the tmp dir
        byte[] buf = new byte[1024];
	while (true)
	{
	    int nRead = in.read(buf, 0, buf.length);
	    if (nRead <= 0)
		break;
	    out.write(buf, 0, nRead);
	}
	out.close();
	//We don't care about the date on the file

        System.setProperty("JSHORTCUT_HOME",tmpDir);
	new JShellLink();	//force the native library to load now
    }

    public void extract(String zipfile)
    {
	File currentArchive = new File(zipfile);

        byte[] buf = new byte[1024];
        SimpleDateFormat formatter =
		new SimpleDateFormat("yyyy-MM-dd HH:mm:ss",Locale.getDefault());

        ProgressMonitor pm = null;

        boolean overwrite = false;
	boolean stopped = false;

	ZipFile zf = null;
	FileOutputStream out = null;
	InputStream in = null;

        try
        {
	        zf = new ZipFile(currentArchive);

		if (isWindows()) {
		    //extract and set up the native library for installation
		    setupNativeLibrary(zipfile,zf);
		}

		String programAndVersion =
			getProgramAndVersion(currentArchive.getName());
		File outputDir = getInstallDir(currentArchive.getName());
		if (outputDir==null)
			return;		//cancelled

		int size = zf.size();
		int extracted = 0;
		int skipped = 0;
		int fullExtractionCount = size;

		String spaces =
		        "                                        "+
			"                                        ";
		pm = new ProgressMonitor(getParent(),
			"Extracting files...",
			"starting"+spaces,0, size-4);
		pm.setMillisToDecideToPopup(0);
		pm.setMillisToPopup(0);

		Enumeration entries = zf.entries();

		for (int i=0; i<size; i++)
		{
		    ZipEntry entry = (ZipEntry) entries.nextElement();
		    if(entry.isDirectory()) {
			fullExtractionCount--;
			continue;
		    }

		    String pathname = entry.getName();
		    if(myClassName.equals(pathname) ||
		       MANIFEST.equals(pathname.toUpperCase()) ||
		       jsClassName.equals(pathname)) {
			fullExtractionCount--;
			continue;
		    }

                    pm.setProgress(i);
                    pm.setNote(pathname);
		    if(pm.isCanceled()) {
			stopped = true;
			break;
		    }

                    in = zf.getInputStream(entry);

                    File outFile = new File(outputDir, pathname);
		    Date archiveTime = new Date(entry.getTime());

                    if(overwrite==false)
                    {
                        if(outFile.exists())
                        {
                            Object[] options =
			    	{"Yes", "Yes To All", "No", "Cancel"};
                            Date existTime = new Date(outFile.lastModified());
                            Long archiveLen = new Long(entry.getSize());

                            String msg = "File name conflict: "
				+ "There is already a file with "
				+ "that name on the disk!\n"
				+ "\nFile name: " + outFile.getName()
				+ "\nDestination: " + outFile.getPath()
				+ "\nExisting file: "
				+ formatter.format(existTime) + ",  "
				+ outFile.length() + "Bytes"
                                + "\nFile in archive:"
				+ formatter.format(archiveTime) + ",  "
				+ archiveLen + "Bytes"
				+"\n\nWould you like to overwrite the file?";

                            int result = JOptionPane.showOptionDialog(
			        ZipSelfExtractor.this,
				msg, "Warning", JOptionPane.DEFAULT_OPTION,
				JOptionPane.WARNING_MESSAGE, null,
				options,options[0]);

			    if (result==3) {
			    	//Stop the install
				stopped = true;
				break;
			    }

                            if(result == 2) // No, skip this file
                            {
			        skipped++;
                                continue;
                            }
                            else if( result == 1) //YesToAll
                            {
                                overwrite = true;
                            }
                        }
                    }

                    File parent = new File(outFile.getParent());
                    if (parent != null && !parent.exists())
                    {
                        parent.mkdirs();
                    }

                    out = new FileOutputStream(outFile);

                    while (true)
                    {
                        int nRead = in.read(buf, 0, buf.length);
                        if (nRead <= 0)
                            break;
                        out.write(buf, 0, nRead);
                    }
		    extracted ++;

                    out.close();
		    outFile.setLastModified(archiveTime.getTime());
                }

                pm.close();
                zf.close();

	        if (tmpDllFile!=null) {
		    tmpDllFile.delete();	//delete tmp file
		}

                //getToolkit().beep();

		String stoppedMsg = "";
		if (stopped) {
		    stoppedMsg = "Stopped, extraction incomplete.\n \n";
		}

		String skippedMsg = "";
		if (skipped>0) {
		    skippedMsg = " and skipped "+skipped+" file"+
		    	((skipped>1)?"s":"");
		}

		String outOfMsg = "";
		if (extracted+skipped<fullExtractionCount) {
		    outOfMsg = " out of "+fullExtractionCount;
		}

		String currentArchiveName = currentArchive.getName();
		String version = getVersionSuffix(currentArchiveName);
		String targetDir = outputDir.getPath()+File.separator+
			"jshortcut-"+version;
		String title = "Installed "+programAndVersion;
		String msg = stoppedMsg +
		     "Extracted " + extracted +
		     " file" + ((extracted != 1) ? "s": "") +
		     skippedMsg + outOfMsg +
		     " from the\n" +
		     zipfile + "\narchive into the\n" +
		     targetDir +
		     "\ndirectory.";
		JOptionPane.showMessageDialog
			(ZipSelfExtractor.this,msg,title,
			 JOptionPane.INFORMATION_MESSAGE);
	}
	catch (Exception e)
	{
	    System.out.println(e);
	    if(zf!=null) { try { zf.close(); } catch(IOException ioe) {;} }
	    if(out!=null) { try {out.close();} catch(IOException ioe) {;} }
	    if(in!=null) { try { in.close(); } catch(IOException ioe) {;} }
	}
    }
}
