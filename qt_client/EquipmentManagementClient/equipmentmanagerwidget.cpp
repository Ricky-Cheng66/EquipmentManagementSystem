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
#include <QGridLayout>
#include <QToolButton>
#include <QLabel>

EquipmentManagerWidget::EquipmentManagerWidget(TcpClient* tcpClient, MessageDispatcher* dispatcher, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::EquipmentManagerWidget),
    m_tcpClient(tcpClient),
    m_dispatcher(dispatcher),
    m_equipmentModel(new QStandardItemModel(this)),
    m_currentSelectedEquipmentId(),
    m_isRequesting(false),
    m_filterToolBar(nullptr),
    m_scrollArea(nullptr),
    m_cardContainer(nullptr),
    m_viewModeGroup(nullptr),
    m_viewStack(nullptr),
    m_gridLayout(nullptr),      // 新增
    m_isRefreshing(false)       // 新增
{
    ui->setupUi(this);

    // 设置窗口标题
    setWindowTitle("设备管理");

    // 创建延迟刷新定时器
    m_refreshTimer = new QTimer(this);
    m_refreshTimer->setSingleShot(true);
    connect(m_refreshTimer, &QTimer::timeout, this, &EquipmentManagerWidget::refreshCardView);

    // 初始化UI
    setupUI();

    // 确保默认显示卡片视图
    if (m_viewStack) {
        m_viewStack->setCurrentIndex(0);
    }

    // 延迟加载设备列表
    QTimer::singleShot(100, this, &EquipmentManagerWidget::requestEquipmentList);
}

EquipmentManagerWidget::~EquipmentManagerWidget()
{
    delete ui;
}

