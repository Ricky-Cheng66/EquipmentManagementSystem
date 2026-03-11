#include "reservationequipmentcontrolwidget.h"
#include <QMessageBox>
#include <QDebug>
#include <QScrollArea>

ReservationEquipmentControlWidget::ReservationEquipmentControlWidget(const QString &reservationId, QWidget *parent)
    : QWidget(parent)
    , m_reservationId(reservationId)
    , m_tcpClient(nullptr)
    , m_dispatcher(nullptr)
{
    setupUI();
}

void ReservationEquipmentControlWidget::setTcpClient(TcpClient *client)
{
    m_tcpClient = client;
}

void ReservationEquipmentControlWidget::setMessageDispatcher(MessageDispatcher *dispatcher)
{
    m_dispatcher = dispatcher;
    if (m_dispatcher) {
        // 注册设备列表响应处理器
        m_dispatcher->registerHandler(ProtocolParser::QT_MY_CONTROL_RESPONSE,
                                      [this](const ProtocolParser::ParseResult &result) {
                                          QMetaObject::invokeMethod(this, [this, result]() {
                                              handleEquipmentListResponse(result);
                                          });
                                      });
        // 注册控制响应处理器（复用 CONTROL_RESPONSE）
        m_dispatcher->registerHandler(ProtocolParser::CONTROL_RESPONSE,
                                      [this](const ProtocolParser::ParseResult &result) {
                                          QMetaObject::invokeMethod(this, [this, result]() {
                                              handleControlResponse(result);
                                          });
                                      });
    }
}

void ReservationEquipmentControlWidget::setReservationId(const QString &id)
{
    m_reservationId = id;
    m_titleLabel->setText(QString("预约设备控制 - 预约ID: %1").arg(id));
    loadEquipmentList(); // 发送查询请求
}

void ReservationEquipmentControlWidget::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(10, 10, 10, 10);
    mainLayout->setSpacing(10);

    // 顶部栏：返回按钮 + 标题
    QWidget *topBar = new QWidget(this);
    QHBoxLayout *topLayout = new QHBoxLayout(topBar);
    topLayout->setContentsMargins(0, 0, 0, 0);

    m_backButton = new QPushButton("← 返回", topBar);
    m_backButton->setFixedSize(80, 30);
    m_backButton->setStyleSheet(
        "QPushButton {"
        "    background-color: #95a5a6;"
        "    color: white;"
        "    border: none;"
        "    border-radius: 4px;"
        "}"
        "QPushButton:hover {"
        "    background-color: #7f8c8d;"
        "}"
        );
    connect(m_backButton, &QPushButton::clicked, this, &ReservationEquipmentControlWidget::backRequested);

    m_titleLabel = new QLabel(QString("预约设备控制 - 预约ID: %1").arg(m_reservationId), topBar);
    m_titleLabel->setStyleSheet("font-size: 16px; font-weight: bold; color: #2c3e50;");

    topLayout->addWidget(m_backButton);
    topLayout->addSpacing(10);
    topLayout->addWidget(m_titleLabel);
    topLayout->addStretch();

    mainLayout->addWidget(topBar);

    // 卡片容器（网格布局）
    m_cardContainer = new QWidget(this);
    m_cardLayout = new QGridLayout(m_cardContainer);
    m_cardLayout->setContentsMargins(20, 20, 20, 20);
    m_cardLayout->setHorizontalSpacing(20);
    m_cardLayout->setVerticalSpacing(20);
    m_cardLayout->setAlignment(Qt::AlignTop);

    QScrollArea *scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setWidget(m_cardContainer);

    mainLayout->addWidget(scrollArea);
}

void ReservationEquipmentControlWidget::loadEquipmentList()
{
    if (!m_tcpClient || !m_tcpClient->isConnected()) {
        QMessageBox::warning(this, "错误", "网络未连接，无法加载设备列表");
        return;
    }

    // 发送查询请求，使用预约ID
    std::vector<char> msg = ProtocolParser::build_my_control_query(
        ProtocolParser::CLIENT_QT_CLIENT,
        m_reservationId.toStdString()
        );
    m_tcpClient->sendData(QByteArray(msg.data(), msg.size()));
}

