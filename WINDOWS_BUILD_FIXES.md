# Windows Build Fixes

Этот документ описывает исправления, внесенные для решения проблем сборки проекта на Windows.

## 🐛 Основные проблемы и их решения

### 1. Проблема с шаблонами и лямбда-захватами в MSVC

**Проблема**: MSVC имеет проблемы с выводом типов для сложных шаблонов, особенно с `std::shared_ptr<endstone::SchedulerTask>`.

**Решение**: Заменены конкретные типы на `std::shared_ptr<void>` для избежания проблем с шаблонами.

**Файлы**: 
- `include/player_manager.h` (строки 27-28)
- `src/player_manager.cpp` (строки 149-186)

### 2. Проблемы с обработкой исключений в MSVC

**Проблема**: Лямбда-функции с исключениями могут вызывать проблемы компиляции в MSVC.

**Решение**: Добавлены try-catch блоки вокруг вызовов scheduler и безопасная обработка ошибок.

**Файлы**: `src/player_manager.cpp` (строки 158-185)

### 3. Проблемы с линковкой filesystem

**Проблема**: Разные версии MSVC требуют разной конфигурации для `std::filesystem`.

**Решение**: Добавлена условная компиляция для разных версий MSVC:

```cmake
if(WIN32)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_WIN32_WINNT=0x0601")
    if(MSVC_VERSION LESS 1910)
        set(CMAKE_CXX_STANDARD_LIBRARIES "${CMAKE_CXX_STANDARD_LIBRARIES};stdc++fs")
    endif()
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
    add_definitions(-DNOMINMAX)
endif()
```

**Файлы**: `CMakeLists.txt` (строки 8-19, 45-52)

### 4. Локализация сообщений

**Проблема**: Некоторые сообщения оставались на английском языке.

**Решение**: Все сообщения переведены на русский язык для единообразия.

**Файлы**: `src/account_manager.cpp` (строки 178-202)

## 🔧 Технические детали исправлений

### Изменения в PlayerData

```cpp
// Было:
std::shared_ptr<endstone::SchedulerTask> kickTask;
std::shared_ptr<endstone::SchedulerTask> reminderTask;

// Стало:
std::shared_ptr<void> kickTask;  // Using void* to avoid template issues
std::shared_ptr<void> reminderTask;  // Using void* to avoid template issues
```

### Безопасная обработка scheduler

```cpp
// Было:
data.kickTask = pl->getServer().getScheduler().runTaskLater(...);

// Стало:
try {
    auto kickTaskId = pl->getServer().getScheduler().runTaskLater(...);
    data.kickTask = std::make_shared<decltype(kickTaskId)>(kickTaskId);
} catch (...) {
    data.kickTask = nullptr;
}
```

### Условная компиляция для Windows

```cmake
if(WIN32)
    # Enable filesystem linking for Windows
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_WIN32_WINNT=0x0601")
    # Add filesystem library for MSVC 2017 and earlier
    if(MSVC_VERSION LESS 1910)
        set(CMAKE_CXX_STANDARD_LIBRARIES "${CMAKE_CXX_STANDARD_LIBRARIES};stdc++fs")
    endif()
    # Disable warnings for Windows
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
    add_definitions(-DNOMINMAX)
endif()
```

## 📋 Список измененных файлов

1. **CMakeLists.txt**
   - Добавлена условная компиляция для Windows
   - Исправлена линковка filesystem для разных версий MSVC
   - Добавлены определения для отключения предупреждений

2. **include/player_manager.h**
   - Изменены типы задач scheduler на `std::shared_ptr<void>`
   - Добавлены комментарии об избежании проблем с шаблонами

3. **src/player_manager.cpp**
   - Добавлена безопасная обработка scheduler с try-catch
   - Исправлена инициализация задач
   - Добавлена безопасная отмена задач

4. **src/account_manager.cpp**
   - Переведены оставшиеся сообщения на русский язык
   - Унифицированы все сообщения плагина

## 🚀 Инструкции по сборке для Windows

### Требования
- CMake 3.15 или выше
- Visual Studio 2019 или 2022
- Git для клонирования репозитория

### Сборка с помощью Visual Studio

```cmd
mkdir build
cd build
cmake .. -G "Visual Studio 16 2019" -A x64
cmake --build . --config Release
```

### Сборка с помощью vcpkg

```cmd
vcpkg install nlohmann-json:x64-windows
mkdir build
cd build
cmake .. -G "Visual Studio 16 2019" -A x64 -DCMAKE_TOOLCHAIN_FILE=path/to/vcpkg/scripts/buildsystems/vcpkg.cmake
cmake --build . --config Release
```

## 🧪 Тестирование

После сборки плагина:

1. Скопируйте `player_register.dll` в папку `plugins` вашего Endstone сервера
2. Запустите сервер и проверьте логи на наличие ошибок
3. Протестируйте основные функции:
   - `/register <ник> <пароль> <подтверждение>`
   - `/login <ник> <пароль>`
   - `/changepassword <старый> <новый> <подтверждение>`
   - `/account info`
   - `/logout`

## 🔍 Возможные оставшиеся проблемы

### 1. Проблемы с Endstone API

Если возникают ошибки, связанные с Endstone API:
- Проверьте версию Endstone (требуется v0.10)
- Убедитесь, что все зависимости установлены
- Проверьте совместимость версий

### 2. Проблемы с runtime

Если возникают ошибки во время выполнения:
- Убедитесь, что установлена правильная версия Visual C++ Redistributable
- Проверьте, что все зависимости доступны
- Проверьте логи сервера на предмет конкретных ошибок

### 3. Проблемы с путями

Если возникают проблемы с путями к файлам:
- Убедитесь, что у сервера есть права на запись в папку
- Проверьте, что пути не содержат специальных символов
- Проверьте, что filesystem поддерживается на вашей системе

## 📞 Поддержка

Если проблемы сохраняются:

1. Проверьте логи сборки на предмет конкретных ошибок
2. Убедитесь, что у вас установлена последняя версия Visual Studio
3. Проверьте, что все зависимости установлены корректно
4. Создайте issue на GitHub с подробным описанием проблемы

## 🔄 История изменений

- **v1.4.0**: Первоначальная реализация системы регистрации
- **v1.4.1**: Исправления совместимости с Windows
  - Замена типов scheduler для избежания проблем с шаблонами
  - Добавление условной компиляции для разных версий MSVC
  - Улучшение обработки ошибок
  - Локализация всех сообщений

---

**Примечание**: Эти исправления должны решить большинство проблем сборки на Windows, однако в зависимости от конкретной конфигурации системы могут потребоваться дополнительные настройки.