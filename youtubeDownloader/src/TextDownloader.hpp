#ifndef _TEXT_DOWNLOADER_HPP_
#define _TEXT_DOWNLOADER_HPP_

#include "Downloader.hpp"
#include <sstream>

namespace YTD {
  class TextDownloader: public Downloader {
    public:
      TextDownloader();
      ~TextDownloader();
      static size_t Write(void *ptr, size_t size, size_t nmemb, void *stream);
      int Download(const char *pUrl, std::string &response);
      void CleanUp();
     private:
       std::stringstream writer;
  };
}
#endif
