#include "logindialog.h"
#include <QMessageBox>
#include <QTimer>
#include <QDebug>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>

LoginDialog::LoginDialog(TcpClient* tcpClient, MessageDispatcher* dispatcher, QWidget *parent) :
    QDialog(parent),
    m_tcpClient(tcpClient),
    m_dispatcher(dispatcher),
    m_userRole(""),
    m_userId(0),
    m_usernameEdit(nullptr),
    m_passwordEdit(nullptr),
    m_statusLabel(nullptr),
    m_loginButton(nullptr),
    m_cancelButton(nullptr),
    m_mainLayout(nullptr)
{
    setWindowTitle("校园设备综合管理系统 - 登录");

    // 初始化UI
    setupUI();

    // 注册登录响应处理器
    if (m_dispatcher) {
        m_dispatcher->registerHandler(ProtocolParser::QT_LOGIN_RESPONSE,
                                      [this](const ProtocolParser::ParseResult &result) {
                                          QMetaObject::invokeMethod(this, [this, result]() {
                                              this->handleLoginResponse(result);
                                          });
                                      });
    }
}

LoginDialog::~LoginDialog()
{
    // 注意：布局会自动删除其子控件，所以不需要手动删除控件
}

void LoginDialog::setupUI()
{
    // 设置窗口属性 - 自适应布局
    setMinimumSize(500, 400);  // 设置最小大小
    setMaximumSize(800, 600);  // 设置最大大小
    resize(600, 450);          // 设置初始大小

    // 设置窗口标志，禁用最大化按钮
    setWindowFlags(windowFlags() & ~Qt::WindowMaximizeButtonHint);

    // 创建主布局
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(40, 40, 40, 40);
    m_mainLayout->setSpacing(20);

    // 创建表单容器
    QWidget *formContainer = new QWidget(this);
    QVBoxLayout *formLayout = new QVBoxLayout(formContainer);
    formLayout->setContentsMargins(0, 0, 0, 0);
    formLayout->setSpacing(15);

    // 创建表单
    QFormLayout *inputLayout = new QFormLayout();
    inputLayout->setContentsMargins(0, 0, 0, 0);
    inputLayout->setSpacing(10);
    inputLayout->setLabelAlignment(Qt::AlignRight);

    // 用户名标签和输入框
    QLabel *usernameLabel = new QLabel("用户名:", this);
    usernameLabel->setMinimumWidth(80);
    m_usernameEdit = new QLineEdit(this);
    m_usernameEdit->setMinimumHeight(36);
    m_usernameEdit->setPlaceholderText("请输入用户名");
    inputLayout->addRow(usernameLabel, m_usernameEdit);

    // 密码标签和输入框
    QLabel *passwordLabel = new QLabel("密码:", this);
    passwordLabel->setMinimumWidth(80);
    m_passwordEdit = new QLineEdit(this);
    m_passwordEdit->setMinimumHeight(36);
    m_passwordEdit->setEchoMode(QLineEdit::Password);
    m_passwordEdit->setPlaceholderText("请输入密码");
    inputLayout->addRow(passwordLabel, m_passwordEdit);

    // 状态标签
    m_statusLabel = new QLabel(this);
    m_statusLabel->setAlignment(Qt::AlignCenter);
    m_statusLabel->setStyleSheet("color: #666; font-size: 12px;");
    m_statusLabel->setText("请输入用户名和密码");

    // 按钮布局
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->setContentsMargins(0, 0, 0, 0);
    buttonLayout->setSpacing(15);

    m_loginButton = new QPushButton("登录", this);
    m_loginButton->setMinimumSize(120, 40);
    m_loginButton->setProperty("class", "primary-button");

    m_cancelButton = new QPushButton("取消", this);
    m_cancelButton->setMinimumSize(120, 40);
    m_cancelButton->setProperty("class", "secondary-button");

    buttonLayout->addStretch();
    buttonLayout->addWidget(m_loginButton);
    buttonLayout->addWidget(m_cancelButton);
    buttonLayout->addStretch();

    // 添加控件到表单布局
    formLayout->addLayout(inputLayout);
    formLayout->addWidget(m_statusLabel);
    formLayout->addLayout(buttonLayout);

    // 添加表单容器到主布局
    m_mainLayout->addStretch();
    m_mainLayout->addWidget(formContainer);
    m_mainLayout->addStretch();

    // 设置窗口布局
    setLayout(m_mainLayout);

    // 连接信号槽
    connect(m_loginButton, &QPushButton::clicked, this, &LoginDialog::on_loginButton_clicked);
    connect(m_cancelButton, &QPushButton::clicked, this, &LoginDialog::on_cancelButton_clicked);

    // 设置回车键触发登录
    connect(m_passwordEdit, &QLineEdit::returnPressed, m_loginButton, &QPushButton::click);
}

