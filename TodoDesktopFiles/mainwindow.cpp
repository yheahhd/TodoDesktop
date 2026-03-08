#include "mainwindow.h"
#include <QUuid>
#include <QScrollArea>
#include <QTimer>
#include <QDebug>
#include <QMenu>
#include <QAction>
#include <QInputDialog>
#include <QCheckBox>
#include <QDialog>
#include <QVBoxLayout>
#include <QDialogButtonBox>
#include <QStatusBar>
#include <QGroupBox>
#include <QComboBox>
#include <QDate>
#include <QColorDialog>
#include <QSpinBox>
#include <QApplication>
#include <QSettings>
#include <QCoreApplication>
#include <QRadioButton>
#include <QButtonGroup>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setupUI();
    connect(&TodoManager::instance(), &TodoManager::todosChanged, this, &MainWindow::onTodosChanged);
    connect(&TodoManager::instance(), &TodoManager::dateChanged, this, &MainWindow::onDateChanged);

    qDebug() << "主窗口初始化完成";

    m_containerWidget = new TodoContainerWidget();
    connect(m_containerWidget, &TodoContainerWidget::editModeChanged, this, [this](bool edit){
        m_editMode = edit;
        m_editPositionBtn->setText(m_editMode ? "完成编辑" : "编辑位置");
    });

    refreshTodoList();
    refreshDesktopWidgets();

    onDateChanged(TodoManager::instance().getCurrentDateString());
}

MainWindow::~MainWindow()
{
    delete m_containerWidget;
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    QSettings settings(TodoManager::instance().getSettingsPath(), QSettings::IniFormat);
    QString closeAction = settings.value("mainwindow/closeAction", "ask").toString();

    if (closeAction == "quit") {
        QApplication::quit();
        event->accept();
    } else if (closeAction == "hide") {
        hide();
        event->ignore();
    } else {
        QMessageBox msgBox(this);
        msgBox.setWindowTitle("选择关闭行为");
        msgBox.setText("您希望点击关闭按钮后执行什么操作？\n您可以在设置中修改此选项。");
        QPushButton *quitBtn = msgBox.addButton("彻底关闭程序", QMessageBox::AcceptRole);
        QPushButton *hideBtn = msgBox.addButton("只隐藏主界面", QMessageBox::RejectRole);
        msgBox.setDefaultButton(hideBtn);
        msgBox.exec();

        if (msgBox.clickedButton() == quitBtn) {
            settings.setValue("mainwindow/closeAction", "quit");
            QApplication::quit();
        } else {
            settings.setValue("mainwindow/closeAction", "hide");
            hide();
            event->ignore();
        }
    }
}

