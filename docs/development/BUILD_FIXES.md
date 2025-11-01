# Build Fixes - Stage 6

## Issues Discovered During Verification

While testing Stage 6, we discovered critical build issues that prevented `pip install -e .` from working on both Windows and WSL/Linux. These have all been fixed.

## Problems and Solutions

**Summary**: 7 issues fixed
- 5 issues on Windows (compilation and import errors)
- 1 additional issue on WSL/Linux (linking error)
- 1 example issue (missing get_neuron method)

### 1. InputSequence `addTimestep` Method Missing

**Error**:
```
error: 'class InputSequence' has no member named 'addTimestep'
```

**Location**: `python/src/bind_input_sequence.cpp:35`

**Root Cause**: The binding tried to call `addTimestep()` but the C++ class only has `addEvent(tick, neuron_id, value)`.

**Solution**: Implemented `add_timestep` as a lambda that:
1. Determines the next tick (0 for empty, max+1 otherwise)
2. Calls `addEvent()` for each input in the map

**Code**:
```cpp
.def("add_timestep", [](InputSequence &self, const std::map<std::string, float> &inputs) {
    int tick = self.isEmpty() ? 0 : (self.getMaxTick() + 1);
    for (const auto &pair : inputs) {
        self.addEvent(tick, pair.first, pair.second);
    }
},
py::arg("inputs"),
"Add a timestep with input values")
```

---

### 2. Neuron Incomplete Type Error

**Error**:
```
error: invalid use of incomplete type 'class Neuron'
error: 'class Neuron' has no member named 'addTimestep'
```

**Location**: `python/src/bind_network.cpp`

**Root Cause**: `bind_network.cpp` uses `std::shared_ptr<Neuron>` in method signatures, but only had a forward declaration. pybind11 needs the full type definition for shared_ptr bindings.

**Solution**: Include the full header:
```cpp
#include "../../src/arch/neuron.h"  // Need full definition for shared_ptr in method signatures
```

**Note**: This doesn't cause double-registration because the class is only bound once in `bind_neuron.cpp`.

---

### 3. Default Argument Type Not Registered

**Error**:
```
ImportError: arg(): could not convert default argument into a Python object (type not registered yet?).
```

**Location**: `python/src/bind_evolution.cpp:101`

**Root Cause**: The EvolutionEngine constructor had a default argument:
```cpp
py::arg("callbacks") = EvolutionEngine::Callbacks()
```

But `Callbacks` was bound AFTER `EvolutionEngine`, so pybind11 couldn't convert the default value.

**Solution**: 
1. Moved `Callbacks` binding before `EvolutionEngine`
2. Provided two constructor overloads instead of using a default argument:
   - One without callbacks (most common)
   - One with callbacks (advanced use)

**Code**:
```cpp
// Callbacks struct (bind before EvolutionEngine to avoid default arg issues)
py::class_<EvolutionEngine::Callbacks>(m, "EvolutionCallbacks")
    .def(py::init<>());

// EvolutionEngine class
py::class_<EvolutionEngine, std::shared_ptr<EvolutionEngine>>(m, "EvolutionEngine", ...)
    // Constructor without callbacks (most common)
    .def(py::init<...>(), ...)
    // Constructor with callbacks (advanced)
    .def(py::init<..., const EvolutionEngine::Callbacks&>(), ...)
```

---

### 4. train_batch Optional Argument Issue

**Error**: Same as #3 - default argument conversion

**Location**: `python/src/bind_training.cpp:112`

**Root Cause**: `nullptr` default argument for `metrics_out` pointer parameter.

**Solution**: Wrap in lambda that explicitly passes `nullptr`:
```cpp
.def("train_batch", [](Trainer &self, 
                        const std::vector<Trainer::EpisodeData> &batch,
                        const TrainingConfig &config) {
    py::gil_scoped_release release;
    self.trainBatch(batch, config, nullptr);
},
py::arg("batch"), py::arg("config"),
"Train on a batch of episodes (GIL released)")
```

---

### 5. pyproject.toml Missing _core Module

**Error**: Module imports but `_core` not found

**Location**: `pyproject.toml:55`

**Root Cause**: Had `install.components = ["python"]` which excluded the compiled `_core` extension.

