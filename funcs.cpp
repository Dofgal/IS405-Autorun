#include "autorunwindow.h"
#include "ui_autorunwindow.h"
#include <QDebug>
#include <windows.h>
#include <tchar.h>
#include <stdio.h>
#include <string.h>
#include <windef.h>
#include <winreg.h>
#include <winbase.h>
#include <winnt.h>
#include <processenv.h>
#include <string>
#include <iostream>
#include <QStandardItemModel>
using namespace std;

// 获取路径中系统变量的真实路径
QString getRealPath(QString path)
{
    QString env = QString('\%') + path.split('\%')[1] + QString('\%');
    // qDebug() << "env: " << env;

    TCHAR envPathT[128] = _T("");
    TCHAR realPathT[128] = _T("");

    QString ret = QString("");

    env.toWCharArray(envPathT);

    // 获取真实路径的系统函数调用
    if (ExpandEnvironmentStrings(envPathT, realPathT, 128))
        ret = QString::fromWCharArray(realPathT) + path.split('\%')[2];
    else
        ret = QString("未找到系统变量") + path.split('\%')[2];

    return ret;
}

// 查询注册表启动项
void logonReg(HKEY mainKey, LPCTSTR subKey, QStandardItemModel *model)
{
    // 打开注册表项
    HKEY hk;
    LONG result;
    result = RegOpenKeyEx(mainKey, subKey, 0, KEY_READ, &hk);  // Windows api
    if (result != ERROR_SUCCESS)
    {
        if (result == ERROR_FILE_NOT_FOUND)
        {
            // qDebug("no reg found\n");
            return;
        }
        else
        {
            // qDebug("open reg error\n");
            return;
        }
    }

    // 遍历表项的变量设置
    DWORD dwIndex = 0;
    CHAR lpValueName[128];
    DWORD lpcchValueName = 128;
    DWORD lpType;
    BYTE lpData[128];
    DWORD lpcbData = 128;

    // Windows api 遍历表项
    while (RegEnumValueA(hk, dwIndex, lpValueName, &lpcchValueName, NULL, &lpType, lpData, &lpcbData) != ERROR_NO_MORE_ITEMS)
    {
        // byte数组转QString path
        char *tmp;
        QString path = "";

        tmp = (char *)lpData;
        int i = (tmp[0] == '\"') ? 1 : 0;
        for (; tmp[i] != '\"' && tmp[i] != '\0'; i++)
            path += tmp[i];

        qDebug() << lpValueName << ' ' << path;

        // 系统变量转换
        if (path[0] == '\%')
            path = getRealPath(path);

        // 填表
        QString subKQ = QString::fromWCharArray(subKey);
        if (mainKey == HKEY_LOCAL_MACHINE)
            subKQ.insert(0, "HKLM\\");
        else if (mainKey == HKEY_CURRENT_USER)
            subKQ.insert(0, "HKCU\\");
        int rowCnt = model->rowCount();
        model->setItem(rowCnt,2,new QStandardItem(subKQ));
        model->setItem(rowCnt,0,new QStandardItem(QString::fromStdString(lpValueName)));
        model->setItem(rowCnt,3,new QStandardItem(path));

        // 找到表项对应的文件，获取修改时间
        WIN32_FIND_DATA fileData;
        if (FindFirstFile((wchar_t*)path.utf16(), &fileData) == INVALID_HANDLE_VALUE)
            model->setItem(rowCnt,1,new QStandardItem(QString("未找到文件")));
        else
        {
            SYSTEMTIME lastT;
            char lastTStr[128];
            FileTimeToSystemTime(&(fileData.ftLastWriteTime), &lastT);
            sprintf(lastTStr, "%.2d:%.2d:%.2d  %.2d/%.2d/%.2d", (lastT.wHour + 8) % 24, lastT.wMinute, lastT.wSecond, lastT.wYear, lastT.wMonth, lastT.wDay);
            model->setItem(rowCnt,1,new QStandardItem(QString::fromStdString(lastTStr)));
        }

        // 重置变量
        dwIndex++;
        lpcchValueName = 128;
        lpcbData = 128;
        path.clear();
    }
    RegCloseKey(hk);
    return;
}