void EquipmentManagerWidget::setupUI()
{
    // 设置按钮样式
    ui->refreshButton->setText("🔄 刷新");
    ui->turnOnButton->setText("🔌 开机");
    ui->turnOnButton->setEnabled(false);
    ui->turnOffButton->setText("⏻ 关机");
    ui->turnOffButton->setEnabled(false);

    // 设置表格视图
    setupTableView();

    // 创建视图堆栈
    m_viewStack = new QStackedWidget(this);

    // 重要修改：将卡片视图页面放在第一位（索引0）

    // 创建卡片视图页面（放在第一位）
    QWidget *cardViewPage = new QWidget(m_viewStack);
    QVBoxLayout *cardViewLayout = new QVBoxLayout(cardViewPage);

    // 创建筛选工具栏
    m_filterToolBar = new FilterToolBar(cardViewPage);
    cardViewLayout->addWidget(m_filterToolBar);

    // 创建滚动区域
    m_scrollArea = new QScrollArea(cardViewPage);
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setFrameShape(QFrame::NoFrame);

    // 创建卡片容器
    m_cardContainer = new QWidget();
    m_cardContainer->setObjectName("cardContainer");
    m_containerLayout = new QVBoxLayout(m_cardContainer);
    m_containerLayout->setContentsMargins(20, 20, 20, 20);
    m_containerLayout->setSpacing(20);
    m_containerLayout->addStretch();

    m_scrollArea->setWidget(m_cardContainer);
    cardViewLayout->addWidget(m_scrollArea);

    // 卡片视图页面索引为0
    m_viewStack->addWidget(cardViewPage);

    // 创建表格视图页面（放在第二位）
    QWidget *tableViewPage = new QWidget(m_viewStack);
    QVBoxLayout *tableViewLayout = new QVBoxLayout(tableViewPage);
    tableViewLayout->addWidget(ui->equipmentTableView);

    // 表格视图页面索引为1
    m_viewStack->addWidget(tableViewPage);

    // 将视图堆栈添加到主布局
    QVBoxLayout *mainLayout = qobject_cast<QVBoxLayout*>(this->layout());
    if (!mainLayout) {
        mainLayout = new QVBoxLayout(this);
    }
    mainLayout->insertWidget(0, m_viewStack, 1); // 添加拉伸因子

    // 设置视图切换按钮
    setupViewModeToggle();

    // 连接信号槽
    connect(ui->refreshButton, &QPushButton::clicked, this, &EquipmentManagerWidget::on_refreshButton_clicked);
    connect(ui->turnOnButton, &QPushButton::clicked, this, &EquipmentManagerWidget::on_turnOnButton_clicked);
    connect(ui->turnOffButton, &QPushButton::clicked, this, &EquipmentManagerWidget::on_turnOffButton_clicked);
    connect(ui->equipmentTableView->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, &EquipmentManagerWidget::onSelectionChanged);
    connect(m_filterToolBar, &FilterToolBar::filterChanged,
            this, &EquipmentManagerWidget::onFilterChanged);

    // 默认隐藏表格（使用堆栈控制）
    ui->equipmentTableView->setParent(tableViewPage); // 已移到表格页面
}
void EquipmentManagerWidget::setupViewModeToggle()
{
    QHBoxLayout *toolbarLayout = qobject_cast<QHBoxLayout*>(ui->horizontalLayout);
    if (!toolbarLayout) return;

    // 移除旧的选择按钮（如果存在）
    for (int i = toolbarLayout->count() - 1; i >= 0; --i) {
        QLayoutItem *item = toolbarLayout->itemAt(i);
        if (item && item->widget() && item->widget()->objectName().contains("viewButton")) {
            item->widget()->deleteLater();
        }
    }

    // 添加分隔符
    toolbarLayout->insertWidget(2, new QLabel("  视图: ", this));

    // 创建卡片视图按钮
    QPushButton *cardViewBtn = new QPushButton("卡片视图", this);
    cardViewBtn->setObjectName("viewButton");
    cardViewBtn->setCheckable(true);
    cardViewBtn->setChecked(true); // 默认选中卡片视图
    cardViewBtn->setToolTip("卡片视图");
    cardViewBtn->setFixedSize(80, 28);

    // 创建列表视图按钮
    QPushButton *listViewBtn = new QPushButton("列表视图", this);
    listViewBtn->setObjectName("viewButton");
    listViewBtn->setCheckable(true);
    listViewBtn->setToolTip("表格列表视图");
    listViewBtn->setFixedSize(80, 28);

    // 创建按钮组
    m_viewModeGroup = new QButtonGroup(this);
    m_viewModeGroup->addButton(cardViewBtn, 0); // 卡片视图对应索引0
    m_viewModeGroup->addButton(listViewBtn, 1); // 列表视图对应索引1

    toolbarLayout->insertWidget(3, cardViewBtn);
    toolbarLayout->insertWidget(4, listViewBtn);

    // 连接信号 - 使用lambda确保正确切换
    connect(m_viewModeGroup, &QButtonGroup::buttonClicked, this, [this](QAbstractButton *button) {
        int id = m_viewModeGroup->id(button);
        m_viewStack->setCurrentIndex(id); // 0=卡片视图，1=表格视图

        // 确保按钮状态正确
        if (id == 0) {
            qDebug() << "切换到卡片视图";
        } else {
            qDebug() << "切换到表格视图";
        }
    });

    // 确保默认显示卡片视图
    if (m_viewStack) {
        m_viewStack->setCurrentIndex(0);
        qDebug() << "默认设置卡片视图为当前视图";
    }
}

void EquipmentManagerWidget::setupTableView() {
    m_equipmentModel->setHorizontalHeaderLabels({"设备ID", "类型", "位置", "状态", "电源", "最后更新"});
    ui->equipmentTableView->setModel(m_equipmentModel);
    ui->equipmentTableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->equipmentTableView->horizontalHeader()->setStretchLastSection(true);
    ui->equipmentTableView->setEditTriggers(QAbstractItemView::NoEditTriggers);

    ui->equipmentTableView->verticalHeader()->setDefaultSectionSize(36);

    StatusItemDelegate *statusDelegate = new StatusItemDelegate(this);
    ui->equipmentTableView->setItemDelegateForColumn(3, statusDelegate);

    CenterAlignDelegate *centerDelegate = new CenterAlignDelegate(this);
    for (int col = 0; col < m_equipmentModel->columnCount(); ++col) {
        if (col != 3) {
            ui->equipmentTableView->setItemDelegateForColumn(col, centerDelegate);
        }
    }

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
}

