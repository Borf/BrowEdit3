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
int currentVersion = 0;
std::string effectCurrentVersion;
std::string effectLatestVersion;
HINSTANCE hInstance;

json releaseInfo;

void updateEffects(HWND hwnd);
void updateFfmpeg(HWND hwnd);


void setDesc(HWND hDlg, int i)
{
    HWND hwndDetails = GetDlgItem(hDlg, IDC_DETAILS);
    if (releaseInfo[i].find("body") != releaseInfo[i].end())
        SendMessage(hwndDetails, WM_SETTEXT, 0, (LPARAM)releaseInfo[i]["body"].get<std::string>().c_str());
    else
        SendMessage(hwndDetails, WM_SETTEXT, 0, (LPARAM)"no information");
}


INT_PTR CALLBACK Wndproc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
    {
        for (int i = 0; i < 4; i++)
        {
            try {
                auto data = net::fetch_request(net::url(L"https://api.github.com/repos/borf/browedit3/releases"));
                std::string sdata(data.begin(), data.end());
                OutputDebugString(sdata.c_str());
                releaseInfo = json::parse(sdata);
                break;
            }
            catch (...){ }
        }
        OutputDebugString(releaseInfo.dump(2).c_str());
        HWND hwndList = GetDlgItem(hDlg, IDC_LIST);

        int i = 0;
        for (auto release : releaseInfo)
        {
            bool downloaded = false;
            if (std::filesystem::exists("zips\\" + release["name"].get<std::string>() + ".zip"))
                downloaded = true;
            int version = std::stoi(release["name"].get<std::string>().substr(release["name"].get<std::string>().find(".") + 1));

            std::string txt = release["name"].get<std::string>();
            if (downloaded)
                txt += " (downloaded)";
            if (version == currentVersion)
                txt += " (active)";

            bool broken = true;
            for (auto& asset : release["assets"])
            {
                auto name = asset["name"].get<std::string>();
                if (name.substr(name.size() - 4) == ".zip")
                    broken = false;
            }
            if (broken)
                txt += " (broken)";


            int pos = (int)SendMessage(hwndList, LB_ADDSTRING, 0, (LPARAM)txt.c_str());
            SendMessage(hwndList, LB_SETITEMDATA, pos, (LPARAM)i++);
            
            if (version == currentVersion)
            {
                SendMessage(hwndList, LB_SETCURSEL, pos, (LPARAM)0);
                setDesc(hDlg, pos);
            }
        }
        HWND hwndCurrentEffect = GetDlgItem(hDlg, ID_EFFECTCURRENTVERSION);
        SendMessage(hwndCurrentEffect, WM_SETTEXT, 0, (LPARAM)effectCurrentVersion.c_str());
        json effectInfo;
        for (int i = 0; i < 4; i++)
        {
            try {
                auto data = net::fetch_request(net::url(L"https://api.github.com/repos/borf/roeffects/commits"));
                std::string sdata(data.begin(), data.end());
                OutputDebugString(sdata.c_str());
                effectInfo = json::parse(sdata);
                break;
            }
            catch (...) {}
        }
        if (!effectInfo.is_null())
        {
            HWND hwndCurrentEffect = GetDlgItem(hDlg, ID_EFFECTLATESTVERSION);
            effectLatestVersion = effectInfo[0]["sha"].get<std::string>();
            SendMessage(hwndCurrentEffect, WM_SETTEXT, 0, (LPARAM)effectLatestVersion.c_str());

            if (effectLatestVersion != effectCurrentVersion)
            {
                HWND hwndButton = GetDlgItem(hDlg, IDC_UPDATE);
                EnableWindow(hwndButton, TRUE);
            }

        }


    }
        return (INT_PTR)TRUE;
    case WM_CLOSE:
        EndDialog(hDlg, -1);
        break;
    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK)
        {
            HWND hwndList = GetDlgItem(hDlg, IDC_LIST);
            EndDialog(hDlg, 1+SendMessage(hwndList, LB_GETCURSEL, 0, 0));
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
                setDesc(hDlg, i);
            }
        }
        if (LOWORD(wParam) == IDC_UPDATE)
        {
            updateEffects(hDlg);
        }
        if (LOWORD(wParam) == IDC_UPDATEFFMPEG)
        {
            updateFfmpeg(hDlg);
        }

        return 0;
        break;
    }
    return DefWindowProc(hDlg, message, wParam, lParam);
}

