

Скачать последнюю версию компилятора с https://www.nasm.us/

Установить по стандартной процедуре

Установить в переменные среды в Windows

Должна быть доступна команда nasm -v

Установите MSYS2 https://www.msys2.org/

Скачайте установщик (msys2-x86_64-*.exe) и запустите его

Откройте MSYS2 MSYS (из меню Пуск) и выполните:

pacman -Syu

Если система попросит перезапустить терминал — закройте его и откройте снова, затем снова выполните:

pacman -Syu

Установите MinGW-w64 GCC

Откройте MSYS2 MinGW x64 (из меню Пуск).

Далее выполнить
pacman -S mingw-w64-x86_64-gcc

Добавьте MinGW-w64 в PATH (переменные среды)

C:\msys64\mingw64\bin

Должна быть доступна команда gcc -v


установить зависимости Node.js:
npm install

Запуск генерации грамматики:
npx tree-sitter generate

src- исходники парсера


Настройка CLion:

Edit Configurations

program arguments - Конфигурация запуска основного файла программы:
input.mylang ast.mmd cfg assembler-code

Working directory - рабочая директория (корень проекта):
C:/Users/DN3672/CLionProjects/system-software-lab-3

Скомпилировать главный файл в объектный файл windows можно через команду:
nasm -f win64 main.asm -o main.obj

Для линковки и получения исполняемого файла можно использовать:
gcc main.obj -o main.exe

Запустить программу можно через
.\main.exe

Посмотреть результат можно через:
echo $LASTEXITCODE










