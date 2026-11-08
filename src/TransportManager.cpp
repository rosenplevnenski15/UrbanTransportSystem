
#include "TransportManager.h"
#include "TransportType.h"
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <algorithm>
#include <iomanip>

// Помощни функции за CSV (видими само в този файл)

// Маха интервалите, табулациите и новите редове в началото и в края.
// Нужно е най-вече за "\r" в края на редове от файлове, записани на Windows.
static string trim(const string &text)
{
    size_t first = text.find_first_not_of(" \t\r\n");

    if (first == string::npos)
    {
        return "";
    }

    size_t last = text.find_last_not_of(" \t\r\n");

    return text.substr(first, last - first + 1);
}

// Разделя един CSV ред на полета по запетая.
// Запетаята вътре в кавички не разделя. Две поредни кавички вътре в поле
// се четат като една кавичка (стандартното CSV екраниране).
// Файлът се чете ред по ред, затова нов ред вътре в поле не се поддържа.
static vector<string> parseCsvLine(const string &line)
{
    vector<string> fields;
    string field;
    bool insideQuotes = false;

    for (size_t i = 0; i < line.length(); i++)
    {
        char ch = line[i];

        if (ch == '"')
        {
            if (insideQuotes && i + 1 < line.length() &&
                line[i + 1] == '"')
            {
                field += '"';
                i++;
            }
            else
            {
                insideQuotes = !insideQuotes;
            }
        }
        else if (ch == ',' && !insideQuotes)
        {
            fields.push_back(field);
            field.clear();
        }
        else
        {
            field += ch;
        }
    }

    if (insideQuotes)
    {
        throw invalid_argument("Невалиден CSV формат: незатворена кавичка.");
    }

    fields.push_back(field); // последното поле няма запетая след себе си

    return fields;
}

// Подготвя поле за запис. Ако съдържа запетая, кавичка или нов ред, го
// обгражда в кавички и удвоява кавичките вътре в него. Иначе го връща както е.
static string escapeCsvField(const string &field)
{
    if (field.find(',') == string::npos &&
        field.find('"') == string::npos &&
        field.find('\n') == string::npos &&
        field.find('\r') == string::npos)
    {
        return field;
    }

    string escaped = "\"";

    for (char ch : field)
    {
        if (ch == '"')
        {
            escaped += "\"\"";
        }
        else
        {
            escaped += ch;
        }
    }

    escaped += '"';

    return escaped;
}

// Разделя поле от вида "S001;S002;S003" на отделни кодове.
// Маршрутът се пази в едно CSV поле, затова разделителят е ';', а не ','.
// Празните части се пропускат.
static vector<string> splitStopCodes(const string &text)
{
    vector<string> stopCodes;

    if (text.empty())
    {
        return stopCodes;
    }

    stringstream ss(text);
    string stopCode;

    while (getline(ss, stopCode, ';'))
    {
        stopCode = trim(stopCode);

        if (!stopCode.empty())
        {
            stopCodes.push_back(stopCode);
        }
    }

    return stopCodes;
}

// Обратното на splitStopCodes: слепя кодовете с ';'.
static string joinStopCodes(const vector<string> &stopCodes)
{
    string result;

    for (size_t i = 0; i < stopCodes.size(); i++)
    {
        result += stopCodes[i];

        if (i + 1 < stopCodes.size())
        {
            result += ';';
        }
    }

    return result;
}

// Default конструктор, който не прави нищо. Подходящ за работа само в паметта.
TransportManager::TransportManager() = default;

// Конструктор, който зарежда данните от три CSV файла.
//  Редът на зареждане е задължителен: линиите проверяват, че спирките им
//  съществуват, а разписанията проверяват, че линиите съществуват.
TransportManager::TransportManager(
    const string &stopsFile,
    const string &linesFile,
    const string &schedulesFile)
    : stopsFile(stopsFile),
      linesFile(linesFile),
      schedulesFile(schedulesFile)
{
    loadBusStops();
    loadTransitLines();
    loadSchedules();
}

// Добавя спирка в колекцията. Проверява дали вече съществува.
// Кодът е уникален идентификатор: втора спирка със същия код се отхвърля.
void TransportManager::addBusStop(const BusStop &stop)
{
    string stopCode = stop.getCode();

    if (busStops.find(stopCode) != busStops.end())
    {
        throw invalid_argument(
            "Спирка с този код вече съществува.");
    }

    busStops.emplace(stopCode, stop);
}

