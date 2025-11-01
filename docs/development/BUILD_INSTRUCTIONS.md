# GliaGL Build Instructions

Note: Prefer the training CLI in `src/train/` (WSL/Linux recommended). The OpenGL visualizer is frozen and may be out-of-date; keep for reference.

## Training CLI Build (WSL/Linux)

```bash
cd src/train
mkdir -p build && cd build
cmake ..
cmake --build . --config Release
```

This produces `glia_eval` (or `glia_eval.exe`).

### Usage

```bash
# XOR crafted
glia_eval[.exe] --scenario xor --default O0

# XOR baseline (dysfunctional)
glia_eval[.exe] --scenario xor --baseline --default O0

# 3-class crafted with noise
glia_eval[.exe] --scenario 3class --noise 0.10

# Custom net
glia_eval[.exe] --net ..\examples\xor\xor_network.net --default O0
```

Options:
- `--warmup U` and `--window W` control ticks before decision and decision window
- `--alpha A` sets EMA smoothing
- `--threshold T` sets activity threshold
- `--default ID` sets default output when below threshold (e.g., `O0`)

## Visualization Build (reference)

**Required Libraries:**
- OpenGL 3.3+ (or Metal on macOS)
- GLFW 3.x
- GLEW (Linux/Windows) or built-in OpenGL loader (macOS)
- GLM (OpenGL Mathematics)
- C++11 compatible compiler

```
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
  ./vis --network ../../../examples/xor/xor_network.net
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
   ./vis --network ../../../examples/xor/xor_network.net
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