void MainWindow::setupUI()
{
    setWindowTitle("桌面待办事项");
    setMinimumSize(700, 550);

    setWindowIcon(QIcon(":/resources/icon.ico"));

    QWidget *centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);

    QGroupBox *inputGroup = new QGroupBox("添加新待办事项");
    QVBoxLayout *inputGroupLayout = new QVBoxLayout(inputGroup);

    QHBoxLayout *contentLayout = new QHBoxLayout();
    contentLayout->addWidget(new QLabel("内容:"));
    m_todoEdit = new QLineEdit();
    m_todoEdit->setPlaceholderText("输入待办事项内容...");
    contentLayout->addWidget(m_todoEdit, 1);

    QHBoxLayout *optionLayout = new QHBoxLayout();

    m_dailyCheckbox = new QCheckBox("每日事项");
    m_weeklyCheckbox = new QCheckBox("每周固定日");
    QLabel *dayLabel = new QLabel("选择星期:");

    QComboBox *dayComboBox = new QComboBox();
    dayComboBox->addItem("周日", 0);
    dayComboBox->addItem("周一", 1);
    dayComboBox->addItem("周二", 2);
    dayComboBox->addItem("周三", 3);
    dayComboBox->addItem("周四", 4);
    dayComboBox->addItem("周五", 5);
    dayComboBox->addItem("周六", 6);
    dayComboBox->setEnabled(false);

    connect(m_dailyCheckbox, &QCheckBox::toggled, [dayComboBox, this](bool checked) {
        if(checked) {
            m_weeklyCheckbox->setChecked(false);
            dayComboBox->setEnabled(false);
        }
    });

    connect(m_weeklyCheckbox, &QCheckBox::toggled, [dayComboBox, this](bool checked) {
        if(checked) {
            m_dailyCheckbox->setChecked(false);
            dayComboBox->setEnabled(true);
        } else {
            dayComboBox->setEnabled(false);
        }
    });

    optionLayout->addWidget(m_dailyCheckbox);
    optionLayout->addWidget(m_weeklyCheckbox);
    optionLayout->addWidget(dayLabel);
    optionLayout->addWidget(dayComboBox);
    optionLayout->addStretch();

    QHBoxLayout *commandLayout = new QHBoxLayout();
    commandLayout->addWidget(new QLabel("命令:"));
    m_commandEdit = new QLineEdit();
    m_commandEdit->setPlaceholderText("可选：点击执行的命令（如程序路径）");
    commandLayout->addWidget(m_commandEdit, 1);

    QHBoxLayout *buttonLayout = new QHBoxLayout();
    QPushButton *addButton = new QPushButton("添加");
    QPushButton *removeButton = new QPushButton("删除选中");
    QPushButton *completeButton = new QPushButton("标记完成");
    m_editPositionBtn = new QPushButton("编辑位置");
    m_settingsBtn = new QPushButton("设置");

    buttonLayout->addWidget(addButton);
    buttonLayout->addWidget(removeButton);
    buttonLayout->addWidget(completeButton);
    buttonLayout->addWidget(m_editPositionBtn);
    buttonLayout->addWidget(m_settingsBtn);
    buttonLayout->addStretch();

    inputGroupLayout->addLayout(contentLayout);
    inputGroupLayout->addLayout(optionLayout);
    inputGroupLayout->addLayout(commandLayout);
    inputGroupLayout->addLayout(buttonLayout);

    mainLayout->addWidget(inputGroup);

    m_tabWidget = new QTabWidget();
    QStringList tabNames = {"每日事项", "周一", "周二", "周三", "周四", "周五", "周六", "周日", "所有任务"};
    for(int i = 0; i < tabNames.size(); i++) {
        createDayTab(i, tabNames[i]);
    }

    mainLayout->addWidget(m_tabWidget, 1);

    QStatusBar *statusBar = new QStatusBar();
    m_dateLabel = new QLabel();
    statusBar->addWidget(m_dateLabel);
    statusBar->addPermanentWidget(new QLabel(" | "));
    m_weekLabel = new QLabel();
    statusBar->addPermanentWidget(m_weekLabel);

    setStatusBar(statusBar);

    connect(addButton, &QPushButton::clicked, this, [this, dayComboBox]() {
        int dayOfWeek = -1;
        if(m_dailyCheckbox->isChecked()) {
            dayOfWeek = -2;
        } else if(m_weeklyCheckbox->isChecked()) {
            dayOfWeek = dayComboBox->currentData().toInt();
        } else {
            int currentTab = m_tabWidget->currentIndex();
            if(currentTab == 0) dayOfWeek = -2;
            else if(currentTab >= 1 && currentTab <= 6) dayOfWeek = currentTab;
            else if(currentTab == 7) dayOfWeek = 0;
            else dayOfWeek = -1;
        }
        addTodoWithDay(dayOfWeek);
    });

    connect(removeButton, &QPushButton::clicked, this, &MainWindow::removeSelectedTodo);
    connect(completeButton, &QPushButton::clicked, this, &MainWindow::markTodoCompleted);
    connect(m_editPositionBtn, &QPushButton::clicked, this, &MainWindow::toggleEditMode);
    connect(m_settingsBtn, &QPushButton::clicked, this, &MainWindow::showSettings);
    connect(m_todoEdit, &QLineEdit::returnPressed, this, [this]() { addTodo(); });
}

