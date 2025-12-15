#ifndef MAINWINDOW_H
#define MAINWINDOW_H
#pragma once
#include <QMainWindow>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include "protocol_parser.h"
#include "message_buffer.h"
#include "tcpclient.h"
#include "messagedispatcher.h"
#include "equipmentmanagerwidget.h"
#include "logindialog.h"
QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onConnectButtonClicked();
    void onLoginButtonClicked(); // 替换原有的连接按钮
    void showLoginDialog();
    void onLoginRequested(const QString& username, const QString& password);
    void handleLoginResponse(const ProtocolParser::ParseResult& result);

    void onSendHeartbeatButtonClicked();
    void onClientConnected();
    void onClientDisconnected();
    void onProtocolMessageReceived(const ProtocolParser::ParseResult& result);
    void onClientErrorOccurred(const QString& errorString);
    void handleHeartbeatResponse(const ProtocolParser::ParseResult &result);

private:
    Ui::MainWindow *ui;
    TcpClient* m_tcpClient; // 声明TCP客户端指针
    MessageDispatcher* m_dispatcher;
    LoginDialog* m_loginDialog;
    EquipmentManagerWidget* m_equipmentManagerWidget;

    bool m_isLoggedIn; // 登录状态标志
    QString m_currentUsername;

    void logMessage(const QString& msg); // 辅助日志函数
    void setupConnection(); // 连接信号槽
    void enableMainUI(bool enable); // 根据登录状态启用/禁用主UI
};

#endif // MAINWINDOW_H
