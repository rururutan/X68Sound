cl /G6 /MT /GX /Ox /Ot /Oa /Og /Oi /Ob2  /c  JULIET.CPP
cl /G6 /MT /GX /Ox /Ot /Oa /Og /Oi /Ob2  /c  X68Sound.cpp
rc x68sound.rc
link /dll /def:x68sound.def x68sound.obj juliet.obj x68sound.res winmm.lib user32.lib
