#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <QTime>
#include <QMessageBox>
#include <QLabel>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_tcpClient(new TcpClient(this))
    , m_dispatcher(new MessageDispatcher(m_tcpClient, this))
    , m_loginDialog(nullptr)
    , m_isLoggedIn(false)
    , m_currentUsername("")
    , m_equipmentManagerWidget(nullptr)
{
    ui->setupUi(this);
    setWindowTitle("设备管理系统 - 未连接");

    // --- 新增：为centralWidget创建垂直布局 ---
    // 1. 创建一个垂直布局
    QVBoxLayout* mainLayout = new QVBoxLayout(ui->centralwidget);

    // 2. 创建一个容器Widget来放置原来的测试控件，保持它们的位置
    QWidget* testControlsWidget = new QWidget(this);
    // 将原来的测试控件重新设置父对象到这个容器中
    // 注意：需要逐个设置，因为ui文件中的控件父对象原本就是centralWidget

    // 创建一个表单布局用于放置连接相关的控件
    QFormLayout* connectionLayout = new QFormLayout();
    connectionLayout->addRow("主机:", ui->hostLineEdit);
    connectionLayout->addRow("端口:", ui->portSpinBox);

    // 创建一个水平布局用于放置连接按钮和测试按钮
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->addWidget(ui->connectButton);
    buttonLayout->addWidget(ui->sendHeartbeatButton);
    buttonLayout->addStretch(); // 添加弹性空间

    // 将表单布局和按钮布局添加到测试控件容器中
    QVBoxLayout* testWidgetLayout = new QVBoxLayout(testControlsWidget);
    testWidgetLayout->addLayout(connectionLayout);
    testWidgetLayout->addLayout(buttonLayout);
    testWidgetLayout->addWidget(ui->logTextEdit); // 日志框在下方

    // 3. 创建设备管理界面
    m_equipmentManagerWidget = new EquipmentManagerWidget(m_tcpClient, m_dispatcher, this);
    m_equipmentManagerWidget->setVisible(false); // 登录前隐藏

    // 4. 将测试控件区域和设备管理界面添加到主布局中
    mainLayout->addWidget(testControlsWidget);
    mainLayout->addWidget(m_equipmentManagerWidget);

    // 设置布局比例，让设备管理界面占据更多空间
    mainLayout->setStretch(0, 1); // 测试控件区域占1份
    mainLayout->setStretch(1, 3); // 设备管理界面占3份

    // 修改按钮文本和连接
    ui->connectButton->setText("登录");
    disconnect(ui->connectButton, nullptr, nullptr, nullptr); // 断开旧连接
    connect(ui->connectButton, &QPushButton::clicked, this, &MainWindow::onLoginButtonClicked);

    // 禁用发送测试按钮，直到登录成功
    ui->sendHeartbeatButton->setEnabled(false);

    // 在构造函数中一次性创建菜单（初始禁用菜单项）
    QMenuBar* menuBar = this->menuBar();
    QMenu* managementMenu = menuBar->addMenu("管理");

    m_reservationAction = managementMenu->addAction("预约管理");
    m_reservationAction->setEnabled(false); // 初始禁用
    connect(m_reservationAction, &QAction::triggered, this, &MainWindow::showReservationWidget);

    // 连接网络相关信号
    setupConnection();

    // 创建预约管理窗口（初始隐藏）
    m_reservationWidget = new ReservationWidget(this);
    m_reservationWidget->setVisible(false);

    // 连接预约窗口的信号到主窗口的发送槽
    connect(m_reservationWidget, &ReservationWidget::reservationApplyRequested,
            this, &MainWindow::onReservationApplyRequested);
    connect(m_reservationWidget, &ReservationWidget::reservationQueryRequested,
            this, &MainWindow::onReservationQueryRequested);
    connect(m_reservationWidget, &ReservationWidget::reservationApproveRequested,
            this, &MainWindow::onReservationApproveRequested);

    // 注册消息处理器（心跳响应已注册，新增登录响应）
    m_dispatcher->registerHandler(ProtocolParser::QT_LOGIN_RESPONSE,
                                  [this](const ProtocolParser::ParseResult &result) {
                                      QMetaObject::invokeMethod(this, [this, result]() {
                                          this->handleLoginResponse(result);
                                      });
                                  });
    // 注册预约响应处理器
    m_dispatcher->registerHandler(ProtocolParser::RESERVATION_APPLY,
                                  [this](const ProtocolParser::ParseResult &result) {
                                      QMetaObject::invokeMethod(this, [this, result]() {
                                          this->handleReservationApplyResponse(result);
                                      });
                                  });

    m_dispatcher->registerHandler(ProtocolParser::RESERVATION_QUERY,
                                  [this](const ProtocolParser::ParseResult &result) {
                                      QMetaObject::invokeMethod(this, [this, result]() {
                                          this->handleReservationQueryResponse(result);
                                      });
                                  });

    m_dispatcher->registerHandler(ProtocolParser::RESERVATION_APPROVE,
                                  [this](const ProtocolParser::ParseResult &result) {
                                      QMetaObject::invokeMethod(this, [this, result]() {
                                          this->handleReservationApproveResponse(result);
                                      });
                                  });

    logMessage("客户端初始化完成。请点击'登录'按钮开始。");
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::onConnectButtonClicked()
{
    QString host = ui->hostLineEdit->text();
    quint16 port = static_cast<quint16>(ui->portSpinBox->value());

    logMessage(QString("正在尝试连接到 %1:%2 ...").arg(host).arg(port));
    if (m_tcpClient->connectToServer(host, port)) {
        logMessage("连接成功！");
    } else {
        logMessage("连接失败！");
    }
}

void MainWindow::onLoginButtonClicked()
{
    if (m_tcpClient->isConnected() && m_isLoggedIn) {
        // 如果已登录，点击按钮执行注销
        m_tcpClient->disconnectFromServer();
        m_isLoggedIn = false;
        m_currentUsername.clear();
        ui->connectButton->setText("登录");
        logMessage("已注销");
    } else if (m_tcpClient->isConnected() && !m_isLoggedIn) {
        // 已连接但未登录，显示登录对话框
        showLoginDialog();
    } else {
        // 未连接，先建立连接
        QString host = ui->hostLineEdit->text();
        quint16 port = static_cast<quint16>(ui->portSpinBox->value());
        logMessage(QString("正在连接到 %1:%2 ...").arg(host).arg(port));
        m_tcpClient->connectToServer(host, port);
    }
}

void MainWindow::showLoginDialog()
{
    if (m_loginDialog) {
        m_loginDialog->deleteLater();
    }

    m_loginDialog = new LoginDialog(this);
    connect(m_loginDialog, &LoginDialog::loginRequested, this, &MainWindow::onLoginRequested);
    connect(m_loginDialog, &LoginDialog::rejected, this, [this]() {
        logMessage("登录已取消");
        if (!m_isLoggedIn) {
            // 如果取消登录且未认证，可以断开连接
            m_tcpClient->disconnectFromServer();
        }
    });

    m_loginDialog->exec(); // 模态显示

}

void MainWindow::onLoginRequested(const QString &username, const QString &password)
{
    if (!m_tcpClient->isConnected()) {
        if (m_loginDialog) {
            m_loginDialog->setStatusMessage("网络未连接，登录失败", true);
            m_loginDialog->setLoginButtonEnabled(true);
            m_loginDialog->setCancelButtonEnabled(true);
        }
        return;
    }

    // 构建登录消息并发送
    std::vector<char> loginMsg = ProtocolParser::build_qt_login_message(ProtocolParser::CLIENT_QT_CLIENT, username.toStdString(), password.toStdString());
    if (m_tcpClient->sendData(QByteArray(loginMsg.data(), loginMsg.size())) > 0) {
        logMessage(QString("登录请求已发送，用户: %1").arg(username));
        if (m_loginDialog) {
            m_loginDialog->setStatusMessage("验证中...", false);
        }
    } else {
        logMessage("发送登录请求失败");
        if (m_loginDialog) {
            m_loginDialog->setStatusMessage("发送失败，请重试", true);
            m_loginDialog->setLoginButtonEnabled(true);
            m_loginDialog->setCancelButtonEnabled(true);
        }
    }
}

void MainWindow::handleLoginResponse(const ProtocolParser::ParseResult &result)
{
    // 注意：按照新协议格式，result.payload 只包含"success"及后面的字段
    // 原始消息格式：客户端类型|消息类型|设备ID|success|用户名|角色
    // 所以 result.payload = "success|admin|管理员"
     qDebug() << "LOGIN RESPONSE payload:" << QString::fromStdString(result.payload);
    QString payload = QString::fromStdString(result.payload);
    QStringList parts = payload.split('|');

    bool success = false;
    QString message;
    QString username;
    QString role;

    if (parts.size() > 0) {
        success = (parts[0] == "success");

        if (success) {
            // 成功时格式：success|用户名|角色
            if (parts.size() > 1) {
                username = parts[1];
                m_currentUsername = username;
                m_currentUserId = 1;  //后面需要完善
            }
            if (parts.size() > 2) {
                role = parts[2];

            }
            m_userRole = role;  // 确保这行存在
            qDebug() << "DEBUG: 登录成功，role=" << m_userRole;  // 添加调试

            m_isLoggedIn = true;

            logMessage(QString("登录成功！欢迎，%1 (%2)").arg(username).arg(role));
            setWindowTitle(QString("设备管理系统 - 用户: %1").arg(username));

            if (m_loginDialog) {
                m_loginDialog->accept();
                m_loginDialog = nullptr;
            }

            enableMainUI(true);

            // 登录后启用预约管理菜单项
            if (m_reservationAction) {
                m_reservationAction->setEnabled(true);
            }

            ui->connectButton->setText("注销");
            ui->sendHeartbeatButton->setEnabled(true);

        } else {
            // 失败时格式：fail|错误信息
            if (parts.size() > 1) {
                message = parts[1];
            }

            logMessage(QString("登录失败: %1").arg(message));
            if (m_loginDialog) {
                m_loginDialog->setStatusMessage(QString("登录失败: %1").arg(message), true);
                m_loginDialog->setLoginButtonEnabled(true);
                m_loginDialog->setCancelButtonEnabled(true);
            }
            m_isLoggedIn = false;
        }
    }
}

void MainWindow::onSendHeartbeatButtonClicked()
{
    // 发送一个测试心跳，设备ID暂时留空或填写测试ID
    if(m_tcpClient->sendProtocolMessage(ProtocolParser::CLIENT_QT_CLIENT ,ProtocolParser::HEARTBEAT, "test_device")) {
        logMessage("已发送测试心跳消息。");
    } else {
        logMessage("发送心跳失败。");
    }
}

void MainWindow::onClientConnected()
{
    logMessage("[信号] 已连接到服务器。");
    ui->sendHeartbeatButton->setEnabled(true);
    ui->connectButton->setText("断开连接");
    // 切换连接按钮的功能为断开
    disconnect(ui->connectButton, nullptr, this, nullptr);
    connect(ui->connectButton, &QPushButton::clicked, m_tcpClient, &TcpClient::disconnectFromServer);
}

void MainWindow::onClientDisconnected()
{
    logMessage("[信号] 与服务器断开连接。");
    ui->sendHeartbeatButton->setEnabled(false);
    ui->connectButton->setText("连接服务器");
    // 切换断开按钮的功能为连接
    disconnect(ui->connectButton, nullptr, m_tcpClient, nullptr);
    connect(ui->connectButton, &QPushButton::clicked, this, &MainWindow::onConnectButtonClicked);
}

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
    ui->logTextEdit->append(QString("[%1] %2").arg(QTime::currentTime().toString("hh:mm:ss.zzz"), msg));
}

