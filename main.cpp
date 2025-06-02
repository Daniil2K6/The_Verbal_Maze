#include <windows.h>
#include <string>
#include <vector>
#include <ctime>
#include <cstdlib>
#include <algorithm>

#define FIELD_SIZE 5
#define WORDS_COUNT 3

// Список слов для игры (можно расширить)
const char* WORDS[] = { "КОТ", "ДОМ", "ЛЕС", "МОРЕ", "САД", "НОС", "СОН", "ЛУК", "РОТ", "СУД" };
const int WORDS_TOTAL = sizeof(WORDS) / sizeof(WORDS[0]);

TCHAR field[FIELD_SIZE][FIELD_SIZE + 1]; // +1 для '\0'
std::vector<std::string> hiddenWords;
HWND hEdits[WORDS_COUNT];
HWND hButton;
HWND hStaticResult;

void GenerateField() {
    // Заполняем поле случайными буквами
    for (int i = 0; i < FIELD_SIZE; ++i) {
        for (int j = 0; j < FIELD_SIZE; ++j) {
            field[i][j] = L'А' + rand() % 32; // Русские буквы А-Я (без Ё)
        }
        field[i][FIELD_SIZE] = 0;
    }
    // Выбираем случайные слова и размещаем их
    hiddenWords.clear();
    std::vector<int> used;
    while (hiddenWords.size() < WORDS_COUNT) {
        int idx = rand() % WORDS_TOTAL;
        if (std::find(used.begin(), used.end(), idx) == used.end()) {
            hiddenWords.push_back(WORDS[idx]);
            used.push_back(idx);
        }
    }
    // Вставляем слова в поле (по горизонтали или вертикали)
    for (int w = 0; w < WORDS_COUNT; ++w) {
        std::string word = hiddenWords[w];
        int len = word.length();
        bool placed = false;
        for (int attempt = 0; attempt < 100 && !placed; ++attempt) {
            int dir = rand() % 2; // 0 - горизонталь, 1 - вертикаль
            int x = rand() % FIELD_SIZE;
            int y = rand() % FIELD_SIZE;
            if (dir == 0 && y + len <= FIELD_SIZE) { // горизонталь
                for (int i = 0; i < len; ++i)
                    field[x][y + i] = word[i];
                placed = true;
            } else if (dir == 1 && x + len <= FIELD_SIZE) { // вертикаль
                for (int i = 0; i < len; ++i)
                    field[x + i][y] = word[i];
                placed = true;
            }
        }
    }
}

void DrawField(HDC hdc, RECT& rc) {
    int cellSize = std::min((rc.right - rc.left) / FIELD_SIZE, (rc.bottom - rc.top) / FIELD_SIZE);
    HFONT hFont = CreateFontW(cellSize / 2, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, RUSSIAN_CHARSET,
        OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Arial");
    HFONT hOldFont = (HFONT)SelectObject(hdc, hFont);

    for (int i = 0; i < FIELD_SIZE; ++i) {
        for (int j = 0; j < FIELD_SIZE; ++j) {
            RECT cell = { rc.left + j * cellSize, rc.top + i * cellSize,
                          rc.left + (j + 1) * cellSize, rc.top + (i + 1) * cellSize };
            Rectangle(hdc, cell.left, cell.top, cell.right, cell.bottom);
            TCHAR ch[2] = { field[i][j], 0 };
            DrawText(hdc, ch, 1, &cell, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        }
    }
    SelectObject(hdc, hOldFont);
    DeleteObject(hFont);
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE: {
        srand((unsigned)time(0));
        GenerateField();
        // Создаем поля для ввода
        for (int i = 0; i < WORDS_COUNT; ++i) {
            hEdits[i] = CreateWindowW(L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
                20, 320 + i * 30, 120, 25, hwnd, NULL, NULL, NULL);
        }
        hButton = CreateWindowW(L"BUTTON", L"Проверить", WS_CHILD | WS_VISIBLE,
            160, 320, 100, 30, hwnd, (HMENU)1, NULL, NULL);
        hStaticResult = CreateWindowW(L"STATIC", L"", WS_CHILD | WS_VISIBLE,
            20, 420, 300, 30, hwnd, NULL, NULL, NULL);
        break;
    }
    case WM_COMMAND:
        if (LOWORD(wParam) == 1) { // Кнопка "Проверить"
            int correct = 0;
            for (int i = 0; i < WORDS_COUNT; ++i) {
                wchar_t buf[32];
                GetWindowTextW(hEdits[i], buf, 32);
                std::wstring ws(buf);
                std::string s(ws.begin(), ws.end());
                // Приводим к верхнему регистру
                std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) {
                    return (c >= L'а' && c <= L'я') ? c - 32 : c;
                });
                for (auto& c : s) if (c >= 'a' && c <= 'z') c -= 32; // латиница в верхний
                if (std::find(hiddenWords.begin(), hiddenWords.end(), s) != hiddenWords.end())
                    ++correct;
            }
            wchar_t res[128];
            if (correct == WORDS_COUNT)
                wsprintf(res, L"Поздравляем! Все слова найдены!");
            else
                wsprintf(res, L"Найдено %d из %d слов.", correct, WORDS_COUNT);
            SetWindowText(hStaticResult, res);
        }
        break;
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        RECT rc;
        GetClientRect(hwnd, &rc);
        rc.left += 20; rc.top += 20; rc.right -= 20; rc.bottom = 300;
        DrawField(hdc, rc);
        EndPaint(hwnd, &ps);
        break;
    }
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR, int nCmdShow) {
    const wchar_t CLASS_NAME[] = L"WordMazeWindow";
    WNDCLASSW wc = {};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);

    RegisterClassW(&wc);

    HWND hwnd = CreateWindowExW(
        0, CLASS_NAME, L"Словесный лабиринт",
        WS_OVERLAPPEDWINDOW ^ WS_THICKFRAME,
        CW_USEDEFAULT, CW_USEDEFAULT, 400, 500,
        NULL, NULL, hInstance, NULL
    );

    if (hwnd == NULL) return 0;

    ShowWindow(hwnd, nCmdShow);

    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return 0;
}
