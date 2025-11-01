# GliaGL Documentation

Welcome to GliaGL's comprehensive documentation! This guide will help you find exactly what you need.

---

## 📚 Quick Navigation

### For New Users
👉 Start here: **[Quickstart Guide](user-guide/QUICKSTART.md)** (5 minutes)

### For Experienced Users
👉 Jump to: **[API Reference](user-guide/API_REFERENCE.md)** (Complete API)

### For CLI Migrators
👉 Read: **[Migration Guide](user-guide/MIGRATION_GUIDE.md)** (CLI → Python)

### For Researchers
👉 Explore: **[Advanced Usage](user-guide/ADVANCED_USAGE.md)** (Advanced techniques)

---

## 📖 Documentation Structure

### [user-guide/](user-guide/) - User Documentation

**Essential reading for all users:**

- **[QUICKSTART.md](user-guide/QUICKSTART.md)** - Get started in 5 minutes
  - Installation
  - First network
  - Training basics
  - Complete workflow example
  
- **[API_REFERENCE.md](user-guide/API_REFERENCE.md)** - Complete API documentation
  - All classes and methods
  - Parameters and return values
  - Code examples
  - Performance tips
  
- **[ADVANCED_USAGE.md](user-guide/ADVANCED_USAGE.md)** - Advanced techniques
  - Evolutionary training
  - Custom training loops
  - Network architectures
  - Performance optimization
  - NumPy advanced integration
  - Hyperparameter tuning
  
- **[MIGRATION_GUIDE.md](user-guide/MIGRATION_GUIDE.md)** - Migrating from CLI
  - CLI → Python mapping table
  - Data format conversion
  - Workflow examples
  - Shell script migration
  
- **[QUICK_REFERENCE.md](user-guide/QUICK_REFERENCE.md)** - Quick lookup
  - Common commands
  - Code snippets
  - Troubleshooting

---

### [development/](development/) - Development Documentation

**For contributors and advanced developers:**

- **[BUILD_INSTRUCTIONS.md](development/BUILD_INSTRUCTIONS.md)** - Building from source
  - Platform-specific instructions
  - Dependencies
  - Troubleshooting
  
- **[VENV_SETUP.md](development/VENV_SETUP.md)** - Virtual environment setup
  - WSL/Linux setup
  - Windows setup
  - IDE configuration
  
- **[BUILD_FIXES.md](development/BUILD_FIXES.md)** - Known build issues
  - 7 critical issues and fixes
  - Platform-specific problems
  - Solutions and workarounds
  
- **[development_guide.md](development/development_guide.md)** - Development workflow
  - Code style
  - Testing
  - Pull requests
  
- **[release_process.md](development/release_process.md)** - Release workflow
  - Version management
  - Publishing to PyPI
  - Release checklist
  
- **[numpy_interface.md](development/numpy_interface.md)** - NumPy integration details
  - Zero-copy arrays
  - Memory layout
  - Performance
  
- **[gil_release_points.md](development/gil_release_points.md)** - Threading details
  - GIL release points
  - Thread safety
  - Parallel execution

---

### [reference/](reference/) - Technical Reference

**Deep-dive technical documentation:**

- **[OUTPUT_DETECTION_OPTIONS.md](reference/OUTPUT_DETECTION_OPTIONS.md)** - Output detection
  - Detection strategies
  - EMA rates
  - Margin thresholds
  
- **[architecture/README.md](architecture/README.md)** - Architecture overview
  - System design
  - Core components
  - Data flow

---

### [project-history/](project-history/) - Project History

**Historical documentation and transformation records:**

- **[TRANSFORMATION.md](project-history/TRANSFORMATION.md)** - Python transformation history
  - All 7 stages
  - Key achievements
  - Lessons learned
  
- **[REORGANIZATION.md](project-history/REORGANIZATION.md)** - Repository cleanup
  - Cleanup process
  - File movements
  - Structure changes
  
- **[DEPRECATED.md](project-history/DEPRECATED.md)** - Deprecated features
  - CLI tools
  - Migration timeline
  - Alternatives
  
- **[CLEANUP_AUDIT.md](project-history/CLEANUP_AUDIT.md)** - Cleanup audit
  - What was removed
  - What was kept
  - Rationale
  
- **[EXAMPLES_VERIFIED.md](project-history/EXAMPLES_VERIFIED.md)** - Example verification
  - Test results
  - Issues found
  - Fixes applied
  
- **[PYTHON_OVERHAUL_PLAN.md](project-history/PYTHON_OVERHAUL_PLAN.md)** - Original plan
  - Initial design
  - Stage definitions
  - Goals and objectives
  
- **[BUILD_WINDOWS_VS2022.md](project-history/BUILD_WINDOWS_VS2022.md)** - Windows build history
  - MSVC-specific instructions
  - Historical context
  
- **[PYTHON_TRANSFORMATION_COMPLETE.md](project-history/PYTHON_TRANSFORMATION_COMPLETE.md)** - Final summary
  - Complete transformation overview
  - Statistics
  - Next steps

---

### [archive/](archive/) - Archived Documentation

**Outdated or historical documentation:**

- Old CLI-focused guides
- Superseded technical plans
- Historical progress reports
- Legacy visualization docs

See [archive/README.md](archive/README.md) for details.

