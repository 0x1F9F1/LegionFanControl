/*
   PM-port(62/66)
   KBC-port(60/64)
   EC-port(2E/2F or 4E/4F)
*/

#include <Windows.h>

#include "winio.h"
#pragma comment(lib, "WinIox64.lib")

//==========================The hardware port to read/write function================================
#define READ_PORT(port, data2) GetPortVal(port, &data2, 1);
#define WRITE_PORT(port, data2) SetPortVal(port, data2, 1)
//==================================================================================================

//======================================== PM channel ==============================================
#define PM_STATUS_PORT66 0x66
#define PM_CMD_PORT66 0x66
#define PM_DATA_PORT62 0x62
#define PM_OBF 0x01
#define PM_IBF 0x02
//------------wait EC PM channel port66 output buffer full-----/
void Wait_PM_OBF(void)
{
    DWORD data;
    READ_PORT(PM_STATUS_PORT66, data);
    while (!(data & PM_OBF)) {
        READ_PORT(PM_STATUS_PORT66, data);
    }
}

//------------wait EC PM channel port66 input buffer empty-----/
void Wait_PM_IBE(void)
{
    DWORD data;
    READ_PORT(PM_STATUS_PORT66, data);
    while (data & PM_IBF) {
        READ_PORT(PM_STATUS_PORT66, data);
    }
}

//------------send command by EC PM channel--------------------/
void Send_cmd_by_PM(BYTE Cmd)
{
    Wait_PM_IBE();
    WRITE_PORT(PM_CMD_PORT66, Cmd);
    Wait_PM_IBE();
}

//------------send data by EC PM channel-----------------------/
void Send_data_by_PM(BYTE Data)
{
    Wait_PM_IBE();
    WRITE_PORT(PM_DATA_PORT62, Data);
    Wait_PM_IBE();
}

//-------------read data from EC PM channel--------------------/
BYTE Read_data_from_PM(void)
{
    DWORD data;
    Wait_PM_OBF();
    READ_PORT(PM_DATA_PORT62, data);
    return (BYTE)data;
}
//--------------write EC RAM-----------------------------------/
void EC_WriteByte_PM(BYTE index, BYTE data)
{
    Send_cmd_by_PM(0x81);
    Send_data_by_PM(index);
    Send_data_by_PM(data);
}
//--------------read EC RAM------------------------------------/
BYTE EC_ReadByte_PM(BYTE index)
{
    BYTE data;
    Send_cmd_by_PM(0x80);
    Send_data_by_PM(index);
    data = Read_data_from_PM();
    return data;
}
//==================================================================================================

//================================KBC channel=======================================================
#define KBC_STATUS_PORT64 0x64
#define KBC_CMD_PORT64 0x64
#define KBC_DATA_PORT60 0x60
#define KBC_OBF 0x01
#define KBC_IBF 0x02
// wait EC KBC channel port64 output buffer full
void Wait_KBC_OBF(void)
{
    DWORD data;
    READ_PORT(KBC_STATUS_PORT64, data);
    while (!(data & KBC_OBF)) {
        READ_PORT(KBC_STATUS_PORT64, data);
    }
}

// wait EC KBC channel port64 output buffer empty
void Wait_KBC_OBE(void)
{
    DWORD data;
    READ_PORT(KBC_STATUS_PORT64, data);
    while (data & KBC_OBF) {
        READ_PORT(KBC_DATA_PORT60, data);
        READ_PORT(KBC_STATUS_PORT64, data);
    }
}

// wait EC KBC channel port64 input buffer empty
void Wait_KBC_IBE(void)
{
    DWORD data;
    READ_PORT(KBC_STATUS_PORT64, data);
    while (data & KBC_IBF) {
        READ_PORT(KBC_STATUS_PORT64, data);
    }
}

// send command by EC KBC channel
void Send_cmd_by_KBC(BYTE Cmd)
{
    Wait_KBC_OBE();
    Wait_KBC_IBE();
    WRITE_PORT(KBC_CMD_PORT64, Cmd);
    Wait_KBC_IBE();
}

