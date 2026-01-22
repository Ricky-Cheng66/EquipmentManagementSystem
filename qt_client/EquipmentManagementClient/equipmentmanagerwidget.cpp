#include "equipmentmanagerwidget.h"
#include "ui_equipmentmanagerwidget.h"
#include "tcpclient.h"
#include "messagedispatcher.h"
#include "protocol_parser.h"
#include "statusitemdelegate.h"
#include "centeraligndelegate.h"
#include "logindialog.h"
#include <QMessageBox>
#include <QDebug>
#include <QDateTime>
#include <QMainWindow>
#include <QStatusBar>

EquipmentManagerWidget::EquipmentManagerWidget(TcpClient* tcpClient, MessageDispatcher* dispatcher, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::EquipmentManagerWidget),
    m_tcpClient(tcpClient),
    m_dispatcher(dispatcher),
    m_equipmentModel(new QStandardItemModel(this)),
    m_currentSelectedEquipmentId(),
    m_isRequesting(false)
{
    ui->setupUi(this);
    setupTableView();

    // 按钮样式设置
    ui->refreshButton->setProperty("class", "icon-font");
    ui->refreshButton->setText(QChar(0xf021) + QString(" 刷新"));

    ui->turnOnButton->setProperty("class", "icon-font");
    ui->turnOnButton->setText(QChar(0xf011) + QString(" 开机"));

    ui->turnOffButton->setProperty("class", "icon-font");
    ui->turnOffButton->setText(QChar(0xf011) + QString(" 关机"));

    // 连接按钮信号
    connect(ui->refreshButton, &QPushButton::clicked, this, &EquipmentManagerWidget::on_refreshButton_clicked);
    connect(ui->turnOnButton, &QPushButton::clicked, this, &EquipmentManagerWidget::on_turnOnButton_clicked);
    connect(ui->turnOffButton, &QPushButton::clicked, this, &EquipmentManagerWidget::on_turnOffButton_clicked);

    // 连接表格选择变化信号 - 修复：只连接一个处理函数
    connect(ui->equipmentTableView->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, &EquipmentManagerWidget::onSelectionChanged);

    // 向消息分发器注册本Widget的处理函数
    if (m_dispatcher) {
        // 注册设备状态更新处理
        m_dispatcher->registerHandler(ProtocolParser::STATUS_UPDATE,
                                      [this](const ProtocolParser::ParseResult &result) {
                                          QMetaObject::invokeMethod(this, [this, result]() {
                                              this->handleEquipmentStatusUpdate(result);
                                          });
                                      });
        // 注册控制响应处理
        m_dispatcher->registerHandler(ProtocolParser::CONTROL_RESPONSE,
                                      [this](const ProtocolParser::ParseResult &result) {
                                          QMetaObject::invokeMethod(this, [this, result]() {
                                              this->handleControlResponse(result);
                                          });
                                      });
        // 注册设备列表响应处理
        m_dispatcher->registerHandler(ProtocolParser::QT_EQUIPMENT_LIST_RESPONSE,
                                      [this](const ProtocolParser::ParseResult &result) {
                                          QMetaObject::invokeMethod(this, [this, result]() {
                                              this->handleEquipmentListResponse(result);
                                          });
                                      });
    }
}

EquipmentManagerWidget::~EquipmentManagerWidget()
{
    delete ui;
}

void EquipmentManagerWidget::setupTableView() {
    // 设置表格模型和表头
    m_equipmentModel->setHorizontalHeaderLabels({"设备ID", "类型", "位置", "状态", "电源", "最后更新"});
    ui->equipmentTableView->setModel(m_equipmentModel);
    ui->equipmentTableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->equipmentTableView->horizontalHeader()->setStretchLastSection(true);
    ui->equipmentTableView->setEditTriggers(QAbstractItemView::NoEditTriggers);

    // 美化表格行高和字体
    ui->equipmentTableView->verticalHeader()->setDefaultSectionSize(36);
    ui->equipmentTableView->setAlternatingRowColors(false); // 统一颜色

    // 设置状态列的委托
    StatusItemDelegate *statusDelegate = new StatusItemDelegate(this);
    ui->equipmentTableView->setItemDelegateForColumn(3, statusDelegate); // 只对状态列使用

    // 对其他列使用居中对齐委托
    CenterAlignDelegate *centerDelegate = new CenterAlignDelegate(this);
    for (int col = 0; col < m_equipmentModel->columnCount(); ++col) {
        if (col != 3) { // 状态列已经设置了委托
            ui->equipmentTableView->setItemDelegateForColumn(col, centerDelegate);
        }
    }

    // 设置表格样式
    ui->equipmentTableView->setStyleSheet(
        "QTableView {"
        "    background-color: white;"
        "    gridline-color: #f0f0f0;"
        "}"
        "QTableView::item {"
        "    padding: 6px;"
        "    border-bottom: 1px solid #f0f0f0;"
        "}"
        "QTableView::item:selected {"
        "    background-color: #e3f2fd;"
        "    color: #1976d2;"
        "}"
        "QHeaderView::section {"
        "    background-color: #f5f6fa;"
        "    padding: 8px;"
        "    border: none;"
        "    border-right: 1px solid #e0e0e0;"
        "    border-bottom: 2px solid #e0e0e0;"
        "    font-weight: bold;"
        "    text-align: center;"
        "}");

    qDebug() << "表格设置完成";
}

