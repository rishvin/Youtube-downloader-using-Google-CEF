// Copyright (c) 2013 The Chromium Embedded Framework Authors. All rights
// reserved. Use of this source code is governed by a BSD-style license that
// can be found in the LICENSE file.

#include "cefclient/client_handler.h"
#include <stdio.h>
#include <algorithm>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#include "include/cef_browser.h"
#include "include/cef_frame.h"
#include "include/cef_path_util.h"
#include "include/cef_process_util.h"
#include "include/cef_runnable.h"
#include "include/cef_trace.h"
#include "include/cef_url.h"
#include "include/wrapper/cef_stream_resource_handler.h"
#include "cefclient/cefclient.h"
#include "cefclient/client_renderer.h"
#include "cefclient/client_switches.h"
#include "cefclient/ytd_app.h"
#include "cefclient/resource_util.h"
#include "cefclient/string_util.h"

namespace {

// Custom menu command Ids.
enum client_menu_ids {
  CLIENT_ID_SHOW_DEVTOOLS   = MENU_ID_USER_FIRST,
  CLIENT_ID_TESTMENU_SUBMENU,
  CLIENT_ID_TESTMENU_CHECKITEM,
  CLIENT_ID_TESTMENU_RADIOITEM1,
  CLIENT_ID_TESTMENU_RADIOITEM2,
  CLIENT_ID_TESTMENU_RADIOITEM3,
};

const char kTestOrigin[] = "http://tests/";

// Retrieve the file name and mime type based on the specified url.
bool ParseTestUrl(const std::string& url,
                  std::string* file_name,
                  std::string* mime_type) {
  // Retrieve the path component.
  CefURLParts parts;
  CefParseURL(url, parts);
  std::string file = CefString(&parts.path);
  if (file.size() < 2)
    return false;

  // Remove the leading slash.
  file = file.substr(1);

  // Verify that the file name is valid.
  for(size_t i = 0; i < file.size(); ++i) {
    const char c = file[i];
    if (!isalpha(c) && !isdigit(c) && c != '_' && c != '.')
      return false;
  }

  // Determine the mime type based on the file extension, if any.
  size_t pos = file.rfind(".");
  if (pos != std::string::npos) {
    std::string ext = file.substr(pos + 1);
    if (ext == "html")
      *mime_type = "text/html";
    else if (ext == "png")
      *mime_type = "image/png";
    else
      return false;
  } else {
    // Default to an html extension if none is specified.
    *mime_type = "text/html";
    file += ".html";
  }

  *file_name = file;
  return true;
}

}  // namespace

int ClientHandler::m_BrowserCount = 0;

ClientHandler::ClientHandler()
  : m_MainHwnd(NULL),
    m_BrowserId(0),
    m_bIsClosing(false),
    m_bFocusOnEditableField(false) {
  CreateProcessMessageDelegates(process_message_delegates_);

  // Read command line settings.
  CefRefPtr<CefCommandLine> command_line =
      CefCommandLine::GetGlobalCommandLine();

  if (command_line->HasSwitch(cefclient::kUrl))
    m_StartupURL = command_line->GetSwitchValue(cefclient::kUrl);
  if (m_StartupURL.empty())
    m_StartupURL = YTD::kTestUrl;

  // Also use external dev tools if off-screen rendering is enabled since we
  // disallow popup windows.
  m_bExternalDevTools =
      command_line->HasSwitch(cefclient::kExternalDevTools) ||
      AppIsOffScreenRenderingEnabled();

  m_bMouseCursorChangeDisabled =
      command_line->HasSwitch(cefclient::kMouseCursorChangeDisabled);
}

ClientHandler::~ClientHandler() {
}

bool ClientHandler::OnProcessMessageReceived(
    CefRefPtr<CefBrowser> browser,
    CefProcessId source_process,
    CefRefPtr<CefProcessMessage> message) {
  // Check for messages from the client renderer.
  std::string message_name = message->GetName();
  if (message_name == client_renderer::kFocusedNodeChangedMessage) {
    // A message is sent from ClientRenderDelegate to tell us whether the
    // currently focused DOM node is editable. Use of |m_bFocusOnEditableField|
    // is redundant with CefKeyEvent.focus_on_editable_field in OnPreKeyEvent
    // but is useful for demonstration purposes.
    m_bFocusOnEditableField = message->GetArgumentList()->GetBool(0);
    return true;
  }

  bool handled = false;

  // Execute delegate callbacks.
  ProcessMessageDelegateSet::iterator it = process_message_delegates_.begin();
  for (; it != process_message_delegates_.end() && !handled; ++it) {
    handled = (*it)->OnProcessMessageReceived(this, browser, source_process,
                                              message);
  }

  return handled;
}

