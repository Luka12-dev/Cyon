import os
import subprocess

__version__ = "1.0.0"
__author__ = "Luka"
__license__ = "MIT"

RUNTIME_PATH = os.path.join(os.path.dirname(__file__), "runtime")
LIB_PATH = os.path.join(RUNTIME_PATH, "libcyon.a")

def ensure_runtime_built():
    if not os.path.exists(LIB_PATH):
        print("⚙️  libcyon.a not found - building runtime...")
        try:
            subprocess.run(["make"], cwd=RUNTIME_PATH, check=True)
            print("Runtime built successfully.")
        except subprocess.CalledProcessError:
            print("Failed to build CYON runtime. Check your Makefile or GCC installation.")

def info():
    return {
        "name": "CYON Core",
        "version": __version__,
        "runtime_built": os.path.exists(LIB_PATH),
        "runtime_path": RUNTIME_PATH,
        "author": __author__,
    }


# Automatically check runtime on import
ensure_runtime_built()