// send data by EC KBC channel
void Send_data_by_KBC(BYTE Data)
{
    Wait_KBC_OBE();
    Wait_KBC_IBE();
    WRITE_PORT(KBC_DATA_PORT60, Data);
    Wait_KBC_IBE();
}

// read data from EC KBC channel
BYTE Read_data_from_KBC(void)
{
    DWORD data;
    Wait_KBC_OBF();
    READ_PORT(KBC_DATA_PORT60, data);
    return (BYTE)data;
}
// Write EC RAM via KBC port(60/64)
void EC_WriteByte_KBC(BYTE index, BYTE data)
{
    Send_cmd_by_KBC(0x81);
    Send_data_by_KBC(index);
    Send_data_by_KBC(data);
}

// Read EC RAM via KBC port(60/64)
BYTE EC_ReadByte_KBC(BYTE index)
{
    Send_cmd_by_KBC(0x80);
    Send_data_by_KBC(index);
    return Read_data_from_KBC();
}
//==================================================================================================

//=======================================EC Direct Access interface=================================
// Port Config:
//  BADRSEL(0x200A) bit1-0  Addr    Data
//                  00      2Eh     2Fh
//                  01      4Eh     4Fh
//
//              01      4Eh     4Fh
//  ITE-EC Ram Read/Write Algorithm:
//  Addr    w   0x2E
//  Data    w   0x11
//  Addr    w   0x2F
//  Data    w   high byte
//  Addr    w   0x2E
//  Data    w   0x10
//  Addr    w   0x2F
//  Data    w   low byte
//  Addr    w   0x2E
//  Data    w   0x12
//  Addr    w   0x2F
//  Data    rw  value
UINT8 EC_ADDR_PORT = 0x4E; // 0x2E or 0x4E
UINT8 EC_DATA_PORT = 0x4F; // 0x2F or 0x4F

unsigned char ECRamReadExt_Direct(unsigned short iIndex)
{
    DWORD data;
    WRITE_PORT(EC_ADDR_PORT, 0x2E);
    WRITE_PORT(EC_DATA_PORT, 0x11);
    WRITE_PORT(EC_ADDR_PORT, 0x2F);
    WRITE_PORT(EC_DATA_PORT, iIndex >> 8); // High byte

    WRITE_PORT(EC_ADDR_PORT, 0x2E);
    WRITE_PORT(EC_DATA_PORT, 0x10);
    WRITE_PORT(EC_ADDR_PORT, 0x2F);
    WRITE_PORT(EC_DATA_PORT, iIndex & 0xFF); // Low byte

    WRITE_PORT(EC_ADDR_PORT, 0x2E);
    WRITE_PORT(EC_DATA_PORT, 0x12);
    WRITE_PORT(EC_ADDR_PORT, 0x2F);
    READ_PORT(EC_DATA_PORT, data);
    return (BYTE)data;
}

void ECRamWriteExt_Direct(unsigned short iIndex, BYTE data)
{
    DWORD data1;
    data1 = data;
    WRITE_PORT(EC_ADDR_PORT, 0x2E);
    WRITE_PORT(EC_DATA_PORT, 0x11);
    WRITE_PORT(EC_ADDR_PORT, 0x2F);
    WRITE_PORT(EC_DATA_PORT, iIndex >> 8); // High byte

    WRITE_PORT(EC_ADDR_PORT, 0x2E);
    WRITE_PORT(EC_DATA_PORT, 0x10);
    WRITE_PORT(EC_ADDR_PORT, 0x2F);
    WRITE_PORT(EC_DATA_PORT, iIndex & 0xFF); // Low byte

    WRITE_PORT(EC_ADDR_PORT, 0x2E);
    WRITE_PORT(EC_DATA_PORT, 0x12);
    WRITE_PORT(EC_ADDR_PORT, 0x2F);
    WRITE_PORT(EC_DATA_PORT, data1);
}

