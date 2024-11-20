# cpp_imgui_implot_tcc
testing c++ imgui implot tcc stack for A7 whilst also being really easy to install on any system

## Installation
### Windows (REALLY IMPORTANT: Run PowerShell as Admin)
> git clone [this repo] \
(PowerShell as Admin)> python compile.py
#### run program
> .\bin\main.exe

## sources
### ImGUI Adobe Branch
https://github.com/adobe/imgui
### ImPlot
https://github.com/epezent/implot
### TCC - Tiny C Compiler
Makes it easier and more performant to make custom graphs for the user in A7. Becuase it can compile a string to c function in runtime, meaning it compiles a function after the whole application is compiled, and this runtime c function easily integrates with c++. when runtime compiling a string you can access any data or function allready declared in this scope by writing extern inside the string. E.g. extern int add(int a, int b) or extern const char hello[] (see src/test_tcc.hpp)
https://github.com/Tiny-C-Compiler/mirror-repository
### GLFW
It's for OpenGL which is the backend for ImGUI
#### Windows
https://sourceforge.net/projects/glfw/files/glfw/3.3.10/glfw-3.3.10.bin.WIN64.zip/download
### GLEW
It's for OpenGL which is the backend for ImGUI
#### Windows
https://sourceforge.net/projects/glew/files/glew/2.1.0/glew-2.1.0-win32.zip/download

## How To Install Python
### Windows
>curl -o python-installer.exe https://www.python.org/ftp/python/3.12.1/python-3.12.1-amd64.exe
>python-installer.exe /quiet InstallAllUsers=1 PrependPath=1
