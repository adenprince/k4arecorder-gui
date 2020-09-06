/* Aden Prince
 * HiMER Lab at U. of Illinois, Chicago
 * K4ARecorder GUI
 * 
 * main.cpp
 * Runs K4ARecorder, a tool to record from an Azure Kinect, using options from a GUI.
 * 
 * ImGui sample code obtained from: https://github.com/ocornut/imgui/blob/master/examples/example_win32_directx11/main.cpp
 * ImGui font scaling code obtained from: https://github.com/microsoft/Azure-Kinect-Sensor-SDK/blob/develop/tools/k4aviewer/k4aviewer.cpp
 * CreateProcess code obtained from: https://docs.microsoft.com/en-us/windows/win32/procthread/creating-processes
 */

#include "GUIWidgets.h"

#include <iostream>

#include <Windows.h>

#include "GLFW/glfw3.h"

using namespace std;

int main(int argc, char* argv[]) {
    const float GUIScalingFactor = 1.5f;

    // 0: Continue running GUI, 1: Start K4ARecorder, -1: Quit program
    int startRecorder = 0;

    // Passed to getArgs function to be updated
    string argsStr;
    string errorText;
    string recorderPathStr;
    
    // Detect Azure Kinect SDK folder in Program Files
    WIN32_FIND_DATAA SDKFolderData;
    HANDLE SDKFolderHandle;
    bool SDKFolderFound = false;

    SDKFolderHandle = FindFirstFileA("C:\\Program Files\\Azure Kinect SDK*", &SDKFolderData);

    if(SDKFolderHandle != INVALID_HANDLE_VALUE) {
        SDKFolderFound = true;
        FindClose(SDKFolderHandle);
    }

    // Set recorder path string to folder name
    if(SDKFolderFound) {
        recorderPathStr = SDKFolderData.cFileName;
    }
    else {
        // Set to latest version (at time of writing) if SDK folder not found
        recorderPathStr = "Azure Kinect SDK v1.4.1";
    }

    // Set full path for K4ARecorder executable
    recorderPathStr = "C:\\Program Files\\" + recorderPathStr + "\\tools\\k4arecorder.exe";

    // Correct font scaling
    if(!glfwInit()) {
        string errorText = "GLFW failed to initialize.";
        cout << errorText << endl;
        MessageBoxA(0, errorText.c_str(), NULL, MB_OK | MB_ICONHAND);
        return 1;
    }

    // Create application window
    WNDCLASSEX wc = {sizeof(WNDCLASSEX), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(NULL), NULL, NULL, NULL, NULL, _T("K4ARecorder Options"), NULL};
    ::RegisterClassEx(&wc);
    HWND hwnd = ::CreateWindow(wc.lpszClassName, _T("K4ARecorder Options"), WS_OVERLAPPEDWINDOW, 100, 100, 800, 590, NULL, NULL, wc.hInstance, NULL);

    // Initialize Direct3D
    if(!CreateDeviceD3D(hwnd)) {
        string errorText = "Direct3D failed to initialize.";
        cout << errorText << endl;
        MessageBoxA(0, errorText.c_str(), NULL, MB_OK | MB_ICONHAND);
        CleanupDeviceD3D();
        ::UnregisterClass(wc.lpszClassName, wc.hInstance);
        return 1;
    }

    // Show the window
    ::ShowWindow(hwnd, SW_SHOWDEFAULT);
    ::UpdateWindow(hwnd);

    // Set up Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void) io;

    // Set up Dear ImGui style
    ImGui::StyleColorsDark();

    // Set up Platform/Renderer bindings
    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

    // Our state
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    ImGui::GetStyle().ScaleAllSizes(GUIScalingFactor);

    // Scale ImGui font
    ImFontConfig fontConfig;
    constexpr float defaultFontSize = 13.0f;
    fontConfig.SizePixels = defaultFontSize * GUIScalingFactor;
    ImGui::GetIO().Fonts->AddFontDefault(&fontConfig);

    // Main loop
    MSG msg;
    ZeroMemory(&msg, sizeof(msg));

    // Run until the window is closed or Quit is clicked
    while(msg.message != WM_QUIT && startRecorder == 0) {
        // Poll and handle messages (inputs, window resize, etc.)
        if(::PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE)) {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
            continue;
        }

        // Start the Dear ImGui frame
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        // Make next ImGui window fill OS window
        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(io.DisplaySize);
        
        // Open options window
        ImGui::Begin("Options", (bool*) 0, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize);
        startRecorder = getArgs(argsStr, errorText, recorderPathStr);
        ImGui::End();

        // Render
        ImGui::Render();
        g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, NULL);
        g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, (float*) &clear_color);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        g_pSwapChain->Present(1, 0); // Present with vsync
        //g_pSwapChain->Present(0, 0); // Present without vsync
    }

    // Cleanup
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    CleanupDeviceD3D();
    ::DestroyWindow(hwnd);
    ::UnregisterClass(wc.lpszClassName, wc.hInstance);

    // Stop program if the window was closed or Quit was clicked
    if(msg.message == WM_QUIT || startRecorder == -1) {
        return 0;
    }

    // Empty message queue
    while(::PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE) != 0) {}

    cout << "Arguments: " << argsStr << endl;

    STARTUPINFO si;
    PROCESS_INFORMATION pi;

    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    // Copy arguments to LPWSTR variable to be passed to CreateProcess
    wstring argsWStr(argsStr.length(), L' ');
    copy(argsStr.begin(), argsStr.end(), argsWStr.begin());
    LPWSTR args = const_cast<LPWSTR>(argsWStr.c_str());

    // Copy recorder file path to LPCWSTR variable to be passed to CreateProcess
    wstring recorderPathWStr(recorderPathStr.length(), L' ');
    copy(recorderPathStr.begin(), recorderPathStr.end(), recorderPathWStr.begin());
    LPCWSTR recorderPathArg = const_cast<LPCWSTR>(recorderPathWStr.c_str());

    // Start K4ARecorder process
    if(!CreateProcess(recorderPathArg,   // Program file
        args,           // Command line
        NULL,           // Process handle not inheritable
        NULL,           // Thread handle not inheritable
        FALSE,          // Set handle inheritance to FALSE
        0,              // No creation flags
        NULL,           // Use parent's environment block
        NULL,           // Use parent's starting directory
        &si,            // Pointer to STARTUPINFO structure
        &pi)            // Pointer to PROCESS_INFORMATION structure
        ) {
        printf("CreateProcess failed (%d).\n", GetLastError());
        return 1;
    }

    // Wait until child process exits.
    WaitForSingleObject(pi.hProcess, INFINITE);

    // Close process and thread handles. 
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    return 0;
}