// 0x0000 - 0xBFFF: EC Regs @ 0x00F00000
// 0xC000 - 0xFFFF: EC RAM  @ 0x00080000 (memory mapped to 0xFE00D000)
#define EC_RAM_WRITE ECRamWriteExt_Direct
#define EC_RAM_READ ECRamReadExt_Direct

//==================================================================================================

#include <conio.h>
#include <stdint.h>
#include <stdio.h>
#include <wchar.h>

#include <AclAPI.h>
#include <sddl.h>

#include <algorithm>
#include <array>
#include <cinttypes>
#include <vector>

bool CheckECVersion()
{
    // Designed for GKCN57WW. Modify these to proceed at your own risk.
    uint8_t EC_CHIP_ID1 = EC_RAM_READ(0x2000);
    uint8_t EC_CHIP_ID2 = EC_RAM_READ(0x2001);
    uint8_t EC_CHIP_VER = EC_RAM_READ(0x2002);
    printf("ITE-%02X%02X v%u\n", EC_CHIP_ID1, EC_CHIP_ID2, EC_CHIP_VER);

    // IT8227e-192
    if (EC_CHIP_ID1 != 0x82 || EC_CHIP_ID2 != 0x27 || EC_CHIP_VER != 2)
        return false;

    uint8_t EC_BIOS_VER_L = EC_RAM_READ(0xC2C7);
    printf("BIOS Version: %u\n", EC_BIOS_VER_L);

    if (EC_BIOS_VER_L != 57)
        return false;

    uint8_t EC_KeyBoardID = EC_RAM_READ(0xC2BA);
    printf("KeyBoard ID: %u\n", EC_KeyBoardID);

    if (EC_KeyBoardID != 4)
        return false;

    return true;
}

void CheckFanLevel(uint16_t addr, uint8_t index)
{
    static uint8_t FanLevels[3];
    static uint16_t FanLevelDurations[3];

    uint8_t level = EC_RAM_READ(addr);

    if (level != FanLevels[index]) {
        FanLevels[index] = level;
        FanLevelDurations[index] = 0;
    } else if (level > 0) {
        if (++FanLevelDurations[index] == 120) {
            --level;
            EC_RAM_WRITE(addr, level);
            FanLevelDurations[index] = 0;
            // printf("Lowered Level%u to %u\n", index, level);
        }
    }
}

struct FanLevel {
    // RPM (/100) of the fans
    uint8_t RPM;

    // Speed at which the fans accelerate to the target RPM (lower is faster)
    uint8_t Accel;

    // Speed at which the fans decelerate to the target RPM (lower is faster)
    uint8_t Decel;

    // The max CPU temp before moving to the next level
    uint8_t CPU_Max;

    // The min CPU temp before moving to the prev level
    uint8_t CPU_Min;

    // The max GPU temp before moving to the next level
    uint8_t GPU_Max;

    // The min GPU temp before moving to the prev level
    uint8_t GPU_Min;

    // The max IC temp before moving to the next level
    uint8_t IC_Max;

    // The min IC temp before moving to the prev level
    uint8_t IC_Min;
};

