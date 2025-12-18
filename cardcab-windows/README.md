# CardCab - Windows Edition

Кроссплатформенная версия менеджера коллекционных карт для Windows.

## Требования

### Для сборки

1. **Visual Studio 2019/2022** с компонентами C++ (или MinGW-w64)
2. **Qt 5.12+** с компонентами:
   - Qt Core
   - Qt GUI
   - Qt Widgets
   - Qt SQL
   - PostgreSQL plugin (qsqlpsql)
3. **CMake 3.16+**
4. **PostgreSQL** (для клиентских библиотек libpq)

### Для запуска

1. PostgreSQL сервер (можно через Docker)
2. Qt runtime DLLs (или статическая сборка)

## Установка Qt

### Вариант 1: Qt Installer (рекомендуется)
1. Скачайте Qt Online Installer: https://www.qt.io/download-qt-installer
2. Установите Qt 5.15.2 (или новее) для MSVC или MinGW
3. Выберите компоненты:
   - Qt 5.15.2 → MSVC 2019 64-bit (или MinGW)
   - Qt 5.15.2 → Sources (опционально)
4. Обязательно установите **SQL Drivers** → **PostgreSQL**

### Вариант 2: vcpkg
```powershell
vcpkg install qt5-base qt5-sql[psql] libpq
```

## Сборка

### С Visual Studio (MSVC)

1. Откройте **Developer Command Prompt for VS 2022**
2. Установите переменную окружения Qt:
   ```cmd
   set Qt5_DIR=C:\Qt\5.15.2\msvc2019_64\lib\cmake\Qt5
   ```
3. Запустите скрипт сборки:
   ```cmd
   build_msvc.bat
   ```

### С MinGW

1. Убедитесь, что MinGW в PATH
2. Установите переменную окружения Qt:
   ```cmd
   set Qt5_DIR=C:\Qt\5.15.2\mingw81_64\lib\cmake\Qt5
   ```
3. Запустите скрипт сборки:
   ```cmd
   build_mingw.bat
   ```

### PowerShell (рекомендуется)

```powershell
.\build_and_deploy.ps1 -QtDir "C:\Qt\5.15.2\msvc2019_64" -Deploy
```

### Ручная сборка

```cmd
mkdir build
cd build
cmake -G "Visual Studio 17 2022" -A x64 -DCMAKE_PREFIX_PATH="C:\Qt\5.15.2\msvc2019_64\lib\cmake\Qt5" ..
cmake --build . --config Release
```

## Развертывание

После сборки необходимо скопировать Qt DLL файлы:

```cmd
cd build\Release
C:\Qt\5.15.2\msvc2019_64\bin\windeployqt.exe CardCab.exe
```

Или используйте PowerShell скрипт с флагом `-Deploy`.

### Необходимые дополнительные файлы

1. **PostgreSQL DLLs** (скопируйте из установки PostgreSQL):
   - `libpq.dll`
   - `libcrypto-*.dll`
   - `libssl-*.dll`
   - `libiconv-2.dll` (если есть)
   - `libintl-8.dll` (если есть)

2. **Qt SQL Plugin** (скопируйте из Qt):
   - `sqldrivers/qsqlpsql.dll`

## Настройка PostgreSQL

### Через Docker (рекомендуется)

Создайте файл `docker-compose.yml`:

```yaml
version: '3.8'
services:
  postgres:
    image: postgres:15
    environment:
      POSTGRES_DB: cardcab_db
      POSTGRES_USER: cardcab_user
      POSTGRES_PASSWORD: cardcab_secure_password_2025
    ports:
      - "5436:5432"
    volumes:
      - ./sql/init.sql:/docker-entrypoint-initdb.d/init.sql
      - pgdata:/var/lib/postgresql/data

volumes:
  pgdata:
```

Запуск:
```cmd
docker-compose up -d
```

### Локальная установка PostgreSQL

1. Установите PostgreSQL 15+
2. Создайте базу данных и пользователя:
   ```sql
   CREATE USER cardcab_user WITH PASSWORD 'cardcab_secure_password_2025';
   CREATE DATABASE cardcab_db OWNER cardcab_user;
   ```
3. Выполните скрипт инициализации: `sql/init.sql`

## Переменные окружения

Приложение поддерживает следующие переменные окружения:

| Переменная | По умолчанию | Описание |
|------------|--------------|----------|
| `DB_HOST` | `localhost` | Хост PostgreSQL |
| `DB_PORT` | `5436` | Порт PostgreSQL |
| `DB_NAME` | `cardcab_db` | Имя базы данных |
| `DB_USER` | `cardcab_user` | Пользователь БД |
| `DB_PASSWORD` | `cardcab_secure_password_2025` | Пароль БД |

Пример запуска с кастомными параметрами:
```cmd
set DB_HOST=192.168.1.100
set DB_PORT=5432
CardCab.exe
```

## Структура проекта

```
cardcab-windows/
├── CMakeLists.txt          # Кроссплатформенный CMake
├── build_msvc.bat          # Сборка с MSVC
├── build_mingw.bat         # Сборка с MinGW
├── build_and_deploy.ps1    # PowerShell скрипт
├── README.md               # Этот файл
├── include/                # Заголовочные файлы
│   ├── card.h
│   ├── cardwidget.h
│   ├── carddialog.h
│   ├── constants.h         # Кроссплатформенные константы
│   ├── database.h
│   ├── logindialog.h
│   ├── mainwindow.h
│   └── user.h
├── src/                    # Исходный код
│   ├── main.cpp            # Кроссплатформенный main
│   ├── carddialog.cpp
│   ├── cardwidget.cpp
│   ├── database.cpp
│   ├── logindialog.cpp
│   └── mainwindow.cpp
└── sql/
    └── init.sql            # Инициализация БД
```

## Изменения для Windows

По сравнению с Linux-версией были внесены следующие изменения:

1. **main.cpp**: Добавлена кроссплатформенная обработка сигналов (Windows использует `SetConsoleCtrlHandler` вместо POSIX сигналов)

2. **constants.h**: Путь `HOST_HOME` теперь использует `QStandardPaths::HomeLocation` для кроссплатформенного определения домашней директории пользователя

3. **CMakeLists.txt**: Добавлена поддержка MSVC и MinGW компиляторов, UTF-8 кодировка для MSVC

## Решение проблем

### Qt не найден
```
Could not find a package configuration file provided by "Qt5"
```
Убедитесь, что `Qt5_DIR` указывает на правильную директорию, например:
```
C:\Qt\5.15.2\msvc2019_64\lib\cmake\Qt5
```

### PostgreSQL plugin не найден
```
QSqlDatabase: QPSQL driver not loaded
```
1. Убедитесь, что `qsqlpsql.dll` находится в `sqldrivers/`
2. Скопируйте `libpq.dll` и зависимости в директорию с exe

### Не удается подключиться к БД
1. Проверьте, что PostgreSQL запущен
2. Проверьте правильность порта (по умолчанию 5436)
3. Проверьте файрвол Windows

## Лицензия

НГТУ, кафедра Защиты информации, 2025
