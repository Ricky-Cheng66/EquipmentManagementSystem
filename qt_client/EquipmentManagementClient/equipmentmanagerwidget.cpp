#include "equipmentmanagerwidget.h"
#include "ui_equipmentmanagerwidget.h"
#include "tcpclient.h"
#include "messagedispatcher.h"
#include "protocol_parser.h"
#include <QMessageBox>
#include <QDebug>

EquipmentManagerWidget::EquipmentManagerWidget(TcpClient* tcpClient, MessageDispatcher* dispatcher, QWidget *parent) :
    QWidget(parent), // 注意：父对象需要是QWidget*，这里先设为nullptr，在MainWindow中设置
    ui(new Ui::EquipmentManagerWidget),
    m_tcpClient(tcpClient),
    m_dispatcher(dispatcher),
    m_equipmentModel(new QStandardItemModel(this)),
    m_currentSelectedEquipmentId()
{
    ui->setupUi(this);
    setupTableView();

    // 连接按钮信号
    connect(ui->refreshButton, &QPushButton::clicked, this, &EquipmentManagerWidget::on_refreshButton_clicked);
    connect(ui->turnOnButton, &QPushButton::clicked, this, &EquipmentManagerWidget::on_turnOnButton_clicked);
    connect(ui->turnOffButton, &QPushButton::clicked, this, &EquipmentManagerWidget::on_turnOffButton_clicked);

    // 连接表格选择变化信号
    connect(ui->equipmentTableView->selectionModel(), &QItemSelectionModel::selectionChanged,
            [this](const QItemSelection &selected, const QItemSelection &deselected) {
                Q_UNUSED(deselected);
                bool hasSelection = !selected.isEmpty();
                this->updateControlButtonsState(hasSelection);
                if (hasSelection) {
                    QModelIndex index = selected.indexes().first(); // 获取选中行第一列的索引
                    m_currentSelectedEquipmentId = m_equipmentModel->data(index.siblingAtColumn(0)).toString(); // 第0列是设备ID
                } else {
                    m_currentSelectedEquipmentId.clear();
                }
            });

    // 向消息分发器注册本Widget的处理函数
    if (m_dispatcher) {
        // 注册设备状态更新处理
        m_dispatcher->registerHandler(ProtocolParser::STATUS_UPDATE,
                                      [this](const ProtocolParser::ParseResult &result) {
                                          QMetaObject::invokeMethod(this, [this, result]() {
                                              this->handleEquipmentStatusUpdate(result);
                                          });
                                      });
        // 注册控制响应处理 (类型已在协议中定义，例如 CONTROL_RESPONSE = 7)
        m_dispatcher->registerHandler(ProtocolParser::CONTROL_RESPONSE,
                                      [this](const ProtocolParser::ParseResult &result) {
                                          QMetaObject::invokeMethod(this, [this, result]() {
                                              this->handleControlResponse(result);
                                          });
                                      });
        // 注册设备列表响应处理 (需先在协议中定义，例如 QT_EQUIPMENT_LIST_RESPONSE = 102)
        // m_dispatcher->registerHandler(ProtocolParser::QT_EQUIPMENT_LIST_RESPONSE, ...);
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
    ui->equipmentTableView->horizontalHeader()->setStretchLastSection(true); // 最后一列填充
    ui->equipmentTableView->setEditTriggers(QAbstractItemView::NoEditTriggers); // 不可编辑
}

void EquipmentManagerWidget::requestEquipmentList() {
    // TODO: 发送设备列表查询请求到服务端
    // 协议类型需要先在 protocol_parser.h 中定义，例如 QT_EQUIPMENT_LIST_QUERY = 101
    // 示例: m_tcpClient->sendProtocolMessage(ProtocolParser::QT_EQUIPMENT_LIST_QUERY, "");
    qDebug() << "请求设备列表...";
    // --- 临时：添加模拟数据用于测试UI ---
    QList<QStandardItem*> row;
    row << new QStandardItem("projector_101") << new QStandardItem("投影仪") << new QStandardItem("101教室")
        << new QStandardItem("online") << new QStandardItem("off") << new QStandardItem("-");
    m_equipmentModel->appendRow(row);
    // --- 模拟结束 ---
}

// 以下为槽函数实现框架
void EquipmentManagerWidget::on_refreshButton_clicked() {
    requestEquipmentList();
}
void EquipmentManagerWidget::on_turnOnButton_clicked() {
    if (!m_currentSelectedEquipmentId.isEmpty()) {
        sendControlCommand(m_currentSelectedEquipmentId, ProtocolParser::TURN_ON);
    }
}
void EquipmentManagerWidget::on_turnOffButton_clicked() {
    if (!m_currentSelectedEquipmentId.isEmpty()) {
        sendControlCommand(m_currentSelectedEquipmentId, ProtocolParser::TURN_OFF);
    }
}
void EquipmentManagerWidget::sendControlCommand(const QString& equipmentId, ProtocolParser::ControlCommandType command) {
    if (m_tcpClient && m_tcpClient->isConnected()) {
        // 使用你已经实现的通用控制命令发送接口
        // 参数可能需要根据你的具体协议调整
        bool sent = m_tcpClient->sendProtocolMessage(ProtocolParser::QT_CONTROL_REQUEST,
                                                     equipmentId,
                                                     QString::number(static_cast<int>(command)));
        if (sent) {
            qDebug() << "已发送控制命令:" << equipmentId << ", 类型:" << static_cast<int>(command);
        } else {
            QMessageBox::warning(this, "发送失败", "控制命令发送失败，请检查网络连接。");
        }
    }
}
void EquipmentManagerWidget::updateControlButtonsState(bool hasSelection) {
    ui->turnOnButton->setEnabled(hasSelection);
    ui->turnOffButton->setEnabled(hasSelection);
}
void EquipmentManagerWidget::handleEquipmentStatusUpdate(const ProtocolParser::ParseResult& result) {
    // 解析 result.payload (格式例如: "online|on|45")
    // 更新表格中对应 equipmentId 的"状态"、"电源"和"最后更新"列
    qDebug() << "收到状态更新:" << QString::fromStdString(result.equipment_id) << QString::fromStdString(result.payload);
    // 具体更新逻辑需你根据 payload 格式实现
}
void EquipmentManagerWidget::handleControlResponse(const ProtocolParser::ParseResult& result) {
    // 解析控制命令执行结果，更新UI或给出提示
    qDebug() << "收到控制响应:" << QString::fromStdString(result.equipment_id) << QString::fromStdString(result.payload);
    bool success = (result.payload.find("success") != std::string::npos);
    QString message = QString("设备 [%1] 控制命令执行%2.")
                          .arg(QString::fromStdString(result.equipment_id))
                          .arg(success ? "成功" : "失败");
    // 可以更新状态栏或弹窗提示
    emit showStatusMessage(message);
}
// handleEquipmentListResponse 函数留待协议定义后实现
void EquipmentManagerWidget::handleEquipmentListResponse(const ProtocolParser::ParseResult &result)
{
    // 解析设备列表结果，更新表格
    qDebug() << "收到设备列表:" << QString::fromStdString(result.payload);
    // TODO：按你的协议格式解析并填充 m_equipmentModel
}
