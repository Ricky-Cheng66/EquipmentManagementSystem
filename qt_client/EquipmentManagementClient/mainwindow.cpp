#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <QTime>
#include <QFile>
#include <QMessageBox>
#include <QLabel>
#include <QRegularExpression>
#define EXIT_CODE_RELOGOUT 1001
// 导航项定义
enum NavigationPages {
    PAGE_DASHBOARD = 0,
    PAGE_EQUIPMENT,
    PAGE_RESERVATION,
    PAGE_ENERGY,
    PAGE_SETTINGS
};

MainWindow::MainWindow(TcpClient* tcpClient, MessageDispatcher* dispatcher,
                       const QString& username, const QString& role, int userId,
                       QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_tcpClient(tcpClient)
    , m_dispatcher(dispatcher)
    , m_currentUsername(username)
    , m_userRole(role)
    , m_currentUserId(userId)
    , m_isLoggedIn(true)
    , m_loginDialog(nullptr)
    , m_reservationAction(nullptr)
    , m_energyAction(nullptr)
    , m_clientHeartbeatId("")
    , m_menuBar(nullptr)
    , m_toolBar(nullptr)
    , m_navigationDock(nullptr)
    , m_navigationTree(nullptr)
    , m_centralStack(nullptr)
    , m_statusBar(nullptr)
    , m_dashboardPage(nullptr)
    , m_equipmentPage(nullptr)
    , m_reservationPage(nullptr)
    , m_energyPage(nullptr)
    , m_settingsPage(nullptr)
{
    ui->setupUi(this);

    // 设置窗口属性
    setMinimumSize(1024, 768);
    setWindowTitle(QString("校园设备综合管理系统 - 用户: %1 (ID: %2)").arg(username).arg(userId));

    // 初始化UI
    setupUI();

    // 设置预约页面的用户角色
    if (m_reservationPage) {
        m_reservationPage->setUserRole(m_userRole, QString::number(m_currentUserId));
    }

    // 注意：移除了自动请求设备列表的代码
    // 设备列表将在用户首次切换到设备管理页面时请求

    logMessage(QString("系统启动完成，欢迎 %1 (ID: %2, 角色: %3)").arg(username).arg(userId).arg(role));
}

MainWindow::~MainWindow()
{
    delete ui;
}


void MainWindow::setupUI()
{
    // 1. 创建菜单栏
    setupMenuBar();

    // 2. 创建工具栏
    setupToolBar();

    // 3. 创建状态栏
    setupStatusBar();

    // 4. 创建中央堆栈窗口
    setupCentralStack();

    // 5. 创建左侧导航栏
    setupNavigation();

    // 6. 设置主窗口布局
    setCentralWidget(m_centralStack);
    addDockWidget(Qt::LeftDockWidgetArea, m_navigationDock);

    // 7. 根据角色设置权限
    setupPermissionByRole();

    // 8. 设置消息处理器
    setupMessageHandlers();
}

void MainWindow::setupMenuBar()
{
    // 这里需要补充具体的实现代码
    // 由于之前已经提供了代码，这里只是占位符
    m_menuBar = menuBar();

    // 文件菜单
    QMenu *fileMenu = m_menuBar->addMenu("文件(&F)");
    QAction *logoutAction = fileMenu->addAction(QIcon(":/icons/logout.png"), "注销(&L)");
    QAction *exitAction = fileMenu->addAction(QIcon(":/icons/exit.png"), "退出(&X)");
    connect(logoutAction, &QAction::triggered, this, &MainWindow::onLogout);
    connect(exitAction, &QAction::triggered, qApp, &QApplication::quit);

    // 管理菜单
    QMenu *managementMenu = m_menuBar->addMenu("管理(&M)");
    m_reservationAction = managementMenu->addAction(QIcon(":/icons/reservation.png"), "预约管理(&R)");
    m_energyAction = managementMenu->addAction(QIcon(":/icons/energy.png"), "能耗统计(&E)");
    connect(m_reservationAction, &QAction::triggered, this, [this]() { switchPage(PAGE_RESERVATION); });
    connect(m_energyAction, &QAction::triggered, this, [this]() { switchPage(PAGE_ENERGY); });

    // 视图菜单
    QMenu *viewMenu = m_menuBar->addMenu("视图(&V)");
    QAction *showNavAction = viewMenu->addAction("显示导航栏");
    showNavAction->setCheckable(true);
    showNavAction->setChecked(true);
    connect(showNavAction, &QAction::toggled, [this](bool checked) {
        m_navigationDock->setVisible(checked);
    });

    // 帮助菜单
    QMenu *helpMenu = m_menuBar->addMenu("帮助(&H)");
    QAction *aboutAction = helpMenu->addAction("关于(&A)");
    connect(aboutAction, &QAction::triggered, []() {
        QMessageBox::about(nullptr, "关于",
                           "校园设备综合管理系统\n"
                           "版本: 1.0.0\n"
                           "© 2024 校园设备管理中心");
    });
}

void MainWindow::setupToolBar()
{
    m_toolBar = addToolBar("常用工具");
    m_toolBar->setMovable(false);

    // 添加工具按钮
    QAction *refreshAction = m_toolBar->addAction(QIcon(":/icons/refresh.png"), "刷新");
    QAction *connectAction = m_toolBar->addAction(QIcon(":/icons/connect.png"), "连接状态");
    m_toolBar->addSeparator();
    QAction *dashboardAction = m_toolBar->addAction(QIcon(":/icons/dashboard.png"), "仪表板");
    QAction *equipmentAction = m_toolBar->addAction(QIcon(":/icons/equipment.png"), "设备管理");

    connect(refreshAction, &QAction::triggered, [this]() {
        // 根据当前页面决定刷新什么
        int currentPage = m_centralStack->currentIndex();
        switch (currentPage) {
        case PAGE_EQUIPMENT:
            if (m_equipmentPage) {
                qDebug() << "手动刷新设备列表";
                m_equipmentPage->requestEquipmentList();
            }
            break;
        case PAGE_RESERVATION:
            // 可以添加预约页面的刷新逻辑
            break;
        case PAGE_ENERGY:
            // 可以添加能耗页面的刷新逻辑
            break;
        }
    });

    connect(connectAction, &QAction::triggered, [this]() {
        QString status = m_tcpClient->isConnected() ? "已连接" : "未连接";
        QMessageBox::information(this, "连接状态",
                                 QString("服务器连接状态: %1\n用户名: %2\n角色: %3")
                                     .arg(status).arg(m_currentUsername).arg(m_userRole));
    });

    connect(dashboardAction, &QAction::triggered, [this]() { switchPage(PAGE_DASHBOARD); });
    connect(equipmentAction, &QAction::triggered, [this]() { switchPage(PAGE_EQUIPMENT); });
}

void MainWindow::setupStatusBar()
{
    m_statusBar = statusBar();

    // 连接状态标签
    QLabel *connectionLabel = new QLabel();
    connectionLabel->setText(m_tcpClient->isConnected() ? "● 已连接" : "○ 未连接");
    connectionLabel->setStyleSheet(m_tcpClient->isConnected() ?
                                       "color: green; font-weight: bold;" : "color: red;");
    m_statusBar->addPermanentWidget(connectionLabel);

    // 用户信息标签
    QLabel *userLabel = new QLabel();
    userLabel->setText(QString("用户: %1").arg(m_currentUsername));
    m_statusBar->addPermanentWidget(userLabel);

    // 在线设备数量标签（动态更新）
    QLabel *deviceLabel = new QLabel("设备: 0");
    deviceLabel->setObjectName("deviceCountLabel");
    m_statusBar->addPermanentWidget(deviceLabel);

    // 连接状态变化更新
    connect(m_tcpClient, &TcpClient::connected, [connectionLabel]() {
        connectionLabel->setText("● 已连接");
        connectionLabel->setStyleSheet("color: green; font-weight: bold;");
    });

    connect(m_tcpClient, &TcpClient::disconnected, [connectionLabel]() {
        connectionLabel->setText("○ 未连接");
        connectionLabel->setStyleSheet("color: red;");
    });
}

void MainWindow::setupCentralStack()
{
    m_centralStack = new QStackedWidget(this);

    // 1. 仪表板页面
    m_dashboardPage = new QWidget();
    QVBoxLayout *dashboardLayout = new QVBoxLayout(m_dashboardPage);

    QLabel *welcomeLabel = new QLabel(
        QString("<h1>欢迎使用校园设备综合管理系统</h1>"
                "<p>当前用户: <b>%1</b> (%2)</p>"
                "<p>登录时间: %3</p>")
            .arg(m_currentUsername)
            .arg(m_userRole)
            .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss")));
    welcomeLabel->setAlignment(Qt::AlignCenter);

    // 添加一些快速操作按钮
    QHBoxLayout *quickActions = new QHBoxLayout();
    QPushButton *quickRefresh = new QPushButton("刷新设备列表");
    QPushButton *quickReserve = new QPushButton("快速预约");
    QPushButton *quickEnergy = new QPushButton("查看能耗");

    connect(quickRefresh, &QPushButton::clicked, [this]() {
        if (m_equipmentPage) {
            m_equipmentPage->requestEquipmentList();
            logMessage("已从仪表板刷新设备列表");
        }
    });

    connect(quickReserve, &QPushButton::clicked, [this]() {
        switchPage(PAGE_RESERVATION);
    });

    connect(quickEnergy, &QPushButton::clicked, [this]() {
        switchPage(PAGE_ENERGY);
    });

    quickActions->addWidget(quickRefresh);
    quickActions->addWidget(quickReserve);
    quickActions->addWidget(quickEnergy);
    quickActions->addStretch();

    dashboardLayout->addWidget(welcomeLabel);
    dashboardLayout->addLayout(quickActions);
    dashboardLayout->addStretch();

    m_centralStack->addWidget(m_dashboardPage);

    // 2. 设备管理页面
    m_equipmentPage = new EquipmentManagerWidget(m_tcpClient, m_dispatcher, this);
    m_centralStack->addWidget(m_equipmentPage);

    // 3. 预约管理页面
    m_reservationPage = new ReservationWidget(this);
    connect(m_reservationPage, &ReservationWidget::reservationApplyRequested,
            this, &MainWindow::onReservationApplyRequested);
    connect(m_reservationPage, &ReservationWidget::reservationQueryRequested,
            this, &MainWindow::onReservationQueryRequested);
    connect(m_reservationPage, &ReservationWidget::reservationApproveRequested,
            this, &MainWindow::onReservationApproveRequested);
    m_centralStack->addWidget(m_reservationPage);

    // 4. 能耗统计页面
    m_energyPage = new EnergyStatisticsWidget(this);
    connect(m_energyPage, &EnergyStatisticsWidget::energyQueryRequested,
            this, &MainWindow::onEnergyQueryRequested);
    m_centralStack->addWidget(m_energyPage);

    // 5. 系统设置页面
    m_settingsPage = new QWidget();
    QVBoxLayout *settingsLayout = new QVBoxLayout(m_settingsPage);
    QLabel *settingsLabel = new QLabel("<h2>系统设置</h2>");
    settingsLabel->setAlignment(Qt::AlignCenter);
    settingsLayout->addWidget(settingsLabel);
    settingsLayout->addStretch();
    m_centralStack->addWidget(m_settingsPage);
}

void MainWindow::setupNavigation()
{
    m_navigationDock = new QDockWidget("导航", this);
    m_navigationDock->setFeatures(QDockWidget::NoDockWidgetFeatures);
    m_navigationDock->setMinimumWidth(200);

    m_navigationTree = new QTreeWidget();
    m_navigationTree->setHeaderHidden(true);
    m_navigationTree->setIconSize(QSize(24, 24));

    // 添加导航项
    QTreeWidgetItem *dashboardItem = new QTreeWidgetItem(m_navigationTree);
    dashboardItem->setText(0, "仪表板");
    dashboardItem->setIcon(0, QIcon(":/icons/dashboard.png"));
    dashboardItem->setData(0, Qt::UserRole, PAGE_DASHBOARD);

    QTreeWidgetItem *equipmentItem = new QTreeWidgetItem(m_navigationTree);
    equipmentItem->setText(0, "设备管理");
    equipmentItem->setIcon(0, QIcon(":/icons/equipment.png"));
    equipmentItem->setData(0, Qt::UserRole, PAGE_EQUIPMENT);

    QTreeWidgetItem *reservationItem = new QTreeWidgetItem(m_navigationTree);
    reservationItem->setText(0, "预约管理");
    reservationItem->setIcon(0, QIcon(":/icons/reservation.png"));
    reservationItem->setData(0, Qt::UserRole, PAGE_RESERVATION);

    QTreeWidgetItem *energyItem = new QTreeWidgetItem(m_navigationTree);
    energyItem->setText(0, "能耗统计");
    energyItem->setIcon(0, QIcon(":/icons/energy.png"));
    energyItem->setData(0, Qt::UserRole, PAGE_ENERGY);

    QTreeWidgetItem *settingsItem = new QTreeWidgetItem(m_navigationTree);
    settingsItem->setText(0, "系统设置");
    settingsItem->setIcon(0, QIcon(":/icons/settings.png"));
    settingsItem->setData(0, Qt::UserRole, PAGE_SETTINGS);

    m_navigationTree->addTopLevelItem(dashboardItem);
    m_navigationTree->addTopLevelItem(equipmentItem);
    m_navigationTree->addTopLevelItem(reservationItem);
    m_navigationTree->addTopLevelItem(energyItem);
    m_navigationTree->addTopLevelItem(settingsItem);

    // 连接点击事件
    connect(m_navigationTree, &QTreeWidget::itemClicked,
            this, &MainWindow::onNavigationItemClicked);

    m_navigationDock->setWidget(m_navigationTree);

    // 默认选中仪表板
    m_navigationTree->setCurrentItem(dashboardItem);
}

void MainWindow::onNavigationItemClicked(QTreeWidgetItem *item, int column)
{
    Q_UNUSED(column);

    int pageIndex = item->data(0, Qt::UserRole).toInt();
    switchPage(pageIndex);
}

void MainWindow::switchPage(int pageIndex)
{
    static QSet<int> loadedPages;  // 记录已经加载过的页面

    if (pageIndex >= 0 && pageIndex < m_centralStack->count()) {
        m_centralStack->setCurrentIndex(pageIndex);

        // 更新导航树选中状态
        for (int i = 0; i < m_navigationTree->topLevelItemCount(); ++i) {
            QTreeWidgetItem *item = m_navigationTree->topLevelItem(i);
            if (item->data(0, Qt::UserRole).toInt() == pageIndex) {
                m_navigationTree->setCurrentItem(item);
                break;
            }
        }

        // 页面切换时的特殊处理
        switch (pageIndex) {
        case PAGE_EQUIPMENT:
            if (m_equipmentPage) {
                // 只有在首次切换到设备页面或手动刷新时才请求
                if (!loadedPages.contains(PAGE_EQUIPMENT)) {
                    m_equipmentPage->requestEquipmentList();
                    loadedPages.insert(PAGE_EQUIPMENT);
                    qDebug() << "首次切换到设备页面，发送设备列表查询请求";
                } else {
                    qDebug() << "设备页面已加载过，跳过自动刷新";
                }
            }
            break;

        case PAGE_RESERVATION:
            // 只有在首次切换到预约页面时才请求场所信息
            if (!loadedPages.contains(PAGE_RESERVATION) && m_tcpClient && m_tcpClient->isConnected()) {
                // 请求场所列表
                std::vector<char> msg = ProtocolParser::pack_message(
                    ProtocolParser::build_message_body(
                        ProtocolParser::CLIENT_QT_CLIENT,
                        ProtocolParser::QT_PLACE_LIST_QUERY,
                        "",  // equipment_id为空
                        {""} // payload为空
                        )
                    );
                m_tcpClient->sendData(QByteArray(msg.data(), msg.size()));
                loadedPages.insert(PAGE_RESERVATION);
                logMessage("首次加载场所信息...");
            }
            break;

        case PAGE_ENERGY:
            // 能耗页面切换逻辑
            if (!loadedPages.contains(PAGE_ENERGY) && m_energyPage && m_equipmentPage) {
                // 加载设备列表到能耗页面
                QStringList equipmentIds;
                QStandardItemModel* model = m_equipmentPage->m_equipmentModel;
                if (model) {
                    for (int row = 0; row < model->rowCount(); ++row) {
                        equipmentIds << model->item(row, 0)->text();
                    }
                }
                m_energyPage->setEquipmentList(equipmentIds);
                loadedPages.insert(PAGE_ENERGY);
            }
            break;
        }
    }
}


void MainWindow::onLogout()
{
    int result = QMessageBox::question(this, "注销确认",
                                       "确定要注销当前用户吗？",
                                       QMessageBox::Yes | QMessageBox::No);

    if (result == QMessageBox::Yes) {
        // 停止心跳
        m_tcpClient->stopHeartbeat();

        // 断开连接
        m_tcpClient->disconnectFromServer();

        // 关闭主窗口
        close();

        // 注意：这里应该返回到登录界面
        // 但由于我们修改了启动流程，需要重新启动程序
        // 更优雅的做法是重启应用程序
        QTimer::singleShot(100, []() {
            qApp->exit(EXIT_CODE_RELOGOUT);
        });
    }
}

// 其他原有的槽函数实现需要保持不变，但需要从构造函数移动到setupMessageHandlers中

// 注意：原有的所有消息处理器注册代码需要从构造函数移动到setupMessageHandlers函数中
// 这里只展示了关键修改，其他部分需要您从原代码中复制过来


//------------------------------------------------


void MainWindow::onProtocolMessageReceived(const ProtocolParser::ParseResult& result)
{
    QString msg = QString("[收到协议消息] 类型: %1, 设备: %2, 载荷: %3")
                      .arg(result.type)
                      .arg(QString::fromStdString(result.equipment_id))
                      .arg(QString::fromStdString(result.payload));
    logMessage(msg);
}

void MainWindow::onClientErrorOccurred(const QString& errorString)
{
    logMessage(QString("[网络错误] %1").arg(errorString));
}

void MainWindow::logMessage(const QString& msg)
{
    // 在状态栏显示临时消息
    m_statusBar->showMessage(msg, 5000);

    // 也可以同时输出到控制台
    qDebug() << "[MainWindow]" << msg;
}





// 新增：心跳响应的具体处理函数
void MainWindow::handleHeartbeatResponse(const ProtocolParser::ParseResult &result)
{
    QString msg = QString("[业务处理] 心跳响应来自设备: %1").arg(QString::fromStdString(result.equipment_id));
    logMessage(msg);
    // 这里可以更新UI状态，比如让某个设备图标闪烁表示在线
}



// 发送预约申请
void MainWindow::onReservationApplyRequested(const QString &placeId, const QString &purpose,
                                             const QString &startTime, const QString &endTime)
{
    if (!m_tcpClient || !m_tcpClient->isConnected()) {
        QMessageBox::warning(this, "预约失败", "网络未连接");
        return;
    }

    // 检查用户ID是否有效
    if (m_currentUserId <= 0) {
        QMessageBox::warning(this, "预约失败", "用户信息无效，请重新登录");
        return;
    }

    // payload格式: "userId|start_time|end_time|purpose"
    QString payload = QString("%1|%2|%3|%4")
                          .arg(m_currentUserId)
                          .arg(startTime)
                          .arg(endTime)
                          .arg(purpose);

    qDebug() << "DEBUG: m_currentUserId=" << m_currentUserId << ", payload=" << payload;

    std::vector<char> msg = ProtocolParser::build_reservation_message(
        ProtocolParser::CLIENT_QT_CLIENT,
        placeId.toStdString(),  // 场所ID
        payload.toStdString()
        );

    m_tcpClient->sendData(QByteArray(msg.data(), msg.size()));
    logMessage(QString("预约申请已发送: 场所[%1] 用户[%2]").arg(placeId).arg(m_currentUserId));
}

// 发送预约查询
void MainWindow::onReservationQueryRequested(const QString &placeId)
{
    qDebug() << "MainWindow::onReservationQueryRequested 收到信号，placeId =" << placeId;

    if (!m_tcpClient || !m_tcpClient->isConnected()) {
        QMessageBox::warning(this, "查询失败", "网络未连接");
        return;
    }

    std::vector<char> msg = ProtocolParser::build_reservation_query(
        ProtocolParser::CLIENT_QT_CLIENT,
        placeId.toStdString());

    qint64 bytesSent = m_tcpClient->sendData(QByteArray(msg.data(), msg.size()));
    if (bytesSent > 0) {
        logMessage(QString("预约查询已发送: 场所[%1]").arg(placeId));
    } else {
        logMessage("预约查询发送失败");
    }
}

// 发送预约审批
void MainWindow::onReservationApproveRequested(int reservationId, const QString &placeId, bool approve)
{
    // ✅ 简化权限：默认所有登录用户都能审批
    qDebug() << "接收到审批请求：预约ID" << reservationId << " 场所:" << placeId << " 操作：" << (approve ? "批准" : "拒绝");

    if (!m_tcpClient || !m_tcpClient->isConnected()) {
        QMessageBox::warning(this, "审批失败", "网络未连接");
        return;
    }


    // ✅ 使用传递过来的 placeId 直接发送审批消息
    QString payload = QString("%1|%2").arg(reservationId).arg(approve ? "approve" : "reject");
    std::vector<char> msg = ProtocolParser::build_reservation_approve(
        ProtocolParser::CLIENT_QT_CLIENT,
        placeId.toStdString(),  // ✅ 直接使用参数中的 placeId
        payload.toStdString());

    qint64 bytesSent = m_tcpClient->sendData(QByteArray(msg.data(), msg.size()));
    if (bytesSent > 0) {
        logMessage(QString("预约审批已发送: [%1] %2").arg(reservationId).arg(approve ? "批准" : "拒绝"));
    } else {
        QMessageBox::warning(this, "发送失败", "审批命令发送失败");
    }
}

void MainWindow::handleReservationApplyResponse(const ProtocolParser::ParseResult &result)
{
    QString payload = QString::fromStdString(result.payload);
    QStringList parts = payload.split('|');

    if (parts.size() >= 2 && parts[0] == "success") {
        QMessageBox::information(this, "预约成功", parts[1]);
        logMessage("预约申请提交成功");
    } else {
        QString errorMsg = parts.size() >= 2 ? parts[1] : "未知错误";
        QMessageBox::warning(this, "预约失败", errorMsg);
        logMessage(QString("预约申请失败: %1").arg(errorMsg));
    }
}

void MainWindow::handleReservationQueryResponse(const ProtocolParser::ParseResult &result)
{
    QString payload = QString::fromStdString(result.payload);
    QStringList parts = payload.split('|', Qt::SkipEmptyParts);

    qDebug() << "Query response payload:" << payload;

    if (parts.isEmpty() || parts[0] != "success") {
        QString err = parts.size() >= 2 ? parts[1] : "查询失败";
        if (m_reservationPage && m_reservationPage->isVisible())  // 改为 m_reservationPage
            QMessageBox::warning(m_reservationPage, "查询失败", err);
        return;
    }

    QString data = payload.mid(parts[0].length() + 1);

    // 改为 m_reservationPage
    if (!m_reservationPage || !m_reservationPage->isVisible()) {
        qDebug() << "预约窗口未打开，丢弃查询结果";
        return;
    }

    int currentTab = m_reservationPage->m_tabWidget->currentIndex();  // 改为 m_reservationPage
    qDebug() << "Distributing to tab:" << currentTab;

    if (currentTab == 2) {            // 审批页
        m_reservationPage->loadAllReservationsForApproval(data);  // 改为 m_reservationPage
    } else if (currentTab == 1) {     // 查询页
        m_reservationPage->updateQueryResultTable(data);  // 改为 m_reservationPage
    }
}

void MainWindow::handleReservationApproveResponse(const ProtocolParser::ParseResult &result)
{
    QString payload = QString::fromStdString(result.payload);
    QStringList parts = payload.split('|');

    if (parts.size() >= 2 && parts[0] == "success") {
        QMessageBox::information(this, "审批成功", parts[1]);
        logMessage("预约审批操作成功");

        // ✅ 自动刷新审批页面（延迟500ms确保数据库已更新）
        QTimer::singleShot(500, [this]() {
            if (m_reservationPage && m_reservationPage->m_tabWidget->currentIndex() == 2) {
                emit m_reservationPage->reservationQueryRequested("all");
                logMessage("自动刷新审批列表...");
            }
        });
    } else {
        QString errorMsg = parts.size() >= 2 ? parts[1] : "未知错误";
        QMessageBox::warning(this, "审批失败", errorMsg);
        logMessage(QString("预约审批失败: %1").arg(errorMsg));
    }
}

void MainWindow::handlePlaceListResponse(const ProtocolParser::ParseResult &result)
{
    QString payload = QString::fromStdString(result.payload);
    QStringList places = payload.split(';', Qt::SkipEmptyParts);

    if (m_reservationPage) {
        // 清空旧数据
        m_reservationPage->m_placeComboApply->clear();
        m_reservationPage->m_placeComboQuery->clear();
        m_reservationPage->m_placeComboQuery->addItem("全部场所", "all");

        // 填充场所列表
        for (const QString &placeStr : places) {
            QStringList fields = placeStr.split('|');
            if (fields.size() >= 3) {
                QString placeId = fields[0];
                QString placeName = fields[1];
                QString equipmentIds = fields[2];
                m_reservationPage->m_placeComboApply->addItem(placeName, placeId);
                QStringList equipmentList = equipmentIds.split(',');
                int index = m_reservationPage->m_placeComboApply->count() - 1;
                m_reservationPage->m_placeComboApply->setItemData(
                    index,
                    equipmentList,
                    Qt::UserRole + 1);
                m_reservationPage->m_placeComboQuery->addItem(placeName, placeId);
            }
        }

        // 修复：如果有场所，自动选中第一个并显示设备信息
        if (m_reservationPage->m_placeComboApply->count() > 0) {
            m_reservationPage->m_placeComboApply->setCurrentIndex(0);
            // 延迟一点时间确保UI更新完成
            QTimer::singleShot(100, m_reservationPage, &ReservationWidget::updateEquipmentListDisplay);
        }
    }

    emit m_reservationPage->placeListLoaded();
}

void MainWindow::handleEnergyResponse(const ProtocolParser::ParseResult &result)
{
    qDebug() << "=== 能耗响应接收调试 ===";
    qDebug() << "equipment_id:" << QString::fromStdString(result.equipment_id);
    qDebug() << "payload:" << QString::fromStdString(result.payload);

    QString data = QString::fromStdString(result.payload);

    // 检查是否为错误响应
    if (data.startsWith("fail|")) {
        QString errorMsg = data.mid(5);
        QMessageBox::warning(this, "查询失败", errorMsg);
        logMessage(QString("能耗查询失败: %1").arg(errorMsg));
        return;
    }

    // 检查数据是否为空
    if (data.isEmpty()) {
        QMessageBox::information(this, "查询结果", "返回数据为空");
        return;
    }

    if (m_energyPage) {
        m_energyPage->updateEnergyChart(data);
    } else {
        qWarning() << "EnergyStatisticsWidget 未初始化！";
    }

    logMessage(QString("能耗数据接收成功，准备显示"));
}

void MainWindow::handleQtHeartbeatResponse(const ProtocolParser::ParseResult &result)
{
    QString clientId = QString::fromStdString(result.equipment_id);
    QString timestamp = QString::fromStdString(result.payload);

    qDebug() << "收到服务端心跳响应:" << clientId << "时间戳:" << timestamp;
    logMessage(QString("心跳响应: %1").arg(clientId));
}

void MainWindow::handleAlertMessage(const ProtocolParser::ParseResult &result)
{
    // 解析payload: "alarm_type|severity|message"
    QString payload = QString::fromStdString(result.payload);
    QStringList parts = payload.split('|');

    if (parts.size() < 3) {
        qWarning() << "告警消息格式错误:" << payload;
        return;
    }

    QString alarmType = parts[0];
    QString severity = parts[1];
    QString message = parts[2];

    // 弹窗显示
    QMessageBox::warning(this, "系统告警", message);

    logMessage("[告警] " + message);  // 只使用 logMessage
}


void MainWindow::setupPermissionByRole() {
    bool isAdmin = (m_userRole == "admin");

    // 更新菜单项权限
    if (m_reservationAction) {
        m_reservationAction->setEnabled(true); // 所有用户都可预约
    }
    if (m_energyAction) {
        m_energyAction->setEnabled(isAdmin); // 仅管理员可查看能耗
    }

    // 更新导航栏显示 - 这是关键修复！
    if (m_navigationTree) {
        for (int i = 0; i < m_navigationTree->topLevelItemCount(); ++i) {
            QTreeWidgetItem *item = m_navigationTree->topLevelItem(i);
            int pageIndex = item->data(0, Qt::UserRole).toInt();

            // 普通用户隐藏能耗统计和系统设置
            if (!isAdmin && (pageIndex == PAGE_ENERGY || pageIndex == PAGE_SETTINGS)) {
                item->setHidden(true);
            } else {
                item->setHidden(false);
            }
        }
    }
}

void MainWindow::setupMessageHandlers() {
    if (!m_dispatcher) return;

    // 设备列表响应 - 只注册一次
    m_dispatcher->registerHandler(ProtocolParser::QT_EQUIPMENT_LIST_RESPONSE,
                                  [this](const ProtocolParser::ParseResult &result) {
                                      QMetaObject::invokeMethod(this, [this, result]() {
                                          if (m_equipmentPage) {
                                              m_equipmentPage->handleEquipmentListResponse(result);

                                              // 更新状态栏的设备数量
                                              int rowCount = m_equipmentPage->m_equipmentModel->rowCount();
                                              QLabel *deviceLabel = m_statusBar->findChild<QLabel*>("deviceCountLabel");
                                              if (deviceLabel) {
                                                  deviceLabel->setText(QString("设备: %1").arg(rowCount));
                                              }
                                          }
                                      });
                                  });

    // 状态更新 - 修复：使用 m_equipmentPage
    m_dispatcher->registerHandler(ProtocolParser::STATUS_UPDATE,
                                  [this](const ProtocolParser::ParseResult &result) {
                                      QMetaObject::invokeMethod(this, [this, result]() {
                                          if (m_equipmentPage) {
                                              m_equipmentPage->handleEquipmentStatusUpdate(result);
                                          }
                                      });
                                  });

    // 控制响应 - 修复：使用 m_equipmentPage
    m_dispatcher->registerHandler(ProtocolParser::CONTROL_RESPONSE,
                                  [this](const ProtocolParser::ParseResult &result) {
                                      QMetaObject::invokeMethod(this, [this, result]() {
                                          if (m_equipmentPage) {
                                              m_equipmentPage->handleControlResponse(result);
                                          }
                                      });
                                  });

    // 场所列表响应 - 修复：使用 m_reservationPage
    m_dispatcher->registerHandler(ProtocolParser::QT_PLACE_LIST_RESPONSE,
                                  [this](const ProtocolParser::ParseResult &result) {
                                      QMetaObject::invokeMethod(this, [this, result]() {
                                          this->handlePlaceListResponse(result);
                                      });
                                  });

    // 预约申请响应
    m_dispatcher->registerHandler(ProtocolParser::RESERVATION_APPLY,
                                  [this](const ProtocolParser::ParseResult &result) {
                                      QMetaObject::invokeMethod(this, [this, result]() {
                                          this->handleReservationApplyResponse(result);
                                      });
                                  });

    // 预约查询响应
    m_dispatcher->registerHandler(ProtocolParser::RESERVATION_QUERY,
                                  [this](const ProtocolParser::ParseResult &result) {
                                      QMetaObject::invokeMethod(this, [this, result]() {
                                          this->handleReservationQueryResponse(result);
                                      });
                                  });

    // 预约审批响应
    m_dispatcher->registerHandler(ProtocolParser::RESERVATION_APPROVE,
                                  [this](const ProtocolParser::ParseResult &result) {
                                      QMetaObject::invokeMethod(this, [this, result]() {
                                          this->handleReservationApproveResponse(result);
                                      });
                                  });

    // 能耗响应 - 修复：使用 m_energyPage
    m_dispatcher->registerHandler(ProtocolParser::QT_ENERGY_RESPONSE,
                                  [this](const ProtocolParser::ParseResult &result) {
                                      QMetaObject::invokeMethod(this, [this, result]() {
                                          this->handleEnergyResponse(result);
                                      });
                                  });

    // 心跳响应
    m_dispatcher->registerHandler(ProtocolParser::QT_HEARTBEAT_RESPONSE,
                                  [this](const ProtocolParser::ParseResult &result) {
                                      QMetaObject::invokeMethod(this, [this, result]() {
                                          this->handleQtHeartbeatResponse(result);
                                      });
                                  });

    // 告警消息
    m_dispatcher->registerHandler(ProtocolParser::QT_ALERT_MESSAGE,
                                  [this](const ProtocolParser::ParseResult &result) {
                                      QMetaObject::invokeMethod(this, [this, result]() {
                                          this->handleAlertMessage(result);
                                      });
                                  });
}

void MainWindow::showReservationWidget()
{
    // 改为切换到预约页面，而不是打开独立窗口
    switchPage(PAGE_RESERVATION);

    // 如果需要，可以在这里触发场所列表加载
    if (m_reservationPage && m_tcpClient && m_tcpClient->isConnected()) {
        // 请求场所列表
        std::vector<char> msg = ProtocolParser::pack_message(
            ProtocolParser::build_message_body(
                ProtocolParser::CLIENT_QT_CLIENT,
                ProtocolParser::QT_PLACE_LIST_QUERY,
                "",  // equipment_id为空
                {""} // payload为空
                )
            );
        m_tcpClient->sendData(QByteArray(msg.data(), msg.size()));
        logMessage("已发送场所列表查询请求");
    }
}

void MainWindow::showEnergyStatisticsWidget()
{
    // 改为切换到能耗页面
    switchPage(PAGE_ENERGY);

    // 如果需要，可以在这里加载设备列表
    if (m_energyPage && m_equipmentPage) {
        QStringList equipmentIds;
        QStandardItemModel* model = m_equipmentPage->m_equipmentModel;
        if (model) {
            for (int row = 0; row < model->rowCount(); ++row) {
                equipmentIds << model->item(row, 0)->text();
            }
        }
        m_energyPage->setEquipmentList(equipmentIds);
    }
}

void MainWindow::onEnergyQueryRequested(const QString &equipmentId, const QString &timeRange)
{
    if (!m_tcpClient || !m_tcpClient->isConnected()) {
        QMessageBox::warning(this, "查询失败", "网络未连接");
        return;
    }

    // 修复：直接调用widget的公有方法，避免findChild
    QString startDate = m_energyPage->getStartDate().toString("yyyy-MM-dd");  // 改为 m_energyPage
    QString endDate = m_energyPage->getEndDate().toString("yyyy-MM-dd");  // 改为 m_energyPage

    qDebug() << "发送能耗查询:" << equipmentId << timeRange << startDate << endDate;

    // 构建payload
    QString payload = QString("%1|%2|%3").arg(timeRange).arg(startDate).arg(endDate);

    // 发送查询
    std::vector<char> packet = ProtocolParser::pack_message(
        ProtocolParser::build_message_body(
            ProtocolParser::CLIENT_QT_CLIENT,
            ProtocolParser::QT_ENERGY_QUERY,
            equipmentId.toStdString(),
            {payload.toStdString()}
            )
        );

    m_tcpClient->sendData(QByteArray(packet.data(), packet.size()));
    logMessage(QString("能耗查询已发送: %1 %2 %3至%4").arg(equipmentId, timeRange, startDate, endDate));
}
