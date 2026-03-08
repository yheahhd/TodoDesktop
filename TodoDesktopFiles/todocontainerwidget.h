#ifndef TODOCONTAINERWIDGET_H
#define TODOCONTAINERWIDGET_H

#include <QWidget>
#include <QVBoxLayout>
#include <QList>
#include <QMap>
#include <QDateTime>
#include "todoitemwidget.h"
#include "todomanager.h"

class TodoContainerWidget : public QWidget
{
    Q_OBJECT
public:
    explicit TodoContainerWidget(QWidget *parent = nullptr);
    void refreshItems(const QList<TodoItem> &items);
    void setEditMode(bool edit);
    bool editMode() const { return m_editMode; }
    void loadSettings();
    void saveSettings();
    bool isEmpty() const { return m_itemWidgets.isEmpty(); }

signals:
    void editModeChanged(bool edit);
    void positionChanged(const QPoint &pos);
    void sizeChanged(const QSize &size);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void contextMenuEvent(QContextMenuEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    void updateMinSize();
    bool isInResizeArea(const QPoint &pos) const;
    bool canRunCommand(const QString &command);

    QVBoxLayout *m_layout;
    QList<TodoItemWidget*> m_itemWidgets;
    bool m_editMode = false;
    bool m_dragging = false;
    bool m_resizing = false;
    QPoint m_dragStartPos;
    QPoint m_resizeStartPos;
    QSize m_resizeStartSize;
    QColor m_normalBgColor;
    QColor m_editBgColor;
    QColor m_commandItemBgColor;
    QColor m_textColor;
    QColor m_commandTextColor;
    int m_commandCooldown;
    QMap<QString, QDateTime> m_lastCommandTimes;
};

#endif
