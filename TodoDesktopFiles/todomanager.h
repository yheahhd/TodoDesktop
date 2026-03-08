#ifndef TODOMANAGER_H
#define TODOMANAGER_H

#include <QObject>
#include <QMap>
#include <QList>
#include <QDate>
#include <QPoint>
#include <QSize>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QFile>
#include <QDir>
#include <QSettings>
#include <QDateTime>
#include <QTimer>

struct TodoItem {
    QString id;
    QString content;
    int dayOfWeek;
    bool completed;
    QPoint position;
    QSize size;
    QString command;
    QDateTime completedTime;
    int completionCount;
    QDateTime lastCompletionDate;

    QString getDisplayText() const {
        QString text = content;
        if(completionCount > 0) {
            text += QString(" (已完成%1次)").arg(completionCount);
        }
        return text;
    }

    QString getToolTip() const {
        QString tip = content;
        if(completionCount > 0) {
            tip += QString("\n已完成次数: %1").arg(completionCount);
            if(lastCompletionDate.isValid()) {
                tip += QString("\n最后完成时间: %1").arg(lastCompletionDate.toString("yyyy-MM-dd hh:mm"));
            }
        }
        if(completed && completedTime.isValid()) {
            tip += QString("\n本次完成时间: %1").arg(completedTime.toString("yyyy-MM-dd hh:mm"));
        }
        if(!command.isEmpty()) {
            tip += QString("\n命令: %1").arg(command);
        }
        return tip;
    }

    bool canCompleteToday() const {
        if(!lastCompletionDate.isValid()) return true;
        QDateTime now = QDateTime::currentDateTime();
        if(lastCompletionDate.date() == now.date()) {
            return completionCount < 2;
        }
        return true;
    }

    QJsonObject toJson() const {
        QJsonObject obj;
        obj["id"] = id;
        obj["content"] = content;
        obj["dayOfWeek"] = dayOfWeek;
        obj["completed"] = completed;
        obj["position_x"] = position.x();
        obj["position_y"] = position.y();
        obj["width"] = size.width();
        obj["height"] = size.height();
        obj["command"] = command;
        obj["completionCount"] = completionCount;
        if(completedTime.isValid()) {
            obj["completedTime"] = completedTime.toString(Qt::ISODate);
        }
        if(lastCompletionDate.isValid()) {
            obj["lastCompletionDate"] = lastCompletionDate.toString(Qt::ISODate);
        }
        return obj;
    }

    static TodoItem fromJson(const QJsonObject &obj) {
        TodoItem item;
        item.id = obj["id"].toString();
        item.content = obj["content"].toString();
        item.dayOfWeek = obj["dayOfWeek"].toInt();
        item.completed = obj["completed"].toBool();
        item.position = QPoint(obj["position_x"].toInt(), obj["position_y"].toInt());
        item.size = QSize(obj["width"].toInt(300), obj["height"].toInt(80));
        item.command = obj["command"].toString();
        item.completionCount = obj["completionCount"].toInt();

        QString timeStr = obj["completedTime"].toString();
        if(!timeStr.isEmpty()) {
            item.completedTime = QDateTime::fromString(timeStr, Qt::ISODate);
        }

        QString lastDateStr = obj["lastCompletionDate"].toString();
        if(!lastDateStr.isEmpty()) {
            item.lastCompletionDate = QDateTime::fromString(lastDateStr, Qt::ISODate);
        }
        return item;
    }
};

class TodoManager : public QObject
{
    Q_OBJECT
public:
    static TodoManager& instance();

    QString getSettingsPath() const;
    void loadTodos();
    void saveTodos();

    QList<TodoItem> getTodosForDay(int dayOfWeek) const;
    QList<TodoItem> getAllTodos() const { return m_todos.values(); }
    QList<TodoItem> getActiveTodos() const;
    void addTodo(const TodoItem &todo);
    void removeTodo(const QString &id);
    void markCompleted(const QString &id);
    void updatePosition(const QString &id, const QPoint &pos);
    void updateSize(const QString &id, const QSize &size);

    void checkDailyReset();
    void checkWeeklyReset();

    void setAutoStart(bool enabled);
    bool isAutoStart() const;

    QString getCurrentDateString() const;
    int getCurrentDayOfWeek() const;

    QString getDataPath() const;

signals:
    void todosChanged();
    void dateChanged(const QString &dateString);

private:
    TodoManager(QObject *parent = nullptr);

    QMap<QString, TodoItem> m_todos;
    QDate m_lastCheckDate;
    int m_lastWeekNumber = -1;
    int m_lastYear = -1;
    QTimer *m_dateTimer;
    QString m_currentDateString;

private slots:
    void updateDate();
};

#endif
