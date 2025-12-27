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
#include "reservationwidget.h"
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

    // 预约响应处理
    void handleReservationApplyResponse(const ProtocolParser::ParseResult &result);
    void handleReservationQueryResponse(const ProtocolParser::ParseResult &result);
    void handleReservationApproveResponse(const ProtocolParser::ParseResult &result);
    // 预约相关槽函数
    void onReservationApplyRequested(const QString &equipmentId, const QString &purpose,
                                     const QString &startTime, const QString &endTime);
    void onReservationQueryRequested(const QString &equipmentId);
    void onReservationApproveRequested(int reservationId, bool approve);

    void showReservationWidget();
private:
    Ui::MainWindow *ui;
    TcpClient* m_tcpClient; // 声明TCP客户端指针
    MessageDispatcher* m_dispatcher;
    LoginDialog* m_loginDialog;
    EquipmentManagerWidget* m_equipmentManagerWidget;
    QString m_currentUserId;    // 登录成功后从响应解析保存
    bool m_isLoggedIn; // 登录状态标志
    QString m_currentUsername;

    QAction* m_reservationAction;  // 预约管理菜单项指针

    ReservationWidget* m_reservationWidget;

    void logMessage(const QString& msg); // 辅助日志函数
    void setupConnection(); // 连接信号槽
    void enableMainUI(bool enable); // 根据登录状态启用/禁用主UI
};

#endif // MAINWINDOW_H
