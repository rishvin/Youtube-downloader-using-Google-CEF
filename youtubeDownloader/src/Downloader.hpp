#ifndef _CURL_HPP_
#define _CURL_HPP_

#include <iostream>
#include "curl/curl.h"

namespace YTD {

  typedef size_t (*pFnWr)(void *ptr, size_t size, size_t nmemb, void *stream);
  typedef void (*FnUpdate)(std::pair<void*, size_t> pair);
  class Downloader {
      public:
        Downloader(pFnWr pWriter, void *pwriter, void *pKey = NULL, 
            FnUpdate pfnUpdate = NULL, int *pInterrupt = NULL);
        virtual ~Downloader();                    
        int Start(const char *pUrl);
        double DownloadedBytes();
        static int Progress(void *p, curl_off_t dltotal, curl_off_t dlnow, 
            curl_off_t ultotal, curl_off_t ulnow);
      private:
        CURL *pCurl;
        void *pKey;
        FnUpdate pfnUpdate;
        int *pInterrupt;
  };
}

#endif