void ClientHandler::OnLoadingStateChange(CefRefPtr<CefBrowser> browser,
                                         bool isLoading,
                                         bool canGoBack,
                                         bool canGoForward) {
  REQUIRE_UI_THREAD();
  SetLoading(isLoading);
  SetNavState(canGoBack, canGoForward);
}

bool ClientHandler::OnConsoleMessage(CefRefPtr<CefBrowser> browser,
                                     const CefString& message,
                                     const CefString& source,
                                     int line) {
  REQUIRE_UI_THREAD();

  bool first_message;
  std::string logFile;

  {
    AutoLock lock_scope(this);

    first_message = m_LogFile.empty();
    if (first_message) {
      std::stringstream ss;
      ss << AppGetWorkingDirectory();
#if defined(OS_WIN)
      ss << "\\";
#else
      ss << "/";
#endif
      ss << "console.log";
      m_LogFile = ss.str();
    }
    logFile = m_LogFile;
  }

  FILE* file = fopen(logFile.c_str(), "a");
  if (file) {
    std::stringstream ss;
    ss << "Message: " << std::string(message) << "\r\nSource: " <<
        std::string(source) << "\r\nLine: " << line <<
        "\r\n-----------------------\r\n";
    fputs(ss.str().c_str(), file);
    fclose(file);

    if (first_message)
      SendNotification(NOTIFY_CONSOLE_MESSAGE);
  }

  return false;
}

bool ClientHandler::OnPreKeyEvent(CefRefPtr<CefBrowser> browser,
                                  const CefKeyEvent& event,
                                  CefEventHandle os_event,
                                  bool* is_keyboard_shortcut) {
  if (!event.focus_on_editable_field && event.windows_key_code == 0x20) {
    // Special handling for the space character when an input element does not
    // have focus. Handling the event in OnPreKeyEvent() keeps the event from
    // being processed in the renderer. If we instead handled the event in the
    // OnKeyEvent() method the space key would cause the window to scroll in
    // addition to showing the alert box.
    if (event.type == KEYEVENT_RAWKEYDOWN) {
      browser->GetMainFrame()->ExecuteJavaScript(
          "alert('You pressed the space bar!');", "", 0);
    }
    return true;
  }

  return false;
}

bool ClientHandler::OnBeforePopup(CefRefPtr<CefBrowser> browser,
                                  CefRefPtr<CefFrame> frame,
                                  const CefString& target_url,
                                  const CefString& target_frame_name,
                                  const CefPopupFeatures& popupFeatures,
                                  CefWindowInfo& windowInfo,
                                  CefRefPtr<CefClient>& client,
                                  CefBrowserSettings& settings,
                                  bool* no_javascript_access) {
  if (browser->GetHost()->IsWindowRenderingDisabled()) {
    // Cancel popups in off-screen rendering mode.
    return true;
  }
  return false;
}

void ClientHandler::OnAfterCreated(CefRefPtr<CefBrowser> browser) {
  REQUIRE_UI_THREAD();

  // Disable mouse cursor change if requested via the command-line flag.
  if (m_bMouseCursorChangeDisabled)
    browser->GetHost()->SetMouseCursorChangeDisabled(true);

  AutoLock lock_scope(this);
  if (!m_Browser.get())   {
    // We need to keep the main child window, but not popup windows
    m_Browser = browser;
    m_BrowserId = browser->GetIdentifier();
  } else if (browser->IsPopup()) {
    // Add to the list of popup browsers.
    m_PopupBrowsers.push_back(browser);
  }

  m_BrowserCount++;
}

bool ClientHandler::DoClose(CefRefPtr<CefBrowser> browser) {
  REQUIRE_UI_THREAD();

  // Closing the main window requires special handling. See the DoClose()
  // documentation in the CEF header for a detailed destription of this
  // process.
  if (m_BrowserId == browser->GetIdentifier()) {
    // Notify the browser that the parent window is about to close.
    browser->GetHost()->ParentWindowWillClose();

    // Set a flag to indicate that the window close should be allowed.
    m_bIsClosing = true;
  }

  // Allow the close. For windowed browsers this will result in the OS close
  // event being sent.
  return false;
}

