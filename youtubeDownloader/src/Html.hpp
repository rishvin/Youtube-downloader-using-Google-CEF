#ifndef _HTML_HPP_
#define _HTML_HPP_

#include <string>
#include "curl/curl.h"

namespace YTD {
  class Html {
    public:
      Html();
      virtual ~Html();
      void DecodeUrl(const char *pUrl, int len, std::string &outPage);          
    private:
      CURL *pCurl;
    };
}

#endif