void MainWindow::setupConnection()
{
    connect(m_tcpClient, &TcpClient::connected, this, [this]() {
        logMessage("[网络] 已连接到服务器");
        // 连接成功后自动弹出登录对话框
        if (!m_isLoggedIn && !m_loginDialog) {
            showLoginDialog();
        }
    });

    connect(m_tcpClient, &TcpClient::disconnected, this, [this]() {
        logMessage("[网络] 与服务器断开连接");
        m_isLoggedIn = false;
        enableMainUI(false);
        ui->connectButton->setText("登录");
        ui->sendHeartbeatButton->setEnabled(false);
        setWindowTitle("设备管理系统 - 未连接");
    });

    connect(m_tcpClient, &TcpClient::errorOccurred, this, [this](const QString& errorString) {
        logMessage(QString("[网络错误] %1").arg(errorString));
    });
}



// 新增：心跳响应的具体处理函数
void MainWindow::handleHeartbeatResponse(const ProtocolParser::ParseResult &result)
{
    QString msg = QString("[业务处理] 心跳响应来自设备: %1").arg(QString::fromStdString(result.equipment_id));
    logMessage(msg);
    // 这里可以更新UI状态，比如让某个设备图标闪烁表示在线
}

void MainWindow::enableMainUI(bool enable)
{
    // 控制设备管理界面的显示
    if (m_equipmentManagerWidget) {
        m_equipmentManagerWidget->setVisible(enable);
        if (enable) {
            // 登录成功后，主动请求一次设备列表
            m_equipmentManagerWidget->requestEquipmentList();
            // 可以选择隐藏测试控件区域，释放空间
            // ui->centralWidget->findChild<QWidget*>("testControlsWidget")->setVisible(false);
        }
    }
    // 禁用测试按钮
    ui->sendHeartbeatButton->setEnabled(false);
}

