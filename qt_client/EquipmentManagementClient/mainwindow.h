#ifndef MAINWINDOW_H
#define MAINWINDOW_H
#pragma once
#include <QMainWindow>
#include <QDockWidget>
#include <QTreeWidget>
#include <QStackedWidget>
#include <QToolBar>
#include <QStatusBar>
#include <QMenuBar>
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
    MainWindow(TcpClient* tcpClient, MessageDispatcher* dispatcher,
               const QString& username, const QString& role, int userId,  // 增加userId参数
               QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    // 原有槽函数保留...
    void onProtocolMessageReceived(const ProtocolParser::ParseResult& result);
    void onClientErrorOccurred(const QString& errorString);
    void handleHeartbeatResponse(const ProtocolParser::ParseResult &result);

    // 导航栏切换槽函数
    void onNavigationItemClicked(QTreeWidgetItem *item, int column);

    // 预约响应处理
    void handleReservationApplyResponse(const ProtocolParser::ParseResult &result);
    void handleReservationQueryResponse(const ProtocolParser::ParseResult &result);
    void handleReservationApproveResponse(const ProtocolParser::ParseResult &result);
    void handlePlaceListResponse(const ProtocolParser::ParseResult &result);

    // 预约相关槽函数
    void onReservationApplyRequested(const QString &placetId, const QString &purpose,
                                     const QString &startTime, const QString &endTime);
    void onReservationQueryRequested(const QString &placeId);
    void onReservationApproveRequested(int reservationId, const QString &placeId, bool approve);

    void showReservationWidget();
    void showEnergyStatisticsWidget();
    void onEnergyQueryRequested(const QString &equipmentId, const QString &timeRange);
    void handleEnergyResponse(const ProtocolParser::ParseResult &result);
    void handleQtHeartbeatResponse(const ProtocolParser::ParseResult &result);

    //智能告警
    void handleAlertMessage(const ProtocolParser::ParseResult &result);

    // 新增：注销槽函数
    void onLogout();

private:
    Ui::MainWindow *ui;
    TcpClient* m_tcpClient;
    MessageDispatcher* m_dispatcher;

    // 新增：主界面组件
    QMenuBar *m_menuBar;
    QToolBar *m_toolBar;
    QDockWidget *m_navigationDock;
    QTreeWidget *m_navigationTree;
    QStackedWidget *m_centralStack;
    QStatusBar *m_statusBar;

    // 页面组件
    QWidget *m_dashboardPage;           // 仪表板页面
    EquipmentManagerWidget *m_equipmentPage;  // 设备管理页面
    ReservationWidget *m_reservationPage;     // 预约管理页面
    EnergyStatisticsWidget *m_energyPage;     // 能耗统计页面
    QWidget *m_settingsPage;            // 系统设置页面

    // 原有成员变量
    LoginDialog* m_loginDialog;
    int  m_currentUserId;
    bool m_isLoggedIn;
    QString m_currentUsername;
    QString m_userRole;
    QAction* m_reservationAction;
    QAction* m_energyAction;
    QString m_clientHeartbeatId;

    // 仪表板相关控件指针
    QLabel *m_totalDevicesLabel;
    QLabel *m_onlineDevicesLabel;
    QLabel *m_offlineDevicesLabel;
    QLabel *m_reservedDevicesLabel;
    QLabel *m_todayEnergyLabel;
    QLabel *m_activeAlertsLabel;
    QLabel *m_todayReservationsLabel;
    QLabel *m_placeUsageLabel;

    QTextEdit *m_alertTextEdit;
    QTextEdit *m_activityTextEdit;

    // 仪表板相关函数
    void updateDashboardStats();  // 更新仪表板统计数据
    void updateRecentAlerts();    // 更新最近告警
    void updateActivityLog();     // 更新活动日志
    void setupDashboardConnections(); // 设置仪表板信号连接
    bool eventFilter(QObject *watched, QEvent *event);

    void logMessage(const QString& msg);
    void setupUI();                     // 主UI设置函数
    void setupNavigation();             // 设置导航栏
    void setupMenuBar();                // 设置菜单栏
    void setupToolBar();                // 设置工具栏
    void setupStatusBar();              // 设置状态栏
    void setupMessageHandlers();        // 设置消息处理器
    void setupPermissionByRole();       // 根据角色设置权限
    void switchPage(int pageIndex);     // 切换页面
    void setupCentralStack();           // 设置中央堆栈页面
};

#endif // MAINWINDOW_H
