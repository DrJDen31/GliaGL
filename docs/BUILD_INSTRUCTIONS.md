# GliaGL Build Instructions

## Network Visualization Build

### Prerequisites

**Required Libraries:**
- OpenGL 3.3+ (or Metal on macOS)
- GLFW 3.x
- GLEW (Linux/Windows) or built-in OpenGL loader (macOS)
- GLM (OpenGL Mathematics)
- C++11 compatible compiler

**Linux/WSL:**
```bash
sudo apt-get update
sudo apt-get install build-essential cmake
sudo apt-get install libglfw3-dev libglew-dev libglm-dev
sudo apt-get install xorg-dev libglu1-mesa-dev
```

**macOS:**
```bash
brew install cmake glfw glew glm
```

**Windows:**
- Install Visual Studio 2019 or later
- Install CMake
- Libraries will be managed by vcpkg or manually

### Building

#### Linux / WSL with X11

```bash
# Navigate to visualization directory
cd src/vis

# Create build directory
mkdir build && cd build

# Configure with CMake
cmake ..

# Build
make

# Run (example)
./vis --network ../../../testing/xor/xor_network.net
```

#### macOS

```bash
cd src/vis
mkdir build && cd build
cmake ..
make
./vis --network ../../../testing/xor/xor_network.net
```

#### Windows (Visual Studio)

```bash
cd src\vis
mkdir build
cd build
cmake ..
cmake --build . --config Release
.\Release\vis.exe --network ..\..\..\testing\xor\xor_network.net
```

### Build Verification

If the build succeeds, you should see:
```
-- The C compiler identification is ...
-- The CXX compiler identification is ...
-- Found OpenGL: ...
-- Found GLFW: ...
-- Found GLEW: ...
-- Configuring done
-- Generating done
-- Build files written to: ...
```

Then during compilation:
```
[ 10%] Building CXX object CMakeFiles/vis.dir/main.cpp.o
[ 20%] Building CXX object CMakeFiles/vis.dir/argparser.cpp.o
...
[100%] Linking CXX executable vis
[100%] Built target vis
```

### Testing Without Full Build

If you're in an environment without OpenGL (like WSL without X11), you can test core components:

```bash
cd src/vis

# Test basic neuron particle functionality
make -f Makefile.simple
./test_network_simple

# Expected output:
# === NetworkGraph Simple Test ===
# ✓ NeuronParticle test passed!
# ✓ Network loading: ✓
# ✓ Network execution: ✓
# === All Tests Passed! ===
```

## Troubleshooting

### CMake Can't Find Libraries

**Error:**
```
Could not find a package configuration file provided by "GLFW"
```

**Solution (Linux/WSL):**
```bash
sudo apt-get install libglfw3-dev
```

**Solution (macOS):**
```bash
brew install glfw
```

**Solution (Windows):**
Use vcpkg:
```bash
vcpkg install glfw3 glew glm
cmake .. -DCMAKE_TOOLCHAIN_FILE=path/to/vcpkg/scripts/buildsystems/vcpkg.cmake
```

### OpenGL Version Too Old

**Error:**
```
OpenGL version 2.1 detected, but 3.3 required
```

**Solution:**
- Update graphics drivers
- Use software rendering (Mesa): `export LIBGL_ALWAYS_SOFTWARE=1`
- Use a machine with newer GPU

### WSL: No Display Available

**Error:**
```
Failed to open GLFW window
Cannot open display
```

**Solution:**
1. Install X Server on Windows (VcXsrv or Xming)
2. Export display:
   ```bash
   export DISPLAY=:0
   ```
3. Rerun the program

### Link Errors with Arch Files

**Error:**
```
undefined reference to `Glia::step()'
```

**Solution:**
Ensure `CMakeLists.txt` includes:
```cmake
${PROJECT_SOURCE_DIR}/../arch/glia.cpp
${PROJECT_SOURCE_DIR}/../arch/neuron.cpp
```

And:
```cmake
include_directories(${PROJECT_SOURCE_DIR}/../arch)
```

### Compilation Errors in network_graph.cpp

**Error:**
```
no match for 'operator/' (operand types are 'Vec3f' and 'double')
```

**Solution:**
This should be fixed in the latest version. If you encounter it:
```cpp
// Wrong:
Vec3f dir = diff / length;