QString LoginDialog::getUsername() const {
    if (m_usernameEdit) {
        return m_usernameEdit->text().trimmed();
    }
    return "";
}

QString LoginDialog::getPassword() const {
    if (m_passwordEdit) {
        return m_passwordEdit->text();
    }
    return "";
}

void LoginDialog::setStatusMessage(const QString& message, bool isError) {
    if (m_statusLabel) {
        m_statusLabel->setText(message);
        if (isError) {
            m_statusLabel->setStyleSheet("color: red; font-size: 12px;");
        } else {
            m_statusLabel->setStyleSheet("color: blue; font-size: 12px;");
        }
    }
}

void LoginDialog::setLoginButtonEnabled(bool enabled) {
    if (m_loginButton) {
        m_loginButton->setEnabled(enabled);
    }
}

void LoginDialog::setCancelButtonEnabled(bool enabled) {
    if (m_cancelButton) {
        m_cancelButton->setEnabled(enabled);
    }
}

void LoginDialog::on_loginButton_clicked() {
    QString username = getUsername();
    QString password = getPassword();

    if (username.isEmpty() || password.isEmpty()) {
        setStatusMessage("用户名和密码不能为空", true);
        return;
    }

    // 禁用按钮防止重复点击
    setLoginButtonEnabled(false);
    setCancelButtonEnabled(false);
    setStatusMessage("正在连接服务器...", false);

    // 连接服务器
    QString host = "192.168.198.129";
    quint16 port = 9000;

    if (m_tcpClient && m_tcpClient->connectToServer(host, port)) {
        setStatusMessage("服务器连接成功，正在验证...", false);

        // 发送登录消息
        std::vector<char> loginMsg = ProtocolParser::build_qt_login_message(
            ProtocolParser::CLIENT_QT_CLIENT,
            username.toStdString(),
            password.toStdString());

        if (m_tcpClient->sendData(QByteArray(loginMsg.data(), loginMsg.size())) > 0) {
            setStatusMessage("登录请求已发送，请等待...", false);
        } else {
            setStatusMessage("发送登录请求失败", true);
            setLoginButtonEnabled(true);
            setCancelButtonEnabled(true);
        }
    } else {
        setStatusMessage("连接服务器失败", true);
        setLoginButtonEnabled(true);
        setCancelButtonEnabled(true);
    }
}

void LoginDialog::on_cancelButton_clicked() {
    reject();
}

void LoginDialog::handleLoginResponse(const ProtocolParser::ParseResult& result) {
    QString payload = QString::fromStdString(result.payload);
    QStringList parts = payload.split('|');

    if (parts.size() >= 4 && parts[0] == "success") {
        // 登录成功，格式: success|user_id|username|role
        m_userId = parts[1].toInt();
        m_userRole = parts[3];

        setStatusMessage("登录成功！正在进入系统...", false);

        // 延迟500ms后接受对话框
        QTimer::singleShot(500, this, [this]() {
            accept();
        });
    } else {
        // 登录失败
        QString errorMsg = parts.size() >= 2 ? parts[1] : "未知错误";
        setStatusMessage("登录失败: " + errorMsg, true);
        setLoginButtonEnabled(true);
        setCancelButtonEnabled(true);

        // 断开连接
        if (m_tcpClient) {
            m_tcpClient->disconnectFromServer();
        }
    }
}