// Премахва спирка от колекцията. Освен самата спирка се чисти и всеки маршрут,
// който я съдържа, за да не остане код, сочещ към несъществуваща спирка.
void TransportManager::removeBusStop(const string &stopCode)
{
    auto it = busStops.find(stopCode);

    if (it == busStops.end())
    {
        throw invalid_argument(
            "Спирка с този код не съществува.");
    }

    for (auto &lineEntry : transitLines)
    {
        TransitLine &line = lineEntry.second;

        const vector<string> &stopCodes = line.getStopCodes();

        auto stopIt = find(
            stopCodes.begin(),
            stopCodes.end(),
            stopCode);

        if (stopIt != stopCodes.end())
        {
            line.removeStop(stopCode);
        }
    }

    busStops.erase(it);
}

// Връща копие на спирката, така че извикващият не може да промени
// вътрешното състояние на мениджъра.
BusStop TransportManager::getBusStop(
    const string &stopCode) const
{
    auto it = busStops.find(stopCode);

    if (it == busStops.end())
    {
        throw invalid_argument(
            "Спирка с този код не съществува.");
    }

    return it->second;
}

// Добавя транспортна линия в колекцията.
//  Линията се приема само ако номерът ѝ е свободен и всички спирки
//  в маршрута вече са регистрирани.
void TransportManager::addTransitLine(const TransitLine &line)
{
    int lineNumber = line.getLineNumber();

    if (transitLines.find(lineNumber) != transitLines.end())
    {
        throw invalid_argument(
            "Линия с този номер вече съществува.");
    }

    for (const string &stopCode : line.getStopCodes())
    {
        if (busStops.find(stopCode) == busStops.end())
        {
            throw invalid_argument(
                "Не може да се добави линия: спирка в маршрута не съществува.");
        }
    }

    transitLines.emplace(lineNumber, line);
}

// Разписанието на линията се изтрива заедно с нея. Ако линията няма
// разписание, erase просто не прави нищо.
void TransportManager::removeTransitLine(int lineNumber)
{
    auto it = transitLines.find(lineNumber);

    if (it == transitLines.end())
    {
        throw invalid_argument(
            "Линия с този номер не съществува.");
    }

    schedules.erase(lineNumber);

    transitLines.erase(it);
}

// Връща копие на линията, така че извикващият не може да промени
// вътрешното състояние на мениджъра.
TransitLine TransportManager::getTransitLine(
    int lineNumber) const
{
    auto it = transitLines.find(lineNumber);

    if (it == transitLines.end())
    {
        throw invalid_argument(
            "Линия с този номер не съществува.");
    }

    return it->second;
}

// Добавя съществуваща спирка в края на маршрута на съществуваща линия.
// Дубликатите се хващат от TransitLine::addStop.
void TransportManager::assignStopToLine(
    int lineNumber,
    const string &stopCode)
{
    auto lineIt = transitLines.find(lineNumber);

    if (lineIt == transitLines.end())
    {
        throw invalid_argument(
            "Линия с този номер не съществува.");
    }

    auto stopIt = busStops.find(stopCode);

    if (stopIt == busStops.end())
    {
        throw invalid_argument(
            "Спирка с този код не съществува.");
    }

    lineIt->second.addStop(stopCode);
}

// Премахва съществуваща спирка от маршрута на съществуваща линия.
void TransportManager::removeStopFromLine(
    int lineNumber,
    const string &stopCode)
{
    auto lineIt = transitLines.find(lineNumber);

    if (lineIt == transitLines.end())
    {
        throw invalid_argument(
            "Линия с този номер не съществува.");
    }

    lineIt->second.removeStop(stopCode);
}

// Обхожда всички линии и събира номерата на тези, в чийто маршрут
// присъства кодът. Редът в резултата не е определен, защото линиите
// са в unordered_map.
vector<int> TransportManager::findLinesByStop(
    const string &stopCode) const
{
    if (busStops.find(stopCode) == busStops.end())
    {
        throw invalid_argument(
            "Спирка с този код не съществува.");
    }

    vector<int> result;

    for (const auto &lineEntry : transitLines)
    {
        const TransitLine &line = lineEntry.second;

        const vector<string> &stopCodes = line.getStopCodes();

        if (find(
                stopCodes.begin(),
                stopCodes.end(),
                stopCode) != stopCodes.end())
        {
            result.push_back(line.getLineNumber());
        }
    }

    return result;
}

// Добавя разписание за съществуваща линия.
// Всяка линия има най-много едно разписание.
void TransportManager::addSchedule(const Schedule &schedule)
{
    int lineNumber = schedule.getLineNumber();

    if (transitLines.find(lineNumber) == transitLines.end())
    {
        throw invalid_argument(
            "Не може да се добави разписание: линията не съществува.");
    }

    if (schedules.find(lineNumber) != schedules.end())
    {
        throw invalid_argument(
            "Разписание за тази транспортна линия вече съществува.");
    }

    schedules.emplace(lineNumber, schedule);
}

