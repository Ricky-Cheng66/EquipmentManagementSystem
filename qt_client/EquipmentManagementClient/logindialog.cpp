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
    m_mainLayout(nullptr),
    m_backgroundLabel(nullptr),
    m_overlayWidget(nullptr),
    m_formContainer(nullptr)
{
    setWindowTitle("校园设备综合管理系统 - 登录");

    // 修复问题1：设置窗口标志，包含最小化和最大化按钮
    setWindowFlags(Qt::Window | Qt::WindowMinimizeButtonHint |
                   Qt::WindowMaximizeButtonHint | Qt::WindowCloseButtonHint);

    // 修复问题2：根据背景图片尺寸1920×974计算合适窗口大小
    // 保持宽高比 1920:974 ≈ 2:1，设置窗口大小为原图的60%
    int windowWidth = 1152;  // 1920 * 0.6
    int windowHeight = 584;  // 974 * 0.6

    setMinimumSize(windowWidth, windowHeight);
    setMaximumSize(windowWidth * 2, windowHeight * 2); // 允许放大到2倍
    resize(windowWidth, windowHeight);

    // 初始化UI
    setupBackground();
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
    // 布局会自动删除其子控件
}

void LoginDialog::setupBackground()
{
    // 创建背景标签
    m_backgroundLabel = new QLabel(this);
    m_backgroundLabel->setGeometry(0, 0, width(), height());

    // 加载背景图片
    QString bgPath = ":/resources/resources/images/Login_background.jpg";
    QPixmap bgPixmap(bgPath);

    if (bgPixmap.isNull()) {
        qDebug() << "❌ 背景图片加载失败:" << bgPath;
        // 使用纯色背景作为备选
        m_backgroundLabel->setStyleSheet("background-color: #f5f6fa;");
    } else {
        // 设置背景图片并填满窗口，保持宽高比
        bgPixmap = bgPixmap.scaled(size(), Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
        m_backgroundLabel->setPixmap(bgPixmap);
    }
    m_backgroundLabel->setScaledContents(false);
    m_backgroundLabel->setAlignment(Qt::AlignCenter);

    // 创建右侧半透明遮罩（占窗口的35%宽度，更窄一些）
    m_overlayWidget = new QWidget(this);
    int overlayWidth = width() * 0.35;  // 改为35%宽度
    m_overlayWidget->setGeometry(width() - overlayWidth, 0, overlayWidth, height());
    m_overlayWidget->setStyleSheet(
        "QWidget {"
        "    background-color: rgba(255, 255, 255, 0.9);"  // 改为90%不透明度
        "    border-left: 1px solid rgba(0, 0, 0, 0.1);"
        "}"
        );

    // 创建表单容器
    m_formContainer = new QWidget(m_overlayWidget);
}

void LoginDialog::setupUI()
{
    // 表单容器布局
    QVBoxLayout *formLayout = new QVBoxLayout(m_formContainer);
    formLayout->setContentsMargins(30, 40, 30, 40);
    formLayout->setSpacing(20);

    // 添加标题
    QLabel *titleLabel = new QLabel("校园设备综合管理系统", m_formContainer);
    titleLabel->setStyleSheet(
        "QLabel {"
        "    font-size: 24px;"
        "    font-weight: bold;"
        "    color: #2c3e50;"
        "    margin-bottom: 10px;"
        "}"
        );
    titleLabel->setAlignment(Qt::AlignCenter);
    formLayout->addWidget(titleLabel);

    // 添加副标题
    QLabel *subtitleLabel = new QLabel("登录您的账户", m_formContainer);
    subtitleLabel->setStyleSheet(
        "QLabel {"
        "    font-size: 14px;"
        "    color: #7f8c8d;"
        "    margin-bottom: 30px;"
        "}"
        );
    subtitleLabel->setAlignment(Qt::AlignCenter);
    formLayout->addWidget(subtitleLabel);

    // 创建表单输入区域
    QFormLayout *inputLayout = new QFormLayout();
    inputLayout->setContentsMargins(0, 0, 0, 0);
    inputLayout->setSpacing(15);
    inputLayout->setVerticalSpacing(20);

    // 用户名标签和输入框
    QLabel *usernameLabel = new QLabel("用户名:", m_formContainer);
    usernameLabel->setStyleSheet("font-size: 14px; font-weight: bold; color: #2c3e50;");

    m_usernameEdit = new QLineEdit(m_formContainer);
    m_usernameEdit->setMinimumHeight(40);
    m_usernameEdit->setPlaceholderText("请输入用户名");
    m_usernameEdit->setStyleSheet(
        "QLineEdit {"
        "    border: 1px solid #dcdde1;"
        "    border-radius: 6px;"
        "    padding: 8px 12px;"
        "    background-color: white;"
        "    font-size: 14px;"
        "}"
        "QLineEdit:focus {"
        "    border-color: #4a69bd;"
        "    outline: none;"
        "}"
        );
    inputLayout->addRow(usernameLabel, m_usernameEdit);

    // 密码标签和输入框
    QLabel *passwordLabel = new QLabel("密码:", m_formContainer);
    passwordLabel->setStyleSheet("font-size: 14px; font-weight: bold; color: #2c3e50;");

    m_passwordEdit = new QLineEdit(m_formContainer);
    m_passwordEdit->setMinimumHeight(40);
    m_passwordEdit->setEchoMode(QLineEdit::Password);
    m_passwordEdit->setPlaceholderText("请输入密码");
    m_passwordEdit->setStyleSheet(
        "QLineEdit {"
        "    border: 1px solid #dcdde1;"
        "    border-radius: 6px;"
        "    padding: 8px 12px;"
        "    background-color: white;"
        "    font-size: 14px;"
        "}"
        "QLineEdit:focus {"
        "    border-color: #4a69bd;"
        "    outline: none;"
        "}"
        );
    inputLayout->addRow(passwordLabel, m_passwordEdit);

    formLayout->addLayout(inputLayout);

    // 状态标签
    m_statusLabel = new QLabel("请输入用户名和密码", m_formContainer);
    m_statusLabel->setAlignment(Qt::AlignCenter);
    m_statusLabel->setStyleSheet(
        "QLabel {"
        "    color: #666;"
        "    font-size: 12px;"
        "    min-height: 20px;"
        "    margin: 10px 0;"
        "}"
        );
    formLayout->addWidget(m_statusLabel);

    // 按钮布局
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->setContentsMargins(0, 15, 0, 0);
    buttonLayout->setSpacing(12);

    m_loginButton = new QPushButton("登录", m_formContainer);
    m_loginButton->setMinimumSize(120, 42);
    m_loginButton->setStyleSheet(
        "QPushButton {"
        "    background-color: #4a69bd;"
        "    color: white;"
        "    border: none;"
        "    border-radius: 6px;"
        "    font-size: 14px;"
        "    font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "    background-color: #3c5aa6;"
        "}"
        "QPushButton:pressed {"
        "    background-color: #2c4a96;"
        "}"
        "QPushButton:disabled {"
        "    background-color: #c8d6e5;"
        "}"
        );

    m_cancelButton = new QPushButton("取消", m_formContainer);
    m_cancelButton->setMinimumSize(120, 42);
    m_cancelButton->setStyleSheet(
        "QPushButton {"
        "    background-color: #95a5a6;"
        "    color: white;"
        "    border: none;"
        "    border-radius: 6px;"
        "    font-size: 14px;"
        "    font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "    background-color: #7f8c8d;"
        "}"
        );

    buttonLayout->addStretch();
    buttonLayout->addWidget(m_loginButton);
    buttonLayout->addWidget(m_cancelButton);
    buttonLayout->addStretch();

    formLayout->addLayout(buttonLayout);
    formLayout->addStretch();

    // 添加底部版权信息
    QLabel *copyrightLabel = new QLabel("© 2024 校园设备管理中心", m_formContainer);
    copyrightLabel->setStyleSheet("color: #95a5a6; font-size: 11px;");
    copyrightLabel->setAlignment(Qt::AlignCenter);
    formLayout->addWidget(copyrightLabel);

    // 连接信号槽
    connect(m_loginButton, &QPushButton::clicked, this, &LoginDialog::on_loginButton_clicked);
    connect(m_cancelButton, &QPushButton::clicked, this, &LoginDialog::on_cancelButton_clicked);
    connect(m_passwordEdit, &QLineEdit::returnPressed, m_loginButton, &QPushButton::click);

    // 调整表单容器位置
    updateFormPosition();
}

// 更新表单位置（在遮罩内居中）
void LoginDialog::updateFormPosition()
{
    if (!m_formContainer || !m_overlayWidget) return;

    int formWidth = 320; // 表单固定宽度
    int overlayWidth = m_overlayWidget->width();
    int overlayHeight = m_overlayWidget->height();

    // 表单在遮罩内水平居中，垂直方向顶部留一定空间
    int formLeft = (overlayWidth - formWidth) / 2;
    int formTop = 50; // 距离顶部50像素
    int formHeight = overlayHeight - formTop - 20; // 留出底部空间

    m_formContainer->setGeometry(formLeft, formTop, formWidth, formHeight);
}

// 窗口大小改变时重新布局
void LoginDialog::resizeEvent(QResizeEvent *event)
{
    QDialog::resizeEvent(event);

    // 更新背景
    if (m_backgroundLabel) {
        m_backgroundLabel->setGeometry(0, 0, width(), height());
        // 重新缩放背景图片
        QString bgPath = ":/resources/resources/images/Login_background.jpg";
        QPixmap bgPixmap(bgPath);
        if (!bgPixmap.isNull()) {
            // 保持图片宽高比，填满窗口
            QPixmap scaled = bgPixmap.scaled(size(), Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
            m_backgroundLabel->setPixmap(scaled);
            m_backgroundLabel->setAlignment(Qt::AlignCenter);
        }
    }

    // 更新遮罩位置和大小（始终占窗口的35%右侧）
    if (m_overlayWidget) {
        int overlayWidth = width() * 0.35; // 35%宽度
        m_overlayWidget->setGeometry(width() - overlayWidth, 0, overlayWidth, height());
    }

    // 更新表单位置
    updateFormPosition();
}

// 以下原有函数保持不变
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
            m_statusLabel->setStyleSheet("color: #e74c3c; font-size: 12px; font-weight: bold;");
        } else {
            m_statusLabel->setStyleSheet("color: #27ae60; font-size: 12px; font-weight: bold;");
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
