#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <QTime>

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

    // 连接网络相关信号
    setupConnection();

    // 注册消息处理器（心跳响应已注册，新增登录响应）
    m_dispatcher->registerHandler(ProtocolParser::QT_LOGIN_RESPONSE,
                                  [this](const ProtocolParser::ParseResult &result) {
                                      QMetaObject::invokeMethod(this, [this, result]() {
                                          this->handleLoginResponse(result);
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
    // 解析响应载荷: "success" 或 "fail|错误信息"
    QString payload = QString::fromStdString(result.payload);
    QStringList parts = payload.split('|');

    bool success = false;
    QString message;

    if (parts.size() > 0) {
        success = (parts[0] == "success");
        if (parts.size() > 1) {
            message = parts[1];
        }
    }

    if (success) {
        m_isLoggedIn = true;
        m_currentUsername = parts.size() > 2 ? parts[2] : "用户"; // 服务器可返回用户名

        logMessage(QString("登录成功！欢迎，%1").arg(m_currentUsername));
        setWindowTitle(QString("设备管理系统 - 用户: %1").arg(m_currentUsername));

        if (m_loginDialog) {
            m_loginDialog->accept(); // 关闭登录对话框
            m_loginDialog = nullptr;
        }

        enableMainUI(true);
        ui->connectButton->setText("注销");
        ui->sendHeartbeatButton->setEnabled(true);

    } else {
        logMessage(QString("登录失败: %1").arg(message));
        if (m_loginDialog) {
            m_loginDialog->setStatusMessage(QString("登录失败: %1").arg(message), true);
            m_loginDialog->setLoginButtonEnabled(true);
            m_loginDialog->setCancelButtonEnabled(true);
        }
        m_isLoggedIn = false;
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
