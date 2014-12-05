chcp 1251

del *.obj
del rfcsrv_odbc.exe

rem nwrfc sdk root
set nwrfcsdk=C:\nwrfcsdk

rem windows SDK root aka "C:\Program Files\Microsoft SDKs\Windows\v7.0\" 
set winsdk=C:\PROGRA~1\MIA713~1\Windows\v7.0

rem visual studio root aka c:\program files\Microsoft Visual Studio 8
set vsroot=C:\PROGRA~1\MICROS~2


cl  -DBCDASM -nologo -Od -Ob1 -fp:strict -Gy -GF -EHs -Z7 -W3 -Wp64 -D_X86_ -DWIN32 -DSAPwithUNICODE -DUNICODE -D_UNICODE -MD -D_AFXDLL -FR -J -RTC1 -D_CRT_NON_CONFORMING_SWPRINTFS -D_CRT_SECURE_NO_DEPRECATE -D_CRT_NONSTDC_NO_DEPRECATE -DSAPonNT -c /EHc- /TP /I %vsroot%\VC\include   /I %winsdk%\Include   /I %nwrfcsdk%\include    main.c

cl  -DBCDASM -nologo -Od -Ob1 -fp:strict -Gy -GF -EHs -Z7 -W3 -Wp64 -D_X86_ -DWIN32 -DSAPwithUNICODE -DUNICODE -D_UNICODE -MD -D_AFXDLL -FR -J -RTC1 -D_CRT_NON_CONFORMING_SWPRINTFS -D_CRT_SECURE_NO_DEPRECATE -D_CRT_NONSTDC_NO_DEPRECATE -DSAPonNT -c /EHc- /TP /I %vsroot%\VC\include   /I %winsdk%\Include   /I %nwrfcsdk%\include    dbc.cpp

cl  -DBCDASM -nologo -Od -Ob1 -fp:strict -Gy -GF -EHs -Z7 -W3 -Wp64 -D_X86_ -DWIN32 -DSAPwithUNICODE -DUNICODE -D_UNICODE -MD -D_AFXDLL -FR -J -RTC1 -D_CRT_NON_CONFORMING_SWPRINTFS -D_CRT_SECURE_NO_DEPRECATE -D_CRT_NONSTDC_NO_DEPRECATE -DSAPonNT -c /EHc- /TP /I %vsroot%\VC\include   /I %winsdk%\Include   /I %nwrfcsdk%\include    rfc.cpp


link -nologo /NXCOMPAT -STACK:0x800000  ole32.lib rpcrt4.lib oleaut32.lib oledb.lib uuid.lib kernel32.lib advapi32.lib user32.lib gdi32.lib winspool.lib ws2_32.lib Iphlpapi.lib netapi32.lib comdlg32.lib shell32.lib dbghelp.lib version.lib mpr.lib secur32.lib ^
  -OPT:REF -LARGEADDRESSAWARE -subsystem:console ^
  -out:rfcsrv_odbc.exe ^
  main.obj dbc.obj rfc.obj  ^
  sapnwrfc.lib libsapucum.lib odbc32.lib ^
  /LIBPATH:%winsdk%\LIB^
  /LIBPATH:%nwrfcsdk%\lib ^
  /LIBPATH:%vsroot%\VC\LIB

rem  /LIBPATH:C:\PROGRA~1\MI3EDC~1\90\SDK\Lib\x86 ^

rem [and possibly sapdecfICUlib.lib]
