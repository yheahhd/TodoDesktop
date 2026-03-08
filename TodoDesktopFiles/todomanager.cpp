#include "todomanager.h"
#include <QStandardPaths>
#include <QDebug>
#include <QCoreApplication>
#include <QTimer>

TodoManager::TodoManager(QObject *parent) : QObject(parent)
{
    loadTodos();

    m_dateTimer = new QTimer(this);
    m_dateTimer->setInterval(60000);
    connect(m_dateTimer, &QTimer::timeout, this, &TodoManager::updateDate);
    m_dateTimer->start();

    updateDate();
}

QString TodoManager::getSettingsPath() const
{
    return QFileInfo(getDataPath()).absolutePath() + "/settings.ini";
}

TodoManager& TodoManager::instance()
{
    static TodoManager instance;
    return instance;
}

QString TodoManager::getDataPath() const
{
    QString appDir = QCoreApplication::applicationDirPath();
    QString dataDir = appDir + "/data";
    QDir dir(dataDir);
    if (!dir.exists()) {
        dir.mkpath(".");
    }
    return dataDir + "/todos.json";
}

void TodoManager::updateDate()
{
    QString newDateString = getCurrentDateString();
    if(newDateString != m_currentDateString) {
        m_currentDateString = newDateString;
        emit dateChanged(m_currentDateString);
        checkDailyReset();
    }
}

QString TodoManager::getCurrentDateString() const
{
    QDate currentDate = QDate::currentDate();
    QString weekDay;

    switch(currentDate.dayOfWeek()) {
    case 1: weekDay = "星期一"; break;
    case 2: weekDay = "星期二"; break;
    case 3: weekDay = "星期三"; break;
    case 4: weekDay = "星期四"; break;
    case 5: weekDay = "星期五"; break;
    case 6: weekDay = "星期六"; break;
    case 7: weekDay = "星期日"; break;
    default: weekDay = ""; break;
    }

    return currentDate.toString("今天是yyyy年MM月dd日 ") + weekDay;
}

int TodoManager::getCurrentDayOfWeek() const
{
    int qtDay = QDate::currentDate().dayOfWeek();
    return (qtDay == 7) ? 0 : qtDay;
}

void TodoManager::loadTodos()
{
    QFile file(getDataPath());
    if(!file.open(QIODevice::ReadOnly)) {
        qDebug() << "无法打开待办事项文件，将创建新文件";
        m_lastCheckDate = QDate::currentDate();
        QDate current = QDate::currentDate();
        int year;
        int week = current.weekNumber(&year);
        m_lastYear = year;
        m_lastWeekNumber = week;
        return;
    }

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();

    if(doc.isObject()) {
        QJsonObject root = doc.object();
        QJsonArray todosArray = root["todos"].toArray();

        m_todos.clear();
        for(const QJsonValue &value : todosArray) {
            TodoItem item = TodoItem::fromJson(value.toObject());
            m_todos[item.id] = item;
        }

        m_lastCheckDate = QDate::fromString(root["lastCheckDate"].toString(), Qt::ISODate);
        if(!m_lastCheckDate.isValid()) {
            m_lastCheckDate = QDate::currentDate();
        }

        m_lastYear = root["lastYear"].toInt(-1);
        m_lastWeekNumber = root["lastWeek"].toInt(-1);
        if(m_lastYear == -1 || m_lastWeekNumber == -1) {
            QDate current = QDate::currentDate();
            int year;
            int week = current.weekNumber(&year);
            m_lastYear = year;
            m_lastWeekNumber = week;
        }
    }

    checkDailyReset();
}

void TodoManager::saveTodos()
{
    QJsonObject root;
    QJsonArray todosArray;

    for(const TodoItem &item : m_todos) {
        todosArray.append(item.toJson());
    }

    root["todos"] = todosArray;
    root["lastCheckDate"] = m_lastCheckDate.toString(Qt::ISODate);
    root["lastYear"] = m_lastYear;
    root["lastWeek"] = m_lastWeekNumber;

    QFile file(getDataPath());
    if(!file.open(QIODevice::WriteOnly)) {
        qDebug() << "无法保存待办事项文件";
        return;
    }

    file.write(QJsonDocument(root).toJson());
    file.close();

    emit todosChanged();
}

