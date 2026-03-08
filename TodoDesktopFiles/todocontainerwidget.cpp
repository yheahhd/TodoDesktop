#include "todocontainerwidget.h"
#include <QPainter>
#include <QMenu>
#include <QAction>
#include <QInputDialog>
#include <QMessageBox>
#include <QSettings>
#include <QCoreApplication>
#include <windows.h>


TodoContainerWidget::TodoContainerWidget(QWidget *parent)
    : QWidget(parent)
{
    setWindowFlags(Qt::FramelessWindowHint | Qt::Tool);
    setAttribute(Qt::WA_TranslucentBackground);
    setMouseTracking(true);
    setFocusPolicy(Qt::ClickFocus);

    m_layout = new QVBoxLayout(this);
    m_layout->setContentsMargins(5, 5, 5, 5);
    m_layout->setSpacing(2);

    loadSettings();

    QSettings settings(QCoreApplication::organizationName(), QCoreApplication::applicationName());
    if (settings.contains("container/pos")) {
        move(settings.value("container/pos").toPoint());
    }
    if (settings.contains("container/size")) {
        resize(settings.value("container/size").toSize());
    } else {
        resize(300, 200);
    }
}

void TodoContainerWidget::loadSettings()
{
    QSettings settings(TodoManager::instance().getSettingsPath(), QSettings::IniFormat);
    m_normalBgColor = settings.value("container/normalBgColor", QColor(0,0,0,0)).value<QColor>();
    m_editBgColor = settings.value("container/editBgColor", QColor(255,255,255,100)).value<QColor>();
    m_commandItemBgColor = settings.value("container/commandItemBgColor", QColor(255,200,200,200)).value<QColor>();
    m_textColor = settings.value("container/textColor", QColor(0,0,0)).value<QColor>();
    m_commandTextColor = settings.value("container/commandTextColor", QColor(0,0,0)).value<QColor>();
    m_commandCooldown = settings.value("container/commandCooldown", 3).toInt();

    if (settings.contains("container/pos")) {
        move(settings.value("container/pos").toPoint());
    }
    if (settings.contains("container/size")) {
        resize(settings.value("container/size").toSize());
    } else {
        resize(300, 200);
    }
}

void TodoContainerWidget::saveSettings()
{
    QSettings settings(QCoreApplication::organizationName(), QCoreApplication::applicationName());
    settings.setValue("container/pos", pos());
    settings.setValue("container/size", size());
}


void TodoContainerWidget::refreshItems(const QList<TodoItem> &items)
{
    qDeleteAll(m_itemWidgets);
    m_itemWidgets.clear();

    for (const TodoItem &item : items) {
        if (item.completed) continue;
        TodoItemWidget *w = new TodoItemWidget(item, this);
        if (item.command.isEmpty()) {
            w->setColors(m_normalBgColor, m_textColor);
        } else {
            w->setColors(m_commandItemBgColor, m_commandTextColor);
        }
        connect(w, &TodoItemWidget::completed, this, [this](const QString &id) {
            TodoManager::instance().markCompleted(id);
        });
        connect(w, &TodoItemWidget::editRequested, this, [this](const QString &id) {
            auto allItems = TodoManager::instance().getAllTodos();
            for (const TodoItem &it : allItems) {
                if (it.id == id) {
                    bool ok;
                    QString newContent = QInputDialog::getText(this, "编辑内容", "内容:",
                                                               QLineEdit::Normal, it.content, &ok);
                    if (ok && !newContent.isEmpty() && newContent != it.content) {
                        TodoItem newItem = it;
                        newItem.content = newContent;
                        TodoManager::instance().removeTodo(id);
                        TodoManager::instance().addTodo(newItem);
                    }
                    break;
                }
            }
        });
        connect(w, &TodoItemWidget::runCommand, this, [this](const QString &cmd) {
            if (canRunCommand(cmd)) {
                std::wstring wcmd = cmd.toStdWString();
                STARTUPINFOW si = { sizeof(si) };
                PROCESS_INFORMATION pi;
                if (!CreateProcessW(nullptr, &wcmd[0], nullptr, nullptr, FALSE, 0, nullptr, nullptr, &si, &pi)) {
                    ShellExecuteW(nullptr, L"open", wcmd.c_str(), nullptr, nullptr, SW_SHOW);
                } else {
                    CloseHandle(pi.hProcess);
                    CloseHandle(pi.hThread);
                }
                m_lastCommandTimes[cmd] = QDateTime::currentDateTime();
            } else {
                QMessageBox::information(this, "提示", QString("命令执行过于频繁，请等待 %1 秒后再试。").arg(m_commandCooldown));
            }
        });
        m_layout->addWidget(w);
        m_itemWidgets.append(w);
    }

    updateMinSize();
    m_layout->activate();
    update();
    repaint();
    adjustSize();
}

