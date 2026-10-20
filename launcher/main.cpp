#include <windows.h>
#include <commdlg.h>
#include <bcrypt.h>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>
#include <array>
#include "launch_args.h"

namespace fs = std::filesystem;
struct Game {
    const wchar_t* name;
    const wchar_t* folder;
    const wchar_t* hash;
    std::wstring rom;
    HWND field = nullptr;
    HWND status = nullptr;
};
Game games[] = {
    {L"Oracle of Ages", L"ages", L"880374fb978b18af4aa529e2e32f7ffb4d7dd2f4"},
    {L"Oracle of Seasons", L"seasons", L"ba1268290fb2b1b70505d2d7b5825fc8a4816a4b"}
};
fs::path exe_directory, settings;
HFONT normal_font = nullptr, title_font = nullptr;
HWND window_handle = nullptr;

void error(const std::wstring& message) {
    MessageBoxW(window_handle, message.c_str(), L"Oracle Launcher", MB_OK | MB_ICONERROR);
}

bool validate_rom(const fs::path& path, const wchar_t* expected, std::wstring& message) {
    std::ifstream input(path, std::ios::binary | std::ios::ate);
    if (!input) { message = L"Cannot open this ROM. Select an accessible file."; return false; }
    if (input.tellg() != std::streampos(1048576)) {
        message = L"This ROM must be exactly 1,048,576 bytes (1 MiB)."; return false;
    }
    input.seekg(0);
    std::vector<unsigned char> bytes(1048576);
    if (!input.read(reinterpret_cast<char*>(bytes.data()), bytes.size())) {
        message = L"Could not read the complete ROM."; return false;
    }
    BCRYPT_ALG_HANDLE algorithm = nullptr;
    BCRYPT_HASH_HANDLE hash = nullptr;
    DWORD object_size = 0, written = 0;
    bool ok = BCryptOpenAlgorithmProvider(&algorithm, BCRYPT_SHA1_ALGORITHM, nullptr, 0) >= 0;
    if (ok) ok = BCryptGetProperty(algorithm, BCRYPT_OBJECT_LENGTH,
        reinterpret_cast<PUCHAR>(&object_size), sizeof(object_size), &written, 0) >= 0;
    std::vector<unsigned char> object(object_size);
    std::array<unsigned char, 20> digest{};
    if (ok) ok = BCryptCreateHash(algorithm, &hash, object.data(), object_size, nullptr, 0, 0) >= 0;
    if (ok) ok = BCryptHashData(hash, bytes.data(), static_cast<ULONG>(bytes.size()), 0) >= 0;
    if (ok) ok = BCryptFinishHash(hash, digest.data(), static_cast<ULONG>(digest.size()), 0) >= 0;
    if (hash) BCryptDestroyHash(hash);
    if (algorithm) BCryptCloseAlgorithmProvider(algorithm, 0);
    if (!ok) { message = L"Windows could not verify this ROM's SHA-1."; return false; }
    const wchar_t* hex = L"0123456789abcdef";
    std::wstring actual;
    for (unsigned char byte : digest) { actual += hex[byte >> 4]; actual += hex[byte & 15]; }
    if (actual != expected) {
        message = L"Wrong game or ROM revision. Select the supported USA ROM for this game.";
        return false;
    }
    message = L"USA ROM verified. Ready to play.";
    return true;
}

fs::path game_executable(const Game& game) {
    const std::wstring filename = std::wstring(L"oracle-") + game.folder + L".exe";
    // Supports both the source build layout and a future portable folder layout.
    const fs::path roots[] = {exe_directory, exe_directory.parent_path().parent_path()};
    for (const auto& root : roots) {
        const fs::path candidates[] = {
            root / L"build" / L"bin" / game.folder / filename,
            root / L"bin" / game.folder / filename
        };
        for (const auto& candidate : candidates) {
            std::error_code ec;
            if (fs::is_regular_file(candidate, ec)) return candidate;
        }
    }
    return {};
}

void choose_rom(int index) {
    auto& game = games[index];
    std::vector<wchar_t> file(32768, 0);
    OPENFILENAMEW dialog{};
    dialog.lStructSize = sizeof(dialog);
    dialog.hwndOwner = window_handle;
    dialog.lpstrFilter = L"Game Boy Color ROM (*.gbc)\0*.gbc\0All files\0*.*\0";
    dialog.lpstrFile = file.data();
    dialog.nMaxFile = static_cast<DWORD>(file.size());
    dialog.lpstrTitle = game.name;
    dialog.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;
    if (!GetOpenFileNameW(&dialog)) return;
    std::wstring message;
    if (!validate_rom(fs::path(file.data()), game.hash, message)) { error(message); return; }
    game.rom = file.data();
    SetWindowTextW(game.field, game.rom.c_str());
    SetWindowTextW(game.status, message.c_str());
    if (!WritePrivateProfileStringW(L"ROMs", game.folder, game.rom.c_str(), settings.c_str()))
        error(L"The ROM is selected, but its location could not be saved. Check folder write access.");
}

