
#ifndef SCHEDULE_H
#define SCHEDULE_H
#include <string>
#include <vector>
#include <iostream>

using namespace std;

/**
 * Разписание на една транспортна линия.
 *
 * Пази само началния и крайния час на движение. Конкретните часове на
 * тръгване не се съхраняват, а се изчисляват от интервала на линията
 * (generateDepartureTimes). Връзката с линията е по нейния номер.
 *
 * Часовете са текст във формат "HH:MM" (24-часов), например "06:00".
 */

class Schedule
{
private:
    int lineNumber;   // номер на линията, за която важи разписанието
    string startTime; // първо тръгване, "HH:MM"
    string endTime;   // последно възможно тръгване, "HH:MM"

    // Валидация. Всеки метод хвърля invalid_argument със съобщение за проблема.
    void validateLineNumber(int lineNumber) const;
    void validateTime(const string &time) const;

public:
    // Разписание със стандартни часове: от 06:00 до 23:00.
    Schedule(int lineNumber);

    // Разписание със зададени часове. Началният не може да е след крайния.
    Schedule(
        int lineNumber,
        const string &startTime,
        const string &endTime);

    // Четене на полетата.
    int getLineNumber() const;
    string getStartTime() const;
    string getEndTime() const;

    // Сетърите запазват правилото начало <= край.
    void setLineNumber(int lineNumber);
    void setStartTime(const string &startTime);
    void setEndTime(const string &endTime);

    // Връща всички часове на тръгване от началния до крайния час включително,
    // през зададения интервал в минути.
    vector<string> generateDepartureTimes(
        int movementInterval) const;
};

// Извежда разписанието на няколко реда: линия, начален и краен час.
ostream &operator<<(ostream &out, const Schedule &schedule);

#endif