void EquipmentManagerWidget::requestEquipmentList() {
    if (m_isRequesting) return;

    if (m_tcpClient && m_tcpClient->isConnected()) {
        m_isRequesting = true;

        std::vector<char> message = ProtocolParser::build_qt_equipment_list_query(ProtocolParser::CLIENT_QT_CLIENT);
        m_tcpClient->sendData(QByteArray(message.data(), message.size()));
        qDebug() << "已发送设备列表查询请求";

        QTimer::singleShot(3000, this, [this]() {
            m_isRequesting = false;
        });
    } else {
        qWarning() << "网络未连接，无法查询设备列表";
        m_isRequesting = false;
    }
}

void EquipmentManagerWidget::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);

    // 延迟刷新卡片布局
    if (m_viewStack && m_viewStack->currentIndex() == 0) {
        if (m_refreshTimer->isActive()) {
            m_refreshTimer->stop();
        }
        m_refreshTimer->start(100);
    }
}

void EquipmentManagerWidget::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);

    // 窗口显示后延迟刷新卡片布局
    if (m_viewStack && m_viewStack->currentIndex() == 0) {
        if (m_refreshTimer->isActive()) {
            m_refreshTimer->stop();
        }
        m_refreshTimer->start(150);
    }
}

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

    std::string parameters = "";
    std::vector<char> controlMsg = ProtocolParser::build_control_command_to_server(
        ProtocolParser::CLIENT_QT_CLIENT,
        equipmentId.toStdString(),
        command,
        parameters
        );

    if (m_tcpClient->sendData(QByteArray(controlMsg.data(), controlMsg.size())) > 0) {
        QString commandStr = (command == ProtocolParser::TURN_ON) ? "开机" : "关机";
        logMessage(QString("控制命令已发送: [%1] -> %2").arg(equipmentId, commandStr));
        ui->turnOnButton->setEnabled(false);
        ui->turnOffButton->setEnabled(false);
    } else {
        QMessageBox::warning(this, "发送失败", "控制命令发送失败，请检查网络连接。");
    }
}

void EquipmentManagerWidget::updateControlButtonsState(bool hasSelection) {
    ui->turnOnButton->setEnabled(hasSelection);
    ui->turnOffButton->setEnabled(hasSelection);
}

void EquipmentManagerWidget::handleEquipmentStatusUpdate(const ProtocolParser::ParseResult& result)
{
    QString equipmentId = QString::fromStdString(result.equipment_id);
    QString payload = QString::fromStdString(result.payload);

    QStringList parts = payload.split('|');
    if (parts.size() >= 2) {
        QString status = parts[0];
        QString power = parts[1];

        // 清理状态
        if (status.contains("online")) status = "online";
        else if (status.contains("offline")) status = "offline";

        // 清理电源
        if (power.contains("on")) power = "开";
        else if (power.contains("off")) power = "关";

        // 更新模型
        updateEquipmentItem(equipmentId, 3, status, 4, power);

        // 更新卡片
        if (m_deviceCardMap.contains(equipmentId)) {
            m_deviceCardMap[equipmentId]->updateStatus(status, power);
            logMessage(QString("设备卡片状态更新: [%1] -> %2 %3").arg(equipmentId, status, power));
        }
    }

    // 使用 m_viewStack->currentIndex() 替代 m_isCardView
    if (m_viewStack && m_viewStack->currentIndex() == 0) {
        QTimer::singleShot(100, this, &EquipmentManagerWidget::refreshCardView);
    }
}

