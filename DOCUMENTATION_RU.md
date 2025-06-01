# Инструкция для разработчика: STM32F103C4 LED Matrix Control

## Требования к программному обеспечению для Windows 11

### 1. Основные инструменты разработки

#### Visual Studio Code
- **Скачать**: https://code.visualstudio.com/
- **Назначение**: Основная IDE для редактирования кода
- **Настройка**: После установки добавить рекомендуемые расширения (см. раздел "Расширения")

#### STM32CubeCLT (Command Line Tools)
- **Скачать**: https://www.st.com/en/development-tools/stm32cubeclt.html
- **Версия**: STM32CubeCLT v1.16.0 (или новее)
- **Назначение**: Полный набор инструментов разработки для STM32
- **Включает**:
  - ARM GNU Toolchain (компилятор и линкер)
  - CMake
  - Ninja build system
  - STM32CubeProgrammer
  - Отладочные утилиты
- **Установка**:
  1. Запустить установщик от имени администратора
  2. Выбрать полную установку (Full installation)
  3. Добавить в PATH автоматически выбирается при установке
  4. Проверить установку: `arm-none-eabi-gcc --version`

#### SEGGER J-Link Software
- **Скачать**: https://www.segger.com/downloads/jlink/
- **Версия**: JLink_Windows_V794e.exe (или новее)
- **Назначение**: Отладка и программирование через J-Link
- **Компоненты**:
  - J-Link Driver
  - J-Link Commander
  - J-Link GDB Server
  - J-Link SWO Viewer

#### Git для Windows (опционально)
- **Скачать**: https://git-scm.com/download/win
- **Назначение**: Контроль версий (нужен только для работы с Git репозиториями)
- **Настройка**:
  ```bash
  git config --global user.name "Your Name"
  git config --global user.email "your.email@domain.com"
  ```

### 2. Расширения VS Code

Установить следующие расширения через Marketplace:

```json
{
  "recommendations": [
    "ms-vscode.cpptools",
    "ms-vscode.cpptools-extension-pack",
    "ms-vscode.cmake-tools",
    "marus25.cortex-debug",
    "dan-c-underwood.arm",
    "ms-vscode.vscode-json",
    "stmicroelectronics.stm32cube-ide-core",
    "stmicroelectronics.stm32cube-ide-build-cmake",
    "stmicroelectronics.stm32cube-ide-debug-core",
    "stmicroelectronics.stm32cube-ide-clangd"
  ]
}
```

#### Основные расширения:
- **C/C++**: IntelliSense, отладка
- **CMake Tools**: Интеграция с CMake
- **Cortex-Debug**: Отладка ARM Cortex-M
- **ARM Assembly**: Подсветка синтаксиса ARM ассемблера

#### STM32Cube расширения:
- **STM32Cube for Visual Studio Code Core**: Ядро STM32Cube для VS Code
- **STM32Cube CMake Support**: Поддержка CMake для STM32Cube проектов
- **STM32Cube Debug Core**: Ядро отладки для STM32Cube
- **STM32Cube clangd**: C/C++ автодополнение и навигация с поддержкой clangd

### 3. Настройка рабочего окружения

#### Проверка установки
Все необходимые переменные окружения устанавливаются автоматически при установке STM32CubeCLT.
Выполнить в командной строке для проверки:
```bash
arm-none-eabi-gcc --version
cmake --version
STM32_Programmer_CLI --version
JLink.exe -?
```

#### Файлы для других сред разработки
В проекте присутствуют файлы для других IDE, но они не используются в данной конфигурации:
- `Led_Matrix_Control.ioc` - файл STM32CubeMX для генерации кода
- `Led_Matrix_Control Debug Segger.launch` - конфигурация для STM32CubeIDE

Для сборки в VS Code используется система CMake с готовыми задачами.

## Структура проекта