void MainWindow::createDayTab(int tabIndex, const QString &dayName)
{
    QWidget *tab = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(tab);
    QListWidget *listWidget = new QListWidget();
    listWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    m_dayLists.append(listWidget);
    layout->addWidget(listWidget);

    listWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(listWidget, &QListWidget::customContextMenuRequested, [this, listWidget, tabIndex](const QPoint &pos) {
        QListWidgetItem *item = listWidget->itemAt(pos);
        if(item) {
            QMenu menu;
            QAction *completeAction = new QAction("标记完成", &menu);
            QAction *deleteAction = new QAction("删除", &menu);
            QAction *editAction = new QAction("编辑", &menu);
            menu.addAction(completeAction);
            menu.addAction(deleteAction);
            menu.addAction(editAction);

            QString id = item->data(Qt::UserRole).toString();
            connect(completeAction, &QAction::triggered, [this, id]() {
                TodoManager::instance().markCompleted(id);
            });
            connect(deleteAction, &QAction::triggered, [this, id]() {
                TodoManager::instance().removeTodo(id);
            });
            connect(editAction, &QAction::triggered, [this, id]() {
                QList<TodoItem> allTodos = TodoManager::instance().getAllTodos();
                for(const TodoItem &todo : allTodos) {
                    if(todo.id == id) {
                        bool ok;
                        QString newContent = QInputDialog::getText(this, "编辑内容",
                                                                   "修改待办事项:",
                                                                   QLineEdit::Normal,
                                                                   todo.content, &ok);
                        if(ok && !newContent.isEmpty()) {
                            TodoItem newTodo = todo;
                            newTodo.content = newContent;
                            TodoManager::instance().removeTodo(id);
                            TodoManager::instance().addTodo(newTodo);
                        }
                        break;
                    }
                }
            });
            menu.exec(listWidget->mapToGlobal(pos));
        }
    });

    m_tabWidget->addTab(tab, dayName);
}

void MainWindow::addTodoWithDay(int dayOfWeek)
{
    QString content = m_todoEdit->text().trimmed();
    if(content.isEmpty()) {
        QMessageBox::warning(this, "提示", "请输入待办事项内容");
        return;
    }

    TodoItem item;
    item.id = QUuid::createUuid().toString();
    item.content = content;
    item.dayOfWeek = dayOfWeek;
    item.completed = false;
    item.position = QPoint(100, 100);
    item.command = m_commandEdit->text().trimmed();
    item.completionCount = 0;

    TodoManager::instance().addTodo(item);

    m_todoEdit->clear();
    m_commandEdit->clear();
    m_dailyCheckbox->setChecked(false);
    m_weeklyCheckbox->setChecked(false);

    if(dayOfWeek == -2) m_tabWidget->setCurrentIndex(0);
    else if(dayOfWeek >= 1 && dayOfWeek <= 6) m_tabWidget->setCurrentIndex(dayOfWeek);
    else if(dayOfWeek == 0) m_tabWidget->setCurrentIndex(7);
    else m_tabWidget->setCurrentIndex(8);

    refreshTodoList();
    refreshDesktopWidgets();
}

void MainWindow::addTodo()
{
    addTodoWithDay(-1);
}

void MainWindow::removeSelectedTodo()
{
    int currentTab = m_tabWidget->currentIndex();
    if(currentTab < 0 || currentTab >= m_dayLists.size()) return;
    QListWidget *currentList = m_dayLists[currentTab];
    QListWidgetItem *item = currentList->currentItem();
    if(!item) return;

    QString id = item->data(Qt::UserRole).toString();
    if(QMessageBox::question(this, "确认删除", "确定删除？", QMessageBox::Yes|QMessageBox::No) == QMessageBox::Yes) {
        TodoManager::instance().removeTodo(id);
    }
}

void MainWindow::markTodoCompleted()
{
    int currentTab = m_tabWidget->currentIndex();
    if(currentTab < 0 || currentTab >= m_dayLists.size()) return;
    QListWidget *currentList = m_dayLists[currentTab];
    QListWidgetItem *item = currentList->currentItem();
    if(!item) return;
    QString id = item->data(Qt::UserRole).toString();
    TodoManager::instance().markCompleted(id);
}