**Solution**: Removed the line - scikit-build-core automatically includes all install() targets:
```toml
[tool.scikit-build]
cmake.build-type = "Release"
cmake.source-dir = "python"
wheel.packages = ["python/glia"]
# Removed: install.components = ["python"]
```

**Verification**: Wheel size increased from 7KB → 235KB (includes C++ module)

---

## Testing

After all fixes:

**Quick Test**:
```
✓ Imported glia v0.1.0
✓ Created network: <Network neurons=5 connections=0>
```

**Comprehensive Test** (10 tests):
```
✓ Import and version
✓ Network creation  
✓ State access
✓ State modification
✓ Injection and simulation
✓ Weights
✓ Save/Load
✓ Training config
✓ Dataset
✓ Trainer

ALL TESTS PASSED! ✅
```

---

## Best Practices Learned

### 1. pybind11 Type Registration Order
- Bind types before they're used in default arguments
- Bind types before they're used in return types (if using holders)
- Use forward declarations only when not needed for bindings

### 2. Default Arguments in Bindings
- Avoid C++ objects as default arguments
- Use constructor overloads instead
- Use lambdas to provide defaults explicitly

### 3. Incomplete Types
- Include full headers when types are used in signatures
- Even if bound elsewhere, the definition is needed for method binding
- Multiple includes are fine - class is only bound once

### 4. Package Structure
- Don't over-constrain install components
- Let scikit-build-core handle installation automatically
- Verify wheel size includes compiled modules

### 5. Testing
- Always test `pip install -e .` before assuming build works
- Test import before assuming package works
- Create comprehensive tests covering all major features

---

### 6. Missing -fPIC for Static Library (WSL/Linux)

**Error**: 
```
relocation R_X86_64_PC32 against symbol can not be used when making a shared object; 
recompile with -fPIC
```

**Location**: `python/CMakeLists.txt`

**Root Cause**: The static library `glia_core` needs to be compiled with Position Independent Code (`-fPIC`) on Linux because it's linked into a shared library (the Python extension). This is required on Linux but not enforced on Windows.

**Solution**: Set the CMake property:
```cmake
# Enable PIC for static library (required for linking into shared library on Linux)
set_target_properties(glia_core PROPERTIES POSITION_INDEPENDENT_CODE ON)
```

**Why this matters**: Without `-fPIC`, the linker cannot create relocatable code suitable for shared libraries on Linux/WSL.

---

### 7. get_neuron Method Commented Out (Examples Failed)

**Error**:
```
AttributeError: 'glia._core.Network' object has no attribute 'get_neuron'
```

**Location**: `python/src/bind_network.cpp:47-50`

**Root Cause**: During the initial build fixes, `get_neuron()` was commented out because of type registration issues with Neuron. However, this method is used by the examples for accessing individual neurons.

**Solution**: Re-enabled the method after verifying:
1. Neuron is bound before Network in `bind_core.cpp` (line 39 before 40)
2. `neuron.h` is included in `bind_network.cpp` for complete type definition
3. All other type registration issues were fixed

**Code**:
```cpp
// Neuron access
.def("get_neuron", &Glia::getNeuronById,
     py::arg("neuron_id"),
     "Get neuron by ID")
```

**Impact**: All 5 Python examples now work correctly.

---

## Files Modified

1. `python/src/bind_input_sequence.cpp` - Fixed `add_timestep` implementation
2. `python/src/bind_network.cpp` - Added neuron.h include + re-enabled `get_neuron()`
3. `python/src/bind_evolution.cpp` - Reordered bindings, removed default args
4. `python/src/bind_training.cpp` - Wrapped train_batch in lambda
5. `pyproject.toml` - Removed install.components constraint
6. `python/CMakeLists.txt` - Added POSITION_INDEPENDENT_CODE for Linux/WSL
7. `python/test_import.py` - Fixed to use properties instead of methods

---

## Impact

**Before Fixes**:
- ❌ `pip install -e .` failed with compilation errors
- ❌ Module could not be imported
- ❌ No Python API available

**After Fixes**:
- ✅ Clean compilation on Windows and WSL
- ✅ Module imports successfully
- ✅ All API features work correctly
- ✅ Ready for Stage 7

---

**Status**: ALL BUILD ISSUES RESOLVED ✅

The Python API is now fully functional and ready for documentation and polish (Stage 7).
