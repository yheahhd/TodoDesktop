#ifndef TODOITEMWIDGET_H
#define TODOITEMWIDGET_H

#include <QWidget>
#include <QLabel>
#include <QMouseEvent>
#include <QMenu>
#include <QAction>
#include "todomanager.h"

class TodoItemWidget : public QWidget
{
    Q_OBJECT
public:
    explicit TodoItemWidget(const TodoItem &item, QWidget *parent = nullptr);
    void updateItem(const TodoItem &item);
    void setColors(const QColor &bgColor, const QColor &textColor);
    QString getId() const { return m_item.id; }
    bool hasCommand() const { return !m_item.command.isEmpty(); }

signals:
    void completed(const QString &id);
    void editRequested(const QString &id);
    void runCommand(const QString &command);

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void contextMenuEvent(QContextMenuEvent *event) override;

private:
    TodoItem m_item;
    QLabel *m_label;
    bool m_editMode;
    QColor m_bgColor;
    QColor m_textColor;
};

#endif
