#include <assert.h>
#include <strsafe.h>
#include "ytd_resource.h"

namespace YTD {

  #define GC_CHECK_EMPTY		0
  #define GC_CHECK_TIMESTAMP	1
  #define GC_REACQUIRE_LOCK		2
  #define GC_CLEANUP			3


  GC gGarbageCollector;

  GC::GC() {
    
    mutex = CreateMutex(NULL, FALSE, NULL);
    ASSERT(mutex);

    sema = CreateSemaphore(NULL, 0, 1, NULL);
    ASSERT(sema);

    thHand = CreateThread(NULL, 0, SpawnGC, this, 0, NULL);
    ASSERT(thHand);
  }

  GC::~GC() {
    CloseHandle(thHand);
  }

  void GC::InsertToFreeList(HANDLE &thHand) {
      DWORD retcode = WaitForSingleObject(mutex,INFINITE);
      if(WAIT_OBJECT_0 == retcode) {
          gcList.insert(Data(GetTickCount64(), thHand));
          ReleaseMutex(mutex);
          ReleaseSemaphore(sema, 1, NULL);
      }
  }  

  DWORD WINAPI GC::SpawnGC(LPVOID lpParam) {
      GC *pThis = (GC*)lpParam;
      FreeList &rList = pThis->gcList;
      HANDLE &rMutex = pThis->mutex;
      HANDLE &rSema = pThis->sema;
      Iterator lastItr = rList.end();
      int state = GC_CHECK_EMPTY;
      while(1) {
          switch(state) {
            case GC_CHECK_EMPTY: {
                
              if(WAIT_OBJECT_0 != WaitForSingleObject(rMutex,INFINITE))
                  break;
              
              if(rList.empty()) {
                  ReleaseMutex(rMutex);
                  WaitForSingleObject(rSema, INFINITE);
                  break;
              }
            }
            // fall through
              
            case GC_CHECK_TIMESTAMP: {
              lastItr = rList.begin();
              if((GetTickCount64() - lastItr->first) < 1000) {
                  ReleaseMutex(rMutex);
                  Sleep(1000);
                  state = GC_REACQUIRE_LOCK;
                  break;
              }
            }
            // fall through
              
            case GC_CLEANUP: {
              HANDLE thHand = lastItr->second; 
              rList.erase(lastItr);
              ReleaseMutex(rMutex);
              CloseHandle(thHand);
              state = GC_CHECK_EMPTY;
              continue;             
            }

            case GC_REACQUIRE_LOCK: {
              if(WAIT_OBJECT_0 == WaitForSingleObject(rMutex,INFINITE))
                  state = GC_CLEANUP;
              }
          }   
      }
	  return 0;
  }  

  MTDownloader::MTDownloader(MTNotify fnNotify)
    :fnNotify(fnNotify) {
      ASSERT(fnNotify);
      mutex = CreateMutex(NULL, FALSE, NULL);
      ASSERT(mutex);
  }

  MTDownloader::~MTDownloader() {
      CloseHandle(mutex);
  }
  
  bool MTDownloader::Download(CefString &url , CefString &file) {

      ThNode thNode;          
      YTD_PAIR &fileInfo = thNode.fileInfo;
      
      YTD_PAIR_KEY(fileInfo) = url;
      YTD_PAIR_VALUE(fileInfo) = file;

      thNode.interrupt = 0;

      DWORD thId;
      
      thNode.handle = CreateThread(NULL, 0, SpawnDownload,
          this, 0, &thId);
      if(NULL == thNode.handle) {
          return false;
      }
      
      InsertData(thId, thNode);
      return true;
  }
  
  DWORD WINAPI MTDownloader::SpawnDownload(LPVOID lpParam) {
    
      MTDownloader *pThis = (MTDownloader*)lpParam;
      Storage &rStorage = pThis->mtStorage;
      HANDLE &rMutex = pThis->mutex;

      DWORD id = GetCurrentThreadId();
      
      Iterator itr;
      while((itr = rStorage.find(id)) == rStorage.end()) {
          // Parent function might not have inserted
          // the data to the map.
      }
      
      
      ThNode &thNode = itr->second;
      YTD_PAIR &fileInfo = thNode.fileInfo;
      
      CefString &url = YTD_PAIR_KEY(fileInfo);
      CefString &file = YTD_PAIR_VALUE(fileInfo);

      int &interrupt = thNode.interrupt;
      
      pThis->fnNotify(MT_INIT, FormatData(file.ToString(), id));
      DownloaderKey *pKey = new DownloaderKey(pThis, id);
      uint32 retcode = App::Download(url.ToString().c_str(), 
          file.ToString().c_str(), pKey, ProgressCB, interrupt);

      if(MT_CANCEL == interrupt)
          pThis->fnNotify(MT_CANCEL, FormatData(id, retcode));
      else 
          pThis->fnNotify(MT_DONE, FormatData(id, retcode));
      
      delete pKey;
      pThis->DeleteData(id);
      return 0;
  }

  void MTDownloader::InsertData(DWORD id, ThNode &thNode) {
      DWORD retcode = WaitForSingleObject(mutex,INFINITE);
      if(WAIT_OBJECT_0 == retcode) {
          mtStorage.insert(Data(id, thNode));
          ReleaseMutex(mutex);;
      }
  }

  void MTDownloader::DeleteData(DWORD id) {
    DWORD retcode = WaitForSingleObject(mutex,INFINITE);
      if(WAIT_OBJECT_0 == retcode) {
          Iterator itr = mtStorage.find(id);
          ASSERT(itr != mtStorage.end());
          ReleaseMutex(mutex);
          HANDLE thHand = (itr->second).handle;
          mtStorage.erase(itr);
          // Let GC clean the thread
          gGarbageCollector.InsertToFreeList(thHand);
          
      }
  }

  void MTDownloader::ProgressCB(DOWNLOADER_PAIR pair) {
      DownloaderKey *pKey = (DownloaderKey*)DOWNLOADER_PAIR_KEY(pair);
      size_t bytes = DOWNLOADER_PAIR_VALUE(pair);
      DWORD &id = pKey->thId;
      MTDownloader *pThis = pKey->pThis;
      pThis->fnNotify(MT_PROGRESS, FormatData(id, ConvertToSize(KB, bytes)));
    }

  void MTDownloader::CancelDownload(DWORD id) {
      DWORD retcode = WaitForSingleObject(mutex,INFINITE);
      if(WAIT_OBJECT_0 == retcode) {
          Iterator itr = mtStorage.find(id);
          if(itr == mtStorage.end()) {
              ReleaseMutex(mutex);
          }
          ThNode &thNode = itr->second;
          ReleaseMutex(mutex);
          int &interrupt = thNode.interrupt;

          interrupt = MT_CANCEL;
          
      }
  }
}

