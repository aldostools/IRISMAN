@echo off
make
del sprx_iso.h
bin2c.exe -o sprx_iso.h rawseciso.sprx
del rawseciso.sym
del rawseciso.prx
del rawseciso.sprx
del objs\*.d
del objs\*.o
rd objs
pause