// 发送预约申请
void MainWindow::onReservationApplyRequested(const QString &equipmentId, const QString &purpose,
                                             const QString &startTime, const QString &endTime)
{
    if (!m_tcpClient || !m_tcpClient->isConnected()) {
        QMessageBox::warning(this, "预约失败", "网络未连接");
        return;
    }

    // payload格式: "userId|start_time|end_time|purpose"
    // TODO: 登录成功后需保存当前用户ID到m_currentUserId
    QString payload = QString("%1|%2|%3|%4").arg(m_currentUserId).arg(startTime).arg(endTime).arg(purpose);
qDebug() << "DEBUG: m_currentUserId=" << m_currentUserId << ", payload=" << payload;  // 添加这行
    std::vector<char> msg = ProtocolParser::build_reservation_message(
        ProtocolParser::CLIENT_QT_CLIENT, equipmentId.toStdString(), payload.toStdString());

    m_tcpClient->sendData(QByteArray(msg.data(), msg.size()));
    logMessage(QString("预约申请已发送: 设备[%1]").arg(equipmentId));
}

// 发送预约查询
void MainWindow::onReservationQueryRequested(const QString &equipmentId)
{
    if (!m_tcpClient || !m_tcpClient->isConnected()) {
        QMessageBox::warning(this, "查询失败", "网络未连接");
        return;
    }

    // equipmentId为空表示查询所有设备
    std::string eqId = equipmentId.isEmpty() ? "all" : equipmentId.toStdString();
    std::vector<char> msg = ProtocolParser::build_reservation_query(
        ProtocolParser::CLIENT_QT_CLIENT, eqId);

    m_tcpClient->sendData(QByteArray(msg.data(), msg.size()));
    logMessage(QString("预约查询已发送: 设备[%1]").arg(equipmentId.isEmpty() ? "全部" : equipmentId));
}

