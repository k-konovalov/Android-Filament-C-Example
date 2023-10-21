# Android Filament C++ Example
Alternative for official Java use - almost pure C++ Example

## Setup Environment
### Versions
CMake 3.22.1
NDK 25.1.8937393

### Filament SDK
- ~Download to local folder~
- Download as Git submodule (current)

Before use, change path to filament framework dir from [repo](https://github.com/google/filament) in app level build.gradle:
```kotlin
arguments "-DFILAMENT_OFFICIAL_REPO_DIR=" + "/Users/konovalovkirill/Documents/FrameWorks/filament"
```
