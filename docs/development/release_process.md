# Release Process

This document describes the release process for GliaGL.

## Pre-Release Checklist

### 1. Code Quality

- [ ] All tests pass locally
- [ ] All CI checks pass on GitHub
- [ ] No critical bugs or issues
- [ ] Code review completed for major changes
- [ ] Documentation is up-to-date

### 2. Version Bump

Update version number in:
- [ ] `pyproject.toml`
- [ ] `python/glia/__init__.py` (if different)
- [ ] `python/src/bind_core.cpp` (if different)

### 3. Changelog

- [ ] Update `CHANGELOG.md` with all changes since last release
- [ ] Categorize changes: Added, Changed, Deprecated, Removed, Fixed, Security
- [ ] Include migration notes if breaking changes
- [ ] Set release date

### 4. Documentation

- [ ] README is accurate and up-to-date
- [ ] API documentation reflects current state
- [ ] Installation instructions tested
- [ ] Examples work with new version

## Release Types

### Patch Release (0.1.X)

**When**: Bug fixes, minor improvements, no API changes

```bash
# Example: 0.1.0 -> 0.1.1
```

**Process**: Simple, fast

### Minor Release (0.X.0)

**When**: New features, backward-compatible changes

```bash
# Example: 0.1.0 -> 0.2.0
```

**Process**: Standard release

### Major Release (X.0.0)

**When**: Breaking changes, major refactoring

```bash
# Example: 0.9.0 -> 1.0.0
```

**Process**: Extended testing, migration guide required

## Release Steps

### 1. Prepare Release Branch

```bash
# From main branch
git checkout main
git pull origin main

# Create release branch
git checkout -b release/v0.1.0
```

### 2. Update Version

Edit `pyproject.toml`:
```toml
[project]
name = "glia"
version = "0.1.0"  # Update this
```

### 3. Update Changelog

Edit `CHANGELOG.md`:
```markdown
## [0.1.0] - 2025-01-15

### Added
- Feature X
- Feature Y

### Changed
- Improvement Z

### Fixed
- Bug fix A
```

### 4. Commit Changes

```bash
git add pyproject.toml CHANGELOG.md
git commit -m "Bump version to 0.1.0"
git push origin release/v0.1.0
```

### 5. Create Pull Request

- Create PR from `release/v0.1.0` to `main`
- Title: "Release v0.1.0"
- Description: Changelog summary
- Wait for CI to pass
- Get approval
- Merge to main

### 6. Tag Release

```bash
# Switch to main after merge
git checkout main
git pull origin main

# Create annotated tag
git tag -a v0.1.0 -m "Release version 0.1.0"

# Push tag
git push origin v0.1.0
```

### 7. GitHub Release

The tag push triggers:
- GitHub Actions workflow `build-wheels.yml`
- Builds wheels for Linux, macOS, Windows
- Builds source distribution
- Uploads to PyPI (if configured)

**Manual GitHub Release**:
1. Go to GitHub Releases
2. Click "Draft a new release"
3. Select tag `v0.1.0`
4. Title: "GliaGL v0.1.0"
5. Description: Copy from CHANGELOG
6. Attach any additional files
7. Click "Publish release"

### 8. Verify PyPI Upload

```bash
# Wait for GitHub Actions to complete

# Check PyPI
# https://pypi.org/project/glia/

# Test installation
pip install glia==0.1.0

# Verify
python -c "import glia; print(glia.__version__)"
```

### 9. Announce Release

- [ ] Update README badges if needed
- [ ] Post announcement (GitHub Discussions, Twitter, etc.)
- [ ] Update documentation site if applicable

## Post-Release

### 1. Update Development Version

On main branch, bump to next development version:

```toml
# pyproject.toml
version = "0.2.0.dev0"
```

### 2. Close Milestone

- Close GitHub milestone for this release
- Create milestone for next release

### 3. Monitor Issues

- Watch for bug reports related to new release
- Prepare patch release if critical bugs found

## Hotfix Process

For critical bugs in released version:

