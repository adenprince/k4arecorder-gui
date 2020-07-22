/* Aden Prince
 * HiMER Lab at U. of Illinois, Chicago
 * K4ARecorder GUI
 * 
 * GUIWidgets.h
 * Contains code used in multiple source files, such as libraries
 * and function definitions.
 */

#include <string>

#include "imgui_dx11.h"
#include "imgui_internal.h"

// Enable or disable an integer input based on a passed boolean value
void conditionalInputInt(const char* label, int* value, bool enabled);
// Create ImGui widgets and get program arguments from them
int getArgs(std::string& argsStr, std::string& errorText, std::string& recorderPathStr);