void ReservationEquipmentControlWidget::handleEquipmentListResponse(const ProtocolParser::ParseResult &result)
{
    QString payload = QString::fromStdString(result.payload);
    if (payload.startsWith("fail|")) {
        QMessageBox::warning(this, "加载失败", payload.mid(5));
        return;
    }

    // 解析 payload 格式: "success|设备1|设备2;..." 或直接 "设备1;..."
    QString data = payload;
    if (data.startsWith("success|"))
        data = data.mid(8);

    QList<EquipmentInfo> equipmentList;
    QStringList records = data.split(';', Qt::SkipEmptyParts);
    for (const QString &rec : records) {
        QStringList fields = rec.split('|');
        if (fields.size() >= 6) {
            EquipmentInfo info;
            info.id = fields[0];
            info.type = fields[1];
            info.name = fields[2];
            info.location = fields[3];
            info.powerState = fields[4];
            info.online = (fields[5] == "online");
            equipmentList.append(info);
        }
    }

    updateEquipmentList(equipmentList);
}

void ReservationEquipmentControlWidget::updateEquipmentList(const QList<EquipmentInfo> &equipmentList)
{
    clearCards();

    if (equipmentList.isEmpty()) {
        QLabel *emptyLabel = new QLabel("该预约无可用设备", m_cardContainer);
        emptyLabel->setAlignment(Qt::AlignCenter);
        emptyLabel->setStyleSheet("color: #7f8c8d; font-size: 16px; padding: 60px;");
        m_cardLayout->addWidget(emptyLabel, 0, 0, 1, 1);
        return;
    }

    // 计算每行卡片数
    int containerWidth = m_cardContainer->width();
    if (containerWidth <= 0) containerWidth = 800;
    int cardsPerRow = qMax(1, containerWidth / 300);

    int row = 0, col = 0;
    for (const EquipmentInfo &info : equipmentList) {
        addEquipmentCard(info);
        col++;
        if (col >= cardsPerRow) {
            col = 0;
            row++;
        }
    }
}

void ReservationEquipmentControlWidget::addEquipmentCard(const EquipmentInfo &info)
{
    // 创建设备卡片，参考 EquipmentManagerWidget 中的卡片样式
    QWidget *card = new QWidget(m_cardContainer);
    card->setObjectName("equipmentCard");
    card->setFixedSize(280, 160);
    card->setStyleSheet(
        "QWidget#equipmentCard {"
        "    background-color: white;"
        "    border: 1px solid #e0e0e0;"
        "    border-radius: 8px;"
        "    padding: 10px;"
        "}"
        "QWidget#equipmentCard:hover {"
        "    border-color: #4a69bd;"
        "    background-color: #f8f9fa;"
        "}"
        );

    QVBoxLayout *cardLayout = new QVBoxLayout(card);
    cardLayout->setContentsMargins(10, 10, 10, 10);
    cardLayout->setSpacing(6);

    // 设备名称
    QLabel *nameLabel = new QLabel(info.name, card);
    nameLabel->setStyleSheet("font-weight: bold; font-size: 14px; color: #2c3e50;");
    cardLayout->addWidget(nameLabel);

    // 设备ID和类型
    QLabel *idLabel = new QLabel(QString("ID: %1 | 类型: %2").arg(info.id, info.type), card);
    idLabel->setStyleSheet("font-size: 11px; color: #7f8c8d;");
    cardLayout->addWidget(idLabel);

    // 位置
    QLabel *locLabel = new QLabel("位置: " + info.location, card);
    locLabel->setStyleSheet("font-size: 11px; color: #7f8c8d;");
    cardLayout->addWidget(locLabel);

    // 状态
    QString statusText = info.online ? (info.powerState == "on" ? "已开机" : "已关机") : "离线";
    QString statusColor = info.online ? (info.powerState == "on" ? "#27ae60" : "#f39c12") : "#e74c3c";
    QLabel *statusLabel = new QLabel("状态: " + statusText, card);
    statusLabel->setStyleSheet(QString("font-size: 11px; color: %1;").arg(statusColor));
    cardLayout->addWidget(statusLabel);

    // 控制按钮（仅当在线时显示）
    if (info.online) {
        QHBoxLayout *btnLayout = new QHBoxLayout();
        btnLayout->setSpacing(5);

        QPushButton *onBtn = new QPushButton("开机", card);
        onBtn->setFixedSize(50, 24);
        onBtn->setStyleSheet(
            "QPushButton {"
            "    background-color: #27ae60;"
            "    color: white;"
            "    border: none;"
            "    border-radius: 3px;"
            "    font-size: 11px;"
            "}"
            "QPushButton:hover { background-color: #219653; }"
            "QPushButton:disabled { background-color: #b0bec5; }"
            );
        onBtn->setProperty("equipmentId", info.id);
        onBtn->setProperty("command", "turn_on");
        connect(onBtn, &QPushButton::clicked, this, &ReservationEquipmentControlWidget::onControlButtonClicked);

        QPushButton *offBtn = new QPushButton("关机", card);
        offBtn->setFixedSize(50, 24);
        offBtn->setStyleSheet(
            "QPushButton {"
            "    background-color: #e74c3c;"
            "    color: white;"
            "    border: none;"
            "    border-radius: 3px;"
            "    font-size: 11px;"
            "}"
            "QPushButton:hover { background-color: #c0392b; }"
            "QPushButton:disabled { background-color: #b0bec5; }"
            );
        offBtn->setProperty("equipmentId", info.id);
        offBtn->setProperty("command", "turn_off");
        connect(offBtn, &QPushButton::clicked, this, &ReservationEquipmentControlWidget::onControlButtonClicked);

        EquipmentControls controls;
        controls.statusLabel = statusLabel;
        controls.onBtn = onBtn;
        controls.offBtn = offBtn;
        m_equipmentControls[info.id] = controls;

        // 根据当前电源状态禁用对应按钮
        if (info.powerState == "on") {
            onBtn->setEnabled(false);
        } else {
            offBtn->setEnabled(false);
        }

        btnLayout->addStretch();
        btnLayout->addWidget(onBtn);
        btnLayout->addWidget(offBtn);
        btnLayout->addStretch();

        cardLayout->addLayout(btnLayout);
    } else {
        QLabel *offlineLabel = new QLabel("设备离线，无法控制", card);
        offlineLabel->setAlignment(Qt::AlignCenter);
        offlineLabel->setStyleSheet("color: #95a5a6; font-size: 11px;");
        cardLayout->addWidget(offlineLabel);
    }

    m_cardLayout->addWidget(card, m_cardLayout->rowCount(), 0); // 先简单添加，后续由 updateEquipmentList 按网格放置
    m_cardWidgets.append(card);
}

