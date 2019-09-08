@echo off
make
del sm_sprx.h
bin2c.exe -o sm_sprx.h sm.sprx
del sm.sym
del sm.prx
del sm.sprx
del objs\*.d
del objs\*.o
rd objs
pause