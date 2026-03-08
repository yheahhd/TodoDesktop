#include <QApplication>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QAction>
#include <QMessageBox>
#include <QPainter>
#include <QSharedMemory>
#include <QLocalServer>
#include <QLocalSocket>
#include <windows.h>
#include <shellapi.h>
#include "mainwindow.h"
#include "todomanager.h"

bool isRunningAsAdmin() {
    BOOL isAdmin = FALSE;
    PSID adminGroup = nullptr;
    SID_IDENTIFIER_AUTHORITY ntAuthority = SECURITY_NT_AUTHORITY;
    if (AllocateAndInitializeSid(&ntAuthority, 2, SECURITY_BUILTIN_DOMAIN_RID,
                                 DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, &adminGroup)) {
        CheckTokenMembership(nullptr, adminGroup, &isAdmin);
        FreeSid(adminGroup);
    }
    return isAdmin;
}

bool checkDataDirectoryWritable() {
    QString dataPath = TodoManager::instance().getDataPath();
    QFileInfo info(dataPath);
    QDir dir(info.absolutePath());
    if (!dir.exists()) {
        if (!dir.mkpath(".")) {
            return false;
        }
    }
    QString testFile = dir.absoluteFilePath("write_test.tmp");
    QFile file(testFile);
    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }
    file.close();
    file.remove();
    return true;
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    app.setApplicationName("桌面待办事项");
    app.setApplicationVersion("1.0.0");
    app.setOrganizationName("TodoDesktop");
    app.setOrganizationDomain("tododesktop.com");

    app.setAttribute(Qt::AA_EnableHighDpiScaling);
    app.setAttribute(Qt::AA_UseHighDpiPixmaps);

    QSharedMemory sharedMemory("TodoDesktop_UniqueKey");
    if (!sharedMemory.create(1)) {
        QLocalSocket socket;
        socket.connectToServer("TodoDesktop_Server");
        if (socket.waitForConnected(1000)) {
            socket.write("show");
            socket.waitForBytesWritten(1000);
        }
        return 0;
    }

    QLocalServer server;
    server.listen("TodoDesktop_Server");
    QObject::connect(&server, &QLocalServer::newConnection, [&]() {
        QLocalSocket *client = server.nextPendingConnection();
        if (client) {
            client->waitForReadyRead(1000);
            if (client->readAll() == "show") {
                foreach (QWidget *widget, QApplication::topLevelWidgets()) {
                    MainWindow *mainWin = qobject_cast<MainWindow*>(widget);
                    if (mainWin) {
                        mainWin->showMainWindow();
                        break;
                    }
                }
            }
            client->deleteLater();
        }
    });

    if (!checkDataDirectoryWritable()) {
        QString appPath = QCoreApplication::applicationDirPath();
        bool inProgramFiles = appPath.contains("Program Files", Qt::CaseInsensitive);
        QString message = "数据文件夹创建失败，请确保程序有权限在以下位置写入文件：\n" +
                          TodoManager::instance().getDataPath() + "\n\n";
        if (inProgramFiles) {
            message += "由于程序位于 Program Files 目录下，建议以管理员身份运行。\n"
                       "请右键点击程序，选择“以管理员身份运行”。";
        } else {
            message += "请检查文件夹权限，或尝试以管理员身份运行。";
        }
        QMessageBox::critical(nullptr, "启动失败", message);
        return 1;
    }

    QString appPath = QCoreApplication::applicationDirPath();
    if (appPath.contains("Program Files", Qt::CaseInsensitive) && !isRunningAsAdmin()) {
        QMessageBox::warning(nullptr, "权限提示",
                             "程序正在 Program Files 目录下运行，但没有管理员权限。\n"
                             "这可能导致数据保存失败。建议以管理员身份重新启动。");
    }

    MainWindow window;

    QObject::connect(&app, &QApplication::aboutToQuit, &window, &MainWindow::saveContainerSettings);

    QSystemTrayIcon *trayIcon = new QSystemTrayIcon();

    // 尝试加载图标资源
    QIcon trayIconResource(":/resources/icon.ico");
    if (!trayIconResource.isNull()) {
        trayIcon->setIcon(trayIconResource);
    } else {
        // 如果资源加载失败，手动绘制一个简单图标
        QPixmap pixmap(32, 32);
        pixmap.fill(Qt::transparent);
        QPainter painter(&pixmap);
        painter.setBrush(QColor(76, 175, 80));
        painter.setPen(Qt::NoPen);
        painter.drawEllipse(4, 4, 24, 24);
        painter.setPen(Qt::white);
        painter.setFont(QFont("Arial", 16));
        painter.drawText(pixmap.rect(), Qt::AlignCenter, "T");
        trayIcon->setIcon(QIcon(pixmap));
    }

    QMenu *trayMenu = new QMenu();
    QAction *showAction = new QAction("打开主界面");
    QAction *refreshAction = new QAction("刷新任务");
    QAction *addTodoAction = new QAction("添加待办事项");
    QAction *exitAction = new QAction("退出");

    trayMenu->addAction(showAction);
    trayMenu->addAction(refreshAction);
    trayMenu->addAction(addTodoAction);
    trayMenu->addSeparator();
    trayMenu->addAction(exitAction);

    trayIcon->setContextMenu(trayMenu);

    QObject::connect(showAction, &QAction::triggered, &window, &MainWindow::showMainWindow);
    QObject::connect(refreshAction, &QAction::triggered, [&window]() {
        window.refreshAll();
        QMessageBox::information(&window, "提示", "任务已刷新");
    });
    QObject::connect(addTodoAction, &QAction::triggered, [&window]() {
        window.showMainWindow();
        window.activateWindow();
    });
    static bool exiting = false;
    QObject::connect(exitAction, &QAction::triggered, [&]() {
        if (exiting) return;
        exiting = true;

        window.saveContainerSettings();

        QTimer::singleShot(0, qApp, &QApplication::quit);
    });

    QObject::connect(trayIcon, &QSystemTrayIcon::activated, [&](QSystemTrayIcon::ActivationReason reason) {
        if (reason == QSystemTrayIcon::DoubleClick) {
            window.showMainWindow();
        }
    });

    trayIcon->show();
    trayIcon->setToolTip("桌面待办事项 - 双击打开主界面");
    window.show();

    return app.exec();
}