// 查询自启动目录
void logonDir(TCHAR *find_file, QStandardItemModel *model)
{
    // 打开第一个文件
    WIN32_FIND_DATA fileData;
    HANDLE hFind;
    TCHAR path[128] = _T("");
    TCHAR file[128] = _T("");
    TCHAR realPath[128] = _T("");
    QString envPath = QString::fromWCharArray(find_file);

    // 系统变量转换
    if (envPath[0] == '\%')
        getRealPath(envPath).toWCharArray(realPath);
    else
        _tcscpy(realPath, find_file);

    _tcscpy(path, realPath);
    _tcscpy(file, realPath);
    _tcscat(file, _T("\\*"));

    wcerr << file << '\n';

    hFind = FindFirstFile(file, &fileData);  // Windows api
    if (hFind == INVALID_HANDLE_VALUE)
    {
        qDebug() << "fail " << GetLastError() << '\n';
        return;
    }
    else
        wcerr << "fisrt file: " << fileData.cFileName << '\n';


    SYSTEMTIME lastT;
    char lastTStr[128];

    // 遍历文件夹
    while (FindNextFile(hFind, &fileData))  // Windows api 查询下一个文件
    {
        if (fileData.cFileName[0] != _T('.'))  // 跳过当前路径 '.'
        {
            wcerr << "next files: " << fileData.cFileName << '\n';

            // 跳过 .ini 文件
            QString name = QString::fromWCharArray(fileData.cFileName);
            if (name.split('.').last() == "ini")
                continue;

            if (!(fileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
            {
                // 填表
                FileTimeToSystemTime(&(fileData.ftLastWriteTime), &lastT);
                sprintf(lastTStr, "%.2d:%.2d:%.2d  %.2d/%.2d/%.2d", (lastT.wHour + 8) % 24, lastT.wMinute, lastT.wSecond, lastT.wYear, lastT.wMonth, lastT.wDay);
                int rowCnt = model->rowCount();
                model->setItem(rowCnt, 0, new QStandardItem(name));
                model->setItem(rowCnt, 2, new QStandardItem(QString::fromWCharArray(path)));
                model->setItem(rowCnt, 1, new QStandardItem(QString::fromStdString(lastTStr)));
            }
            else  // 子目录递归
            {
                qDebug() << "is dir" << '\n';
                _tcscat(path, _T("\\"));
                _tcscat(path, fileData.cFileName);
                logonDir(path, model);
            }
        }
        _tcscpy(path, realPath);
    }

    FindClose(hFind);
    return;
}

// 查询系统服务
void service(QStandardItemModel *model)
{
    HKEY subHKEY;

    // Windows api 打开根键
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, _T("System\\CurrentControlSet\\Services"), 0, KEY_ENUMERATE_SUB_KEYS, &subHKEY) == ERROR_SUCCESS)
    {
        FILETIME ftLastWriteTime;
        SYSTEMTIME lastT;
        char lastTStr[128];

        TCHAR subKeyName[128] = _T("");
        DWORD size = 128;
        int i = 0;

        // Windows api 遍历根键下的子键
        while (RegEnumKeyEx(subHKEY, i, subKeyName, &size, NULL, NULL, NULL, &ftLastWriteTime) != ERROR_NO_MORE_ITEMS)
        {
            HKEY subKey;
            TCHAR subKeyPath[128];
            _tcscpy(subKeyPath, _T("System\\CurrentControlSet\\Services\\"));
            _tcscat(subKeyPath, subKeyName);

            // Windows api 打开子键
            if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, subKeyPath, 0, KEY_READ, &subKey) == ERROR_SUCCESS)
            {
                DWORD imagePathType;
                BYTE imagePath[128];
                DWORD imagePathLen;

                DWORD startType;
                DWORD start;
                DWORD startLen;

                // Windows api 查询子键的值
                LONG ret1 = RegQueryValueEx(subKey, _T("ImagePath"), NULL, &imagePathType, imagePath, &imagePathLen);
                LONG ret2 = RegQueryValueEx(subKey, _T("Start"), NULL, &startType, (LPBYTE)&start, &startLen);
                if (ret1 == ERROR_SUCCESS && ret2 == ERROR_SUCCESS)
                {
                    // 不是服务项,或者start>2
                    if (!_tcsstr((TCHAR *)imagePath, _T(".exe")) || start > 2)
                    {
                        memset(subKeyName, 0 ,sizeof(subKeyName));
                        size = 128;
                        i++;
                        continue;
                    }

                    // 将文件时间转化成系统时间
                    FileTimeToSystemTime(&ftLastWriteTime, &lastT);
                    sprintf(lastTStr, "%.2d:%.2d:%.2d  %.2d/%.2d/%.2d",
                            (lastT.wHour + 8) % 24, lastT.wMinute, lastT.wSecond, lastT.wYear, lastT.wMonth, lastT.wDay);

                    // 统一化image path
                    QString imagePathQ = QString::fromWCharArray((TCHAR *)imagePath);
                    bool ifSvc = false;
                    // 如果路径是svchost.exe，转而查找description
                    if (imagePathQ.contains("svchost.exe"))
                    {
                        LONG ret3 = RegQueryValueEx(subKey, _T("Description"), NULL, &imagePathType, imagePath, &imagePathLen);
                        if (ret3 == ERROR_SUCCESS)
                        {
                            ifSvc = true;
                            imagePathQ = QString::fromWCharArray((TCHAR *)imagePath);
                            imagePathQ.remove(0, 1);
                            imagePathQ = imagePathQ.split(',')[0];
                        }
                    }
                    qDebug() << imagePathQ;

                    if (imagePathQ[0] == '\"')
                        imagePathQ = imagePathQ.split('\"')[1];

                    if (imagePathQ[0] == '\%')
                        imagePathQ = getRealPath(imagePathQ);

                    {
                        int i = 0;
                        QString tmp = "";
                        if (!ifSvc)
                            while(!tmp.contains(".exe"))
                                tmp = tmp + imagePathQ.split(' ')[i++] + ' ';
                        else
                            while(!tmp.contains(".dll"))
                                tmp = tmp + imagePathQ.split(' ')[i++] + ' ';
                        imagePathQ = tmp;

                        if (!imagePathQ.contains("C:"))
                            imagePathQ.insert(0, "C:\\WINDOWS\\system32\\");
                    }

                    // 填表
                    int rowCnt = model->rowCount();
                    QString subkeyname = QString::fromWCharArray(subKeyPath);
                    subkeyname = subkeyname.split('\\').last();

                    model->setItem(rowCnt, 0, new QStandardItem(subkeyname));
                    model->setItem(rowCnt, 2, new QStandardItem(QString::number(start)));
                    model->setItem(rowCnt, 3, new QStandardItem(imagePathQ));
                    model->setItem(rowCnt, 1, new QStandardItem(QString::fromStdString(lastTStr)));
                }
                else
                    printf("query fail\n");
            }
            else
                printf("subsub error.\n");

            memset(subKeyName, 0 ,sizeof(subKeyName));
            size = 128;
            i++;
        }
    }
    return;
}

