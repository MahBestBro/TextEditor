@echo off

set commonCompilerFlags= -nologo -MDd -Gm- -GR- -EHa- -Od -Oi -WX -W4 -wd4201 -wd4100 -FC -Z7 
set commonLinkerFlags= -incremental:no -opt:ref  
set libraries= user32.lib gdi32.lib shell32.lib Comdlg32.lib freetyped.lib
set includeDirs= includes
set cppFiles= code\TextEditor_win32.cpp code\TextEditor.cpp code\TextEditor_input.cpp code\TextEditor_string.cpp code\TextEditor_config.cpp

rem -GR- and -EHa- turn off exception handling stuff
rem -Oi turns on commpiler intrisics (replaces c functions if it knows how to do it in assembly)
rem -Z7 and -Zi are for debugging. -Z7 does not produce vc140.pdb
rem -nologo gets rid of compiler logo
rem -MT gets rid of multithread dll bug, is used for release builds
rem -Gm- turns off minimal rebuild
rem -Od turns off all optimisation
rem -incremental:no turns off incremental build
rem -FC uses full path for error and warning messages

rem IGNORED WARNINGS:
rem - 4100: Unused function parameters (does not work with winmain or win32 callback)
rem - 4201: non-standard anonymous struct or union (we use them) 
 

mkdir config_code_generator 
cls
pushd config_code_generator 

rem Config Preprocessor
del *.pdb > NUL 2> NUL
cl  %commonCompilerFlags% /EHsc ../code/TextEditor_config_code_generator.cpp /I %includeDirs% /link %commonLinkerFlags% -libpath:"../lib" %libraries% /out:TextEditor_config_code_generator.exe
TextEditor_config_code_generator.exe

popd

rem Debug build
del *.pdb > NUL 2> NUL
cl  %commonCompilerFlags% /EHsc %cppFiles% /I %includeDirs% /link %commonLinkerFlags% -libpath:"lib" %libraries% /out:TextEditor.exe  



rem /EHsc adds in error handler
rem /LD builds as DLL when using "cd" command
rem /PDB:(name).pdb creates a pdb with the file name
rem %date:~-4,4%%date:~-10,2%%date:~-7,2% is noise for visual studio pdb bypassing