void MainWindow::toggleEditMode()
{
    m_editMode = !m_editMode;
    m_editPositionBtn->setText(m_editMode ? "完成编辑" : "编辑位置");
    if (m_containerWidget) {
        m_containerWidget->setEditMode(m_editMode);
    }
}

void MainWindow::onTodosChanged()
{
    refreshTodoList();
    refreshDesktopWidgets();
}

void MainWindow::refreshAll()
{
    onTodosChanged();
}

void MainWindow::onDateChanged(const QString &dateString)
{
    m_dateLabel->setText(dateString);
}

void MainWindow::refreshTodoList()
{
    for(int i = 0; i < m_dayLists.size(); i++) {
        QListWidget *listWidget = m_dayLists[i];
        listWidget->clear();

        int dayOfWeek = -3;
        if(i == 0) dayOfWeek = -2;
        else if(i >= 1 && i <= 6) dayOfWeek = i;
        else if(i == 7) dayOfWeek = 0;
        else dayOfWeek = -1;

        QList<TodoItem> todos = TodoManager::instance().getTodosForDay(dayOfWeek);
        for(const TodoItem &item : todos) {
            QString displayText = item.getDisplayText();
            if(!item.command.isEmpty()) displayText += " [命令]";
            QListWidgetItem *listItem = new QListWidgetItem(displayText);
            listItem->setData(Qt::UserRole, item.id);
            listItem->setToolTip(item.getToolTip());
            if(item.completed) {
                listItem->setForeground(Qt::gray);
                QFont font = listItem->font();
                font.setStrikeOut(true);
                listItem->setFont(font);
                listItem->setFlags(listItem->flags() & ~Qt::ItemIsSelectable);
            }
            listWidget->addItem(listItem);
        }
    }
}

void MainWindow::refreshDesktopWidgets()
{
    if (!m_containerWidget) return;

    QList<TodoItem> activeTodos = TodoManager::instance().getActiveTodos();
    bool wasEmpty = m_containerWidget->isEmpty();

    m_containerWidget->refreshItems(activeTodos);
    if (!m_containerWidget->isVisible()) {
        m_containerWidget->show();
    }
    m_containerWidget->raise();
    m_containerWidget->activateWindow();

    QRect screenGeometry = QApplication::primaryScreen()->availableGeometry();
    if (!screenGeometry.intersects(QRect(m_containerWidget->pos(), m_containerWidget->size()))) {
        m_containerWidget->move(100, 100);
    }

    if (wasEmpty && !activeTodos.isEmpty()) {
        m_containerWidget->setEditMode(true);
        m_editMode = true;
        m_editPositionBtn->setText("完成编辑");
    }
}
void MainWindow::showMainWindow()
{
    show();
    raise();
    activateWindow();
}

void MainWindow::saveContainerSettings()
{
    if (m_containerWidget)
        m_containerWidget->saveSettings();
}

