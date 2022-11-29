echo off

set PATH_7ZIP="C:\Program Files\7-Zip"
set /p suffix="Enter suffix >"

rem set time2=%time: =0%
rem set fn=%date:~-10,4%%date:~-5,2%%date:~-2,2%%time2:~0,2%%time2:~3,2%%time2:~6,2%
set fn=%date:~-10,4%%date:~-5,2%%date:~-2,2%
set srcdir=.
set dstdir1=..\_Backup\rp2040_trackball_mouse
set dstdir2=rp2040_trackball_mouse_%fn%%suffix%
set dstdir=%dstdir1%\%dstdir2%

robocopy "%srcdir%" "%dstdir%" /MIR /S /XA:SH /XF *.o *.d *.a *.elf *.uf2 /XD .svn .git .vs .settings .metadata bin obj Debug Release packages build

cd %dstdir1%

del %dstdir2%.7z
%PATH_7ZIP%\7z a -mx9 %dstdir2%.7z %dstdir2%
