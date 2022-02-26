// Patcher.cpp : Defines the entry point for the application.
//
#define NOMINMAX

#include "framework.h"
#include "Patcher.h"
#include "net.h"
#include "zip_file.hpp"

#include <string>
#include <filesystem>
#include <fstream>
#include <thread>
#include <CommCtrl.h>

#include "../lib/sfl/json.hpp"
using json = nlohmann::json;

json releaseInfo;

INT_PTR CALLBACK Wndproc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
    {
        auto data = net::fetch_request(net::url(L"https://api.github.com/repos/borf/browedit3/releases"));
        std::string sdata(data.begin(), data.end());
        OutputDebugString(sdata.c_str());
        releaseInfo = json::parse(sdata);
        OutputDebugString(releaseInfo.dump(2).c_str());
        HWND hwndList = GetDlgItem(hDlg, IDC_LIST);

        int i = 0;
        for (auto release : releaseInfo)
        {
            bool downloaded = false;
            if (std::filesystem::exists("zips\\" + release["name"].get<std::string>() + ".zip"))
                downloaded = true;

            std::string txt = release["name"].get<std::string>() + (downloaded ? " (downloaded)" : "");
            int pos = (int)SendMessage(hwndList, LB_ADDSTRING, 0, (LPARAM)txt.c_str());
            SendMessage(hwndList, LB_SETITEMDATA, pos, (LPARAM)i++);
        }
        SendMessage(hwndList, LB_SETCURSEL, 0, (LPARAM)0);
    }
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK)
        {
            HWND hwndList = GetDlgItem(hDlg, IDC_LIST);
            EndDialog(hDlg, SendMessage(hwndList, LB_GETCURSEL, 0, 0));
            return 0;
        }
        if (LOWORD(wParam) == IDC_LIST)
        {
            switch (HIWORD(wParam))
            {
            case LBN_SELCHANGE:
                HWND hwndList = GetDlgItem(hDlg, IDC_LIST);
                int lbItem = (int)SendMessage(hwndList, LB_GETCURSEL, 0, 0);
                int i = (int)SendMessage(hwndList, LB_GETITEMDATA, lbItem, 0);

                HWND hwndDetails = GetDlgItem(hDlg, IDC_DETAILS);
                if (releaseInfo[i].find("body") != releaseInfo[i].end())
                    SendMessage(hwndDetails, WM_SETTEXT, 0, (LPARAM)releaseInfo[i]["body"].get<std::string>().c_str());
                else
                    SendMessage(hwndDetails, WM_SETTEXT, 0, (LPARAM)"no information");

            }
        }
        return 0;
        break;
    }
    return DefWindowProc(hDlg, message, wParam, lParam);
}

HWND hProgressDlg;

INT_PTR CALLBACK DownloadWndProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    hProgressDlg = hDlg;
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        return 0;
    }
    return DefWindowProc(hDlg, message, wParam, lParam);
}


int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    if (!std::filesystem::exists("zips"))
        std::filesystem::create_directories("zips");

    int index = (int)DialogBox(hInstance, MAKEINTRESOURCE(IDD_ABOUTBOX), nullptr, Wndproc);
    std::string wantedVersionStr = releaseInfo[index]["name"].get<std::string>();
    int wantedVersion = std::stoi(wantedVersionStr.substr(wantedVersionStr.find(".")+1));
    int currentVersion = 0;

    DWORD  verHandle = 0;
    UINT   size = 0;
    LPBYTE lpBuffer = NULL;
    DWORD  verSize = GetFileVersionInfoSize(".\\BrowEdit3.exe", &verHandle);

    if (verSize != NULL)
    {
        LPSTR verData = new char[verSize];
        if(GetFileVersionInfo(".\\BrowEdit3.exe", verHandle, verSize, verData))
        {
            if (VerQueryValue(verData, "\\", (VOID FAR * FAR*) & lpBuffer, &size))
            {
                if (size)
                {
                    VS_FIXEDFILEINFO* verInfo = (VS_FIXEDFILEINFO*)lpBuffer;
                    if (verInfo->dwSignature == 0xfeef04bd)
                    {
                        currentVersion = verInfo->dwFileVersionLS;
                    }
                }
            }
        }
        delete[] verData;
    }

    if (wantedVersion != currentVersion)
    {
        auto& release = releaseInfo[index];
        std::string zipFileName = "zips\\" + release["name"].get<std::string>() + ".zip";
        if (!std::filesystem::exists(zipFileName))
        {
            std::thread t([&]()
                {
                    Sleep(500);
                    HWND hwndDialogBar = GetDlgItem(hProgressDlg, IDC_PROGRESS1);
                    SendMessage(hwndDialogBar, PBM_SETRANGE, 0, MAKELPARAM(0, 100));
                    SendMessage(hwndDialogBar, PBM_SETSTEP, (WPARAM)1, 0);

                    auto url = release["assets"][0]["browser_download_url"].get<std::string>();
                    auto data = net::fetch_request(net::url(std::wstring(url.begin(), url.end())), L"", L"", L"Mozilla/5.0 (Windows NT 6.1; WOW64) AppleWebKit/537.36 (KHTML, like Gecko)", [&](float p) {
                        SendMessage(hwndDialogBar, PBM_SETPOS, (int)(p*100), 0);
                        });
                    std::ofstream outStream(zipFileName, std::ios_base::out | std::ios_base::binary);
                    std::copy(data.begin(), data.end(), std::ostream_iterator<char>(outStream));
                    outStream.close();
                    EndDialog(hProgressDlg, 0);
                });
            DialogBox(hInstance, MAKEINTRESOURCE(IDD_DOWNLOADING), nullptr, DownloadWndProc);
            t.join();
        }

        miniz_cpp::zip_file zip(zipFileName);
        auto list = zip.infolist();
        for (auto& l : list)
            if (l.filename.ends_with("/"))
                std::filesystem::create_directories(l.filename);
        zip.extractall(".");


    }




    ShellExecute(nullptr, nullptr, "BrowEdit3.exe", "", nullptr, SW_SHOWDEFAULT);

    return 0;
}