void EquipmentManagerWidget::handleControlResponse(const ProtocolParser::ParseResult& result)
{
    QString equipmentId = QString::fromStdString(result.equipment_id);
    QString payload = QString::fromStdString(result.payload);
    QStringList parts = payload.split('|');

    if (parts.size() >= 2) {
        bool success = (parts[1] == "success");
        QString command = parts.size() > 2 ? parts[2] : "unknown";
        QString message = QString("设备 [%1] %2命令执行%3")
                              .arg(equipmentId)
                              .arg(command)
                              .arg(success ? "成功" : "失败");

        if (success) {
            logMessage(message);

            // 控制成功后，延迟刷新设备列表以获取最新状态
            QTimer::singleShot(500, this, [this]() {
                requestEquipmentList();
                logMessage("控制成功后自动刷新设备列表");
            });
        } else {
            logMessage(message + "，原因: " + (parts.size() > 3 ? parts[3] : "未知"));
            QMessageBox::warning(this, "控制失败", message);
        }
    } else {
        logMessage(QString("收到格式异常的控制响应: %1").arg(payload));
    }

    updateControlButtonsState(!m_currentSelectedEquipmentId.isEmpty());
}

void EquipmentManagerWidget::handleEquipmentListResponse(const ProtocolParser::ParseResult &result)
{
    qDebug() << "开始处理设备列表响应";

    if (!m_equipmentModel) {
        qCritical() << "设备模型未初始化!";
        return;
    }

    // 清空现有模型数据
    m_equipmentModel->removeRows(0, m_equipmentModel->rowCount());

    // 清空卡片视图
    clearCardView();

    // 解析payload
    QString payload = QString::fromStdString(result.payload);
    qDebug() << "设备列表数据长度:" << payload.length();

    if (payload.isEmpty() || payload == "0") {
        qDebug() << "设备列表为空";
        // 添加空状态提示
        QLabel *emptyLabel = new QLabel("暂无设备数据", m_cardContainer);
        emptyLabel->setAlignment(Qt::AlignCenter);
        emptyLabel->setStyleSheet("color: #999; font-size: 16px;");
        m_containerLayout->insertWidget(0, emptyLabel);
        return;
    }

    QStringList deviceList = payload.split(";", Qt::SkipEmptyParts);
    qDebug() << "设备列表数量:" << deviceList.size();

    QSet<QString> uniquePlaces;
    QSet<QString> uniqueTypes;

    for (int i = 0; i < deviceList.size(); ++i) {
        QString deviceStr = deviceList[i];
        QStringList fields = deviceStr.split("|");
        if (fields.size() >= 5) {
            QString deviceId = fields[0];
            QString type = fields[1];
            QString location = fields[2];

            QString status = fields[3].trimmed();
            if (status.contains("online")) {
                status = "online";
            } else if (status.contains("offline")) {
                status = "offline";
            }

            QString power = fields[4].trimmed();
            if (power.contains("on")) {
                power = "开";
            } else if (power.contains("off")) {
                power = "关";
            } else if (power.isEmpty()) {
                power = "关";
            }

            // 添加到模型
            QList<QStandardItem*> row;
            row << new QStandardItem(deviceId)
                << new QStandardItem(type)
                << new QStandardItem(location)
                << new QStandardItem(status)
                << new QStandardItem(power)
                << new QStandardItem(QDateTime::currentDateTime().toString("hh:mm:ss"));

            m_equipmentModel->appendRow(row);

            // 收集场所和类型信息
            uniquePlaces.insert(location);
            uniqueTypes.insert(type);

            // 创建设备卡片
            DeviceCard *card = new DeviceCard(deviceId, type, location, status, power, m_cardContainer);
            connect(card, &DeviceCard::cardClicked, this, &EquipmentManagerWidget::onCardClicked);
            connect(card, &DeviceCard::powerControlRequested, this, &EquipmentManagerWidget::onPowerControlRequested);

            m_deviceCards.append(card);
            m_deviceCardMap[deviceId] = card;

            if (deviceId == m_currentSelectedEquipmentId) {
                card->setSelected(true);
            }
        }
    }

    // 更新筛选器
    if (m_filterToolBar) {
        m_filterToolBar->setPlaces(uniquePlaces.values());
        m_filterToolBar->setTypes(uniqueTypes.values());
    }

    // 刷新卡片视图
    refreshCardView();

    qDebug() << "设备列表更新完成，共" << m_deviceCards.size() << "个设备";
    qDebug() << "当前视图索引:" << m_viewStack->currentIndex();
}

