@echo off
reg query HKLM\HARDWARE\DESCRIPTION\System\CentralProcessor\0 > result.log
benchmark.exe >> result.log
