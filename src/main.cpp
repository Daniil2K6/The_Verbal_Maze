#include <windows.h>
#include <string>
#include <vector>
#include <ctime>
#include <cstdlib>
#include <algorithm>
#include <fstream>
#include <locale>
#include <codecvt>

#define FIELD_SIZE 7
#define WORDS_COUNT 3

// Пути к файлам
const wchar_t* WORDS_FILE = L"data/words.txt";
const wchar_t* POINTS_FILE = L"data/point.txt";

std::vector<std::wstring> allWords;
int playerPoints = 0; // Текущие очки игрока

wchar_t field[FIELD_SIZE][FIELD_SIZE + 1]; // +1 для '\0'
std::vector<std::wstring> hiddenWords;
std::vector<std::vector<bool>> revealedLetters; // Массив для хранения открытых букв
std::vector<bool> wordFound; // Массив для отслеживания найденных слов
HWND hEdits[WORDS_COUNT];
HWND hHintButtons[WORDS_COUNT]; // Массив кнопок подсказок
HWND hButton;
HWND hStaticResult;
HWND hStaticHint;
HWND hStaticPoints;

// Функция для загрузки очков из файла
void LoadPoints() {
    std::ifstream fin(POINTS_FILE);
    if (fin) {
        fin >> playerPoints;
        fin.close();
    } else {
        playerPoints = 0;
    }
}

// Функция для сохранения очков в файл
void SavePoints() {
    std::ofstream fout(POINTS_FILE);
    if (fout) {
        fout << playerPoints;
        fout.close();
    }
}

// Функция для обновления отображения очков
void UpdatePointsDisplay(HWND hwnd) {
    // Уничтожаем старый статик-контрол
    if (hStaticPoints != NULL) {
        DestroyWindow(hStaticPoints);
    }
    
    // Создаем новый статик-контрол
    hStaticPoints = CreateWindowW(L"STATIC", L"", WS_CHILD | WS_VISIBLE,
        280, 420, 100, 30, hwnd, NULL, NULL, NULL);
        
    // Обновляем текст
    wchar_t pointsText[64];
    swprintf(pointsText, 64, L"Очки: %d", playerPoints);
    SetWindowTextW(hStaticPoints, pointsText);
}

void ToUpper(std::wstring& s) {
    for (auto& c : s) {
        if (c >= L'а' && c <= L'я') c -= 32;
        if (c >= L'a' && c <= L'z') c -= 32;
        if (c == L'ё') c = L'Ё';
    }
}

void LoadWordsFromFile(const wchar_t* filename) {
    allWords.clear();
    std::ifstream fin(filename);
    if (!fin) {
        MessageBoxW(NULL, L"Не удалось открыть файл words.txt", L"Ошибка", MB_ICONERROR);
        exit(1);
    }
    std::string line;
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> conv;
    while (std::getline(fin, line)) {
        std::wstring word = conv.from_bytes(line);
        ToUpper(word);
        word.erase(std::remove_if(word.begin(), word.end(), [](wchar_t c) { return c == L'\r' || c == L'\n' || c == L' '; }), word.end());
        if (word.length() == 5 || word.length() == 6) {
            allWords.push_back(word);
        }
    }
    if (allWords.empty()) {
        MessageBoxW(NULL, L"В файле words.txt нет слов длиной 5-6 букв!", L"Ошибка", MB_ICONERROR);
        exit(1);
    }
}