void EquipmentManagerWidget::clearCardView()
{
    // 安全地清理卡片
    for (DeviceCard *card : m_deviceCards) {
        if (card) {
            // 断开信号连接
            card->disconnect();
            // 从布局中移除
            if (m_gridLayout) {
                m_gridLayout->removeWidget(card);
            }
            // 延迟删除
            card->deleteLater();
        }
    }
    m_deviceCards.clear();
    m_deviceCardMap.clear();
}


void EquipmentManagerWidget::refreshCardView()
{
    if (m_isRefreshing) {
        // 如果正在刷新，延迟执行
        m_refreshTimer->start(100);
        return;
    }

    m_isRefreshing = true;

    qDebug() << "开始刷新卡片视图";

    if (!m_containerLayout || !m_cardContainer) {
        qWarning() << "卡片容器或布局未初始化";
        m_isRefreshing = false;
        return;
    }

    // 计算每行卡片数量
    int containerWidth = m_cardContainer->width();
    if (containerWidth <= 0) {
        containerWidth = this->width() - 40;
    }
    int cardsPerRow = qMax(1, containerWidth / 300);

    // 获取筛选条件
    QString selectedPlace = m_filterToolBar->selectedPlace();
    QString selectedType = m_filterToolBar->selectedType();
    QString selectedStatus = m_filterToolBar->selectedStatus();
    QString searchText = m_filterToolBar->searchText();
    bool onlineOnly = m_filterToolBar->showOnlineOnly();

    // 如果已有网格布局，先移除（但不立即删除）
    if (m_gridLayout) {
        // 从容器布局中移除网格布局
        for (int i = 0; i < m_containerLayout->count(); ++i) {
            QLayoutItem *item = m_containerLayout->itemAt(i);
            if (item && item->layout() == m_gridLayout) {
                m_containerLayout->removeItem(item);
                break;
            }
        }

        // 安全地删除网格布局（延迟删除）
        QTimer::singleShot(0, m_gridLayout, [this]() {
            if (m_gridLayout) {
                // 从网格布局中移除所有控件（不删除控件）
                QLayoutItem *child;
                while ((child = m_gridLayout->takeAt(0)) != nullptr) {
                    delete child; // 只删除布局项，不删除控件
                }
                m_gridLayout->deleteLater();
                m_gridLayout = nullptr;
            }
        });
    }

    // 创建新的网格布局
    m_gridLayout = new QGridLayout();
    m_gridLayout->setContentsMargins(0, 0, 0, 0);
    m_gridLayout->setHorizontalSpacing(20);
    m_gridLayout->setVerticalSpacing(20);

    int row = 0;
    int col = 0;
    int visibleCards = 0;

    // 创建可见卡片列表的副本，避免在迭代时修改
    QList<DeviceCard*> visibleCardsList;

    for (DeviceCard *card : m_deviceCards) {
        // 应用筛选条件
        bool shouldShow = true;

        if (selectedPlace != "all" && card->location() != selectedPlace) {
            shouldShow = false;
        }

        if (selectedType != "all" && card->deviceType() != selectedType) {
            shouldShow = false;
        }

        if (selectedStatus != "all" && card->status().toLower() != selectedStatus) {
            shouldShow = false;
        }

        if (onlineOnly && !card->status().toLower().contains("online") && card->status() != "在线") {
            shouldShow = false;
        }

        if (!searchText.isEmpty() &&
            !card->deviceId().contains(searchText, Qt::CaseInsensitive) &&
            !card->location().contains(searchText, Qt::CaseInsensitive)) {
            shouldShow = false;
        }

        card->setVisible(shouldShow);

        if (shouldShow) {
            visibleCardsList.append(card);
        }
    }

    // 使用可见卡片列表进行布局
    for (DeviceCard *card : visibleCardsList) {
        m_gridLayout->addWidget(card, row, col);
        visibleCards++;

        col++;
        if (col >= cardsPerRow) {
            col = 0;
            row++;
        }
    }

    // 将网格布局添加到容器布局
    m_containerLayout->insertLayout(0, m_gridLayout);

    // 如果没有可见卡片，显示提示信息
    if (visibleCards == 0) {
        QLabel *emptyLabel = new QLabel("暂无符合条件的设备", m_cardContainer);
        emptyLabel->setAlignment(Qt::AlignCenter);
        emptyLabel->setStyleSheet(
            "QLabel {"
            "    color: #999;"
            "    font-size: 14px;"
            "    padding: 40px;"
            "}"
            );
        m_gridLayout->addWidget(emptyLabel, 0, 0, 1, cardsPerRow, Qt::AlignCenter);
    }

    // 更新容器布局
    m_cardContainer->updateGeometry();

    qDebug() << "刷新完成，可见卡片数量:" << visibleCards;
    m_isRefreshing = false;
}

