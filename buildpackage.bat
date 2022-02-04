"c:\Program Files\WinRAR\winRar.exe" a browedit3.zip imgui.ini
"c:\Program Files\WinRAR\winRar.exe" a browedit3.zip BrowEdit3.exe
"c:\Program Files\WinRAR\winRar.exe" a browedit3.zip BrowEdit3.pdb
"c:\Program Files\WinRAR\winRar.exe" a -r browedit3.zip doc\*
"c:\Program Files\WinRAR\winRar.exe" a -r browedit3.zip data\*
xcopy /y browedit3.zip w:\browedit\
cd ..
pause