#include "devicecard.h"
#include <QPainter>
#include <QStyleOption>
#include <QFontDatabase>
#include <QDebug>

DeviceCard::DeviceCard(const QString &deviceId, const QString &type,
                       const QString &location, const QString &status,
                       const QString &power, QWidget *parent)
    : QWidget(parent)
    , m_deviceId(deviceId)
    , m_deviceType(type)
    , m_location(location)
    , m_status(status.trimmed())
    , m_power(power.trimmed())
    , m_selected(false)
{
    setupUI();

    // 固定卡片大小
    setFixedSize(280, 160);

    // 设置鼠标跟踪
    setMouseTracking(true);

    // 初始样式
    updateCardStyle();
}

void DeviceCard::setupUI()
{
    // 设置卡片样式
    setObjectName("deviceCard");

    // 创建主布局
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(12, 12, 12, 12);
    m_mainLayout->setSpacing(8);

    // 创建内容容器
    m_contentWidget = new QWidget(this);
    m_contentWidget->setObjectName("cardContent");
    m_mainLayout->addWidget(m_contentWidget);

    // 内容布局
    QVBoxLayout *contentLayout = new QVBoxLayout(m_contentWidget);
    contentLayout->setContentsMargins(0, 0, 0, 0);
    contentLayout->setSpacing(10);

    // ===== 头部：图标和设备名称 =====
    m_headerLayout = new QHBoxLayout();
    m_headerLayout->setContentsMargins(0, 0, 0, 0);
    m_headerLayout->setSpacing(10);

    // 设备图标
    m_iconLabel = new QLabel(m_contentWidget);
    m_iconLabel->setFixedSize(36, 36);
    m_iconLabel->setAlignment(Qt::AlignCenter);

    // 设置图标（根据设备类型）
    QString iconText = getTypeIcon(m_deviceType);
    m_iconLabel->setText(iconText);
    m_iconLabel->setStyleSheet(
        "QLabel {"
        "    font-family: 'Font Awesome 6 Free';"
        "    font-size: 18px;"
        "    color: #4a69bd;"
        "}"
        );

    // 设备名称和ID
    QVBoxLayout *nameLayout = new QVBoxLayout();
    nameLayout->setContentsMargins(0, 0, 0, 0);
    nameLayout->setSpacing(4);

    m_nameLabel = new QLabel(m_deviceId, m_contentWidget);
    m_nameLabel->setStyleSheet(
        "QLabel {"
        "    font-weight: bold;"
        "    font-size: 14px;"
        "    color: #2c3e50;"
        "}"
        );

    QLabel *typeLabel = new QLabel(m_deviceType, m_contentWidget);
    typeLabel->setStyleSheet(
        "QLabel {"
        "    font-size: 12px;"
        "    color: #7f8c8d;"
        "}"
        );

    nameLayout->addWidget(m_nameLabel);
    nameLayout->addWidget(typeLabel);

    m_headerLayout->addWidget(m_iconLabel);
    m_headerLayout->addLayout(nameLayout);
    m_headerLayout->addStretch();

    contentLayout->addLayout(m_headerLayout);

    // ===== 中部：位置和电源信息 =====
    QWidget *infoWidget = new QWidget(m_contentWidget);
    QHBoxLayout *infoLayout = new QHBoxLayout(infoWidget);
    infoLayout->setContentsMargins(0, 0, 0, 0);
    infoLayout->setSpacing(10);

    // 位置信息
    m_locationLabel = new QLabel("📍 " + m_location, infoWidget);
    m_locationLabel->setStyleSheet(
        "QLabel {"
        "    font-size: 12px;"
        "    color: #666;"
        "    padding: 4px 8px;"
        "    background-color: #f8f9fa;"
        "    border-radius: 4px;"
        "}"
        );

    // 电源状态信息
    m_powerLabel = new QLabel(infoWidget);  // 新增电源状态标签
    updatePowerLabel();

    infoLayout->addWidget(m_locationLabel);
    infoLayout->addWidget(m_powerLabel);
    infoLayout->addStretch();

    contentLayout->addWidget(infoWidget);

    // ===== 底部：状态和控制 =====
    m_footerLayout = new QHBoxLayout();
    m_footerLayout->setContentsMargins(0, 0, 0, 0);
    m_footerLayout->setSpacing(10);

    // 状态指示器和标签
    QWidget *statusContainer = new QWidget(m_contentWidget);
    QHBoxLayout *statusLayout = new QHBoxLayout(statusContainer);
    statusLayout->setContentsMargins(0, 0, 0, 0);
    statusLayout->setSpacing(6);

    // 状态指示灯
    m_statusIndicator = new QWidget(statusContainer);
    m_statusIndicator->setFixedSize(8, 8);

    // 状态标签
    m_statusLabel = new QLabel(statusContainer);
    m_statusLabel->setStyleSheet(
        "QLabel {"
        "    font-size: 12px;"
        "    font-weight: bold;"
        "}"
        );

    statusLayout->addWidget(m_statusIndicator);
    statusLayout->addWidget(m_statusLabel);
    statusLayout->addStretch();

    // 电源按钮
    m_powerButton = new QPushButton(m_contentWidget);
    m_powerButton->setFixedSize(70, 28);
    m_powerButton->setCursor(Qt::PointingHandCursor);

    // 根据当前电源状态设置按钮
    updatePowerButton();

    m_footerLayout->addWidget(statusContainer);
    m_footerLayout->addStretch();
    m_footerLayout->addWidget(m_powerButton);

    contentLayout->addLayout(m_footerLayout);

    // 连接信号
    connect(m_powerButton, &QPushButton::clicked, this, &DeviceCard::onPowerButtonClicked);
}

