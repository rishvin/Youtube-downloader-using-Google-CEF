#include <Windows.h>
#include <commdlg.h>
#include <sstream>
#include "ytd_inc.h"

namespace YTD {

  CefString ToString(uint32 value) {
      std::stringstream stream;
      stream << value;
      return stream.str();
  }

  YTD_PAIR FormatData(CefString key, uint32 value) {
      YTD_PAIR pair;
      YTD_PAIR_KEY(pair) = key;
      YTD_PAIR_VALUE(pair) = ToString(value);
      return pair;
  }

  YTD_PAIR FormatData(uint32 key, uint32 value) {
      YTD_PAIR pair;
      YTD_PAIR_KEY(pair) = ToString(key);
      YTD_PAIR_VALUE(pair) = ToString(value);
      return pair;
  }
      
  void SetMsg( CefRefPtr<CefDOMDocument> &document, std::string msg) {
      CefRefPtr<CefDOMNode> msgNode = document->GetElementById(MSG);
      ASSERT(msgNode.get());
      CefRefPtr<CefDOMNode> msgText = msgNode->GetFirstChild();
      ASSERT(msgText.get());
      ASSERT(msgText->IsText());
      msgText->SetValue(msg);
  }
  
  void SetList(int index, CefString source, CefRefPtr<CefV8Value> &target) {
      ASSERT(target->IsArray());  
      CefRefPtr<CefV8Value>  val = CefV8Value::CreateString(source);
      target->SetValue(index, val);
  }

  void SetList(int index, uint32 source, CefRefPtr<CefV8Value> &target) {
      ASSERT(target->IsArray());
      CefRefPtr<CefV8Value> val = CefV8Value::CreateUInt(source);
      target->SetValue(index, val);
  }
    
  bool GetFileSaveLocation(CefString &file, YTD_PAIR &fileInfo) {
      
      TCHAR szFile[MAX_PATH];
	  wcscpy_s(szFile, MAX_PATH, YTD_PAIR_VALUE(fileInfo).c_str());
     //MAX_PATH MessageBox(NULL, szFile, szFile, MB_OK);
      OPENFILENAME ofn;
      ZeroMemory(&ofn, sizeof(ofn));
      ofn.lStructSize = sizeof(ofn);
      ofn.lpstrFile = szFile;
      ofn.nMaxFile = MAX_PATH;
      ofn.lpstrFilter = L"All\0*.*\0";
      ofn.lpstrDefExt = (LPWSTR)((YTD_PAIR_KEY(fileInfo)).c_str());
      ofn.nFilterIndex = 1;
      ofn.lpstrInitialDir = NULL;
      ofn.Flags = OFN_PATHMUSTEXIST;
      
      if(GetSaveFileName(&ofn) == TRUE) {
          file = szFile;
          return true;
      }
      //std::stringstream str;
      //str << CommDlgExtendedError();
      //CefString s(str.str());
      //MessageBox(NULL, s.c_str(), s.c_str(), MB_OK);
      return false;
  }

  void ExtractFileName(CefString &innerHtml, YTD_PAIR &fileInfo) {

      YTD_PAIR_KEY(fileInfo).clear();
      YTD_PAIR_VALUE(fileInfo).clear();

      std::string temp(innerHtml.ToString());
      
      // display format is filaname|extension[quality]
      size_t i = temp.rfind("[");
      if(std::string::npos == i) {
            return;
      }

      size_t j = temp.rfind("|", --i);
      if(std::string::npos == j) {
          return;
      }

      size_t len = i - j;
      if(!len) {
          return;
      }
      
      std::string ext(temp, j + 1, len);
      
      len = j;
      std::string name;
      if(len) {
          name.append(temp, 0, len);
      }
      else {
          name.append("Untitled");
      }
      
      YTD_PAIR_KEY(fileInfo) = ext;
      YTD_PAIR_VALUE(fileInfo) = name;
      
  }
}