// Извежда началния и крайния час, после всички часове на тръгване.
// Интервалът се взема от линията, а не от разписанието.
void TransportManager::displaySchedule(int lineNumber) const
{
    auto lineIt = transitLines.find(lineNumber);

    if (lineIt == transitLines.end())
    {
        throw invalid_argument(
            "Линия с този номер не съществува.");
    }

    auto scheduleIt = schedules.find(lineNumber);

    if (scheduleIt == schedules.end())
    {
        throw invalid_argument(
            "Разписание за тази транспортна линия не съществува.");
    }

    const TransitLine &line = lineIt->second;
    const Schedule &schedule = scheduleIt->second;

    cout << schedule << endl;

    vector<string> departureTimes =
        schedule.generateDepartureTimes(
            line.getMovementInterval());

    cout << "\nЧасове на тръгване:\n";

    for (const string &time : departureTimes)
    {
        cout << time << endl;
    }
}

// Извежда всички спирки в колекцията. Редът не е определен, защото спирките
// са в unordered_map.
void TransportManager::displayAllBusStops() const
{
    if (busStops.empty())
    {
        cout << "Няма регистрирани спирки.\n";
        return;
    }

    cout << "\n--- Всички спирки ---\n";

    for (const auto &stopEntry : busStops)
    {
        cout << stopEntry.second << endl;
    }
}

// Извежда всички линии в колекцията. Редът не е определен, защото линиите
// са в unordered_map.
void TransportManager::displayAllTransitLines() const
{
    if (transitLines.empty())
    {
        cout << "Няма регистрирани транспортни линии.\n";
        return;
    }

    cout << "\n--- Всички транспортни линии ---\n";

    for (const auto &lineEntry : transitLines)
    {
        cout << lineEntry.second << endl;
    }
}

// Показва маршрута с имената на спирките, разделени със стрелки:
// Централна гара [S001] -> Лъвов мост [S002]
void TransportManager::displayRoute(int lineNumber) const
{
    auto lineIt = transitLines.find(lineNumber);

    if (lineIt == transitLines.end())
    {
        throw invalid_argument(
            "Линия с този номер не съществува.");
    }

    const TransitLine &line = lineIt->second;

    cout << "\nМаршрут на линия "
         << line.getLineNumber()
         << " ("
         << transportTypeToString(line.getTransportType())
         << "):\n";

    const vector<string> &stopCodes = line.getStopCodes();

    if (stopCodes.empty())
    {
        cout << "Няма добавени спирки.\n";
        return;
    }

    for (size_t i = 0; i < stopCodes.size(); i++)
    {
        auto stopIt = busStops.find(stopCodes[i]);

        // Не би трябвало да се случва, защото премахването на спирка я чисти
        // от маршрутите, но проверката пази от срив при повредено състояние.
        if (stopIt == busStops.end())
        {
            cout << stopCodes[i]
                 << " (неизвестна спирка)";
        }
        else
        {
            cout << stopIt->second.getName()
                 << " [" << stopIt->second.getCode() << "]";
        }

        if (i + 1 < stopCodes.size())
        {
            cout << " -> ";
        }
    }

    cout << endl;
}

// Формат: code,name,address,latitude,longitude
// Първият ред е заглавен и се пропуска. Всеки ред минава през
// конструктора на BusStop, така че валидацията е същата като при ръчно
// въвеждане. Първият невалиден ред прекъсва зареждането с изключение.
void TransportManager::loadBusStops()
{
    ifstream file(stopsFile);

    if (!file.is_open())
    {
        throw runtime_error(
            "Не може да се отвори CSV файлът със спирки: " + stopsFile);
    }

    string line;
    bool firstLine = true;

    while (getline(file, line))
    {
        if (line.empty())
        {
            continue;
        }

        vector<string> fields = parseCsvLine(line);

        // Заглавният ред се разпознава по първото поле.
        if (firstLine)
        {
            firstLine = false;

            if (!fields.empty() && trim(fields[0]) == "code")
            {
                continue;
            }
        }

        if (fields.size() != 5)
        {
            throw invalid_argument(
                "Невалиден ред в CSV файла със спирки. Очакват се 5 полета.");
        }

        string code = trim(fields[0]);
        string name = trim(fields[1]);
        string address = trim(fields[2]);

        // stod хвърля изключение, ако полето не е число.
        double latitude = stod(trim(fields[3]));
        double longitude = stod(trim(fields[4]));

        BusStop stop(
            code,
            name,
            address,
            latitude,
            longitude);

        addBusStop(stop);
    }

    file.close();
}

