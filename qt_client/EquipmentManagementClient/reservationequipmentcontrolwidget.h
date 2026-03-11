#ifndef RESERVATIONEQUIPMENTCONTROLWIDGET_H
#define RESERVATIONEQUIPMENTCONTROLWIDGET_H

#include <QWidget>
#include <QGridLayout>
#include <QVBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QList>
#include "protocol_parser.h"
#include "tcpclient.h"
#include "messagedispatcher.h"

// 设备信息结构体
struct EquipmentInfo {
    QString id;
    QString type;
    QString name;
    QString location;
    QString powerState;
    bool online;
};

class ReservationEquipmentControlWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ReservationEquipmentControlWidget(const QString &reservationId, QWidget *parent = nullptr);

    // 设置 TCP 客户端（由 MainWindow 传入，用于发送消息）
    void setTcpClient(TcpClient *client);

    // 设置消息分发器（由 MainWindow 传入，用于注册响应处理）
    void setMessageDispatcher(MessageDispatcher *dispatcher);

    void setReservationId(const QString &id);

signals:
    void backRequested(); // 返回上一级信号

private slots:
    void onBackButtonClicked();
    void onControlButtonClicked(); // 处理控制按钮点击
    void handleControlResponse(const ProtocolParser::ParseResult &result); // 处理控制响应
    void handleEquipmentListResponse(const ProtocolParser::ParseResult &result); // 处理设备列表响应

private:

    struct EquipmentControls {
        QLabel *statusLabel;
        QPushButton *onBtn;
        QPushButton *offBtn;
    };
    QMap<QString, EquipmentControls> m_equipmentControls;

    void setupUI();
    void loadEquipmentList(); // 发送查询请求
    void updateEquipmentList(const QList<EquipmentInfo> &equipmentList); // 更新界面卡片
    void addEquipmentCard(const EquipmentInfo &info);
    void clearCards();

    QString m_reservationId;
    TcpClient *m_tcpClient;
    MessageDispatcher *m_dispatcher;

    QPushButton *m_backButton;
    QLabel *m_titleLabel;
    QWidget *m_cardContainer;
    QGridLayout *m_cardLayout;
    QList<QWidget*> m_cardWidgets; // 用于清理卡片
};

#endif // RESERVATIONEQUIPMENTCONTROLWIDGET_H
