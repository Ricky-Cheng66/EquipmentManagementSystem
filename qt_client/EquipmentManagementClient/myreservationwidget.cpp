#include "myreservationwidget.h"
#include <QMessageBox>
#include <QDebug>

MyReservationWidget::MyReservationWidget(QWidget *parent)
    : QWidget(parent)
    , m_filterBar(nullptr)
    , m_cardContainer(nullptr)
    , m_cardLayout(nullptr)
    , m_refreshTimer(nullptr)
    , m_isRefreshing(false)
{
    setupUI();
}

MyReservationWidget::~MyReservationWidget()
{
    qDeleteAll(m_cards);
    m_cards.clear();
}

void MyReservationWidget::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // 筛选工具栏（复用 ReservationFilterToolBar）
    m_filterBar = new ReservationFilterToolBar(this);
    m_filterBar->setTeacherMode();  // 隐藏场所相关控件，只保留状态和日期搜索
    m_filterBar->setStatusComboDefault("all"); // 默认显示全部状态
    mainLayout->addWidget(m_filterBar);

    // 滚动区域
    QScrollArea *scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);

    m_cardContainer = new QWidget();
    m_cardContainer->setObjectName("myReservationCardContainer");
    m_cardLayout = new QGridLayout(m_cardContainer);
    m_cardLayout->setContentsMargins(20, 20, 20, 20);
    m_cardLayout->setHorizontalSpacing(20);
    m_cardLayout->setVerticalSpacing(20);
    m_cardLayout->setAlignment(Qt::AlignTop);

    scrollArea->setWidget(m_cardContainer);
    mainLayout->addWidget(scrollArea);

    // 刷新定时器
    m_refreshTimer = new QTimer(this);
    m_refreshTimer->setSingleShot(true);
    connect(m_refreshTimer, &QTimer::timeout, this, &MyReservationWidget::refreshView);

    // 连接筛选变化
    connect(m_filterBar, &ReservationFilterToolBar::filterChanged,
            this, &MyReservationWidget::onFilterChanged);
    connect(m_filterBar, &ReservationFilterToolBar::refreshRequested,
            this, &MyReservationWidget::onRefreshRequested);
}

void MyReservationWidget::setUserInfo(const QString &role, const QString &userId)
{
    m_userRole = role;
    m_userId = userId;
}

void MyReservationWidget::setTodayFilter()
{
    if (!m_filterBar) return;
    m_filterBar->setDateFilterCombo("today");  // 设置为“今天”
    m_filterBar->setStatusComboDefault("all");
    onFilterChanged();
}

void MyReservationWidget::setPendingFilter()
{
    if (!m_filterBar) return;
    m_filterBar->setDateFilterCombo("all");      // 确保日期为全部
    m_filterBar->setStatusComboDefault("pending");
    onFilterChanged();
}

void MyReservationWidget::clearFilters()
{
    if (!m_filterBar) return;
    m_filterBar->setDateFilterCombo("all");      // 日期设为“全部日期”
    m_filterBar->setStatusComboDefault("all");   // 状态设为“全部状态”
    onFilterChanged(); // 触发刷新
}

void MyReservationWidget::handleReservationResponse(const QString &data)
{
    // 清空旧卡片
    qDeleteAll(m_cards);
    m_cards.clear();
    m_cardMap.clear();

    if (data.isEmpty() || data.startsWith("fail|") || data == "暂无预约记录") {
        refreshView();
        return;
    }

    QStringList records = data.split(';', Qt::SkipEmptyParts);
    for (const QString &rec : records) {
        QStringList fields = rec.split('|');
        if (fields.size() < 7) continue;

        QString reservationId = fields[0];
        QString placeId = fields[1];
        QString userId = fields[2];
        QString purpose = fields[3];
        QString startTime = fields[4];
        QString endTime = fields[5];
        QString status = fields[6];
        QString role = (fields.size() >= 8) ? fields[7] : "";

        // 创建卡片，父对象设为 m_cardContainer（后续会重新设置父对象）
        ReservationCard *card = new ReservationCard(
            reservationId, placeId, "待获取名称", userId, purpose,
            startTime, endTime, status, "", role, false, this);

        // 连接点击信号
        connect(card, &ReservationCard::cardClicked,
                this, &MyReservationWidget::onCardClicked);

        m_cards.append(card);
        m_cardMap[reservationId] = card;
    }

    refreshView();
}

