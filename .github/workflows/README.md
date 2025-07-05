# GitHub Workflows for lsfg-vk

This directory contains GitHub Actions workflows for continuous integration, security scanning, and quality checks for the lsfg-vk project.

## Workflows Overview

### 1. Build Workflow (`build.yml`)

**Triggers:**
- Push to main/master/develop branches
- Pull requests to main/master/develop branches
- Manual workflow dispatch

**Jobs:**
- **build-linux**: Builds the project on Ubuntu latest with both Release and Debug configurations
- **build-ubuntu-versions**: Tests compatibility across Ubuntu 20.04 and 22.04
- **static-analysis**: Runs clang-tidy for static code analysis
- **release**: Creates release artifacts when tags are pushed

**Key Features:**
- Installs all required dependencies (Clang, Vulkan SDK, CMake, Meson, Ninja)
- Builds external dependencies (DXVK, pe-parse)
- Generates compile_commands.json for development tools
- Uploads build artifacts for each configuration
- Validates that required shared libraries are built

### 2. Security Workflow (`security.yml`)

**Triggers:**
- Push to main/master/develop branches
- Pull requests to main/master/develop branches
- Weekly scheduled scans (Mondays at 2 AM UTC)

**Jobs:**
- **codeql**: GitHub's CodeQL static analysis for C++
- **dependency-review**: Reviews dependencies in pull requests
- **trivy-scan**: Vulnerability scanning with Trivy

### 3. Quality Workflow (`quality.yml`)

**Triggers:**
- Push to main/master/develop branches
- Pull requests to main/master/develop branches

**Jobs:**
- **format-check**: Validates code formatting with clang-format
- **documentation-check**: Ensures README completeness and checks for TODO/FIXME
- **build-docs**: Generates API documentation with Doxygen

## Dependencies Installed

The workflows automatically install these system dependencies:

### Core Build Tools
- `build-essential` - GCC toolchain and build utilities
- `clang` - Clang C++ compiler (required by the project)
- `cmake` - Build system generator
- `ninja-build` - Fast build system
- `meson` - Build system (for DXVK)
- `git`, `bash`, `sed` - Version control and scripting tools

### Vulkan Development
- `libvulkan-dev` - Vulkan development headers
- `vulkan-tools` - Vulkan utilities and tools
- `vulkan-validationlayers-dev` - Validation layers for debugging
- `spirv-tools` - SPIR-V tools and utilities
- `libspirv-cross-dev` - SPIR-V cross-compilation library
- `glslang-tools` - GLSL to SPIR-V compiler

### Analysis Tools
- `clang-tidy` - Static analysis tool
- `clang-format` - Code formatting tool
- `doxygen` - Documentation generator
- `graphviz` - Graph visualization (for Doxygen)

## External Dependencies

The project automatically fetches and builds these external dependencies:

### DXVK (v2.6.2)
- DirectX to Vulkan translation layer
- Built with Meson/Ninja
- Provides DXBC compilation support
- Custom build process that creates libdxbc.so

### pe-parse (v2.1.1)
- Portable Executable parser library
- Built with CMake/Ninja
- Used for processing Windows DLL files

## Artifacts

### Build Artifacts
- `lsfg-vk-Release-linux/` - Release build for Ubuntu latest
- `lsfg-vk-Debug-linux/` - Debug build for Ubuntu latest
- `lsfg-vk-ubuntu-20.04-Release/` - Ubuntu 20.04 compatibility build
- `lsfg-vk-ubuntu-22.04-Release/` - Ubuntu 22.04 compatibility build

### Documentation Artifacts
- `documentation/` - Generated API documentation (HTML)

### Release Artifacts
- `lsfg-vk-linux-x64.tar.gz` - Packaged release build (created on tag push)

## Configuration Files

### `.clang-format`
Defines code formatting rules:
- Based on LLVM style
- 4-space indentation
- 120-character line limit
- C++20 standard
- Consistent spacing and alignment rules

### `.github/codeql/codeql-config.yml`
CodeQL analysis configuration:
- Focuses on security and quality queries
- Scans `src/`, `lsfg-vk-gen/src/`, and include directories
- Excludes build artifacts and documentation files

## Environment Variables

The workflows set these environment variables:

```bash
CC=clang          # Use Clang as C compiler
CXX=clang++       # Use Clang++ as C++ compiler
```

## Troubleshooting

### Common Build Issues

1. **Vulkan headers not found**
   - The workflow installs `libvulkan-dev` package
   - Verify package installation in the workflow logs

2. **DXVK build fails**
   - External project builds are cached between runs
   - Clear cache by triggering a fresh workflow run

3. **Clang-tidy failures**
   - The project uses extensive warning flags
   - Failures are non-blocking in the static-analysis job

### Debugging Workflows

1. **Enable verbose output**
   - `CMAKE_VERBOSE_MAKEFILE=ON` is set for detailed build logs

2. **Check artifact contents**
   - Download build artifacts to inspect generated files

3. **Review dependency installation**
   - Each workflow logs package installation steps

## Local Development

To replicate the CI environment locally:

```bash
# Install dependencies (Ubuntu/Debian)
sudo apt-get install build-essential clang cmake ninja-build meson \
                     libvulkan-dev vulkan-tools spirv-tools

# Configure and build
CC=clang CXX=clang++ cmake -B build -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=/path/to/install

cmake --build build
cmake --install build
```

## Security Considerations

- CodeQL scans for security vulnerabilities
- Trivy scans for known CVEs in dependencies
- Dependency review checks for supply chain risks
- Regular scheduled scans detect new vulnerabilities

## Contributing

When contributing to the project:

1. Ensure code follows the clang-format style
2. Run clang-tidy locally before submitting PRs
3. Check that all CI workflows pass
4. Update documentation for significant changes
