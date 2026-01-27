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
    // 1. 加载QSS样式
    QString qssPath = ":/resources/resources/style.qss";
    QFile styleFile(qssPath);

    if (styleFile.open(QFile::ReadOnly)) {
        QString styleSheet = QString::fromUtf8(styleFile.readAll());

        // 移除所有包含 content 的样式
        QStringList styleLines = styleSheet.split('\n');
        QStringList cleanLines;
        for (const QString &line : styleLines) {
            if (!line.contains("content:")) {
                cleanLines.append(line);
            }
        }
        styleSheet = cleanLines.join('\n');

        // 添加基本的下拉框样式（不含content属性）
        styleSheet += "\n\n/* ========== 下拉框基本样式 ========== */\n"
                      "QComboBox {\n"
                      "    border: 1px solid #dcdde1;\n"
                      "    border-radius: 4px;\n"
                      "    padding: 6px 30px 6px 12px;\n"
                      "    background-color: white;\n"
                      "    min-height: 28px;\n"
                      "}\n"
                      "QComboBox::drop-down {\n"
                      "    border: none;\n"
                      "    width: 24px;\n"
                      "    border-left: 1px solid #dcdde1;\n"
                      "    border-top-right-radius: 4px;\n"
                      "    border-bottom-right-radius: 4px;\n"
                      "    background-color: #f8f9fa;\n"
                      "}\n"
                      "QComboBox QAbstractItemView {\n"
                      "    border: 1px solid #dcdde1;\n"
                      "    border-radius: 4px;\n"
                      "    background-color: white;\n"
                      "    selection-background-color: #4a69bd;\n"
                      "    selection-color: white;\n"
                      "}\n"
                      "QDateEdit, QTimeEdit, QDateTimeEdit {\n"
                      "    border: 1px solid #dcdde1;\n"
                      "    border-radius: 4px;\n"
                      "    padding: 6px 30px 6px 12px;\n"
                      "    background-color: white;\n"
                      "    min-height: 28px;\n"
                      "}\n"
                      "QDateEdit::drop-down, QTimeEdit::drop-down, QDateTimeEdit::drop-down {\n"
                      "    border: none;\n"
                      "    width: 24px;\n"
                      "    border-left: 1px solid #dcdde1;\n"
                      "    border-top-right-radius: 4px;\n"
                      "    border-bottom-right-radius: 4px;\n"
                      "    background-color: #f8f9fa;\n"
                      "}";

        app.setStyleSheet(styleSheet);
        styleFile.close();
        qDebug() << "✅ QSS样式加载成功（包含下拉框修复）";
    } else {
        qDebug() << "❌ QSS样式加载失败，路径:" << qssPath;
    }

    // 2. 加载字体文件
    QString fontPath = ":/resources/resources/fonts/fontawesome-webfont.ttf";

    int fontId = QFontDatabase::addApplicationFont(fontPath);
    if (fontId == -1) {
        fontPath = QCoreApplication::applicationDirPath() + "/resources/fonts/fontawesome-webfont.ttf";
        fontId = QFontDatabase::addApplicationFont(fontPath);
        if (fontId == -1) {
            qDebug() << "❌ 字体加载失败!";
            return false;
        }
    }

    // 3. 设置应用程序默认字体
    QFont defaultFont("Microsoft YaHei", 9);
    app.setFont(defaultFont);

    qDebug() << "✅ 应用程序默认字体已设置";

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
    QPixmap test(":/resources/resources/images/Login_background.jpg");
    if(!test.isNull()) {
        qDebug() << "✅ 图片加载成功";
    }
    else {
        qDebug() << "图片加载失败";
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