void ClientHandler::OnBeforeClose(CefRefPtr<CefBrowser> browser) {
  REQUIRE_UI_THREAD();

  if (m_BrowserId == browser->GetIdentifier()) {
    // Free the browser pointer so that the browser can be destroyed
    m_Browser = NULL;

    
  } else if (browser->IsPopup()) {
    // Remove the record for DevTools popup windows.
    std::set<std::string>::iterator it =
        m_OpenDevToolsURLs.find(browser->GetMainFrame()->GetURL());
    if (it != m_OpenDevToolsURLs.end())
      m_OpenDevToolsURLs.erase(it);

    // Remove from the browser popup list.
    BrowserList::iterator bit = m_PopupBrowsers.begin();
    for (; bit != m_PopupBrowsers.end(); ++bit) {
      if ((*bit)->IsSame(browser)) {
        m_PopupBrowsers.erase(bit);
        break;
      }
    }
  }

  if (--m_BrowserCount == 0) {
    // All browser windows have closed. Quit the application message loop.
    AppQuitMessageLoop();
  }
}

void ClientHandler::OnLoadStart(CefRefPtr<CefBrowser> browser,
                                CefRefPtr<CefFrame> frame) {
  REQUIRE_UI_THREAD();

  if (m_BrowserId == browser->GetIdentifier() && frame->IsMain()) {
    // We've just started loading a page
    SetLoading(true);
  }
}

void ClientHandler::OnLoadEnd(CefRefPtr<CefBrowser> browser,
                              CefRefPtr<CefFrame> frame,
                              int httpStatusCode) {
  REQUIRE_UI_THREAD();

  if (m_BrowserId == browser->GetIdentifier() && frame->IsMain()) {
    // We've just finished loading a page
    SetLoading(false);

    if (frame->GetURL() == YTD::kTestUrl)
      YTD::OnLoadEnd(browser);
  }
}

void ClientHandler::OnLoadError(CefRefPtr<CefBrowser> browser,
                                CefRefPtr<CefFrame> frame,
                                ErrorCode errorCode,
                                const CefString& errorText,
                                const CefString& failedUrl) {
  REQUIRE_UI_THREAD();

  // Don't display an error for downloaded files.
  if (errorCode == ERR_ABORTED)
    return;

  // Don't display an error for external protocols that we allow the OS to
  // handle. See OnProtocolExecution().
  if (errorCode == ERR_UNKNOWN_URL_SCHEME) {
    std::string urlStr = frame->GetURL();
    if (urlStr.find("spotify:") == 0)
      return;
  }

  // Display a load error message.
  std::stringstream ss;
  ss << "<html><body><h2>Failed to load URL " << std::string(failedUrl) <<
        " with error " << std::string(errorText) << " (" << errorCode <<
        ").</h2></body></html>";
  frame->LoadString(ss.str(), failedUrl);
}

void ClientHandler::OnRenderProcessTerminated(CefRefPtr<CefBrowser> browser,
                                              TerminationStatus status) {
  // Load the startup URL if that's not the website that we terminated on.
  CefRefPtr<CefFrame> frame = browser->GetMainFrame();
  std::string url = frame->GetURL();
  std::transform(url.begin(), url.end(), url.begin(), tolower);

  std::string startupURL = GetStartupURL();
  if (url.find(startupURL) != 0)
    frame->LoadURL(startupURL);
}

CefRefPtr<CefResourceHandler> ClientHandler::GetResourceHandler(
      CefRefPtr<CefBrowser> browser,
      CefRefPtr<CefFrame> frame,
      CefRefPtr<CefRequest> request) {
  std::string url = request->GetURL();
  if (url.find(kTestOrigin) == 0) {
    // Handle URLs in the test origin.
    std::string file_name, mime_type;
    if (ParseTestUrl(url, &file_name, &mime_type)) {
      if (file_name == "request.html") {
        // Show the request contents.
        std::string dump;
        DumpRequestContents(request, dump);
        CefRefPtr<CefStreamReader> stream =
            CefStreamReader::CreateForData(
                static_cast<void*>(const_cast<char*>(dump.c_str())),
                dump.size());
        ASSERT(stream.get());
        return new CefStreamResourceHandler("text/plain", stream);
      } else {
        // Load the resource from file.
        CefRefPtr<CefStreamReader> stream =
            GetBinaryResourceReader(file_name.c_str());
        if (stream.get())
          return new CefStreamResourceHandler(mime_type, stream);
      }
    }
  }

  return NULL;
}

