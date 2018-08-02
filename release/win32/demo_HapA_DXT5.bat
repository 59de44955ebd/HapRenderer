::::::::::::::::::::::::::::::::::::::::
:: HapRenderer Demo - DXT5
:: Note: this demo only works if you have the Renderheads Hap codec installed
::::::::::::::::::::::::::::::::::::::::
@echo off
cd "%~dp0"

:: CLSID constants
set CLSID_AsyncReader={E436EBB5-524F-11CE-9F53-0020AF0BA770}
set CLSID_AVISplitter={1B544C20-FD0B-11CE-8C63-00AA0044B51E}
set CLSID_AVIDecompressor={CF49D4E0-1115-11CE-B03A-0020AF0BA770}
set CLSID_HapRenderer={7DB57FC5-6810-449F-935B-BC9EA3631560}

:: render sample-1080p30-HapA.avi with HapRenderer
bin\dscmd^
 -graph ^
%CLSID_AsyncReader%;src=..\assets\sample-1080p30-HapA.avi,^
%CLSID_AVISplitter%,^
%CLSID_AVIDecompressor%,^
%CLSID_HapRenderer%;file=HapRenderer.ax^
!0:1,1:2,2:3^
 -noWindow^
 -loop -1^
 -i

echo.
pause