void DeviceCard::updateCardStyle()
{
    // 根据状态设置指示灯颜色
    QString status = m_status.toLower();
    QString colorStr;

    if (status.contains("online") || status == "在线") {
        colorStr = "#27ae60"; // 绿色
        m_statusLabel->setText("在线");
    } else if (status.contains("offline") || status == "离线") {
        colorStr = "#e74c3c"; // 红色
        m_statusLabel->setText("离线");
    } else if (status.contains("reserved") || status.contains("预约")) {
        colorStr = "#f39c12"; // 橙色
        m_statusLabel->setText("预约中");
    } else {
        colorStr = "#95a5a6"; // 灰色
        m_statusLabel->setText(m_status);
    }

    // 更新指示灯样式
    m_statusIndicator->setStyleSheet(
        QString(
            "QWidget {"
            "    border-radius: 4px;"
            "    background-color: %1;"
            "}"
            ).arg(colorStr)
        );

    // 更新卡片整体样式
    QString cardStyle = QString(
                            "QWidget#deviceCard {"
                            "    background-color: %1;"
                            "    border: 2px solid %2;"
                            "    border-radius: 8px;"
                            "}"
                            "QWidget#cardContent {"
                            "    background-color: transparent;"
                            "}"
                            ).arg(m_selected ? "#e3f2fd" : "white")
                            .arg(m_selected ? "#4a69bd" : "#e0e0e0");

    setStyleSheet(cardStyle);
}

void DeviceCard::updateStatus(const QString &status, const QString &power)
{
    m_status = status.trimmed();
    m_power = power.trimmed();

    // 更新电源状态标签和按钮
    updatePowerLabel();
    updatePowerButton();

    updateCardStyle();
}

void DeviceCard::setSelected(bool selected)
{
    if (m_selected != selected) {
        m_selected = selected;
        updateCardStyle();
        emit selectionChanged(m_selected);
    }
}

