/*
 * Copyright (C) 2015 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "Path.h"

#include <sstream>
#include <ostream>
#include <iterator>

#include <dirent.h>
#include <limits.h>
#include <unistd.h>
#include <sys/stat.h>

#if __APPLE__
#include <mach-o/dyld.h>
#endif

namespace utils {

Path::Path(const char* path)
    : Path(std::string(path)) {
}

Path::Path(const std::string& path)
    : m_path(getCanonicalPath(path)) {
}

bool Path::exists() const {
    struct stat file;
    return stat(c_str(), &file) == 0;
}

bool Path::isFile() const {
    struct stat file;
    if (stat(c_str(), &file) == 0) {
        return S_ISREG(file.st_mode);
    }
    return false;
}

bool Path::isDirectory() const {
    struct stat file;
    if (stat(c_str(), &file) == 0) {
        return S_ISDIR(file.st_mode);
    }
    return false;
}

Path Path::concat(const Path& path) const {
    if (path.isEmpty()) return *this;
    if (path.isAbsolute()) return path;
    if (m_path.back() != '/') return Path(m_path + '/' + path.getPath());
    return Path(m_path + path.getPath());
}

void Path::concatToSelf(const Path& path)  {
    if (!path.isEmpty()) {
        if (path.isAbsolute()) {
            m_path = path.getPath();
        } else if (m_path.back() != '/') {
            m_path = getCanonicalPath(m_path + '/' + path.getPath());
        } else {
            m_path = getCanonicalPath(m_path + path.getPath());
        }
    }
}

Path Path::concat(const std::string& root, const std::string& leaf) {
    return Path(root).concat(Path(leaf));
}

Path Path::getCurrentDirectory() {
    char directory[PATH_MAX + 1];
    if (getcwd(directory, PATH_MAX) == nullptr) {
        return Path();
    }
    return Path(directory);
}

Path Path::getCurrentExecutable() {
    // First, need to establish resource path.
    char exec_buf[2048];
    Path result;

#if __APPLE__
    uint32_t buffer_size = sizeof(exec_buf);
    if (_NSGetExecutablePath(exec_buf, &buffer_size) == 0) {
        result.setPath(exec_buf);
    }
#else
    uint32_t buffer_size = sizeof(exec_buf)-1;
    ssize_t sz = readlink("/proc/self/exe", exec_buf, buffer_size);
    if (sz > 0) {
        exec_buf[sz] = 0;
        result.setPath(exec_buf);
    }
#endif

    return result;
}

Path Path::getAbsolutePath() const {
    if (isEmpty() || isAbsolute()) {
        return *this;
    }
    return getCurrentDirectory().concat(*this);
}

Path Path::getParent() const {
    if (isEmpty()) return "";

    std::string result;

    // if our path starts with '/', keep the '/'
    if (m_path.front() == '/') {
        result.append("/");
    }

    std::vector<std::string> segments(split());
    segments.pop_back(); // peel the last one
    for (auto const& s : segments) {
        result.append(s).append("/");
    }
    return getCanonicalPath(result);
}

std::string Path::getName() const {
    if (isEmpty()) return "";

    std::vector<std::string> segments(split());
    return segments.back();
}

std::string Path::getExtension() const {
    if (isEmpty() || isDirectory()) {
        return "";
    }

    auto name = getName();
    auto index = name.rfind(".");
    if (index != std::string::npos && index != 0) {
        return name.substr(index + 1);
    } else {
        return "";
    }
}

std::string Path::getNameWithoutExtension() const {
    std::string name = getName();
    size_t index = name.rfind(".");
    if (index != std::string::npos) {
        return name.substr(0, index);
    }
    return name;
}

std::ostream& operator<<(std::ostream& os, const Path& path) {
    os << path.getPath();
    return os;
}

std::vector<std::string> Path::split() const {
    std::vector<std::string> segments;
    if (isEmpty()) return segments;

    if (m_path.front() == '/') segments.push_back("/");

    size_t current;
    ssize_t next = -1;

    do {
      current = size_t(next + 1);
      next = m_path.find_first_of("/", current);

      std::string segment(m_path.substr(current, next - current));
      if (!segment.empty()) segments.push_back(segment);
    } while (next != std::string::npos);

    if (segments.size() == 0) segments.push_back(m_path);

    return segments;
}

std::string Path::getCanonicalPath(const std::string& path) {
    if (path.empty()) return "";

    std::vector<std::string> segments;

    // If the path starts with a / we must preserve it
    bool starts_with_slash = path.front() == '/';
    // If the path does not end with a / we need to remove the
    // extra / added by the join process
    bool ends_with_slash = path.back() == '/';

    size_t current;
    ssize_t next = -1;

    do {
      current = size_t(next + 1);
      next = path.find_first_of("/", current);

      std::string segment(path.substr(current, next - current));
      size_t size = segment.length();

      // skip empty (keep initial)
      if (size == 0 && segments.size() > 0) {
          continue;
      }

      // skip . (keep initial)
      if (segment == "." && segments.size() > 0) {
          continue;
      }

      // remove ..
      if (segment == ".." && segments.size() > 0) {
          if (segments.back().empty()) { // ignore if .. follows initial /
              continue;
          }
          if (segments.back() != "..") {
              segments.pop_back();
              continue;
          }
      }

      segments.push_back(segment);
    } while (next != std::string::npos);

    // Join the vector as a single string, every element is
    // separated by a '/'. This process adds an extra / at
    // the end that might need to be removed
    std::stringstream clean_path;
    std::copy(segments.begin(), segments.end(),
            std::ostream_iterator<std::string>(clean_path, "/"));
    std::string new_path = clean_path.str();

    if (starts_with_slash && new_path.empty()) {
        new_path = "/";
    }

    if (!ends_with_slash && new_path.length() > 1) {
        new_path.pop_back();
    }

    return new_path;
}

bool Path::mkdir() const {
    return ::mkdir(m_path.c_str(), S_IRUSR | S_IWUSR | S_IXUSR) == 0;
}

bool Path::mkdirRecursive() const {
    if (isEmpty()) {
        return true;
    }
    errno = 0;
    bool success = mkdir();
    if (!success) {
        int saveErrno = errno;
        switch (saveErrno) {
        case EEXIST: {
            bool result = isDirectory();
            errno = saveErrno;
            return result;
        }
        case ENOENT:
            getParent().mkdirRecursive();
            return mkdir();
        default:
            break;
        }
    }
    return success;
}

std::vector<Path> Path::listContents() const {
    // Return an empty vector if the path doesn't exist or is not a directory
    if (!isDirectory() || !exists()) {
        return {};
    }

    struct dirent* directory;
    DIR* dir;

    dir = opendir(c_str());
    if (dir == nullptr) {
        // Path does not exist or could not be read
        return {};
    }

    std::vector<Path> directory_contents;

    while ((directory = readdir(dir)) != nullptr) {
        const char* file = directory->d_name;
        if (file[0] != '.') {
            directory_contents.push_back(concat(directory->d_name));
        }
    }

    closedir(dir);
    return directory_contents;
}

bool Path::unlinkFile() {
    return ::unlink(m_path.c_str()) == 0;
}

} // namespace utils
