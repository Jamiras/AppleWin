#include "StdAfx.h"

#include "frontends/common2/gnuframe.h"
#include "frontends/common2/fileregistry.h"
#include "frontends/common2/programoptions.h"

#include "linux/resources.h"

#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <libgen.h>

#ifdef __APPLE__
#include "mach-o/dyld.h"
#endif

#include "Core.h"
#include "config.h"

#ifdef __MINGW32__
#define realpath(N,R) _fullpath((R),(N),_MAX_PATH)
#endif

namespace
{

  bool dirExists(const std::string & folder)
  {
    struct stat stdbuf;

    if (stat(folder.c_str(), &stdbuf) == 0 && S_ISDIR(stdbuf.st_mode))
    {
      return true;
    }
    else
    {
      return false;
    }
  }

  std::string getResourceFolder(const std::string & target)
  {
    std::vector<std::string> paths;

    char self[1024] = {0};

#ifdef _WIN32
    int ch = GetModuleFileNameA(0, self, sizeof(self));
    if (ch >= sizeof(self))
        ch = -1;
#elif defined(__APPLE__)
    uint32_t size = sizeof(self);
    const int ch = _NSGetExecutablePath(self, &size);
#else
    const int ch = readlink("/proc/self/exe", self,  sizeof(self));
#endif

    if (ch != -1)
    {
      const char * path = dirname(self);

      // case 1: run from the build folder
      paths.emplace_back(std::string(path) + '/'+ ROOT_PATH);
      // case 2: run from the installation folder
      paths.emplace_back(std::string(path) + '/'+ SHARE_PATH);
    }

    // case 3: use the source folder
    paths.emplace_back(CMAKE_SOURCE_DIR);

    for (const std::string & path : paths)
    {
      char * real = realpath(path.c_str(), nullptr);
      if (real)
      {
        const std::string resourcePath = std::string(real) + target;
        free(real);
        if (dirExists(resourcePath))
        {
          return resourcePath;
        }
      }
    }

    throw std::runtime_error("Cannot found the resource path: " + target);
  }

}

namespace common2
{
  GNUFrame::GNUFrame(const EmulatorOptions & options)
  : CommonFrame(options.autoBoot, options.fixedSpeed, options.syncWithTimer, !options.noVideoUpdate)
  , myHomeDir(GetHomeDir())
  , myResourceFolder(getResourceFolder("/resource/"))
  {
    // should this go down to LinuxFrame (maybe Initialisation?)
    g_sProgramDir = getResourceFolder("/bin/");
  }

  
  BYTE* GNUFrame::GetResource(WORD id, LPCSTR lpType, DWORD expectedSize)
  {
    myResource.clear();

    const std::string & filename = getResourceName(id);
    const std::string path = getResourcePath(filename);

    const int fd = open(path.c_str(), O_RDONLY);

    if (fd != -1)
    {
      struct stat stdbuf;
      if ((fstat(fd, &stdbuf) == 0) && S_ISREG(stdbuf.st_mode))
      {
        const off_t size = stdbuf.st_size;
        std::vector<BYTE> data(size);
        const ssize_t rd = read(fd, data.data(), size);
        if (rd == expectedSize)
        {
          std::swap(myResource, data);
        }
      }
      close(fd);
    }

    if (myResource.empty())
    {
      LogFileOutput("FindResource: could not load resource %s\n", filename.c_str());
    }

    return myResource.data();
  }

  std::string GNUFrame::getResourcePath(const std::string & filename)
  {
    return myResourceFolder + filename;
  }

  std::string GNUFrame::Video_GetScreenShotFolder() const
  {
    return myHomeDir + "/Pictures/";
  }

}
