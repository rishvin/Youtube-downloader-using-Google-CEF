#include "BinaryDownloader.hpp"

namespace YTD {

  BinaryDownloader::BinaryDownloader(void *pKey, FnUpdate pfnUpdate, int &interrupt)
      :Downloader(BinaryDownloader::Write, &writer, pKey, pfnUpdate, &interrupt) {
  }

  BinaryDownloader::~BinaryDownloader() {
      writer.close();
  }

  size_t BinaryDownloader::Write(void *ptr, size_t size, size_t nmemb, void *param) {
      std::ofstream *pWriter = (std::ofstream *)param;
      size_t buffSize = size * nmemb;
      pWriter->write((char*)ptr, buffSize);
      return buffSize;
  }
  
  int BinaryDownloader::Download(const char *pUrl, const char *pFile) {
      int iRet;
      writer.clear();
      writer.open(pFile, std::ofstream::out | std::ofstream::binary);
		if(!writer.is_open()) {
          iRet = -1;
      }
      else {
          iRet = Start(pUrl);
          CleanUp();
      }
      return iRet;
  }
  
  void BinaryDownloader::CleanUp() {
      writer.close();
  }
           
}