HWND hProgressDlg = 0;

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

HWND hSwitchDlg;

INT_PTR CALLBACK SwitchWndProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    hSwitchDlg = hDlg;
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
    ::hInstance = hInstance;

    if (!std::filesystem::exists("zips"))
        std::filesystem::create_directories("zips");


    DWORD  verHandle = 0;
    UINT   size = 0;
    LPBYTE lpBuffer = NULL;
    DWORD  verSize = GetFileVersionInfoSize(".\\BrowEdit3.exe", &verHandle);

    if (verSize != NULL)
    {
        LPSTR verData = new char[verSize];
        if (GetFileVersionInfo(".\\BrowEdit3.exe", verHandle, verSize, verData))
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

    std::ifstream effectVersionFile("data\\texture\\effect\\version.txt");
    effectVersionFile >> effectCurrentVersion;
    effectVersionFile.close();

    int index = ((int)DialogBox(hInstance, MAKEINTRESOURCE(IDD_ABOUTBOX), nullptr, Wndproc)) - 1;
    if (index < 0)
        return -1;
    std::string wantedVersionStr = releaseInfo[index]["name"].get<std::string>();
    int wantedVersion = std::stoi(wantedVersionStr.substr(wantedVersionStr.find(".")+1));


    if (wantedVersion != currentVersion)
    {
        auto& release = releaseInfo[index];
        std::string zipFileName = "zips\\" + release["name"].get<std::string>() + ".zip";
        if (!std::filesystem::exists(zipFileName))
        {
            std::thread t([&]()
                {
                    while(hProgressDlg == 0)
                        Sleep(500);
                    HWND hwndDialogBar = GetDlgItem(hProgressDlg, IDC_PROGRESS1);
                    SendMessage(hwndDialogBar, PBM_SETRANGE, 0, MAKELPARAM(0, 100));
                    SendMessage(hwndDialogBar, PBM_SETSTEP, (WPARAM)1, 0);

                    std::string url = "";
                    for (auto& asset : release["assets"])
                    {
                        auto name = asset["name"].get<std::string>();
                        if(name.substr(name.size()-4) == ".zip")
                            url = asset["browser_download_url"] .get<std::string>();
                    }
                    if (url == "")
                    {
                        MessageBox(nullptr, "Error downloading asset. Make sure this release has a zipfile", "Error downloading", MB_OK);
                        ShellExecute(nullptr, nullptr, release["html_url"].get<std::string>().c_str(), "", nullptr, SW_SHOWDEFAULT);
                        EndDialog(hProgressDlg, 0);
                        exit(0);
                        return;
                    }
                    for (int i = 0; i < 4; i++)
                    {
                        try {
                            auto data = net::fetch_request(net::url(std::wstring(url.begin(), url.end())), L"", L"", L"Mozilla/5.0 (Windows NT 6.1; WOW64) AppleWebKit/537.36 (KHTML, like Gecko)", [&](float p) {
                                SendMessage(hwndDialogBar, PBM_SETPOS, (int)(p * 100), 0);
                                });
                            std::ofstream outStream(zipFileName, std::ios_base::out | std::ios_base::binary);
                            std::copy(data.begin(), data.end(), std::ostream_iterator<char>(outStream));
                            outStream.close();
                            break;
                        }
                        catch (...){}
                    }
                    EndDialog(hProgressDlg, 0);
                    hProgressDlg = 0;
                });
            DialogBox(hInstance, MAKEINTRESOURCE(IDD_DOWNLOADING), nullptr, DownloadWndProc);
            t.join();
        }

        std::thread t([&]() {
            miniz_cpp::zip_file zip(zipFileName);
            auto list = zip.infolist();
            for (auto& l : list)
                if (l.filename.ends_with("/"))
                    std::filesystem::create_directories(l.filename);
            zip.extractall(".");
            EndDialog(hSwitchDlg, 0);
            });
        DialogBox(hInstance, MAKEINTRESOURCE(IDD_SWITCHING), nullptr, SwitchWndProc);
        t.join();
    }

    ShellExecute(nullptr, nullptr, "BrowEdit3.exe", "", nullptr, SW_SHOWDEFAULT);

    return 0;
}




