#ifndef MYRESERVATIONWIDGET_H
#define MYRESERVATIONWIDGET_H

#include <QWidget>
#include <QGridLayout>
#include <QVBoxLayout>
#include <QScrollArea>
#include <QLabel>
#include <QTimer>
#include "reservationcard.h"
#include "reservationfiltertoolbar.h"

class MyReservationWidget : public QWidget
{
    Q_OBJECT

public:
    explicit MyReservationWidget(QWidget *parent = nullptr);
    ~MyReservationWidget();

    // 接收预约数据
    void handleReservationResponse(const QString &data);

    // 设置用户信息（角色、ID），由 MainWindow 调用
    void setUserInfo(const QString &role, const QString &userId);

    void setTodayFilter();   // 筛选今日记录
    void setPendingFilter(); // 筛选待审批记录

    void clearFilters(); // 重置所有筛选条件（全部日期、全部状态）


signals:
    // 请求查询我的预约（由 MainWindow 转发给服务器）
    void queryRequested();

    // 请求打开设备控制界面，传递预约ID
    void equipmentControlRequested(const QString &reservationId);

private slots:
    void onFilterChanged();
    void onRefreshRequested();
    void onCardClicked(const QString &reservationId);

private:
    void setupUI();
    void refreshView();

    // 成员变量
    ReservationFilterToolBar *m_filterBar;
    QWidget *m_cardContainer;
    QGridLayout *m_cardLayout;
    QList<ReservationCard*> m_cards;
    QMap<QString, ReservationCard*> m_cardMap;
    QTimer *m_refreshTimer;

    QString m_userRole;
    QString m_userId;
    bool m_isRefreshing;
};

#endif // MYRESERVATIONWIDGET_H
