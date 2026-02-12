#ifndef EQUIPMENTMANAGERWIDGET_H
#define EQUIPMENTMANAGERWIDGET_H

#include <QWidget>
#include <QStandardItemModel>
#include <QItemSelection>
#include <QScrollArea>
#include <QButtonGroup>
#include <QStackedWidget>
#include "protocol_parser.h"
#include "devicecard.h"
#include "filtertoolbar.h"

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
protected:
    void resizeEvent(QResizeEvent *event) override;
    void showEvent(QShowEvent *event) override;

signals:
    void showStatusMessage(const QString &msg);
    void deviceListLoaded();

private slots:
    void on_refreshButton_clicked();
    void on_turnOnButton_clicked();
    void on_turnOffButton_clicked();

    // 新增槽函数
    void onCardClicked(const QString &deviceId);
    void onPowerControlRequested(const QString &deviceId, bool turnOn);
    void onFilterChanged();
    void onViewModeChanged();

public slots:
    void handleEquipmentStatusUpdate(const ProtocolParser::ParseResult& result);
    void handleEquipmentListResponse(const ProtocolParser::ParseResult& result);
    void handleControlResponse(const ProtocolParser::ParseResult& result);

private:
    Ui::EquipmentManagerWidget *ui;
    TcpClient* m_tcpClient;
    MessageDispatcher* m_dispatcher;

    QString m_currentSelectedEquipmentId;
    bool m_isRequesting;

    // 新增成员变量
    FilterToolBar *m_filterToolBar;
    QScrollArea *m_scrollArea;
    QWidget *m_cardContainer;
    QVBoxLayout *m_containerLayout;
    QButtonGroup *m_viewModeGroup;
    QStackedWidget *m_viewStack;

    QList<DeviceCard*> m_deviceCards;
    QMap<QString, DeviceCard*> m_deviceCardMap;

    QGridLayout *m_gridLayout;  // 网格布局
    QTimer *m_refreshTimer;     // 延迟刷新定时器
    bool m_isRefreshing;        // 是否正在刷新

    // 新增私有方法
    void logMessage(const QString &msg);
    void setupUI();
    void setupTableView();
    void setupCardView();
    void clearCardView();
    void applyFilters();
    void refreshCardView();
    void setupViewModeToggle();

    void sendControlCommand(const QString& equipmentId, ProtocolParser::ControlCommandType command);
    void updateControlButtonsState(bool hasSelection);
    void updateEquipmentItem(const QString& equipmentId, int statusCol, const QString& status, int powerCol, const QString& powerState);
    void onSelectionChanged(const QItemSelection &selected, const QItemSelection &deselected);
};

#endif // EQUIPMENTMANAGERWIDGET_H
