#include <windows.h>
#include <iostream>
#include <string>

// Структура ошибок — подстраивай под реальную из твоего .h файла
struct ERRSTRUCT {
    int     iErrCode;       // код ошибки
    char    szErrText[512]; // текст ошибки
    // добавь другие поля, если они есть в оригинале
};

int main() {
    // 1. Загружаем DLL
    HINSTANCE hDll = LoadLibrary(L"oledbio.dll");  // укажи точное имя своей DLL
    if (!hDll) {
        std::cerr << "Не удалось загрузить DLL. Ошибка: " << GetLastError() << std::endl;
        return 1;
    }

    // 2. Определяем тип функции
    typedef void(__stdcall* PInitializeOLEDBIO)(
        char* sAlias,
        char* sMode,
        char* sType,
        long lAsofDate,
        int iPrepareWhat,
        ERRSTRUCT* pzErr
        );

    // 3. Получаем адрес функции
    PInitializeOLEDBIO InitializeOLEDBIO =
        (PInitializeOLEDBIO)GetProcAddress(hDll, "InitializeOLEDBIO");

    if (!InitializeOLEDBIO) {
        std::cerr << "Не найдена функция InitializeOLEDBIO в DLL. Ошибка: "
            << GetLastError() << std::endl;
        FreeLibrary(hDll);
        return 1;
    }

    // 4. Подготавливаем параметры
    char sAlias[256] = "demo_Sales";          // алиас базы
    char sMode[256] = "";     // режим
    char sType[256] = "SQLSERVER";     // тип (или ACCESS, ORACLE и т.д.)
    long lAsofDate = 20251230;        // дата "as of", например 30 декабря 2025
    int  iPrepareWhat = 1;              // что подготавливать (зависит от твоей логики)

    ERRSTRUCT zErr = { 0 };
    strcpy_s(zErr.szErrText, sizeof(zErr.szErrText), "No error");

    // 5. Вызываем функцию
    std::cout << "Вызываем InitializeOLEDBIO..." << std::endl;
    InitializeOLEDBIO(sAlias, sMode, sType, lAsofDate, iPrepareWhat, &zErr);

    // 6. Проверяем результат по структуре ошибок
    if (zErr.iErrCode != 0) {
        std::cerr << "Ошибка инициализации: " << zErr.iErrCode
            << " - " << zErr.szErrText << std::endl;
    }
    else {
        std::cout << "InitializeOLEDBIO успешно выполнен!" << std::endl;
    }

    // 7. Освобождаем DLL
    FreeLibrary(hDll);

    std::cout << "Нажмите Enter для выхода...";
    std::cin.get();

    return 0;
}