// 发送预约审批
void MainWindow::onReservationApproveRequested(int reservationId, bool approve)
{
    if (!m_tcpClient || !m_tcpClient->isConnected()) {
        QMessageBox::warning(this, "审批失败", "网络未连接");
        return;
    }

    // payload格式: "reservation_id|approve|reject"
    QString payload = QString("%1|%2").arg(reservationId).arg(approve ? "approve" : "reject");

    std::vector<char> msg = ProtocolParser::build_reservation_approve(
        ProtocolParser::CLIENT_QT_CLIENT, m_currentUsername.toStdString(), payload.toStdString());

    m_tcpClient->sendData(QByteArray(msg.data(), msg.size()));
    logMessage(QString("预约审批已发送: ID[%1] 操作[%2]").arg(reservationId).arg(approve ? "批准" : "拒绝"));
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

    if (parts.isEmpty() || parts[0] != "success") {
        QString errorMsg = parts.size() >= 2 ? parts[1] : "查询失败";
        QMessageBox::warning(this, "查询失败", errorMsg);
        return;
    }

    // 正确提取 data 部分（去掉 "success|" 前缀）
    QString data = payload.mid(parts[0].length() + 1); // +1 跳过 '|'

    if (m_reservationWidget) {
        // 判断当前在哪个标签页
        int currentTab = m_reservationWidget->m_tabWidget->currentIndex();

        if (currentTab == 1) {  // 查询页
            m_reservationWidget->updateQueryResultTable(data);
        } else if (currentTab == 2 && m_userRole == "admin") {  // 审批页
            m_reservationWidget->loadPendingReservations(data);
        }
    }
}

