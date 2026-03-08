#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTabWidget>
#include <QListWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QCheckBox>
#include <QSystemTrayIcon>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QCloseEvent>
#include <QComboBox>
#include <QGroupBox>
#include <QStatusBar>
#include "todocontainerwidget.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

public slots:
    void saveContainerSettings();
    void showMainWindow();
    void refreshAll();

protected:
    void closeEvent(QCloseEvent *event) override;

private slots:
    void addTodo();
    void removeSelectedTodo();
    void markTodoCompleted();
    void toggleEditMode();
    void onTodosChanged();
    void refreshDesktopWidgets();
    void showSettings();
    void onDateChanged(const QString &dateString);

private:
    void setupUI();
    void refreshTodoList();
    void createDayTab(int day, const QString &dayName);
    void addTodoWithDay(int dayOfWeek);

    QTabWidget *m_tabWidget;
    QList<QListWidget*> m_dayLists;
    QLineEdit *m_todoEdit;
    QCheckBox *m_dailyCheckbox;
    QCheckBox *m_weeklyCheckbox;
    QLineEdit *m_commandEdit;
    QPushButton *m_editPositionBtn;
    QPushButton *m_settingsBtn;
    QLabel *m_dateLabel;
    QLabel *m_weekLabel;
    bool m_editMode = false;
    TodoContainerWidget *m_containerWidget;
    QSystemTrayIcon *m_trayIcon;
};

#endif