void MainWindow::showSettings()
{
    QDialog dialog(this);
    dialog.setWindowTitle("设置");
    dialog.setFixedSize(500, 550);

    QVBoxLayout *layout = new QVBoxLayout(&dialog);

    QCheckBox *autoStartCheckbox = new QCheckBox("开机自启动");
    autoStartCheckbox->setChecked(TodoManager::instance().isAutoStart());

    QHBoxLayout *cooldownLayout = new QHBoxLayout();
    cooldownLayout->addWidget(new QLabel("命令冷却时间(秒):"));
    QSpinBox *cooldownSpin = new QSpinBox();
    cooldownSpin->setRange(0, 3600);

    QSettings fileSettings(TodoManager::instance().getSettingsPath(), QSettings::IniFormat);
    cooldownSpin->setValue(fileSettings.value("container/commandCooldown", 3).toInt());

    cooldownLayout->addWidget(cooldownSpin);
    cooldownLayout->addStretch();

    QGroupBox *colorGroup = new QGroupBox("颜色设置");
    QVBoxLayout *colorLayout = new QVBoxLayout(colorGroup);

    struct ColorRow {
        QHBoxLayout *layout;
        QLineEdit *edit;
    };
    auto createColorRow = [&](const QString &label, const QString &key, const QColor &defaultColor) -> ColorRow {
        QHBoxLayout *row = new QHBoxLayout();
        row->addWidget(new QLabel(label));
        QLineEdit *edit = new QLineEdit();
        edit->setFixedWidth(120);
        QColor current = fileSettings.value(key, defaultColor).value<QColor>();
        edit->setText(current.name(QColor::HexArgb));
        QPushButton *btn = new QPushButton("选择");
        row->addWidget(edit);
        row->addWidget(btn);
        row->addStretch();
        connect(btn, &QPushButton::clicked, [edit]() {
            QColor c = QColorDialog::getColor(QColor(edit->text()), nullptr, "选择颜色",
                                              QColorDialog::ShowAlphaChannel);
            if (c.isValid()) {
                edit->setText(c.name(QColor::HexArgb));
            }
        });
        return {row, edit};
    };

    auto editBgRow   = createColorRow("编辑模式背景色:", "container/editBgColor", QColor(255,255,255,100));
    auto cmdBgRow    = createColorRow("命令项背景色:", "container/commandItemBgColor", QColor(255,200,200,200));
    auto textRow     = createColorRow("正常文字颜色:", "container/textColor", QColor(0,0,0));
    auto cmdTextRow  = createColorRow("命令项文字颜色:", "container/commandTextColor", QColor(0,0,0));

    QPushButton *resetBtn = new QPushButton("重置为默认颜色");
    connect(resetBtn, &QPushButton::clicked, [&]() {
        editBgRow.edit->setText(QColor(255,255,255,100).name(QColor::HexArgb));
        cmdBgRow.edit->setText(QColor(255,200,200,200).name(QColor::HexArgb));
        textRow.edit->setText(QColor(0,0,0).name(QColor::HexArgb));
        cmdTextRow.edit->setText(QColor(0,0,0).name(QColor::HexArgb));
    });

    colorLayout->addLayout(editBgRow.layout);
    colorLayout->addLayout(cmdBgRow.layout);
    colorLayout->addLayout(textRow.layout);
    colorLayout->addLayout(cmdTextRow.layout);
    colorLayout->addWidget(resetBtn);

    layout->addWidget(autoStartCheckbox);
    layout->addLayout(cooldownLayout);
    layout->addWidget(colorGroup);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    layout->addWidget(buttonBox);

    connect(buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    if (dialog.exec() == QDialog::Accepted) {
        TodoManager::instance().setAutoStart(autoStartCheckbox->isChecked());

        fileSettings.setValue("container/editBgColor", QColor(editBgRow.edit->text()));
        fileSettings.setValue("container/commandItemBgColor", QColor(cmdBgRow.edit->text()));
        fileSettings.setValue("container/textColor", QColor(textRow.edit->text()));
        fileSettings.setValue("container/commandTextColor", QColor(cmdTextRow.edit->text()));
        fileSettings.setValue("container/commandCooldown", cooldownSpin->value());
        fileSettings.sync();
        m_containerWidget->loadSettings();
        m_containerWidget->refreshItems(TodoManager::instance().getActiveTodos());
        QMessageBox::information(this, "提示", "设置已保存");
    }

    QGroupBox *closeGroup = new QGroupBox("主窗口关闭行为");
    QVBoxLayout *closeLayout = new QVBoxLayout(closeGroup);
    QRadioButton *quitRadio = new QRadioButton("彻底退出程序");
    QRadioButton *hideRadio = new QRadioButton("隐藏到托盘（保留待办窗口）");
    QButtonGroup *closeActionGroup = new QButtonGroup(&dialog);
    closeActionGroup->addButton(quitRadio);
    closeActionGroup->addButton(hideRadio);

    QString currentAction = fileSettings.value("mainwindow/closeAction", "hide").toString();
    if (currentAction == "quit")
        quitRadio->setChecked(true);
    else
        hideRadio->setChecked(true);

    closeLayout->addWidget(quitRadio);
    closeLayout->addWidget(hideRadio);
    closeLayout->addStretch();

    layout->addWidget(closeGroup);

    if (quitRadio->isChecked())
        fileSettings.setValue("mainwindow/closeAction", "quit");
    else
        fileSettings.setValue("mainwindow/closeAction", "hide");

}
