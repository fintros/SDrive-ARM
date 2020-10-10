@echo off
if exist CortexM0\ARM_GCC_541\Debug\SDLauncher.elf (
echo Generating debug symbols...
arm-none-eabi-objdump.exe -t CortexM0\ARM_GCC_541\Debug\SDLauncher.elf | C:\Windows\System32\findstr.exe /G:reqsyms.in | C:\Windows\System32\cscript.exe  -nologo gensyms.vbs > reqsyms.dbg
)
if exist CortexM0\ARM_GCC_541\Release\SDLauncher.elf (
echo Generating release symbols...
arm-none-eabi-objdump.exe -t CortexM0\ARM_GCC_541\Release\SDLauncher.elf |  C:\Windows\System32\findstr.exe /G:reqsyms.in | C:\Windows\System32\cscript.exe  -nologo gensyms.vbs > reqsyms.rel
)