void EquipmentManagerWidget::requestEquipmentList() {
    // 防止重复请求
    if (m_isRequesting) {
        qDebug() << "设备列表请求正在进行中，跳过重复请求";
        return;
    }

    if (m_tcpClient && m_tcpClient->isConnected()) {
        m_isRequesting = true;

        std::vector<char> message = ProtocolParser::build_qt_equipment_list_query(ProtocolParser::CLIENT_QT_CLIENT);
        m_tcpClient->sendData(QByteArray(message.data(), message.size()));
        qDebug() << "已发送设备列表查询请求";

        // 设置超时重置标志
        QTimer::singleShot(3000, this, [this]() {
            m_isRequesting = false;
        });
    } else {
        qWarning() << "网络未连接，无法查询设备列表";
        m_isRequesting = false;
    }
}

// 以下为槽函数实现框架
void EquipmentManagerWidget::on_refreshButton_clicked() {
    requestEquipmentList();
}
void EquipmentManagerWidget::on_turnOnButton_clicked() {
    qDebug() << "开机按钮被点击，当前选中设备:" << m_currentSelectedEquipmentId;
    if (!m_currentSelectedEquipmentId.isEmpty()) {
        sendControlCommand(m_currentSelectedEquipmentId, ProtocolParser::TURN_ON);
    } else {
        qWarning() << "尝试开机但未选中设备!";
    }
}
void EquipmentManagerWidget::on_turnOffButton_clicked() {
    qDebug() << "关机按钮被点击，当前选中设备:" << m_currentSelectedEquipmentId;
    if (!m_currentSelectedEquipmentId.isEmpty()) {
        sendControlCommand(m_currentSelectedEquipmentId, ProtocolParser::TURN_OFF);
    } else {
        qWarning() << "尝试关机但未选中设备!";
    }
}
void EquipmentManagerWidget::sendControlCommand(const QString& equipmentId, ProtocolParser::ControlCommandType command) {
    if (!m_tcpClient || !m_tcpClient->isConnected()) {
        QMessageBox::warning(this, "发送失败", "网络未连接，无法发送控制命令。");
        return;
    }
    if (equipmentId.isEmpty()) {
        QMessageBox::warning(this, "发送失败", "未选择任何设备。");
        return;
    }

    // 构造控制请求消息。
    // 协议格式参考：CLIENT_QT_CLIENT|QT_CONTROL_REQUEST|equipment_id|command_type|parameters
    // 例如：“2|10|projector_101|1|” 表示开启 projector_101 设备。
    std::string parameters = ""; // 附加参数，根据协议需要可扩展
    std::vector<char> controlMsg = ProtocolParser::build_control_command_to_server(
        ProtocolParser::CLIENT_QT_CLIENT,
        equipmentId.toStdString(),
        command,
        parameters
        );

    if (m_tcpClient->sendData(QByteArray(controlMsg.data(), controlMsg.size())) > 0) {
        QString commandStr = (command == ProtocolParser::TURN_ON) ? "开机" : "关机";
        logMessage(QString("控制命令已发送: [%1] -> %2").arg(equipmentId, commandStr));
        // 可选：禁用按钮，等待响应后再启用，防止重复点击
        ui->turnOnButton->setEnabled(false);
        ui->turnOffButton->setEnabled(false);
    } else {
        QMessageBox::warning(this, "发送失败", "控制命令发送失败，请检查网络连接。");
    }
}
void EquipmentManagerWidget::updateControlButtonsState(bool hasSelection) {
    ui->turnOnButton->setEnabled(hasSelection);
    ui->turnOffButton->setEnabled(hasSelection);
    // 调试输出
    qDebug() << "按钮状态更新 - 开机按钮:" << ui->turnOnButton->isEnabled()
             << " 关机按钮:" << ui->turnOffButton->isEnabled();
}
void EquipmentManagerWidget::handleEquipmentStatusUpdate(const ProtocolParser::ParseResult& result) {
    QString equipmentId = QString::fromStdString(result.equipment_id);
    QString payload = QString::fromStdString(result.payload);
    logMessage(QString("设备状态更新: [%1] -> %2").arg(equipmentId, payload));

    // 获取主窗口，在其状态栏显示临时消息
    QMainWindow* mainWindow = qobject_cast<QMainWindow*>(this->window());
    if (mainWindow && mainWindow->statusBar()) {
        mainWindow->statusBar()->showMessage(
            QString("设备 [%1] 状态已更新 (点击右侧'刷新列表'查看)").arg(equipmentId),
            5000 // 显示5秒
            );
    }
}
void EquipmentManagerWidget::handleControlResponse(const ProtocolParser::ParseResult& result) {
    QString equipmentId = QString::fromStdString(result.equipment_id);
    // 解析payload，例如 “qt_fd|success|turn_on|result_message” 或 “qt_fd|fail|turn_off|result_message”
    QString payload = QString::fromStdString(result.payload);
    QStringList parts = payload.split('|');

    if (parts.size() >= 2) {
        bool success = (parts[1] == "success");
        QString command = parts[2];
        QString message = QString("设备 [%1] %2命令执行%3")
                              .arg(equipmentId)
                              .arg(command)
                              .arg(success ? "成功" : "失败");

        if (success) {
            logMessage(message);
            // 如果控制成功，可以更新本地模型中的设备电源状态（假设我们相信这个响应）
            updateEquipmentItem(equipmentId, 4, (command == "turn_on") ? "on" : "off", -1, "");
        } else {
            logMessage(message + "，原因: " + (parts.size() > 2 ? parts[2] : "未知"));
            QMessageBox::warning(this, "控制失败", message);
        }
    } else {
        logMessage(QString("收到格式异常的控制响应: %1").arg(payload));
    }
    // 无论成功与否，重新启用控制按钮
    updateControlButtonsState(!m_currentSelectedEquipmentId.isEmpty());
}
// handleEquipmentListResponse 函数留待协议定义后实现
void EquipmentManagerWidget::handleEquipmentListResponse(const ProtocolParser::ParseResult &result)
{
    qDebug() << "收到设备列表响应";

    // 清空现有模型数据
    m_equipmentModel->removeRows(0, m_equipmentModel->rowCount());

    // 解析payload
    QString payload = QString::fromStdString(result.payload);
    QStringList deviceList = payload.split(";", Qt::SkipEmptyParts);

    for (const QString& deviceStr : deviceList) {
        QStringList fields = deviceStr.split("|");
        if (fields.size() >= 5) {
            // 清理状态字段
            QString status = fields[3].trimmed();
            if (status.contains("online")) {
                status = "online";
            } else if (status.contains("offline")) {
                status = "offline";
            }

            // 清理电源字段
            QString power = fields[4].trimmed();
            if (power.contains("on")) {
                power = "开";
            } else if (power.contains("off")) {
                power = "关";
            } else if (power.isEmpty()) {
                power = "关";
            }

            QList<QStandardItem*> row;
            row << new QStandardItem(fields[0])  // ID
                << new QStandardItem(fields[1])  // 类型
                << new QStandardItem(fields[2])  // 位置
                << new QStandardItem(status)      // 状态
                << new QStandardItem(power)       // 电源（已清理）
                << new QStandardItem(QDateTime::currentDateTime().toString("hh:mm:ss"));

            m_equipmentModel->appendRow(row);
        }
    }

    qDebug() << "设备列表更新完成，共" << m_equipmentModel->rowCount() << "个设备";
}

