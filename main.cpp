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

#include <fstream>
#include <iostream>
#include <string>

#include <Windows.h>

#include "GLFW/glfw3.h"
#include "imgui_dx11.h"
#include "imgui_internal.h"

using namespace std;

// Check if a file exists with the passed filename
bool fileExists(string filename) {
    ifstream inputFile;
    inputFile.open(filename);
    bool isOpen = inputFile.is_open();
    inputFile.close();
    return isOpen;
}

// Enables or disables an integer input based on a passed boolean value
void conditionalInputInt(const char* label, int* value, bool enabled) {
    if(enabled == false) {
        // Disable next ImGui widget and gray it out
        ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
    }
    ImGui::InputInt(label, value);
    if(enabled == false) {
        // Remove disabled widget settings
        ImGui::PopItemFlag();
        ImGui::PopStyleVar();
    }
}

// Create ImGui widgets and get program arguments from them
int getArgs(string& argsStr, string& errorText, string& recorderPathStr) {
    // 0: Continue running GUI, 1: Start K4ARecorder, -1: Quit program
    int startRecorder = 0;

    if(ImGui::Button("Print help")) {
        argsStr += " --help";
        return 1;
    }
    ImGui::SameLine();
    if(ImGui::Button("List devices")) {
        argsStr += " --list";
        return 1;
    }
    
    // Display recording options
    static bool imu_recording_mode = false;
    static bool record_for_time = false;
    static int recording_time = 0;
    static int depth_delay = 0;

    if(ImGui::CollapsingHeader("Recording options")) {
        ImGui::Checkbox("Record IMU data", &imu_recording_mode);
        ImGui::Checkbox("Record for set time", &record_for_time);
        conditionalInputInt("Seconds to record", &recording_time, record_for_time);
        ImGui::InputInt("Depth delay", &depth_delay);
        ImGui::Separator();
    }

    // Display camera options
    const char* color_modes[] = {"OFF", "720p_YUY2", "720p_NV12", "720p", "1080p", "1440p", "1536p", "2160p", "3072p"};
    static int color_mode_index = 3; // Default color mode is 720p
    const char* depth_modes[] = {"OFF", "PASSIVE_IR", "WFOV_UNBINNED", "WFOV_2X2BINNED", "NFOV_UNBINNED", "NFOV_2X2BINNED"};
    static int depth_mode_index = 4; // Default depth mode is NFOV_UNBINNED
    const char* frame_rates[] = {"5", "15", "30"};
    static int frame_rate_index = 2; // Default frame rate is 30 FPS
    static bool manual_exposure = false;
    static bool manual_gain = false;
    static int exposure_value = 0;
    static int gain_value = 0;

    if(ImGui::CollapsingHeader("Camera options")) {
        ImGui::Combo("Color mode", &color_mode_index, color_modes, IM_ARRAYSIZE(color_modes));
        ImGui::Combo("Depth mode", &depth_mode_index, depth_modes, IM_ARRAYSIZE(depth_modes));
        ImGui::Combo("Frame rate", &frame_rate_index, frame_rates, IM_ARRAYSIZE(frame_rates));
        ImGui::Checkbox("Manual exposure", &manual_exposure);
        ImGui::Checkbox("Manual gain", &manual_gain);
        conditionalInputInt("Exposure value", &exposure_value, manual_exposure);
        conditionalInputInt("Gain value", &gain_value, manual_gain);
        ImGui::Separator();
    }

    // Multiple device options
    const char* external_sync_modes[] = {"Standalone", "Subordinate", "Master"};
    static int external_sync_mode_index = 0; // Default external sync mode is Standalone
    static int external_sync_delay = 0;
    static int device_index = 0;

    if(ImGui::CollapsingHeader("Multiple device options")) {
        ImGui::Combo("External sync mode", &external_sync_mode_index, external_sync_modes, IM_ARRAYSIZE(external_sync_modes));
        conditionalInputInt("External sync delay", &external_sync_delay, (external_sync_mode_index == 1));
        ImGui::InputInt("Device index", &device_index);
        ImGui::Separator();
    }
    
    static char recorder_path[128] = "C:/Program Files/Azure Kinect SDK v1.4.0/tools/k4arecorder.exe";
    ImGui::InputText("Recorder file path", recorder_path, IM_ARRAYSIZE(recorder_path));
    recorderPathStr = recorder_path;

    static char output_filename[128] = "";
    ImGui::InputText("Output filename", output_filename, IM_ARRAYSIZE(output_filename));

    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(ImColor::HSV(0.4f, 0.6f, 0.6f)));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(ImColor::HSV(0.4f, 0.7f, 0.7f)));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(ImColor::HSV(0.4f, 0.8f, 0.8f)));

    // Set arguments and attempt to start K4ARecorder when Start is clicked
    if(ImGui::Button("Start")) {
        // Reset args text
        argsStr = "";

        // Reset error text
        errorText = "";

        string output_filename_str = output_filename;

        // Set K4ARecorder command-line arguments
        argsStr += " --imu";
        if(imu_recording_mode) {
            argsStr += " ON";
        }
        else {
            argsStr += " OFF";
        }

        if(record_for_time) {
            argsStr += " --record-length " + to_string(recording_time);
        }

        argsStr += " --depth-delay " + to_string(depth_delay);
        argsStr += " --color-mode " + string(color_modes[color_mode_index]);
        argsStr += " --depth-mode " + string(depth_modes[depth_mode_index]);
        argsStr += " --rate " + string(frame_rates[frame_rate_index]);

        if(manual_exposure) {
            argsStr += " --exposure-control " + to_string(exposure_value);
        }

        if(manual_gain) {
            argsStr += " --gain " + to_string(gain_value);
        }
        
        argsStr += " --external-sync " + string(external_sync_modes[external_sync_mode_index]);
        argsStr += " --sync-delay " + to_string(external_sync_delay);
        argsStr += " --device " + to_string(device_index);
        argsStr += " " + output_filename_str;

        // 1 is returned and K4ARecorder starts if there are no errors
        startRecorder = 1;

        // Check for errors
        if(record_for_time && recording_time < 0) {
            errorText += "ERROR: Recording length cannot be negative\n";
            startRecorder = 0; // K4ARecorder will not start and the GUI will continue running
        }

        if(exposure_value < -11 || exposure_value > 200000) {
            errorText += "ERROR: Exposure value must be between -11 and 200,000\n";
            startRecorder = 0;
        }

        if(gain_value < 0 || gain_value > 255) {
            errorText += "ERROR: Gain value must be between 0 and 255\n";
            startRecorder = 0;
        }

        if(device_index < 0 || device_index > 255) {
            errorText += "ERROR: Device index must be between 0 and 255\n";
            startRecorder = 0;
        }

        if(external_sync_delay < 0) {
            errorText += "ERROR: External sync delay cannot be negative\n";
            startRecorder = 0;
        }

        if(fileExists(recorderPathStr) == false) {
            errorText += "ERROR: Recorder file path \"" +recorderPathStr + "\" not found\n";
            startRecorder = 0;
        }

        // Check if there are no non-space characters in the output filename
        if(output_filename_str.find_first_not_of(' ') == std::string::npos) {
            errorText += "ERROR: Output filename is empty\n";
            startRecorder = 0;
        }

        if(fileExists(output_filename_str)) {
            errorText += "ERROR: Output file \"" + output_filename_str + "\" already exists\n";
            startRecorder = 0;
        }
    }

    ImGui::SameLine();

    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(ImColor::HSV(0.0f, 0.6f, 0.6f)));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(ImColor::HSV(0.0f, 0.7f, 0.7f)));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(ImColor::HSV(0.0f, 0.8f, 0.8f)));

    if(ImGui::Button("Quit")) {
        startRecorder = -1; // Quit program
    }

    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.3f, 0.0f, 1.0f));
    ImGui::TextWrapped(errorText.c_str());

    // Remove style settings
    ImGui::PopStyleColor(7);

    // Return whether the program should continue running the GUI, start K4ARecorder, or quit
    return startRecorder;
}

int main(int argc, char* argv[]) {
    const float GUIScalingFactor = 1.5f;

    // 0: Continue running GUI, 1: Start K4ARecorder, -1: Quit program
    int startRecorder = 0;

    // Passed to getArgs function to be updated
    string argsStr;
    string errorText;
    string recorderPathStr;

    // Correct font scaling
    if(!glfwInit()) {
        return 1;
    }

    // Create application window
    WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(NULL), NULL, NULL, NULL, NULL, _T("K4ARecorder Options"), NULL };
    ::RegisterClassEx(&wc);
    HWND hwnd = ::CreateWindow(wc.lpszClassName, _T("K4ARecorder Options"), WS_OVERLAPPEDWINDOW, 100, 100, 720, 590, NULL, NULL, wc.hInstance, NULL);

    // Initialize Direct3D
    if(!CreateDeviceD3D(hwnd)) {
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