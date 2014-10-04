// Copyright (c) 2013 The Chromium Embedded Framework Authors. All rights
// reserved. Use of this source code is governed by a BSD-style license that
// can be found in the LICENSE file.

#include "cefclient/cefclient.h"
#include <windows.h>
#include <commdlg.h>
#include <shellapi.h>
#include <direct.h>
#include <sstream>
#include <string>
#include "include/cef_app.h"
#include "include/cef_browser.h"
#include "include/cef_frame.h"
#include "include/cef_runnable.h"
#include "cefclient/client_handler.h"
#include "cefclient/client_switches.h"
#include "cefclient/resource.h"
#include "cefclient/string_util.h"

#define MAX_LOADSTRING 100
#define MAX_URL_LENGTH  255
#define BUTTON_WIDTH 72
#define RECT_HEIGHT  5
#define WINDOW_WIDTH 1060
#define WINDOW_HEIGHT 550

// Global Variables:
HINSTANCE hInst;   // current instance
TCHAR szTitle[MAX_LOADSTRING];  // The title bar text
TCHAR szWindowClass[MAX_LOADSTRING];  // the main window class name
TCHAR szOSRWindowClass[MAX_LOADSTRING];  // the OSR window class name
char szWorkingDir[MAX_PATH];  // The current working directory

// Forward declarations of functions included in this code module:
ATOM MyRegisterClass(HINSTANCE hInstance);
BOOL InitInstance(HINSTANCE, int);
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

// Used for processing messages on the main application thread while running
// in multi-threaded message loop mode.
HWND hMessageWnd = NULL;
HWND CreateMessageWindow(HINSTANCE hInstance);
LRESULT CALLBACK MessageWndProc(HWND, UINT, WPARAM, LPARAM);

// The global ClientHandler reference.
extern CefRefPtr<ClientHandler> g_handler;

#if defined(OS_WIN)
// Add Common Controls to the application manifest because it's required to
// support the default tooltip implementation.
#pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")  // NOLINT(whitespace/line_length)
#endif

// Program entry point function.
int APIENTRY wWinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPTSTR    lpCmdLine,
                     int       nCmdShow) {
  UNREFERENCED_PARAMETER(hPrevInstance);
  UNREFERENCED_PARAMETER(lpCmdLine);

  CefMainArgs main_args(hInstance);
  CefRefPtr<ClientApp> app(new ClientApp);

  // Execute the secondary process, if any.
  int exit_code = CefExecuteProcess(main_args, app.get());
  if (exit_code >= 0)
    return exit_code;

  // Retrieve the current working directory.
  if (_getcwd(szWorkingDir, MAX_PATH) == NULL)
    szWorkingDir[0] = 0;

  // Parse command line arguments. The passed in values are ignored on Windows.
  AppInitCommandLine(0, NULL);

  CefSettings settings;

  // Populate the settings based on command line arguments.
  AppGetSettings(settings);

  // Initialize CEF.
  CefInitialize(main_args, settings, app.get());

  // Register the scheme handler.

  HACCEL hAccelTable;

  // Initialize global strings
  LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
  LoadString(hInstance, IDC_CEFCLIENT, szWindowClass, MAX_LOADSTRING);
  MyRegisterClass(hInstance);

  // Perform application initialization
  if (!InitInstance (hInstance, nCmdShow))
    return FALSE;

  hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_CEFCLIENT));

  int result = 0;

  if (!settings.multi_threaded_message_loop) {
    // Run the CEF message loop. This function will block until the application
    // recieves a WM_QUIT message.
    CefRunMessageLoop();
  } else {
    // Create a hidden window for message processing.
    hMessageWnd = CreateMessageWindow(hInstance);
    ASSERT(hMessageWnd);

    MSG msg;

    // Run the application message loop.
    while (GetMessage(&msg, NULL, 0, 0)) {
      if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
      }
    }

    DestroyWindow(hMessageWnd);
    hMessageWnd = NULL;

    result = static_cast<int>(msg.wParam);
  }

  // Shut down CEF.
  CefShutdown();

  return result;
}

//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
//  COMMENTS:
//
//    This function and its usage are only necessary if you want this code
//    to be compatible with Win32 systems prior to the 'RegisterClassEx'
//    function that was added to Windows 95. It is important to call this
//    function so that the application will get 'well formed' small icons
//    associated with it.
//
ATOM MyRegisterClass(HINSTANCE hInstance) {
  WNDCLASSEX wcex;

  wcex.cbSize = sizeof(WNDCLASSEX);

  wcex.style         = CS_HREDRAW | CS_VREDRAW;
  wcex.lpfnWndProc   = WndProc;
  wcex.cbClsExtra    = 0;
  wcex.cbWndExtra    = 0;
  wcex.hInstance     = hInstance;
  wcex.hIcon         = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_CEFCLIENT));
  wcex.hCursor       = LoadCursor(NULL, IDC_ARROW);
  wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
  wcex.lpszMenuName  = MAKEINTRESOURCE(IDC_CEFCLIENT);
  wcex.lpszClassName = szWindowClass;
  wcex.hIconSm       = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

  return RegisterClassEx(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow) {
  HWND hWnd;

  hInst = hInstance;  // Store instance handle in our global variable

  hWnd = CreateWindow(szWindowClass, szTitle,
                      WS_OVERLAPPEDWINDOW ^ WS_THICKFRAME | WS_CLIPCHILDREN, CW_USEDEFAULT, 0,
                      WINDOW_WIDTH, WINDOW_HEIGHT, NULL, NULL, hInstance, NULL);

  if (!hWnd)
    return FALSE;

  ShowWindow(hWnd, nCmdShow);
  UpdateWindow(hWnd);

  return TRUE;
}


