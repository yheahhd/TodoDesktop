#include "todoitemwidget.h"
#include <QHBoxLayout>
#include <QInputDialog>
#include <QMessageBox>

TodoItemWidget::TodoItemWidget(const TodoItem &item, QWidget *parent)
    : QWidget(parent), m_item(item), m_editMode(false)
{
    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setContentsMargins(5, 5, 5, 5);
    m_label = new QLabel(m_item.getDisplayText());
    m_label->setWordWrap(true);
    m_label->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    layout->addWidget(m_label);

    m_bgColor = Qt::white;
    m_textColor = Qt::black;
    setColors(m_bgColor, m_textColor);
}

void TodoItemWidget::updateItem(const TodoItem &item)
{
    m_item = item;
    m_label->setText(m_item.getDisplayText());
}

void TodoItemWidget::setColors(const QColor &bgColor, const QColor &textColor)
{
    m_bgColor = bgColor;
    m_textColor = textColor;
    QString style = QString("QLabel { background-color: %1; color: %2; border-radius: 4px; padding: 5px; }")
                        .arg(bgColor.name(QColor::HexArgb))
                        .arg(textColor.name(QColor::HexArgb));
    m_label->setStyleSheet(style);
}

void TodoItemWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && hasCommand() && !m_editMode) {
        emit runCommand(m_item.command);
    }
    QWidget::mousePressEvent(event);
}

void TodoItemWidget::contextMenuEvent(QContextMenuEvent *event)
{
    QMenu menu;
    QAction *completeAction = new QAction("标记完成", &menu);
    QAction *editAction = new QAction("编辑内容", &menu);
    QAction *runAction = nullptr;
    if (hasCommand()) {
        runAction = new QAction("执行命令", &menu);
    }

    connect(completeAction, &QAction::triggered, this, [this]() {
        emit completed(m_item.id);
    });
    connect(editAction, &QAction::triggered, this, [this]() {
        emit editRequested(m_item.id);
    });
    if (runAction) {
        connect(runAction, &QAction::triggered, this, [this]() {
            emit runCommand(m_item.command);
        });
        menu.addAction(runAction);
    }

    menu.addAction(completeAction);
    menu.addAction(editAction);
    menu.exec(event->globalPos());
}
