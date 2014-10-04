#include "Ytd.hpp"
#include "Common.hpp"
#include "TextDownloader.hpp"
#include "BinaryDownloader.hpp"
#include "Html.hpp"

namespace YTD {

  int GetPage(const char *pUrl, std::string &page) {
      TextDownloader downloader;
      std::string url(pUrl);
      UpateToUnsecureUrl(url);
      return downloader.Download(url.c_str(), page);
  }

  int App::GetVideos(const char *pUrl, std::vector<Videos> &urls) {
      std::string page;
      if(ERR_OK != GetPage(pUrl, page)) {
          return ERR_PAGE_NOT_FOUND;
      }
  
      size_t pos, len;
      GetTitle(page, pos, len);
      std::string title(page.c_str() + pos, len);
      CHECK(!len, return ERR_TITLE); // Title not found
      size_t start = GetUrlSection(page);
      while(std::string::npos != start) {
          start = GetUrl(page, start, pos, len);
          CHECK(!len, return ERR_OK);
          std::string url;
          Html html;
          html.DecodeUrl((char *)page.c_str() + pos, len, url);
          CHECK(!url.size(), return ERR_DECODE_URL);     // Unable to decode URL       
          size_t iTag = GetiTag(url);
          if(iTag) {
              urls.push_back(Videos(title, url, iTag, 0));
          }
      }
      return ERR_OK;
  }     

  size_t App::Download(const char *pUrl, const char *pFile, 
      void *pKey, FnUpdate pfnUpdate, int &interrupt) {        
        BinaryDownloader downloader(pKey, pfnUpdate, interrupt);
        return downloader.Download(pUrl, pFile);
        //return ConvertToSize(KB, (size_t)downloader.DownloadedBytes());
  }
}
