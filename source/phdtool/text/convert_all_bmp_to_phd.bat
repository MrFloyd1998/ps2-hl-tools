@echo off

echo Converting all *.bmp images in current folder ...

echo __________________________________________________

for %%I in (*.bmp) do phdtool.exe %%I

echo __________________________________________________

echo Done. Press any key to exit ...

pause
