#include "placecard.h"
#include <QMouseEvent>
#include <QStyleOption>
#include <QPainter>
#include <QDebug>

PlaceCard::PlaceCard(const QString &placeId, const QString &placeName,
                     const QStringList &equipmentList, QWidget *parent)
    : QWidget(parent)
    , m_placeId(placeId)
    , m_placeName(placeName)
    , m_equipmentList(equipmentList)
    , m_selected(false)
    , m_placeNameLabel(nullptr)
    , m_equipmentLabel(nullptr)
{
    qDebug() << "创建PlaceCard:" << placeName;

    if (placeId.isEmpty() || placeName.isEmpty()) {
        qDebug() << "警告: 创建PlaceCard时placeId或placeName为空";
    }

    setObjectName("placeCard");
    setFixedSize(200, 150);
    setMouseTracking(true);

    try {
        QVBoxLayout *layout = new QVBoxLayout(this);
        layout->setContentsMargins(12, 12, 12, 12);
        layout->setSpacing(8);

        m_placeNameLabel = new QLabel(placeName, this);
        m_placeNameLabel->setStyleSheet(
            "QLabel {"
            "    font-weight: bold;"
            "    font-size: 14px;"
            "    color: #2c3e50;"
            "    text-align: center;"
            "}");

        QString equipmentText;
        if (equipmentList.isEmpty()) {
            equipmentText = "无设备";
        } else {
            equipmentText = equipmentList.join("\n");
        }

        m_equipmentLabel = new QLabel(equipmentText, this);
        m_equipmentLabel->setStyleSheet(
            "QLabel {"
            "    font-size: 11px;"
            "    color: #666;"
            "    text-align: center;"
            "}");
        m_equipmentLabel->setWordWrap(true);

        layout->addWidget(m_placeNameLabel);
        layout->addWidget(m_equipmentLabel);
        layout->addStretch();

        updateCardStyle();

    } catch (const std::exception &e) {
        qDebug() << "PlaceCard构造函数异常:" << e.what();
    } catch (...) {
        qDebug() << "PlaceCard构造函数未知异常";
    }
}

void PlaceCard::setSelected(bool selected)
{
    if (m_selected != selected) {
        m_selected = selected;
        updateCardStyle();
    }
}

void PlaceCard::updateCardStyle()
{
    QString cardStyle = QString(
                            "QWidget#placeCard {"
                            "    background-color: %1;"
                            "    border: 2px solid %2;"
                            "    border-radius: 8px;"
                            "}"
                            ).arg(m_selected ? "#e3f2fd" : "white")
                            .arg(m_selected ? "#4a69bd" : "#e0e0e0");

    setStyleSheet(cardStyle);
}

void PlaceCard::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        emit cardClicked(m_placeId);
        event->accept();
    } else {
        QWidget::mousePressEvent(event);
    }
}

void PlaceCard::enterEvent(QEnterEvent *event)
{
    if (!m_selected) {
        setStyleSheet(
            "QWidget#placeCard {"
            "    background-color: #f8f9fa;"
            "    border: 2px solid #4a69bd;"
            "    border-radius: 8px;"
            "}"
            );
    }
    QWidget::enterEvent(event);
}

void PlaceCard::leaveEvent(QEvent *event)
{
    updateCardStyle();
    QWidget::leaveEvent(event);
}

void PlaceCard::paintEvent(QPaintEvent *event)
{
    QStyleOption opt;
    opt.initFrom(this);
    QPainter p(this);
    style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);

    QWidget::paintEvent(event);
}
