#include <windows.h>
#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include <cmath>
#include <fstream>

#pragma comment(lib, "Advapi32.lib")

using namespace std;


//------------------------------------------------
// 打印EDID HEX
//------------------------------------------------
void PrintHex(const vector<BYTE>& edid)
{
    for (size_t i = 0; i < edid.size(); i++)
    {
        cout
            << hex
            << setw(2)
            << setfill('0')
            << (int)edid[i]
            << " ";

        if ((i + 1) % 16 == 0)
            cout << endl;
    }

    cout << dec << endl;
}


//------------------------------------------------
// EDID解析
//------------------------------------------------
void ParseEDID(const vector<BYTE>& edid)
{
    if (edid.size() < 128)
        return;


    // Manufacturer
    WORD id =
        ((WORD)edid[8] << 8) |
        edid[9];


    char manufacturer[4];

    manufacturer[0] =
        ((id >> 10) & 0x1F)
        + 'A' - 1;

    manufacturer[1] =
        ((id >> 5) & 0x1F)
        + 'A' - 1;

    manufacturer[2] =
        (id & 0x1F)
        + 'A' - 1;

    manufacturer[3] = 0;



    // Product ID
    WORD productID =
        edid[10] |
        ((WORD)edid[11] << 8);



    // Serial
    DWORD serial =
        edid[12] |
        ((DWORD)edid[13] << 8) |
        ((DWORD)edid[14] << 16) |
        ((DWORD)edid[15] << 24);



    // Date
    int week = edid[16];
    int year = edid[17] + 1990;



    // Size
    int width = edid[21];
    int height = edid[22];


    float diagonal =
        sqrt(
            width * width +
            height * height
        ) / 2.54f;



    // Monitor Name
    string name = "Unknown";
    string sn = "Unknown";

    for (int i = 54; i <= 108; i += 18)
    {
        if (edid[i] == 0x00 &&
            edid[i + 1] == 0x00 &&
            edid[i + 2] == 0x00 &&
            edid[i + 3] == 0xFC)
        {
            char temp[14] = { 0 };


            for (int j = 0; j < 13; j++)
            {
                if (edid[i + 5 + j] == 0x0A)
                    break;

                temp[j] = edid[i + 5 + j];
            }


            name = temp;

            break;
        }
    }

    // Serial Number Descriptor
    for (int i = 54; i <= 108; i += 18)
    {
        if (edid[i] == 0x00 &&
            edid[i + 1] == 0x00 &&
            edid[i + 2] == 0x00 &&
            edid[i + 3] == 0xFF)
        {
            char temp[14] = { 0 };


            for (int j = 0; j < 13; j++)
            {
                if (edid[i + 5 + j] == 0x0A)
                    break;


                if (edid[i + 5 + j] >= 0x20)
                {
                    temp[j] = edid[i + 5 + j];
                }
            }


            sn = temp;

            break;
        }
    }

    cout
        << "\n----------- EDID INFO -----------\n";


    cout
        << "Manufacturer : "
        << manufacturer
        << endl;


    cout
        << "Product ID   : 0x"
        << hex
        << uppercase
        << productID
        << dec
        << endl;


    cout
        << "Serial       : "
        << serial
        << endl;

    cout
        << "SN           : "
        << sn
        << endl;

    cout
        << "Date         : Week "
        << week
        << " Year "
        << year
        << endl;


    cout
        << "EDID Version : "
        << (int)edid[18]
        << "."
        << (int)edid[19]
        << endl;


    cout
        << "Name         : "
        << name
        << endl;


    cout
        << "Size         : "
        << width
        << "cm x "
        << height
        << "cm"
        << endl;


    cout
        << "Diagonal     : "
        << fixed
        << setprecision(1)
        << diagonal
        << " inch"
        << endl;


    cout
        << "---------------------------------\n";
}

