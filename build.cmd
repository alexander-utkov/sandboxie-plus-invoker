cl.exe /std:c++20 /EHsc /O2 main.cpp /link /out:jail.exe
xcopy.exe /y "jail.exe" "D:\Program Files\Custom\jail.exe"
