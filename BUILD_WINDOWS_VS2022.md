# Windows Build Instructions (Visual Studio 2022)

## Your Existing Workflow - Now with Network Support!

### Build Process (Same as Before)

```cmd
REM 1. Open Developer Command Prompt for VS 2022
REM 2. Navigate to GliaGL directory
cd GliaGL

REM 3. Create/enter build directory
mkdir build
cd build

REM 4. Generate Visual Studio solution
cmake -G"Visual Studio 17" -A x64 ../src/vis

REM 5. Build the project
cmake --build .
```

### Running the Visualization

#### Original Cloth Simulation (Still Works!)
```cmd
debug\vis.exe --cloth ../src/vis/cloth_flag.txt --size 1500 1500
```

#### NEW: Neural Network Visualization
```cmd
debug\vis.exe --network ../../src/testing/xor/xor_network.net --size 1500 1500
```

### Complete Workflow Example

```cmd
REM Clean build from scratch
cd C:\Users\jaden\OneDrive\Documents\AI\GliaGL
rmdir /s /q build
mkdir build
cd build

cmake -G"Visual Studio 17" -A x64 ../src/vis

REM Build
cmake --build .

REM# Run XOR network visualization
debug\vis.exe --network ../../src/testing/xor/xor_network.net --size 1500 1500

### Essential
- `A` - Start animation
- `X` - Stop animation
- `Q` - Quit

### Network Modes
- `T` - Training mode (neurons move via physics)
- `I` - Inference mode (show activations)
- `N` - Single network step

### Input Injection
- `1` - Inject S0 = 200.0
- `2` - Inject S1 = 200.0
- `0` - Clear inputs

### Display Toggles
- `P` - Toggle neurons (particles)
- `W` - Toggle connections (wireframe)
- `B` - Toggle bounding box

### Mouse
- **Left drag** - Rotate camera
- **Right drag** - Pan camera
- **Scroll** - Zoom

## Expected Output

### Console Output
```
-------------------------------------------------------
OpenGL Version: [your version]
-------------------------------------------------------
Loading network from: ../testing/xor/xor_network.net
Network configuration loaded from ../testing/xor/xor_network.net
Network loaded and spatialized successfully
Compiling shader : ../src/vis/OpenGL.vertexshader
Compiling shader : ../src/vis/OpenGL.fragmentshader
Linking program
```

### Visual Output
- **Window opens** with 3D visualization
- **Left side**: 2 blue dots (sensory neurons S0, S1)
- **Center/Right**: 3 gray/purple dots (interneurons + outputs)
- **Lines**: Green (excitatory) and red (inhibitory) connections

## Troubleshooting

### Build Errors

**Error: Cannot find GLFW, GLEW, or GLM**

These libraries should be automatically found on Windows with Visual Studio. If not:

```cmd
REM Install using vcpkg
vcpkg install glfw3:x64-windows
vcpkg install glew:x64-windows
vcpkg install glm:x64-windows

REM Then configure with vcpkg toolchain
cmake -G"Visual Studio 17" -A x64 -DCMAKE_TOOLCHAIN_FILE=[path-to-vcpkg]/scripts/buildsystems/vcpkg.cmake ../src/vis
```

**Error: Network file not found**

Make sure the path is relative to the build directory:
```cmd
REM From GliaGL/build/
debug\vis.exe --network ../testing/xor/xor_network.net
```

### Runtime Issues

**Neurons not visible**
- Press `P` to toggle particles on
- Press `A` to start animation

**Connections not visible**
- Press `W` to toggle wireframe (connections) on

**Neurons not moving in training mode**
- Press `T` to ensure training mode is active
- Press `A` to start animation

**Neurons not lighting up in inference mode**
- Press `I` for inference mode
- Press `1` or `2` to inject inputs
- Press `N` to step the network

## Testing Without Full Build

If you want to test just the core logic without OpenGL:

```cmd
cd src\vis

REM Windows equivalent of Makefile.simple
cl /EHsc /std:c++11 /I. /I..\arch test_network_simple.cpp neuron_particle.cpp network_graph.cpp ..\arch\glia.cpp ..\arch\neuron.cpp /Fe:test_network_simple.exe

REM Run test
test_network_simple.exe
```

## Debugging in Visual Studio

### Option 1: Command Line Arguments

1. Open `GliaGL/build/vis.sln` in Visual Studio
2. Right-click `vis` project → Properties
3. Configuration Properties → Debugging
4. Command Arguments: `--network ../testing/xor/xor_network.net --size 1500 1500`
5. Working Directory: `$(OutDir)` (should be `debug\`)
6. Press F5 to debug

### Option 2: Command Prompt

```cmd
cd GliaGL\build

REM Build with debug symbols
cmake --build . --config Debug

REM Run with debugger attached
devenv debug\vis.exe --network ../testing/xor/xor_network.net
```

## File Paths Reference

When running from `GliaGL/build/debug/`:

```
GliaGL/
├── build/
│   └── debug/
│       └── vis.exe          ← You are here
│
├── src/
│   └── vis/
│       ├── cloth_flag.txt   → ../src/vis/cloth_flag.txt
│       ├── OpenGL.*shader   → ../src/vis/OpenGL.*shader (auto-found)
│       └── [other files]
│
└── testing/
    └── xor/
        └── xor_network.net  → ../testing/xor/xor_network.net
```

## Quick Commands Cheatsheet

```cmd
REM Full rebuild
cd C:\Users\jaden\OneDrive\Documents\AI\GliaGL
rmdir /s /q build && mkdir build && cd build
cmake -G"Visual Studio 17" -A x64 ../src/vis && cmake --build .

REM Run cloth (original)
debug\vis.exe --cloth ../src/vis/cloth_flag.txt --size 1500 1500

REM Run XOR network
debug\vis.exe --network ../testing/xor/xor_network.net --size 1500 1500

REM Rebuild only
cd build && cmake --build . && cd ..

REM Clean build
cd build && cmake --build . --clean-first
```

## Performance Notes

- **XOR (5 neurons)**: Should run at 60 FPS easily
- **Medium networks (<100 neurons)**: 30-60 FPS
- **Large networks (>1000)**: May need optimization

## Next Steps

1. **Build it**: Follow the build process above
2. **Run XOR**: Use the network command
3. **Explore**: Try the keyboard controls
4. **Test modes**:
   - Training mode: Watch neurons organize spatially
   - Inference mode: Inject inputs and see activations
5. **Create your own**: Make custom .net files

## What's Different from Cloth?

**Exactly the same workflow, just:**
- Replace `--cloth file.txt` with `--network file.net`
- Different keyboard controls (T/I/N/1/2 for network)
- Different visual (dots + lines instead of mesh)

**Everything else identical:**
- Same build process
- Same cmake commands
- Same window size flags
- Same camera controls
- Same debug process

---

**Your existing workflow is preserved - just added network support!** 🎉
