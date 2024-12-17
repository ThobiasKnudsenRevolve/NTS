import subprocess
import os
from colorama import Fore, init
import platform
import sys

cwd_win      = os.getcwd()
cwd_bash     = cwd_win.replace("\\", "/")
cwd_unix     = cwd_win.replace("\\", "/")
choco        = "\"C:\\ProgramData\\chocolatey\\bin\\choco.exe\""
curl         = "\"C:\\ProgramData\\chocolatey\\bin\\curl.exe\""
make         = "\"C:\\ProgramData\\chocolatey\\bin\\make.exe\""
make_bash    = "C:/ProgramData/chocolatey/bin/make.exe"
cmake        = "\"C:\\Program Files\\CMake\\bin\\cmake.exe\""
cmake_bash   = "C/Program Files/CMake/bin/cmake.exe"
cmake_debian = "/usr/bin/cmake"
openssl      = "\"C:\\Program Files\\OpenSSL-Win64\\bin\\openssl.exe\""
mingw32_make = "\"C:\\ProgramData\\mingw64\\mingw64\\bin\\mingw32-make.exe\""
mingw32_make_bash = "C:/ProgramData/mingw64/mingw64/bin/mingw32-make.exe"
gpp          = "\"C:\\ProgramData\\mingw64\\mingw64\\bin\\g++.exe\""
gcc          = "\"C:\\ProgramData\\mingw64\\mingw64\\bin\\gcc.exe\""
gcc_bash     = "C/ProgramData/mingw64/mingw64/bin/gcc.exe"
git          = "\"C:\\Program Files\\Git\\bin\\git.exe\""
bash_bash    = "C:/Program Files/Git/bin/bash.exe"
_7zip        = "C:\\Program Files\\7-Zip\\7z.exe"
cl           = "\"C:\\Program Files (x86)\\Microsoft Visual Studio\\2022\\BuildTools\\VC\\Auxiliary\\Build\\vcvars64.bat\""
vcpkg        = f"\"{cwd_win}\\external\\vcpkg\\vcpkg.exe\""

def cl_exe_path():
    vswhere_path = "C:\\Program Files (x86)\\Microsoft Visual Studio\\Installer\\vswhere.exe"
    if not os.path.isfile(vswhere_path):
        print("vswhere.exe not found at expected location.")
        return None
    try:
        output = subprocess.check_output([
            vswhere_path,
            "-latest",
            "-products", "*",
            "-requires", "Microsoft.VisualStudio.Component.VC.Tools.x86.x64",
            "-property", "installationPath",
            "-format", "value"
        ], encoding='utf-8', errors='ignore')
    except subprocess.CalledProcessError as e:
        print("Error running vswhere.exe:", e)
        return None
    installation_path = output.strip()
    if not installation_path:
        print("No installation path found.")
        return None
    print("Visual Studio installation path:", installation_path)
    msvc_tools_path = os.path.join(installation_path, "VC\\Tools\\MSVC")
    if not os.path.isdir(msvc_tools_path):
        print("MSVC tools directory not found.")
        return None
    version_dirs = os.listdir(msvc_tools_path)
    if not version_dirs:
        print("No MSVC version directories found.")
        return None
    version_dirs.sort(reverse=True)
    latest_version = version_dirs[0]
    print("Latest MSVC version:", latest_version)
    cl_exe_path = os.path.join(
        msvc_tools_path,
        latest_version,
        r"bin\Hostx64\x64\cl.exe"
    )
    if os.path.isfile(cl_exe_path):
        print("Found cl.exe at:", cl_exe_path)
        return cl_exe_path
    else:
        print("cl.exe not found at expected location.")
        return None


init(autoreset=True)
def cmd(*args, **kwargs) -> bool:
    command_str = args[0] if isinstance(args[0], str) else ' '.join(args[0])
    print(Fore.CYAN + f"COMMAND  " + Fore.WHITE + f"{command_str}")
    try:
        result = subprocess.run(*args, **kwargs)
        if result.stdout:
            print(Fore.YELLOW + "STDOUT  " + Fore.WHITE + f"{result.stdout}")
        if result.stderr:
            print(Fore.YELLOW + "MESSAGE  " + Fore.WHITE + f"{result.stderr}")
        if result.returncode == 0:
            print(Fore.GREEN + "SUCCESS  \n")
            return True
        else:
            print(Fore.RED + "ERROR  " + Fore.WHITE + f"{result}\n")
            return False
    except FileNotFoundError:
        print(Fore.RED + f"ERRORR: FileNotFoundError occurred  " + Fore.WHITE + f"Command '{command_str}' not found\n")
        return False
    except subprocess.CalledProcessError as e:
        print(Fore.RED + "ERROR: CalledProcessError occurred   " + Fore.WHITE + f"Details: {e}\n")
        return False
def bash(script: str) -> bool:
    return cmd(f"\"C:\\Program Files\\Git\\bin\\bash.exe\" -c \"{script}\"", shell=True, capture_output=True, text=True)

