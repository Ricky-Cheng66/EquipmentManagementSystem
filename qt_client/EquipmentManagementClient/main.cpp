#include "mainwindow.h"
#include "logindialog.h"
#include <QApplication>
#include <QFontDatabase>
#include <QFile>
#include <QFileInfo>
#include <QDebug>
#include <QPushButton>
#include <QVBoxLayout>
#include <QWidget>
#include <QMessageBox>

// 定义退出码
#define EXIT_CODE_RELOGOUT 1001

// 全局样式和字体加载函数
bool setupApplicationStyle(QApplication &app)
{
    // 1. 加载QSS样式 - 使用Qt资源系统
    QString qssPath = ":/resources/resources/style.qss";
    QFile styleFile(qssPath);

    if (styleFile.open(QFile::ReadOnly)) {
        QString styleSheet = QString::fromUtf8(styleFile.readAll());
        app.setStyleSheet(styleSheet);
        styleFile.close();
        qDebug() << "✅ QSS样式从资源文件加载成功";
    } else {
        qDebug() << "❌ QSS样式加载失败，路径:" << qssPath;
        qDebug() << "❌ 错误信息:" << styleFile.errorString();
    }

    // 2. 加载字体文件
    QString fontPath = ":/resources/resources/fonts/fontawesome-webfont.ttf";

    int fontId = QFontDatabase::addApplicationFont(fontPath);
    if (fontId == -1) {
        // 尝试备选路径
        fontPath = QCoreApplication::applicationDirPath() + "/resources/fonts/fontawesome-webfont.ttf";
        fontId = QFontDatabase::addApplicationFont(fontPath);
        if (fontId == -1) {
            qDebug() << "❌ 字体加载失败!";
            return false;
        }
    }

    // 获取加载的字体族
    QStringList loadedFamilies = QFontDatabase::applicationFontFamilies(fontId);
    qDebug() << "✅ 字体加载成功!";
    for (int i = 0; i < loadedFamilies.size(); ++i) {
        qDebug() << "  字体族 [" << i << "]: " << loadedFamilies.at(i);
    }

    // 3. 设置应用程序默认字体，避免字体警告
    QFont defaultFont("Microsoft YaHei", 9);
    app.setFont(defaultFont);

    qDebug() << "✅ 应用程序默认字体已设置: " << defaultFont.family()
             << ", 大小: " << defaultFont.pointSize();

    return true;
}

int main(int argc, char *argv[]) {
    QApplication a(argc, argv);

    // 设置应用程序信息
    a.setApplicationName("校园设备综合管理系统");
    a.setApplicationVersion("1.0.0");
    a.setOrganizationName("校园设备管理中心");

    // 加载样式和字体
    if (!setupApplicationStyle(a)) {
        QMessageBox::critical(nullptr, "启动失败",
                              "应用程序样式加载失败，请检查资源文件路径。\n程序将继续运行。");
        // 不返回，继续运行程序
    }

    int exitCode = 0;

    do {
        // 创建TCP客户端和消息分发器
        TcpClient* tcpClient = new TcpClient();
        MessageDispatcher* dispatcher = new MessageDispatcher(tcpClient);

        // 创建并显示登录对话框
        LoginDialog loginDialog(tcpClient, dispatcher);
        loginDialog.setStyleSheet(a.styleSheet());

        if (loginDialog.exec() != QDialog::Accepted) {
            // 用户取消登录
            qDebug() << "用户取消登录，程序退出";
            delete dispatcher;
            delete tcpClient;
            return 0;
        }

        // 获取登录信息
        QString username = loginDialog.getUsername();
        QString role = loginDialog.getUserRole();
        int userId = loginDialog.getUserId();  // 获取用户ID

        qDebug() << "✅ 用户登录成功，用户ID:" << userId << "用户名:" << username << "角色:" << role;

        // 创建主窗口
        MainWindow w(tcpClient, dispatcher, username, role, userId);  // 传递用户ID
        w.show();

        exitCode = a.exec();

        // 清理资源
        delete dispatcher;
        delete tcpClient;

    } while (exitCode == EXIT_CODE_RELOGOUT); // 如果是注销，重新循环显示登录对话框

    return exitCode;
}