void updateEffects(HWND hwnd)
{
    std::thread t([&]()
        {
            while(hProgressDlg == 0)
                Sleep(500);
            HWND hwndDialogBar = GetDlgItem(hProgressDlg, IDC_PROGRESS1);
            HWND hwndDialogText = GetDlgItem(hProgressDlg, IDC_STATIC);
            SendMessage(hwndDialogBar, PBM_SETRANGE, 0, MAKELPARAM(0, 100));
            SendMessage(hwndDialogBar, PBM_SETSTEP, (WPARAM)1, 0);

            //std::string url = "https://api.github.com/repos/borf/roeffects/zipball/main";
            std::string url = "https://codeload.github.com/Borf/roeffects/zip/refs/heads/main";
            for (int i = 0; i < 4; i++)
            {
                try {
                    auto data = net::fetch_request(net::url(std::wstring(url.begin(), url.end())), L"", L"", L"Mozilla/5.0 (Windows NT 6.1; WOW64) AppleWebKit/537.36 (KHTML, like Gecko)", [&](float p) {
                        SendMessage(hwndDialogBar, PBM_SETPOS, (int)(p * 100), 0);
                        });
                    std::ofstream outStream("effects.zip", std::ios_base::out | std::ios_base::binary);
                    for (int i = 0; i < data.size(); i += 1024)
                        outStream.write(data.data() + i, std::min(1024, (int)data.size() - i));
                    //std::copy(data.begin(), data.end(), std::ostream_iterator<char>(outStream));
                    outStream.close();
                    break;
                }
                catch (net::error e)
                {
                    MessageBox(hwnd, "Error downloading effects", "Error", MB_OK);
                }
                catch (...) {}
            }

            std::thread t([&]() {
                SendMessage(hwndDialogBar, PBM_SETPOS, 0, 0);
                SendMessage(hwndDialogText, WM_SETTEXT, 0, (LPARAM)"Unzipping....");
                miniz_cpp::zip_file zip("effects.zip");
                auto list = zip.infolist();
                std::filesystem::create_directories("data\\texture\\effect");
                for (auto i = 0; i < list.size(); i++)
                {
                    SendMessage(hwndDialogBar, PBM_SETPOS, (int)((i / (float)list.size()) * 100), 0);
                    std::string file = list[i].filename;
                    if (file.ends_with(".gif") || file.ends_with(".gif.png"))
                    {
                        if (file.find("/") != std::string::npos)
                            file = file.substr(file.rfind("/") + 1);
                        if (file.find("\\") != std::string::npos)
                            file = file.substr(file.rfind("\\") + 1);
                        auto data = zip.read(list[i]);
                        std::ofstream out("data\\texture\\effect\\" + file, std::ios_base::binary | std::ios_base::out);
                        for (auto ii = 0; ii < data.size(); ii += 1024)
                            out.write(data.data()+ii, std::min(1024, (int)data.size()-ii));
                    }
                }
            });

            t.join();
            EndDialog(hProgressDlg, 0);
            hProgressDlg = 0;

            std::ofstream effectVersionFile("data\\texture\\effect\\version.txt");
            effectVersionFile << effectLatestVersion;
            effectVersionFile.close();
        });
    DialogBox(hInstance, MAKEINTRESOURCE(IDD_DOWNLOADING), hwnd, DownloadWndProc);
    t.join();

    HWND hwndCurrentEffect = GetDlgItem(hwnd, ID_EFFECTCURRENTVERSION);
    SendMessage(hwndCurrentEffect, WM_SETTEXT, 0, (LPARAM)effectLatestVersion.c_str());
    HWND hwndButton = GetDlgItem(hwnd, IDC_UPDATE);
    EnableWindow(hwndButton, FALSE);
}