//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE:  Processes messages for the main window.
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam,
                         LPARAM lParam) {

  // Static members used for the find dialog.
  static FINDREPLACE fr;
  static WCHAR szFindWhat[80] = {0};
  static WCHAR szLastFindWhat[80] = {0};
  static bool findNext = false;
  static bool lastMatchCase = false;

  PAINTSTRUCT ps;
  HDC hdc;
  // Callback for the main window
  switch (message) {
  case WM_CREATE: {
    // Create the single static handler class instance
    g_handler = new ClientHandler();
    g_handler->SetMainHwnd(hWnd);

    // Create the child windows used for navigation
    RECT rect;
    int x = 0;

    GetClientRect(hWnd, &rect);
    CefWindowInfo info;
    CefBrowserSettings settings;

    info.SetAsChild(hWnd, rect);
    // Creat the new child browser window
    CefBrowserHost::CreateBrowser(info, g_handler.get(),
        g_handler->GetStartupURL(), settings);

    return 0;
  }

  case WM_PAINT:
    hdc = BeginPaint(hWnd, &ps);
    EndPaint(hWnd, &ps);
    return 0;

  case WM_SETFOCUS:
    if (g_handler.get() && g_handler->GetBrowser()) {
      // Pass focus to the browser window
      CefWindowHandle hwnd =
          g_handler->GetBrowser()->GetHost()->GetWindowHandle();
      if (hwnd)
        PostMessage(hwnd, WM_SETFOCUS, wParam, NULL);
    }
    return 0;

  case WM_ERASEBKGND:
  if (g_handler.get() && g_handler->GetBrowser()) {
      CefWindowHandle hwnd =
          g_handler->GetBrowser()->GetHost()->GetWindowHandle();
      if (hwnd) {
        // Dont erase the background if the browser window has been loaded
        // (this avoids flashing)
        return 0;
      }
    }
    break;

  case WM_ENTERMENULOOP:
    if (!wParam) {
      // Entering the menu loop for the application menu.
      CefSetOSModalLoop(true);
    }
    break;

  case WM_EXITMENULOOP:
    if (!wParam) {
      // Exiting the menu loop for the application menu.
      CefSetOSModalLoop(false);
    }
    break;

  case WM_CLOSE:
    if (g_handler.get() && !g_handler->IsClosing()) {
      CefRefPtr<CefBrowser> browser = g_handler->GetBrowser();
      if (browser.get()) {
        // Notify the browser window that we would like to close it. This
        // will result in a call to ClientHandler::DoClose() if the
        // JavaScript 'onbeforeunload' event handler allows it.
        browser->GetHost()->CloseBrowser(false);

        // Cancel the close.
        return 0;
      }
    }

    // Allow the close.
    break;

  case WM_DESTROY:
    // Quitting CEF is handled in ClientHandler::OnBeforeClose().
    return 0;
  }

    return DefWindowProc(hWnd, message, wParam, lParam);
}

HWND CreateMessageWindow(HINSTANCE hInstance) {
  static const wchar_t kWndClass[] = L"ClientMessageWindow";

  WNDCLASSEX wc = {0};
  wc.cbSize = sizeof(wc);
  wc.lpfnWndProc = MessageWndProc;
  wc.hInstance = hInstance;
  wc.lpszClassName = kWndClass;
  RegisterClassEx(&wc);

  return CreateWindow(kWndClass, 0, 0, 0, 0, 0, 0, HWND_MESSAGE, 0,
                      hInstance, 0);
}

LRESULT CALLBACK MessageWndProc(HWND hWnd, UINT message, WPARAM wParam,
                                LPARAM lParam) {
  switch (message) {
    case WM_COMMAND: {
      int wmId = LOWORD(wParam);
      switch (wmId) {
        case ID_QUIT:
          PostQuitMessage(0);
          return 0;
      }
    }
  }
  return DefWindowProc(hWnd, message, wParam, lParam);
}


// Global functions

std::string AppGetWorkingDirectory() {
  return szWorkingDir;
}

void AppQuitMessageLoop() {
  CefRefPtr<CefCommandLine> command_line = AppGetCommandLine();
  if (command_line->HasSwitch(cefclient::kMultiThreadedMessageLoop)) {
    // Running in multi-threaded message loop mode. Need to execute
    // PostQuitMessage on the main application thread.
    ASSERT(hMessageWnd);
    PostMessage(hMessageWnd, WM_COMMAND, ID_QUIT, 0);
  } else {
    CefQuitMessageLoop();
  }
}
