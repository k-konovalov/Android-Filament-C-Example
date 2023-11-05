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
arguments "-DFILAMENT_OFFICIAL_REPO_DIR=" + "~/filament"
```
Or use it as subrepo and don't do nothing.
### Setup .a files

Build binaries from [there](https://github.com/google/filament/blob/main/BUILDING.md#easy-android-build)

Rub build.sh.
build.sh android debug

Copy it to .so folder
