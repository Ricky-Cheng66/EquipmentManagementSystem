#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <QTime>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_tcpClient(new TcpClient(this)) // 初始化客户端
    , m_dispatcher(new MessageDispatcher(m_tcpClient))
{
    ui->setupUi(this);
    // 连接按钮信号到我们的槽函数
    connect(ui->connectButton, &QPushButton::clicked, this, &MainWindow::onConnectButtonClicked);
    connect(ui->sendHeartbeatButton, &QPushButton::clicked, this, &MainWindow::onSendHeartbeatButtonClicked);

    // 连接TcpClient的信号到MainWindow的槽函数
    connect(m_tcpClient, &TcpClient::connected, this, &MainWindow::onClientConnected);
    connect(m_tcpClient, &TcpClient::disconnected, this, &MainWindow::onClientDisconnected);
    //connect(m_tcpClient, &TcpClient::protocolMessageReceived, this, &MainWindow::onProtocolMessageReceived);
    connect(m_tcpClient, &TcpClient::errorOccurred, this, &MainWindow::onClientErrorOccurred);
    // --- 注册消息处理器 ---
    // 示例：注册心跳响应（类型为9）的处理函数
    m_dispatcher->registerHandler(ProtocolParser::HEARTBEAT_RESPONSE,
                                  [this](const ProtocolParser::ParseResult &result) {
                                      // 使用Lambda表达式，调用成员函数，确保在正确的线程上下文
                                      QMetaObject::invokeMethod(this, [this, result]() {
                                          this->handleHeartbeatResponse(result);
                                      });
                                  });
    // 未来可以在这里注册更多： m_dispatcher->registerHandler(ProtocolParser::STATUS_UPDATE, ...);
    logMessage("客户端初始化完成。");
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

void MainWindow::onSendHeartbeatButtonClicked()
{
    // 发送一个测试心跳，设备ID暂时留空或填写测试ID
    if(m_tcpClient->sendProtocolMessage(ProtocolParser::HEARTBEAT, "test_device")) {
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

// 新增：心跳响应的具体处理函数
void MainWindow::handleHeartbeatResponse(const ProtocolParser::ParseResult &result)
{
    QString msg = QString("[业务处理] 心跳响应来自设备: %1").arg(QString::fromStdString(result.equipment_id));
    logMessage(msg);
    // 这里可以更新UI状态，比如让某个设备图标闪烁表示在线
}
