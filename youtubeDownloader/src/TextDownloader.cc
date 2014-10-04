#include "TextDownloader.hpp"

namespace YTD {

  TextDownloader::TextDownloader()
      :Downloader(TextDownloader::Write, &writer) {
  }
  
  TextDownloader::~TextDownloader() {
      writer.clear();
  }
  
  size_t TextDownloader::Write(void *ptr, size_t size, size_t nmemb, void *param) {
      std::stringstream *pWriter = (std::stringstream *)param;
      size_t buffSize = size * nmemb;
      std::string partialPage((char *)ptr, buffSize);
      (*pWriter) << partialPage;
      return buffSize;
  }
  
  int TextDownloader::Download(const char *pUrl, std::string &response) {
      int iRet;
      writer.clear();
      response.clear();
      iRet = Start(pUrl);
      if(0 == iRet) {
          response.assign(writer.str());
          CleanUp();
      }
      return iRet;
  }

  void TextDownloader::CleanUp() {
      writer.flush();
  }            
}
