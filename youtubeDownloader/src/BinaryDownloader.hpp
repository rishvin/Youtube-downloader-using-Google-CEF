#ifndef _BINARY_DOWNLOADER_HPP_
#define _BINARY_DOWNLOADER_HPP_

#include "Downloader.hpp"
#include <fstream>

namespace YTD {
  class BinaryDownloader: public Downloader {
    public:
      BinaryDownloader(void *pKey, FnUpdate pfnUpdate, int &interrupt);
      ~BinaryDownloader();
      static size_t Write(void *ptr, size_t size, size_t nmemb, void *stream);
      int Download(const char *pUrl, const char *pFile);
      void CleanUp();
    private:
      std::string url;
      static void * Process(void *param);
      std::ofstream writer;
      
};
}
#endif