void DeviceCard::updatePowerLabel()
{
    QString powerText;
    QString powerStyle;

    if (m_power.toLower().contains("on") || m_power == "开") {
        powerText = "⚡ 电源: 开";
        powerStyle =
            "QLabel {"
            "    font-size: 12px;"
            "    color: #27ae60;"
            "    font-weight: bold;"
            "    padding: 4px 8px;"
            "    background-color: #e8f6f3;"
            "    border: 1px solid #27ae60;"
            "    border-radius: 4px;"
            "}";
    } else {
        powerText = "🔌 电源: 关";
        powerStyle =
            "QLabel {"
            "    font-size: 12px;"
            "    color: #e74c3c;"
            "    font-weight: bold;"
            "    padding: 4px 8px;"
            "    background-color: #fdedec;"
            "    border: 1px solid #e74c3c;"
            "    border-radius: 4px;"
            "}";
    }

    if (m_powerLabel) {
        m_powerLabel->setText(powerText);
        m_powerLabel->setStyleSheet(powerStyle);
    }
}

void DeviceCard::updatePowerButton()
{
    if (m_power.toLower().contains("on") || m_power == "开") {
        m_powerButton->setText("关机");
        m_powerButton->setStyleSheet(
            "QPushButton {"
            "    background-color: #e74c3c;"
            "    color: white;"
            "    border: none;"
            "    border-radius: 4px;"
            "    font-size: 11px;"
            "    font-weight: bold;"
            "}"
            "QPushButton:hover {"
            "    background-color: #c0392b;"
            "}"
            );
    } else {
        m_powerButton->setText("开机");
        m_powerButton->setStyleSheet(
            "QPushButton {"
            "    background-color: #27ae60;"
            "    color: white;"
            "    border: none;"
            "    border-radius: 4px;"
            "    font-size: 11px;"
            "    font-weight: bold;"
            "}"
            "QPushButton:hover {"
            "    background-color: #219653;"
            "}"
            );
    }
}

void DeviceCard::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        emit cardClicked(m_deviceId);
        event->accept();
    } else {
        QWidget::mousePressEvent(event);
    }
}

void DeviceCard::enterEvent(QEnterEvent *event)
{
    // 悬停效果
    if (!m_selected) {
        setStyleSheet(
            "QWidget#deviceCard {"
            "    background-color: #f8f9fa;"
            "    border: 2px solid #4a69bd;"
            "    border-radius: 8px;"
            "}"
            "QWidget#cardContent {"
            "    background-color: transparent;"
            "}"
            );
    }
    QWidget::enterEvent(event);
}

void DeviceCard::leaveEvent(QEvent *event)  // 修改参数类型为 QEvent*
{
    // 恢复样式
    updateCardStyle();
    QWidget::leaveEvent(event);
}

void DeviceCard::paintEvent(QPaintEvent *event)
{
    QStyleOption opt;
    opt.initFrom(this);
    QPainter p(this);
    style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);

    QWidget::paintEvent(event);
}

void DeviceCard::onPowerButtonClicked()
{
    bool turnOn = (m_powerButton->text() == "开机");
    emit powerControlRequested(m_deviceId, turnOn);
}

QString DeviceCard::getTypeIcon(const QString &type) const
{
    QString typeLower = type.toLower();

    if (typeLower.contains("projector") || typeLower.contains("投影")) {
        return "📽️";
    } else if (typeLower.contains("air") || typeLower.contains("空调")) {
        return "❄️";
    } else if (typeLower.contains("camera") || typeLower.contains("摄像")) {
        return "📷";
    } else if (typeLower.contains("light") || typeLower.contains("照明")) {
        return "💡";
    } else if (typeLower.contains("computer") || typeLower.contains("电脑")) {
        return "💻";
    } else if (typeLower.contains("printer") || typeLower.contains("打印")) {
        return "🖨️";
    } else {
        return "⚙️";
    }
}

QColor DeviceCard::getStatusColor(const QString &status) const
{
    QString statusLower = status.toLower();

    if (statusLower.contains("online") || statusLower == "在线") {
        return QColor("#27ae60");
    } else if (statusLower.contains("offline") || statusLower == "离线") {
        return QColor("#e74c3c");
    } else if (statusLower.contains("reserved") || statusLower.contains("预约")) {
        return QColor("#f39c12");
    } else {
        return QColor("#95a5a6");
    }
}
