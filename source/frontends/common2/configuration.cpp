#include "frontends/common2/configuration.h"
#include <frontends/common2/programoptions.h>

#include "Log.h"
#include "linux/windows/files.h"
#include "frontends/qt/applicationname.h"

#include <boost/property_tree/ini_parser.hpp>
#include <boost/algorithm/string/replace.hpp>

#include <sys/stat.h>

namespace
{

  void parseOption(const std::string & s, std::string & path, std::string & value)
  {
    const size_t pos = s.find('=');
    if (pos == std::string::npos)
    {
      throw std::runtime_error("Invalid option format: " + s + ", expected: section.key=value");
    }
    path = s.substr(0, pos);
    std::replace(path.begin(), path.end(), '_', ' ');
    value = s.substr(pos + 1);
    std::replace(value.begin(), value.end(), '_', ' ');
  }

  struct KeyEncodedLess
  {
    static std::string decodeKey(const std::string & key)
    {
      std::string result = key;
      // quick implementation, just to make it work.
      // is there a library function available somewhere?
      boost::algorithm::replace_all(result, "%20", " ");
      return result;
    }

    bool operator()( const std::string & lhs, const std::string & rhs ) const
    {
      const std::string key1 = decodeKey(lhs);
      const std::string key2 = decodeKey(rhs);
      return key1 < key2;
    }
  };

  class Configuration
  {
  public:
    typedef boost::property_tree::basic_ptree<std::string, std::string, KeyEncodedLess> ini_t;

    Configuration(const std::string & filename, const bool saveOnExit);
    ~Configuration();

    static std::shared_ptr<Configuration> instance;

    template<typename T>
    T getValue(const std::string & section, const std::string & key) const;

    template<typename T>
    void putValue(const std::string & section, const std::string & key, const T & value);

    const ini_t & getProperties() const;

    void addExtraOptions(const std::vector<std::string> & options);

  private:
    const std::string myFilename;
    const bool mySaveOnExit;

    static std::string decodeKey(const std::string & key);

    ini_t myINI;
  };

  std::shared_ptr<Configuration> Configuration::instance;

  Configuration::Configuration(const std::string & filename, const bool saveOnExit) : myFilename(filename), mySaveOnExit(saveOnExit)
  {
    if (GetFileAttributes(myFilename.c_str()) != INVALID_FILE_ATTRIBUTES)
    {
      boost::property_tree::ini_parser::read_ini(myFilename, myINI);
    }
    else
    {
      LogFileOutput("Registry: configuration file '%s' not found\n", filename.c_str());
    }
  }

  Configuration::~Configuration()
  {
    if (mySaveOnExit)
    {
      boost::property_tree::ini_parser::write_ini(myFilename, myINI);
    }
  }

  const Configuration::ini_t & Configuration::getProperties() const
  {
    return myINI;
  }

  template <typename T>
  T Configuration::getValue(const std::string & section, const std::string & key) const
  {
    const std::string path = section + "." + key;
    const T value = myINI.get<T>(path);
    return value;
  }

  template <typename T>
  void Configuration::putValue(const std::string & section, const std::string & key, const T & value)
  {
    const std::string path = section + "." + key;
    myINI.put(path, value);
  }

  void Configuration::addExtraOptions(const std::vector<std::string> & options)
  {
    for (const std::string & option : options)
    {
      std::string path, value;
      parseOption(option, path, value);
      myINI.put(path, value);
    }
  }
}

void InitializeRegistry(const EmulatorOptions & options)
{
  const char* homeDir = getenv("HOME");
  if (!homeDir)
  {
    throw std::runtime_error("${HOME} not set, cannot locate configuration file");
  }

  std::string filename;
  bool saveOnExit;

  if (options.useQtIni)
  {
    filename = std::string(homeDir) + "/.config/" + ORGANIZATION_NAME + "/" + APPLICATION_NAME + ".conf";
    saveOnExit = false;
  }
  else
  {
    const std::string dir = std::string(homeDir) + "/.applewin";
    const int status = mkdir(dir.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    filename = dir + "/applewin.conf";
    if (!status || (errno == EEXIST))
    {
      saveOnExit = options.saveConfigurationOnExit;
    }
    else
    {
      const char * s = strerror(errno);
      LogFileOutput("No registry. Cannot create %s: %s\n", dir.c_str(), s);
      saveOnExit = false;
    }
  }

  std::shared_ptr<Configuration> config(new Configuration(filename, saveOnExit));
  config->addExtraOptions(options.registryOptions);
  std::swap(Configuration::instance, config);
}

BOOL RegLoadString (LPCTSTR section, LPCTSTR key, BOOL peruser,
                    LPTSTR buffer, DWORD chars)
{
  BOOL result;
  try
  {
    const std::string s = Configuration::instance->getValue<std::string>(section, key);
    strncpy(buffer, s.c_str(), chars);
    buffer[chars - 1] = 0;
    result = TRUE;
    LogFileOutput("RegLoadString: %s - %s = %s\n", section, key, buffer);
  }
  catch (const std::exception & e)
  {
    result = FALSE;
    LogFileOutput("RegLoadString: %s - %s = ??\n", section, key);
  }
  return result;
}

BOOL RegLoadValue (LPCTSTR section, LPCTSTR key, BOOL peruser, DWORD *value)
{
  BOOL result;
  try
  {
    *value = Configuration::instance->getValue<DWORD>(section, key);
    result = TRUE;
    LogFileOutput("RegLoadValue: %s - %s = %d\n", section, key, *value);
  }
  catch (const std::exception & e)
  {
    result = FALSE;
    LogFileOutput("RegLoadValue: %s - %s = ??\n", section, key);
  }
  return result;
}

BOOL RegLoadValue (LPCTSTR section, LPCTSTR key, BOOL peruser, BOOL *value)
{
  BOOL result;
  try
  {
    *value = Configuration::instance->getValue<BOOL>(section, key);
    result = TRUE;
    LogFileOutput("RegLoadValue: %s - %s = %d\n", section, key, *value);
  }
  catch (const std::exception & e)
  {
    result = FALSE;
    LogFileOutput("RegLoadValue: %s - %s = ??\n", section, key);
  }
  return result;
}

void RegSaveString (LPCTSTR section, LPCTSTR key, BOOL peruser, const std::string & buffer)
{
  Configuration::instance->putValue(section, key, buffer);
  LogFileOutput("RegSaveString: %s - %s = %s\n", section, key, buffer.c_str());
}

void RegSaveValue (LPCTSTR section, LPCTSTR key, BOOL peruser, DWORD value)
{
  Configuration::instance->putValue(section, key, value);
  LogFileOutput("RegSaveValue: %s - %s = %d\n", section, key, value);
}