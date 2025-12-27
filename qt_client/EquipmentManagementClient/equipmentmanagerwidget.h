#ifndef EQUIPMENTMANAGERWIDGET_H
#define EQUIPMENTMANAGERWIDGET_H

#include <QWidget>
#include <QStandardItemModel>
#include <QItemSelection>
#include "protocol_parser.h"

// 前向声明
class ProtocolParser;
class TcpClient;
class MessageDispatcher;

namespace Ui {
class EquipmentManagerWidget;
}

class EquipmentManagerWidget : public QWidget
{
    Q_OBJECT

public:
    explicit EquipmentManagerWidget(TcpClient* tcpClient, MessageDispatcher* dispatcher, QWidget *parent = nullptr);
    ~EquipmentManagerWidget();

    QStandardItemModel* m_equipmentModel;

    // 提供给外部调用的刷新接口
    void requestEquipmentList();

signals:
    void showStatusMessage(const QString &msg);

private slots:
    void on_refreshButton_clicked();
    void on_turnOnButton_clicked();
    void on_turnOffButton_clicked();
    // 处理从网络层传来的设备状态更新
    void handleEquipmentStatusUpdate(const ProtocolParser::ParseResult& result);
    // 处理从网络层传来的设备列表响应
    void handleEquipmentListResponse(const ProtocolParser::ParseResult& result);
    // 处理从网络层传来的控制响应
    void handleControlResponse(const ProtocolParser::ParseResult& result);

private:
    Ui::EquipmentManagerWidget *ui;
    TcpClient* m_tcpClient;
    MessageDispatcher* m_dispatcher;

    QString m_currentSelectedEquipmentId; // 当前在表格中选中的设备ID

    void logMessage(const QString &msg);
    void setupTableView();
    void sendControlCommand(const QString& equipmentId, ProtocolParser::ControlCommandType command);
    void updateControlButtonsState(bool hasSelection);
    void updateEquipmentItem(const QString& equipmentId, int statusCol, const QString& status, int powerCol, const QString& powerState);
    void onSelectionChanged(const QItemSelection &selected, const QItemSelection &deselected);
};

#endif // EQUIPMENTMANAGERWIDGET_H
