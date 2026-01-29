#include "reservationcard.h"
#include <QPainter>
#include <QStyleOption>
#include <QDebug>
#include <QDateTime>

ReservationCard::ReservationCard(const QString &reservationId, const QString &placeId, const QString &placeName,
                                 const QString &userId, const QString &purpose,
                                 const QString &startTime, const QString &endTime,
                                 const QString &status, const QString &equipmentList,
                                 QWidget *parent)
    : QWidget(parent)
    , m_reservationId(reservationId.isEmpty() ? "未知ID" : reservationId)
    , m_placeId(placeId.isEmpty() ? "未知场所ID" : placeId)
    , m_placeName(placeName.isEmpty() ? "未知场所" : placeName)
    , m_userId(userId.isEmpty() ? "未知用户" : userId)
    , m_purpose(purpose.isEmpty() ? "未指定用途" : purpose)
    , m_startTime(startTime.isEmpty() ? QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss") : startTime)
    , m_endTime(endTime.isEmpty() ? QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss") : endTime)
    , m_status(status.trimmed().isEmpty() ? "pending" : status.trimmed())
    , m_equipmentList(equipmentList.isEmpty() ? "无设备" : equipmentList)
    , m_selected(false)
{
    qDebug() << "创建预约卡片: ID=" << m_reservationId
             << "场所ID=" << m_placeId
             << "场所名称=" << m_placeName
             << "状态=" << m_status;

    try {
        setupUI();
        setFixedSize(320, 220);
        setMouseTracking(true);
        updateCardStyle();
    } catch (const std::exception &e) {
        qCritical() << "创建预约卡片时异常:" << e.what();
    } catch (...) {
        qCritical() << "创建预约卡片时未知异常";
    }
}

void ReservationCard::setupUI()
{
    setObjectName("reservationCard");

    // 创建主布局
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(12, 12, 12, 12);
    m_mainLayout->setSpacing(8);

    // 创建内容容器
    m_contentWidget = new QWidget(this);
    m_contentWidget->setObjectName("cardContent");
    m_mainLayout->addWidget(m_contentWidget);

    QVBoxLayout *contentLayout = new QVBoxLayout(m_contentWidget);
    contentLayout->setContentsMargins(0, 0, 0, 0);
    contentLayout->setSpacing(6);

    // ===== 顶部：状态和ID =====
    QHBoxLayout *topLayout = new QHBoxLayout();
    topLayout->setContentsMargins(0, 0, 0, 0);
    topLayout->setSpacing(8);

    // 状态标签
    m_statusLabel = new QLabel(getStatusText(m_status), m_contentWidget);
    m_statusLabel->setAlignment(Qt::AlignCenter);
    m_statusLabel->setFixedSize(60, 24);

    // ID标签
    m_idLabel = new QLabel("预约 #" + m_reservationId, m_contentWidget);
    m_idLabel->setStyleSheet(
        "QLabel {"
        "    font-weight: bold;"
        "    font-size: 13px;"
        "    color: #2c3e50;"
        "}"
        );

    topLayout->addWidget(m_statusLabel);
    topLayout->addWidget(m_idLabel);
    topLayout->addStretch();

    contentLayout->addLayout(topLayout);

    // ===== 场所信息 =====
    m_placeLabel = new QLabel("🏢 " + m_placeName, m_contentWidget);  // 使用场所名称
    m_placeLabel->setStyleSheet(
        "QLabel {"
        "    font-size: 14px;"
        "    font-weight: bold;"
        "    color: #4a69bd;"
        "}"
        );
    contentLayout->addWidget(m_placeLabel);

    // ===== 时间信息 =====
    QString timeText = QString("🕐 %1 - %2").arg(m_startTime, m_endTime);
    m_timeLabel = new QLabel(timeText, m_contentWidget);
    m_timeLabel->setStyleSheet(
        "QLabel {"
        "    font-size: 12px;"
        "    color: #666;"
        "    padding: 4px 8px;"
        "    background-color: #f8f9fa;"
        "    border-radius: 4px;"
        "}"
        );
    contentLayout->addWidget(m_timeLabel);

    // ===== 用途信息 =====
    m_purposeLabel = new QLabel("📝 " + m_purpose, m_contentWidget);
    m_purposeLabel->setStyleSheet(
        "QLabel {"
        "    font-size: 12px;"
        "    color: #333;"
        "    padding: 4px 0;"
        "}"
        );
    contentLayout->addWidget(m_purposeLabel);

    // ===== 用户信息 =====
    m_userLabel = new QLabel("👤 用户: " + m_userId, m_contentWidget);
    m_userLabel->setStyleSheet(
        "QLabel {"
        "    font-size: 11px;"
        "    color: #7f8c8d;"
        "}"
        );
    contentLayout->addWidget(m_userLabel);

    // ===== 设备信息 =====
    if (!m_equipmentList.isEmpty() && m_equipmentList != "无设备") {
        m_equipmentLabel = new QLabel("🔧 " + m_equipmentList, m_contentWidget);
        m_equipmentLabel->setStyleSheet(
            "QLabel {"
            "    font-size: 11px;"
            "    color: #27ae60;"
            "    padding: 3px 6px;"
            "    background-color: #e8f6f3;"
            "    border-radius: 3px;"
            "    margin-top: 2px;"
            "}"
            );
        m_equipmentLabel->setWordWrap(true);
        contentLayout->addWidget(m_equipmentLabel);
    }

    contentLayout->addStretch();

    // ===== 操作按钮（根据状态显示不同按钮）=====
    if (m_status == "pending" || m_status == "待审批") {
        m_actionButton = new QPushButton("审批", m_contentWidget);
        m_actionButton->setFixedSize(80, 26);
        m_actionButton->setStyleSheet(
            "QPushButton {"
            "    background-color: #f39c12;"
            "    color: white;"
            "    border: none;"
            "    border-radius: 4px;"
            "    font-size: 11px;"
            "    font-weight: bold;"
            "}"
            "QPushButton:hover {"
            "    background-color: #e67e22;"
            "}"
            );
        m_actionButton->setProperty("action", "approve");
        connect(m_actionButton, &QPushButton::clicked, this, &ReservationCard::onActionButtonClicked);

        QHBoxLayout *buttonLayout = new QHBoxLayout();
        buttonLayout->addStretch();
        buttonLayout->addWidget(m_actionButton);
        contentLayout->addLayout(buttonLayout);
    }
}


void ReservationCard::updateCardStyle()
{
    QString statusColor = getStatusColor(m_status);
    QString statusText = getStatusText(m_status);

    // 更新状态标签样式
    m_statusLabel->setText(statusText);
    m_statusLabel->setStyleSheet(QString(
                                     "QLabel {"
                                     "    color: white;"
                                     "    background-color: %1;"
                                     "    border-radius: 3px;"
                                     "    font-size: 10px;"
                                     "    font-weight: bold;"
                                     "    padding: 2px 6px;"
                                     "}"
                                     ).arg(statusColor));

    // 更新卡片整体样式
    QString cardStyle = QString(
                            "QWidget#reservationCard {"
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

QDate ReservationCard::getStartDate() const
{
    // 支持多种日期时间格式
    QString dateStr = m_startTime.split(" ")[0]; // 获取日期部分
    QDate date = QDate::fromString(dateStr, "yyyy-MM-dd");
    if (!date.isValid()) {
        date = QDate::fromString(dateStr, "yyyy/MM/dd");
    }
    return date;
}

QDate ReservationCard::getEndDate() const
{
    QString dateStr = m_endTime.split(" ")[0];
    QDate date = QDate::fromString(dateStr, "yyyy-MM-dd");
    if (!date.isValid()) {
        date = QDate::fromString(dateStr, "yyyy/MM/dd");
    }
    return date;
}

void ReservationCard::updateStatus(const QString &status)
{
    m_status = status.trimmed();
    updateCardStyle();
}

void ReservationCard::setSelected(bool selected)
{
    if (m_selected != selected) {
        m_selected = selected;
        updateCardStyle();
    }
}

void ReservationCard::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        emit cardClicked(m_reservationId);
        event->accept();
    } else {
        QWidget::mousePressEvent(event);
    }
}

void ReservationCard::enterEvent(QEnterEvent *event)
{
    if (!m_selected) {
        setStyleSheet(
            "QWidget#reservationCard {"
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

void ReservationCard::leaveEvent(QEvent *event)
{
    updateCardStyle();
    QWidget::leaveEvent(event);
}

void ReservationCard::paintEvent(QPaintEvent *event)
{
    QStyleOption opt;
    opt.initFrom(this);
    QPainter p(this);
    style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);

    QWidget::paintEvent(event);
}

void ReservationCard::onActionButtonClicked()
{
    QString action = m_actionButton->property("action").toString();
    emit statusActionRequested(m_reservationId, action);
}

QString ReservationCard::getStatusColor(const QString &status) const
{
    QString statusLower = status.toLower();

    if (statusLower == "approved" || statusLower == "已批准" || statusLower == "通过") {
        return "#27ae60"; // 绿色
    } else if (statusLower == "rejected" || statusLower == "已拒绝" || statusLower == "拒绝") {
        return "#e74c3c"; // 红色
    } else if (statusLower == "pending" || statusLower == "待审批" || statusLower == "未审批") {
        return "#f39c12"; // 橙色
    } else if (statusLower == "completed" || statusLower == "已完成") {
        return "#3498db"; // 蓝色
    } else if (statusLower == "cancelled" || statusLower == "已取消") {
        return "#95a5a6"; // 灰色
    } else {
        return "#9b59b6"; // 紫色（未知状态）
    }
}

QString ReservationCard::getStatusText(const QString &status) const
{
    QString statusLower = status.toLower();

    if (statusLower == "approved" || statusLower == "已批准" || statusLower == "通过") {
        return "已批准";
    } else if (statusLower == "rejected" || statusLower == "已拒绝" || statusLower == "拒绝") {
        return "已拒绝";
    } else if (statusLower == "pending" || statusLower == "待审批" || statusLower == "未审批") {
        return "待审批";
    } else if (statusLower == "completed" || statusLower == "已完成") {
        return "已完成";
    } else if (statusLower == "cancelled" || statusLower == "已取消") {
        return "已取消";
    } else {
        return status;
    }
}