# Updated configuration to allow multiple source directories
config = {}
if platform.system() == "Windows":
    print("hello")
    config = {
        "compiler": {
            "c_compiler": f"{gcc}",      # C compiler
            "cpp_compiler": f"{gpp}"  # C++ compiler
        },
        "cflags": {
            "common": "-Wall",        # Common flags for both C and C++ files
            "c": "",                  # Additional flags specific to C
            "cpp": "-std=c++17 "      # Additional flags specific to C++
        },
        "ldflags": "",                # Linking flags (for libraries)
        "output": {
            "object_dir": ".\\obj",   # Directory for object files
            "binary_dir": ".\\bin",   # Directory for the output executable
            "binary_name": "main"     # Name of the output binary
        },
        "src_dirs": [".\\src"],       # Allow multiple source directories
        "src_files": []               # Allow individual source files
    }
if platform.system() == "Linux":
    config = {
        "compiler": {
            "c_compiler": "gcc",    # C compiler
            "cpp_compiler": "g++"   # C++ compiler
        },
        "cflags": {
            "common": "-Wall",       # Common flags for both C and C++ files
            "c": "",                 # Additional flags specific to C
            "cpp": "-std=c++17 "     # Additional flags specific to C++
        },
        "ldflags": "",               # Linking flags (for libraries)
        "output": {
            "object_dir": "./obj",   # Directory for object files
            "binary_dir": "./bin",   # Directory for the output executable
            "binary_name": "main"    # Name of the output binary
        },
        "src_dirs": ["./src"],       # Allow multiple source directories
        "src_files": []              # Allow individual source files
    }

def compile(config):
    obj_dir = config["output"]["object_dir"]
    bin_dir = config["output"]["binary_dir"]

    # Create obj and bin directories if they do not exist
    if not os.path.exists(obj_dir):
        os.makedirs(obj_dir)
    if not os.path.exists(bin_dir):
        os.makedirs(bin_dir)

    object_files = []

    # Function to check if recompilation is needed
    def needs_recompilation(obj_file, dep_file):
        if not os.path.exists(obj_file):
            return True
        if not os.path.exists(dep_file):
            return True

        obj_mtime = os.path.getmtime(obj_file)
        deps = []

        # Read and parse the dependency file
        try:
            with open(dep_file, 'r') as f:
                content = f.read()
                # Remove line continuations
                content = content.replace('\\\n', '')
                # Split dependencies
                parts = content.split(':', 1)
                if len(parts) != 2:
                    return True  # Malformed dep file; force recompilation
                dep_list = parts[1].strip().split()
                deps.extend(dep_list)
        except Exception as e:
            print(f"Error reading dependency file {dep_file}: {e}")
            return True  # Force recompilation if dep file is unreadable

        for dep in deps:
            if os.path.exists(dep):
                if os.path.getmtime(dep) > obj_mtime:
                    return True
            else:
                # If a dependency doesn't exist, force recompilation
                return True
        return False

    # Function to compile a source file
    def compile_source(src_file, compiler, cflags, obj_file):
        dep_file = obj_file + '.d'
        compile_cmd = (
            f"{compiler} "
            f"{cflags} "
            f"-MMD -MF {dep_file} "
            f"-c {src_file} -o {obj_file}"
        )
        if not cmd(compile_cmd, shell=True):
            print(f"Failed to compile {src_file}")
            return False
        print(f"Compiled {src_file} into {obj_file}")
        return True

    # Iterate over multiple source directories
    for src_dir in config["src_dirs"]:
        # Get list of C and C++ source files
        c_files = [f for f in os.listdir(src_dir) if f.endswith(".c")]
        cpp_files = [f for f in os.listdir(src_dir) if f.endswith(".cpp")]

        # Compile C files
        for c_file in c_files:
            src_file = os.path.join(src_dir, c_file)
            obj_file = os.path.join(obj_dir, os.path.splitext(c_file)[0] + ".o")
            dep_file = obj_file + '.d'

            cflags = f"{config['cflags']['common']} {config['cflags']['c']}"
            compiler = config['compiler']['c_compiler']

            if needs_recompilation(obj_file, dep_file):
                if not compile_source(src_file, compiler, cflags, obj_file):
                    return False
            else:
                print(f"Skipping compilation of {src_file}; up-to-date.")
            object_files.append(obj_file)

        # Compile C++ files
        for cpp_file in cpp_files:
            src_file = os.path.join(src_dir, cpp_file)
            obj_file = os.path.join(obj_dir, os.path.splitext(cpp_file)[0] + ".o")
            dep_file = obj_file + '.d'

            cflags = f"{config['cflags']['common']} {config['cflags']['cpp']}"
            compiler = config['compiler']['cpp_compiler']

            if needs_recompilation(obj_file, dep_file):
                if not compile_source(src_file, compiler, cflags, obj_file):
                    return False
            else:
                print(f"Skipping compilation of {src_file}; up-to-date.")
            object_files.append(obj_file)

    # Compile individual source files
    for src_file in config["src_files"]:
        file_ext = os.path.splitext(src_file)[1]
        base_name = os.path.splitext(os.path.basename(src_file))[0]
        obj_file = os.path.join(obj_dir, base_name + ".o")
        dep_file = obj_file + '.d'

        if file_ext == ".c":
            cflags = f"{config['cflags']['common']} {config['cflags']['c']}"
            compiler = config['compiler']['c_compiler']
        elif file_ext == ".cpp":
            cflags = f"{config['cflags']['common']} {config['cflags']['cpp']}"
            compiler = config['compiler']['cpp_compiler']
        else:
            print(f"Unknown file extension for {src_file}")
            return False

        if needs_recompilation(obj_file, dep_file):
            if not compile_source(src_file, compiler, cflags, obj_file):
                return False
        else:
            print(f"Skipping compilation of {src_file}; up-to-date.")
        object_files.append(obj_file)

    # Link all object files into a final executable
    output_executable = os.path.join(bin_dir, config["output"]["binary_name"])
    # Check if the executable needs to be relinked
    recompile_executable = False
    if not os.path.exists(output_executable):
        recompile_executable = True
    else:
        exe_mtime = os.path.getmtime(output_executable)
        for obj_file in object_files:
            if os.path.getmtime(obj_file) > exe_mtime:
                recompile_executable = True
                break

    if recompile_executable:
        link_cmd = (
            f"{config['compiler']['cpp_compiler']} "
            f"{' '.join(object_files)} "
            f"{config['ldflags']} "
            f"-o {output_executable}"
        )

        if not cmd(link_cmd, shell=True):
            print(f"Failed to link object files.")
            return False
        print(f"Linked object files into executable {output_executable}")
    else:
        print(f"Skipping linking; executable {output_executable} is up-to-date.")

    print(f"Compilation successful. Executable is at {output_executable}")
    return True

