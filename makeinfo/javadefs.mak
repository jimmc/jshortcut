#Include this piece of makefile to set up defs for java stuff

#Change these definitions to point to the location on your machine
#containing these files
JDKVER        = j2sdk1.4.0

#You can changes these definitions, or you can create localdefs.mak in the
#top-level directory and put your definitions there.
JDK           = /u/java/$(JDKVER)
JRE           = $(JDK)/jre

HOSTNAME = $(shell uname -n)

#These definitions are built on the previous definitions.
#You should not need to change them.
JAVA          = $(JDK)/bin/java
JAVAC         = $(JDK)/bin/javac
JAR           = $(JDK)/bin/jar
JAVADOC       = $(JDK)/bin/javadoc

JUNITJAR      = $(JUNITDIR)/junit.jar