void OnTick()
{
    // uint8_t EC_CPUT = EC_RAM_READ(0xC4B0);
    // uint8_t EC_CPUS = EC_RAM_READ(0xC4B1);
    // uint8_t EC_PCHS = EC_RAM_READ(0xC4B2);
    // uint8_t EC_GPUS = EC_RAM_READ(0xC4B3);
    // uint8_t EC_GPUT = EC_RAM_READ(0xC4B4);
    // uint8_t EC_DGDF = EC_RAM_READ(0xC4BA);
    //
    // printf("CPU Temp: %2u / %2u\n", EC_CPUT, EC_CPUS);
    // printf("GPU Temp: %2u / %2u (throttle=%u)\n", EC_GPUT, EC_GPUS, EC_DGDF);
    // printf("PCH Temp: %2u\n", EC_PCHS);

    uint8_t FAN_TEMP_CPU = EC_RAM_READ(0xC5E6);
    uint8_t FAN_TEMP_GPU = EC_RAM_READ(0xC5E7);
    uint8_t FAN_TEMP_SENSOR = EC_RAM_READ(0xC5E8);

    uint8_t FAN1_RPM = EC_RAM_READ(0xC406);
    uint8_t FAN2_RPM = EC_RAM_READ(0xC4FE);

    uint8_t FAN1_PWM = EC_RAM_READ(0x1807);
    uint8_t FAN2_PWM = EC_RAM_READ(0x1806);

    // Send_cmd_by_PM(0x46);
    // Send_data_by_PM(0x81);
    // uint8_t FAN1_RPM = Read_data_from_PM();

    // Send_cmd_by_PM(0x46);
    // Send_data_by_PM(0x82);
    // uint8_t FAN2_RPM = Read_data_from_PM();

    if (FAN1_RPM > 50)
        FAN1_RPM = 0;

    if (FAN2_RPM > 50)
        FAN2_RPM = 0;

    printf("Temps: CPU %2u, GPU %2u, IC %2u | Fans: %2u/%2u (%2u/%2u)\n", FAN_TEMP_CPU, FAN_TEMP_GPU, FAN_TEMP_SENSOR, FAN1_RPM, FAN2_RPM, FAN1_PWM, FAN2_PWM);

    // CheckFanLevel(0xC634, 0); // CPU
    // CheckFanLevel(0xC635, 1); // GPU
    // CheckFanLevel(0xC636, 2); // SENSOR
}

void SetManualFanRPM(BYTE rpm)
{
    Send_cmd_by_PM(0x46);
    Send_data_by_PM(rpm);
}

void ReleaseManualFanControl()
{
    Send_cmd_by_PM(0x46);
    Send_data_by_PM(0x84);
}

bool FixDriverSecurity()
{
    // Although starting the WINIO driver requires admin, using it does not.
    // To try and avoid giving any program access to physical memory,
    // limit the driver access to administrators.
    // This isn't the ideal solution (the driver itself should limit access),
    // but it's better than nothing.

    bool success = false;

    // [O]wner: [B]uiltin [A]dministrators
    // [G]roup: Local [SY]stem
    // [D]ACL: [A]llow [F]ILE_ALL_[A]CCESS for Local [SY]stem and [B]uiltin [A]dministrators
    PSECURITY_DESCRIPTOR pSD = NULL;
    if (ConvertStringSecurityDescriptorToSecurityDescriptorA("O:BAG:SYD:(A;;FA;;;SY)(A;;FA;;;BA)", SDDL_REVISION_1, &pSD, NULL)) {
        PSID pOwner = NULL;
        BOOL bOwnerDefault = FALSE;

        PSID pGroup = NULL;
        BOOL bGroupDefault = FALSE;

        PACL pDacl = NULL;
        BOOL bDaclPresent = FALSE;
        BOOL bDaclDefault = FALSE;

        if (GetSecurityDescriptorOwner(pSD, &pOwner, &bOwnerDefault) && GetSecurityDescriptorGroup(pSD, &pGroup, &bGroupDefault) && GetSecurityDescriptorDacl(pSD, &bDaclPresent, &pDacl, &bDaclDefault) && !bGroupDefault && !bOwnerDefault && bDaclPresent && !bDaclDefault) {
            char driver_name[] = "\\\\.\\WINIO";

            if (SetNamedSecurityInfoA(driver_name, SE_FILE_OBJECT,
                    OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION,
                    pOwner, pGroup, pDacl, NULL)
                == ERROR_SUCCESS) {
                success = true;
            } else {
                printf("Failed to set driver security info\n");
            }
        } else {
            printf("Failed to retrieve security descriptor info\n");
        }

        LocalFree(pSD);
    } else {
        printf("Failed to parse security descriptor\n");
    }

    return success;
}

