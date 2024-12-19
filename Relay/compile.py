import subprocess
import os
from colorama import Fore, init
import platform
import sys

cwd     = os.getcwd().replace("\\", "/")
ssh_github = "github-revolve"

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

def git():
    if not cmd(f"git --version", shell=True):
        cmd("sudo apt-get update", shell=True)
        cmd("sudo apt-get install git", shell=True)
        cmd("source ~/.bashrc")
        if not cmd(f"git --version"):
            print("could not install git")
            sys.exit()

def gpp():
    if not cmd("g++ --version", shell=True):
        cmd("sudo apt-get update", shell=True)
        cmd("sudo apt-get install g++", shell=True)
        if not cmd("g++ --version", shell=True):
            print(Fore.RED + "could not install g++")
            sys.exit()
def canard():
    git()
    if not os.path.exists(f"{cwd}/external/libcanard"):
        cmd(f"git clone https://github.com/OpenCyphal/libcanard.git {cwd}/external/libcanard", shell=True)
        if not os.path.exists(f"{cwd}/external/libcanard"):
            print(Fore.RED + "Could not install libcanard")
            sys.exit()
    config["cflags"]["common"] += f" -I{cwd}/external/libcanard "
    config["src_dirs"].append(f"{cwd}/external/libcanard/libcanard")
def PcapPlusPlus():
    git()
    if not os.path.exists(f"{cwd}/external/PcapPlusPlus/build"):
        cmd(f"git clone https://github.com/seladb/PcapPlusPlus.git {cwd}/external/PcapPlusPlus", shell=True)
        cmd(f"mkdir build", cwd=f"{cwd}/external/PcapPlusPlus", shell=True)
        cmd(f"cmake -S . -B build", cwd=f"{cwd}/external/PcapPlusPlus", shell=True)
        cmd(f"cmake --build build -j16", cwd=f"{cwd}/external/PcapPlusPlus", shell=True)
        cmd(f"sudo cmake --install build", cwd=f"{cwd}/external/PcapPlusPlus", shell=True)
        if not os.path.exists(f"{cwd}/external/PcapPlusPlus/build"):
            print(Fore.RED + "Could not install PcapPlusPlus")
            sys.exit()
    config["cflags"]["common"] += f" -I/usr/local/include "
    config["ldflags"] += " -L/usr/local/lib -lPcap++ -lPacket++ -lCommon++ -lpcap "
def dsdl():
    git()
    cmd("sudo apt-get install python3-venv", shell=True)
    if not os.path.exists(f"{cwd}/external/dsdl/compiled"):
        cmd(f"git clone github-revolve:RevolveNTNU/dsdl-definitions.git {cwd}/external/dsdl", shell=True)
        cmd(f"python3 -m venv .venvs/nunavut", cwd=f"{cwd}/external/dsdl", shell=True)
        pip = f"{cwd}/external/dsdl/.venvs/nunavut/bin/pip"
        cmd(f"{pip} install --upgrade pip", cwd=f"{cwd}/external/dsdl", shell=True)
        cmd(f"{pip} install nunavut", cwd=f"{cwd}/external/dsdl", shell=True)
        nnvg = f"{cwd}/external/dsdl/.venvs/nunavut/bin/nnvg"
        if not cmd(f"{nnvg} --version", shell=True):
            print("could not install nnvg")
            sys.exit()
        path = f"{cwd}/external/dsdl/.venvs/nunavut/bin"
        python = f"{path}/python"
        #cmd(f"{python} scripts/verify_dsdl.py", cwd=f"{cwd}/external/dsdl", shell=True)
        cmd(f"PATH={path}:$PATH {python} scripts/parse_dsdl_to_c.py", cwd=f"{cwd}/external/dsdl", shell=True)
        if not os.path.exists(f"{cwd}/external/dsdl/compiled"):
            print(Fore.RED + "Could not install PcapPlusPlus")
            sys.exit()
    config["cflags"]["common"] += f" -I{cwd}/external/dsdl/compiled -DNUNAVUT_ASSERT\(x\)=assert\(x\) "
def json():
    if not os.path.exists(f"{cwd}/external/json"):
        git()
        cmd(f"git clone https://github.com/nlohmann/json.git {cwd}/external/json", shell=True, capture_output=True, text=True)
        if not os.path.exists(f"{cwd}/external/json"):
            print(Fore.RED + "Could not install json")
            sys.exit()
    config["cflags"]["common"] += f" -I{cwd}/external/json/include "
def websocket():
    if not cmd("dpkg -l | grep libboost-all-dev", shell=True) \
    or not cmd("dpkg -l | grep libwebsocketpp-dev", shell=True):
        cmd("sudo apt-get update", shell=True)
        cmd("sudo apt-get install libboost-all-dev", shell=True)
        cmd("sudo apt-get install libwebsocketpp-dev", shell=True)
        if not cmd("dpkg -l | grep libboost-all-dev", shell=True) \
        or not cmd("dpkg -l | grep libwebsocketpp-dev", shell=True):
            print("could not install websocket")
            sys.exit()
def asio():
    git()
    cmd("sudo apt-get install libasio-dev", shell=True)


if __name__ == "__main__":
    ssh_github = "github-revolve" # maybe you is 'github' you have to use the key that is associated with revolve access
    gpp()
    canard()
    PcapPlusPlus()
    dsdl()
    json()
    websocket()
    asio()

    config["cflags"]["common"] += f"{cwd}/include"

    # compile(config)