void EquipmentManagerWidget::updateEquipmentItem(const QString& equipmentId, int statusCol,
                                                 const QString& status, int powerCol,
                                                 const QString& powerState) {
    for (int row = 0; row < m_equipmentModel->rowCount(); ++row) {
        QStandardItem* idItem = m_equipmentModel->item(row, 0);
        if (idItem && idItem->text() == equipmentId) {
            if (statusCol >= 0) {
                m_equipmentModel->item(row, statusCol)->setText(status);
            }
            if (powerCol >= 0) {
                m_equipmentModel->item(row, powerCol)->setText(powerState);
            }
            m_equipmentModel->item(row, 5)->setText(QDateTime::currentDateTime().toString("hh:mm:ss"));
            break;
        }
    }
}

void EquipmentManagerWidget::onCardClicked(const QString &deviceId)
{
    if (m_currentSelectedEquipmentId != deviceId) {
        if (m_deviceCardMap.contains(m_currentSelectedEquipmentId)) {
            m_deviceCardMap[m_currentSelectedEquipmentId]->setSelected(false);
        }

        m_currentSelectedEquipmentId = deviceId;
        if (m_deviceCardMap.contains(deviceId)) {
            m_deviceCardMap[deviceId]->setSelected(true);
        }

        updateControlButtonsState(true);
    }
}

void EquipmentManagerWidget::onPowerControlRequested(const QString &deviceId, bool turnOn)
{
    ProtocolParser::ControlCommandType command = turnOn ?
                                                     ProtocolParser::TURN_ON : ProtocolParser::TURN_OFF;
    sendControlCommand(deviceId, command);
}

void EquipmentManagerWidget::onFilterChanged()
{
    // 使用定时器延迟刷新，避免快速连续调用
    if (m_refreshTimer->isActive()) {
        m_refreshTimer->stop();
    }
    m_refreshTimer->start(200); // 200ms延迟
}

void EquipmentManagerWidget::onViewModeChanged()
{
    // 已在setupViewModeToggle的lambda中处理
}

void EquipmentManagerWidget::onSelectionChanged(const QItemSelection &selected, const QItemSelection &deselected)
{
    Q_UNUSED(deselected);

    bool hasSelection = !selected.isEmpty();
    updateControlButtonsState(hasSelection);

    if (hasSelection) {
        QModelIndex index = selected.indexes().first();
        if (index.isValid()) {
            m_currentSelectedEquipmentId = m_equipmentModel->item(index.row(), 0)->text();
        }
    } else {
        m_currentSelectedEquipmentId.clear();
    }
}

void EquipmentManagerWidget::logMessage(const QString &msg)
{
    qDebug() << "[EquipmentManagerWidget]" << msg;
}
