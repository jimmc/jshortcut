package net.jimmyc.jshortcut;

import java.io.File;

import net.jimmc.jshortcut.JShellLink;

import org.junit.Assert;
import org.junit.Test;

public class JShellLinkTest {

	@Test
    public void testDirLookup()
    {
		File fileLink = new File("src/test/resources/ref.lnk");
		
        JShellLink link = new JShellLink(fileLink.getParentFile().getAbsolutePath(),fileLink.getName().replaceFirst(".lnk$", ""));   
        link.load();
        Assert.assertEquals("C:\\test\\a.txt",link.getPath());
        Assert.assertEquals("C:\\test",link.getWorkingDirectory());

        Assert.assertEquals("C:\\Users\\" + System.getenv("username") + "\\Desktop", JShellLink.getDirectory("desktop"));
    }
}