```bash
# Create hotfix branch from release tag
git checkout -b hotfix/v0.1.1 v0.1.0

# Make fix
# Edit files...

# Update version
# pyproject.toml: 0.1.0 -> 0.1.1

# Commit
git add .
git commit -m "Fix critical bug X"

# Tag
git tag -a v0.1.1 -m "Hotfix: Fix critical bug X"

# Push
git push origin hotfix/v0.1.1
git push origin v0.1.1

# Merge back to main
git checkout main
git merge hotfix/v0.1.1
git push origin main
```

## Automation

### GitHub Actions (Configured)

`.github/workflows/build-wheels.yml`:
- Triggers on tag push
- Builds wheels for all platforms
- Uploads to PyPI using `PYPI_API_TOKEN` secret

### Manual PyPI Upload

If automation fails:

```bash
# Build locally
python -m build

# Install twine
pip install twine

# Upload to TestPyPI first
twine upload --repository testpypi dist/*

# Test installation from TestPyPI
pip install --index-url https://test.pypi.org/simple/ glia

# Upload to PyPI
twine upload dist/*
```

## Rolling Back

If a release has critical issues:

1. **Yank release on PyPI** (doesn't delete, just hides):
   ```bash
   pip install twine
   twine yank glia==0.1.0 -r pypi -m "Critical bug, use 0.1.1 instead"
   ```

2. **Create hotfix release** (0.1.1) immediately

3. **Update documentation** to warn users

4. **Announce** the issue and fix

## Version Numbering

Follow [Semantic Versioning](https://semver.org/):

**Format**: MAJOR.MINOR.PATCH

- **MAJOR**: Breaking changes
- **MINOR**: New features (backward compatible)
- **PATCH**: Bug fixes (backward compatible)

**Examples**:
- `0.1.0`: Initial release
- `0.1.1`: Bug fix
- `0.2.0`: New features
- `1.0.0`: Stable API, production ready

**Pre-release versions**:
- `0.1.0a1`: Alpha 1
- `0.1.0b1`: Beta 1
- `0.1.0rc1`: Release candidate 1

## Release Artifacts

Each release should include:
- [ ] Source distribution (`.tar.gz`)
- [ ] Wheels for:
  - [ ] Linux (manylinux)
  - [ ] macOS (x86_64, arm64)
  - [ ] Windows (win_amd64)
- [ ] GitHub Release with:
  - [ ] Changelog
  - [ ] Installation instructions
  - [ ] Upgrade notes

## Testing Release

Before final release:

```bash
# Build locally
python -m build

# Install from wheel
pip install dist/glia-0.1.0-*.whl

# Run full test suite
python python/test_import.py
python python/test_api.py
python python/examples/quick_start.py

# Test in fresh environment
python -m venv test_env
source test_env/bin/activate
pip install dist/glia-0.1.0-*.whl
python -c "import glia; print(glia.info())"
```

## Emergency Procedures

### Critical Bug Found After Release

1. Immediately yank release on PyPI
2. Create hotfix branch
3. Fix bug
4. Fast-track release (skip some process steps)
5. Publish hotfix ASAP
6. Announce via all channels

### Build Failure

1. Check GitHub Actions logs
2. Fix build issue
3. Delete and recreate tag
4. Retry

### PyPI Upload Failure

1. Check credentials
2. Verify package builds locally
3. Test upload to TestPyPI first
4. Upload manually if needed

## Checklist Template

Copy this for each release:

```markdown
## Release v0.X.0 Checklist

### Pre-Release
- [ ] All tests pass
- [ ] CI passes
- [ ] Version updated in all files
- [ ] CHANGELOG updated
- [ ] Documentation updated

### Release
- [ ] Create release branch
- [ ] Update version and changelog
- [ ] Create PR and merge
- [ ] Tag release
- [ ] Verify GitHub Actions
- [ ] Verify PyPI upload

### Post-Release
- [ ] Test installation
- [ ] Update development version
- [ ] Close milestone
- [ ] Announce release
```

## Resources

- [Python Packaging Guide](https://packaging.python.org/)
- [Semantic Versioning](https://semver.org/)
- [Keep a Changelog](https://keepachangelog.com/)
- [GitHub Releases](https://docs.github.com/en/repositories/releasing-projects-on-github)