void MyReservationWidget::refreshView()
{
    m_isRefreshing = true;

    // 清空布局
    QLayoutItem *child;
    while ((child = m_cardLayout->takeAt(0)) != nullptr) {
        if (child->widget()) {
            child->widget()->setParent(nullptr);
        }
        delete child;
    }

    if (m_cards.isEmpty()) {
        QLabel *emptyLabel = new QLabel("暂无预约记录", m_cardContainer);
        emptyLabel->setAlignment(Qt::AlignCenter);
        emptyLabel->setStyleSheet("color: #7f8c8d; font-size: 16px; padding: 60px;");
        m_cardLayout->addWidget(emptyLabel, 0, 0, 1, 1);
        m_isRefreshing = false;
        return;
    }

    // 获取筛选条件
    QString searchText = m_filterBar->searchText();
    QString selectedStatus = m_filterBar->selectedStatus();
    QString selectedDateRange = m_filterBar->selectedDate();
    QDate startDate = m_filterBar->startDate();
    QDate endDate = m_filterBar->endDate();

    // 计算每行卡片数
    int containerWidth = m_cardContainer->width();
    if (containerWidth <= 0) containerWidth = 800;
    int cardsPerRow = qMax(1, containerWidth / 340);

    int row = 0, col = 0, visible = 0;
    QWidget *gridContainer = new QWidget(m_cardContainer);
    QGridLayout *gridLayout = new QGridLayout(gridContainer);
    gridLayout->setContentsMargins(0, 0, 0, 0);
    gridLayout->setHorizontalSpacing(20);
    gridLayout->setVerticalSpacing(20);

    for (ReservationCard *card : m_cards) {
        bool shouldShow = true;

        // 状态筛选
        if (selectedStatus != "all") {
            QString cardStatus = card->status().toLower();
            if (!cardStatus.contains(selectedStatus.toLower()))
                shouldShow = false;
        }

        // 日期范围筛选
        if (shouldShow && selectedDateRange != "all" && startDate.isValid() && endDate.isValid()) {
            QDate cardStart = card->getStartDate();
            if (cardStart < startDate || cardStart > endDate)
                shouldShow = false;
        }

        // 搜索筛选
        if (shouldShow && !searchText.isEmpty()) {
            QString lowerSearch = searchText.toLower();
            QString cardText = card->reservationId() + "|" + card->purpose() + "|" + card->placeName();
            if (!cardText.toLower().contains(lowerSearch))
                shouldShow = false;
        }

        if (shouldShow) {
            card->setParent(gridContainer);
            card->setVisible(true);
            card->setFixedSize(320, 220);
            gridLayout->addWidget(card, row, col);
            visible++;
            col++;
            if (col >= cardsPerRow) {
                col = 0;
                row++;
            }
        }
    }

    if (visible == 0) {
        delete gridContainer;
        QString filterInfo;
        if (!searchText.isEmpty()) filterInfo = QString("搜索: %1").arg(searchText);
        QLabel *noMatchLabel = new QLabel(
            filterInfo.isEmpty() ? "没有符合条件的预约记录" : QString("没有符合条件的预约记录\n筛选条件: %1").arg(filterInfo),
            m_cardContainer);
        noMatchLabel->setAlignment(Qt::AlignCenter);
        noMatchLabel->setStyleSheet("color: #95a5a6; font-size: 15px; padding: 60px;");
        m_cardLayout->addWidget(noMatchLabel, 0, 0, 1, 1);
    } else {
        m_cardLayout->addWidget(gridContainer, 0, 0);
        // 添加统计标签
        QLabel *statsLabel = new QLabel(QString("共 %1 条记录").arg(visible), m_cardContainer);
        statsLabel->setStyleSheet("color: #4a69bd; font-size: 12px; font-weight: bold; padding: 5px;");
        gridLayout->addWidget(statsLabel, row + 1, 0, 1, cardsPerRow, Qt::AlignCenter);
    }

    m_isRefreshing = false;
}

void MyReservationWidget::onFilterChanged()
{
    if (m_refreshTimer->isActive())
        m_refreshTimer->stop();
    m_refreshTimer->start(300);
}

void MyReservationWidget::onRefreshRequested()
{
    emit queryRequested();
}

void MyReservationWidget::onCardClicked(const QString &reservationId)
{
    ReservationCard *card = m_cardMap.value(reservationId);
    if (!card) return;

    // 状态必须为 approved
    if (card->status().toLower() != "approved") {
        QMessageBox::information(this, "提示", "只有已批准的预约才能进行设备控制");
        return;
    }

    // 检查当前时间是否在预约时间段内
    QDateTime now = QDateTime::currentDateTime();
    QDateTime start = QDateTime::fromString(card->startTime(), "yyyy-MM-dd HH:mm:ss");
    QDateTime end = QDateTime::fromString(card->endTime(), "yyyy-MM-dd HH:mm:ss");

    if (!start.isValid() || !end.isValid()) {
        QMessageBox::warning(this, "错误", "预约时间格式错误");
        return;
    }

    if (now < start) {
        QMessageBox::information(this, "提示", "预约尚未开始，请在开始时间后操作");
        return;
    }

    if (now > end) {
        QMessageBox::information(this, "提示", "预约已结束");
        return;
    }

    // 通过，发出信号
    emit equipmentControlRequested(reservationId);
}