// 查询驱动程序
void driver(QStandardItemModel *model)
{
    HKEY subHKEY;

    // Windows api 打开根键
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, _T("System\\CurrentControlSet\\Services"), 0, KEY_ENUMERATE_SUB_KEYS, &subHKEY) == ERROR_SUCCESS)
    {
        FILETIME ftLastWriteTime;
        SYSTEMTIME lastT;
        char lastTStr[128];

        TCHAR subKeyName[128] = _T("");
        DWORD size = 128;
        int i = 0;

        // Windows api 遍历根键下的子键
        while (RegEnumKeyEx(subHKEY, i, subKeyName, &size, NULL, NULL, NULL, &ftLastWriteTime) != ERROR_NO_MORE_ITEMS)
        {
            HKEY subKey;
            TCHAR subKeyPath[128];
            _tcscpy(subKeyPath, _T("System\\CurrentControlSet\\Services\\"));
            _tcscat(subKeyPath, subKeyName);

            // Windows api 打开子键
            if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, subKeyPath, 0, KEY_READ, &subKey) == ERROR_SUCCESS)
            {
                DWORD imagePathType;
                BYTE imagePath[128];
                DWORD imagePathLen;

                DWORD startType;
                DWORD start;
                DWORD startLen;

                // Windows api 查询子键的值
                LONG ret1 = RegQueryValueEx(subKey, _T("ImagePath"), NULL, &imagePathType, imagePath, &imagePathLen);
                LONG ret2 = RegQueryValueEx(subKey, _T("Start"), NULL, &startType, (LPBYTE)&start, &startLen);
                if (ret1 == ERROR_SUCCESS && ret2 == ERROR_SUCCESS)
                {
                    // 不是驱动程序，或者start>2
                    if (!_tcsstr((TCHAR *)imagePath, _T(".sys")) || start > 2)
                    {
                        memset(subKeyName, 0 ,sizeof(subKeyName));
                        size = 128;
                        i++;
                        continue;
                    }

                    // 将文件时间转化成系统时间
                    FileTimeToSystemTime(&ftLastWriteTime, &lastT);
                    sprintf(lastTStr, "%.2d:%.2d:%.2d  %.2d/%.2d/%.2d",
                            (lastT.wHour + 8) % 24, lastT.wMinute, lastT.wSecond, lastT.wYear, lastT.wMonth, lastT.wDay);

                    // 统一化image path
                    QString imagePathQ = QString::fromWCharArray((TCHAR *)imagePath);
                    QStringList imgPtSp = imagePathQ.split('\\');

                    if (imgPtSp[0].contains("system32", Qt::CaseInsensitive) || imgPtSp[0].contains("syswow64", Qt::CaseInsensitive))
                        imagePathQ.insert(0, QString("C:\\WINDOWS\\"));

                    if (imgPtSp[1].contains("systemroot", Qt::CaseInsensitive))
                    {
                        imgPtSp.replace(1, QString("\%SystemRoot\%"));
                        imgPtSp.removeAt(0);
                        imagePathQ = imgPtSp.join('\\');
                    }

                    if (imgPtSp.length() >= 3)
                        if (imgPtSp[2] == "C:")
                        {
                            imgPtSp.removeAt(0);
                            imgPtSp.removeAt(0);
                            imagePathQ = imgPtSp.join('\\');
                        }

                    if (imagePathQ[0] == '\%')
                        imagePathQ = getRealPath(imagePathQ);

//                    if (imagePathQ[0] == '\"')
//                        imagePathQ = imagePathQ.split('\"')[1];

//                    if (imagePathQ[0] == '\%')
//                        imagePathQ = getRealPath(imagePathQ);

//                    {
//                        int i = 0;
//                        QString tmp = "";
//                        while(!tmp.contains(".exe"))
//                            tmp = tmp + imagePathQ.split(' ')[i++] + ' ';
//                        imagePathQ = tmp;
//                    }

                    // 填表
                    int rowCnt = model->rowCount();
                    QString subkeyname = QString::fromWCharArray(subKeyPath);
                    subkeyname = subkeyname.split('\\').last();

                    model->setItem(rowCnt, 0, new QStandardItem(subkeyname));
                    model->setItem(rowCnt, 2, new QStandardItem(QString::number(start)));
                    model->setItem(rowCnt, 3, new QStandardItem(imagePathQ));
                    model->setItem(rowCnt, 1, new QStandardItem(QString::fromStdString(lastTStr)));
                }
//                else
//                    printf("query fail\n");
            }
            else
                printf("subsub error.\n");

            memset(subKeyName, 0 ,sizeof(subKeyName));
            size = 128;
            i++;
        }
    }
    return;
}

