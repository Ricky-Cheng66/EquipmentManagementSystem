#include "reservationfiltertoolbar.h"
#include <QLabel>
#include <QDebug>

ReservationFilterToolBar::ReservationFilterToolBar(QWidget *parent)
    : QWidget(parent)
{
    setObjectName("reservationFilterToolBar");

    // 创建主布局
    m_mainLayout = new QHBoxLayout(this);
    m_mainLayout->setContentsMargins(10, 5, 10, 5);
    m_mainLayout->setSpacing(15);

    // 场所筛选
    QLabel *placeLabel = new QLabel("场所:", this);
    m_placeCombo = new QComboBox(this);
    m_placeCombo->addItem("全部场所", "all");
    m_placeCombo->setFixedWidth(150);

    // 状态筛选
    QLabel *statusLabel = new QLabel("状态:", this);
    m_statusCombo = new QComboBox(this);
    m_statusCombo->addItem("全部状态", "all");
    m_statusCombo->addItem("待审批", "pending");
    m_statusCombo->addItem("已批准", "approved");
    m_statusCombo->addItem("已拒绝", "rejected");
    m_statusCombo->addItem("已完成", "completed");
    m_statusCombo->addItem("已取消", "cancelled");
    m_statusCombo->setFixedWidth(100);

    // 日期筛选
    QLabel *dateLabel = new QLabel("日期:", this);
    m_dateFilterCombo = new QComboBox(this);
    m_dateFilterCombo->addItem("全部日期", "all");
    m_dateFilterCombo->addItem("今天", "today");
    m_dateFilterCombo->addItem("本周", "week");
    m_dateFilterCombo->addItem("本月", "month");
    m_dateFilterCombo->setFixedWidth(100);

    // 日期范围
    QLabel *startLabel = new QLabel("从:", this);
    m_startDateEdit = new QDateEdit(QDate::currentDate(), this);
    m_startDateEdit->setFixedWidth(100);
    m_startDateEdit->setDisplayFormat("yyyy-MM-dd");
    m_startDateEdit->setCalendarPopup(true);

    QLabel *endLabel = new QLabel("到:", this);
    m_endDateEdit = new QDateEdit(QDate::currentDate(), this);
    m_endDateEdit->setFixedWidth(100);
    m_endDateEdit->setDisplayFormat("yyyy-MM-dd");
    m_endDateEdit->setCalendarPopup(true);

    // 搜索框
    QLabel *searchLabel = new QLabel("搜索:", this);
    m_searchEdit = new QLineEdit(this);
    m_searchEdit->setPlaceholderText("用途或用户ID");
    m_searchEdit->setFixedWidth(180);
    m_searchEdit->setClearButtonEnabled(true);

    // 刷新按钮
    m_refreshButton = new QPushButton("🔄 刷新", this);
    m_refreshButton->setFixedWidth(80);

    // 添加到布局
    m_mainLayout->addWidget(placeLabel);
    m_mainLayout->addWidget(m_placeCombo);
    m_mainLayout->addWidget(statusLabel);
    m_mainLayout->addWidget(m_statusCombo);
    m_mainLayout->addWidget(dateLabel);
    m_mainLayout->addWidget(m_dateFilterCombo);
    m_mainLayout->addWidget(startLabel);
    m_mainLayout->addWidget(m_startDateEdit);
    m_mainLayout->addWidget(endLabel);
    m_mainLayout->addWidget(m_endDateEdit);
    m_mainLayout->addWidget(searchLabel);
    m_mainLayout->addWidget(m_searchEdit);
    m_mainLayout->addStretch();
    m_mainLayout->addWidget(m_refreshButton);

    // 设置样式
    setStyleSheet(
        "QWidget#reservationFilterToolBar {"
        "    background-color: #f5f6fa;"
        "    border-bottom: 1px solid #e0e0e0;"
        "}"
        "QLabel {"
        "    color: #666;"
        "    font-weight: bold;"
        "}"
        "QPushButton {"
        "    background-color: #4a69bd;"
        "    color: white;"
        "    border: none;"
        "    border-radius: 4px;"
        "    padding: 6px 12px;"
        "}"
        "QPushButton:hover {"
        "    background-color: #3c5aa6;"
        "}"
        );

    // 连接信号
    connect(m_placeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ReservationFilterToolBar::filterChanged);
    connect(m_statusCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ReservationFilterToolBar::filterChanged);
    connect(m_dateFilterCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ReservationFilterToolBar::filterChanged);
    connect(m_startDateEdit, &QDateEdit::dateChanged,
            this, &ReservationFilterToolBar::filterChanged);
    connect(m_endDateEdit, &QDateEdit::dateChanged,
            this, &ReservationFilterToolBar::filterChanged);
    connect(m_searchEdit, &QLineEdit::textChanged,
            this, &ReservationFilterToolBar::filterChanged);
    connect(m_refreshButton, &QPushButton::clicked,
            this, &ReservationFilterToolBar::refreshRequested);

    // 日期筛选变化时更新日期范围
    connect(m_dateFilterCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            [this](int index) {
                QDate currentDate = QDate::currentDate();
                QString dateRange = m_dateFilterCombo->itemData(index).toString();

                if (dateRange == "today") {
                    m_startDateEdit->setDate(currentDate);
                    m_endDateEdit->setDate(currentDate);
                } else if (dateRange == "week") {
                    m_startDateEdit->setDate(currentDate.addDays(-7));
                    m_endDateEdit->setDate(currentDate);
                } else if (dateRange == "month") {
                    m_startDateEdit->setDate(currentDate.addMonths(-1));
                    m_endDateEdit->setDate(currentDate);
                }
            });
}

QString ReservationFilterToolBar::selectedPlace() const
{
    return m_placeCombo->currentData().toString();
}

QString ReservationFilterToolBar::selectedStatus() const
{
    return m_statusCombo->currentData().toString();
}

QString ReservationFilterToolBar::selectedDate() const
{
    return m_dateFilterCombo->currentData().toString();
}

QString ReservationFilterToolBar::searchText() const
{
    return m_searchEdit->text().trimmed();
}

void ReservationFilterToolBar::setPlaces(const QStringList &places)
{
    m_placeCombo->clear();
    m_placeCombo->addItem("全部场所", "all");

    for (const QString &place : places) {
        m_placeCombo->addItem(place, place);
    }
}

void ReservationFilterToolBar::setStatuses(const QStringList &statuses)
{
    // 这个方法可以根据需要添加更多状态
    Q_UNUSED(statuses);
}

QDate ReservationFilterToolBar::startDate() const
{
    return m_startDateEdit ? m_startDateEdit->date() : QDate();
}

QDate ReservationFilterToolBar::endDate() const
{
    return m_endDateEdit ? m_endDateEdit->date() : QDate();
}

void ReservationFilterToolBar::setDateRange(const QDate &start, const QDate &end)
{
    if (m_startDateEdit) m_startDateEdit->setDate(start);
    if (m_endDateEdit) m_endDateEdit->setDate(end);
}
