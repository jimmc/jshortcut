#Makefile for jshortcut

PROJECT      := jshortcut
PKGS         := net.jimmc.jshortcut
MAINPKG      := net.jimmc.jshortcut

PKGDIRS      := $(subst .,/,$(PKGS))
MAINPKGDIR   := $(subst .,/,$(MAINPKG))
BASENAME     := $(PROJECT)
TOPDIR       := .
ARCHDIR      := Arch
MAKEINFODIR  := makeinfo
include $(MAKEINFODIR)/javadefs.mak
-include $(TOPDIR)/localdefs.mak

CLASSPATH    = obj
SOURCEPATH   = src

#MAIN refers to the main java sources, not including test sources
MAIN_SRCS    := $(wildcard $(PKGDIRS:%=src/%/*.java))
MAIN_OBJS    := $(MAIN_SRCS:src/%.java=obj/%.class)
MAIN_PROPS   := $(wildcard $(PKGDIRS:%=src/%/*.properties))
MAIN_MAKEFILES := $(PKGDIRS:%=src/%/Makefile)

#TEST refers to the java sources for unit testing
TEST_SRCS    := $(wildcard $(PKGDIRS:%=test/%/*.java))
TEST_OBJS    := $(TEST_SRCS:test/%.java=testobj/%.class)
TEST_PROPS   := $(wildcard $(PKGDIRS:%=test/%/*.properties))
TEST_MAKEFILES := $(PKGDIRS:%=test/%/Makefile)

NATIVE_SRCS  := src/jni/*.cpp src/jni/*.def src/jni/*.bat

#SRCS is all java source files, both regular and test
SRCS         := $(MAIN_SRCS) $(TEST_SRCS) $(JARINST_SRCS)
OBJS	     := $(MAIN_OBJS) $(TEST_OBJS)
PROPS	     := $(MAIN_PROPS) $(TEST_PROPS)
MAKEFILES    := $(MAIN_MAKEFILES) $(TEST_MAKEFILES)
SRCHTMLS     := $(wildcard $(PKGDIRS:%=src/%/*.html))

VERSION      := $(shell cat VERSION | awk '{print $$2}')
_VERSION     := $(shell cat VERSION | awk '{print $$2}' | \
			sed -e 's/\./_/g' -e 's/v//')
VDATE        := $(shell cat VERSION | awk '{print $$3, $$4, $$5}')
JARFILE      := $(BASENAME).jar
JARMANIFEST  := misc/manifest.mf
JAROBJS      := $(OBJS)

JAR_LIST_OBJS := $(PKGDIRS:%=%/*.class)
JAR_LIST_PROPS:= $(MAIN_PROPS:src/%=%)

RELDIR       := $(BASENAME)-$(_VERSION)
RELBINFILES  := README README.html VERSION LICENSE COPYRIGHT HISTORY \
		$(JARFILE) $(JNIFILE)
RELAPIDOC    := doc/api
RELSRCS      := README.build misc/manifest.mf \
		Makefile $(MAKEFILES) makeinfo/*.mak $(SRCS) \
		$(PROPS) $(SRCHTMLS) src/jimmc/selfunzip/*.mf

default:	jar jni

doc:		apidoc

all:		jar jni apidoc

objdir:;	[ -d obj ] || mkdir obj
		[ -d testobj ] || mkdir testobj

classfiles:	$(JAROBJS)

jar:		objdir $(JARFILE)

$(JARFILE):	$(JAROBJS) $(JARMANIFEST) jaronly

jaronly:;	cd obj && $(JAR) -cmf ../$(JARMANIFEST) ../$(JARFILE) \
			$(JAR_LIST_OBJS)
		#cd testobj && $(JAR) -uf ../$(JARFILE) $(JAR_LIST_OBJS)
		#cd test && $(JAR) -uf ../$(JARFILE) $(JAR_LIST_PROPS)

$(MAIN_OBJS):	javacmain

$(TEST_OBJS):	javactest

#javac:		javacmain javactest
javac:		javacmain

javacmain:;	@echo Building main classes
		@echo $(JAVAC) $(JAVAC_DEBUG_OPTS) \
			-classpath $(CLASSPATH) -d obj \
			-sourcepath $(SOURCEPATH) $(MAIN_SRCS)
		@$(JAVAC) $(JAVAC_DEBUG_OPTS) \
			-classpath $(CLASSPATH) -d obj \
			-sourcepath $(SOURCEPATH) $(MAIN_SRCS)

javactest:;	@echo Building test classes
		@echo $(JAVAC) $(JAVAC_DEBUG_OPTS) \
			-classpath $(TESTCLASSPATH) -d testobj \
			-sourcepath $(TESTSOURCEPATH) $(TEST_SRCS)
		@$(JAVAC) $(JAVAC_DEBUG_OPTS) \
			-classpath $(TESTCLASSPATH) -d testobj \
			-sourcepath $(TESTSOURCEPATH) $(TEST_SRCS)

#The JNI tag builds the Windows DLL, so only works on a Windows machine
jni:;		cd src/jni && make

#runtest:;	cd test/$(MAINPKGDIR) && $(MAKE) alltest

apidoc:		docdir app_doc

docdir:;	[ -d doc/api ] || mkdir doc/api

app_doc:;	$(JAVADOC) -J-mx50m -d doc/api -classpath $(CLASSPATH) \
			-sourcepath $(SOURCEPATH) \
			-overview src/jimmc/racer/overview.html \
			$(PKGS)

#Make the release directory, including source and docs
rel:		relbin relsrc relapidoc

#Make the release directory with just the binary-kit files
relbin:;	mkdir $(RELDIR)
		cp -p $(RELBINFILES) $(RELDIR)
		mkdir $(RELDIR)/data
		cp -p $(RELDATAFILES) $(RELDIR)/data
		tar cf - $(RELDOCFILES) | (cd $(RELDIR) && tar xf -)

#Make the zip file of sources in the release directory
relsrc:;	zip -r $(RELDIR)/src.zip $(RELSRCS)

#Make the zip file of api doc files in the release directory
relapidoc:;	zip -r $(RELDIR)/apidoc.zip $(RELAPIDOC)

#After making the release directory, make a self-extracting jar file for it
reljar:;	zip -r $(RELDIR).jar $(RELDIR)
		(cd src/jimmc/selfunzip; $(JAR) ufm ../../../$(RELDIR).jar \
			manifest.mf )
		(cd obj; $(JAR) uf ../$(RELDIR).jar \
			jimmc/selfunzip/ZipSelfExtractor.class )

#After making reljar, copy that files and the README file into
#the archive directory with the appropriate names.
arch:;		mkdir $(ARCHDIR)/$(VERSION)	#Fail if it already exists
		cp -p $(RELDIR).jar $(ARCHDIR)/$(VERSION)
		cp -p README.html $(ARCHDIR)/$(VERSION)/README-$(_VERSION).html
		chmod -w $(ARCHDIR)/$(VERSION)/$(RELDIR).* \
			$(ARCHDIR)/$(VERSION)/README-$(_VERSION).html
		chmod -w $(ARCHDIR)/$(VERSION)

#Set the CVS tag for the current release
cvstag:;	cvs tag $(BASENAME)-$(_VERSION)

#Clean only removes the files made by "make" (default).
#Use apiclean to remove files made by "make apidoc",
#use allclean to remove files made by "make all",
clean:;		rm -f $(JARFILE)
		rm -rf obj/* testobj/*

#Remove file files generated by "make all"
allclean:	clean apiclean

#Remove the files generated by "make apidoc"
apiclean:;	rm -rf doc/api

wc:;		wc $(SRCS)

wcmain:;	wc $(MAIN_SRCS)

wctest:;	wc $(TEST_SRCS)
