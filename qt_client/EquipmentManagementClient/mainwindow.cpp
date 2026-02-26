#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <QTime>
#include <QTimer>
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
    // 初始化仪表板控件指针
    , m_totalDevicesLabel(nullptr)
    , m_onlineDevicesLabel(nullptr)
    , m_offlineDevicesLabel(nullptr)
    , m_reservedDevicesLabel(nullptr)
    , m_todayEnergyLabel(nullptr)
    , m_activeAlertsLabel(nullptr)
    , m_todayReservationsLabel(nullptr)
    , m_placeUsageLabel(nullptr)
    , m_alertTextEdit(nullptr)
    , m_activityTextEdit(nullptr)
    , m_alarmPage(nullptr)
    , m_isRequestingTodayEnergy(false)
    , m_isRequestingTodayReservations(false)
    , m_isRequestingPlaceUsage(false)
    , m_isRequestingAlarms(false)
{
    ui->setupUi(this);

    // 设置窗口属性
    setMinimumSize(1024, 768);
    setWindowTitle(QString("校园设备综合管理系统 - 用户: %1 (ID: %2)").arg(username).arg(userId));

    // 初始化UI
    setupUI();

    // 启动Qt客户端心跳（如果已连接）
    if (m_tcpClient && m_tcpClient->isConnected()) {
        QString heartbeatId = QString("qt_client_%1").arg(m_currentUsername);
        m_tcpClient->startHeartbeat(heartbeatId, 30); // 30秒间隔
        logMessage(QString("心跳已启动，标识: %1").arg(heartbeatId));
    }

    // 安装事件过滤器
    installEventFilter(this);

    // 设置预约页面的用户角色
    if (m_reservationPage) {
        m_reservationPage->setUserRole(m_userRole, QString::number(m_currentUserId));
    }

    m_alarms.clear();   // 初始化告警列表

    logMessage(QString("系统启动完成，欢迎 %1 (ID: %2, 角色: %3)").arg(username).arg(userId).arg(role));
    // 延迟100ms启动初始数据请求，确保窗口已显示且网络稳定
    QTimer::singleShot(100, this, &MainWindow::requestInitialData);
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

    // 添加工具按钮，移除原有的"连接状态"按钮
    QAction *refreshAction = m_toolBar->addAction(QIcon(":/icons/refresh.png"), "刷新");
    m_toolBar->addSeparator();
    QAction *dashboardAction = m_toolBar->addAction(QIcon(":/icons/dashboard.png"), "仪表板");
    QAction *equipmentAction = m_toolBar->addAction(QIcon(":/icons/equipment.png"), "设备管理");

    // 移除原有的连接状态检查逻辑
    connect(refreshAction, &QAction::triggered, [this]() {
        // 根据当前页面决定刷新什么
        int currentPage = m_centralStack->currentIndex();
        switch (currentPage) {
        case PAGE_EQUIPMENT:
            if (m_equipmentPage) {
                qDebug() << "手动刷新设备列表";
                m_equipmentPage->requestEquipmentList();
                updateDashboardStats(); // 刷新仪表板数据
            }
            break;
        case PAGE_RESERVATION:
            // 可以添加预约页面的刷新逻辑
            break;
        case PAGE_ENERGY:
            // 可以添加能耗页面的刷新逻辑
            break;
        case PAGE_DASHBOARD:
            // 刷新仪表板数据
            updateDashboardStats();
            break;
        }
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
    //给整个中央区域设置浅灰色背景
    // 1. 仪表板页面 - 完全重写
    m_dashboardPage = new QWidget();
    m_dashboardPage->setObjectName("dashboardPage");

    // 设置主布局
    QVBoxLayout *mainLayout = new QVBoxLayout(m_dashboardPage);
    mainLayout->setContentsMargins(20, 15, 20, 15);
    mainLayout->setSpacing(20);

    // 顶部欢迎区域
    QWidget *welcomeSection = new QWidget(m_dashboardPage);
    welcomeSection->setObjectName("welcomeSection");
    QHBoxLayout *welcomeLayout = new QHBoxLayout(welcomeSection);
    welcomeLayout->setContentsMargins(0, 0, 0, 0);

    QLabel *welcomeText = new QLabel(
        QString("<h1 style='margin:0;'>欢迎回来，%1</h1>"
                "<p style='color:#7f8c8d; margin:5px 0 0 0;'>上次登录时间: %2</p>")
            .arg(m_currentUsername)
            .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss")),
        welcomeSection);

    QPushButton *quickRefreshBtn = new QPushButton("🔄 刷新数据", welcomeSection);
    quickRefreshBtn->setProperty("class", "primary-button");
    quickRefreshBtn->setMinimumSize(120, 36);

    welcomeLayout->addWidget(welcomeText);
    welcomeLayout->addStretch();
    welcomeLayout->addWidget(quickRefreshBtn);

    // 统计卡片区域 - 2行网格
    QWidget *statsSection = new QWidget(m_dashboardPage);
    QGridLayout *statsGrid = new QGridLayout(statsSection);
    statsGrid->setContentsMargins(0, 0, 0, 0);
    statsGrid->setHorizontalSpacing(15);
    statsGrid->setVerticalSpacing(15);

    // 创建统计卡片函数
    auto createStatCard = [statsSection, this](const QString &title, const QString &value,
                                               const QString &icon, const QString &color,
                                               QLabel **valueLabelPtr = nullptr) -> QWidget* {
        QWidget *card = new QWidget(statsSection);
        card->setMinimumHeight(120);
        card->setStyleSheet(QString(
                                "QWidget {"
                                "    background-color: white;"
                                "    border-radius: 8px;"
                                "    border: 1px solid #e0e0e0;"
                                "}"
                                "QWidget:hover {"
                                "    border-color: %1;"
                                "    background-color: #f8f9fa;"
                                "    cursor: pointer;"
                                "}"
                                ).arg(color));

        // 为卡片添加点击事件
        card->installEventFilter(this);
        card->setProperty("cardType", title);

        QVBoxLayout *cardLayout = new QVBoxLayout(card);
        cardLayout->setContentsMargins(20, 20, 20, 20);
        cardLayout->setSpacing(12);

        // 图标和标题
        QHBoxLayout *headerLayout = new QHBoxLayout();
        headerLayout->setContentsMargins(0, 0, 0, 0);

        QLabel *iconLabel = new QLabel(card);
        iconLabel->setStyleSheet(QString(
                                     "QLabel {"
                                     "    font-family: 'Font Awesome 6 Free';"
                                     "    font-size: 20px;"
                                     "    color: %1;"
                                     "}"
                                     ).arg(color));

        // 简单模拟图标，实际使用FontAwesome
        if (icon == "devices") iconLabel->setText("📱");
        else if (icon == "online") iconLabel->setText("🟢");
        else if (icon == "offline") iconLabel->setText("🔴");
        else if (icon == "reserved") iconLabel->setText("🟡");
        else if (icon == "energy") iconLabel->setText("⚡");
        else if (icon == "alert") iconLabel->setText("🚨");
        else if (icon == "reservation") iconLabel->setText("📅");
        else if (icon == "usage") iconLabel->setText("📊");

        QLabel *titleLabel = new QLabel(title, card);
        titleLabel->setStyleSheet("font-weight: bold; color: #666; font-size: 14px;");

        headerLayout->addWidget(iconLabel);
        headerLayout->addWidget(titleLabel);
        headerLayout->addStretch();

        // 数值
        QLabel *valueLabel = new QLabel(value, card);
        valueLabel->setStyleSheet("font-size: 28px; font-weight: bold; color: #2c3e50;");

        // 保存数值标签指针
        if (valueLabelPtr) {
            *valueLabelPtr = valueLabel;
        }

        // 底部描述
        QLabel *descLabel = new QLabel("点击查看详情", card);
        descLabel->setStyleSheet("color: #95a5a6; font-size: 12px;");

        cardLayout->addLayout(headerLayout);
        cardLayout->addWidget(valueLabel);
        cardLayout->addWidget(descLabel);
        cardLayout->addStretch();

        return card;
    };

    // 修改统计卡片创建，保存指针
    statsGrid->addWidget(createStatCard("设备总数", "0", "devices", "#3498db", &m_totalDevicesLabel), 0, 0);
    statsGrid->addWidget(createStatCard("在线设备", "0", "online", "#27ae60", &m_onlineDevicesLabel), 0, 1);
    statsGrid->addWidget(createStatCard("离线设备", "0", "offline", "#e74c3c", &m_offlineDevicesLabel), 0, 2);
    statsGrid->addWidget(createStatCard("预约中", "0", "reserved", "#f39c12", &m_reservedDevicesLabel), 0, 3);

    // 第二行：能耗和告警卡片
    statsGrid->addWidget(createStatCard("今日能耗", "0 kWh", "energy", "#9b59b6", &m_todayEnergyLabel), 1, 0);
    statsGrid->addWidget(createStatCard("待处理告警", "0", "alert", "#e67e22", &m_activeAlertsLabel), 1, 1);
    statsGrid->addWidget(createStatCard("今日预约", "0", "reservation", "#1abc9c", &m_todayReservationsLabel), 1, 2);
    statsGrid->addWidget(createStatCard("场所使用率", "0%", "usage", "#34495e", &m_placeUsageLabel), 1, 3);

    // 在实时信息区域保存控件指针
    //m_alertTextEdit = alertList;
    //m_activityTextEdit = logList;

    // 快速操作区域
    QWidget *quickActionsSection = new QWidget(m_dashboardPage);
    QVBoxLayout *actionsLayout = new QVBoxLayout(quickActionsSection);
    actionsLayout->setContentsMargins(0, 0, 0, 0);

    QLabel *actionsTitle = new QLabel("快速操作");
    actionsTitle->setStyleSheet("font-size: 16px; font-weight: bold; color: #2c3e50; margin-bottom: 15px;");

    QGridLayout *actionsGrid = new QGridLayout();
    actionsGrid->setSpacing(15);

    // 创建快速操作按钮
    auto createActionButton = [quickActionsSection](const QString &text, const QString &icon,
                                                    const QString &desc) -> QPushButton* {
        QPushButton *btn = new QPushButton(quickActionsSection);
        btn->setMinimumSize(200, 80);
        btn->setStyleSheet(
            "QPushButton {"
            "    background-color: white;"
            "    border: 1px solid #e0e0e0;"
            "    border-radius: 8px;"
            "    text-align: left;"
            "    padding: 15px;"
            "}"
            "QPushButton:hover {"
            "    border-color: #4a69bd;"
            "    background-color: #f8f9fa;"
            "}"
            );

        QHBoxLayout *btnLayout = new QHBoxLayout(btn);
        btnLayout->setContentsMargins(10, 10, 10, 10);

        // 图标
        QLabel *iconLabel = new QLabel(btn);
        iconLabel->setStyleSheet("font-size: 24px; color: #4a69bd;");
        if (icon == "refresh") iconLabel->setText("🔄");
        else if (icon == "reserve") iconLabel->setText("📅");
        else if (icon == "energy") iconLabel->setText("📊");
        else if (icon == "control") iconLabel->setText("🎛️");
        else if (icon == "report") iconLabel->setText("📈");

        // 文本
        QVBoxLayout *textLayout = new QVBoxLayout();
        textLayout->setSpacing(4);

        QLabel *titleLabel = new QLabel(text, btn);
        titleLabel->setStyleSheet("font-weight: bold; font-size: 14px; color: #2c3e50;");

        QLabel *descLabel = new QLabel(desc, btn);
        descLabel->setStyleSheet("color: #7f8c8d; font-size: 12px;");

        textLayout->addWidget(titleLabel);
        textLayout->addWidget(descLabel);
        textLayout->addStretch();

        btnLayout->addWidget(iconLabel);
        btnLayout->addLayout(textLayout);
        btnLayout->addStretch();

        return btn;
    };

    actionsGrid->addWidget(createActionButton("刷新设备列表", "refresh", "获取最新设备状态"), 0, 0);
    actionsGrid->addWidget(createActionButton("预约会议室", "reserve", "快速预约设备/场所"), 0, 1);
    actionsGrid->addWidget(createActionButton("能耗分析", "energy", "查看能耗统计数据"), 0, 2);
    actionsGrid->addWidget(createActionButton("设备控制", "control", "批量开关设备"), 1, 0);
    actionsGrid->addWidget(createActionButton("生成报告", "report", "导出设备使用报告"), 1, 1);
    actionsGrid->addWidget(createActionButton("查看告警", "alert", "查看系统告警信息"), 1, 2);

    actionsLayout->addWidget(actionsTitle);
    actionsLayout->addLayout(actionsGrid);

    // 实时信息区域
    QWidget *realtimeSection = new QWidget(m_dashboardPage);
    QHBoxLayout *realtimeLayout = new QHBoxLayout(realtimeSection);
    realtimeLayout->setContentsMargins(0, 0, 0, 0);
    realtimeLayout->setSpacing(15);

    // 最近告警卡片
    QWidget *alertCard = new QWidget(realtimeSection);
    alertCard->setMinimumHeight(200);
    alertCard->setStyleSheet("background-color: white; border-radius: 8px; border: 1px solid #e0e0e0;");

    QVBoxLayout *alertLayout = new QVBoxLayout(alertCard);
    alertLayout->setContentsMargins(20, 15, 20, 15);

    QLabel *alertTitle = new QLabel("最近告警");
    alertTitle->setStyleSheet("font-size: 16px; font-weight: bold; color: #2c3e50; margin-bottom: 10px;");

    QTextEdit *alertList = new QTextEdit(alertCard);
    alertList->setReadOnly(true);
    alertList->setPlaceholderText("暂无告警信息");
    alertList->setStyleSheet(
        "QTextEdit {"
        "    border: none;"
        "    background-color: transparent;"
        "    font-size: 12px;"
        "}"
        );

    alertLayout->addWidget(alertTitle);
    alertLayout->addWidget(alertList);

    // 设备活动日志卡片
    QWidget *logCard = new QWidget(realtimeSection);
    logCard->setMinimumHeight(200);
    logCard->setStyleSheet("background-color: white; border-radius: 8px; border: 1px solid #e0e0e0;");

    QVBoxLayout *logLayout = new QVBoxLayout(logCard);
    logLayout->setContentsMargins(20, 15, 20, 15);

    QLabel *logTitle = new QLabel("最近活动");
    logTitle->setStyleSheet("font-size: 16px; font-weight: bold; color: #2c3e50; margin-bottom: 10px;");

    QTextEdit *logList = new QTextEdit(logCard);
    logList->setReadOnly(true);
    logList->setPlaceholderText("暂无活动记录");
    logList->setStyleSheet(
        "QTextEdit {"
        "    border: none;"
        "    background-color: transparent;"
        "    font-size: 12px;"
        "}"
        );

    logLayout->addWidget(logTitle);
    logLayout->addWidget(logList);

    realtimeLayout->addWidget(alertCard);
    realtimeLayout->addWidget(logCard);

    // 添加到主布局
    mainLayout->addWidget(welcomeSection);
    mainLayout->addWidget(statsSection);
    mainLayout->addWidget(quickActionsSection);
    mainLayout->addWidget(realtimeSection);
    mainLayout->addStretch();

    m_centralStack->addWidget(m_dashboardPage);

    // 2. 设备管理页面（保持不变）
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

    // 连接设备管理页面的设备列表加载完成信号
    if (m_equipmentPage) {
        // 如果设备列表已存在数据，立即填充
        if (m_equipmentPage->m_equipmentModel->rowCount() > 0) {
            populateEnergyPageFilters();
        }
        // 无论是否已有数据，都连接信号确保后续更新时刷新
        connect(m_equipmentPage, &EquipmentManagerWidget::deviceListLoaded,
                this, &MainWindow::populateEnergyPageFilters);
    }
    // 连接设备列表加载完成信号，用于填充阈值页面的设备下拉框
    connect(m_equipmentPage, &EquipmentManagerWidget::deviceListLoaded,
            this, [this]() {
                // 如果阈值页面和设备页面都已就绪
                if (m_thresholdSettingsPage && m_equipmentPage) {
                    QHash<QString, QString> devMap;
                    QStandardItemModel *model = m_equipmentPage->m_equipmentModel;
                    // 遍历设备模型，提取设备ID、类型和位置
                    for (int row = 0; row < model->rowCount(); ++row) {
                        QString devId = model->item(row, 0)->text();   // 设备ID
                        QString type = model->item(row, 1)->text();   // 设备类型
                        QString loc = model->item(row, 2)->text();    // 位置
                        QString display = type + " @ " + loc;         // 组合显示名称
                        devMap[devId] = display;
                    }
                    // 将设备列表传递给阈值设置页面
                    m_thresholdSettingsPage->setEquipmentList(devMap);
                }
            });

    // 5. 系统设置页面（告警中心 + 阈值设置）
    QWidget *settingsContainer = new QWidget();
    QVBoxLayout *settingsLayout = new QVBoxLayout(settingsContainer);
    settingsLayout->setContentsMargins(0, 0, 0, 0);

    QTabWidget *settingsTab = new QTabWidget(settingsContainer);
    settingsTab->setDocumentMode(true);
    settingsTab->tabBar()->setExpanding(false);
    settingsTab->setObjectName("settingsTab");

    // 告警中心页面
    m_alarmPage = new AlarmWidget(this);
    settingsTab->addTab(m_alarmPage, "告警中心");

    // 阈值设置页面
    m_thresholdSettingsPage = new ThresholdSettingsWidget(this);
    connect(m_thresholdSettingsPage, &ThresholdSettingsWidget::setThresholdRequested,
            this, &MainWindow::onSetThresholdRequested);
    settingsTab->addTab(m_thresholdSettingsPage, "阈值设置");

    settingsLayout->addWidget(settingsTab);
    m_centralStack->addWidget(settingsContainer);

    // 连接告警确认信号
    connect(m_alarmPage, &AlarmWidget::acknowledgeAlarm,
            this, &MainWindow::onAcknowledgeAlarm);

    // 如果已有告警缓存，初始化告警页面
    if (!m_alarms.isEmpty()) {
        m_alarmPage->setAlarms(m_alarms);
    }

    // 连接快速操作按钮的信号
    QList<QPushButton*> actionButtons = quickActionsSection->findChildren<QPushButton*>();
    for (QPushButton* btn : actionButtons) {
        if (btn->text().contains("刷新设备列表")) {
            connect(btn, &QPushButton::clicked, this, [this]() {
                if (m_equipmentPage) {
                    m_equipmentPage->requestEquipmentList();
                    logMessage("已从仪表板刷新设备列表");
                }
            });
        } else if (btn->text().contains("预约会议室")) {
            connect(btn, &QPushButton::clicked, this, [this]() {
                switchPage(PAGE_RESERVATION);
            });
        } else if (btn->text().contains("能耗分析")) {
            connect(btn, &QPushButton::clicked, this, [this]() {
                switchPage(PAGE_ENERGY);
            });
        }
    }
    // 设置仪表板信号连接
    setupDashboardConnections();

    // 初始更新仪表板数据
    updateDashboardStats();
}

void MainWindow::requestTodayEnergy()
{
    if (!m_tcpClient || !m_tcpClient->isConnected()) return;
    if (m_isRequestingTodayEnergy) {
        logMessage("今日能耗请求已在进行中，忽略");
        return;
    }
    QDate today = QDate::currentDate();
    QString payload = QString("day|%1|%1").arg(today.toString("yyyy-MM-dd"));
    std::string body = ProtocolParser::build_message_body(
        ProtocolParser::CLIENT_QT_CLIENT,
        ProtocolParser::QT_ENERGY_QUERY,
        "all",
        {payload.toStdString()}
        );
    std::vector<char> packet = ProtocolParser::pack_message(body);
    m_tcpClient->sendData(QByteArray(packet.data(), packet.size()));
    m_isRequestingTodayEnergy = true;
    logMessage("请求今日总能耗");
}

void MainWindow::requestUnreadAlarms()
{
    if (!m_tcpClient || !m_tcpClient->isConnected()) return;
    if (m_isRequestingAlarms) {
        logMessage("告警请求已在进行中，忽略");
        return;
    }
    std::vector<char> packet = ProtocolParser::build_alarm_query_message(
        ProtocolParser::CLIENT_QT_CLIENT
        );
    m_tcpClient->sendData(QByteArray(packet.data(), packet.size()));
    m_isRequestingAlarms = true;
    logMessage("请求未处理告警列表");
}

void MainWindow::switchToAlarmCenter()
{
    // 切换到设置页面（包含告警中心）
    switchPage(PAGE_SETTINGS);

    // 获取设置页面容器中的 TabWidget，并选中告警中心标签（索引0）
    QWidget *settingsContainer = m_centralStack->widget(PAGE_SETTINGS);
    if (settingsContainer) {
        // 通过对象名查找（需要在 setupCentralStack 中设置 settingsTab 的对象名）
        QTabWidget *tabWidget = settingsContainer->findChild<QTabWidget*>("settingsTab");
        if (tabWidget) {
            tabWidget->setCurrentIndex(0); // 告警中心索引为0
        }
    }

    logMessage("跳转到告警中心");
}

void MainWindow::requestTodayReservations()
{
    if (!m_tcpClient || !m_tcpClient->isConnected()) return;
    if (m_isRequestingTodayReservations) {
        logMessage("今日预约请求已在进行中，忽略");
        return;
    }
    std::vector<char> msg = ProtocolParser::build_reservation_query(
        ProtocolParser::CLIENT_QT_CLIENT,
        ""
        );
    m_tcpClient->sendData(QByteArray(msg.data(), msg.size()));
    m_isRequestingTodayReservations = true;
    logMessage("请求今日预约数量");
}

void MainWindow::requestInitialData()
{
    // 请求设备列表（设备页面会自动处理响应并更新仪表板）
    if (m_equipmentPage) {
        m_equipmentPage->requestEquipmentList();
    }

    // 请求场所列表（用于预约页面和场所使用率）
    if (m_tcpClient && m_tcpClient->isConnected()) {
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

    // 调用 updateDashboardStats 发送能耗、告警、预约请求
    updateDashboardStats();

    logMessage("初始数据请求已发送");
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
        case PAGE_DASHBOARD:
            // 切换到仪表板时自动刷新数据
            updateDashboardStats();
            logMessage("仪表板数据已刷新");
            break;

        case PAGE_EQUIPMENT:
            // if (m_equipmentPage) {
            //     // 只有在首次切换到设备页面或手动刷新时才请求
            //     if (!loadedPages.contains(PAGE_EQUIPMENT)) {
            //         m_equipmentPage->requestEquipmentList();
            //         loadedPages.insert(PAGE_EQUIPMENT);
            //         qDebug() << "首次切换到设备页面，发送设备列表查询请求";
            //     } else {
            //         qDebug() << "设备页面已加载过，跳过自动刷新";
            //     }
            // }
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
        case PAGE_SETTINGS:
            if (m_tcpClient && m_tcpClient->isConnected()) {
                // 请求阈值
                std::vector<char> msg = ProtocolParser::build_get_all_thresholds_message(
                    ProtocolParser::CLIENT_QT_CLIENT);
                m_tcpClient->sendData(QByteArray(msg.data(), msg.size()));
                logMessage("请求所有设备阈值...");

                // 请求告警列表
                std::vector<char> alarmMsg = ProtocolParser::build_alarm_query_message(
                    ProtocolParser::CLIENT_QT_CLIENT);
                m_tcpClient->sendData(QByteArray(alarmMsg.data(), alarmMsg.size()));
                logMessage("请求未处理告警列表...");
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

void MainWindow::populateEnergyPageFilters()
{
    if (!m_energyPage || !m_equipmentPage || !m_equipmentPage->m_equipmentModel) {
        qDebug() << "能耗页面或设备模型未就绪，跳过填充";
        return;
    }

    QStandardItemModel *model = m_equipmentPage->m_equipmentModel;
    if (model->rowCount() == 0) {
        qDebug() << "设备模型当前无数据，等待下次信号";
        return;
    }

    QSet<QString> typeSet, placeSet;
    QHash<QString, QPair<QString, QString>> devInfoMap;

    for (int row = 0; row < model->rowCount(); ++row) {
        QString devId   = model->item(row, 0)->text(); // 设备ID
        QString type    = model->item(row, 1)->text(); // 设备类型
        QString place   = model->item(row, 2)->text(); // 场所
        typeSet.insert(type);
        placeSet.insert(place);
        devInfoMap.insert(devId, qMakePair(type, place));
    }

    m_energyPage->setDeviceTypeList(typeSet.values());
    m_energyPage->setPlaceList(placeSet.values());
    m_energyPage->setDeviceInfoMap(devInfoMap);

    qDebug() << "能耗页面筛选条件填充完成，类型:" << typeSet.size() << "场所:" << placeSet.size();
}

void MainWindow::onSetThresholdRequested(const QString &equipmentId, double value)
{
    if (!m_tcpClient || !m_tcpClient->isConnected()) {
        QMessageBox::warning(this, "设置失败", "网络未连接");
        return;
    }

    // 使用协议构建消息（需在 ProtocolParser 中添加对应函数）
    std::vector<char> msg = ProtocolParser::build_set_threshold_message(
        ProtocolParser::CLIENT_QT_CLIENT,
        equipmentId.toStdString(),
        value
        );
    m_tcpClient->sendData(QByteArray(msg.data(), msg.size()));
    logMessage(QString("发送阈值设置请求: %1 = %2 W").arg(equipmentId).arg(value));
}

void MainWindow::handleSetThresholdResponse(const ProtocolParser::ParseResult &result)
{
    QString payload = QString::fromStdString(result.payload);
    QStringList parts = payload.split('|');
    bool success = (parts.size() >= 1 && parts[0] == "success");
    QString message = parts.size() >= 2 ? parts[1] : "";

    if (m_thresholdSettingsPage) {
        m_thresholdSettingsPage->handleSetThresholdResponse(success, message);
    }
}

void MainWindow::handleGetAllThresholdsResponse(const ProtocolParser::ParseResult &result)
{
    QString payload = QString::fromStdString(result.payload);
    // payload 格式: "success|dev1|val1;dev2|val2..." 或 "fail|error"
    QStringList parts = payload.split('|', Qt::SkipEmptyParts);
    if (parts.isEmpty()) return;
    bool success = (parts[0] == "success");
    if (success) {
        // 取出数据部分（可能包含多个|，但数据部分是用分号分隔的，所以重新组合）
        QString data = parts.mid(1).join('|'); // 注意：数据中可能包含|，所以需要重新组合
        QHash<QString, double> thresholds;
        QStringList records = data.split(';', Qt::SkipEmptyParts);
        for (const QString &rec : records) {
            QStringList kv = rec.split('|');
            if (kv.size() >= 2) {
                thresholds[kv[0]] = kv[1].toDouble();
            }
        }
        if (m_thresholdSettingsPage) {
            m_thresholdSettingsPage->setCurrentThresholds(thresholds);
        }
    } else {
        QString errorMsg = parts.mid(1).join('|');
        qDebug() << "获取阈值失败:" << errorMsg;
        if (m_thresholdSettingsPage) {
            m_thresholdSettingsPage->setCurrentThresholds(QHash<QString, double>());
        }
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

// 更新仪表板统计数据
void MainWindow::updateDashboardStats()
{
    // 从设备管理页面获取设备状态数据
    if (m_equipmentPage && m_equipmentPage->m_equipmentModel) {
        QStandardItemModel* model = m_equipmentPage->m_equipmentModel;
        int totalDevices = model->rowCount();
        int onlineDevices = 0;
        int offlineDevices = 0;
        int reservedDevices = 0;

        for (int i = 0; i < totalDevices; ++i) {
            QString status = model->item(i, 3)->text(); // 第3列是状态
            if (status == "online") {
                onlineDevices++;
            } else if (status == "offline") {
                offlineDevices++;
            } else if (status == "reserved") {
                reservedDevices++;
            }
        }

        if (m_totalDevicesLabel) m_totalDevicesLabel->setText(QString::number(totalDevices));
        if (m_onlineDevicesLabel) m_onlineDevicesLabel->setText(QString::number(onlineDevices));
        if (m_offlineDevicesLabel) m_offlineDevicesLabel->setText(QString::number(offlineDevices));
        if (m_reservedDevicesLabel) m_reservedDevicesLabel->setText(QString::number(reservedDevices));

        QLabel *deviceLabel = m_statusBar->findChild<QLabel*>("deviceCountLabel");
        if (deviceLabel) {
            deviceLabel->setText(QString("设备: %1").arg(totalDevices));
        }
    }

    // 更新最近告警和活动日志（模拟，可暂时保留）
    updateRecentAlerts();
    updateActivityLog();

    // 发起请求获取实时数据
    requestTodayEnergy();
    requestUnreadAlarms();
    requestTodayReservations();
}

// 更新最近告警
void MainWindow::updateRecentAlerts()
{
    if (!m_alertTextEdit) return;

    // 模拟告警数据
    QStringList alerts;
    alerts << "10:25 设备 projector_101 离线"
           << "09:15 会议室 classroom_101 能耗异常"
           << "08:30 空调 aircon_202 温度过高";

    m_alertTextEdit->clear();
    for (const QString &alert : alerts) {
        m_alertTextEdit->append(alert);
    }
}

// 更新活动日志
void MainWindow::updateActivityLog()
{
    if (!m_activityTextEdit) return;

    // 模拟活动数据
    QStringList activities;
    activities << "10:30 用户 admin 登录系统"
               << "10:15 设备 projector_101 被开机"
               << "09:45 会议室 classroom_102 预约成功"
               << "09:20 能耗数据已更新"
               << "08:50 设备 camera_301 离线告警";

    m_activityTextEdit->clear();
    for (const QString &activity : activities) {
        m_activityTextEdit->append(activity);
    }
}

// 设置仪表板信号连接
void MainWindow::setupDashboardConnections()
{
    // 在设备列表更新时刷新仪表板
    if (m_equipmentPage) {
        connect(m_equipmentPage, &EquipmentManagerWidget::showStatusMessage,
                this, [this](const QString &msg) {
                    // 如果是设备列表相关消息，更新仪表板
                    if (msg.contains("设备") || msg.contains("更新")) {
                        QTimer::singleShot(500, this, &MainWindow::updateDashboardStats);
                    }
                });
    }
}

// 重写事件过滤器，处理卡片点击
bool MainWindow::eventFilter(QObject *watched, QEvent *event)
{
    if (event->type() == QEvent::MouseButtonPress) {
        QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
        if (mouseEvent->button() == Qt::LeftButton) {
            // 检查是否是仪表板卡片
            QString cardType = watched->property("cardType").toString();
            if (!cardType.isEmpty()) {
                // 根据卡片类型执行不同操作
                if (cardType == "设备总数" || cardType == "在线设备" ||
                    cardType == "离线设备" || cardType == "预约中") {
                    switchPage(PAGE_EQUIPMENT);
                    logMessage(QString("切换到设备管理页面，查看%1").arg(cardType));
                }
                else if (cardType == "今日能耗") {
                    switchPage(PAGE_ENERGY);
                    logMessage("切换到能耗统计页面");
                }
                else if (cardType == "待处理告警") {
                    switchToAlarmCenter();
                    return true;
                }
                else if (cardType == "今日预约") {
                    switchPage(PAGE_RESERVATION);
                    // 切换到查询标签页（索引1）
                    if (m_reservationPage && m_reservationPage->m_tabWidget) {
                        m_reservationPage->m_tabWidget->setCurrentIndex(1);
                    }
                    logMessage("切换到预约管理页面，查看今日预约");
                    return true;
                }
                else if (cardType == "场所使用率") {
                    if (!m_tcpClient || !m_tcpClient->isConnected()) {
                        QMessageBox::warning(this, "提示", "网络未连接，无法获取场所使用率");
                        return true;
                    }
                    m_isRequestingPlaceUsage = true;
                    requestTodayReservations();   // 复用今日预约请求
                    logMessage("请求场所使用率数据");
                    return true;
                }
        }
    }
    return QMainWindow::eventFilter(watched, event);
}

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

    if (!m_tcpClient) {
        qCritical() << "错误: TCP客户端未初始化";
        return;
    }

    if (!m_tcpClient->isConnected()) {
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
        QMessageBox::warning(this, "发送失败", "预约查询请求发送失败");
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

    qDebug() << "=== 查询响应处理 ===";
    qDebug() << "原始payload:" << payload;

    // ---------- 1. 场所使用率请求（点击卡片触发） ----------
    if (m_isRequestingPlaceUsage) {
        QMap<QString, int> placeTotalMinutes;
        QStringList records = payload.split(';', Qt::SkipEmptyParts);
        for (const QString &rec : records) {
            QStringList fields = rec.split('|');
            if (fields.size() >= 5) {
                QString placeId = fields[1];
                QString startStr = fields[3];
                QString endStr = fields[4];
                QDateTime start = QDateTime::fromString(startStr, "yyyy-MM-dd HH:mm:ss");
                QDateTime end = QDateTime::fromString(endStr, "yyyy-MM-dd HH:mm:ss");
                if (start.isValid() && end.isValid()) {
                    int minutes = start.secsTo(end) / 60;
                    placeTotalMinutes[placeId] += minutes;
                }
            }
        }

        const int AVAILABLE_MINUTES = 14 * 60; // 每个场所可用840分钟

        QString usageText = "今日场所使用率统计：\n";
        if (placeTotalMinutes.isEmpty()) {
            usageText += "  暂无预约记录";
        } else {
            for (auto it = placeTotalMinutes.begin(); it != placeTotalMinutes.end(); ++it) {
                QString placeId = it.key();
                int usedMinutes = it.value();
                double percentage = (usedMinutes * 100.0) / AVAILABLE_MINUTES;
                if (percentage > 100.0) percentage = 100.0;
                QString placeDisplay = m_placeNameMap.value(placeId, placeId);
                usageText += QString("  • %1: %2 分钟 (%3%)\n")
                                 .arg(placeDisplay)
                                 .arg(usedMinutes)
                                 .arg(QString::number(percentage, 'f', 1));
            }
        }

        QMessageBox::information(this, "场所使用率", usageText);
        m_isRequestingPlaceUsage = false;
        return;
    }

    // ---------- 2. 仪表板今日预约请求（更新卡片） ----------
    if (m_isRequestingTodayReservations) {
        int todayCount = 0;
        int totalUsedMinutes = 0; // 累计所有预约的总时长（分钟）
        QDate today = QDate::currentDate();
        QStringList records = payload.split(';', Qt::SkipEmptyParts);
        for (const QString &rec : records) {
            QStringList fields = rec.split('|');
            if (fields.size() >= 5) { // 假设包含开始和结束时间
                QString startStr = fields[3];
                QString endStr = fields[4];
                QDateTime start = QDateTime::fromString(startStr, "yyyy-MM-dd HH:mm:ss");
                QDateTime end = QDateTime::fromString(endStr, "yyyy-MM-dd HH:mm:ss");
                if (start.isValid() && end.isValid() && start.date() == today) {
                    todayCount++;
                    int minutes = start.secsTo(end) / 60;
                    totalUsedMinutes += minutes;
                }
            }
        }

        // 更新今日预约数量
        if (m_todayReservationsLabel) {
            m_todayReservationsLabel->setText(QString::number(todayCount));
        }

        // 计算并更新场所使用率（整体使用率）
        if (m_placeUsageLabel) {
            int placeCount = m_placeNameMap.size();
            if (placeCount > 0) {
                const int AVAILABLE_MINUTES_PER_PLACE = 14 * 60;
                int totalAvailableMinutes = placeCount * AVAILABLE_MINUTES_PER_PLACE;
                double usagePercent = (totalUsedMinutes * 100.0) / totalAvailableMinutes;
                if (usagePercent > 100.0) usagePercent = 100.0;
                m_placeUsageLabel->setText(QString::number(usagePercent, 'f', 1) + "%");
            } else {
                // 场所列表尚未加载，暂时设为0%
                m_placeUsageLabel->setText("0%");
            }
        }

        m_isRequestingTodayReservations = false;
        return;
    }

    // ---------- 3. 原有的错误检查和预约页面分发逻辑 ----------
    if (payload.isEmpty() || payload.startsWith("fail|")) {
        QString errorMsg = payload.isEmpty() ? "查询失败" : payload.mid(5);
        qDebug() << "查询失败:" << errorMsg;
        if (m_reservationPage) {
            QMetaObject::invokeMethod(m_reservationPage, [this]() {
                m_reservationPage->updateQueryResultTable("");
            }, Qt::QueuedConnection);
        }
        return;
    }

    QString data;
    if (payload.startsWith("success|")) {
        data = payload.mid(8);
    } else {
        data = payload;
    }

    qDebug() << "处理后的数据:" << data;

    if (!m_reservationPage) {
        qDebug() << "错误: ReservationWidget 未初始化";
        return;
    }

    int currentTab = m_reservationPage->m_tabWidget->currentIndex();
    qDebug() << "当前标签页:" << currentTab;

    if (currentTab == 2) {
        qDebug() << "分发到审批页";
        QMetaObject::invokeMethod(m_reservationPage, [this, data]() {
            m_reservationPage->loadAllReservationsForApproval(data);
        }, Qt::QueuedConnection);
    } else if (currentTab == 1) {
        qDebug() << "分发到查询页";
        QMetaObject::invokeMethod(m_reservationPage, [this, data]() {
            m_reservationPage->updateQueryResultTable(data);
        }, Qt::QueuedConnection);
    } else {
        qDebug() << "数据未处理，当前标签页:" << currentTab;
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
    qDebug() << "=== 处理场所列表响应 ===";
    QString payload = QString::fromStdString(result.payload);
    qDebug() << "场所列表数据:" << payload;

    QStringList places = payload.split(';', Qt::SkipEmptyParts);
    qDebug() << "解析出场所数量:" << places.size();

    if (!m_reservationPage) {
        qDebug() << "错误: reservationPage 为空指针!";
        return;
    }

    qDebug() << "开始填充场所列表...";

    // 清空旧数据
    if (m_reservationPage->m_placeComboApply) {
        m_reservationPage->m_placeComboApply->clear();
    } else {
        qDebug() << "警告: m_placeComboApply 为空，创建新的";
        m_reservationPage->m_placeComboApply = new QComboBox(m_reservationPage);
        m_reservationPage->m_placeComboApply->setVisible(false);
    }

    if (m_reservationPage->m_placeComboQuery) {
        m_reservationPage->m_placeComboQuery->clear();
        m_reservationPage->m_placeComboQuery->addItem("全部场所", "all");
    } else {
        qDebug() << "警告: m_placeComboQuery 为空";
    }

    int validPlaces = 0;

    // 填充场所列表
    for (const QString &placeStr : places) {
        QStringList fields = placeStr.split('|');
        if (fields.size() >= 3) {
            QString placeId = fields[0];
            QString placeName = fields[1];
            QString equipmentIds = fields[2];

            // 填充场所名称映射
            m_placeNameMap[placeId] = placeName;

            qDebug() << "添加场所:" << placeName << "ID:" << placeId;

            if (m_reservationPage->m_placeComboApply) {
                m_reservationPage->m_placeComboApply->addItem(placeName, placeId);
                QStringList equipmentList = equipmentIds.split(',');
                int index = m_reservationPage->m_placeComboApply->count() - 1;
                m_reservationPage->m_placeComboApply->setItemData(
                    index,
                    equipmentList,
                    Qt::UserRole + 1);
            }

            if (m_reservationPage->m_placeComboQuery) {
                m_reservationPage->m_placeComboQuery->addItem(placeName, placeId);
            }

            validPlaces++;
        } else {
            qDebug() << "警告: 无效的场所数据格式:" << placeStr;
        }
    }

    qDebug() << "成功加载场所数量:" << validPlaces;

    if (m_reservationPage->m_placeComboApply) {
        qDebug() << "m_placeComboApply 项目数:" << m_reservationPage->m_placeComboApply->count();
    }

    // 确保在主线程执行UI更新
    QMetaObject::invokeMethod(m_reservationPage, [this]() {
        if (m_reservationPage) {
            qDebug() << "在主线程中更新场所卡片...";
            try {
                m_reservationPage->updatePlaceCards();
                qDebug() << "场所卡片更新完成";
            } catch (const std::exception &e) {
                qDebug() << "更新场所卡片时发生异常:" << e.what();
            } catch (...) {
                qDebug() << "更新场所卡片时发生未知异常";
            }
        } else {
            qDebug() << "错误: reservationPage 在延迟调用中变为空!";
        }
    });

    // ★ 新增：场所列表填充完成后，如果当前在仪表板页面，刷新预约数据以更新使用率
    if (m_centralStack && m_centralStack->currentIndex() == PAGE_DASHBOARD) {
        requestTodayReservations();
    }

    emit m_reservationPage->placeListLoaded();
}

void MainWindow::handleEnergyResponse(const ProtocolParser::ParseResult &result)
{
    QString data = QString::fromStdString(result.payload);

    // 仪表板请求分支
    if (m_isRequestingTodayEnergy) {
        double total = 0.0;
        QStringList records = data.split(';', Qt::SkipEmptyParts);
        for (const QString &rec : records) {
            QStringList fields = rec.split('|');
            if (fields.size() >= 3) {
                total += fields[2].toDouble();
            }
        }
        if (m_todayEnergyLabel)
            m_todayEnergyLabel->setText(QString::number(total, 'f', 2) + " kWh");
        m_isRequestingTodayEnergy = false;
        return;   // 仪表板请求不继续往下
    }

    // 能耗页面正常更新
    if (data.isEmpty() || data.startsWith("fail|")) {
        logMessage("能耗数据无效或查询失败");
        return;
    }
    if (m_energyPage) m_energyPage->updateEnergyChart(data);
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
    QString payload = QString::fromStdString(result.payload);
    QStringList parts = payload.split('|');
    if (parts.size() < 4) {
        qWarning() << "告警消息格式错误:" << payload;
        return;
    }

    bool ok;
    int alarmId = parts[0].toInt(&ok);
    if (!ok) alarmId = 0; // 降级处理

    QString alarmType = parts[1];
    QString severity = parts[2];
    QString message = parts[3];

    // 根据严重程度决定是否弹窗
    if (severity == "critical" || severity == "error") {
        QMessageBox::warning(this, "系统告警", message);
    } else {
        logMessage("[告警] " + message);
    }

    AlarmInfo alarm;
    alarm.id = alarmId;
    alarm.type = alarmType;
    alarm.equipmentId = QString::fromStdString(result.equipment_id);
    alarm.severity = severity;
    alarm.message = message;
    alarm.timestamp = QDateTime::currentDateTime();
    alarm.acknowledged = false;

    m_alarms.append(alarm);
    if (m_alarmPage) {
        m_alarmPage->addAlarm(alarm);
    }

    logMessage(QString("[告警] ID=%1 | %2 | %3 | %4").arg(alarmId).arg(severity, alarm.equipmentId, message));
}

void MainWindow::onAcknowledgeAlarm(int alarmId)
{
    if (!m_tcpClient || !m_tcpClient->isConnected()) {
        QMessageBox::warning(this, "操作失败", "网络未连接");
        return;
    }

    // 构建告警确认消息，payload 为告警ID
    std::vector<char> msg = ProtocolParser::build_alert_ack(
        ProtocolParser::CLIENT_QT_CLIENT,
        "",  // equipment_id 可选，可留空
        alarmId
        );
    m_tcpClient->sendData(QByteArray(msg.data(), msg.size()));
    logMessage(QString("发送告警确认: ID=%1").arg(alarmId));

    // 本地乐观更新（假设服务端成功）
    for (AlarmInfo &a : m_alarms) {
        if (a.id == alarmId) {
            a.acknowledged = true;
            break;
        }
    }
    if (m_alarmPage) {
        m_alarmPage->setAlarms(m_alarms);
    }
}

void MainWindow::handleAlarmQueryResponse(const ProtocolParser::ParseResult &result)
{
    // 重置标志（无论成功失败，请求结束）
    if (m_isRequestingAlarms) m_isRequestingAlarms = false;
    QString payload = QString::fromStdString(result.payload);
    QStringList parts = payload.split('|', Qt::KeepEmptyParts);
    if (parts.isEmpty()) return;
    bool success = (parts[0] == "success");
    if (!success) {
        qDebug() << "获取告警列表失败:" << payload;
        return;
    }
    QString data = parts.mid(1).join('|');
    QStringList records = data.split(';', Qt::SkipEmptyParts);
    QList<AlarmInfo> alarms;
    for (const QString &rec : records) {
        QStringList fields = rec.split('|');
        if (fields.size() < 5) continue;
        AlarmInfo alarm;
        alarm.id = fields[0].toInt();
        alarm.type = fields[1];
        alarm.equipmentId = fields[2];
        alarm.severity = fields[3];
        alarm.message = fields[4];
        if (fields.size() >= 6) {
            alarm.timestamp = QDateTime::fromString(fields[5], "yyyy-MM-dd HH:mm:ss");
        } else {
            alarm.timestamp = QDateTime::currentDateTime();
        }
        alarm.acknowledged = false;  // 查询的是未处理告警
        alarms.append(alarm);
    }

    if (m_alarmPage) {
        m_alarmPage->setAlarms(alarms);
    }
    m_alarms = alarms;
    // 更新待处理告警卡片
    if (m_activeAlertsLabel) {
        m_activeAlertsLabel->setText(QString::number(m_alarms.size()));
    }
    // 如果告警页面存在，更新它
    if (m_alarmPage) m_alarmPage->setAlarms(m_alarms);
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

                                              // ★ 新增：刷新仪表板上的设备卡片
                                              updateDashboardStats();
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
    //阈值设置
    m_dispatcher->registerHandler(ProtocolParser::QT_SET_THRESHOLD_RESPONSE,
                                  [this](const ProtocolParser::ParseResult &result) {
                                      QMetaObject::invokeMethod(this, [this, result]() {
                                          this->handleSetThresholdResponse(result);
                                      });
                                  });

    m_dispatcher->registerHandler(ProtocolParser::QT_GET_ALL_THRESHOLDS_RESPONSE,
                                  [this](const ProtocolParser::ParseResult &result) {
                                      QMetaObject::invokeMethod(this, [this, result]() {
                                          this->handleGetAllThresholdsResponse(result);
                                      });
                                  });
    m_dispatcher->registerHandler(ProtocolParser::QT_ALARM_QUERY_RESPONSE,
                                  [this](const ProtocolParser::ParseResult &result) {
                                      QMetaObject::invokeMethod(this, [this, result]() {
                                          this->handleAlarmQueryResponse(result);
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

    // 修复：直接调用widget的公有方法
    QString startDate = m_energyPage->getStartDate().toString("yyyy-MM-dd");
    QString endDate = m_energyPage->getEndDate().toString("yyyy-MM-dd");

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