bool ValidateFanProfile(const std::vector<FanLevel>& levels)
{
    if (levels.size() < 2) {
        printf("Too few fan levels");
        return false;
    }

    if (levels.size() > 13) {
        printf("Too many fan levels\n");
        return false;
    }

    for (size_t i = 0; i < levels.size(); ++i) {
        const auto& level = levels[i];

        if (level.RPM > 0 && level.RPM < 5) {
            printf("Fan RPM too low\n");
            return false;
        }

        if (level.RPM > 45) {
            printf("Fan RPM too high\n");
            return false;
        }

        if (level.Accel < 2) {
            printf("Fan Accel too low (too fast)\n");
            return false;
        }

        if (level.Accel > 5) {
            printf("Fan Accel too high (too slow)\n");
            return false;
        }

        if (level.Decel < 2) {
            printf("Fan Decel too low (too fast)\n");
            return false;
        }

        if (level.Decel > 5) {
            printf("Fan Decel too high (too slow)\n");
            return false;
        }

        if ((std::max)({ level.CPU_Max, level.CPU_Min, level.GPU_Max, level.GPU_Min, level.IC_Max, level.IC_Min }) > 127) {
            printf("Temperature too high\n");
            return false;
        }

        if (level.CPU_Max < level.CPU_Min || level.GPU_Max < level.GPU_Min || level.IC_Max < level.IC_Min) {
            printf("Max Temperature lower than Min Temperature\n");
            return false;
        }

        if (i) {
            const auto& prev_level = levels[i - 1];

            if (level.CPU_Min > prev_level.CPU_Max || level.GPU_Min > prev_level.GPU_Max || level.IC_Min > prev_level.IC_Max) {
                printf("Min Temperature higher than previous Max Temperature\n");
                return false;
            }

            if (level.CPU_Max < prev_level.CPU_Max || level.GPU_Max < prev_level.GPU_Max || level.IC_Max < prev_level.IC_Max) {
                printf("Max Temperature in incorrect order");
                return false;
            }

            if (level.CPU_Min < prev_level.CPU_Min || level.GPU_Min < prev_level.GPU_Min || level.IC_Min < prev_level.IC_Min) {
                printf("Min Temperature in incorrect order");
                return false;
            }
        }
    }

    {
        const auto& first_level = levels.front();

        if (first_level.RPM != 0) {
            printf("First level RPM must be zero");
            return false;
        }

        if (first_level.CPU_Min != 0 || first_level.GPU_Min != 0 || first_level.IC_Min != 0) {
            printf("First level off must be zero");
            return false;
        }
    }

    {
        const auto& last_level = levels.back();

        if (last_level.CPU_Max != 127 || last_level.GPU_Max != 127 || last_level.IC_Max != 127) {
            printf("Last level on must be 127");
            return false;
        }
    }

    return true;
}

void WriteFanProfile(const std::vector<FanLevel>& levels)
{
    // Reset fan update counters (try to avoid any race conditions)
    EC_RAM_WRITE(0xC5FE, 0);
    EC_RAM_WRITE(0xC5FF, 0);

    uint8_t num_levels = (uint8_t)levels.size();

    for (uint8_t i = 0; i < num_levels; ++i) {
        const auto& level = levels[i];

        EC_RAM_WRITE(0xC540 + i, level.RPM); // Fan1 RPM
        EC_RAM_WRITE(0xC550 + i, level.RPM); // Fan2 RPM

        EC_RAM_WRITE(0xC560 + i, level.Accel); // Fan Acceleration
        EC_RAM_WRITE(0xC570 + i, level.Decel); // Fan Deceleration

        EC_RAM_WRITE(0xC580 + i, level.CPU_Max); // CPU Temp Max
        EC_RAM_WRITE(0xC590 + i, level.CPU_Min); // CPU Temp Min

        EC_RAM_WRITE(0xC5A0 + i, level.GPU_Max); // GPU Temp Max
        EC_RAM_WRITE(0xC5B0 + i, level.GPU_Min); // GPU Temp Min

        EC_RAM_WRITE(0xC5C0 + i, level.IC_Max); // IC Temp Max
        EC_RAM_WRITE(0xC5D0 + i, level.IC_Min); // IC Temp Min
    }

    // Reset current fan level
    EC_RAM_WRITE(0xC534, 0);

    // Set the number of fan levels
    EC_RAM_WRITE(0xC535, num_levels);

    // Reset internal fan levels
    EC_RAM_WRITE(0xC634, 0); // CPU
    EC_RAM_WRITE(0xC635, 0); // GPU
    EC_RAM_WRITE(0xC636, 0); // SENSOR

    printf("Wrote custom fan curves\n");
}