```
Led_Matrix_Control/
├── App/                          # Прикладной код
│   ├── Application.c/.h          # Основная логика приложения
│   ├── CAN_manager.c/.h         # Управление CAN интерфейсом
│   ├── LED_display.c/.h         # Управление LED дисплеем
│   ├── Symbols.c/.h             # Определения символов (8x8)
│   └── Symbols_Remaper.c/.h     # Переназначение символов
├── Core/                        # Системный код STM32
│   ├── Src/
│   │   ├── main.c               # Точка входа
│   │   ├── can.c                # Инициализация CAN
│   │   └── stm32f1xx_it.c      # Обработчики прерываний
│   └── Inc/
│       ├── main.h
│       ├── can.h
│       └── stm32f1xx_hal_conf.h # Конфигурация HAL
├── Drivers/                     # HAL драйверы STM32
├── Middlewares/                 # FreeRTOS
├── cmake/                       # Файлы сборки CMake
│   └── gcc-arm-none-eabi.cmake  # Конфигурация компилятора
├── CMakeLists.txt              # Основной файл CMake
└── .vscode/                    # Настройки VS Code
    ├── launch.json             # Конфигурация отладки
    ├── tasks.json              # Задачи сборки
    └── c_cpp_properties.json   # Настройки C++
```

## Сборка и отладка проекта

### 1. Открытие проекта
1. Запустить VS Code
2. File → Open Folder → Выбрать папку `Led_Matrix_Control`
3. VS Code предложит установить рекомендуемые расширения - согласиться

### 2. Конфигурация и сборка проекта

#### Сборка через CMake tasks (рекомендуемый способ)
1. Открыть Command Palette (Ctrl+Shift+P)
2. Выбрать `CMake: Show Pinned Commands`
3. Нажать `Run Task` и выбрать одну из задач:
   - **This project CMake Configure** - конфигурация проекта
   - **This project Build (Debug -Os -g2)** - сборка отладочной версии
   - **This project Build (Release -O2 -g0)** - сборка релизной версии

#### Альтернативный способ через терминал:
```bash
# Конфигурация Debug сборки
cmake -S . -B Debug -G Ninja -DCMAKE_BUILD_TYPE=Debug

# Сборка Debug версии
ninja -C Debug

# Конфигурация Release сборки
cmake -S . -B Release -G Ninja -DCMAKE_BUILD_TYPE=Release

# Сборка Release версии
ninja -C Release
```

### 3. Отладка через J-Link
1. Подключить J-Link к STM32F103C4
2. В VS Code: F5 или Run → Start Debugging
3. Конфигурация в `.vscode/launch.json`:
   ```json
   {
     "name": "Cortex Debug",
     "type": "cortex-debug",
     "request": "launch",
     "servertype": "jlink",
     "device": "STM32F103C8",
     "interface": "swd",
     "executable": "./build/Led_Matrix_Control.elf"
   }
   ```

## Изменение скорости CAN шины

Текущая конфигурация: **562.5 кбит/с** при тактовой частоте CAN_CLK = 36 МГц

### Расположение настроек CAN

#### 1. Аппаратная инициализация: `Core/Src/can.c`
```c
// Строки 38-45
hcan.Init.Prescaler = 4;                    // Предделитель
hcan.Init.SyncJumpWidth = CAN_SJW_4TQ;     // 4 кванта времени
hcan.Init.TimeSeg1 = CAN_BS1_11TQ;         // 11 квантов времени
hcan.Init.TimeSeg2 = CAN_BS2_4TQ;          // 4 кванта времени
```

#### 2. Документация параметров: `App/CAN_manager.c`
```c
// Строки 26-45 - подробные расчеты текущих параметров
/*
 * Параметры CAN для скорости ~555555 бит/с при CAN_CLK = 36 МГц:
 * Prescaler = 4, SJW = 4, TS1 = 11, TS2 = 4
 * Итого квантов на бит: 1 + 11 + 4 = 16 Tq
 * Скорость: 36MHz / (4 * 16) = 562500 бит/с
 */
```

### Расчет новых параметров

Формула расчета скорости CAN:
```
Bitrate = CAN_CLK / (Prescaler × (1 + TimeSeg1 + TimeSeg2))
```

#### Популярные скорости:

**125 кбит/с:**
```c
hcan.Init.Prescaler = 18;
hcan.Init.SyncJumpWidth = CAN_SJW_1TQ;
hcan.Init.TimeSeg1 = CAN_BS1_13TQ;
hcan.Init.TimeSeg2 = CAN_BS2_2TQ;
// 36MHz / (18 × 16) = 125000 бит/с
```

**250 кбит/с:**
```c
hcan.Init.Prescaler = 9;
hcan.Init.SyncJumpWidth = CAN_SJW_1TQ;
hcan.Init.TimeSeg1 = CAN_BS1_13TQ;
hcan.Init.TimeSeg2 = CAN_BS2_2TQ;
// 36MHz / (9 × 16) = 250000 бит/с
```

