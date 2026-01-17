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
#include "logindialog.h"
#include "reservationwidget.h"
#include "equipmentmanagerwidget.h"
#include "energystatisticswidget.h"
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
    void handlePlaceListResponse(const ProtocolParser::ParseResult &result);
    // 预约相关槽函数
    void onReservationApplyRequested(const QString &placetId, const QString &purpose,
                                     const QString &startTime, const QString &endTime);
    void onReservationQueryRequested(const QString &placeId);
    void onReservationApproveRequested(int reservationId, bool approve);

    void showReservationWidget();

    void showEnergyStatisticsWidget();

    void onEnergyQueryRequested(const QString &equipmentId, const QString &timeRange);

    void handleEnergyResponse(const ProtocolParser::ParseResult &result);

    void handleQtHeartbeatResponse(const ProtocolParser::ParseResult &result);

    //智能告警
    void handleAlertMessage(const ProtocolParser::ParseResult &result);
private:
    Ui::MainWindow *ui;
    TcpClient* m_tcpClient; // 声明TCP客户端指针
    MessageDispatcher* m_dispatcher;
    LoginDialog* m_loginDialog;
    EquipmentManagerWidget* m_equipmentManagerWidget;
    int  m_currentUserId;    // 登录成功后从响应解析保存
    bool m_isLoggedIn; // 登录状态标志
    QString m_currentUsername;

    QString m_userRole;

    QAction* m_reservationAction;  // 预约管理菜单项指针

    ReservationWidget* m_reservationWidget;

    EnergyStatisticsWidget* m_energyStatisticsWidget;

    QAction* m_energyAction;

    // 新增：用于心跳的客户端标识
    QString m_clientHeartbeatId;  // 如 "qt_client_admin"

    void logMessage(const QString& msg); // 辅助日志函数
    void setupConnection(); // 连接信号槽
    void enableMainUI(bool enable); // 根据登录状态启用/禁用主UI
};

#endif // MAINWINDOW_H