QList<TodoItem> TodoManager::getActiveTodos() const
{
    QList<TodoItem> result;
    int currentDay = getCurrentDayOfWeek();

    for(const TodoItem &item : m_todos) {
        bool shouldShow = false;

        if(item.dayOfWeek == -2) {
            shouldShow = !item.completed;
        }
        else if(item.dayOfWeek >= 0 && item.dayOfWeek <= 6) {
            if(item.dayOfWeek == currentDay && !item.completed) {
                shouldShow = true;
            }
        }
        else if(item.dayOfWeek == -1) {
            shouldShow = !item.completed;
        }

        if(shouldShow) {
            result.append(item);
        }
    }

    return result;
}

void TodoManager::checkDailyReset()
{
    QDate currentDate = QDate::currentDate();
    if(m_lastCheckDate != currentDate) {
        qDebug() << "日期变化，重置每日事项";

        for(auto it = m_todos.begin(); it != m_todos.end(); ++it) {
            if(it->dayOfWeek == -2 && it->completed) {
                it->completed = false;
                it->completedTime = QDateTime();
                qDebug() << "重置每日事项:" << it->content;
            }
        }

        m_lastCheckDate = currentDate;
    }

    int currentYear;
    int currentWeek = currentDate.weekNumber(&currentYear);
    if(currentYear != m_lastYear || currentWeek != m_lastWeekNumber) {
        qDebug() << "周数变化，重置每周事项";

        for(auto it = m_todos.begin(); it != m_todos.end(); ++it) {
            if(it->dayOfWeek >= 0 && it->dayOfWeek <= 6) {
                if(it->completed) {
                    it->completed = false;
                    it->completedTime = QDateTime();
                    qDebug() << "重置每周事项:" << it->content;
                }
            }
        }

        m_lastYear = currentYear;
        m_lastWeekNumber = currentWeek;
    }

    if(m_lastCheckDate != currentDate || currentYear != m_lastYear || currentWeek != m_lastWeekNumber) {
        saveTodos();
    }
}

QList<TodoItem> TodoManager::getTodosForDay(int dayOfWeek) const
{
    QList<TodoItem> result;
    int currentDay = getCurrentDayOfWeek();

    for(const TodoItem &item : m_todos) {
        bool include = false;

        if(dayOfWeek == -1) {
            include = !item.completed;
        }
        else if(dayOfWeek == -2) {
            include = (item.dayOfWeek == -2);
        }
        else {
            if(item.dayOfWeek == dayOfWeek) {
                include = true;
            }
            else if(item.dayOfWeek == -2) {
                include = true;
            }
        }

        if(include) {
            result.append(item);
        }
    }

    return result;
}

void TodoManager::addTodo(const TodoItem &todo)
{
    m_todos[todo.id] = todo;
    saveTodos();
}

void TodoManager::removeTodo(const QString &id)
{
    if(m_todos.contains(id)) {
        m_todos.remove(id);
        saveTodos();
        qDebug() << "删除任务:" << id;
    }
}

void TodoManager::markCompleted(const QString &id)
{
    if(m_todos.contains(id)) {
        TodoItem &item = m_todos[id];

        if(!item.canCompleteToday()) {
            qDebug() << "今天已完成超过2次，不能再完成:" << item.content;
            return;
        }

        QDateTime now = QDateTime::currentDateTime();

        item.completed = true;
        item.completedTime = now;
        item.completionCount++;
        item.lastCompletionDate = now;

        qDebug() << "标记完成:" << item.content << "完成次数:" << item.completionCount;

        if(item.dayOfWeek == -1) {
            m_todos.remove(id);
            qDebug() << "删除一次性任务:" << item.content;
        }

        saveTodos();
    }
}

void TodoManager::updatePosition(const QString &id, const QPoint &pos)
{
    if(m_todos.contains(id)) {
        m_todos[id].position = pos;
        saveTodos();
    }
}

void TodoManager::updateSize(const QString &id, const QSize &size)
{
    if(m_todos.contains(id)) {
        m_todos[id].size = size;
        saveTodos();
    }
}

void TodoManager::setAutoStart(bool enabled)
{
    QSettings settings("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run", QSettings::NativeFormat);
    QString appName = QCoreApplication::applicationName();
    QString appPath = QCoreApplication::applicationFilePath().replace('/', '\\');

    if (enabled) {
        settings.setValue(appName, appPath);
    } else {
        settings.remove(appName);
    }
}

bool TodoManager::isAutoStart() const
{
    QSettings settings("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run", QSettings::NativeFormat);
    QString appName = QCoreApplication::applicationName();
    return settings.contains(appName);
}
