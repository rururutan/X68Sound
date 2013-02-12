cl /MT /EHsc /Ox /Ot /Oi /Ob2 /DC86CTL /c X68Sound.cpp
rc x68sound.rc
link /dll /def:x68sound.def x68sound.obj x68sound.res winmm.lib user32.lib