// 查询计划任务
void schdTask(TCHAR *find_file, QStandardItemModel *model)
{
    // 打开第一个文件
    WIN32_FIND_DATA fileData;
    HANDLE hFind;
    TCHAR path[128] = _T("");
    TCHAR file[128] = _T("");
    TCHAR realPath[128] = _T("");
    QString envPath = QString::fromWCharArray(find_file);
    if (envPath[0] == '\%')
        getRealPath(envPath).toWCharArray(realPath);
    else
        _tcscpy(realPath, find_file);

    _tcscpy(path, realPath);
    _tcscpy(file, realPath);
    _tcscat(file, _T("\\*"));

    // wcerr << file << '\n';

    hFind = FindFirstFile(file, &fileData);  // Windows api
    if (hFind == INVALID_HANDLE_VALUE)
    {
        qDebug() << "fail " << GetLastError() << '\n';
        return;
    }

    SYSTEMTIME lastT;
    char lastTStr[128];

    // Windows api 遍历文件夹
    while (FindNextFile(hFind, &fileData))
    {
        if (fileData.cFileName[0] != _T('.'))
        {
            //wcerr << "next files: " << fileData.cFileName << '\n';

            // 跳过 .ini 文件
            QString name = QString::fromWCharArray(fileData.cFileName);
            if (name.split('.').last() == "ini")
                continue;

            if (!(fileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
            {
                // 填表
                FileTimeToSystemTime(&(fileData.ftLastWriteTime), &lastT);
                sprintf(lastTStr, "%.2d:%.2d:%.2d  %.2d/%.2d/%.2d", (lastT.wHour + 8) % 24, lastT.wMinute, lastT.wSecond, lastT.wYear, lastT.wMonth, lastT.wDay);
                int rowCnt = model->rowCount();
                model->setItem(rowCnt, 0, new QStandardItem(name));
                model->setItem(rowCnt, 2, new QStandardItem(QString::fromWCharArray(path)));
                model->setItem(rowCnt, 1, new QStandardItem(QString::fromStdString(lastTStr)));
            }
            else  // 子目录操作
            {
                //qDebug() << "is dir" << '\n';
                _tcscat(path, _T("\\"));
                _tcscat(path, fileData.cFileName);
                logonDir(path, model);
            }
        }
        _tcscpy(path, realPath);
    }

    FindClose(hFind);
    return;
}

// 查询浏览器BHO
void ieExplorer(HKEY mainKey, TCHAR *sub, QStandardItemModel *model)
{
    QString mainKeyQ;
    if (mainKey == HKEY_LOCAL_MACHINE)
        mainKeyQ = "HKLM\\";
    else if (mainKey == HKEY_CURRENT_USER)
        mainKeyQ = "HKCU\\";
    HKEY subHKEY;

    // Windows api 打开根键
    if (RegOpenKeyEx(mainKey, sub, 0, KEY_ENUMERATE_SUB_KEYS, &subHKEY) == ERROR_SUCCESS)
    {
        FILETIME ftLastWriteTime;
        SYSTEMTIME lastT;
        char lastTStr[128];

        TCHAR subKeyName[128] = _T("");
        DWORD size = 128;
        int i = 0;

        // Windows api 遍历根键下的子键
        while (RegEnumKeyEx(subHKEY, i, subKeyName, &size,
                            NULL, NULL, NULL, &ftLastWriteTime) != ERROR_NO_MORE_ITEMS)
        {
            wcerr << subKeyName << ": ";

            HKEY subKey;
            TCHAR subKeyPath[128];

            // 将文件时间转化成系统时间
            FileTimeToSystemTime(&ftLastWriteTime, &lastT);
            sprintf(lastTStr, "%.2d:%.2d:%.2d  %.2d/%.2d/%.2d",
                    (lastT.wHour + 8) % 24, lastT.wMinute, lastT.wSecond, lastT.wYear, lastT.wMonth, lastT.wDay);

            _tcscpy(subKeyPath, sub);
            _tcscat(subKeyPath, _T("\\"));
            _tcscat(subKeyPath, subKeyName);
            wcerr << subKeyPath << '\n';

            // Windows api 依次打开子键
            if (RegOpenKeyEx(mainKey, subKeyPath, 0, KEY_READ, &subKey) == ERROR_SUCCESS)
            {
                DWORD btnType = 0;
                TCHAR btnValue[128] = _T("");
                DWORD btnValueLen = 128;

                int rowCnt = model->rowCount();
                DWORD iconType = 0;
                TCHAR iconValue[128] = _T("");
                DWORD iconValueLen = 128;

                // Windows api 查询相应项
                LONG ret1 = RegQueryValueEx(subKey, _T("ButtonText"), NULL, &btnType, (LPBYTE)&btnValue, &btnValueLen);
                LONG ret2 = RegQueryValueEx(subKey, _T("Icon"), NULL, &iconType, (LPBYTE)&iconValue, &iconValueLen);

                if (ret1 == ERROR_SUCCESS)  // 是IE Extensions中的项
                {
                    // 填表
                    model->setItem(rowCnt, 0, new QStandardItem(QString::fromWCharArray(btnValue)));
                    if (ret2 == ERROR_SUCCESS)
                    {
                        QString tmp = QString::fromWCharArray(iconValue);
                        tmp = tmp.split(',')[0];
                        model->setItem(rowCnt, 2, new QStandardItem(tmp));
                    }
                    else
                        model->setItem(rowCnt, 2, new QStandardItem(mainKeyQ + QString::fromWCharArray(subKeyPath)));
                    model->setItem(rowCnt, 1, new QStandardItem(QString::fromStdString(lastTStr)));
                }
                else  // 是Current Version/Explorer中的项
                {
                    // 查询不同的值
                    ret1 =RegQueryValueEx(subKey, _T(""), NULL, &btnType, (LPBYTE)&btnValue, &btnValueLen);

                    // 得到结果，则填表
                    if (ret1 == ERROR_SUCCESS && _tcslen(btnValue) > 0)
                    {
                        model->setItem(rowCnt, 0, new QStandardItem(QString::fromWCharArray(btnValue)));
                        model->setItem(rowCnt, 2, new QStandardItem(mainKeyQ + QString::fromWCharArray(subKeyPath)));
                        model->setItem(rowCnt, 1, new QStandardItem(QString::fromStdString(lastTStr)));
                    }
                    else
                        qDebug("query fail.");
                }

                btnValueLen = 128;
            }
            else
                qDebug("subsub error.\n");

            size = 128;
            i++;
        }
    }
    return;
}

// 查询知名DLL
void knownDll(LPCTSTR subKey, QStandardItemModel *model)
{
    // 打开注册表项
    HKEY hk;
    LONG result;
    result = RegOpenKeyEx(HKEY_LOCAL_MACHINE, subKey, 0, KEY_READ, &hk);  // Windows api
    if (result != ERROR_SUCCESS)
    {
        if (result == ERROR_FILE_NOT_FOUND)
        {
            // qDebug("no reg found\n");
            return;
        }
        else
        {
            // qDebug("open reg error\n");
            return;
        }
    }

    // 遍历表项的变量设置
    DWORD dwIndex = 0;
    TCHAR lpValueName[128];
    DWORD lpcchValueName = 128;
    DWORD lpType;
    TCHAR lpData[128];
    DWORD lpcbData = 128;


    // Windwos api 遍历表项
    while (RegEnumValue(hk, dwIndex, lpValueName, &lpcchValueName, NULL, &lpType, (LPBYTE)&lpData, &lpcbData) != ERROR_NO_MORE_ITEMS)
    {
        // 填表
        int rowCnt = model->rowCount();
        model->setItem(rowCnt,0,new QStandardItem(QString::fromWCharArray(lpData)));

        // 分别在两个文件夹中查找文件
        WIN32_FIND_DATA fileData;
        TCHAR dllPath1[128] = _T("C:\\Windows\\SysWow64\\");
        TCHAR dllPath2[128] = _T("C:\\Windows\\System32\\");
        TCHAR dllPathF[128] = _T("");

        _tcscat(dllPath1, lpData);
        if (FindFirstFile(dllPath1, &fileData) == INVALID_HANDLE_VALUE)  // Windows api 查找第一个文件
        {
            _tcscat(dllPath2, lpData);
            // 第一个文件夹中没找到，则在第二个文件夹中找
            if (FindFirstFile(dllPath2, &fileData) == INVALID_HANDLE_VALUE)
            {
                model->setItem(rowCnt,2,new QStandardItem("未找到文件"));
                model->setItem(rowCnt,1,new QStandardItem("----------"));
                dwIndex++;
                lpcchValueName = 128;
                lpcbData = 128;
                continue;
            }
            else
                _tcscpy(dllPathF, dllPath2);
        }
        else
            _tcscpy(dllPathF, dllPath1);

        // 路径
        model->setItem(rowCnt,2,new QStandardItem(QString::fromWCharArray(dllPathF)));

        // 时间
        SYSTEMTIME lastT;
        char lastTStr[128];
        FileTimeToSystemTime(&(fileData.ftLastWriteTime), &lastT);
        sprintf(lastTStr, "%.2d:%.2d:%.2d  %.2d/%.2d/%.2d", (lastT.wHour + 8) % 24, lastT.wMinute, lastT.wSecond, lastT.wYear, lastT.wMonth, lastT.wDay);
        model->setItem(rowCnt,1,new QStandardItem(QString::fromStdString(lastTStr)));


        dwIndex++;
        lpcchValueName = 128;
        lpcbData = 128;
    }
    RegCloseKey(hk);
    return;
}