class program:
    def choco():
        if not cmd(f"{choco} --version", shell=True):
            cmd("powershell -Command \"Set-ExecutionPolicy Bypass -Scope Process -Force; [System.Net.ServicePointManager]::SecurityProtocol = 3072; iex ((New-Object System.Net.WebClient).DownloadString('https://community.chocolatey.org/install.ps1'))\"", shell=True) 
        if not cmd(f"{choco} --version", shell=True):
            print(Fore.RED + "could not install choco")
            sys.exit()
    def make():
        if not bash(f"make --version"):
            if not cmd(f"{make} --version", shell=True):
                program.choco()
                cmd(f"{choco} install make  --installargs 'ADD_MAKE_TO_PATH=System' -y", shell=True)
                if not cmd(f"{make} --version", shell=True):
                    print(Fore.RED + "could not install make")
                    sys.exit()
            env = os.environ.copy()
            env["PATH"] = f"{make}\\..;{env['PATH']}"
            if not bash(f"make --version"):
                print(Fore.RED + "could not install make")
                sys.exit()
    def gpp():
        if platform.system() == "Windows":
            if not cmd(f"{gpp} --version", shell=True):
                program.choco()
                cmd(f"{choco} install mingw  --installargs 'ADD_MINGW_TO_PATH=System' -y", shell=True)
                if not cmd(f"{gpp} --version", shell=True):
                    print(Fore.RED + "could not install gpp")
                    sys.exit()
                env = os.environ.copy()
                env["PATH"] = f"{gpp}\\..;{env['PATH']}"
                cmd("refreshenv", shell=True)
        if platform.system() == "Linux":
            if not cmd("g++ --version", shell=True):
                cmd("sudo apt-get update", shell=True)
                cmd("sudo apt-get install g++", shell=True)
                if not cmd("g++ --version", shell=True):
                    print(Fore.RED + "could not install g++")
                    sys.exit()
        if platform.system() == "Darwin":
            pass
    def gcc():
        if not cmd("gcc --version"):
            if not bash(f"gcc --version"):
                if not cmd(f"{gcc} --version", shell=True):
                    program.choco()
                    cmd(f"{choco} install mingw --installargs 'ADD_MINGW_TO_PATH=System' -y", shell=True)
                    if not cmd(f"{gcc} --version", shell=True):
                        print(Fore.RED + "could not install gcc")
                        sys.exit()
                env = os.environ.copy()
                env["PATH"] = f"{gcc}\\..;{env['PATH']}"
                if not bash(f"gcc --version"):
                    print(Fore.RED + "could not install gcc")
                    sys.exit()
        if platform.system() == "Linux":
            if not cmd("gcc --version", shell=True):
                cmd("sudo apt-get update", shell=True)
                cmd("sudo apt-get install gcc", shell=True)
                if not cmd("gcc --version", shell=True):
                    print(Fore.RED + "could not install gcc")
                    sys.exit()
        if platform.system() == "Darwin":
            pass
    def curl():
        if not cmd(f"{curl} --version", shell=True):
            program.choco()
            cmd(f"{choco} install curl -y", shell=True)
            if not cmd(f"{curl} --version", shell=True):
                print(Fore.RED + "could not install curl")
                sys.exit()
    def mingw32_make():
        if platform.system() == "Windows":
            if not bash(f"mingw32-make --version"):
                if not cmd(f"{mingw32_make} --version", shell=True):
                    program.choco()
                    cmd(f"{choco} install mingw --installargs 'ADD_MINGW_TO_PATH=System' -y", shell=True)
                    if not cmd(f"{mingw32_make} --version", shell=True):
                        print(Fore.RED + "could not install mingw32_make")
                        sys.exit()
                env = os.environ.copy()
                env["PATH"] = f"{mingw32_make}\\..;{env['PATH']}"
                if not bash(f"mingw32-make --version"):
                    print(Fore.RED + "could not install mingw32_make")
                    sys.exit()
        if platform.system() == "Linux":
            pass
        if platform.system() == "Darwin":
            pass
    def git():
        if platform.system() == "Windows":
            if not cmd(f"{git} --version", shell=True):
                if not cmd(f"{git} --version", shell=True):
                    program.choco()
                    cmd(f"{choco} install git --force --installargs 'ADD_GIT_TO_PATH=System' -y", shell=True)
                    if not cmd(f"{git} --version", shell=True):
                        print(Fore.RED + "could not install git")
                        sys.exit()
                env = os.environ.copy()
                env["PATH"] = f"{git}\\..;{env['PATH']}"
                if not cmd(f"{git} --version", shell=True):
                    print(Fore.RED + "could not install git")
                    sys.exit()
        if platform.system() == "Linux":
            if not cmd(f"git --version", shell=True):
                cmd("sudo apt-get update", shell=True)
                cmd("sudo apt-get install git", shell=True)
                cmd("source ~/.bashrc")
                if not cmd(f"git --version"):
                    print("could not install git")
                    sys.exit()
        if platform.system() == "Darwin":
            sys.exit()
    def cmake():
        if not bash(f"cmake --version"):
            if not cmd(f"{cmake} --version", shell=True):
                program.choco()
                cmd(f"{choco} uninstall cmake -y",  shell=True)
                cmd(f"{choco} install cmake --force --installargs 'ADD_CMAKE_TO_PATH=System' -y",  shell=True)
                if not cmd(f"{cmake} --version", shell=True):
                    print(Fore.RED + "could not install cmake")
                    sys.exit()
            env = os.environ.copy()
            env["PATH"] = f"{cmake}\\..;{env['PATH']}"
            if not bash(f"cmake --version"):
                print(Fore.RED + "could not install cmake")
                sys.exit()
    def openssl():
        if not cmd(f"{openssl} --version", shell=True):
            program.choco()
            cmd(f"{choco} uninstall openssl -y", shell=True)
            cmd(f"{choco} install openssl --force --installargs 'ADD_OPENSSL_TO_PATH=System' -y", shell=True)
            if not cmd(f"{openssl} --version", shell=True):
                print(Fore.RED + "could not install openssl")
                sys.exit()
    def _7zip():
        if not bash(f"7z"):
            if not cmd(f"\"{_7zip}\"", shell=True):
                program.choco()
                cmd(f"{choco} uninstall 7zip -y",  shell=True)
                cmd(f"{choco} install 7zip --force -y",  shell=True)
                if not cmd(f"\"{_7zip}\"", shell=True):
                    print(Fore.RED + "could not install 7zip")
                    sys.exit()
            env = os.environ.copy()
            env["PATH"] = f"{_7zip}\\..;{env['PATH']}"
            if not bash(f"7z"):
                print(Fore.RED + "could not install 7zip")
                sys.exit()
    def msvc():
        if not cmd("cl", shell=True):
            program.choco()
            #cmd(f"{choco} uninstall -n visualstudio2022buildtools -y", shell=True)
            cmd(f"{choco} install visualstudio2022buildtools -y --package-parameters \"--add Microsoft.VisualStudio.Workload.VCTools --includeRecommended --includeOptional\"", shell=True)
            cmd("\"C:\\Program Files (x86)\\Microsoft Visual Studio\\2022\\BuildTools\\VC\\Auxiliary\\Build\\vcvarsall.bat\" x64", shell=True)
            cmd(f"setx /M PATH \"%PATH%;{cl_exe_path()}\\..\"", shell=True)
            cmd("refreshenv", shell=True)
            if cl_exe_path() == None:
                print(Fore.RED + "could not find cl full path")
                sys.exit()
            if not cmd("cl", shell=True) and False:
                print(Fore.RED + "could not install msvc")
                sys.exit()
    def vcpkg():
        if platform.system() == "Windows":
            if not cmd(f"{vcpkg} --version", shell=True):
                program.git()
                program.msvc()
                if os.path.exists(f"{cwd_win}\\external\\vcpkg"):
                    bash(f"rm -rf {cwd_win}\\external\\vcpkg")
                
                cmd(f"powershell -Command \"Remove-Item -Recurse -Force {cwd_win}\\external\\vcpkg\"", shell=True)
                cmd(f"mkdir {cwd_win}\\external\\vcpkg", shell=True)
                cmd(f"{git} clone https://github.com/microsoft/vcpkg.git {cwd_win}\\external\\vcpkg", shell=True)
                cmd(f".\\bootstrap-vcpkg.bat", cwd=f"{cwd_win}\\external\\vcpkg", shell=True)
                if not cmd(f"{vcpkg} --version", shell=True):
                    print(Fore.RED + "could not install vcpkg")
                    sys.exit()
            else:
                program.git()
                if not cmd(f"{git} pull", cwd=f"{cwd_win}\\external\\vcpkg", shell=True):
                    sys.exit()
                if not cmd(".\\vcpkg update", cwd=f"{cwd_win}\\external\\vcpkg", shell=True):
                    sys.exit()
        if platform.system() == "Linux":
            print("vcpkg should not be installed on linux")
            sys.exit()
        if platform.system() == "Darwin":
            print("vcpkg should not be installed on MacOS")
            sys.exit()