void ReservationEquipmentControlWidget::clearCards()
{
    qDeleteAll(m_cardWidgets);
    m_cardWidgets.clear();
    m_equipmentControls.clear();
}

void ReservationEquipmentControlWidget::onBackButtonClicked()
{
    emit backRequested();
}

void ReservationEquipmentControlWidget::onControlButtonClicked()
{
    QPushButton *btn = qobject_cast<QPushButton*>(sender());
    if (!btn) return;

    QString equipmentId = btn->property("equipmentId").toString();
    QString command = btn->property("command").toString();

    if (!m_tcpClient || !m_tcpClient->isConnected()) {
        QMessageBox::warning(this, "错误", "网络未连接");
        return;
    }

    // 发送控制请求
    std::vector<char> msg = ProtocolParser::build_my_control_request(
        ProtocolParser::CLIENT_QT_CLIENT,
        equipmentId.toStdString(),
        command.toStdString()
        );
    m_tcpClient->sendData(QByteArray(msg.data(), msg.size()));
}

void ReservationEquipmentControlWidget::handleControlResponse(const ProtocolParser::ParseResult &result)
{
    QString equipmentId = QString::fromStdString(result.equipment_id);  // 设备ID从消息中获取
    QString payload = QString::fromStdString(result.payload);
    QStringList parts = payload.split('|');
    if (parts.size() < 4) return;  // 格式错误

    bool success = (parts[1] == "success");
    QString command = parts[2];
    QString message = parts[3];

    if (success) {
        if (m_equipmentControls.contains(equipmentId)) {
            auto &ctrls = m_equipmentControls[equipmentId];
            if (command == "turn_on") {
                ctrls.statusLabel->setText("状态: 已开机");
                ctrls.statusLabel->setStyleSheet("color: #27ae60;");
                ctrls.onBtn->setEnabled(false);
                ctrls.offBtn->setEnabled(true);
            } else if (command == "turn_off") {
                ctrls.statusLabel->setText("状态: 已关机");
                ctrls.statusLabel->setStyleSheet("color: #f39c12;");
                ctrls.onBtn->setEnabled(true);
                ctrls.offBtn->setEnabled(false);
            }
        }
        QMessageBox::information(this, "控制成功", message);
    } else {
        QMessageBox::warning(this, "控制失败", message);
    }
}