// Correct:
Vec3f dir = diff;
dir /= float(length);
```

## Build Targets

### Main Visualization

**Target:** `vis`
**Command:** `./vis --network path/to/network.net`
**Purpose:** Full 3D visualization with OpenGL

### Simple Test (No OpenGL)

**Target:** `test_network_simple`
**Command:** `make -f Makefile.simple && ./test_network_simple`
**Purpose:** Test core NetworkGraph without rendering

### Full Test (Requires OpenGL stubs)

**Target:** `test_network_graph`
**Command:** `make -f Makefile.test && ./test_network_graph`
**Purpose:** Test full NetworkGraph including rendering data packing

## File Structure After Build

```
src/vis/
├── build/               # CMake build directory
│   ├── vis              # Main executable (Linux/Mac)
│   ├── vis.exe          # Main executable (Windows)
│   └── CMakeFiles/      # Build artifacts
├── *.cpp, *.h           # Source files
├── CMakeLists.txt       # Build configuration
├── Makefile.simple      # Minimal test build
└── Makefile.test        # Full test build
```

## Platform-Specific Notes

### Linux
- Tested on Ubuntu 20.04 LTS
- Requires X11 for display
- Use `libglfw3-dev` (version 3.3+)

### WSL (Windows Subsystem for Linux)
- Requires X Server on Windows host
- May have performance issues
- Consider native Windows build instead

### macOS
- Uses Metal backend (Objective-C code in codebase)
- Requires Xcode command line tools
- May need code signing for app bundle

### Windows
- Requires Visual Studio 2019+
- Use vcpkg for dependencies
- May need to adjust CMake generator:
  ```bash
  cmake -G "Visual Studio 16 2019" ..
  ```

## Quick Start Commands

### Full Build and Run (Linux)
```bash
cd src/vis && \
  mkdir build && cd build && \
  cmake .. && \
  make && \
  ./vis --network ../../../testing/xor/xor_network.net
```

### Simple Test (No OpenGL)
```bash
cd src/vis && \
  make -f Makefile.simple clean && \
  make -f Makefile.simple && \
  ./test_network_simple
```

### Rebuild from Scratch
```bash
cd src/vis
rm -rf build
mkdir build && cd build
cmake ..
make clean
make
```

## Performance Expectations

| Network Size | Expected FPS | Notes |
|--------------|--------------|-------|
| XOR (5 neurons) | 60 FPS | Smooth, no lag |
| Medium (100 neurons) | 30-60 FPS | Depends on GPU |
| Large (1000 neurons) | 10-30 FPS | May need optimization |
| Huge (10000+ neurons) | <10 FPS | Needs spatial partitioning |

## Next Steps After Successful Build

1. **Run XOR visualization:**
   ```bash
   ./vis --network ../../../testing/xor/xor_network.net
   ```

2. **Test keyboard controls:**
   - Press `A` to start animation
   - Press `T` for training mode
   - Press `I` for inference mode
   - Press `1` and `2` to inject inputs

3. **Explore camera:**
   - Left drag to rotate
   - Right drag to pan
   - Scroll to zoom

4. **Toggle display:**
   - Press `P` to toggle neurons
   - Press `W` to toggle connections
   - Press `B` to toggle bounding box

5. **Read the usage guide:**
   ```bash
   cat ../../docs/RENDERING_USAGE.md
   ```

## Support

If you encounter issues:
1. Check this documentation
2. Verify all prerequisites are installed
3. Check CMake output for specific errors
4. Try the simple test first to isolate OpenGL issues
5. Consult `docs/RENDERING_USAGE.md` for runtime issues

---

**Happy Building!** 🔨