void GenerateField() {
    try {
        // Загружаем слова из файла
        LoadWordsFromFile(WORDS_FILE);
        
        // Заполняем поле случайными буквами
        for (int i = 0; i < FIELD_SIZE; ++i) {
            for (int j = 0; j < FIELD_SIZE; ++j) {
                field[i][j] = L'А' + rand() % 32; // Русские буквы А-Я (без Ё)
            }
            field[i][FIELD_SIZE] = 0;
        }
        
        // Маска занятых клеток
        bool used[FIELD_SIZE][FIELD_SIZE] = {};
        // Выбираем 3 случайных уникальных индекса
        hiddenWords.clear();
        revealedLetters.clear(); // Очищаем массив открытых букв
        std::vector<int> indices;
        while (indices.size() < WORDS_COUNT) {
            int idx = rand() % allWords.size();
            if (std::find(indices.begin(), indices.end(), idx) == indices.end()) {
                indices.push_back(idx);
            }
        }
        
        for (int i = 0; i < WORDS_COUNT; ++i) {
            hiddenWords.push_back(allWords[indices[i]]);
            revealedLetters.push_back(std::vector<bool>(hiddenWords[i].length(), false));
        }
        
        // Вставляем слова в поле (по горизонтали или вертикали)
        bool allPlaced = true;
        for (int w = 0; w < WORDS_COUNT; ++w) {
            std::wstring word = hiddenWords[w];
            int len = word.length();
            bool placed = false;
            for (int attempt = 0; attempt < 10000 && !placed; ++attempt) {
                int dir = rand() % 2; // 0 - горизонталь, 1 - вертикаль
                int x = rand() % FIELD_SIZE;
                int y = rand() % FIELD_SIZE;
                bool can_place = true;
                if (dir == 0 && y + len <= FIELD_SIZE) { // горизонталь
                    for (int i = 0; i < len; ++i) {
                        wchar_t cell = field[x][y + i];
                        if (used[x][y + i] && cell != word[i]) { // если уже занято и буква не совпадает
                            can_place = false;
                            break;
                        }
                    }
                    if (can_place) {
                        for (int i = 0; i < len; ++i) {
                            field[x][y + i] = word[i];
                            used[x][y + i] = true;
                        }
                        placed = true;
                    }
                } else if (dir == 1 && x + len <= FIELD_SIZE) { // вертикаль
                    for (int i = 0; i < len; ++i) {
                        wchar_t cell = field[x + i][y];
                        if (used[x + i][y] && cell != word[i]) {
                            can_place = false;
                            break;
                        }
                    }
                    if (can_place) {
                        for (int i = 0; i < len; ++i) {
                            field[x + i][y] = word[i];
                            used[x + i][y] = true;
                        }
                        placed = true;
                    }
                }
            }
            if (!placed) {
                allPlaced = false;
                GenerateField(); // Пробуем сгенерировать поле заново
                return;
            }
        }
    } catch (...) {
        exit(1);
    }
}