void EquipmentManagerWidget::updateEquipmentItem(const QString& equipmentId, int statusCol, const QString& status, int powerCol, const QString& powerState) {
    for (int row = 0; row < m_equipmentModel->rowCount(); ++row) {
        QStandardItem* idItem = m_equipmentModel->item(row, 0);
        if (idItem && idItem->text() == equipmentId) {
            if (statusCol >= 0) {
                m_equipmentModel->item(row, statusCol)->setText(status);
            }
            if (powerCol >= 0) {
                m_equipmentModel->item(row, powerCol)->setText(powerState);
            }
            // 更新“最后更新”时间
            m_equipmentModel->item(row, 5)->setText(QDateTime::currentDateTime().toString("hh:mm:ss"));
            break;
        }
    }
}

void EquipmentManagerWidget::logMessage(const QString &msg)
{
    qDebug() << "[EquipmentManagerWidget]" << msg;
}

void EquipmentManagerWidget::onSelectionChanged(const QItemSelection &selected, const QItemSelection &deselected) {
    Q_UNUSED(deselected);

    bool hasSelection = !selected.isEmpty();

    // 调试输出，确认函数被调用
    qDebug() << "表格选择变化，是否有选中行:" << hasSelection;

    updateControlButtonsState(hasSelection);

    if (hasSelection) {
        // 获取选中行的第一列（设备ID）
        QModelIndex index = selected.indexes().first();
        if (index.isValid()) {
            m_currentSelectedEquipmentId = m_equipmentModel->item(index.row(), 0)->text();
            qDebug() << "选中设备ID:" << m_currentSelectedEquipmentId;

            // 可选：获取选中行的状态信息，用于判断按钮可用性
            QString status = m_equipmentModel->item(index.row(), 3)->text(); // 第3列是状态
            QString power = m_equipmentModel->item(index.row(), 4)->text();  // 第4列是电源

            // 根据设备当前状态调整按钮可用性（可选）
            // 例如：如果已经是开机状态，则"开机"按钮可禁用
            ui->turnOnButton->setEnabled(power != "on");
            ui->turnOffButton->setEnabled(power != "off");
        }
    } else {
        m_currentSelectedEquipmentId.clear();
    }
}
