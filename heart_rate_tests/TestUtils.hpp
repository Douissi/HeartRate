#ifndef TEST_UTILS_HPP
#define TEST_UTILS_HPP

// Helpers shared by the unit tests (Microsoft Native C++ Unit Test Framework).

#include "CppUnitTest.h"

#include <cmath>
#include <cstddef>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

// Wide string helpers for the test framework messages
#define UT_WIDEN2(s) L##s
#define UT_WIDEN(s) UT_WIDEN2(s)

// UT_VERIFY(condition) : equivalent of QVERIFY(condition). On failure the test fails with the
// text of the condition, the file and the line.
#define UT_VERIFY(condition)                                                             \
    do                                                                                   \
    {                                                                                    \
        if (!(condition))                                                                \
            ::ut::fail(UT_WIDEN(#condition), UT_WIDEN(__FILE__), __LINE__);              \
    } while (0)

namespace ut {

inline void fail(const std::wstring& what, const wchar_t* file, int line)
{
    std::wstringstream msg;
    msg << L"UT_VERIFY(" << what << L") failed at " << file << L"(" << line << L")";
    ::Microsoft::VisualStudio::CppUnitTestFramework::Assert::Fail(msg.str().c_str());
}

inline int rand()
{
    return std::rand()%1000;
}

// Checks |actual[i] - expected[i]| < tolerance for i in [0, count)
template <typename T>
void verifyAllNear(const std::vector<T>& actual, const std::vector<T>& expected, std::size_t count,
                   double tolerance, const wchar_t* file, int line)
{
    if (actual.size() < count || expected.size() < count)
    {
        std::wstringstream msg;
        msg << L"verifyAllNear : not enough samples (actual " << actual.size() << L", expected "
            << expected.size() << L", compared " << count << L") at " << file << L"(" << line << L")";
        ::Microsoft::VisualStudio::CppUnitTestFramework::Assert::Fail(msg.str().c_str());
    }
    for (std::size_t i = 0; i < count; i++)
    {
        const double diff = std::fabs(static_cast<double>(actual[i]) - static_cast<double>(expected[i]));
        if (!(diff < tolerance))
        {
            std::wstringstream msg;
            msg.precision(10);
            msg << L"sample " << i << L" : actual " << actual[i] << L", expected " << expected[i]
                << L", |difference| = " << diff << L" >= tolerance " << tolerance
                << L" at " << file << L"(" << line << L")";
            ::Microsoft::VisualStudio::CppUnitTestFramework::Assert::Fail(msg.str().c_str());
        }
    }
}

#define UT_VERIFY_ALL_NEAR(actual, expected, count, tolerance) \
    ::ut::verifyAllNear((actual), (expected), (count), (tolerance), UT_WIDEN(__FILE__), __LINE__)

// Reads a signal file written by signalProcessing::saveSigToFile ("index value" on each line)
template <typename T>
void loadSignalFromFile(const std::string& myFileString, std::vector<T>& signalVector)
{
    T values;
    std::fstream myfile;
    myfile.open(myFileString,std::fstream::in);

    if(myfile.is_open())
    {
        std::string line;
        while(myfile)
        {
            std::getline(myfile, line , ' ');
            myfile >> values;
            myfile.get();
            if(!myfile) break;
            signalVector.push_back(values);
        }
        myfile.close();
    }
    else
    {
        const std::wstring name(myFileString.begin(), myFileString.end());
        ::Microsoft::VisualStudio::CppUnitTestFramework::Assert::Fail(
            (L"Unable to open in reading mode the file " + name).c_str());
    }
}

// Reads a 2-column text file ("x y" on each line, e.g. "frequency power" written by signalFFT)
inline void loadColumnsFromFile(const std::string& fileName, std::vector<double>& x, std::vector<double>& y)
{
    std::ifstream file(fileName);
    if (!file.is_open())
    {
        const std::wstring name(fileName.begin(), fileName.end());
        ::Microsoft::VisualStudio::CppUnitTestFramework::Assert::Fail((L"Unable to open in reading mode the file " + name).c_str());
    }
    double a = 0.0, b = 0.0;
    while (file >> a >> b)
    {
        x.push_back(a);
        y.push_back(b);
    }
}

} // namespace ut

#endif // TEST_UTILS_HPP
