#ifndef YTD_RESOURCE_H_
#define YTD_RESOURCE_H_

#include <Windows.h>
#include <map>

#include "ytd_inc.h"

namespace YTD {

#define MT_INIT      0
#define MT_PROGRESS  1
#define MT_DONE      2
#define MT_SUCCESS   0
#define MT_CANCEL    3


  class GC {
    public:
      typedef std::map<ULONGLONG, HANDLE> FreeList;
      typedef std::pair<ULONGLONG, HANDLE> Data;
      typedef std::map<ULONGLONG, HANDLE>::iterator Iterator;

      GC();
      ~GC();
      void InsertToFreeList(HANDLE &thHand);
    private:
      static DWORD WINAPI SpawnGC(LPVOID lpParam);

      FreeList gcList;
      HANDLE mutex;
      HANDLE thHand;
      HANDLE sema;
  };
  
  class MTDownloader {
    public:
      class DownloaderKey {
        public:
          DownloaderKey(MTDownloader *pThis, DWORD thId)
              :pThis(pThis), thId(thId) {
          }
          ~DownloaderKey() {
          }
          MTDownloader *pThis;
          DWORD thId;
      };
      typedef void (*MTNotify)(uint32 state, YTD_PAIR pair);
      class ThNode {
        public:
          YTD_PAIR fileInfo;
          int interrupt;
          HANDLE handle;
      };
  
      typedef std::map<DWORD, ThNode> Storage;
      typedef std::pair<DWORD, ThNode> Data;
      typedef std::map<DWORD, ThNode>::iterator Iterator;
      
	  MTDownloader(MTNotify fnNotify);
	  ~MTDownloader();
      bool Download(CefString &url , CefString &file);
      void CancelDownload(DWORD id);
  
    private:
      static DWORD WINAPI SpawnDownload(LPVOID lpParam);
      void InsertData(DWORD id, ThNode &thNode);
      void DeleteData(DWORD id);
      static void ProgressCB(DOWNLOADER_PAIR pair);
      MTNotify fnNotify;
      Storage mtStorage;
      HANDLE mutex;
  };
}

#endif