**500 кбит/с:**
```c
hcan.Init.Prescaler = 4;
hcan.Init.SyncJumpWidth = CAN_SJW_1TQ;
hcan.Init.TimeSeg1 = CAN_BS1_15TQ;
hcan.Init.TimeSeg2 = CAN_BS2_2TQ;
// 36MHz / (4 × 18) = 500000 бит/с
```

**1 Мбит/с:**
```c
hcan.Init.Prescaler = 2;
hcan.Init.SyncJumpWidth = CAN_SJW_1TQ;
hcan.Init.TimeSeg1 = CAN_BS1_15TQ;
hcan.Init.TimeSeg2 = CAN_BS2_2TQ;
// 36MHz / (2 × 18) = 1000000 бит/с
```

### Пошаговая процедура изменения:

1. **Отредактировать** `Core/Src/can.c`, строки 38-45
2. **Обновить документацию** в `App/CAN_manager.c`, строки 26-45
3. **Пересобрать проект** (Ctrl+Shift+P → CMake: Build)
4. **Запрограммировать** контроллер
5. **Проверить** связь с другими устройствами на шине

### Ограничения:
- Предделитель: 1-1024
- TimeSeg1: 1-16 квантов
- TimeSeg2: 1-8 квантов
- SJW: 1-4 кванта (≤ TimeSeg2)

## Модификация символов и добавление новых

Система символов поддерживает **8×8 пиксельные** изображения для LED матрицы.

### Структура системы символов

#### 1. Определения символов: `App/Symbols.h`
```c
// Цифры 0-9
#define SYM_0           0
#define SYM_1           1
...
#define SYM_9           9

// Специальные символы
#define SYM_FRAME_THICK 10
#define SYM_DOT_SMALL   13
...

// Буквы
#define SYM_A           30
#define SYM_B           31
...
```

#### 2. Растровые данные: `App/Symbols.c`
```c
unsigned char Symbols[][8] = {
  {  // 0: SYM_0 - символ '0'
    ________,    // строка 0 (верх)
    ___XX___,    // строка 1
    __X__X__,    // строка 2
    __X__X__,    // строка 3
    __X__X__,    // строка 4
    __X__X__,    // строка 5
    ___XX___,    // строка 6
    ________,    // строка 7 (низ)
  },
  // ... остальные символы
};
```

#### 3. Система переназначения: `App/Symbols_Remaper.c`
Позволяет переназначать коды символов для разных конфигураций.

### Добавление нового символа

#### Шаг 1: Добавить определение в `App/Symbols.h`
```c
// В конец списка определений
#define SYM_NEW_SYMBOL  52  // Новый символ
```

#### Шаг 2: Создать растровые данные в `App/Symbols.c`
```c
// В конец массива Symbols, перед закрывающей скобкой
{  // 52: SYM_NEW_SYMBOL - описание символа
  ________,    // Строка 0: ________
  ___XX___,    // Строка 1: ___XX___
  __XXXX__,    // Строка 2: __XXXX__
  _XXXXXX_,    // Строка 3: _XXXXXX_
  XXXXXXXX,    // Строка 4: XXXXXXXX
  _XXXXXX_,    // Строка 5: _XXXXXX_
  __XXXX__,    // Строка 6: __XXXX__
  ___XX___,    // Строка 7: ___XX___
},
```

#### Пояснения по растровому формату:
- `X` = включенный пиксель (LED горит)
- `_` = выключенный пиксель (LED не горит)
- Каждая строка = 8 бит = 1 байт
- Строка 0 = верх дисплея, строка 7 = низ дисплея

#### Шаг 3: Обновить переназначение (при необходимости)
В `App/Symbols_Remaper.c` найти активную конфигурацию и добавить новый символ:
```c
#elif defined(REMAP_KG_EG_OG_3_4_5_6)

T_remap_sym remap_array[REMAP_SZ] =
{
  {0, SYM_KG},
  {1, SYM_EG},
  {2, SYM_OG},
  {3, SYM_NEW_SYMBOL},  // Добавить здесь
  // ... остальные
};
```

### Модификация существующего символа

#### Через CAN протокол (динамически):
```c
// Команды для изменения символа №5
// PDISPLx_SET_SYMBOL_PTRN1: загрузка строк 0-3
uint8_t cmd1[] = {0x15, 5, 0, 0, 0xFF, 0x81, 0x81, 0xFF};

// PDISPLx_SET_SYMBOL_PTRN2: загрузка строк 4-7
uint8_t cmd2[] = {0x16, 5, 0, 0, 0xFF, 0x81, 0x81, 0xFF};
```