void TodoContainerWidget::updateMinSize()
{
    int maxWidth = 0;
    int totalHeight = m_layout->contentsMargins().top() + m_layout->contentsMargins().bottom();
    for (int i = 0; i < m_itemWidgets.size(); ++i) {
        QWidget *w = m_itemWidgets[i];
        int wHint = w->sizeHint().width();
        if (wHint > maxWidth) maxWidth = wHint;
        totalHeight += w->sizeHint().height();
        if (i < m_itemWidgets.size() - 1) totalHeight += m_layout->spacing();
    }
    QSize minSize(maxWidth + m_layout->contentsMargins().left() + m_layout->contentsMargins().right(),
                  totalHeight);
    setMinimumSize(minSize);
    if (size().width() < minSize.width() || size().height() < minSize.height()) {
        resize(minSize);
    }
}

void TodoContainerWidget::setEditMode(bool edit)
{
    if (m_editMode == edit) return;
    m_editMode = edit;
    if (!m_editMode) {
        saveSettings();
    }
    update();
    emit editModeChanged(m_editMode);
}

void TodoContainerWidget::paintEvent(QPaintEvent *event)
{
    QWidget::paintEvent(event);
    if (m_editMode) {
        QPainter painter(this);
        painter.setBrush(m_editBgColor);
        painter.setPen(Qt::NoPen);
        painter.drawRect(rect());

        int handleSize = 12;
        int x = width() - handleSize;
        int y = height() - handleSize;
        painter.setBrush(QColor(255,193,7,180));
        painter.setPen(QPen(QColor(255,152,0),1));
        painter.drawRect(x,y,handleSize,handleSize);
        painter.setPen(QPen(QColor(255,152,0),2));
        painter.drawLine(x+3, y+handleSize-3, x+handleSize-3, y+3);
        painter.drawLine(x+6, y+handleSize-3, x+handleSize-3, y+6);
        painter.drawLine(x+9, y+handleSize-3, x+handleSize-3, y+9);
    }
}

bool TodoContainerWidget::isInResizeArea(const QPoint &pos) const
{
    if (!m_editMode) return false;
    int handleSize = 15;
    return (pos.x() > width() - handleSize && pos.y() > height() - handleSize);
}

bool TodoContainerWidget::canRunCommand(const QString &command)
{
    if (command.isEmpty()) return false;
    if (!m_lastCommandTimes.contains(command)) return true;
    QDateTime last = m_lastCommandTimes[command];
    return last.secsTo(QDateTime::currentDateTime()) >= m_commandCooldown;
}

void TodoContainerWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        if (isInResizeArea(event->pos())) {
            m_resizing = true;
            m_resizeStartPos = event->globalPos();
            m_resizeStartSize = size();
        } else if (m_editMode) {
            m_dragging = true;
            m_dragStartPos = event->globalPos() - frameGeometry().topLeft();
            setCursor(Qt::ClosedHandCursor);
        }
    }
}

void TodoContainerWidget::mouseMoveEvent(QMouseEvent *event)
{
    if (isInResizeArea(event->pos())) {
        setCursor(Qt::SizeFDiagCursor);
    } else if (m_editMode) {
        setCursor(Qt::ArrowCursor);
    } else {
        setCursor(Qt::ArrowCursor);
    }

    if (m_resizing) {
        QPoint delta = event->globalPos() - m_resizeStartPos;
        QSize newSize = m_resizeStartSize + QSize(delta.x(), delta.y());
        newSize = newSize.expandedTo(minimumSize());
        resize(newSize);
    } else if (m_dragging) {
        move(event->globalPos() - m_dragStartPos);
    }
}

void TodoContainerWidget::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        if (m_resizing) {
            m_resizing = false;
            setCursor(Qt::ArrowCursor);
        } else if (m_dragging) {
            m_dragging = false;
            setCursor(Qt::ArrowCursor);
        }
    }
}

void TodoContainerWidget::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        setEditMode(!m_editMode);
    }
}

void TodoContainerWidget::contextMenuEvent(QContextMenuEvent *event)
{
    QMenu menu;
    QAction *editModeAction = new QAction(m_editMode ? "退出编辑模式" : "进入编辑模式", &menu);
    connect(editModeAction, &QAction::triggered, this, [this]() { setEditMode(!m_editMode); });
    menu.addAction(editModeAction);
    menu.exec(event->globalPos());
}

void TodoContainerWidget::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
}
