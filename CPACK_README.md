# CPack Packaging Guide
SGCT uses CMake's CPack module to generate installers and packages for easy distribution.

## Platform-Specific Notes

### Windows (Visual Studio / Multi-Configuration)

For Windows builds using Visual Studio (multi-configuration generator), special configuration is required because:

1. **Multi-configuration generators** like Visual Studio place files in subdirectories like `lib/RelWithDebInfo` instead of `lib/`
2. **Default configuration mismatch** - CPack defaults to `Release` but SGCT uses `RelWithDebInfo`
3. **Manual install prefix** - Must be set to ensure files are installed to the correct location

#### Correct Workflow for Windows with Visual Studio:

```powershell
# Configure (uses CPack-specific settings in CMakeLists.txt)
cmake --preset windows-msvc

# Build with RelWithDebInfo configuration
cmake --build build/windows-msvc --config RelWithDebInfo

# Generate packages (CPack uses the configured install settings)
cd build/windows-msvc
cpack -G ZIP
cpack -G NSIS
```

### Linux
For Linux builds using Ninja or Unix Makefiles (single-configuration generators), packaging is straightforward:

```bash
# Configure
cmake --preset linux-ninja-release

# Build
cmake --build build/linux-ninja-release

# Generate packages
cd build/linux-ninja-release
cpack -G ZIP
cpack -G TGZ
cpack -G DEB  # Debian/Ubuntu
cpack -G RPM  # Fedora/RHEL
```

## Generated Packages

### Windows
- **ZIP**: Portable package containing all SGCT files
- **NSIS**: GUI installer with Start Menu shortcuts (`winget install NSIS.NSIS`)

### Linux
- **ZIP**: Portable package
- **TGZ**: Compressed tarball
- **DEB**: Debian/Ubuntu package (.deb)
- **RPM**: Fedora/RHEL package (.rpm)

## Prerequisites

### Windows
- CMake 4.0 or newer
- NSIS (Nullsoft Scriptable Install System) - for .exe installers (`winget install NSIS.NSIS`)

### Linux
- CMake 4.0 or newer
- Debian/Ubuntu: `apt-get install dpkg rpm`
- Fedora/RHEL: `dnf install dpkg rpm-build`
- Arch Linux: `pacman -S dpkg rpm-tools`

## Proprietary SDKs
SGCT optionally supports proprietary SDKs that cannot be redistributed:

- **NDI**: NewTek Network Device Interface
- **Scalable**: Scalable Display Technologies

If these features are enabled during build, the resulting package will include the proprietary SDK binaries. Users must ensure they have the appropriate licenses to use and redistribute these components.

To build SGCT with proprietary SDKs:
```bash
cmake -DSGCT_NDI_SUPPORT=ON -DSGCT_SCALABLE_SUPPORT=ON
```

## Verifying Packages
### Windows ZIP
Extract and verify structure:
```powershell
Expand-Archive sgct-4.0.0-Windows.zip -DestinationPath sgct-test
Test-Path sgct-test\bin\sgct_calibrator.exe
```

### Linux DEB/RPM
Install and verify:
```bash
# DEB (Debian/Ubuntu)
sudo dpkg -i sgct-4.0.0-Linux.deb
sudo apt-get install -f  # Fix any missing dependencies

# RPM (Fedora/RHEL)
sudo rpm -ivh sgct-4.0.0-Linux.rpm
sudo dnf install -y sgct  # Or use yum
```
