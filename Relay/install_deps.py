import subprocess
import os
from colorama import Fore, init
import platform
import sys

cwd     = os.getcwd().replace("\\", "/")
ssh_github = "github-revolve"

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
def json():
    if not os.path.exists(f"{cwd}/external/json"):
        git()
        cmd(f"git clone https://github.com/nlohmann/json.git {cwd}/external/json", shell=True, capture_output=True, text=True)
        if not os.path.exists(f"{cwd}/external/json"):
            print(Fore.RED + "Could not install json")
            sys.exit()
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
