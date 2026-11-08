
#ifndef TRANSPORT_MANAGER_H
#define TRANSPORT_MANAGER_H
#include "BusStop.h"
#include "TransitLine.h"
#include "Schedule.h"
#include <unordered_map>
#include <string>
#include <vector>
#include <iostream>

using namespace std;

/**
 * Централният клас на системата. Държи всички спирки, линии и разписания
 * и пази връзките между тях в съгласувано състояние:
 *   - линия може да съдържа само спирки, които съществуват;
 *   - разписание може да се добави само за съществуваща линия;
 *   - при изтриване на спирка тя се маха от всички маршрути;
 *   - при изтриване на линия се изтрива и нейното разписание.
 *
 * Данните се зареждат от и се записват в три CSV файла.
 */

class TransportManager
{
private:
    unordered_map<string, BusStop> busStops;      // колекция, съхраняваща спирките, ключ: код на спирката
    unordered_map<int, TransitLine> transitLines; // колекция, съхраняваща линиите, ключ: номер на линията
    unordered_map<int, Schedule> schedules;       // колекция, съхраняваща разписанията, ключ: номер на линията

    string stopsFile;     // път до CSV със спирките
    string linesFile;     // път до CSV с линиите
    string schedulesFile; // път до CSV с разписанията

public:
    // Празен мениджър, без файлове. Подходящ за работа само в паметта.
    TransportManager();

    // Мениджър, свързан с три файла. Конструкторът зарежда данните веднага,
    // по реда спирки, линии, разписания, защото всяко следващо зависи от
    // предишното. Хвърля изключение.
    TransportManager(
        const string &stopsFile,
        const string &linesFile,
        const string &schedulesFile);

    // Методи за работа със спирки
    void addBusStop(const BusStop &stop);
    void removeBusStop(const string &stopCode);
    BusStop getBusStop(const string &stopCode) const;

    // Методи за работа с линии
    void addTransitLine(const TransitLine &line);
    void removeTransitLine(int lineNumber);
    TransitLine getTransitLine(int lineNumber) const;

    // Методи за работа с връзката линия-спирка
    void assignStopToLine(
        int lineNumber,
        const string &stopCode);
    void removeStopFromLine(
        int lineNumber,
        const string &stopCode);
    vector<int> findLinesByStop(
        const string &stopCode) const;

    // Методи за работа с разписания
    void displaySchedule(int lineNumber) const;
    void addSchedule(const Schedule &schedule);

    // Методи за извеждане на данните за спирки, линии и маршрути в конзолата
    void displayAllBusStops() const;
    void displayAllTransitLines() const;
    void displayRoute(int lineNumber) const;

    // Методи за зареждане и запис на данните във файловете
    void loadBusStops();
    void loadTransitLines();
    void saveBusStops() const;
    void saveTransitLines() const;
    void loadSchedules();
    void saveSchedules() const;
};
// Извежда името на класа
ostream &operator<<(
    ostream &out,
    const TransportManager &manager);

#endif