bool ClientHandler::OnQuotaRequest(CefRefPtr<CefBrowser> browser,
                                   const CefString& origin_url,
                                   int64 new_size,
                                   CefRefPtr<CefQuotaCallback> callback) {
  static const int64 max_size = 1024 * 1024 * 20;  // 20mb.

  // Grant the quota request if the size is reasonable.
  callback->Continue(new_size <= max_size);
  return true;
}

void ClientHandler::OnProtocolExecution(CefRefPtr<CefBrowser> browser,
                                        const CefString& url,
                                        bool& allow_os_execution) {
  std::string urlStr = url;

  // Allow OS execution of Spotify URIs.
  if (urlStr.find("spotify:") == 0)
    allow_os_execution = true;
}

bool ClientHandler::GetRootScreenRect(CefRefPtr<CefBrowser> browser,
    CefRect& rect) {
  return false;
}

bool ClientHandler::GetViewRect(CefRefPtr<CefBrowser> browser, CefRect& rect) {
   return false;
}

bool ClientHandler::GetScreenPoint(CefRefPtr<CefBrowser> browser,
                                   int viewX,
                                   int viewY,
                                   int& screenX,
                                   int& screenY) {
  return false;
}

bool ClientHandler::GetScreenInfo(CefRefPtr<CefBrowser> browser,
                                  CefScreenInfo& screen_info) {
  return false;
}

void ClientHandler::OnPopupShow(CefRefPtr<CefBrowser> browser,
                                bool show) {
}

void ClientHandler::OnPopupSize(CefRefPtr<CefBrowser> browser,
                                const CefRect& rect) {
}

void ClientHandler::OnPaint(CefRefPtr<CefBrowser> browser,
                            PaintElementType type,
                            const RectList& dirtyRects,
                            const void* buffer,
                            int width,
                            int height) {
}


void ClientHandler::SetMainHwnd(CefWindowHandle hwnd) {
  AutoLock lock_scope(this);
  m_MainHwnd = hwnd;
}


void ClientHandler::CloseAllBrowsers(bool force_close) {
  if (!CefCurrentlyOn(TID_UI)) {
    // Execute on the UI thread.
    CefPostTask(TID_UI,
        NewCefRunnableMethod(this, &ClientHandler::CloseAllBrowsers,
                             force_close));
    return;
  }

  if (!m_PopupBrowsers.empty()) {
    // Request that any popup browsers close.
    BrowserList::const_iterator it = m_PopupBrowsers.begin();
    for (; it != m_PopupBrowsers.end(); ++it)
      (*it)->GetHost()->CloseBrowser(force_close);
  }

  if (m_Browser.get()) {
    // Request that the main browser close.
    m_Browser->GetHost()->CloseBrowser(force_close);
  }
}

std::string ClientHandler::GetLogFile() {
  AutoLock lock_scope(this);
  return m_LogFile;
}


// static
void ClientHandler::LaunchExternalBrowser(const std::string& url) {
  if (CefCurrentlyOn(TID_PROCESS_LAUNCHER)) {
    // Retrieve the current executable path.
    CefString file_exe;
    if (!CefGetPath(PK_FILE_EXE, file_exe))
      return;

    // Create the command line.
    CefRefPtr<CefCommandLine> command_line =
        CefCommandLine::CreateCommandLine();
    command_line->SetProgram(file_exe);
    command_line->AppendSwitchWithValue(cefclient::kUrl, url);

    // Launch the process.
    CefLaunchProcess(command_line);
  } else {
    // Execute on the PROCESS_LAUNCHER thread.
    CefPostTask(TID_PROCESS_LAUNCHER,
        NewCefRunnableFunction(&ClientHandler::LaunchExternalBrowser, url));
  }
}

// static
void ClientHandler::CreateProcessMessageDelegates(
      ProcessMessageDelegateSet& delegates) {
      YTD::CreateProcessMessageDelegates(delegates);
}

