// Module initialisation of the unit tests.
//
// The test data (reference signals, 73BPmin.mp4 video, Haar cascade xml file) are copied by the
// build next to heart_rate_tests.dll (see heart_rate_tests.vcxproj). The tests open them with
// relative paths, exactly as the original QtTest project did from its build directory, so the
// working directory is set to the folder containing the test dll before any test runs (the
// working directory chosen by the Test Explorer / vstest host is not the output folder).

#include "CppUnitTest.h"

#include <filesystem>
#include <string>

#ifdef _WIN32
#include <windows.h>
#endif

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace heart_rate_tests
{

static std::filesystem::path testModuleDirectory()
{
#ifdef _WIN32
    HMODULE module = nullptr;
    if (GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                           reinterpret_cast<LPCWSTR>(&testModuleDirectory), &module))
    {
        std::wstring path(MAX_PATH, L'\0');
        for (;;)
        {
            const DWORD length = GetModuleFileNameW(module, &path[0], static_cast<DWORD>(path.size()));
            if (length == 0) break;
            if (length < path.size())
            {
                path.resize(length);
                return std::filesystem::path(path).parent_path();
            }
            path.resize(path.size() * 2); // path longer than the buffer
        }
    }
#endif
    return std::filesystem::current_path();
}

TEST_MODULE_INITIALIZE(HeartRateTestsModuleInitialize)
{
    const std::filesystem::path dir = testModuleDirectory();
    std::filesystem::current_path(dir);
    Logger::WriteMessage((L"heart_rate_tests : working directory set to " + dir.wstring() + L"\n").c_str());
}

} // namespace heart_rate_tests
