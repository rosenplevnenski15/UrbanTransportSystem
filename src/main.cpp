#include "TransportManager.h"
#include "TransportType.h"
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

using namespace std;

namespace
{

    // Пътища до файловете с данни. Папката data/ трябва да съществува преди
    // стартиране: програмата създава файловете, но не и папката.
    const string STOPS_FILE = "data/bus_stops.csv";
    const string LINES_FILE = "data/transit_lines.csv";
    const string SCHEDULES_FILE = "data/schedules.csv";

    // Ако файлът не съществува, го създава само със заглавен ред.
    // Съществуващ файл не се пипа.
    void ensureDataFile(const string &fileName, const string &header)
    {
        ifstream input(fileName);
        if (input.good())
        {
            return;
        }

        ofstream output(fileName);
        if (!output)
        {
            throw runtime_error("Не може да се създаде файлът " + fileName);
        }
        output << header << '\n';
    }

    // Създава файла наново, като изтрива старото съдържание (ios::trunc).
    void createNewDataFile(const string &fileName, const string &header)
    {
        ofstream output(fileName, ios::trunc);
        if (!output)
        {
            throw runtime_error("Не може да се създаде файлът " + fileName);
        }
        output << header << '\n';
    }

    // Записва и трите файла. Вика се след всяка промяна, така че данните
    // не се губят при внезапно спиране на програмата.
    void saveAll(const TransportManager &manager)
    {
        manager.saveBusStops();
        manager.saveTransitLines();
        manager.saveSchedules();
    }

    // Всяка функция чете в цикъл, докато потребителят въведе допустима стойност.
    // За проверка се създава временен обект от съответния клас: така правилата
    // за валидация живеят на едно място (в класовете), а не се повтарят тук.

    // Чете цял ред, включително интервали.
    string readText(const string &prompt)
    {
        cout << prompt;
        string value;
        getline(cin, value);
        return value;
    }

    // Чете код за нова спирка: трябва да е валиден и още да не се използва.
    string readStopCode(const TransportManager &manager)
    {
        while (true)
        {
            const string code = readText("Уникален код на спирката (латински букви/цифри): ");
            try
            {
                // Временна спирка само за проверка на формата на кода.
                const BusStop validationStop(code, "Test", "Test");
                (void)validationStop;
            }
            catch (const exception &error)
            {
                cout << "Невалиден код на спирка: " << error.what() << '\n';
                continue;
            }

            // getBusStop хвърля изключение, когато кодът е свободен. Това е
            // желаният случай, затова връщането става в catch блока.
            try
            {
                manager.getBusStop(code);
                cout << "Спирка с този код вече съществува. Въведете друг код.\n";
            }
            catch (const invalid_argument &)
            {
                return code;
            }
        }
    }

    // Чете код на спирка, която вече е регистрирана.
    string readExistingStopCode(const TransportManager &manager, const string &prompt)
    {
        while (true)
        {
            const string code = readText(prompt);
            try
            {
                manager.getBusStop(code);
                return code;
            }
            catch (const exception &error)
            {
                cout << "Невалиден код на спирка: " << error.what() << '\n';
            }
        }
    }

    // Чете име на спирка. Проверява се само формата.
    string readStopName()
    {
        while (true)
        {
            const string name = readText("Име на спирката: ");
            try
            {
                const BusStop validationStop("TEST", name, "Test address");
                (void)validationStop;
                return name;
            }
            catch (const exception &error)
            {
                cout << "Невалидно име на спирка: " << error.what() << '\n';
            }
        }
    }

    // Името се подава, защото BusStop не може да се създаде без валидно име.
    string readStopAddress(const string &name)
    {
        while (true)
        {
            const string address = readText("Адрес: ");
            try
            {
                const BusStop validationStop("TEST", name, address);
                (void)validationStop;
                return address;
            }
            catch (const exception &error)
            {
                cout << "Невалиден адрес: " << error.what() << '\n';
            }
        }
    }

    // Предварителна декларация: readLatitude и readLongitude я ползват преди
    // дефиницията ѝ по-надолу.
    double readDouble(const string &prompt);

    // Чете географска ширина. Проверява се само формата.
    double readLatitude()
    {
        while (true)
        {
            const double latitude = readDouble("Географска ширина (-90 до 90): ");
            try
            {
                const BusStop validationStop("TEST", "Test", "Test", latitude, 0.0);
                (void)validationStop;
                return latitude;
            }
            catch (const exception &error)
            {
                cout << "Невалидна географска ширина: " << error.what() << '\n';
            }
        }
    }

    // Чете географска дължина. Проверява се само формата.
    double readLongitude()
    {
        while (true)
        {
            const double longitude = readDouble("Географска дължина (-180 до 180): ");
            try
            {
                const BusStop validationStop("TEST", "Test", "Test", 0.0, longitude);
                (void)validationStop;
                return longitude;
            }
            catch (const exception &error)
            {
                cout << "Невалидна географска дължина: " << error.what() << '\n';
            }
        }
    }