// Формат: lineNumber,transportType,movementInterval,stopCodes
// Кодовете на спирките са в едно поле, разделени с ';'.
// Зарежда се след спирките, защото addTransitLine проверява маршрута.
void TransportManager::loadTransitLines()
{
    ifstream file(linesFile);

    if (!file.is_open())
    {
        throw runtime_error(
            "Не може да се отвори CSV файлът с транспортни линии: " + linesFile);
    }

    string line;
    bool firstLine = true;

    while (getline(file, line))
    {
        if (line.empty())
        {
            continue;
        }

        vector<string> fields = parseCsvLine(line);

        if (firstLine)
        {
            firstLine = false;

            if (!fields.empty() &&
                trim(fields[0]) == "lineNumber")
            {
                continue;
            }
        }

        if (fields.size() != 4)
        {
            throw invalid_argument(
                "Невалиден ред в CSV файла с транспортни линии. Очакват се 4 полета.");
        }

        int lineNumber = stoi(trim(fields[0]));

        TransportType type =
            parseTransportType(trim(fields[1]));

        int movementInterval = stoi(trim(fields[2]));

        vector<string> stopCodes =
            splitStopCodes(fields[3]);

        TransitLine transitLine(
            lineNumber,
            type,
            stopCodes,
            movementInterval);

        addTransitLine(transitLine);
    }

    file.close();
}

// Презаписва целия файл със заглавен ред и по един ред за спирка.
// setprecision(15) пази координатите с достатъчно знаци, защото по
// подразбиране потокът закръглява до 6 значещи цифри.
void TransportManager::saveBusStops() const
{
    ofstream file(stopsFile);

    if (!file.is_open())
    {
        throw runtime_error(
            "Не може да се запише CSV файлът със спирки: " + stopsFile);
    }

    file << "code,name,address,latitude,longitude\n";

    for (const auto &stopEntry : busStops)
    {
        const BusStop &stop = stopEntry.second;

        file << escapeCsvField(stop.getCode()) << ","
             << escapeCsvField(stop.getName()) << ","
             << escapeCsvField(stop.getAddress()) << ","
             << setprecision(15) << stop.getLatitude() << ","
             << setprecision(15) << stop.getLongitude()
             << "\n";
    }

    file.close();
}

// Презаписва целия файл със заглавен ред и по един ред за линия.
// Кодовете на спирките се сливат в едно поле, разделени с
// ';'.
void TransportManager::saveTransitLines() const
{
    ofstream file(linesFile);

    if (!file.is_open())
    {
        throw runtime_error(
            "Не може да се запише CSV файлът с транспортни линии: " + linesFile);
    }

    file << "lineNumber,transportType,movementInterval,stopCodes\n";

    for (const auto &lineEntry : transitLines)
    {
        const TransitLine &line = lineEntry.second;

        file << line.getLineNumber() << ","
             << escapeCsvField(
                    transportTypeToString(line.getTransportType()))
             << ","
             << line.getMovementInterval()
             << ","
             << escapeCsvField(
                    joinStopCodes(line.getStopCodes()))
             << "\n";
    }

    file.close();
}

// Зареждане на разписанията от CSV.
//  Формат: lineNumber,startTime,endTime
//  Зарежда се последно, защото addSchedule проверява, че линията съществува.
void TransportManager::loadSchedules()
{
    ifstream file(schedulesFile);

    if (!file.is_open())
    {
        throw runtime_error(
            "Не може да се отвори CSV файлът с разписания: " + schedulesFile);
    }

    string line;
    bool firstLine = true;

    while (getline(file, line))
    {
        if (line.empty())
        {
            continue;
        }

        vector<string> fields = parseCsvLine(line);

        if (firstLine)
        {
            firstLine = false;

            if (!fields.empty() &&
                trim(fields[0]) == "lineNumber")
            {
                continue;
            }
        }

        if (fields.size() != 3)
        {
            throw invalid_argument(
                "Невалиден ред в CSV файла с разписания. Очакват се 3 полета.");
        }

        int lineNumber = stoi(trim(fields[0]));

        string startTime = trim(fields[1]);
        string endTime = trim(fields[2]);

        Schedule schedule(
            lineNumber,
            startTime,
            endTime);

        addSchedule(schedule);
    }

    file.close();
}

// Презаписва целия файл със заглавен ред и по един ред за разписание.
void TransportManager::saveSchedules() const
{
    ofstream file(schedulesFile);

    if (!file.is_open())
    {
        throw runtime_error(
            "Не може да се запише CSV файлът с разписания: " + schedulesFile);
    }

    file << "lineNumber,startTime,endTime\n";

    for (const auto &scheduleEntry : schedules)
    {
        const Schedule &schedule = scheduleEntry.second;

        file << schedule.getLineNumber() << ","
             << schedule.getStartTime() << ","
             << schedule.getEndTime()
             << "\n";
    }

    file.close();
}

// Извеждане
ostream &operator<<(
    ostream &out,
    const TransportManager &manager)
{
    out << "Transport Manager";

    return out;
}