void SaveEDIDTxt(const vector<BYTE>& edid)
{
    ofstream file("EDID.txt");

    if (!file.is_open())
    {
        cout << "Create EDID.txt failed\n";
        return;
    }


    for (size_t i = 0; i < edid.size(); i++)
    {
        file
            << "0x"
            << uppercase
            << hex
            << setw(2)
            << setfill('0')
            << (int)edid[i];


        if (i != edid.size() - 1)
        {
            file << ",";
        }


        // 每16个数据换行
        if ((i + 1) % 16 == 0)
        {
            file << "\n";
        }
        else
        {
            file << " ";
        }
    }


    file.close();


    cout << "Save EDID.txt OK\n";
}

//------------------------------------------------
// 读取EDID
//------------------------------------------------
bool ReadEDID(
    HKEY hKey,
    vector<BYTE>& edid)
{
    DWORD type;
    DWORD size = 0;


    // 获取实际长度
    if (RegQueryValueExA(
        hKey,
        "EDID",
        NULL,
        &type,
        NULL,
        &size)
        != ERROR_SUCCESS)
    {
        return false;
    }


    vector<BYTE> buffer(size);


    if (RegQueryValueExA(
        hKey,
        "EDID",
        NULL,
        &type,
        buffer.data(),
        &size)
        != ERROR_SUCCESS)
    {
        return false;
    }


    edid = buffer;


    return true;
}



//------------------------------------------------
// 扫描显示器
//------------------------------------------------
void ScanDisplay()
{
    HKEY hDisplay;


    if (RegOpenKeyExA(
        HKEY_LOCAL_MACHINE,
        "SYSTEM\\CurrentControlSet\\Enum\\DISPLAY",
        0,
        KEY_READ,
        &hDisplay)
        != ERROR_SUCCESS)
    {
        cout << "Open DISPLAY failed\n";
        return;
    }



    vector<BYTE> maxEDID;



    DWORD vendorIndex = 0;

    char vendor[256];



    while (true)
    {
        DWORD vendorLen = sizeof(vendor);


        if (RegEnumKeyExA(
            hDisplay,
            vendorIndex,
            vendor,
            &vendorLen,
            NULL,
            NULL,
            NULL,
            NULL)
            != ERROR_SUCCESS)
        {
            break;
        }



        HKEY hVendor;


        if (RegOpenKeyExA(
            hDisplay,
            vendor,
            0,
            KEY_READ,
            &hVendor)
            == ERROR_SUCCESS)
        {


            DWORD devIndex = 0;

            char device[256];



            while (true)
            {
                DWORD deviceLen = sizeof(device);



                if (RegEnumKeyExA(
                    hVendor,
                    devIndex,
                    device,
                    &deviceLen,
                    NULL,
                    NULL,
                    NULL,
                    NULL)
                    != ERROR_SUCCESS)
                {
                    break;
                }



                string path =
                    "SYSTEM\\CurrentControlSet\\Enum\\DISPLAY\\";

                path += vendor;
                path += "\\";
                path += device;
                path += "\\Device Parameters";



                HKEY hEDID;



                if (RegOpenKeyExA(
                    HKEY_LOCAL_MACHINE,
                    path.c_str(),
                    0,
                    KEY_READ,
                    &hEDID)
                    == ERROR_SUCCESS)
                {

                    vector<BYTE> data;


                    if (ReadEDID(
                        hEDID,
                        data))
                    {

                        // 保留最大EDID
                        if (data.size() > maxEDID.size())
                        {
                            maxEDID = data;
                        }
                    }


                    RegCloseKey(hEDID);
                }


                devIndex++;
            }


            RegCloseKey(hVendor);
        }


        vendorIndex++;
    }



    RegCloseKey(hDisplay);



    // 最终输出最大EDID

    if (!maxEDID.empty())
    {
        cout
            << "\n============================\n";

        cout
            << "EDID Size : "
            << maxEDID.size()
            << " Bytes\n";


        //解析
        ParseEDID(maxEDID);


        //保存TXT
        SaveEDIDTxt(maxEDID);


        //打印HEX
        cout
            << "\n----------- EDID HEX -----------\n";

        PrintHex(maxEDID);
    }
}



//------------------------------------------------
// Main
//------------------------------------------------
int main()
{

    cout
        << "Scan Monitor EDID...\n";


    ScanDisplay();


    cout
        << "\nPress any key to exit...";


    getchar();


    return 0;
}