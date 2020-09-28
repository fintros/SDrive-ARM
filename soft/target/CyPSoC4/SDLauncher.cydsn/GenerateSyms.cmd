@echo off
if exist CortexM0\ARM_GCC_541\Debug\SDLauncher.elf (
echo Generating debug symbols...
arm-none-eabi-objdump.exe -t CortexM0\ARM_GCC_541\Debug\SDLauncher.elf | findstr /G:reqsyms.in | cscript -nologo gensyms.vbs > reqsyms.dbg
)
if exist CortexM0\ARM_GCC_541\Release\SDLauncher.elf (
echo Generating release symbols...
arm-none-eabi-objdump.exe -t CortexM0\ARM_GCC_541\Release\SDLauncher.elf | findstr /G:reqsyms.in | cscript -nologo gensyms.vbs > reqsyms.rel
)