void play(int index) {
    auto& game = games[index];
    if (game.rom.empty()) { choose_rom(index); if (game.rom.empty()) return; }
    std::wstring message;
    if (!validate_rom(fs::path(game.rom), game.hash, message)) {
        SetWindowTextW(game.status, L"ROM needs attention. Use Select ROM."); error(message); return;
    }
    fs::path executable = game_executable(game);
    if (executable.empty()) {
        error(std::wstring(L"Cannot find oracle-") + game.folder +
            L".exe. Keep this launcher in build-launcher\\bin and build the games in the same project's build folder.");
        return;
    }
    std::wstring command = quote_argument(executable.wstring()) + L" --rom " + quote_argument(game.rom);
    STARTUPINFOW startup{}; startup.cb = sizeof(startup);
    PROCESS_INFORMATION process{};
    const fs::path working_directory = executable.parent_path();
    if (!CreateProcessW(executable.c_str(), command.data(), nullptr, nullptr, FALSE, 0,
        nullptr, working_directory.c_str(), &startup, &process)) {
        error(L"Windows could not start the game (error " + std::to_wstring(GetLastError()) +
            L"). Run the launcher from MINGW64 while testing so game DLLs can be found.");
        return;
    }
    CloseHandle(process.hThread); CloseHandle(process.hProcess);
    SetWindowTextW(game.status, L"Game started. You can leave this launcher open.");
}

HWND control(const wchar_t* type, const wchar_t* text, DWORD style,
             int x, int y, int width, int height, int id = 0) {
    HWND child = CreateWindowExW(0, type, text, WS_CHILD | WS_VISIBLE | style,
        x, y, width, height, window_handle,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)), GetModuleHandleW(nullptr), nullptr);
    SendMessageW(child, WM_SETFONT, reinterpret_cast<WPARAM>(normal_font), TRUE);
    return child;
}

LRESULT CALLBACK window_proc(HWND hwnd, UINT event, WPARAM wparam, LPARAM lparam) {
    switch (event) {
    case WM_CREATE: {
        window_handle = hwnd;
        normal_font = CreateFontW(-16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, 0, 0, CLEARTYPE_QUALITY, 0, L"Segoe UI");
        title_font = CreateFontW(-25, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, 0, 0, CLEARTYPE_QUALITY, 0, L"Segoe UI");
        HWND title = control(L"STATIC", L"Oracle Recompiled", 0, 24, 20, 580, 38);
        SendMessageW(title, WM_SETFONT, reinterpret_cast<WPARAM>(title_font), TRUE);
        control(L"STATIC", L"Choose your own USA ROMs, then start your adventure.", 0, 24, 66, 600, 26);
        for (int index = 0; index < 2; ++index) {
            auto& game = games[index];
            int y = 110 + index * 164;
            control(L"BUTTON", game.name, BS_GROUPBOX, 20, y, 650, 146);
            game.field = control(L"EDIT", L"No ROM selected", WS_BORDER | ES_READONLY | ES_AUTOHSCROLL,
                36, y + 34, 490, 28);
            control(L"BUTTON", L"Select ROM...", BS_PUSHBUTTON | WS_TABSTOP, 538, y + 33, 114, 30, 100 + index);
            game.status = control(L"STATIC", L"Select the ROM for this game to begin.", 0, 36, y + 80, 480, 42);
            control(L"BUTTON", index == 0 ? L"Play Ages" : L"Play Seasons",
                BS_PUSHBUTTON | WS_TABSTOP, 538, y + 88, 114, 34, 200 + index);
            std::vector<wchar_t> value(32768, 0);
            GetPrivateProfileStringW(L"ROMs", game.folder, L"", value.data(),
                static_cast<DWORD>(value.size()), settings.c_str());
            if (value[0]) {
                game.rom = value.data();
                SetWindowTextW(game.field, game.rom.c_str());
                SetWindowTextW(game.status, L"ROM location remembered. Verified again when you play.");
            }
        }
        control(L"STATIC", L"F1 opens the game menu. Each game keeps its own saves.\nROM files stay where you selected them.",
            0, 24, 450, 630, 52);
        return 0;
    }
    case WM_COMMAND:
        if (HIWORD(wparam) == BN_CLICKED) {
            int id = LOWORD(wparam);
            if (id >= 100 && id <= 101) choose_rom(id - 100);
            if (id >= 200 && id <= 201) play(id - 200);
        }
        return 0;
    case WM_DESTROY:
        if (normal_font) DeleteObject(normal_font);
        if (title_font) DeleteObject(title_font);
        PostQuitMessage(0); return 0;
    }
    return DefWindowProcW(hwnd, event, wparam, lparam);
}

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int show) {
    SetProcessDPIAware();
    std::vector<wchar_t> path(32768, 0);
    DWORD count = GetModuleFileNameW(nullptr, path.data(), static_cast<DWORD>(path.size()));
    if (!count || count >= path.size()) return 1;
    exe_directory = fs::path(path.data()).parent_path();
    settings = exe_directory / L"oracle-launcher.ini";
    WNDCLASSW cls{};
    cls.lpfnWndProc = window_proc;
    cls.hInstance = instance;
    cls.lpszClassName = L"OracleRecompiledLauncher";
    cls.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    cls.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    if (!RegisterClassW(&cls)) return 1;
    RECT rectangle{0, 0, 690, 518};
    DWORD style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
    AdjustWindowRect(&rectangle, style, FALSE);
    HWND hwnd = CreateWindowExW(0, cls.lpszClassName, L"Oracle Recompiled Launcher", style,
        CW_USEDEFAULT, CW_USEDEFAULT, rectangle.right - rectangle.left, rectangle.bottom - rectangle.top,
        nullptr, nullptr, instance, nullptr);
    if (!hwnd) return 1;
    ShowWindow(hwnd, show);
    MSG message{};
    int result;
    while ((result = GetMessageW(&message, nullptr, 0, 0)) > 0) {
        if (!IsDialogMessageW(hwnd, &message)) { TranslateMessage(&message); DispatchMessageW(&message); }
    }
    return result < 0 ? 1 : static_cast<int>(message.wParam);
}
