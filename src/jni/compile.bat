cl /I d:\jdk1.3\include /I d:\jdk1.3\include\win32 -c jshortcut.cpp

link /nologo /incremental:no /fixed:no /nod /dll /release /machine:ix86 /out:..\..\jshortcut.dll /def:jshortcut.def jshortcut.obj advapi32.lib shell32.lib ole32.lib uuid.lib libcmt.lib kernel32.lib 

erase ..\..\jshortcut.exp ..\..\jshortcut.lib
