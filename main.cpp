#include <fstream>
#include <iostream>
#include <string>

#include <Windows.h>

#include "GLFW/glfw3.h"
#include "imgui_dx11.h"

using namespace std;

// Check if a file exists with the passed filename
bool fileExists(string filename) {
    ifstream inputFile;
    inputFile.open(filename);
    bool isOpen = inputFile.is_open();
    inputFile.close();
    return isOpen;
}

// Create ImGui widgets and get program arguments from them
int getArgs(string& argsStr, string& errorText) {
    int startProgram = 0;

    const char* color_modes[] = {"OFF", "720p_YUY2", "720p_NV12", "720p", "1080p", "1440p", "1536p", "2160p", "3072p"};
    static int color_mode_index = 3;
    ImGui::Combo("Color mode", &color_mode_index, color_modes, IM_ARRAYSIZE(color_modes));

    const char* frame_rates[] = {"5", "15", "30"};
    static int frame_rate_index = 2;
    ImGui::Combo("Frame rate", &frame_rate_index, frame_rates, IM_ARRAYSIZE(frame_rates));

    static bool imu_recording_mode = false;
    ImGui::Checkbox("Record IMU data", &imu_recording_mode);

    static bool record_for_time = false;
    ImGui::Checkbox("Record for set time", &record_for_time);

    static int recording_time = 0;
    ImGui::InputInt("Seconds to record", &recording_time);

    static char output_filename[128] = "";
    ImGui::InputText("Output filename", output_filename, IM_ARRAYSIZE(output_filename));

    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(ImColor::HSV(0.4f, 0.6f, 0.6f)));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(ImColor::HSV(0.4f, 0.7f, 0.7f)));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(ImColor::HSV(0.4f, 0.8f, 0.8f)));

    if(ImGui::Button("Start")) {
        // Reset args text
        argsStr = "";

        // Reset error text
        errorText = "";

        if(record_for_time) {
            argsStr += " --record-length " + to_string(recording_time);
        }

        string colorMode = color_modes[color_mode_index];
        argsStr += " --color-mode " + colorMode;

        const int frame_rates_int[] = {5, 15, 30};
        argsStr += " --rate " + to_string(frame_rates_int[frame_rate_index]);

        argsStr += " --imu";
        if(imu_recording_mode) {
            argsStr += " ON";
        }
        else {
            argsStr += " OFF";
        }

        argsStr += " " + string(output_filename);

        startProgram = 1;

        string output_filename_str = output_filename;

        // Check for errors
        if(recording_time < 0) {
            errorText += "ERROR: Recording length cannot be negative\n";
            startProgram = 0;
        }

        if(output_filename_str == "") {
            errorText += "ERROR: Output filename is empty\n";
            startProgram = 0;
        }

        if(fileExists(output_filename_str)) {
            errorText += "ERROR: Output file \"" + output_filename_str + "\" already exists\n";
            startProgram = 0;
        }
    }

    ImGui::PopStyleColor(3);

    ImGui::SameLine();

    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(ImColor::HSV(0.0f, 0.6f, 0.6f)));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered,
        ImVec4(ImColor::HSV(0.0f, 0.7f, 0.7f)));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive,
        ImVec4(ImColor::HSV(0.0f, 0.8f, 0.8f)));

    if(ImGui::Button("Quit")) {
        startProgram = -1;
    }

    ImGui::PopStyleColor(3);

    ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.0f, 1.0f), errorText.c_str());

    return startProgram;
}

int main(int argc, char* argv[]) {
    const float scalingFactor = 1.5f;
    int startProgram = 0;
    string argsStr;
    string errorText;

    // Correct font scaling
    if(!glfwInit()) {
        return 1;
    }

    // Create application window
    WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(NULL), NULL, NULL, NULL, NULL, _T("K4ARecorder Options"), NULL };
    ::RegisterClassEx(&wc);
    HWND hwnd = ::CreateWindow(wc.lpszClassName, _T("K4ARecorder Options"), WS_OVERLAPPEDWINDOW, 100, 100, 640, 480, NULL, NULL, wc.hInstance, NULL);

    // Initialize Direct3D
    if(!CreateDeviceD3D(hwnd)) {
        CleanupDeviceD3D();
        ::UnregisterClass(wc.lpszClassName, wc.hInstance);
        return 1;
    }

    // Show the window
    ::ShowWindow(hwnd, SW_SHOWDEFAULT);
    ::UpdateWindow(hwnd);

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void) io;

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsClassic();

    // Setup Platform/Renderer bindings
    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

    // Our state
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    ImGui::GetStyle().ScaleAllSizes(scalingFactor);

    // ImGui doesn't automatically scale fonts, so we have to do that ourselves
    ImFontConfig fontConfig;
    constexpr float defaultFontSize = 13.0f;
    fontConfig.SizePixels = defaultFontSize * scalingFactor;
    ImGui::GetIO().Fonts->AddFontDefault(&fontConfig);

    // Main loop
    MSG msg;
    ZeroMemory(&msg, sizeof(msg));
    while(msg.message != WM_QUIT && startProgram == 0) {
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

        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(io.DisplaySize);
        
        ImGui::Begin("Options", (bool*) 0, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize);
        startProgram = getArgs(argsStr, errorText);
        ImGui::End();

        // Rendering
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
    
    // Empty message queue
    while(::PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE) != 0) {}

    if(startProgram == 2) {
        return 0;
    }

    cout << "Arguments: " << argsStr << endl;

    STARTUPINFO si;
    PROCESS_INFORMATION pi;

    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    // Copy arguments to LPWSTR variable so they can be passed to CreateProcess
    wstring argsWStr(argsStr.length(), L' ');
    copy(argsStr.begin(), argsStr.end(), argsWStr.begin());
    LPWSTR args = const_cast<LPWSTR>(argsWStr.c_str());

    // Start the recorder process
    if(!CreateProcess(L"c:\\Program Files\\Azure Kinect SDK v1.4.0\\tools\\k4arecorder.exe",   // Program file
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

// Helper functions

bool CreateDeviceD3D(HWND hWnd) {
    // Setup swap chain
    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory(&sd, sizeof(sd));
    sd.BufferCount = 2;
    sd.BufferDesc.Width = 0;
    sd.BufferDesc.Height = 0;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    UINT createDeviceFlags = 0;
    //createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0, };
    if(D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext) != S_OK)
        return false;

    CreateRenderTarget();
    return true;
}

void CreateRenderTarget() {
    ID3D11Texture2D* pBackBuffer;
    g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    g_pd3dDevice->CreateRenderTargetView(pBackBuffer, NULL, &g_mainRenderTargetView);
    pBackBuffer->Release();
}