void DrawField(HDC hdc, RECT& rc) {
    int cellSize = std::min((rc.right - rc.left) / FIELD_SIZE, (rc.bottom - rc.top) / FIELD_SIZE);
    
    // Создаем кисть для фона игрового поля
    HBRUSH hBackgroundBrush = CreateSolidBrush(RGB(240, 240, 255));
    RECT fieldRect = {rc.left - 10, rc.top - 10, 
                     rc.left + cellSize * FIELD_SIZE + 10, 
                     rc.top + cellSize * FIELD_SIZE + 10};
    FillRect(hdc, &fieldRect, hBackgroundBrush);
    DeleteObject(hBackgroundBrush);

    // Рисуем рамку вокруг игрового поля
    HPEN hBorderPen = CreatePen(PS_SOLID, 2, RGB(100, 100, 150));
    HPEN hOldPen = (HPEN)SelectObject(hdc, hBorderPen);
    Rectangle(hdc, fieldRect.left, fieldRect.top, fieldRect.right, fieldRect.bottom);
    SelectObject(hdc, hOldPen);
    DeleteObject(hBorderPen);

    HFONT hFont = CreateFontW(cellSize / 2, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, RUSSIAN_CHARSET,
        OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Arial");
    HFONT hOldFont = (HFONT)SelectObject(hdc, hFont);

    // Создаем кисть для ячеек
    HBRUSH hCellBrush = CreateSolidBrush(RGB(255, 255, 255));
    HPEN hCellPen = CreatePen(PS_SOLID, 1, RGB(150, 150, 200));
    SelectObject(hdc, hCellPen);

    for (int i = 0; i < FIELD_SIZE; ++i) {
        for (int j = 0; j < FIELD_SIZE; ++j) {
            RECT cell = { rc.left + j * cellSize, rc.top + i * cellSize,
                         rc.left + (j + 1) * cellSize, rc.top + (i + 1) * cellSize };
            
            // Заполняем ячейку белым цветом
            FillRect(hdc, &cell, hCellBrush);
            // Рисуем границы ячейки
            Rectangle(hdc, cell.left, cell.top, cell.right, cell.bottom);
            
            // Рисуем букву
            wchar_t ch[2] = { field[i][j], 0 };
            SetBkMode(hdc, TRANSPARENT);
            DrawTextW(hdc, ch, 1, &cell, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        }
    }

    SelectObject(hdc, hOldFont);
    DeleteObject(hFont);
    DeleteObject(hCellBrush);
    DeleteObject(hCellPen);
}

void UpdateHint() {
    std::wstring hint;
    for (size_t i = 0; i < hiddenWords.size(); ++i) {
        if (i > 0) hint += L"\n\n\n";  // Добавляем больше пространства между словами
        hint += L"Слово " + std::to_wstring(i + 1) + L": ";
        for (size_t j = 0; j < hiddenWords[i].length(); ++j) {
            if (revealedLetters[i][j] || wordFound[i]) {
                hint += hiddenWords[i][j];
            } else {
                hint += L'_';
            }
            hint += L' ';
        }
    }
    SetWindowTextW(hStaticHint, hint.c_str());
}

// Функция для проверки введенного слова
bool CheckWord(const std::wstring& word, size_t& foundIndex) {
    for (size_t i = 0; i < hiddenWords.size(); ++i) {
        if (hiddenWords[i] == word) {
            foundIndex = i;
            return true;
        }
    }
    return false;
}

// Функция для рисования цветочка
void DrawFlower(HDC hdc, int x, int y, COLORREF color) {
    // Проверяем координаты
    if (x < 0 || y < 0) return;

    HBRUSH hFlowerBrush = CreateSolidBrush(color);
    HPEN hPen = CreatePen(PS_SOLID, 1, RGB(std::min<int>(GetRValue(color) + 40, 255), 
                                          std::min<int>(GetGValue(color) + 40, 255), 
                                          std::min<int>(GetBValue(color) + 40, 255)));
    
    HBRUSH hOldBrush = (HBRUSH)SelectObject(hdc, hFlowerBrush);
    HPEN hOldPen = (HPEN)SelectObject(hdc, hPen);

    // Рисуем лепестки
    const int petalSize = 6; // Уменьшаем размер цветочков
    
    // Рисуем лепестки крестом
    Ellipse(hdc, x - petalSize, y - petalSize/2, x + petalSize, y + petalSize/2); // горизонтальный
    Ellipse(hdc, x - petalSize/2, y - petalSize, x + petalSize/2, y + petalSize); // вертикальный

    // Рисуем центр цветка
    HBRUSH hCenterBrush = CreateSolidBrush(RGB(std::min<int>(GetRValue(color) + 60, 255),
                                               std::min<int>(GetGValue(color) + 60, 255),
                                               std::min<int>(GetBValue(color) + 60, 255)));
    SelectObject(hdc, hCenterBrush);
    Ellipse(hdc, x - petalSize/2, y - petalSize/2, x + petalSize/2, y + petalSize/2);

    // Восстанавливаем контекст и очищаем ресурсы
    SelectObject(hdc, hOldBrush);
    SelectObject(hdc, hOldPen);
    DeleteObject(hFlowerBrush);
    DeleteObject(hCenterBrush);
    DeleteObject(hPen);
}

// Функция для рисования декоративного фона
void DrawDecorativeBackground(HDC hdc, RECT rect) {
    // Создаем массив пастельных цветов
    COLORREF colors[] = {
        RGB(255, 182, 193),  // Светло-розовый
        RGB(173, 216, 230),  // Светло-голубой
        RGB(255, 255, 180)   // Светло-желтый
    };

    // Рисуем цветочки с большим интервалом
    for (int i = rect.left + 15; i < rect.right; i += 35) {
        for (int j = rect.top + 15; j < rect.bottom; j += 35) {
            // Небольшое случайное смещение
            int offsetX = rand() % 6 - 3;
            int offsetY = rand() % 6 - 3;
            
            // Выбираем случайный цвет из массива
            COLORREF color = colors[rand() % 3];
            
            DrawFlower(hdc, i + offsetX, j + offsetY, color);
        }
    }
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    static HBRUSH hYellowBrush = CreateSolidBrush(RGB(255, 255, 150));
    static HBRUSH hGreenBrush = CreateSolidBrush(RGB(0, 180, 0));
    static HBRUSH hLightBlueBrush = CreateSolidBrush(RGB(200, 230, 255));
    static WNDPROC oldButtonProc = NULL;

    switch (msg) {
    case WM_CREATE: {
        srand((unsigned)time(0));
        LoadPoints();
        GenerateField();
        
        // Создаем поля для ввода
        for (int i = 0; i < WORDS_COUNT; ++i) {
            hEdits[i] = CreateWindowW(L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
                20, 420 + i * 40, 120, 25, hwnd, NULL, NULL, NULL);
        }

        // Кнопка "Проверить" справа от полей ввода
        hButton = CreateWindowW(L"BUTTON", L"Проверить", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | BS_OWNERDRAW,
            160, 420, 100, 30, hwnd, (HMENU)1, NULL, NULL);

        // Счетчик очков рядом с кнопкой "Проверить"
        hStaticPoints = CreateWindowW(L"STATIC", L"", WS_CHILD | WS_VISIBLE,
            280, 420, 100, 30, hwnd, NULL, NULL, NULL);

        // Результат проверки
        hStaticResult = CreateWindowW(L"STATIC", L"", WS_CHILD | WS_VISIBLE,
            20, 540, 300, 30, hwnd, NULL, NULL, NULL);

        // Подсказки
        hStaticHint = CreateWindowW(L"STATIC", L"", WS_CHILD | WS_VISIBLE,
            20, 570, 150, 180, hwnd, NULL, NULL, NULL);

        // Кнопки подсказок справа от подсказок
        for (int i = 0; i < WORDS_COUNT; ++i) {
            hHintButtons[i] = CreateWindowW(L"BUTTON", L"Подсказка", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | BS_OWNERDRAW,
                180, 560 + i * 50, 80, 25, hwnd, (HMENU)(INT_PTR)(100 + i), NULL, NULL);
        }

        wordFound.resize(WORDS_COUNT, false);
        UpdateHint();
        UpdatePointsDisplay(hwnd);
        break;
    }
    case WM_COMMAND:
        if (LOWORD(wParam) == 1) { // Кнопка "Проверить"
            int correct = 0;
            for (int i = 0; i < WORDS_COUNT; ++i) {
                wchar_t buf[32];
                GetWindowTextW(hEdits[i], buf, 32);
                std::wstring ws(buf);
                // Приводим к верхнему регистру
                for (auto& c : ws) {
                    if (c >= L'а' && c <= L'я') c -= 32;
                    if (c >= L'a' && c <= L'z') c -= 32;
                    if (c == L'ё') c = L'Ё';
                }
                size_t foundIndex;
                if (CheckWord(ws, foundIndex)) {
                    ++correct;
                    wordFound[foundIndex] = true;
                }
            }
            wchar_t res[128];
            if (correct == WORDS_COUNT) {
                wsprintf(res, L"Поздравляем! Все слова найдены!");
                SetWindowTextW(hStaticResult, res);
                // Начисляем 5 очков за прохождение уровня
                playerPoints += 5;
                SavePoints();
                UpdatePointsDisplay(hwnd);
                // Генерируем новое поле и слова
                GenerateField();
                // Сбрасываем найденные слова
                wordFound.assign(WORDS_COUNT, false);
                // Обновляем подсказку
                UpdateHint();
                // Очищаем поля ввода
                for (int i = 0; i < WORDS_COUNT; ++i) {
                    SetWindowTextW(hEdits[i], L"");
                }
                // Перерисовываем поле
                InvalidateRect(hwnd, NULL, TRUE);
            } else {
                wsprintf(res, L"Найдено %d из %d слов.", correct, WORDS_COUNT);
                SetWindowTextW(hStaticResult, res);
            }
        } else if (LOWORD(wParam) >= 100 && LOWORD(wParam) < 100 + WORDS_COUNT) { // Кнопки подсказок
            int wordIndex = LOWORD(wParam) - 100;
            if (!wordFound[wordIndex]) { // Если слово еще не найдено
                // Находим первую неоткрытую букву в выбранном слове
                bool found = false;
                for (size_t j = 0; j < hiddenWords[wordIndex].length() && !found; ++j) {
                    if (!revealedLetters[wordIndex][j]) {
                        revealedLetters[wordIndex][j] = true;
                        found = true;
                        // Списываем очко только если у игрока есть очки
                        if (playerPoints > 0) {
                            playerPoints--;
                            SavePoints();
                            UpdatePointsDisplay(hwnd);
                        }
                    }
                }
                UpdateHint();
            }
        }
        break;
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        
        // Получаем размеры клиентской области
        RECT clientRect;
        GetClientRect(hwnd, &clientRect);
        
        // Определяем области для декоративного фона
        RECT leftArea = {0, 0, 20, clientRect.bottom};
        RECT rightArea = {clientRect.right - 20, 0, clientRect.right, clientRect.bottom};
        RECT bottomArea = {20, 400, clientRect.right - 20, clientRect.bottom};
        
        // Заполняем фоновым цветом
        HBRUSH hBackgroundBrush = CreateSolidBrush(RGB(240, 240, 255));
        FillRect(hdc, &leftArea, hBackgroundBrush);
        FillRect(hdc, &rightArea, hBackgroundBrush);
        FillRect(hdc, &bottomArea, hBackgroundBrush);
        DeleteObject(hBackgroundBrush);
        
        // Рисуем декоративный фон
        DrawDecorativeBackground(hdc, leftArea);
        DrawDecorativeBackground(hdc, rightArea);
        DrawDecorativeBackground(hdc, bottomArea);
        
        // Рисуем игровое поле
        RECT rc = {20, 20, clientRect.right - 20, 400};
        DrawField(hdc, rc);
        
        EndPaint(hwnd, &ps);
        break;
    }
    case WM_DRAWITEM: {
        LPDRAWITEMSTRUCT lpDIS = (LPDRAWITEMSTRUCT)lParam;
        if (lpDIS->hwndItem == hButton) {
            // Рисуем желтый фон кнопки "Проверить"
            FillRect(lpDIS->hDC, &lpDIS->rcItem, hYellowBrush);
            
            // Рисуем текст кнопки
            SetBkMode(lpDIS->hDC, TRANSPARENT);
            RECT textRect = lpDIS->rcItem;
            wchar_t buttonText[32];
            GetWindowTextW(lpDIS->hwndItem, buttonText, 32);
            DrawTextW(lpDIS->hDC, buttonText, -1, &textRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

            // Рисуем рамку кнопки
            if (lpDIS->itemState & ODS_SELECTED) {
                DrawEdge(lpDIS->hDC, &lpDIS->rcItem, EDGE_SUNKEN, BF_RECT);
            } else {
                DrawEdge(lpDIS->hDC, &lpDIS->rcItem, EDGE_RAISED, BF_RECT);
            }
            return TRUE;
        }
        // Обработка кнопок подсказок
        for (int i = 0; i < WORDS_COUNT; ++i) {
            if (lpDIS->hwndItem == hHintButtons[i]) {
                // Рисуем голубой фон кнопки подсказки
                FillRect(lpDIS->hDC, &lpDIS->rcItem, hLightBlueBrush);
                
                // Рисуем текст кнопки
                SetBkMode(lpDIS->hDC, TRANSPARENT);
                RECT textRect = lpDIS->rcItem;
                wchar_t buttonText[32];
                GetWindowTextW(lpDIS->hwndItem, buttonText, 32);
                DrawTextW(lpDIS->hDC, buttonText, -1, &textRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

                // Рисуем рамку кнопки
                if (lpDIS->itemState & ODS_SELECTED) {
                    DrawEdge(lpDIS->hDC, &lpDIS->rcItem, EDGE_SUNKEN, BF_RECT);
                } else {
                    DrawEdge(lpDIS->hDC, &lpDIS->rcItem, EDGE_RAISED, BF_RECT);
                }
                return TRUE;
            }
        }
        break;
    }
    case WM_CTLCOLORSTATIC: {
        HDC hdcStatic = (HDC)wParam;
        HWND hwndStatic = (HWND)lParam;
        if (hwndStatic == hStaticPoints) {
            SetTextColor(hdcStatic, RGB(0, 180, 0));
            SetBkMode(hdcStatic, TRANSPARENT);
            static HBRUSH hBrush = CreateSolidBrush(RGB(240, 240, 255));
            return (LRESULT)hBrush;
        }
        return DefWindowProcW(hwnd, msg, wParam, lParam);
    }
    case WM_DESTROY:
        DeleteObject(hYellowBrush);
        DeleteObject(hGreenBrush);
        DeleteObject(hLightBlueBrush);
        PostQuitMessage(0);
        SavePoints();
        break;
    default:
        return DefWindowProcW(hwnd, msg, wParam, lParam);
    }
    return 0;
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR, int nCmdShow) {
    try {
        const wchar_t CLASS_NAME[] = L"WordMazeWindow";
        WNDCLASSW wc = {};
        wc.lpfnWndProc = WndProc;
        wc.hInstance = hInstance;
        wc.lpszClassName = CLASS_NAME;
        wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
        
        // Загружаем большую иконку
        wc.hIcon = (HICON)LoadImage(
            hInstance,
            MAKEINTRESOURCE(1),
            IMAGE_ICON,
            64,  // Ширина
            64,  // Высота
            LR_DEFAULTCOLOR
        );

        RegisterClassW(&wc);

        HWND hwnd = CreateWindowExW(
            0, CLASS_NAME, L"Словесный лабиринт",
            WS_OVERLAPPEDWINDOW ^ WS_THICKFRAME,
            100, 100, 420, 750,
            NULL, NULL, hInstance, NULL
        );

        if (hwnd == NULL) return 0;

        ShowWindow(hwnd, nCmdShow);

        MSG msg = {};
        while (GetMessageW(&msg, NULL, 0, 0)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
        return 0;
    } catch (const std::exception& e) {
        std::wstring err = L"std::exception: ";
        std::string what_str = e.what();
        err += std::wstring(what_str.begin(), what_str.end());
        MessageBoxW(NULL, err.c_str(), L"Ошибка", MB_ICONERROR);
        return 1;
    } catch (...) {
        MessageBoxW(NULL, L"Неизвестная ошибка в wWinMain", L"Ошибка", MB_ICONERROR);
        return 1;
    }
}


