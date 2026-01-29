#include "reservationfiltertoolbar.h"
#include <QLabel>
#include <QDebug>

ReservationFilterToolBar::ReservationFilterToolBar(QWidget *parent)
    : QWidget(parent)
    , m_filterTimer(nullptr)
    , m_isPlaceListMode(false)  // 默认不是场所列表模式
{
    setObjectName("reservationFilterToolBar");

    // 创建主布局
    m_mainLayout = new QHBoxLayout(this);
    m_mainLayout->setContentsMargins(10, 5, 10, 5);
    m_mainLayout->setSpacing(15);

    // 返回按钮（初始隐藏）
    m_backButton = new QPushButton("← 返回场所列表", this);
    m_backButton->setFixedWidth(120);
    m_backButton->setVisible(false);
    m_backButton->setStyleSheet(
        "QPushButton {"
        "    background-color: #95a5a6;"
        "    color: white;"
        "    border: none;"
        "    border-radius: 4px;"
        "    padding: 6px 12px;"
        "}"
        "QPushButton:hover {"
        "    background-color: #7f8c8d;"
        "}"
        );

    // 场所类型筛选（新增）
    QLabel *placeTypeLabel = new QLabel("类型:", this);
    m_placeTypeCombo = new QComboBox(this);
    m_placeTypeCombo->addItem("全部类型", "all");
    m_placeTypeCombo->addItem("教室", "classroom");
    m_placeTypeCombo->addItem("实验室", "lab");
    m_placeTypeCombo->addItem("会议室", "meeting");
    m_placeTypeCombo->addItem("办公室", "office");
    m_placeTypeCombo->addItem("体育馆", "gym");
    m_placeTypeCombo->addItem("图书馆", "library");
    m_placeTypeCombo->addItem("其他", "other");
    m_placeTypeCombo->setFixedWidth(100);
    m_placeTypeCombo->setCurrentIndex(0);

    // 场所筛选
    QLabel *placeLabel = new QLabel("场所:", this);
    m_placeCombo = new QComboBox(this);
    m_placeCombo->addItem("全部场所", "all");
    m_placeCombo->setFixedWidth(150);
    m_placeCombo->setCurrentIndex(0);

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
    m_statusCombo->setCurrentIndex(0);

    // 日期筛选
    QLabel *dateLabel = new QLabel("日期:", this);
    m_dateFilterCombo = new QComboBox(this);
    m_dateFilterCombo->addItem("全部日期", "all");
    m_dateFilterCombo->addItem("今天", "today");
    m_dateFilterCombo->addItem("本周", "week");
    m_dateFilterCombo->addItem("本月", "month");
    m_dateFilterCombo->setFixedWidth(100);
    m_dateFilterCombo->setCurrentIndex(0);

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

    // 添加到布局（注意顺序）
    m_mainLayout->addWidget(m_backButton);
    m_mainLayout->addWidget(placeTypeLabel);
    m_mainLayout->addWidget(m_placeTypeCombo);
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

    // 创建防抖定时器
    m_filterTimer = new QTimer(this);
    m_filterTimer->setSingleShot(true);
    m_filterTimer->setInterval(300);

    // 连接信号
    connect(m_backButton, &QPushButton::clicked, this, &ReservationFilterToolBar::backToPlaceListRequested);
    connect(m_placeTypeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            [this]() { m_filterTimer->start(); });
    connect(m_placeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            [this]() { m_filterTimer->start(); });
    connect(m_statusCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            [this]() { m_filterTimer->start(); });
    connect(m_dateFilterCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            [this]() { m_filterTimer->start(); });
    connect(m_startDateEdit, &QDateEdit::dateChanged,
            [this]() { m_filterTimer->start(); });
    connect(m_endDateEdit, &QDateEdit::dateChanged,
            [this]() { m_filterTimer->start(); });
    connect(m_searchEdit, &QLineEdit::textChanged,
            [this]() { m_filterTimer->start(); });
    connect(m_filterTimer, &QTimer::timeout,
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

QString ReservationFilterToolBar::selectedPlaceType() const
{
    return m_placeTypeCombo->currentData().toString();
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
    Q_UNUSED(statuses);
}

void ReservationFilterToolBar::setPlaceTypes(const QStringList &types)
{
    // 这个方法可以根据需要动态添加类型
    Q_UNUSED(types);
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

void ReservationFilterToolBar::setMode(bool isPlaceListMode, const QString &placeName)
{
    m_isPlaceListMode = isPlaceListMode;

    // 根据模式显示/隐藏控件
    m_backButton->setVisible(!isPlaceListMode);      // 非场所列表模式显示返回按钮

    if (isPlaceListMode) {
        // 场所列表模式：显示场所类型筛选，隐藏其他筛选
        m_placeTypeCombo->setVisible(true);
        m_placeCombo->setVisible(false);
        m_statusCombo->setVisible(false);
        m_dateFilterCombo->setVisible(false);
        m_startDateEdit->setVisible(false);
        m_endDateEdit->setVisible(false);
        m_searchEdit->setPlaceholderText("搜索场所名称");
    } else {
        // 场所详情模式：隐藏场所筛选，显示其他筛选
        m_placeTypeCombo->setVisible(false);
        m_placeCombo->setVisible(false);  // 隐藏场所下拉框
        m_statusCombo->setVisible(true);
        m_dateFilterCombo->setVisible(true);
        m_startDateEdit->setVisible(true);
        m_endDateEdit->setVisible(true);
        m_searchEdit->setPlaceholderText("用途或用户ID");

        // 如果传入了场所名称，可以在界面上显示一个标签来标识当前场所
        // 这需要额外的UI元素，目前我们先隐藏场所筛选即可
    }

    qDebug() << "筛选工具栏模式设置完成:" << (isPlaceListMode ? "场所列表模式" : "场所详情模式");
}
