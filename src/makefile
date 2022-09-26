all: hdimg.exe imager.exe

imager.exe: imager.c
  wcc imager.c
  wlink file imager.obj

hdimg.exe: hdimg.c
  wcc hdimg.c
  wlink file hdimg.obj

hdtest.exe: hdtest.c
  wcc hdtest.c
  wlink file hdtest.obj

hdwear.exe: hdwear.c
  wcc hdwear.c
  wlink file hdwear.obj

clean:
  del *.err
  del *.obj
  del hdimg.exe
  del imager.exe
  del hdwear.exe
  del hdtest.exe