    // Чете цяло число. Приема се само ако целият низ е число: "12abc" се
    // отхвърля, защото stoi би върнал 12 и би спряло при първия лош знак.
    int readInt(const string &prompt)
    {
        while (true)
        {
            string value = readText(prompt);
            try
            {
                size_t used = 0;
                int result = stoi(value, &used);
                if (used == value.size())
                {
                    return result;
                }
            }
            catch (const exception &)
            {
                // Празен вход или твърде голямо число: пада към съобщението долу.
            }
            cout << "Моля, въведете цяло число.\n";
        }
    }

    // Чете номер на линия, която вече съществува.
    int readExistingLineNumber(const TransportManager &manager, const string &prompt)
    {
        while (true)
        {
            const int number = readInt(prompt);
            try
            {
                manager.getTransitLine(number);
                return number;
            }
            catch (const exception &error)
            {
                cout << "Невалиден номер на линия: " << error.what() << '\n';
            }
        }
    }

    // Чете дробно число със същата строга проверка като readInt.
    // Десетичният знак е точка.
    double readDouble(const string &prompt)
    {
        while (true)
        {
            string value = readText(prompt);
            try
            {
                size_t used = 0;
                double result = stod(value, &used);
                if (used == value.size())
                {
                    return result;
                }
            }
            catch (const exception &)
            {
            }
            cout << "Моля, въведете валидно число. Използвайте точка за десетичен знак.\n";
        }
    }

    // Типът може да се въведе с цифра (1-3) или с име, което parseTransportType
    // разпознава ("автобус", "tram" и т.н.).
    TransportType readTransportType()
    {
        while (true)
        {
            cout << "Тип транспорт (1 - автобус, 2 - трамвай, 3 - тролейбус): ";
            string choice;
            getline(cin, choice);
            try
            {
                if (choice == "1")
                    return TransportType::BUS;
                if (choice == "2")
                    return TransportType::TRAM;
                if (choice == "3")
                    return TransportType::TROLLEYBUS;
                return parseTransportType(choice);
            }
            catch (const exception &)
            {
                cout << "Невалиден тип транспорт. Опитайте отново.\n";
            }
        }
    }

    // Показва менюто с възможните действия.
    void printMenu()
    {
        cout << "\n========== ГРАДСКИ ТРАНСПОРТ ==========\n"
             << "1. Добавяне на спирка\n"
             << "2. Добавяне на транспортна линия\n"
             << "3. Разпределяне на спирка към линия\n"
             << "4. Търсене на линии по спирка\n"
             << "5. Извеждане на маршрут\n"
             << "6. Добавяне на разписание\n"
             << "7. Извеждане на разписание\n"
             << "8. Показване на всички спирки\n"
             << "9. Показване на всички линии\n"
             << "10. Премахване на спирка\n"
             << "11. Премахване на линия\n"
             << "0. Запис и изход\n";
    }

    // Меню 1: събира данните стъпка по стъпка, добавя спирката и записва.
    void addStop(TransportManager &manager)
    {
        const string code = readStopCode(manager);
        const string name = readStopName();
        const string address = readStopAddress(name);
        const double latitude = readLatitude();
        const double longitude = readLongitude();
        manager.addBusStop(BusStop(code, name, address, latitude, longitude));
        saveAll(manager);
        cout << "Спирката е добавена и записана.\n";
    }

    // Меню 2: създава линия без спирки. Маршрутът се попълва от меню 3.
    void addLine(TransportManager &manager)
    {
        int number;
        while (true)
        {
            number = readInt("Номер на линията: ");
            try
            {
                // Проверка на номера с временна линия (интервал 1 е произволен).
                const TransitLine validationLine(number, TransportType::BUS, 1);
                (void)validationLine;
            }
            catch (const exception &error)
            {
                cout << "Невалиден номер на линия: " << error.what() << '\n';
                continue;
            }

            // Тук изключението от getTransitLine означава, че номерът е свободен.
            try
            {

                manager.getTransitLine(number);
                cout << "Линия с този номер вече съществува. Въведете друг номер.\n";
            }
            catch (const exception &)
            {

                break;
            }
        }

        const TransportType type = readTransportType();
        int interval;
        while (true)
        {
            interval = readInt("Интервал на движение в минути: ");
            try
            {
                const TransitLine validationLine(number, type, interval);
                (void)validationLine;
                break;
            }
            catch (const exception &error)
            {
                cout << "Невалиден интервал: " << error.what() << '\n';
            }
        }

        manager.addTransitLine(TransitLine(number, type, interval));
        saveAll(manager);
        cout << "Линията е добавена. Добавете спирките ѝ от меню 3.\n";
    }

    // Меню 3: добавя съществуваща спирка в края на маршрута на съществуваща линия.
    void assignStop(TransportManager &manager)
    {
        const int number = readExistingLineNumber(manager, "Номер на линията: ");
        const string code = readExistingStopCode(manager, "Код на спирката: ");
        manager.assignStopToLine(number, code);
        saveAll(manager);
        cout << "Спирката е добавена към маршрута.\n";
    }