void LockFanProfile()
{
    // Disable the EC updating the fan profile.
    // This will save the current profile indefinitely, including between restarts.
    EC_RAM_WRITE(0xC5F4, EC_RAM_READ(0xC5F4) | 1);

    printf("Locked fan profile\n");
}

void UnlockFanProfile()
{
    // Enable the EC updating the fan profile
    EC_RAM_WRITE(0xC5F4, EC_RAM_READ(0xC5F4) & ~1);

    printf("Unlocked fan profile\n");
}

bool ReadFanProfile(FILE* input, std::vector<FanLevel>& levels)
{
    levels.clear();

    while (!feof(input)) {
        FanLevel level {};

        if (fscanf_s(input, "%" SCNu8 " %" SCNu8 " %" SCNu8 " %" SCNu8 " %" SCNu8 " %" SCNu8 " %" SCNu8 " %" SCNu8 " %" SCNu8 "\n",
                &level.RPM, &level.Accel, &level.Decel, &level.CPU_Max, &level.CPU_Min, &level.GPU_Max, &level.GPU_Min, &level.IC_Max, &level.IC_Min)
            != 9) {
            printf("Failed to read fan level\n");
            return false;
        }

        levels.push_back(level);
    }

    if (levels.size() < 2 || levels.size() > 13) {
        printf("Invalid fan profile info\n");
        return false;
    }

    if (!ValidateFanProfile(levels)) {
        printf("Invalid fan levels\n");
        return false;
    }

    return true;
}

bool LoadFanProfile(const char* path, std::vector<FanLevel>& profile)
{
    FILE* input = NULL;

    if (fopen_s(&input, path, "r")) {
        printf("Failed to open %s\n", path);
        return false;
    }

    bool success = ReadFanProfile(input, profile);

    fclose(input);
    return success;
}

bool Initialize()
{
    if (!InitializeWinIo()) {
        printf("Failed to initialize WinIO\n");
        return false;
    }

    if (!FixDriverSecurity()) {
        printf("Failed to fix driver security\n");
        return false;
    }

    if (!CheckECVersion()) {
        printf("Unknown EC Version\n");
        return false;
    }

    return true;
}

void Shutdown()
{
    ShutdownWinIo();
}

int MainLoop()
{
    bool running = true;

    while (true) {
        while (_kbhit()) {
            int btn = _getch();

            printf("Button: %i (%i)\n", btn, btn);

            switch (btn) {
            case '1':
                SetManualFanRPM(16);
                break;

            case '2':
                SetManualFanRPM(30);
                break;

            case '3':
                SetManualFanRPM(40);
                break;

            case 'w':
                ReleaseManualFanControl();
                break;

            case 'r':
                LockFanProfile();
                break;

            case 't':
                UnlockFanProfile();
                break;

            case '\x1B':
                running = false;
                break;
            }
        }

        if (!running)
            break;

        OnTick();

        Sleep(1000);
    }

    return 0;
}

int main(int argc, char* argv[])
{
    int result = 0;

    if (Initialize()) {
        if (argc == 1) {
            result = MainLoop();
        } else {
            for (int i = 1; i < argc;) {
                const char* arg = argv[i++];

                if (!strcmp(arg, "LockProfile")) {
                    LockFanProfile();
                } else if (!strcmp(arg, "UnlockProfile")) {
                    UnlockFanProfile();
                } else if (!strcmp(arg, "SetProfile") && (i < argc)) {
                    const char* path = argv[i++];
                    std::vector<FanLevel> profile;

                    if (LoadFanProfile(path, profile)) {
                        WriteFanProfile(profile);
                        printf("Applied profile\n");
                    } else {
                        printf("Failed to load profile\n");
                    }
                } else {
                    printf("Invalid command: '%s'\n", arg);
                }
            }
        }
    }

    Shutdown();

    return result;
}