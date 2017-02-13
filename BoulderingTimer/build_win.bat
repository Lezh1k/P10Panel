if exist "bouldering_timer_bin" ( 
  echo "Try to remove bouldering_timer_bin"
  rd /Q /S "bouldering_timer_bin"
)

md bouldering_timer_bin
cd bouldering_timer_bin
qmake ..\BoulderingTimer.pro -r -spec win32-msvc2013
jom

cd bouldering_timer_bin\release
del *.obj
del *.cpp
del *.h
del *.moc
windeployqt --release --no-translations --compiler-runtime BoulderingTimer.exe
echo SUCCESS
cd ../..