void MainWindow::handleReservationApproveResponse(const ProtocolParser::ParseResult &result)
{
    QString payload = QString::fromStdString(result.payload);
    QStringList parts = payload.split('|');

    if (parts.size() >= 2 && parts[0] == "success") {
        QMessageBox::information(this, "审批成功", parts[1]);
        logMessage("预约审批操作成功");
        // TODO: 刷新审批页面的表格
    } else {
        QString errorMsg = parts.size() >= 2 ? parts[1] : "未知错误";
        QMessageBox::warning(this, "审批失败", errorMsg);
        logMessage(QString("预约审批失败: %1").arg(errorMsg));
    }
}

void MainWindow::showReservationWidget()
{
    if (!m_isLoggedIn) {
        QMessageBox::warning(this, "提示", "请先登录");
        return;
    }
    // 先设置用户角色（必须在显示窗口前）
    qDebug() << "DEBUG: MainWindow userRole=" << m_userRole << ", userId=" << m_currentUserId;
    m_reservationWidget->setUserRole(m_userRole, QString::number(m_currentUserId));

    // 先获取 model（提到前面，确保两个 if 都能访问）
    QStandardItemModel* model = nullptr;
    if (m_equipmentManagerWidget) {
        model = m_equipmentManagerWidget->m_equipmentModel;  // 直接访问 public 成员
    }

    // 填充申请页的设备下拉框
    if (m_reservationWidget) {
        m_reservationWidget->m_equipmentComboApply->clear();

        if (model) {
            for (int row = 0; row < model->rowCount(); ++row) {
                QString equipmentId = model->item(row, 0)->text();
                QString equipmentType = model->item(row, 1)->text();
                m_reservationWidget->m_equipmentComboApply->addItem(
                    QString("[%1] %2").arg(equipmentType).arg(equipmentId), equipmentId);
            }
        }
    }

    // 填充查询页的设备下拉框
    if (m_reservationWidget) {
        m_reservationWidget->m_equipmentComboQuery->clear();
        m_reservationWidget->m_equipmentComboQuery->addItem("全部设备", "all");

        if (model) {
            for (int row = 0; row < model->rowCount(); ++row) {
                QString equipmentId = model->item(row, 0)->text();
                QString equipmentType = model->item(row, 1)->text();
                m_reservationWidget->m_equipmentComboQuery->addItem(
                    QString("[%1] %2").arg(equipmentType).arg(equipmentId), equipmentId);
            }
        }
    }

    // 加载待审批预约列表（仅管理员）
    if (m_userRole == "admin") {
        std::vector<char> msg = ProtocolParser::build_reservation_query(
            ProtocolParser::CLIENT_QT_CLIENT, "all");
        m_tcpClient->sendData(QByteArray(msg.data(), msg.size()));
        logMessage("加载待审批预约列表...");
    }

    m_reservationWidget->show();
    m_reservationWidget->raise();
    m_reservationWidget->activateWindow();
}