class library:
    def tcc():
        if platform.system() == "Windows":
            if not os.path.exists(f"{cwd_win}\\include") \
            or not os.path.exists(f"{cwd_win}\\lib") \
            or not os.path.exists(f"{cwd_win}\\libtcc") \
            or not os.path.exists(f"{cwd_win}\\libtcc.dll"):
                
                program.make()
                program.gpp()
                program.gcc()
                program.git()
                
                cmd(f"powershell -Command \"Remove-Item -Recurse -Force {cwd_win}\\external\\installs\\tcc\n", capture_output=True, shell=True)
                os.makedirs(f"{cwd_win}\\external\\installs\\tcc")
                cmd(f"{git} clone https://github.com/Tiny-C-Compiler/mirror-repository {cwd_win}\\external\\installs\\tcc", capture_output=True, text=True)
                os.makedirs(f"{cwd_win}\\external\\installs\\tcc\\build")

                configure_prefix = (os.path.abspath(os.path.join("external", "installs", "tcc", "build"))).replace("\\", "/")
                bash(f"cd {cwd_bash}/external/installs/tcc && ./configure --prefix={configure_prefix}")
                bash(f"cd {cwd_bash}/external/installs/tcc && make")
                bash(f"cd {cwd_bash}/external/installs/tcc && make install")
                bash(f"cp -ru {cwd_bash}/external/installs/tcc/build/libtcc.dll {cwd_bash}/libtcc.dll")
                bash(f"cp -ru {cwd_bash}/external/installs/tcc/build/libtcc {cwd_bash}")
                bash(f"cp -ru {cwd_bash}/external/installs/tcc/build/include {cwd_bash}")
                bash(f"cp -ru {cwd_bash}/external/installs/tcc/build/lib {cwd_bash}")

                if not os.path.exists(f"{cwd_win}\\include") \
                or not os.path.exists(f"{cwd_win}\\lib") \
                or not os.path.exists(f"{cwd_win}\\libtcc") \
                or not os.path.exists(f"{cwd_win}\\libtcc.dll"):
                    print(Fore.RED + "Could not install TinyCC.")
                    sys.exit()
                print(Fore.GREEN + "TinyCC built successfully in the local folder.\n")
            config["cflags"]["common"] += " -I libtcc "
            config["ldflags"] += " libtcc.dll " 
            
        if platform.system() == "Linux":
            pass
        if platform.system() == "Darwin":
            pass
    def opengl():
        if platform.system() == "Windows":
            if not os.path.exists(f"{cwd_win}\\bin\\glew32.dll") \
            or not os.path.exists(f"{cwd_win}\\bin\\glfw3.dll"):
                
                program.git()
                program.curl()
                    
                cmd(f"powershell -Command \"Remove-Item -Recurse -Force {cwd_win}\\external\\installs\\glfw.zip\"", shell=True)
                cmd(f"mkdir \"{cwd_win}\\external\\installs\"", shell=True)
                cmd(f"{curl} -L -o \"{cwd_win}\\external\\installs\\glfw.zip\" \"https://sourceforge.net/projects/glfw/files/glfw/3.3.10/glfw-3.3.10.bin.WIN64.zip/download\"", shell=True)
                cmd(f"powershell -Command \"Expand-Archive -Force -Path {cwd_win}\\external\\installs\\glfw.zip -DestinationPath {cwd_win}\\external\\glfw\"", shell=True)
                    
                cmd(f"powershell -Command \"Remove-Item -Recurse -Force .\\external\\installs\\glew.zip\"", shell=True)
                cmd(f"{curl} -L -o \"{cwd_win}\\external\\installs\\glew.zip\" \"https://sourceforge.net/projects/glew/files/glew/2.1.0/glew-2.1.0-win32.zip/download\"", shell=True)
                cmd(f"powershell -Command \"Expand-Archive -Force -Path {cwd_win}\\external\\installs\\glew.zip -DestinationPath {cwd_win}\\external\\glew\"", capture_output=True, text=True)

                if not os.path.exists(f"{cwd_win}\\bin"):
                    os.makedirs(f"{cwd_win}\\bin")
                bash(f"cp {cwd_bash}/external/glew/glew-2.1.0/bin/Release/x64/glew32.dll {cwd_bash}/bin")
                bash(f"cp {cwd_bash}/external/glfw/glfw-3.3.10.bin.WIN64/lib-mingw-w64/glfw3.dll {cwd_bash}/bin")
                if not os.path.exists(f"{cwd_win}\\bin\\glew32.dll") \
                or not os.path.exists(f"{cwd_win}\\bin\\glfw3.dll"):
                    print(Fore.RED + "Could not build OpenGL")
                    sys.exit()
                print(Fore.GREEN + "OpenGL buildt successfully in local folder\n")
            
            config["cflags"]["common"] += f" -I{cwd_win}\\external\\glfw\\glfw-3.3.10.bin.WIN64\\include -I{cwd_win}\\external\\glew\\glew-2.1.0\\include "
            config["ldflags"] += f" -L{cwd_win}\\external\\glfw\\glfw-3.3.10.bin.WIN64\\lib-mingw-w64 -L{cwd_win}\\external\\glew\\glew-2.1.0\\lib\\Release\\x64 " 
            config["ldflags"] += " -lglew32 -lglfw3 -lopengl32 -lgdi32 " 
        if platform.system() == "Linux":
            if not cmd("dpkg -s libglfw3-dev", shell=True) or not cmd("dpkg -s libglew-dev", shell=True):
                cmd("sudo apt-get update", shell=True)
                cmd("sudo apt-get install libglfw3-dev libglew-dev", shell=True)
            config["ldflags"] += " -lglfw -lGLEW -lGL "
        if platform.system() == "Darwin":
            sys.exit()
    def imgui():
        if platform.system() == "Windows":
            program.git()
            if not os.path.exists(f"{cwd_win}\\external\\imgui"):
                cmd(f"{git} clone https://github.com/adobe/imgui.git {cwd_win}\\external\\imgui",  shell=True)
                if not os.path.exists(f"{cwd_win}\\external\\imgui"):
                    print(Fore.RED + "Could not install imgui")
                    sys.exit()
            config["cflags"]["common"] += f" -I{cwd_win}\\external\\imgui -I{cwd_win}\\external\\imgui\\backends "
            config["src_dirs"].append(f"{cwd_win}\\external\\imgui")
            config["src_files"].append(f"{cwd_win}\\external\\imgui\\backends\\imgui_impl_glfw.cpp")
            config["src_files"].append(f"{cwd_win}\\external\\imgui\\backends\\imgui_impl_opengl3.cpp")
        if platform.system() == "Linux":
            program.git()
            if not os.path.exists(f"{cwd_unix}/external/imgui"):
                cmd(f"{git} clone https://github.com/adobe/imgui.git {cwd_unix}/external/imgui",  shell=True)
                if not os.path.exists(f"{cwd_unix}/external/imgui"):
                    print(Fore.RED + "Could not install imgui")
                    sys.exit()
            config["cflags"]["common"] += f" -I{cwd_unix}/external/imgui -I{cwd_unix}/external/imgui/backends "
            config["src_dirs"].append(f"{cwd_unix}/external/imgui")
            config["src_files"].append(f"{cwd_unix}/external/imgui/backends/imgui_impl_glfw.cpp")
            config["src_files"].append(f"{cwd_unix}/external/imgui/backends/imgui_impl_opengl3.cpp")
        if platform.system() == "Darwin":
            sys.exit()
    def implot():
        if platform.system() == "Windows":
            program.git()
            if not os.path.exists(f"{cwd_win}\\external\\implot"):
                cmd(f"{git} clone https://github.com/epezent/implot.git {cwd_win}\\external\\implot", shell=True)
                if not os.path.exists(f"{cwd_win}\\external\\implot"):
                    print(Fore.RED + "Could not install implot")
                    sys.exit()
            config["cflags"]["common"] += f" -I{cwd_win}\\external\\implot -I{cwd_win}\\external\\implot\\backends "
            config["src_dirs"].append(f"{cwd_win}\\external\\implot")
        if platform.system() == "Linux":
            program.git()
            if not os.path.exists(f"{cwd_unix}/external/implot"):
                cmd(f"git clone https://github.com/epezent/implot.git {cwd_unix}/external/implot", shell=True)
                if not os.path.exists(f"{cwd_unix}/external/implot"):
                    print(Fore.RED + "Could not install implot")
                    sys.exit()
            config["cflags"]["common"] += f" -I{cwd_unix}/external/implot -I{cwd_unix}/external/implot/backends "
            config["src_dirs"].append(f"{cwd_unix}/external/implot")
        if platform.system() == "Darwin":
            sys.exit()
    def ImGuiFileDialog():
        if platform.system() == "Windows":
            print(Fore.RED + "Windows is not supported for ImGuiFileDialog yet")
            sys.exit()
        if platform.system() == "Linux":
            program.git()
            if not os.path.exists(f"{cwd_unix}/external/ImGuiFileDialog"):
                cmd(f"git clone https://github.com/aiekick/ImGuiFileDialog.git {cwd_unix}/external/ImGuiFileDialog", shell=True)
                if not os.path.exists(f"{cwd_unix}/external/ImGuiFileDialog"):
                    print(Fore.RED + "Could not install ImGuiFileDialog")
                    sys.exit()
            config["cflags"]["common"] += f" -I{cwd_unix}/external/ImGuiFileDialog "
            config["src_dirs"].append(f"{cwd_unix}/external/ImGuiFileDialog")
        if platform.system() == "Darwin":
            print(Fore.RED + "MacOS is not supported for ImGuiFileDialog yet")
            sys.exit()
    def canard():
        if platform.system() == "Windows":
            print(Fore.RED + "Windows is not supported for libcanard yet")
            sys.exit()
        if platform.system() == "Linux":
            program.git()
            if not os.path.exists(f"{cwd_unix}/external/libcanard"):
                cmd(f"git clone git@github-primary:OpenCyphal/libcanard.git {cwd_unix}/external/libcanard", shell=True)
                if not os.path.exists(f"{cwd_unix}/external/libcanard"):
                    print(Fore.RED + "Could not install libcanard")
                    sys.exit()
            config["cflags"]["common"] += f" -I{cwd_unix}/external/libcanard "
            config["src_dirs"].append(f"{cwd_unix}/external/libcanard/libcanard")
        if platform.system() == "Darwin":
            print(Fore.RED + "MacOS is not supported for libcanard yet")
            sys.exit()
    def PcapPlusPlus():
        if platform.system() == "Windows":
            print(Fore.RED + "Windows is not supported for PcapPlusPlus yet")
            sys.exit()
        if platform.system() == "Linux":
            program.git()
            if not os.path.exists(f"{cwd_unix}/external/PcapPlusPlus/build"):
                cmd(f"git clone git@github.com:seladb/PcapPlusPlus.git {cwd_unix}/external/PcapPlusPlus", shell=True)
                cmd(f"mkdir build", cwd=f"{cwd_unix}/external/PcapPlusPlus", shell=True)
                cmd(f"cmake -S . -B build", cwd=f"{cwd_unix}/external/PcapPlusPlus", shell=True)
                cmd(f"cmake --build build -j16", cwd=f"{cwd_unix}/external/PcapPlusPlus", shell=True)
                cmd(f"sudo cmake --install build", cwd=f"{cwd_unix}/external/PcapPlusPlus", shell=True)
                if not os.path.exists(f"{cwd_unix}/external/PcapPlusPlus/build"):
                    print(Fore.RED + "Could not install PcapPlusPlus")
                    sys.exit()
            config["cflags"]["common"] += f" -I/usr/local/include "
            config["ldflags"] += " -L/usr/local/lib -lPcap++ -lPacket++ -lCommon++ -lpcap "
        if platform.system() == "Darwin":
            print(Fore.RED + "MacOS is not supported for PcapPlusPlus yet")
            sys.exit()
    def dsdl():
        if platform.system() == "Windows":
            print(Fore.RED + "Windows is not supported for PcapPlusPlus yet")
            sys.exit()
        if platform.system() == "Linux":
            program.git()
            cmd("sudo apt-get install python3-venv", shell=True)
            if not os.path.exists(f"{cwd_unix}/external/dsdl/compiled"):
                cmd(f"git clone github-revolve:RevolveNTNU/dsdl-definitions.git {cwd_unix}/external/dsdl", shell=True)
                cmd(f"python3 -m venv .venvs/nunavut", cwd=f"{cwd_unix}/external/dsdl", shell=True)
                pip = f"{cwd_unix}/external/dsdl/.venvs/nunavut/bin/pip"
                cmd(f"{pip} install --upgrade pip", cwd=f"{cwd_unix}/external/dsdl", shell=True)
                cmd(f"{pip} install nunavut", cwd=f"{cwd_unix}/external/dsdl", shell=True)
                nnvg = f"{cwd_unix}/external/dsdl/.venvs/nunavut/bin/nnvg"
                if not cmd(f"{nnvg} --version", shell=True):
                    print("could not install nnvg")
                    sys.exit()
                path = f"{cwd_unix}/external/dsdl/.venvs/nunavut/bin"
                python = f"{path}/python"
                #cmd(f"{python} scripts/verify_dsdl.py", cwd=f"{cwd_unix}/external/dsdl", shell=True)
                cmd(f"PATH={path}:$PATH {python} scripts/parse_dsdl_to_c.py", cwd=f"{cwd_unix}/external/dsdl", shell=True)
                if not os.path.exists(f"{cwd_unix}/external/dsdl/compiled"):
                    print(Fore.RED + "Could not install PcapPlusPlus")
                    sys.exit()
            config["cflags"]["common"] += f" -I{cwd_unix}/external/dsdl/compiled -DNUNAVUT_ASSERT\(x\)=assert\(x\) "
        if platform.system() == "Darwin":
            print(Fore.RED + "MacOS is not supported for PcapPlusPlus yet")
            sys.exit()
    def curl():
        if platform.system() == "Windows":
                
            if not os.path.exists(f"{cwd_win}\\include\\curl\\curl.h") \
            or not os.path.exists(f"{cwd_win}\\lib\\libcurl.lib"):    
                
                program.cmake()
                program.openssl()
                program.mingw32_make()

                cmd(f"powershell -Command \"Remove-Item -Recurse -Force {cwd_win}\\external\\installs\\curl\n", shell=True)
                os.makedirs(f"{cwd_win}\\external\\installs\\curl")
                cmd(f"{git} clone https://github.com/curl/curl.git {cwd_win}\\external\\installs\\curl", capture_output=True, text=True)
                os.makedirs(f"{cwd_win}\\external\\installs\\curl\\build")

                bash(f"cd {cwd_bash}/external/installs/curl/build && cmake .. -G 'MinGW Makefiles' -DCMAKE_USE_OPENSSL=ON -DOPENSSL_ROOT_DIR='C:/Program Files/OpenSSL-Win64' -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=OFF -DCURL_STATICLIB=ON -DBUILD_CURL_EXE=OFF -DCMAKE_C_FLAGS='-w'")
                bash(f"cd {cwd_bash}/external/installs/curl/build && mingw32-make -j16")
                
                bash(f"cp -ru {cwd_bash}/external/installs/curl/build/lib/libcurl.lib {cwd_bash}/lib/libcurl.lib")
                bash(f"cp -ru {cwd_bash}/external/installs/curl/include {cwd_bash}")
            
                if not os.path.exists(f"{cwd_win}\\include\\curl\\curl.h") \
                or not os.path.exists(f"{cwd_win}\\lib\\libcurl.lib"):    
                    print(Fore.RED + "Could not install curl")
                    sys.exit()
    def boost():
        if platform.system() == "Windows":
            if not os.path.exists(f"{cwd_win}\\external\\boost") or True:
                program.make()
                program.gcc()
                program.cmake()
                program._7zip()
                program.msvc()

                #program.curl()
                #cmd(f"{curl} -L https://boostorg.jfrog.io/artifactory/main/release/1.81.0/source/boost_1_81_0.zip --output {cwd_win}\\external\\installs\\boost\\boost_1_81_0.zip", shell=True)
                #cmd(f"\"{_7zip}\" x {cwd_win}\\external\\installs\\boost\\boost_1_81_0.zip -o{cwd_win}\\external\\installs\\boost\\boost_1_81_0", shell=True)
                cmd(f".\\bootstrap.bat",cwd=f"{cwd_win}\\external\\installs\\boost\\boost_1_81_0", shell=True)
                #if not os.path.exists(f"{cwd_win}\\external\\boost"): 
                #    os.makedirs(f"{cwd_win}\\external\\boost")
                cmd(f".\\b2 -d1 install --with-system --with-thread address-model=64 -j4 --prefix=..\\..\\..\\boost",cwd=f"{cwd_win}\\external\\installs\\boost\\boost_1_81_0", shell=True)

                
                if not os.path.exists(f"{cwd_win}\\external\\boost"):
                    print(Fore.RED + "Could not install boost")
                    sys.exit()

        if platform.system() == "Linux":
            pass
        if platform.system() == "Darwin":
            pass
    def websocket():
        if platform.system() == "Windows":
            if not os.path.exists(f"{cwd_win}\\external\\vcpkg\\installed\\x64-windows\\lib") or True:
                program.vcpkg()
                cmd(f"{vcpkg} install websocketpp:x64-windows-static", shell=True)
                cmd(f"{vcpkg} install boost-thread:x64-windows-static", shell=True)
                if not os.path.exists(f"{cwd_win}\\external\\vcpkg\\installed\\x64-windows-static\\lib"):
                    print(Fore.RED + "Could not install websocketpp:x64-windows-static")
                    sys.exit()

            config["cflags"]["common"] += f" -I{cwd_win}\\external\\vcpkg\\installed\\x64-windows-static\\include "
            config["ldflags"] += f"-L{cwd_win}\\external\\vcpkg\\installed\\x64-windows-static\\lib \
                -lboost_system-vc143-mt-x64-1_86 -lboost_context-vc143-mt-x64-1_86 -lboost_coroutine-vc143-mt-x64-1_86 -lboost_thread-vc143-mt-x64-1_86 -lws2_32" 

        if platform.system() == "Linux":
            if not cmd("dpkg -l | grep libboost-all-dev", shell=True) \
            or not cmd("dpkg -l | grep libwebsocketpp-dev", shell=True):
                cmd("sudo apt-get update", shell=True)
                cmd("sudo apt-get install libboost-all-dev", shell=True)
                cmd("sudo apt-get install libwebsocketpp-dev", shell=True)
                if not cmd("dpkg -l | grep libboost-all-dev", shell=True) \
                or not cmd("dpkg -l | grep libwebsocketpp-dev", shell=True):
                    print("could not install websocket")
                    sys.exit()
        if platform.system() == "Darwin":
            sys.exit()
    def json():
        if platform.system() == "Windows":
            if not os.path.exists(f"{cwd_win}\\external\\json"):
                program.git()
                cmd(f"{git} clone https://github.com/nlohmann/json.git {cwd_win}\\external\\json", capture_output=True, text=True)
                if not os.path.exists(f"{cwd_win}\\external\\json"):
                    print(Fore.RED + "Could not install json")
                    sys.exit()
            config["cflags"]["common"] += f" -I{cwd_win}\\external\\json\\include "
        if platform.system() == "Linux":
            if not os.path.exists(f"{cwd_unix}/external/json"):
                program.git()
                cmd(f"git clone https://github.com/nlohmann/json.git {cwd_unix}/external/json", shell=True, capture_output=True, text=True)
                if not os.path.exists(f"{cwd_unix}/external/json"):
                    print(Fore.RED + "Could not install json")
                    sys.exit()
            config["cflags"]["common"] += f" -I{cwd_unix}/external/json/include "
        if platform.system() == "Darwin":
            pass
    def asio():
        if platform.system() == "Windows":
            print(Fore.RED + "Windows is not supported for ImGuiFileDialog yet")
            sys.exit()
        if platform.system() == "Linux":
            program.git()
            cmd("sudo apt-get install libasio-dev", shell=True)
        if platform.system() == "Darwin":
            print(Fore.RED + "MacOS is not supported for ImGuiFileDialog yet")
            sys.exit()

if __name__ == "__main__":
    #library.tcc()
    program.gpp()
    library.canard()
    library.PcapPlusPlus()
    library.dsdl()
    library.json()
    library.websocket()
    library.asio()

    config["cflags"]["common"] += f"{cwd_unix}/include"

    # compile(config)


