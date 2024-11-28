#pragma once

#include "frontends/common2/commonframe.h"
#include <string>

namespace common2
{

  class GNUFrame : public CommonFrame
  {
  public:
    GNUFrame(const common2::EmulatorOptions & option);

    std::string Video_GetScreenShotFolder() const override;
    BYTE* GetResource(WORD id, LPCSTR lpType, DWORD expectedSize) override;
    
  protected:
    virtual std::string getResourcePath(const std::string& filename);

  private:
    const std::string myHomeDir;
    const std::string myResourceFolder;

    std::vector<BYTE> myResource;
  };

}