#### Через редактирование кода:
1. Найти символ в `App/Symbols.c`
2. Отредактировать растровые данные
3. Пересобрать проект

### Создание схем переназначения

Для создания новой схемы переназначения:

#### Шаг 1: Добавить макрос в `App/Symbols_Remaper.c`
```c
// В начало файла с другими определениями
#define REMAP_MY_CUSTOM_LAYOUT  // Моя кастомная раскладка
```

#### Шаг 2: Создать таблицу переназначения
```c
#elif defined(REMAP_MY_CUSTOM_LAYOUT)

T_remap_sym remap_array[REMAP_SZ] =
{
 {0, SYM_A},          // Код 0 → показать символ A
 {1, SYM_B},          // Код 1 → показать символ B
 {2, SYM_NEW_SYMBOL}, // Код 2 → показать новый символ
 {3, SYM_3},          // Код 3 → показать цифру 3
 // ... всего 10 элементов
};
```

#### Шаг 3: Активировать новую схему
1. Закомментировать текущую активную схему
2. Раскомментировать `#define REMAP_MY_CUSTOM_LAYOUT`
3. Пересобрать проект

### Динамические символы

Система поддерживает анимированные символы через CAN команды:

#### Команды настройки:
1. **PDISPLx_DIN_SYMBOL_SET1**: номер символа, период, количество шагов
2. **PDISPLx_DIN_SYMBOL_SET2**: приращения по X и Y
3. **PDISPLx_DIN_SYMBOL_SET3**: начальные координаты
4. **PDISPLx_DIN_SYMBOL_SET4**: цвет и запуск анимации

#### Пример использования:
```c
// Настройка анимации символа №10
// Шаг 1: символ, период 100мс, 50 шагов
Handle_CAN_DynamicSymbolSet1({0x11, 10, 100, 0, 50, 0, 0, 0});

// Шаг 2: сдвиг +1 по X, +0 по Y за шаг
Handle_CAN_DynamicSymbolSet2({0x12, 0, 0, 1, 0, 0, 0, 0});

// Шаг 3: стартовая позиция (0,0)
Handle_CAN_DynamicSymbolSet3({0x13, 0, 0, 0, 0, 0, 0, 0});

// Шаг 4: зеленый цвет, запуск
Handle_CAN_DynamicSymbolSet4({0x14, 1, 0, 0, 0, 0, 0, 0});
```

## Полезные инструменты

### 1. Создание растровых изображений
- **Онлайн редактор**: https://www.piskelapp.com/ (8×8 пикселей)
- **GIMP**: Создать изображение 8×8, экспортировать как XBM
- **Текстовый редактор**: Рисовать символами X и _

### 2. Мониторинг CAN шины
- **CANoe** (Vector)
- **PCAN-View** (PEAK System)
- **CAN Hacker** (бесплатный)

### 3. Отладка
- **J-Link RTT Viewer**: Для вывода отладочных сообщений
- **STM32CubeProgrammer**: Программирование и проверка
- **Serial Monitor**: Для UART отладки

## Часто встречающиеся проблемы

### 1. Ошибки сборки
- **arm-none-eabi-gcc not found**: Проверить PATH, переустановить ARM Toolchain
- **CMake configuration failed**: Убедиться, что CMake добавлен в PATH

### 2. Проблемы с J-Link
- **Device not found**: Проверить подключение, драйверы J-Link
- **SWD connection failed**: Проверить линии SWDIO/SWCLK, питание

### 3. CAN проблемы
- **No communication**: Проверить терминаторы 120 Ом, скорость шины
- **Bus error**: Проверить правильность подключения CAN_H/CAN_L

### 4. Отображение символов
- **Символ не отображается**: Проверить номер символа, диапазон массива
- **Неправильное изображение**: Проверить растровые данные, переназначение

## Контакты и поддержка

При возникновении проблем:
1. Проверить данную документацию
2. Изучить комментарии в исходном коде
3. Использовать отладчик для трассировки проблем
4. Обратиться к документации STM32F103 Reference Manual

---

*Документация создана для проекта STM32F103C4 LED Matrix Control*
*Версия: 1.0*
*Дата: 2024*
