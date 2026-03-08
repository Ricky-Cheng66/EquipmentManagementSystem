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
    placeTypeLabel->setObjectName("placeTypeLabel");
    m_placeTypeCombo = new QComboBox(this);
    m_placeTypeCombo->setObjectName("placeTypeCombo");  // 设置对象名称
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

    // ========== 新增：申请人角色筛选 ==========
    m_roleLabel = new QLabel("角色:", this);
    m_roleCombo = new QComboBox(this);
    m_roleCombo->setObjectName("roleCombo");
    m_roleCombo->addItem("全部角色", "all");
    m_roleCombo->addItem("学生申请", "student");
    m_roleCombo->addItem("老师申请", "teacher");
    m_roleCombo->setFixedWidth(100);
    m_roleCombo->setCurrentIndex(0);
    connect(m_roleCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            [this]() { m_filterTimer->start(); });
    // ========================================

    // 场所筛选
    QLabel *placeLabel = new QLabel("场所:", this);
    placeLabel->setObjectName("placeLabel");
    m_placeCombo = new QComboBox(this);
    m_placeCombo->setObjectName("placeCombo");  // 设置对象名称
    m_placeCombo->addItem("全部场所", "all");
    m_placeCombo->setFixedWidth(150);
    m_placeCombo->setCurrentIndex(0);

    // 状态筛选
    QLabel *statusLabel = new QLabel("状态:", this);
    m_statusCombo = new QComboBox(this);
    m_statusCombo->setObjectName("statusCombo");  // 设置对象名称
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
    m_dateFilterCombo->setObjectName("dateFilterCombo");  // 设置对象名称
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
    m_mainLayout->addWidget(m_roleLabel);
    m_mainLayout->addWidget(m_roleCombo);
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

void ReservationFilterToolBar::setStatusComboDefault(const QString &status)
{
    if (m_statusCombo) {
        int index = m_statusCombo->findData(status);
        if (index >= 0) {
            m_statusCombo->setCurrentIndex(index);
        }
    }
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

QString ReservationFilterToolBar::selectedRole() const
{
    return m_roleCombo ? m_roleCombo->currentData().toString() : "all";
}

QString ReservationFilterToolBar::searchText() const
{
    return m_searchEdit->text().trimmed();
}

void ReservationFilterToolBar::setPlaces(const QStringList &places)
{
    // 先保存当前选中的值
    QString currentPlace = m_placeCombo->currentData().toString();

    m_placeCombo->clear();
    m_placeCombo->addItem("全部场所", "all");

    for (const QString &place : places) {
        if (place != "全部场所") {  // 避免重复添加
            m_placeCombo->addItem(place, place);
        }
    }

    // 恢复之前选中的值
    int index = m_placeCombo->findData(currentPlace);
    if (index >= 0) {
        m_placeCombo->setCurrentIndex(index);
    } else {
        m_placeCombo->setCurrentIndex(0);  // 默认选中"全部场所"
    }

    qDebug() << "场所下拉框设置完成，项目数:" << m_placeCombo->count();
}

void ReservationFilterToolBar::setStatuses(const QStringList &statuses)
{
    Q_UNUSED(statuses);
}

void ReservationFilterToolBar::setPlaceTypes(const QStringList &types)
{
    // 这个方法可以根据需要动态添加类型
    // 如果types不为空，可以重新设置下拉框内容
    if (!types.isEmpty()) {
        // 先保存当前选中的值
        QString currentType = m_placeTypeCombo->currentData().toString();

        m_placeTypeCombo->clear();

        // 添加"全部类型"选项
        m_placeTypeCombo->addItem("全部类型", "all");

        // 添加其他类型选项
        for (const QString &type : types) {
            if (type != "全部类型") {  // 避免重复添加
                m_placeTypeCombo->addItem(type, type);
            }
        }

        // 恢复之前选中的值
        int index = m_placeTypeCombo->findData(currentType);
        if (index >= 0) {
            m_placeTypeCombo->setCurrentIndex(index);
        } else {
            m_placeTypeCombo->setCurrentIndex(0);  // 默认选中"全部类型"
        }
    }
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

    m_backButton->setVisible(!isPlaceListMode);

    // 控制角色筛选框显示（仅在详情模式显示）
    if (m_roleLabel && m_roleCombo) {
        m_roleLabel->setVisible(!isPlaceListMode);
        m_roleCombo->setVisible(!isPlaceListMode);
    }

    if (isPlaceListMode) {
        // 场所列表模式：显示所有其他筛选控件
        m_placeTypeCombo->setVisible(true);
        m_placeCombo->setVisible(true);
        m_statusCombo->setVisible(true);
        m_dateFilterCombo->setVisible(true);
        m_startDateEdit->setVisible(true);
        m_endDateEdit->setVisible(true);
        m_searchEdit->setPlaceholderText("搜索场所名称");
    } else {
        // 场所详情模式：隐藏场所下拉框（因为已固定），其他全显示
        m_placeTypeCombo->setVisible(true);
        m_placeCombo->setVisible(false);
        m_statusCombo->setVisible(true);
        m_dateFilterCombo->setVisible(true);
        m_startDateEdit->setVisible(true);
        m_endDateEdit->setVisible(true);
        m_searchEdit->setPlaceholderText("用途或用户ID");

        if (!placeName.isEmpty()) {
            // 可设置标签显示当前场所
        }
    }

    qDebug() << "筛选工具栏模式设置完成:" << (isPlaceListMode ? "场所列表模式" : "场所详情模式");
}

void ReservationFilterToolBar::setTeacherMode()
{
    // 隐藏场所类型下拉框及其标签
    if (m_placeTypeCombo) m_placeTypeCombo->setVisible(false);
    if (QLabel* label = findChild<QLabel*>("placeTypeLabel"))
        label->setVisible(false);

    // 隐藏场所下拉框及其标签
    if (m_placeCombo) m_placeCombo->setVisible(false);
    if (QLabel* label = findChild<QLabel*>("placeLabel"))
        label->setVisible(false);

    // 隐藏角色筛选相关控件（如果有）
    if (m_roleCombo) m_roleCombo->setVisible(false);
    if (m_roleLabel) m_roleLabel->setVisible(false);
}