    // Меню 4: извежда всички линии, които минават през дадена спирка.
    void findLines(const TransportManager &manager)
    {
        const string code = readText("Код на спирката: ");
        const vector<int> lines = manager.findLinesByStop(code);
        if (lines.empty())
        {
            cout << "Няма линии, които минават през тази спирка.\n";
            return;
        }
        cout << "Линии през спирка " << code << ": ";
        for (size_t i = 0; i < lines.size(); ++i)
        {
            if (i != 0)
                cout << ", ";
            cout << lines[i];
        }
        cout << '\n';
    }

    // Меню 6: задава начален и краен час за линия.
    void addSchedule(TransportManager &manager)
    {
        const int number = readExistingLineNumber(manager, "Номер на линията: ");

        string start;
        while (true)
        {
            // Началният час се проверява срещу "23:59", за да не се отхвърли заради
            // още неизвестния краен час. Взаимната проверка е при краен час.
            start = readText("Начален час (HH:MM): ");
            try
            {
                const Schedule validationSchedule(number, start, "23:59");
                (void)validationSchedule;
                break;
            }
            catch (const exception &error)
            {
                cout << "Невалиден начален час: " << error.what() << '\n';
            }
        }

        string end;
        while (true)
        {
            end = readText("Краен час (HH:MM): ");
            try
            {
                const Schedule validationSchedule(number, start, end);
                (void)validationSchedule;
                break;
            }
            catch (const exception &error)
            {
                cout << "Невалиден краен час: " << error.what() << '\n';
            }
        }

        manager.addSchedule(Schedule(number, start, end));
        saveAll(manager);
        cout << "Разписанието е добавено и записано.\n";
    }
}

// Главна програма: показва меню, чете избор и извиква съответната функция.
int main()
{
    // Външният try хваща грешки при стартиране (липсваща папка data/,
    // повреден CSV). Вътрешният, в цикъла на менюто, хваща грешките от
    // отделните команди, така че една грешка не спира програмата.
    try
    {
        cout << "Как искате да започнете?\n"
             << "1. Работа със CSV файловете (зареждат се записаните данни; празните или съдържащите само заглавен ред файлове се приемат за празни; липсващите файлове се създават)\n"
             << "2. Създаване на нов набор от CSV данни (съществуващите файлове ще бъдат изчистени)\n";

        int startChoice;
        do
        {
            startChoice = readInt("Избор (1 или 2): ");
            if (startChoice != 1 && startChoice != 2)
            {
                cout << "Моля, изберете 1 или 2.\n";
            }
        } while (startChoice != 1 && startChoice != 2);

        if (startChoice == 1)
        {
            ensureDataFile(STOPS_FILE, "code,name,address,latitude,longitude");
            ensureDataFile(LINES_FILE, "lineNumber,transportType,movementInterval,stopCodes");
            ensureDataFile(SCHEDULES_FILE, "lineNumber,startTime,endTime");
        }
        else
        {
            createNewDataFile(STOPS_FILE, "code,name,address,latitude,longitude");
            createNewDataFile(LINES_FILE, "lineNumber,transportType,movementInterval,stopCodes");
            createNewDataFile(SCHEDULES_FILE, "lineNumber,startTime,endTime");
        }

        // Конструкторът зарежда данните от файловете.
        TransportManager manager(STOPS_FILE, LINES_FILE, SCHEDULES_FILE);
        if (startChoice == 1)
        {
            cout << "Данните са заредени от CSV файловете.\n";
        }
        else
        {
            cout << "Създаден е нов набор CSV файлове.\n";
        }

        bool running = true;
        while (running)
        {
            printMenu();
            const int choice = readInt("Избор: ");
            try
            {
                switch (choice)
                {
                case 1:
                    addStop(manager);
                    break;
                case 2:
                    addLine(manager);
                    break;
                case 3:
                    assignStop(manager);
                    break;
                case 4:
                    findLines(manager);
                    break;
                case 5:
                    manager.displayRoute(readExistingLineNumber(manager, "Номер на линията: "));
                    break;
                case 6:
                    addSchedule(manager);
                    break;
                case 7:
                    manager.displaySchedule(readExistingLineNumber(manager, "Номер на линията: "));
                    break;
                case 8:
                    manager.displayAllBusStops();
                    break;
                case 9:
                    manager.displayAllTransitLines();
                    break;
                case 10:
                    manager.removeBusStop(readExistingStopCode(manager, "Код на спирката: "));
                    saveAll(manager);
                    cout << "Спирката е премахната.\n";
                    break;
                case 11:
                    manager.removeTransitLine(readExistingLineNumber(manager, "Номер на линията: "));
                    saveAll(manager);
                    cout << "Линията и нейното разписание са премахнати.\n";
                    break;
                case 0:
                    saveAll(manager);
                    running = false;
                    cout << "Промените са записани. Довиждане!\n";
                    break;
                default:
                    cout << "Няма такава команда. Изберете от менюто.\n";
                }
            }
            catch (const exception &error)
            {
                cout << "Грешка: " << error.what() << '\n';
            }
        }
    }
    catch (const exception &error)
    {
        cerr << "Неуспешно стартиране: " << error.what() << '\n';
        return 1;
    }
    return 0;
}
