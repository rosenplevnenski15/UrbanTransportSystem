#ifndef TRANSIT_LINE_H
#define TRANSIT_LINE_H
#include "TransportType.h"
#include <string>
#include <vector>

using namespace std;

/**
 * Транспортна линия: номер, вид транспорт, интервал на движение и маршрут.
 *
 * Маршрутът е подреден списък от кодове на спирки. Редът в списъка е редът,
 * в който линията минава през тях. Класът не знае дали спирка с даден код
 * съществува, а само проверява формата на кода и че няма дубликати.
 * Съществуването се проверява в TransportManager.
 */

class TransitLine
{
private:
    int lineNumber;              // номер на линията, положително цяло число
    TransportType transportType; // автобус, трамвай или тролейбус
    vector<string> stopCodes;    // кодове на спирките по реда на маршрута
    int movementInterval;        // интервал между две тръгвания, в минути

    // Валидация. Всеки метод хвърля invalid_argument със съобщение за проблема.
    void validateLineNumber(int lineNumber) const;
    void validateTransportType(TransportType transportType) const;
    void validateMovementInterval(int movementInterval) const;
    void validateStopCodes(const vector<string> &stopCodes) const;
    void validateCode(const string &code) const;

public:
    // Линия без спирки. Спирките се добавят после с addStop.
    TransitLine(int lineNumber,
                TransportType transportType,
                int movementInterval);

    // Линия със зададен маршрут. Кодовете не могат да се повтарят.
    TransitLine(int lineNumber,
                TransportType transportType,
                const vector<string> &stopCodes,
                int movementInterval);

    // Четене на полетата.
    int getLineNumber() const;
    TransportType getTransportType() const;
    const vector<string> &getStopCodes() const; // връщаме референция, а не копие на колекцията
    int getMovementInterval() const;

    // Сетърите запазват правилата за валидност на полетата.
    void setLineNumber(int lineNumber);
    void setTransportType(TransportType transportType);
    void setStopCodes(const vector<string> &stopCodes);
    void setMovementInterval(int movementInterval);

    // Добавя спирка в края на маршрута. Хвърля грешка, ако вече я има.
    void addStop(const string &stopCode);

    // Премахва спирка от маршрута. Хвърля грешка, ако я няма.
    void removeStop(const string &stopCode);
};
// Извежда линията на един ред: номер, тип, интервал и кодове на спирки.
ostream &operator<<(ostream &out, const TransitLine &line);
#endif