void updateFfmpeg(HWND hwnd)
{
    std::thread t([&]()
        {
            while (hProgressDlg == 0)
                Sleep(500);
            HWND hwndDialogBar = GetDlgItem(hProgressDlg, IDC_PROGRESS1);
            HWND hwndDialogText = GetDlgItem(hProgressDlg, IDC_STATIC);
            SendMessage(hwndDialogBar, PBM_SETRANGE, 0, MAKELPARAM(0, 100));
            SendMessage(hwndDialogBar, PBM_SETSTEP, (WPARAM)1, 0);

            std::string url = "https://github.com/BtbN/FFmpeg-Builds/releases/download/latest/ffmpeg-master-latest-win64-gpl.zip";
            for (int i = 0; i < 4; i++)
            {
                try {
                    auto data = net::fetch_request(net::url(std::wstring(url.begin(), url.end())), L"", L"", L"Mozilla/5.0 (Windows NT 6.1; WOW64) AppleWebKit/537.36 (KHTML, like Gecko)", [&](float p) {
                        SendMessage(hwndDialogBar, PBM_SETPOS, (int)(p * 100), 0);
                        });
                    std::ofstream outStream("ffmpeg.zip", std::ios_base::out | std::ios_base::binary);
                    for (int i = 0; i < data.size(); i += 1024)
                        outStream.write(data.data() + i, std::min(1024, (int)data.size() - i));
                    //std::copy(data.begin(), data.end(), std::ostream_iterator<char>(outStream));
                    outStream.close();
                    break;
                }
                catch (...) {}
            }

            std::thread t([&]() {
                SendMessage(hwndDialogBar, PBM_SETPOS, 0, 0);
                SendMessage(hwndDialogText, WM_SETTEXT, 0, (LPARAM)"Hold on....");
                miniz_cpp::zip_file zip("ffmpeg.zip");
                auto list = zip.infolist();
                for (auto i = 0; i < list.size(); i++)
                {
                    std::string file = list[i].filename;
                    if (file.ends_with("ffmpeg.exe"))
                    {
                        SendMessage(hwndDialogText, WM_SETTEXT, 0, (LPARAM)"Unzipping....");
                        auto data = zip.read(list[i]);
                        std::ofstream out("ffmpeg.exe", std::ios_base::binary | std::ios_base::out);
                        for (auto ii = 0; ii < data.size(); ii += 1024)
                        {
                            SendMessage(hwndDialogBar, PBM_SETPOS, (int)((ii / (float)data.size()) * 100), 0);
                            out.write(data.data() + ii, std::min(1024, (int)data.size() - ii));
                        }
                    }
                }
                });

            t.join();
            EndDialog(hProgressDlg, 0);
            hProgressDlg = 0;
        });
    DialogBox(hInstance, MAKEINTRESOURCE(IDD_DOWNLOADING), hwnd, DownloadWndProc);
    t.join();

    HWND hwndCurrentEffect = GetDlgItem(hwnd, ID_EFFECTCURRENTVERSION);
    SendMessage(hwndCurrentEffect, WM_SETTEXT, 0, (LPARAM)effectLatestVersion.c_str());
    HWND hwndButton = GetDlgItem(hwnd, IDC_UPDATE);
    EnableWindow(hwndButton, FALSE);
}