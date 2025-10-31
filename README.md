# Crysis Wars: The Other Side
Исходный код от модификации The Other Side. Это мультиплеерная модификация к игре Crysis Wars, добавляющая возможность играть за пришельцев и режим игры Zeus. Zeus - это бесплатное дополнение для Arma 3 наделяет одного из игроков ролью 'Зевса', мастера игры, влияющего на игровые события. Используя интуитивно понятный редактор в реальном времени, мастер игры может вводить новые история, испытания и локации. В результате получается всегда уникальное, всегда динамическое сражение - как для 'Зевса', так и 'простых смертных' солдат.

![Статус разработки](https://img.shields.io/badge/статус-разработка-orange)
![Статус билда](https://img.shields.io/badge/билд-успешный-brightgreen)

## Содержание
- [Технологии](#Технологии)
- [Начало работы](#начало-работы)
- [Источники](#Источники)
- [Команда проекта](#команда-проекта)

## Технологии
- [CryENGINE 2](https://www.cryengine.com/)
- [C++ 11](https://isocpp.org/)
- [Git](https://git-scm.com/)
- [Trello](https://trello.com/)

## Начало работы
Чтобы установить и использовать проект, выполните следующие шаги:

1. Установите IDE [Microsoft Visual Studio 2022](https://visualstudio.microsoft.com/ru/)
2. Установите игру Crysis Wars
3. Установите редактор и исходный код [Crysis Wars MOD SDK v1.1](https://www.dj-copniker.de/download/patches/crysis-wars-mod-sdk)
4. Установите исправленные заголовки интерфейсов [Fixed CryENGINE Interfaces](https://github.com/akeeperctl/crysis-wars-cryengine-interfaces/tree/multiplayer)
5. Создайте папку со своей модификацией *CrysisWars/Mods/__ВАШ_МОД_*
6. Клонируйте репозиторий:
   ```sh
   git clone https://github.com/crytechlab/crysis-wars-the-other-side.git
   ```
7. В свойствах проекта GameDLL **Компоновщик->Ввод**, в поле **Дополнительные зависимости** укажите
	```
	version.lib;%(AdditionalDependencies)
	```
8. Установите [IncrementBuild](https://alax.info/blog/1713) в папку *CrysisWars/Mods/__ВАШ_МОД_/IncrementBuild*

	Содержимое этой папки должно быть таким:
	```
	config.ini
	IncrementBuild-Win32.exe
	IncrementBuild-x64.exe
	```
9. В проекте GameDLL настройте путь запуска редактора *Editor.exe* и игры *Crysis.exe* для отладки
	
	Пример как задать путь в **Свойства конфигурации->Отладка** для платформы x64: 
	- Команда:
		```
		 $(ProjectDir)..\..\..\Bin64\Editor.exe
		```
	- Аргументы команды:
		```
		-mod _ВАШ_МОД
		```

## Источники
Для создания этой модификации меня вдохновил [игровой режим Zeus из игры Arma 3](https://store.steampowered.com/app/275700/Arma_3_Zeus/?l=russian). Он делает бои между игроками настолько непредсказуемыми и интересными насколько это возможно! Вся суть в том, что Zeus - это тоже игрок, способный в любой момент заспавнить врагов, командовать ими или наоборот помочь игроками, скинув боеприпасы. Механики Zeus были воссозданы из соображения о том, как они должны работать.

## Команда проекта
- [Даниил Акипский](tg://resolve?domain=slyavanin) - Software Engineer, UI Designer