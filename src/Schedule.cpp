
#include "Schedule.h"
#include <stdexcept>
#include <sstream>
#include <iomanip>
#include <cctype>

using namespace std;

// Превръща "HH:MM" в брой минути от началото на деня ("06:30" -> 390).
// Разчита, че форматът вече е проверен от validateTime.
int timeToMinutes(const string &time)
{
    int hours = stoi(time.substr(0, 2));
    int minutes = stoi(time.substr(3, 2));

    return hours * 60 + minutes;
}

// Обратната операция: минути от началото на деня -> "HH:MM" с водеща нула
// (390 -> "06:30").
string minutesToTime(int totalMinutes)
{
    int hours = totalMinutes / 60;
    int minutes = totalMinutes % 60;

    stringstream ss;

    ss << setw(2) << setfill('0') << hours
       << ":"
       << setw(2) << setfill('0') << minutes;

    return ss.str();
}

// Часът трябва да е точно във вида HH:MM, с цифри на останалите места,
// часове 00-23 и минути 00-59.
void Schedule::validateTime(const string &time) const
{
    // Дължина 5 и двоеточие на третата позиция.
    if (time.length() != 5 || time[2] != ':')
    {
        throw invalid_argument(
            "Невалиден формат на часа. Използвайте HH:MM.");
    }

    // Всички позиции освен двоеточието трябва да са цифри. Проверката е
    // преди stoi, за да не се подадат знаци или интервали.
    for (int i = 0; i < 5; i++)
    {
        if (i == 2)
        {
            continue;
        }

        if (!isdigit(static_cast<unsigned char>(time[i])))
        {
            throw invalid_argument(
                "Часът трябва да съдържа само цифри и двоеточие.");
        }
    }

    int hours = stoi(time.substr(0, 2));
    int minutes = stoi(time.substr(3, 2));

    if (hours < 0 || hours > 23)
    {
        throw invalid_argument(
            "Часовете трябва да бъдат между 00 и 23.");
    }

    if (minutes < 0 || minutes > 59)
    {
        throw invalid_argument(
            "Минутите трябва да бъдат между 00 и 59.");
    }
}

// Валидация на номера на линията. Трябва да е положително число.
void Schedule::validateLineNumber(int lineNumber) const
{
    if (lineNumber <= 0)
    {
        throw invalid_argument(
            "Номерът на линията трябва да бъде положително число.");
    }
}

// Стандартно разписание: 06:00 - 23:00. Тези стойности са константи в кода,
// затова не се валидират отново.
Schedule::Schedule(int lineNumber)
    : lineNumber(lineNumber),
      startTime("06:00"),
      endTime("23:00")
{
    validateLineNumber(lineNumber);
}

// Разписание със зададени часове. Началният не може да е след крайния.
Schedule::Schedule(
    int lineNumber,
    const string &startTime,
    const string &endTime)
    : lineNumber(lineNumber),
      startTime(startTime),
      endTime(endTime)
{
    validateLineNumber(lineNumber);

    validateTime(startTime);
    validateTime(endTime);

    if (timeToMinutes(startTime) > timeToMinutes(endTime))
    {
        throw invalid_argument(
            "Началният час не може да бъде след крайния час.");
    }
}

// Геттъри. Връщат стойността на полетата.
int Schedule::getLineNumber() const
{
    return lineNumber;
}

string Schedule::getStartTime() const
{
    return startTime;
}

string Schedule::getEndTime() const
{
    return endTime;
}

// Сетъри. Проверяват валидността на новите стойности и ги присвояват.
void Schedule::setLineNumber(int lineNumber)
{
    validateLineNumber(lineNumber);

    this->lineNumber = lineNumber;
}

void Schedule::setStartTime(const string &startTime)
{
    validateTime(startTime);

    if (timeToMinutes(startTime) > timeToMinutes(endTime))
    {
        throw invalid_argument(
            "Началният час не може да бъде след крайния час.");
    }

    this->startTime = startTime;
}

void Schedule::setEndTime(const string &endTime)
{
    validateTime(endTime);

    if (timeToMinutes(endTime) < timeToMinutes(startTime))
    {
        throw invalid_argument(
            "Крайният час не може да бъде преди началния час.");
    }

    this->endTime = endTime;
}

// Генериране на часове на тръгване.
//  Първото тръгване е в началния час, всяко следващо е след movementInterval
//  минути, а последното е най-късно в крайния час (включително).
//  Пример: 06:00-07:00 при интервал 20 -> 06:00, 06:20, 06:40, 07:00.
vector<string> Schedule::generateDepartureTimes(
    int movementInterval) const
{
    if (movementInterval <= 0)
    {
        throw invalid_argument(
            "Интервалът на движение трябва да бъде положително число.");
    }

    vector<string> departureTimes;

    int startMinutes = timeToMinutes(startTime);
    int endMinutes = timeToMinutes(endTime);

    for (int currentMinutes = startMinutes;
         currentMinutes <= endMinutes;
         currentMinutes += movementInterval)
    {
        departureTimes.push_back(
            minutesToTime(currentMinutes));
    }

    return departureTimes;
}

// Извеждане
ostream &operator<<(ostream &out, const Schedule &schedule)
{
    out << "Разписание на линия: "
        << schedule.getLineNumber()
        << "\nНачален час: "
        << schedule.getStartTime()
        << "\nКраен час: "
        << schedule.getEndTime();

    return out;
}
