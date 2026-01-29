#include "placequerycard.h"
#include <QPushButton>
#include <QDebug>

PlaceQueryCard::PlaceQueryCard(const QString &placeId, const QString &placeName,
                               const QStringList &equipmentList, int reservationCount,
                               QWidget *parent)
    : QWidget(parent)
    , m_placeId(placeId)
    , m_placeName(placeName)
    , m_equipmentList(equipmentList)
    , m_reservationCount(reservationCount)
    , m_selected(false)
// 不要在初始化列表中初始化这些指针，它们会在setupUI()中创建
{
    m_placeType = detectPlaceType(placeName);
    setupUI();
    setFixedSize(280, 200);
    setMouseTracking(true);
    updateCardStyle();
}

void PlaceQueryCard::setupUI()
{
    setObjectName("placeQueryCard");

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(16, 16, 16, 16);
    mainLayout->setSpacing(12);

    // 顶部：图标和名称
    QHBoxLayout *topLayout = new QHBoxLayout();
    topLayout->setContentsMargins(0, 0, 0, 0);
    topLayout->setSpacing(12);

    m_iconLabel = new QLabel(getPlaceIcon(m_placeType), this);
    m_iconLabel->setStyleSheet(
        "QLabel {"
        "    font-size: 28px;"
        "}"
        );

    QVBoxLayout *nameLayout = new QVBoxLayout();
    nameLayout->setContentsMargins(0, 0, 0, 0);
    nameLayout->setSpacing(4);

    m_nameLabel = new QLabel(m_placeName, this);
    m_nameLabel->setStyleSheet(
        "QLabel {"
        "    font-weight: bold;"
        "    font-size: 16px;"
        "    color: #2c3e50;"
        "}"
        );

    m_countLabel = new QLabel(QString("📅 预约记录: %1 条").arg(m_reservationCount), this);
    m_countLabel->setStyleSheet(
        "QLabel {"
        "    font-size: 12px;"
        "    color: #666;"
        "}"
        );

    nameLayout->addWidget(m_nameLabel);
    nameLayout->addWidget(m_countLabel);

    topLayout->addWidget(m_iconLabel);
    topLayout->addLayout(nameLayout);
    topLayout->addStretch();

    mainLayout->addLayout(topLayout);

    // 设备列表
    if (!m_equipmentList.isEmpty()) {
        m_equipmentLabel = new QLabel("🔧 设备: " + m_equipmentList.join(", "), this);
        m_equipmentLabel->setStyleSheet(
            "QLabel {"
            "    font-size: 12px;"
            "    color: #7f8c8d;"
            "    padding: 8px;"
            "    background-color: #f8f9fa;"
            "    border-radius: 4px;"
            "}"
            );
        m_equipmentLabel->setWordWrap(true);
        mainLayout->addWidget(m_equipmentLabel);
    }

    mainLayout->addStretch();

    // 快速预约按钮
    m_quickReserveButton = new QPushButton("快速预约", this);
    m_quickReserveButton->setStyleSheet(
        "QPushButton {"
        "    background-color: #4a69bd;"
        "    color: white;"
        "    border: none;"
        "    border-radius: 4px;"
        "    padding: 8px 16px;"
        "    font-size: 12px;"
        "}"
        "QPushButton:hover {"
        "    background-color: #3c5aa6;"
        "}"
        );
    m_quickReserveButton->setCursor(Qt::PointingHandCursor);

    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    buttonLayout->addWidget(m_quickReserveButton);

    mainLayout->addLayout(buttonLayout);

    // 连接信号
    connect(m_quickReserveButton, &QPushButton::clicked, [this]() {
        emit quickReserveRequested(m_placeId);
    });
}

void PlaceQueryCard::setReservationCount(int count)
{
    m_reservationCount = count;
    if (m_countLabel) {
        m_countLabel->setText(QString("📅 预约记录: %1 条").arg(count));
    }
}

void PlaceQueryCard::setSelected(bool selected)
{
    if (m_selected != selected) {
        m_selected = selected;
        updateCardStyle();
    }
}

void PlaceQueryCard::updateCardStyle()
{
    QString cardStyle = QString(
                            "QWidget#placeQueryCard {"
                            "    background-color: %1;"
                            "    border: 2px solid %2;"
                            "    border-radius: 10px;"
                            "}"
                            ).arg(m_selected ? "#e3f2fd" : "white")
                            .arg(m_selected ? "#4a69bd" : "#e0e0e0");

    setStyleSheet(cardStyle);
}

void PlaceQueryCard::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        emit cardClicked(m_placeId);
        event->accept();
    } else {
        QWidget::mousePressEvent(event);
    }
}

void PlaceQueryCard::enterEvent(QEnterEvent *event)
{
    if (!m_selected) {
        setStyleSheet(
            "QWidget#placeQueryCard {"
            "    background-color: #f8f9fa;"
            "    border: 2px solid #4a69bd;"
            "    border-radius: 10px;"
            "}"
            );
    }
    QWidget::enterEvent(event);
}

void PlaceQueryCard::leaveEvent(QEvent *event)
{
    updateCardStyle();
    QWidget::leaveEvent(event);
}

void PlaceQueryCard::paintEvent(QPaintEvent *event)
{
    QStyleOption opt;
    opt.initFrom(this);
    QPainter p(this);
    style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
    QWidget::paintEvent(event);
}

QString PlaceQueryCard::detectPlaceType(const QString &placeName)
{
    QString nameLower = placeName.toLower();

    if (nameLower.contains("教室") || nameLower.contains("classroom")) {
        return "classroom";
    } else if (nameLower.contains("实验室") || nameLower.contains("lab")) {
        return "lab";
    } else if (nameLower.contains("会议室") || nameLower.contains("meeting")) {
        return "meeting";
    } else if (nameLower.contains("办公室") || nameLower.contains("office")) {
        return "office";
    } else if (nameLower.contains("体育馆") || nameLower.contains("gym")) {
        return "gym";
    } else if (nameLower.contains("图书馆") || nameLower.contains("library")) {
        return "library";
    } else {
        return "other";
    }
}

QString PlaceQueryCard::getPlaceIcon(const QString &placeType) const
{
    if (placeType == "classroom") return "🏫";
    else if (placeType == "lab") return "🔬";
    else if (placeType == "meeting") return "💼";
    else if (placeType == "office") return "🏢";
    else if (placeType == "gym") return "🏸";
    else if (placeType == "library") return "📚";
    else return "📍";
}