---

## 🚀 Getting Started

### Installation
```bash
pip install -e .
```

See [../INSTALL.md](../INSTALL.md) for detailed installation instructions.

### First Steps

1. **Install**: Follow [../INSTALL.md](../INSTALL.md)
2. **Quickstart**: Read [user-guide/QUICKSTART.md](user-guide/QUICKSTART.md)
3. **Examples**: Try `examples/python/01_basic_simulation.py`
4. **Explore**: Browse [user-guide/API_REFERENCE.md](user-guide/API_REFERENCE.md)

### Learning Path

**Beginner** → **Intermediate** → **Advanced**

```
QUICKSTART.md (5 min)
    ↓
Run examples (10 min)
    ↓
API_REFERENCE.md (as needed)
    ↓
ADVANCED_USAGE.md (advanced patterns)
```

---

## 🔍 Finding What You Need

### By Topic

| Topic | Document |
|-------|----------|
| Installation | [../INSTALL.md](../INSTALL.md) |
| Quick start | [user-guide/QUICKSTART.md](user-guide/QUICKSTART.md) |
| API reference | [user-guide/API_REFERENCE.md](user-guide/API_REFERENCE.md) |
| Training | [user-guide/API_REFERENCE.md#training](user-guide/API_REFERENCE.md) |
| Evolution | [user-guide/ADVANCED_USAGE.md#evolutionary-training](user-guide/ADVANCED_USAGE.md) |
| NumPy | [user-guide/API_REFERENCE.md#numpy-integration](user-guide/API_REFERENCE.md) |
| Visualization | [user-guide/API_REFERENCE.md#visualization](user-guide/API_REFERENCE.md) |
| Building | [development/BUILD_INSTRUCTIONS.md](development/BUILD_INSTRUCTIONS.md) |
| Contributing | [../CONTRIBUTING.md](../CONTRIBUTING.md) |
| Migration | [user-guide/MIGRATION_GUIDE.md](user-guide/MIGRATION_GUIDE.md) |
| Architecture | [reference/architecture/README.md](reference/architecture/README.md) |
| History | [project-history/TRANSFORMATION.md](project-history/TRANSFORMATION.md) |

### By User Type

**New User**:
1. [../INSTALL.md](../INSTALL.md)
2. [user-guide/QUICKSTART.md](user-guide/QUICKSTART.md)
3. `examples/python/01_basic_simulation.py`

**CLI Migrator**:
1. [project-history/DEPRECATED.md](project-history/DEPRECATED.md)
2. [user-guide/MIGRATION_GUIDE.md](user-guide/MIGRATION_GUIDE.md)
3. [user-guide/API_REFERENCE.md](user-guide/API_REFERENCE.md)

**Researcher**:
1. [user-guide/ADVANCED_USAGE.md](user-guide/ADVANCED_USAGE.md)
2. [user-guide/API_REFERENCE.md](user-guide/API_REFERENCE.md)
3. [development/numpy_interface.md](development/numpy_interface.md)

**Contributor**:
1. [../CONTRIBUTING.md](../CONTRIBUTING.md)
2. [development/development_guide.md](development/development_guide.md)
3. [development/BUILD_INSTRUCTIONS.md](development/BUILD_INSTRUCTIONS.md)

---

## 📝 Documentation Standards

All GliaGL documentation follows these standards:

- **Markdown format**: All docs in .md format
- **Code examples**: Working, tested code snippets
- **Progressive complexity**: Easy → Advanced
- **Cross-referenced**: Links to related docs
- **Up-to-date**: Maintained alongside code

---

## 🤝 Contributing to Documentation

Found an issue or want to improve the docs?

1. See [../CONTRIBUTING.md](../CONTRIBUTING.md)
2. Follow documentation standards above
3. Submit a pull request

Documentation improvements are always welcome!

---

## 📊 Documentation Statistics

- **User Guides**: 5 comprehensive guides (2,200+ lines)
- **Development Docs**: 7 technical guides
- **Examples**: 5 Python scripts + 4 Jupyter notebooks + 3 full examples
- **API Coverage**: 100% of public API
- **Code Examples**: 100+ tested snippets

---

## ❓ Need Help?

- **Quick questions**: See [user-guide/QUICK_REFERENCE.md](user-guide/QUICK_REFERENCE.md)
- **Installation issues**: [../INSTALL.md](../INSTALL.md) troubleshooting section
- **API questions**: [user-guide/API_REFERENCE.md](user-guide/API_REFERENCE.md)
- **Build problems**: [development/BUILD_FIXES.md](development/BUILD_FIXES.md)
- **GitHub Issues**: Report bugs or ask questions
- **GitHub Discussions**: Community help

---

## 🎯 Documentation Roadmap

**Current** (✅ Complete):
- Comprehensive user guides
- Complete API reference
- Migration documentation
- Development guides
- Project history

**Future** (Optional):
- [x] Interactive Jupyter tutorials (COMPLETE - 4 notebooks)
- [ ] Video walkthroughs
- [ ] Sphinx/ReadTheDocs hosting
- [ ] Translated documentation
- [ ] FAQ section
- [ ] Glossary of terms

---

**Last Updated**: October 31, 2025  
**Documentation Version**: 1.0  
**GliaGL Version**: 0.1.0
