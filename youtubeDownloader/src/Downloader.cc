#include <assert.h>
#include "Downloader.hpp"
#include <Windows.h>
#include <commdlg.h>

namespace YTD {   

  Downloader::Downloader(pFnWr pWriter, void *pwriter, void *pKey, 
      FnUpdate pfnUpdate, int *pInterrupt): 
      pCurl(NULL), pKey(pKey), pfnUpdate(pfnUpdate) {
      pCurl = curl_easy_init();
      assert(pCurl);
      curl_easy_setopt(pCurl, CURLOPT_FOLLOWLOCATION, 1L);
      curl_easy_setopt(pCurl, CURLOPT_VERBOSE, 1);     
      curl_easy_setopt(pCurl, CURLOPT_ACCEPT_ENCODING, "deflate");
      curl_easy_setopt(pCurl, CURLOPT_WRITEFUNCTION, pWriter);
      curl_easy_setopt(pCurl, CURLOPT_WRITEDATA, pwriter);
      if(NULL != pfnUpdate) {
          curl_easy_setopt(pCurl, CURLOPT_NOPROGRESS, 0L);
          curl_easy_setopt(pCurl, CURLOPT_XFERINFODATA, this);
          curl_easy_setopt(pCurl, CURLOPT_XFERINFOFUNCTION, Progress);
      }
      if(NULL != pInterrupt) {
          this->pInterrupt = pInterrupt;
      }
  }

  Downloader::~Downloader() {
      curl_easy_cleanup(pCurl);
  }
  
  int Downloader::Start(const char *pUrl) {
      curl_easy_setopt(pCurl, CURLOPT_URL, pUrl);
      CURLcode retcode = curl_easy_perform(pCurl);
      return (int)retcode;
  }

  double Downloader::DownloadedBytes() {
      double downloaded;
      curl_easy_getinfo(pCurl, CURLINFO_SIZE_DOWNLOAD, &downloaded);
      return downloaded;
  }

  int Downloader::Progress(void *p, curl_off_t dltotal, curl_off_t dlnow,
      curl_off_t ultotal, curl_off_t ulnow) {
      Downloader *pThis = (Downloader*)(p);
      pThis->pfnUpdate(std::pair<void*, size_t>(pThis->pKey, dlnow));
      if(pThis->pInterrupt)
        return *(pThis->pInterrupt);
      return 0;
  }
}

