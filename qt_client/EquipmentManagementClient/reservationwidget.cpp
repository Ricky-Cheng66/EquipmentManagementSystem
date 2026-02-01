#include "reservationwidget.h"
#include "reservationcard.h"
#include "reservationfiltertoolbar.h"
#include "placecard.h"
#include "placequerycard.h"

#include <QMessageBox>
#include <QHeaderView>
#include <QLabel>
#include <QTimer>
#include <QDebug>
#include <QGridLayout>
#include <QElapsedTimer>
#include <QScrollArea>
#include <QPushButton>
#include <QGroupBox>
#include <QDateEdit>
#include <QTimeEdit>
#include <QLineEdit>
#include <QCheckBox>
#include <QTableWidget>
#include <QStackedWidget>

ReservationWidget::ReservationWidget(QWidget *parent)
    : QWidget(parent), m_tabWidget(new QTabWidget(this))
    , m_placeComboApply(nullptr)
    , m_placeComboQuery(nullptr)
    , m_placeCardsContainer(nullptr)
    , m_placeCardsLayout(nullptr)
    , m_selectedPlaceId("")
    , m_selectedEquipmentText(nullptr)
    , m_approveFilterBar(nullptr)
    , m_approveCardContainer(nullptr)
    , m_approveCardLayout(nullptr)
    , m_selectAllCheck(nullptr)
    , m_batchApproveButton(nullptr)
    , m_batchRejectButton(nullptr)
    , m_isRefreshingQueryView(false)
    , m_currentPlaceId("")
    , m_currentPlaceName("")
    , m_queryFilterBarDetail(nullptr)
    , m_placeDetailNameLabel(nullptr)
    , m_placeDetailStatsLabel(nullptr)
    , m_placeListPage(nullptr)
    , m_placeDetailPage(nullptr)
    , m_placeListLayout(nullptr)
    , m_placeDetailLayout(nullptr)
{
    qDebug() << "ReservationWidget 构造函数开始";

    setWindowTitle("预约管理");
    resize(800, 600);

    // 创建三个标签页
    setupApplyTab();
    setupQueryTab();
    setupApproveTab();

    // 主布局
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(m_tabWidget);
    setLayout(mainLayout);

    connect(m_tabWidget, &QTabWidget::currentChanged, this, &ReservationWidget::onTabChanged);

    // 创建场所列表刷新定时器
    m_placeListRefreshTimer = new QTimer(this);
    m_placeListRefreshTimer->setSingleShot(true);
    connect(m_placeListRefreshTimer, &QTimer::timeout, this, &ReservationWidget::refreshPlaceListView);

    // 初始化当前场所ID和名称
    m_currentPlaceId = "";
    m_currentPlaceName = "";

    qDebug() << "ReservationWidget 构造函数完成";
}

ReservationWidget::~ReservationWidget()
{
    qDebug() << "ReservationWidget destructor called";

    // 停止所有定时器
    if (m_placeListRefreshTimer) {
        m_placeListRefreshTimer->stop();
        delete m_placeListRefreshTimer;
        m_placeListRefreshTimer = nullptr;
    }

    // 清理申请页的场所卡片
    for (PlaceCard *card : m_placeCards.values()) {
        if (card) {
            card->disconnect();
            card->deleteLater();
        }
    }
    m_placeCards.clear();

    // 清理查询页的预约卡片
    qDeleteAll(m_queryCards);
    m_queryCards.clear();
    m_queryCardMap.clear();

    // 清理审批页的预约卡片
    qDeleteAll(m_approveCards);
    m_approveCards.clear();
    m_approveCardMap.clear();

    // 清理场所查询卡片
    qDeleteAll(m_placeQueryCards);
    m_placeQueryCards.clear();

    qDebug() << "ReservationWidget destructor completed";
}

void ReservationWidget::setupApplyTab()
{
    qDebug() << "Setting up apply tab";

    QWidget *applyTab = new QWidget(this);
    QVBoxLayout *mainLayout = new QVBoxLayout(applyTab);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->setSpacing(20);

    // ===== 场所选择卡片网格 =====
    QLabel *placeSelectionLabel = new QLabel("选择场所:", applyTab);
    placeSelectionLabel->setStyleSheet("font-weight: bold; font-size: 14px;");
    mainLayout->addWidget(placeSelectionLabel);

    // 创建场所卡片容器
    m_placeCardsContainer = new QWidget(applyTab);
    m_placeCardsLayout = new QGridLayout(m_placeCardsContainer);
    m_placeCardsLayout->setContentsMargins(0, 0, 0, 0);
    m_placeCardsLayout->setHorizontalSpacing(15);
    m_placeCardsLayout->setVerticalSpacing(15);

    // 创建滚动区域
    QScrollArea *placeScrollArea = new QScrollArea(applyTab);
    placeScrollArea->setWidgetResizable(true);
    placeScrollArea->setFrameShape(QFrame::NoFrame);
    placeScrollArea->setWidget(m_placeCardsContainer);
    placeScrollArea->setMinimumHeight(200);

    mainLayout->addWidget(placeScrollArea);

    // ===== 选中场所的设备列表 =====
    QLabel *selectedPlaceLabel = new QLabel("已选场所设备列表:", applyTab);
    selectedPlaceLabel->setStyleSheet("font-weight: bold; font-size: 14px;");
    mainLayout->addWidget(selectedPlaceLabel);

    m_selectedEquipmentText = new QTextEdit(applyTab);
    m_selectedEquipmentText->setReadOnly(true);
    m_selectedEquipmentText->setMinimumHeight(100);
    m_selectedEquipmentText->setPlaceholderText("请先选择一个场所");
    m_selectedEquipmentText->setStyleSheet(
        "QTextEdit {"
        "    border: 1px solid #dcdde1;"
        "    border-radius: 3px;"
        "    background-color: #f8f9fa;"
        "    padding: 8px;"
        "    font-size: 12px;"
        "}");
    mainLayout->addWidget(m_selectedEquipmentText);

    // ===== 创建一个隐藏的下拉框用于存储场所数据 =====
    m_placeComboApply = new QComboBox(applyTab);
    m_placeComboApply->setVisible(false);  // 隐藏，仅用于存储数据

    // ===== 时间选择部分 =====
    QFormLayout *timeLayout = new QFormLayout();
    timeLayout->setSpacing(12);

    // 开始时间
    QLabel *startLabel = new QLabel("开始时间:", applyTab);
    QWidget *startTimeWidget = new QWidget(applyTab);
    QHBoxLayout *startTimeLayout = new QHBoxLayout(startTimeWidget);
    startTimeLayout->setContentsMargins(0, 0, 0, 0);
    startTimeLayout->setSpacing(8);

    m_startDateEdit = new QDateEdit(QDate::currentDate(), applyTab);
    m_startDateEdit->setProperty("class", "form-control");
    m_startDateEdit->setMinimumHeight(36);
    m_startDateEdit->setDisplayFormat("yyyy-MM-dd");
    m_startDateEdit->setCalendarPopup(true);
    m_startDateEdit->setDate(QDate::currentDate());
    m_startDateEdit->setMinimumDate(QDate::currentDate());

    m_startTimeEdit = new QTimeEdit(applyTab);
    m_startTimeEdit->setProperty("class", "form-control");
    m_startTimeEdit->setMinimumHeight(36);
    m_startTimeEdit->setDisplayFormat("HH:mm");
    m_startTimeEdit->setTime(QTime(QTime::currentTime().hour(), 0, 0));

    startTimeLayout->addWidget(m_startDateEdit);
    startTimeLayout->addWidget(new QLabel(" ", applyTab));
    startTimeLayout->addWidget(m_startTimeEdit);

    // 结束时间
    QLabel *endLabel = new QLabel("结束时间:", applyTab);
    QWidget *endTimeWidget = new QWidget(applyTab);
    QHBoxLayout *endTimeLayout = new QHBoxLayout(endTimeWidget);
    endTimeLayout->setContentsMargins(0, 0, 0, 0);
    endTimeLayout->setSpacing(8);

    m_endDateEdit = new QDateEdit(QDate::currentDate(), applyTab);
    m_endDateEdit->setProperty("class", "form-control");
    m_endDateEdit->setMinimumHeight(36);
    m_endDateEdit->setDisplayFormat("yyyy-MM-dd");
    m_endDateEdit->setCalendarPopup(true);
    m_endDateEdit->setDate(QDate::currentDate());
    m_endDateEdit->setMinimumDate(QDate::currentDate());

    m_endTimeEdit = new QTimeEdit(applyTab);
    m_endTimeEdit->setProperty("class", "form-control");
    m_endTimeEdit->setMinimumHeight(36);
    m_endTimeEdit->setDisplayFormat("HH:mm");
    m_endTimeEdit->setTime(QTime(QTime::currentTime().hour() + 1, 0, 0));

    endTimeLayout->addWidget(m_endDateEdit);
    endTimeLayout->addWidget(new QLabel(" ", applyTab));
    endTimeLayout->addWidget(m_endTimeEdit);

    timeLayout->addRow(startLabel, startTimeWidget);
    timeLayout->addRow(endLabel, endTimeWidget);

    mainLayout->addLayout(timeLayout);

    // ===== 用途输入 =====
    QLabel *purposeLabel = new QLabel("用途:", applyTab);
    purposeLabel->setStyleSheet("font-weight: bold; font-size: 14px;");
    mainLayout->addWidget(purposeLabel);

    m_purposeEdit = new QLineEdit(applyTab);
    m_purposeEdit->setProperty("class", "form-control");
    m_purposeEdit->setMinimumHeight(40);
    m_purposeEdit->setPlaceholderText("请输入预约用途");
    mainLayout->addWidget(m_purposeEdit);

    // ===== 提交按钮 =====
    m_applyButton = new QPushButton("✓ 提交预约", applyTab);
    m_applyButton->setProperty("class", "primary-button");
    m_applyButton->setMinimumHeight(40);
    m_applyButton->setMinimumWidth(120);

    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    buttonLayout->addWidget(m_applyButton);
    buttonLayout->addStretch();

    mainLayout->addLayout(buttonLayout);
    mainLayout->addStretch();

    m_tabWidget->addTab(applyTab, "预约申请");

    connect(m_applyButton, &QPushButton::clicked, this, &ReservationWidget::onApplyButtonClicked);

    qDebug() << "Apply tab setup finished";
    qDebug() << "m_placeComboApply created:" << (m_placeComboApply != nullptr);
}


void ReservationWidget::setupQueryTab()
{
    qDebug() << "Setting up query tab with two-level navigation";

    QWidget *queryTab = new QWidget(this);
    QVBoxLayout *mainLayout = new QVBoxLayout(queryTab);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // 创建查询视图堆栈
    m_queryViewStack = new QStackedWidget(queryTab);

    // ==== 第一级：场所列表页面 ====
    setupPlaceListPage();

    // ==== 第二级：场所详情页面 ====
    setupPlaceDetailPage();

    // 默认显示场所列表页面
    m_queryViewStack->setCurrentIndex(0);

    // 添加到主布局
    mainLayout->addWidget(m_queryViewStack);

    m_tabWidget->addTab(queryTab, "预约查询");

    qDebug() << "Query tab setup finished with two-level navigation";
}

void ReservationWidget::setupApproveTab()
{
    qDebug() << "Setting up approve tab";

    QWidget *approveTab = new QWidget(this);
    QVBoxLayout *mainLayout = new QVBoxLayout(approveTab);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // ===== 筛选工具栏 =====
    m_approveFilterBar = new ReservationFilterToolBar(approveTab);
    mainLayout->addWidget(m_approveFilterBar);

    // ===== 创建卡片容器 =====
    m_approveCardContainer = new QWidget(approveTab);
    m_approveCardContainer->setObjectName("approveCardContainer");
    m_approveCardLayout = new QVBoxLayout(m_approveCardContainer);
    m_approveCardLayout->setContentsMargins(20, 20, 20, 20);
    m_approveCardLayout->setSpacing(20);
    m_approveCardLayout->addStretch();

    // 创建滚动区域
    QScrollArea *approveScrollArea = new QScrollArea(approveTab);
    approveScrollArea->setWidgetResizable(true);
    approveScrollArea->setFrameShape(QFrame::NoFrame);
    approveScrollArea->setWidget(m_approveCardContainer);

    mainLayout->addWidget(approveScrollArea);

    // ===== 批量操作按钮 =====
    QWidget *batchWidget = new QWidget(approveTab);
    batchWidget->setStyleSheet("background-color: #f5f6fa; border-top: 1px solid #e0e0e0;");
    QHBoxLayout *batchLayout = new QHBoxLayout(batchWidget);
    batchLayout->setContentsMargins(10, 10, 10, 10);

    m_selectAllCheck = new QCheckBox("全选", batchWidget);
    m_batchApproveButton = new QPushButton("批量批准", batchWidget);
    m_batchApproveButton->setStyleSheet(
        "QPushButton {"
        "    background-color: #27ae60;"
        "    color: white;"
        "    border: none;"
        "    border-radius: 3px;"
        "    padding: 6px 12px;"
        "    font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "    background-color: #219653;"
        "}"
        "QPushButton:disabled {"
        "    background-color: #c8d6e5;"
        "}");
    m_batchApproveButton->setEnabled(false);

    m_batchRejectButton = new QPushButton("批量拒绝", batchWidget);
    m_batchRejectButton->setStyleSheet(
        "QPushButton {"
        "    background-color: #e74c3c;"
        "    color: white;"
        "    border: none;"
        "    border-radius: 3px;"
        "    padding: 6px 12px;"
        "    font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "    background-color: #c0392b;"
        "}"
        "QPushButton:disabled {"
        "    background-color: #c8d6e5;"
        "}");
    m_batchRejectButton->setEnabled(false);

    batchLayout->addWidget(m_selectAllCheck);
    batchLayout->addStretch();
    batchLayout->addWidget(m_batchApproveButton);
    batchLayout->addWidget(m_batchRejectButton);

    mainLayout->addWidget(batchWidget);

    m_tabWidget->addTab(approveTab, "预约审批");

    // 连接信号
    connect(m_selectAllCheck, &QCheckBox::stateChanged,
            this, &ReservationWidget::onSelectAllChanged);
    connect(m_batchApproveButton, &QPushButton::clicked,
            this, &ReservationWidget::onBatchApprove);
    connect(m_batchRejectButton, &QPushButton::clicked,
            this, &ReservationWidget::onBatchReject);
    connect(m_approveFilterBar, &ReservationFilterToolBar::filterChanged,
            this, &ReservationWidget::onApproveFilterChanged);
    connect(m_approveFilterBar, &ReservationFilterToolBar::refreshRequested,
            this, &ReservationWidget::onApproveRefreshRequested);

    qDebug() << "Approve tab setup finished";
}

void ReservationWidget::setupPlaceListPage()
{
    qDebug() << "设置场所列表页面";

    m_placeListPage = new QWidget(m_queryViewStack);
    QVBoxLayout *mainLayout = new QVBoxLayout(m_placeListPage);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // 创建筛选工具栏（场所列表模式）
    m_queryFilterBar = new ReservationFilterToolBar(m_placeListPage);
    m_queryFilterBar->setMode(true, ""); // 设置为场所列表模式
    mainLayout->addWidget(m_queryFilterBar);

    // 创建滚动区域
    QScrollArea *placeScrollArea = new QScrollArea(m_placeListPage);
    placeScrollArea->setWidgetResizable(true);
    placeScrollArea->setFrameShape(QFrame::NoFrame);

    // 创建场所列表容器
    QWidget *placeListContainer = new QWidget();
    placeListContainer->setObjectName("placeListContainer");
    m_placeListLayout = new QGridLayout(placeListContainer);
    m_placeListLayout->setContentsMargins(20, 20, 20, 20);
    m_placeListLayout->setHorizontalSpacing(20);
    m_placeListLayout->setVerticalSpacing(20);
    m_placeListLayout->setAlignment(Qt::AlignTop);

    placeScrollArea->setWidget(placeListContainer);
    mainLayout->addWidget(placeScrollArea);

    m_queryViewStack->addWidget(m_placeListPage);

    // 连接信号
    connect(m_queryFilterBar, &ReservationFilterToolBar::filterChanged,
            this, &ReservationWidget::onFilterChanged);
    connect(m_queryFilterBar, &ReservationFilterToolBar::refreshRequested,
            this, &ReservationWidget::onRefreshQueryRequested);
}

// 新增：设置场所详情页面
void ReservationWidget::setupPlaceDetailPage()
{
    m_placeDetailPage = new QWidget(m_queryViewStack);
    m_placeDetailLayout = new QVBoxLayout(m_placeDetailPage);
    m_placeDetailLayout->setContentsMargins(0, 0, 0, 0);
    m_placeDetailLayout->setSpacing(0);

    // 创建筛选工具栏（预约记录模式）
    m_queryFilterBarDetail = new ReservationFilterToolBar(m_placeDetailPage);
    m_queryFilterBarDetail->setMode(false); // 设置为场所详情模式
    m_placeDetailLayout->addWidget(m_queryFilterBarDetail);

    // 场所信息概览区域
    QWidget *placeOverviewWidget = new QWidget(m_placeDetailPage);
    placeOverviewWidget->setObjectName("placeOverviewWidget");
    placeOverviewWidget->setStyleSheet(
        "QWidget#placeOverviewWidget {"
        "    background-color: white;"
        "    border-bottom: 1px solid #e0e0e0;"
        "    padding: 15px;"
        "}"
        );
    QVBoxLayout *overviewLayout = new QVBoxLayout(placeOverviewWidget);
    overviewLayout->setContentsMargins(10, 5, 10, 5);
    overviewLayout->setSpacing(5);

    // 场所名称标签
    m_placeDetailNameLabel = new QLabel("", placeOverviewWidget);
    m_placeDetailNameLabel->setStyleSheet(
        "QLabel {"
        "    font-size: 18px;"
        "    font-weight: bold;"
        "    color: #2c3e50;"
        "}"
        );

    // 场所统计信息标签
    m_placeDetailStatsLabel = new QLabel("", placeOverviewWidget);
    m_placeDetailStatsLabel->setStyleSheet(
        "QLabel {"
        "    font-size: 13px;"
        "    color: #666;"
        "    padding: 2px 0;"
        "}"
        );

    overviewLayout->addWidget(m_placeDetailNameLabel);
    overviewLayout->addWidget(m_placeDetailStatsLabel);

    m_placeDetailLayout->addWidget(placeOverviewWidget);

    // 创建预约记录滚动区域
    QScrollArea *reservationScrollArea = new QScrollArea(m_placeDetailPage);
    reservationScrollArea->setWidgetResizable(true);
    reservationScrollArea->setFrameShape(QFrame::NoFrame);

    // 创建预约记录容器
    m_queryCardContainer = new QWidget();
    m_queryCardContainer->setObjectName("queryCardContainer");
    m_queryCardLayout = new QVBoxLayout(m_queryCardContainer);
    m_queryCardLayout->setContentsMargins(20, 20, 20, 20);
    m_queryCardLayout->setSpacing(20);
    m_queryCardLayout->addStretch();

    reservationScrollArea->setWidget(m_queryCardContainer);
    m_placeDetailLayout->addWidget(reservationScrollArea);

    m_queryViewStack->addWidget(m_placeDetailPage);

    // 连接信号
    connect(m_queryFilterBarDetail, &ReservationFilterToolBar::filterChanged,
            this, &ReservationWidget::onFilterChanged);
    connect(m_queryFilterBarDetail, &ReservationFilterToolBar::refreshRequested,
            this, &ReservationWidget::onRefreshQueryRequested);
    connect(m_queryFilterBarDetail, &ReservationFilterToolBar::backToPlaceListRequested,
            this, &ReservationWidget::onBackToPlaceList);
}

// 新增：刷新场所列表视图
void ReservationWidget::refreshPlaceListView()
{
    qDebug() << "刷新场所列表视图";

    if (!m_placeListLayout || !m_placeListPage) {
        qDebug() << "错误: 场所列表布局或页面为空";
        return;
    }

    // 1. 完全清空现有布局，并删除所有子控件
    try {
        // 首先删除所有场所卡片对象
        for (PlaceQueryCard *card : m_placeQueryCards) {
            if (card) {
                card->disconnect();  // 断开所有连接
                card->deleteLater();
            }
        }
        m_placeQueryCards.clear();

        // 从布局中移除并删除所有控件（包括统计标签）
        QLayoutItem *child;
        while ((child = m_placeListLayout->takeAt(0)) != nullptr) {
            if (child->widget()) {
                QWidget *widget = child->widget();
                widget->hide();
                widget->setParent(nullptr);

                // 只删除统计标签，保留空状态标签（如果需要）
                if (widget->objectName() != "emptyPlaceLabel") {
                    widget->deleteLater();
                }
            }
            delete child;
        }
    } catch (...) {
        qWarning() << "清空场所列表布局时异常";
    }

    // 2. 如果没有场所数据，显示提示
    if (m_placeReservationCount.isEmpty()) {
        QLabel *emptyLabel = new QLabel("📭 暂无场所信息\n请先查询预约记录以加载场所数据", m_placeListPage);
        emptyLabel->setObjectName("emptyPlaceLabel");
        emptyLabel->setAlignment(Qt::AlignCenter);
        emptyLabel->setStyleSheet(
            "QLabel {"
            "    color: #7f8c8d;"
            "    font-size: 16px;"
            "    padding: 60px;"
            "    background-color: #f8f9fa;"
            "    border-radius: 8px;"
            "}"
            );
        m_placeListLayout->addWidget(emptyLabel, 0, 0, 1, 1);
        return;
    }

    // 3. 获取筛选条件
    QString selectedPlaceType = m_queryFilterBar ? m_queryFilterBar->selectedPlaceType() : "all";
    QString searchText = m_queryFilterBar ? m_queryFilterBar->searchText() : "";

    // 4. 计算每行卡片数量
    int containerWidth = m_placeListPage->width();
    if (containerWidth <= 0) containerWidth = 800;
    int cardsPerRow = qMax(1, containerWidth / 300); // 适当调整卡片宽度

    int row = 0, col = 0, visibleCards = 0;

    // 5. 收集要显示的场所
    QVector<QPair<QString, QString>> placesToShow; // <placeId, placeName>

    for (auto it = m_placeReservationCount.begin(); it != m_placeReservationCount.end(); ++it) {
        QString placeId = it.key();
        int reservationCount = it.value();

        // 获取场所信息
        QString placeName = getPlaceNameById(placeId);
        QStringList equipmentList = getEquipmentListForPlace(placeId);

        // 检查场所信息是否有效
        if (placeName.isEmpty() || placeId.isEmpty()) {
            qWarning() << "跳过无效场所:" << placeId;
            continue;
        }

        // 检测场所类型
        QString placeType = detectPlaceType(placeName);

        // 应用筛选条件
        bool shouldShow = true;

        // 类型筛选
        if (selectedPlaceType != "all" && placeType != selectedPlaceType) {
            shouldShow = false;
        }

        // 搜索筛选
        if (shouldShow && !searchText.isEmpty()) {
            if (!placeName.contains(searchText, Qt::CaseInsensitive) &&
                !placeId.contains(searchText, Qt::CaseInsensitive)) {
                shouldShow = false;
            }
        }

        if (shouldShow) {
            placesToShow.append(qMakePair(placeId, placeName));
        }
    }

    // 6. 检查去重：确保同一个场所ID只显示一次
    QSet<QString> uniquePlaceIds;
    for (int i = 0; i < placesToShow.size(); i++) {
        const QString &placeId = placesToShow[i].first;
        if (uniquePlaceIds.contains(placeId)) {
            qWarning() << "发现重复的场所ID:" << placeId << "，将被移除";
            placesToShow.removeAt(i);
            i--; // 调整索引
        } else {
            uniquePlaceIds.insert(placeId);
        }
    }

    // 7. 创建卡片
    for (const auto &placeInfo : placesToShow) {
        QString placeId = placeInfo.first;
        QString placeName = placeInfo.second;
        int reservationCount = m_placeReservationCount.value(placeId, 0);
        QStringList equipmentList = getEquipmentListForPlace(placeId);

        try {
            PlaceQueryCard *card = new PlaceQueryCard(placeId, placeName, equipmentList,
                                                      reservationCount, m_placeListPage);

            if (card) {
                connect(card, &PlaceQueryCard::cardClicked,
                        this, &ReservationWidget::onPlaceQueryCardClicked);
                connect(card, &PlaceQueryCard::quickReserveRequested,
                        this, &ReservationWidget::onQuickReserveRequested);

                m_placeQueryCards.append(card);
                m_placeListLayout->addWidget(card, row, col);
                visibleCards++;

                col++;
                if (col >= cardsPerRow) {
                    col = 0;
                    row++;
                }
            }
        } catch (const std::exception &e) {
            qWarning() << "创建场所卡片时异常:" << e.what();
        } catch (...) {
            qWarning() << "创建场所卡片时未知异常";
        }
    }

    // 8. 如果没有可见卡片，显示提示
    if (visibleCards == 0) {
        QString filterInfo;
        if (selectedPlaceType != "all") filterInfo += QString("类型:%1 ").arg(selectedPlaceType);
        if (!searchText.isEmpty()) filterInfo += QString("搜索:%1").arg(searchText);

        QLabel *noMatchLabel = new QLabel(
            filterInfo.isEmpty() ?
                "🔍 没有符合条件的场所" :
                QString("🔍 没有符合条件的场所\n筛选条件: %1").arg(filterInfo),
            m_placeListPage);
        noMatchLabel->setObjectName("emptyPlaceLabel");
        noMatchLabel->setAlignment(Qt::AlignCenter);
        noMatchLabel->setStyleSheet(
            "QLabel {"
            "    color: #95a5a6;"
            "    font-size: 15px;"
            "    padding: 60px;"
            "    background-color: #f8f9fa;"
            "    border-radius: 10px;"
            "}"
            );
        m_placeListLayout->addWidget(noMatchLabel, 0, 0, 1, cardsPerRow, Qt::AlignCenter);
    } else {
        // 只在最后一行之后添加一个统计标签
        // 确定统计标签的位置
        int statsRow = row + 1;
        if (col == 0) {
            // 如果最后一行已满，统计标签放在下一行
            statsRow = row;
        } else {
            // 如果最后一行未满，统计标签放在当前行的下一行
            statsRow = row + 1;
        }

        QLabel *statsLabel = new QLabel(
            QString("共找到 %1 个场所").arg(visibleCards),
            m_placeListPage);
        statsLabel->setObjectName("statsLabel");
        statsLabel->setStyleSheet(
            "QLabel {"
            "    color: #4a69bd;"
            "    font-size: 12px;"
            "    font-weight: bold;"
            "    padding: 5px 15px;"
            "    background-color: #e3f2fd;"
            "    border-radius: 15px;"
            "    margin: 5px;"
            "}"
            );
        m_placeListLayout->addWidget(statsLabel, statsRow, 0, 1, cardsPerRow, Qt::AlignCenter);
    }

    qDebug() << "场所列表刷新完成，显示" << visibleCards << "个场所，去重前总数:" << m_placeReservationCount.size();
}

// 新增：刷新场所详情视图
void ReservationWidget::refreshPlaceDetailView()
{
    qDebug() << "刷新场所详情视图，场所ID:" << m_currentPlaceId;

    if (m_currentPlaceId.isEmpty()) {
        qDebug() << "错误: 当前场所ID为空";
        return;
    }

    // 更新场所信息概览
    QString placeName = getPlaceNameById(m_currentPlaceId);
    int reservationCount = m_placeReservationCount.value(m_currentPlaceId, 0);
    QStringList equipmentList = getEquipmentListForPlace(m_currentPlaceId);

    if (m_placeDetailNameLabel) {
        m_placeDetailNameLabel->setText("🏢 " + placeName);
    }

    if (m_placeDetailStatsLabel) {
        QString equipmentText = equipmentList.isEmpty() ? "无设备" : equipmentList.join(", ");
        m_placeDetailStatsLabel->setText(
            QString("📅 预约记录: %1 条 | 🔧 设备: %2").arg(reservationCount).arg(equipmentText)
            );
    }

    // 刷新该场所的预约记录（只显示当前场所的记录）
    refreshQueryCardViewForPlace(m_currentPlaceId);
}

// 新增：计算场所统计数据
void ReservationWidget::calculatePlaceStats()
{
    qDebug() << "计算场所统计数据，当前卡片数量:" << m_queryCards.size();

    m_placeReservationCount.clear();
    m_placeReservations.clear();

    // 使用QSet来去重，确保每个场所只被统计一次
    QSet<QString> uniquePlaceIds;

    // 遍历所有预约记录，按场所分组
    for (ReservationCard *card : m_queryCards) {
        if (!card) continue;

        QString placeId = card->placeId();

        // 如果已经统计过这个场所，跳过
        if (!uniquePlaceIds.contains(placeId)) {
            uniquePlaceIds.insert(placeId);

            // 统计这个场所的预约数量
            int countForThisPlace = 0;
            for (ReservationCard *otherCard : m_queryCards) {
                if (otherCard && otherCard->placeId() == placeId) {
                    countForThisPlace++;
                }
            }

            m_placeReservationCount[placeId] = countForThisPlace;
        }
    }

    qDebug() << "场所统计完成，共" << m_placeReservationCount.size() << "个场所有预约记录";
}

// 修改：更新查询结果时同时计算场所统计
void ReservationWidget::updateQueryResultTable(const QString &data)
{
    qDebug() << "=== updateQueryResultTable ===";

    // 清空现有数据
    clearQueryCardView();

    // 只清空场所统计，不清空场所列表，因为会在后续刷新
    m_placeReservationCount.clear();
    m_placeReservations.clear();

    // 检查数据是否有效
    if (data.isEmpty() || data == "暂无预约记录" || data == "0") {
        qDebug() << "数据为空";
        // 如果当前在场所详情页面，返回场所列表
        if (m_queryViewStack && m_queryViewStack->currentIndex() == 1) {
            m_queryViewStack->setCurrentIndex(0);
        }
        return;
    }

    qDebug() << "原始数据:" << data;

    // 解析数据
    QStringList reservations = data.split(';', Qt::SkipEmptyParts);
    qDebug() << "预约记录数量:" << reservations.size();

    // 用于收集场所信息
    QSet<QString> uniquePlaces;
    QVector<ReservationCard*> tempCards;  // 使用临时容器存储卡片

    for (int i = 0; i < reservations.size(); ++i) {
        QString reservationStr = reservations[i].trimmed();
        if (reservationStr.isEmpty()) continue;

        QStringList fields = reservationStr.split('|');

        if (fields.size() >= 7) {
            QString reservationId = fields[0].trimmed();
            QString placeId = fields[1].trimmed();
            QString userId = fields[2].trimmed();
            QString purpose = fields[3].trimmed();
            QString startTime = fields[4].trimmed();
            QString endTime = fields[5].trimmed();
            QString status = fields[6].trimmed();

            qDebug() << "解析到预约记录 - ID:" << reservationId
                     << "场所:" << placeId
                     << "用户:" << userId
                     << "状态:" << status;

            // 获取场所名称
            QString placeName = getPlaceNameById(placeId);
            uniquePlaces.insert(placeId);

            // 获取设备列表
            QStringList equipmentList = getEquipmentListForPlace(placeId);
            QString equipmentText = equipmentList.isEmpty() ? "无设备" : equipmentList.join(", ");

            try {
                // 创建预约卡片，但不立即添加到布局
                ReservationCard *card = new ReservationCard(
                    reservationId, placeId, placeName, userId, purpose,
                    startTime, endTime, status, equipmentText, nullptr);

                if (card) {
                    connect(card, &ReservationCard::cardClicked,
                            this, &ReservationWidget::onReservationCardClicked);
                    connect(card, &ReservationCard::statusActionRequested,
                            this, &ReservationWidget::onStatusActionRequested);

                    tempCards.append(card);  // 先添加到临时容器
                }
            } catch (...) {
                qWarning() << "创建卡片时异常";
            }
        } else {
            qWarning() << "记录格式不正确，字段数:" << fields.size();
        }
    }

    // 现在将所有卡片添加到主容器
    m_queryCards = tempCards;
    qDebug() << "解析完成，总共创建了" << m_queryCards.size() << "个卡片";

    // 更新筛选工具栏
    if (m_queryFilterBar) {
        QStringList placeNames;
        for (const QString &placeId : uniquePlaces) {
            placeNames.append(getPlaceNameById(placeId));
        }
        m_queryFilterBar->setPlaces(placeNames);
    }

    // 计算场所统计数据
    calculatePlaceStats();

    // 根据当前视图状态决定显示什么
    if (m_queryViewStack) {
        int currentIndex = m_queryViewStack->currentIndex();
        qDebug() << "当前视图索引:" << currentIndex;

        if (currentIndex == 0) {
            // 当前在场所列表页面，刷新场所列表
            QTimer::singleShot(100, this, &ReservationWidget::refreshPlaceListView);
        } else if (currentIndex == 1 && !m_currentPlaceId.isEmpty()) {
            // 当前在场所详情页面，刷新该场所的预约记录
            QTimer::singleShot(100, this, [this]() {
                refreshQueryCardViewForPlace(m_currentPlaceId);
            });
        }
    }

    qDebug() << "updateQueryResultTable 完成";
}

// 新增：清空场所列表
void ReservationWidget::clearPlaceListView()
{
    qDebug() << "清空场所列表视图";

    // 停止定时器
    if (m_placeListRefreshTimer && m_placeListRefreshTimer->isActive()) {
        m_placeListRefreshTimer->stop();
    }

    // 删除所有场所卡片
    for (PlaceQueryCard *card : m_placeQueryCards) {
        if (card) {
            card->disconnect();  // 断开所有连接
            card->deleteLater();
        }
    }
    m_placeQueryCards.clear();

    // 清空布局
    if (m_placeListLayout) {
        try {
            QLayoutItem *child;
            while ((child = m_placeListLayout->takeAt(0)) != nullptr) {
                if (child->widget()) {
                    QWidget *widget = child->widget();
                    widget->hide();
                    widget->setParent(nullptr);
                    widget->deleteLater();
                }
                delete child;
            }
        } catch (...) {
            qWarning() << "清空场所列表布局时异常";
        }
    }

    qDebug() << "场所列表清空完成";
}

// 新增：场所卡片点击事件处理
void ReservationWidget::onPlaceQueryCardClicked(const QString &placeId)
{
    qDebug() << "场所卡片被点击:" << placeId;

    m_currentPlaceId = placeId;
    m_currentPlaceName = getPlaceNameById(placeId);

    // 切换到场所详情页面
    m_queryViewStack->setCurrentIndex(1);

    // 更新筛选工具栏为场所详情模式，并传入场所名称
    if (m_queryFilterBarDetail) {
        m_queryFilterBarDetail->setMode(false, m_currentPlaceName);

        // 在场所详情页面，我们不设置场所筛选（因为是固定的当前场所）
        // 只保留其他筛选条件：状态、日期、搜索等
    }

    // 刷新场所详情视图
    refreshPlaceDetailView();
}

// 新增：快速预约请求处理
void ReservationWidget::onQuickReserveRequested(const QString &placeId)
{
    qDebug() << "快速预约请求:" << placeId;

    // 切换到申请标签页，并自动选择该场所
    m_tabWidget->setCurrentIndex(0); // 假设申请页是第一个标签页

    // 在申请页选中该场所
    if (m_placeCards.contains(placeId)) {
        onPlaceCardClicked(placeId);
    }

    // 可以添加一个提示
    QMessageBox::information(this, "快速预约",
                             QString("已切换到预约申请页面\n场所: %1\n请填写预约信息").arg(getPlaceNameById(placeId)));
}

// 新增：返回场所列表
void ReservationWidget::onBackToPlaceList()
{
    qDebug() << "返回场所列表";

    // 切换到场所列表页面
    m_queryViewStack->setCurrentIndex(0);

    // 更新筛选工具栏为场所列表模式
    if (m_queryFilterBar) {
        m_queryFilterBar->setMode(true, "");
    }
}

void ReservationWidget::safeUpdateQueryResultTable(const QString &data)
{
    if (!isInMainThread()) {
        QMetaObject::invokeMethod(this, [this, data]() {
            updateQueryResultTable(data);
        }, Qt::QueuedConnection);
    } else {
        updateQueryResultTable(data);
    }
}

void ReservationWidget::onFilterChanged()
{
    qDebug() << "筛选条件改变，当前页面索引:" << m_queryViewStack->currentIndex();

    if (m_queryViewStack->currentIndex() == 0) {
        // 当前在场所列表页面
        if (m_placeListRefreshTimer->isActive()) {
            m_placeListRefreshTimer->stop();
        }
        m_placeListRefreshTimer->start(200);
    } else if (m_queryViewStack->currentIndex() == 1) {
        // 当前在场所详情页面
        if (!m_currentPlaceId.isEmpty()) {
            refreshQueryCardViewForPlace(m_currentPlaceId);
        }
    }
}

void ReservationWidget::refreshQueryCardView()
{
    qDebug() << "=== 刷新预约查询卡片视图 ===";

    // 防止重复刷新
    static QElapsedTimer lastRefreshTime;
    if (lastRefreshTime.isValid() && lastRefreshTime.elapsed() < 100) {
        qDebug() << "跳过频繁刷新";
        return;
    }
    lastRefreshTime.start();

    if (!m_queryCardLayout || !m_queryCardContainer) {
        qDebug() << "错误: 关键控件为空";
        return;
    }

    // 直接清空并重新构建布局
    // 1. 从布局中移除所有卡片（但不删除卡片对象）
    QLayoutItem* child;
    while ((child = m_queryCardLayout->takeAt(0)) != nullptr) {
        if (child->widget()) {
            child->widget()->setVisible(false);
            child->widget()->setParent(nullptr);
        }
        delete child;
    }

    // 2. 如果没有卡片，显示空状态
    if (m_queryCards.isEmpty()) {
        QLabel *emptyLabel = new QLabel("📭 暂无预约记录", m_queryCardContainer);
        emptyLabel->setAlignment(Qt::AlignCenter);
        emptyLabel->setStyleSheet("color: #7f8c8d; font-size: 16px; padding: 60px; background-color: #f8f9fa; border-radius: 8px;");
        m_queryCardLayout->addWidget(emptyLabel);
        return;
    }

    // 3. 获取筛选条件
    QString selectedPlace = m_queryFilterBar ? m_queryFilterBar->selectedPlace() : "all";
    QString selectedStatus = m_queryFilterBar ? m_queryFilterBar->selectedStatus() : "all";
    QString selectedDateRange = m_queryFilterBar ? m_queryFilterBar->selectedDate() : "all";
    QString searchText = m_queryFilterBar ? m_queryFilterBar->searchText() : "";

    QDate startDate, endDate;
    if (selectedDateRange != "all") {
        startDate = m_queryFilterBar->startDate();
        endDate = m_queryFilterBar->endDate();
    }

    // 处理空值
    if (selectedPlace.isEmpty()) selectedPlace = "all";
    if (selectedStatus.isEmpty()) selectedStatus = "all";

    // 4. 创建网格布局容器
    QWidget *gridContainer = new QWidget(m_queryCardContainer);
    QGridLayout *gridLayout = new QGridLayout(gridContainer);
    gridLayout->setContentsMargins(0, 0, 0, 0);
    gridLayout->setHorizontalSpacing(20);
    gridLayout->setVerticalSpacing(20);

    // 5. 计算每行卡片数量
    int containerWidth = m_queryCardContainer->width();
    if (containerWidth <= 0) containerWidth = 800;
    int cardsPerRow = qMax(1, containerWidth / 320);

    // 6. 状态映射表（已完善）
    QMap<QString, QStringList> statusMap = {
        {"all", {"all", "全部状态"}},
        {"pending", {"pending", "待审批", "未审批", "pending", "待审核", "未审核"}},
        {"approved", {"approved", "已批准", "通过", "approved", "已同意", "已授权"}},
        {"rejected", {"rejected", "已拒绝", "拒绝", "rejected", "驳回", "未通过"}},
        {"completed", {"completed", "已完成", "completed", "已结束", "已完成"}},
        {"cancelled", {"cancelled", "已取消", "cancelled", "取消", "已作废"}}
    };

    // 7. 筛选并添加卡片
    int row = 0, col = 0, visibleCards = 0;

    for (ReservationCard *card : m_queryCards) {
        if (!card) continue;

        bool shouldShow = true;

        // 场所筛选
        if (selectedPlace != "all") {
            QString cardPlaceId = card->placeId();
            // 检查是否是完整的场所ID匹配
            bool placeMatch = false;

            // 直接ID匹配
            if (cardPlaceId == selectedPlace) {
                placeMatch = true;
            }
            // 检查场所名称是否包含筛选文本
            else if (card->placeName().contains(selectedPlace, Qt::CaseInsensitive)) {
                placeMatch = true;
            }

            if (!placeMatch) {
                shouldShow = false;
            }
        }

        // 状态筛选
        if (shouldShow && selectedStatus != "all") {
            QString cardStatus = card->status().toLower().trimmed();
            QStringList possibleStatus = statusMap.value(selectedStatus.toLower());
            bool statusMatch = false;

            for (const QString &status : possibleStatus) {
                if (cardStatus.contains(status, Qt::CaseInsensitive)) {
                    statusMatch = true;
                    break;
                }
            }

            if (!statusMatch) {
                shouldShow = false;
            }
        }

        // 日期范围筛选
        if (shouldShow && selectedDateRange != "all" && startDate.isValid() && endDate.isValid()) {
            QDate cardStartDate = card->getStartDate();
            QDate cardEndDate = card->getEndDate();

            // 检查卡片的开始或结束日期是否在筛选范围内
            bool dateInRange = (cardStartDate >= startDate && cardStartDate <= endDate) ||
                               (cardEndDate >= startDate && cardEndDate <= endDate) ||
                               (cardStartDate <= startDate && cardEndDate >= endDate);

            if (!dateInRange) {
                shouldShow = false;
            }
        }

        // 搜索筛选（更全面的字段搜索）
        if (shouldShow && !searchText.isEmpty()) {
            QString searchLower = searchText.toLower();
            QString cardText = card->reservationId() + "|" +
                               card->placeName() + "|" +
                               card->userId() + "|" +
                               card->purpose() + "|" +
                               card->equipmentList() + "|" +
                               card->status();

            if (!cardText.toLower().contains(searchLower)) {
                shouldShow = false;
            }
        }

        // 添加可见卡片
        if (shouldShow) {
            card->setParent(gridContainer);
            card->setVisible(true);
            gridLayout->addWidget(card, row, col);
            visibleCards++;

            col++;
            if (col >= cardsPerRow) {
                col = 0;
                row++;
            }
        }
    }

    // 8. 如果没有可见卡片，显示提示
    if (visibleCards == 0) {
        delete gridContainer;

        // 构建筛选条件提示
        QString filterInfo;
        if (selectedPlace != "all") filterInfo += QString("场所:%1 ").arg(selectedPlace);
        if (selectedStatus != "all") filterInfo += QString("状态:%1 ").arg(selectedStatus);
        if (selectedDateRange != "all") filterInfo += QString("日期:%1 ").arg(selectedDateRange);
        if (!searchText.isEmpty()) filterInfo += QString("搜索:%1").arg(searchText);

        QLabel *noMatchLabel = new QLabel(
            filterInfo.isEmpty() ?
                "🔍 没有符合条件的预约记录" :
                QString("🔍 没有符合条件的预约记录\n筛选条件: %1").arg(filterInfo),
            m_queryCardContainer);
        noMatchLabel->setAlignment(Qt::AlignCenter);
        noMatchLabel->setStyleSheet(
            "QLabel {"
            "    color: #95a5a6;"
            "    font-size: 15px;"
            "    padding: 60px;"
            "    background-color: #f8f9fa;"
            "    border-radius: 10px;"
            "    margin: 10px;"
            "}");
        m_queryCardLayout->addWidget(noMatchLabel);
    } else {
        m_queryCardLayout->addWidget(gridContainer);
        qDebug() << "显示预约记录，可见卡片数量:" << visibleCards << "/" << m_queryCards.size();

        // 添加筛选结果统计
        QLabel *resultLabel = new QLabel(
            QString("共找到 %1 个预约记录").arg(visibleCards),
            m_queryCardContainer);
        resultLabel->setStyleSheet(
            "QLabel {"
            "    color: #4a69bd;"
            "    font-size: 12px;"
            "    font-weight: bold;"
            "    padding: 5px 15px;"
            "    background-color: #e3f2fd;"
            "    border-radius: 15px;"
            "    margin: 5px;"
            "}");
        gridLayout->addWidget(resultLabel, row + 1, 0, 1, cardsPerRow, Qt::AlignCenter);
    }

    m_queryCardLayout->addStretch();
    qDebug() << "刷新完成，筛选条件 - 场所:" << selectedPlace
             << "状态:" << selectedStatus
             << "日期范围:" << selectedDateRange;
}

void ReservationWidget::onReservationCardClicked(const QString &reservationId)
{
    qDebug() << "预约卡片被点击:" << reservationId;
    // 这里可以添加卡片选中逻辑，比如高亮显示
    if (m_queryCardMap.contains(reservationId)) {
        // 取消之前选中的卡片
        for (ReservationCard *card : m_queryCards) {
            if (card->isSelected() && card->reservationId() != reservationId) {
                card->setSelected(false);
            }
        }

        // 选中当前卡片
        m_queryCardMap[reservationId]->setSelected(true);
    }
}

void ReservationWidget::onStatusActionRequested(const QString &reservationId, const QString &action)
{
    qDebug() << "预约状态操作请求:" << reservationId << action;

    // 这里需要获取场所ID，可以从卡片中获取或从原始数据中查找
    // 暂时使用一个默认值，实际使用时需要修改
    QString placeId = "default_place";

    if (action == "approve") {
        emit reservationApproveRequested(reservationId.toInt(), placeId, true);
    }
}

void ReservationWidget::onRefreshQueryRequested()
{
    // 重新查询预约数据
    onQueryButtonClicked();
}

// 在 onPlaceCardClicked 函数中添加调试输出
void ReservationWidget::onPlaceCardClicked(const QString &placeId)
{
    qDebug() << "Place card clicked:" << placeId;

    // 取消之前选中的卡片
    if (!m_selectedPlaceId.isEmpty() && m_placeCards.contains(m_selectedPlaceId)) {
        PlaceCard *prevCard = m_placeCards[m_selectedPlaceId];
        if (prevCard) {
            prevCard->setSelected(false);
        }
    }

    // 选中新卡片
    m_selectedPlaceId = placeId;
    if (m_placeCards.contains(placeId)) {
        PlaceCard *card = m_placeCards[placeId];
        if (card) {
            card->setSelected(true);

            // 显示设备列表
            QStringList equipmentList = card->equipmentList();
            if (m_selectedEquipmentText) {
                if (equipmentList.isEmpty()) {
                    m_selectedEquipmentText->setText("该场所暂无设备信息");
                } else {
                    m_selectedEquipmentText->setText(equipmentList.join("\n"));
                }
            } else {
                qDebug() << "Error: m_selectedEquipmentText is null";
            }
        } else {
            qDebug() << "Error: card is null for placeId:" << placeId;
        }
    } else {
        qDebug() << "Error: placeId not found in m_placeCards:" << placeId;
    }
}
void ReservationWidget::onSelectAllChanged(int state)
{
    bool checked = (state == Qt::Checked);
    for (ReservationCard *card : m_approveCards) {
        card->setSelected(checked);
    }

    // 更新批量操作按钮状态
    m_batchApproveButton->setEnabled(checked);
    m_batchRejectButton->setEnabled(checked);
}

void ReservationWidget::onBatchApprove()
{
    // 实现批量批准逻辑
}

void ReservationWidget::onBatchReject()
{
    // 实现批量拒绝逻辑
}

void ReservationWidget::onApproveFilterChanged()
{
    refreshApproveCardView();
}

void ReservationWidget::onApproveRefreshRequested()
{
    emit reservationQueryRequested("all");
}

void ReservationWidget::refreshApproveCardView()
{
    if (!m_approveCardLayout || !m_approveCardContainer) return;

    // 清空现有布局
    QLayoutItem *item;
    while ((item = m_approveCardLayout->takeAt(0)) != nullptr) {
        if (item->widget()) {
            item->widget()->setParent(nullptr);
            delete item->widget();
        }
        delete item;
    }

    // 如果没有卡片，显示提示信息
    if (m_approveCards.isEmpty()) {
        QLabel *emptyLabel = new QLabel("暂无待审批预约", m_approveCardContainer);
        emptyLabel->setAlignment(Qt::AlignCenter);
        emptyLabel->setStyleSheet(
            "QLabel {"
            "    color: #999;"
            "    font-size: 16px;"
            "    padding: 60px;"
            "}");
        m_approveCardLayout->addWidget(emptyLabel);
        return;
    }

    // 获取筛选条件
    QString selectedPlace = m_approveFilterBar->selectedPlace();
    QString selectedStatus = m_approveFilterBar->selectedStatus();
    QString searchText = m_approveFilterBar->searchText();

    // 处理空值
    if (selectedPlace.isEmpty()) {
        selectedPlace = "all";
    }

    // 创建网格布局
    QGridLayout *gridLayout = new QGridLayout();
    gridLayout->setContentsMargins(0, 0, 0, 0);
    gridLayout->setHorizontalSpacing(20);
    gridLayout->setVerticalSpacing(20);

    // 计算每行卡片数量
    int containerWidth = m_approveCardContainer->width();
    int cardsPerRow = qMax(1, containerWidth / 340);

    int row = 0;
    int col = 0;
    int visibleCards = 0;

    // 状态映射表
    QMap<QString, QStringList> statusMap = {
        {"all", {"all", "全部状态"}},
        {"pending", {"pending", "待审批", "未审批", "pending"}},
        {"approved", {"approved", "已批准", "通过", "approved"}},
        {"rejected", {"rejected", "已拒绝", "拒绝", "rejected"}},
        {"completed", {"completed", "已完成", "completed"}},
        {"cancelled", {"cancelled", "已取消", "cancelled"}}
    };

    // 收集可见卡片
    for (ReservationCard *card : m_approveCards) {
        if (!card) continue;

        bool shouldShow = true;

        // 场所筛选
        if (selectedPlace != "all") {
            QString cardPlaceId = card->placeId();
            if (cardPlaceId != selectedPlace) {
                shouldShow = false;
            }
        }

        // 状态筛选 - 只显示待审批的预约
        if (shouldShow) {
            QString cardStatus = card->status().toLower();
            QString filterStatus = "pending";  // 审批页默认只显示待审批

            if (selectedStatus != "all") {
                filterStatus = selectedStatus;
            }

            QStringList possibleStatusValues = statusMap.value(filterStatus, QStringList());

            bool statusMatch = false;
            for (const QString &possibleStatus : possibleStatusValues) {
                if (cardStatus.contains(possibleStatus, Qt::CaseInsensitive)) {
                    statusMatch = true;
                    break;
                }
            }

            if (!statusMatch) {
                shouldShow = false;
            }
        }

        // 搜索文本筛选
        if (shouldShow && !searchText.isEmpty()) {
            bool textMatch =
                card->reservationId().contains(searchText, Qt::CaseInsensitive) ||
                card->placeName().contains(searchText, Qt::CaseInsensitive) ||
                card->userId().contains(searchText, Qt::CaseInsensitive) ||
                card->purpose().contains(searchText, Qt::CaseInsensitive);

            if (!textMatch) {
                shouldShow = false;
            }
        }

        // 添加到布局
        if (shouldShow) {
            gridLayout->addWidget(card, row, col);
            visibleCards++;

            col++;
            if (col >= cardsPerRow) {
                col = 0;
                row++;
            }
        }
    }

    QWidget *gridWidget = new QWidget(m_approveCardContainer);
    gridWidget->setLayout(gridLayout);
    m_approveCardLayout->addWidget(gridWidget);

    // 如果没有可见卡片，显示提示信息
    if (visibleCards == 0) {
        QLabel *noPendingLabel = new QLabel("没有待审批的预约", gridWidget);
        noPendingLabel->setAlignment(Qt::AlignCenter);
        noPendingLabel->setStyleSheet(
            "QLabel {"
            "    color: #999;"
            "    font-size: 14px;"
            "    padding: 40px;"
            "}");
        gridLayout->addWidget(noPendingLabel, 0, 0, 1, cardsPerRow, Qt::AlignCenter);
    }
}

void ReservationWidget::refreshQueryCardViewForPlace(const QString &placeId)
{
    qDebug() << "刷新场所预约记录视图，场所ID:" << placeId;

    if (!m_queryCardLayout || !m_queryCardContainer) {
        qDebug() << "错误: 关键控件为空";
        return;
    }

    // 清空现有布局
    QLayoutItem* child;
    while ((child = m_queryCardLayout->takeAt(0)) != nullptr) {
        if (child->widget()) {
            child->widget()->setVisible(false);
            child->widget()->setParent(nullptr);
        }
        delete child;
    }

    // 如果没有预约记录，显示提示
    if (m_queryCards.isEmpty()) {
        QLabel *emptyLabel = new QLabel("📭 该场所暂无预约记录", m_queryCardContainer);
        emptyLabel->setAlignment(Qt::AlignCenter);
        emptyLabel->setStyleSheet("color: #7f8c8d; font-size: 16px; padding: 60px; background-color: #f8f9fa; border-radius: 8px;");
        m_queryCardLayout->addWidget(emptyLabel);
        return;
    }

    // 创建网格布局容器
    QWidget *gridContainer = new QWidget(m_queryCardContainer);
    QGridLayout *gridLayout = new QGridLayout(gridContainer);
    gridLayout->setContentsMargins(0, 0, 0, 0);
    gridLayout->setHorizontalSpacing(20);
    gridLayout->setVerticalSpacing(20);

    // 计算每行卡片数量
    int containerWidth = m_queryCardContainer->width();
    if (containerWidth <= 0) containerWidth = 800;
    int cardsPerRow = qMax(1, containerWidth / 320);

    // 获取筛选条件（除了场所筛选）
    QString selectedStatus = m_queryFilterBarDetail ? m_queryFilterBarDetail->selectedStatus() : "all";
    QString selectedDateRange = m_queryFilterBarDetail ? m_queryFilterBarDetail->selectedDate() : "all";
    QString searchText = m_queryFilterBarDetail ? m_queryFilterBarDetail->searchText() : "";

    QDate startDate, endDate;
    if (selectedDateRange != "all") {
        startDate = m_queryFilterBarDetail->startDate();
        endDate = m_queryFilterBarDetail->endDate();
    }

    // 状态映射表
    QMap<QString, QStringList> statusMap = {
        {"all", {"all", "全部状态"}},
        {"pending", {"pending", "待审批", "未审批", "pending", "待审核", "未审核"}},
        {"approved", {"approved", "已批准", "通过", "approved", "已同意", "已授权"}},
        {"rejected", {"rejected", "已拒绝", "拒绝", "rejected", "驳回", "未通过"}},
        {"completed", {"completed", "已完成", "completed", "已结束", "已完成"}},
        {"cancelled", {"cancelled", "已取消", "cancelled", "取消", "已作废"}}
    };

    // 筛选并添加卡片
    int row = 0, col = 0, visibleCards = 0;

    for (ReservationCard *card : m_queryCards) {
        if (!card) continue;

        // 首先，只显示当前场所的记录
        if (card->placeId() != placeId) {
            continue;
        }

        bool shouldShow = true;

        // 状态筛选
        if (shouldShow && selectedStatus != "all") {
            QString cardStatus = card->status().toLower().trimmed();
            QStringList possibleStatus = statusMap.value(selectedStatus.toLower());
            bool statusMatch = false;

            for (const QString &status : possibleStatus) {
                if (cardStatus.contains(status, Qt::CaseInsensitive)) {
                    statusMatch = true;
                    break;
                }
            }

            if (!statusMatch) {
                shouldShow = false;
            }
        }

        // 日期范围筛选
        if (shouldShow && selectedDateRange != "all" && startDate.isValid() && endDate.isValid()) {
            QDate cardStartDate = card->getStartDate();
            QDate cardEndDate = card->getEndDate();

            // 检查卡片的开始或结束日期是否在筛选范围内
            bool dateInRange = (cardStartDate >= startDate && cardStartDate <= endDate) ||
                               (cardEndDate >= startDate && cardEndDate <= endDate) ||
                               (cardStartDate <= startDate && cardEndDate >= endDate);

            if (!dateInRange) {
                shouldShow = false;
            }
        }

        // 搜索筛选
        if (shouldShow && !searchText.isEmpty()) {
            QString searchLower = searchText.toLower();
            QString cardText = card->reservationId() + "|" +
                               card->userId() + "|" +
                               card->purpose() + "|" +
                               card->equipmentList() + "|" +
                               card->status();

            if (!cardText.toLower().contains(searchLower)) {
                shouldShow = false;
            }
        }

        // 添加可见卡片
        if (shouldShow) {
            card->setParent(gridContainer);
            card->setVisible(true);
            gridLayout->addWidget(card, row, col);
            visibleCards++;

            col++;
            if (col >= cardsPerRow) {
                col = 0;
                row++;
            }
        }
    }

    // 如果没有可见卡片，显示提示
    if (visibleCards == 0) {
        delete gridContainer;

        // 构建筛选条件提示
        QString filterInfo;
        if (selectedStatus != "all") filterInfo += QString("状态:%1 ").arg(selectedStatus);
        if (selectedDateRange != "all") filterInfo += QString("日期:%1 ").arg(selectedDateRange);
        if (!searchText.isEmpty()) filterInfo += QString("搜索:%1").arg(searchText);

        QLabel *noMatchLabel = new QLabel(
            filterInfo.isEmpty() ?
                "🔍 该场所没有符合条件的预约记录" :
                QString("🔍 该场所没有符合条件的预约记录\n筛选条件: %1").arg(filterInfo),
            m_queryCardContainer);
        noMatchLabel->setAlignment(Qt::AlignCenter);
        noMatchLabel->setStyleSheet(
            "QLabel {"
            "    color: #95a5a6;"
            "    font-size: 15px;"
            "    padding: 60px;"
            "    background-color: #f8f9fa;"
            "    border-radius: 10px;"
            "    margin: 10px;"
            "}");
        m_queryCardLayout->addWidget(noMatchLabel);
    } else {
        m_queryCardLayout->addWidget(gridContainer);
        qDebug() << "显示该场所的预约记录，可见卡片数量:" << visibleCards;

        // 添加筛选结果统计
        QLabel *resultLabel = new QLabel(
            QString("该场所共找到 %1 个预约记录").arg(visibleCards),
            m_queryCardContainer);
        resultLabel->setStyleSheet(
            "QLabel {"
            "    color: #4a69bd;"
            "    font-size: 12px;"
            "    font-weight: bold;"
            "    padding: 5px 15px;"
            "    background-color: #e3f2fd;"
            "    border-radius: 15px;"
            "    margin: 5px;"
            "}");
        gridLayout->addWidget(resultLabel, row + 1, 0, 1, cardsPerRow, Qt::AlignCenter);
    }

    m_queryCardLayout->addStretch();
    qDebug() << "场所详情视图刷新完成";
}

void ReservationWidget::clearApproveCardView()
{
    for (ReservationCard *card : m_approveCards) {
        if (card) {
            card->deleteLater();
        }
    }
    m_approveCards.clear();
    m_approveCardMap.clear();
}


void ReservationWidget::clearQueryCardView()
{
    qDebug() << "清空查询卡片视图";

    // 先断开所有连接
    for (ReservationCard *card : m_queryCards) {
        if (card) {
            try {
                card->disconnect();  // 断开所有信号连接
            } catch (...) {
                qWarning() << "断开卡片连接时异常";
            }
        }
    }

    // 删除卡片对象
    qDeleteAll(m_queryCards);
    m_queryCards.clear();
    m_queryCardMap.clear();

    // 清空布局
    if (m_queryCardLayout) {
        try {
            QLayoutItem* child;
            while ((child = m_queryCardLayout->takeAt(0)) != nullptr) {
                if (child->widget()) {
                    child->widget()->hide();
                    child->widget()->setParent(nullptr);
                }
                delete child;
            }
        } catch (...) {
            qWarning() << "清空布局时异常";
        }
    }

    qDebug() << "清空完成";
}

void ReservationWidget::setUserRole(const QString &role, const QString &userId)
{
    m_userRole = role;
    m_currentUserId = userId;  // 新增：保存用户ID
    qDebug() << "DEBUG: setUserRole called, role=" << role << ", userId=" << userId;

    // 确保 setupApproveTab 已创建审批页（在构造函数中已调用）
    if (role == "admin") {
        qDebug() << "DEBUG: 管理员，显示审批页";
        // 如果审批页不存在，添加它
        if (m_tabWidget->count() < 3) {
            setupApproveTab();  // 创建审批页
        }
    } else {
        qDebug() << "DEBUG: 非管理员，移除审批页";
        // 检查是否已经有审批页
        for (int i = 0; i < m_tabWidget->count(); i++) {
            if (m_tabWidget->tabText(i) == "预约审批") {
                m_tabWidget->removeTab(i);
                break;
            }
        }
    }
}

void ReservationWidget::onApplyButtonClicked()
{
    if (m_placeComboApply->currentIndex() == -1) {
        QMessageBox::warning(this, "提示", "请先选择场所");
        return;
    }

    // 检查时间有效性
    QDateTime startDateTime = QDateTime(m_startDateEdit->date(), m_startTimeEdit->time());
    QDateTime endDateTime = QDateTime(m_endDateEdit->date(), m_endTimeEdit->time());

    if (startDateTime >= endDateTime) {
        QMessageBox::warning(this, "时间错误", "开始时间必须早于结束时间");
        return;
    }

    if (startDateTime < QDateTime::currentDateTime()) {
        QMessageBox::warning(this, "时间错误", "开始时间不能是过去时间");
        return;
    }

    // 组合日期时间
    QString startDateTimeStr = startDateTime.toString("yyyy-MM-dd HH:mm:ss");
    QString endDateTimeStr = endDateTime.toString("yyyy-MM-dd HH:mm:ss");

    emit reservationApplyRequested(
        m_placeComboApply->currentData().toString(),
        m_purposeEdit->text(),
        startDateTimeStr,
        endDateTimeStr
        );
}

void ReservationWidget::onQueryButtonClicked()
{
    emit reservationQueryRequested(m_placeComboQuery->currentData().toString());
}


void ReservationWidget::loadAllReservationsForApproval(const QString &data)
{
    qDebug() << "=== 审批页数据加载 ===";

    // 清空现有卡片
    clearApproveCardView();

    if (data.isEmpty() || data == "暂无预约记录" || data == "fail|暂无数据") {
        refreshApproveCardView();
        return;
    }

    // 解析数据
    QStringList reservations = data.split(';', Qt::SkipEmptyParts);

    for (int i = 0; i < reservations.size(); ++i) {
        QStringList fields = reservations[i].split('|');
        if (fields.size() >= 7) {
            QString reservationId = fields[0];
            QString placeId = fields[1];
            QString userId = fields[2];
            QString purpose = fields[3];
            QString startTime = fields[4];
            QString endTime = fields[5];
            QString status = fields[6];

            QString placeName = getPlaceNameById(placeId);
            QStringList equipmentList = getEquipmentListForPlace(placeId);
            QString equipmentText = equipmentList.join(", ");

            // 创建审批卡片 - 修复：使用9参数构造函数
            ReservationCard *card = new ReservationCard(
                reservationId,      // reservationId
                placeId,            // placeId
                placeName,          // placeName
                userId,             // userId
                purpose,            // purpose
                startTime,          // startTime
                endTime,            // endTime
                status,             // status
                equipmentText,      // equipmentList
                m_approveCardContainer  // parent
                );

            connect(card, &ReservationCard::cardClicked,
                    this, &ReservationWidget::onReservationCardClicked);
            connect(card, &ReservationCard::statusActionRequested,
                    this, &ReservationWidget::onStatusActionRequested);

            m_approveCards.append(card);
            m_approveCardMap[reservationId] = card;
        }
    }

    // 刷新审批卡片视图
    refreshApproveCardView();
}

// ✅ 新增公有方法：强制刷新当前场所设备
void ReservationWidget::refreshCurrentPlaceEquipment()
{
    if (m_placeComboApply->count() > 0) {
        m_placeComboApply->setCurrentIndex(0);  // 选中第一项
        updateEquipmentListDisplay();           // 立即更新显示
    }
}

void ReservationWidget::clearEquipmentList()
{
    if (m_equipmentListText) {
        m_equipmentListText->clear();
    }
}

QString ReservationWidget::getCurrentSelectedPlaceId() const
{
    if (!m_approveTable || m_approveTable->currentRow() < 0) {
        return QString();  // ✅ 返回空字符串
    }

    // ✅ 第1列是场所ID列（不是第0列）
    QTableWidgetItem *item = m_approveTable->item(m_approveTable->currentRow(), 1);
    if (!item) {
        return QString();
    }

    QString placeIdText = item->text();

    // ✅ 提取括号内的ID（处理"名称 (ID)"格式）
    QRegularExpression rx("\\(([^)]+)\\)");
    QRegularExpressionMatch match = rx.match(placeIdText);
    if (match.hasMatch()) {
        return match.captured(1);
    }

    return placeIdText;  // ✅ 直接返回文本
}

int ReservationWidget::getCurrentSelectedReservationId() const
{
    if (!m_approveTable || m_approveTable->currentRow() < 0) {
        return -1;  // ✅ 返回int
    }

    QTableWidgetItem *item = m_approveTable->item(m_approveTable->currentRow(), 0);
    if (item) {
        return item->text().toInt();  // ✅ 返回int
    }
    return -1;
}

QString ReservationWidget::getPlaceNameById(const QString &placeId)
{
    if (placeId.isEmpty()) return "未知场所";

    // 从申请页的下拉框查找（数据已加载）
    for (int i = 0; i < m_placeComboApply->count(); ++i) {
        if (m_placeComboApply->itemData(i).toString() == placeId) {
            return m_placeComboApply->itemText(i);
        }
    }

    // 从查询页下拉框查找
    for (int i = 0; i < m_placeComboQuery->count(); ++i) {
        if (m_placeComboQuery->itemData(i).toString() == placeId) {
            return m_placeComboQuery->itemText(i);
        }
    }

    // 如果找不到，返回ID本身
    return QString("场所%1").arg(placeId);
}

QStringList ReservationWidget::getEquipmentListForPlace(const QString &placeId) const
{
    if (placeId.isEmpty()) return QStringList();

    // ✅ 从申请页下拉框的用户角色数据中获取设备列表
    for (int i = 0; i < m_placeComboApply->count(); ++i) {
        if (m_placeComboApply->itemData(i).toString() == placeId) {
            QVariant equipmentData = m_placeComboApply->itemData(i, Qt::UserRole + 1);
            return equipmentData.toStringList();
        }
    }
    return QStringList();
}

void ReservationWidget::onApproveButtonClicked()
{
    QPushButton *btn = qobject_cast<QPushButton*>(sender());
    if (!btn) return;

    // ✅ 获取预约ID和场所ID（从按钮属性）
    int reservationId = btn->property("reservationId").toInt();
    QString placeId = btn->property("placeId").toString();

    qDebug() << "批准预约:" << reservationId << "场所:" << placeId;

    // ✅ 修改：发出信号时传递 placeId
    emit reservationApproveRequested(reservationId, placeId, true);
}

void ReservationWidget::onDenyButtonClicked()
{
    QPushButton *btn = qobject_cast<QPushButton*>(sender());
    if (!btn) return;

    int reservationId = btn->property("reservationId").toInt();
    QString placeId = btn->property("placeId").toString();

    qDebug() << "拒绝预约:" << reservationId << "场所:" << placeId;

    // ✅ 修改：发出信号时传递 placeId
    emit reservationApproveRequested(reservationId, placeId, false);
}

void ReservationWidget::onTabChanged(int index)
{
    qDebug() << "DEBUG: Tab changed to index" << index;
    qDebug() << "DEBUG: m_userRole =" << m_userRole;

    // ✅ 切换到查询页（索引1）时，自动请求数据
    if (index == 1) {
        qDebug() << "DEBUG: 切换到查询页，自动查询全部预约...";

        // 只有在当前没有数据时才查询
        if (m_queryCards.isEmpty()) {
            // 延迟请求，确保UI完全加载
            QTimer::singleShot(200, [this]() {
                qDebug() << "DEBUG: 自动发射 reservationQueryRequested('all')";
                emit reservationQueryRequested("all");
            });
        } else {
            qDebug() << "DEBUG: 已有预约数据，刷新场所列表";
            // 刷新场所列表视图
            if (m_queryViewStack->currentIndex() == 0) {
                refreshPlaceListView();
            }
        }
    }

    // ✅ 切换到审批页（索引2）时，自动请求所有预约数据
    if (index == 2 && m_userRole == "admin") {
        qDebug() << "DEBUG: 切换到审批页，准备请求数据...";

        // 只有在当前没有数据时才查询
        if (m_approveCards.isEmpty()) {
            QTimer::singleShot(100, [this]() {
                qDebug() << "DEBUG: 发射 reservationQueryRequested('all')";
                emit reservationQueryRequested("all");
                qDebug() << "DEBUG: 信号已发射";
            });
        } else {
            qDebug() << "DEBUG: 已有审批数据，跳过自动查询";
        }
    }
}

// ✅ 新增辅助函数：更新设备列表显示
void ReservationWidget::updateEquipmentListDisplay()
{
    QString placeId = m_placeComboApply->currentData().toString();
    if (placeId.isEmpty()) {
        m_equipmentListText->clear();
        return;
    }

    QVariant placeData = m_placeComboApply->currentData(Qt::UserRole + 1);
    QStringList equipmentList = placeData.toStringList();

    if (equipmentList.isEmpty()) {
        m_equipmentListText->setText("该场所暂无设备信息");
    } else {
        m_equipmentListText->setText(equipmentList.join("\n"));
    }

}

void ReservationWidget::updatePlaceCardsLayout()
{
    if (!m_placeCardsContainer || !m_placeCardsLayout || m_placeCards.isEmpty()) {
        return;
    }

    // 重新计算布局
    int containerWidth = m_placeCardsContainer->width();
    if (containerWidth <= 0) {
        return;
    }

    int cardsPerRow = qMax(1, containerWidth / 200); // 每张卡片约200px宽

    // 清空布局
    QLayoutItem *item;
    while ((item = m_placeCardsLayout->takeAt(0)) != nullptr) {
        delete item;
    }

    // 重新排列卡片
    int row = 0;
    int col = 0;
    int cardIndex = 0;

    QList<PlaceCard*> cards = m_placeCards.values();

    for (PlaceCard *card : cards) {
        if (card) {
            m_placeCardsLayout->addWidget(card, row, col);

            col++;
            if (col >= cardsPerRow) {
                col = 0;
                row++;
            }
            cardIndex++;
        }
    }

    // 如果没有卡片，显示提示信息
    if (cardIndex == 0) {
        QLabel *emptyLabel = new QLabel("暂无场所信息", m_placeCardsContainer);
        emptyLabel->setAlignment(Qt::AlignCenter);
        emptyLabel->setStyleSheet("color: #999; font-size: 14px; padding: 40px;");
        m_placeCardsLayout->addWidget(emptyLabel, 0, 0, 1, cardsPerRow, Qt::AlignCenter);
    }
}

QString ReservationWidget::detectPlaceType(const QString &placeName)
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

void ReservationWidget::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);

    // 延迟刷新布局
    static QTimer resizeTimer;
    resizeTimer.setSingleShot(true);

    disconnect(&resizeTimer, &QTimer::timeout, this, nullptr);

    connect(&resizeTimer, &QTimer::timeout, this, [this]() {
        if (m_tabWidget) {
            int currentTab = m_tabWidget->currentIndex();
            int currentView = m_queryViewStack ? m_queryViewStack->currentIndex() : 0;

            if (currentTab == 1) { // 查询页
                if (currentView == 0) { // 场所列表页面
                    if (m_placeListRefreshTimer->isActive()) {
                        m_placeListRefreshTimer->stop();
                    }
                    m_placeListRefreshTimer->start(100);
                } else if (currentView == 1) { // 场所详情页面
                    QTimer::singleShot(50, this, &ReservationWidget::refreshQueryCardView);
                }
            }
        }
    });

    resizeTimer.start(200);
}

bool ReservationWidget::event(QEvent *event)
{
    // 处理自定义事件，确保UI操作在主线程
    if (event->type() == QEvent::User + 1) {
        // 自定义事件处理
        return true;
    }
    return QWidget::event(event);
}

void ReservationWidget::updatePlaceCards()
{
    qDebug() << "开始更新场所卡片...";

    // 检查必要的控件是否已初始化
    if (!this) {
        qDebug() << "错误: this 指针为空!";
        return;
    }

    if (!m_placeCardsContainer) {
        qDebug() << "错误: m_placeCardsContainer 未初始化";
        return;
    }

    if (!m_placeCardsLayout) {
        qDebug() << "错误: m_placeCardsLayout 未初始化";
        return;
    }

    if (!m_placeComboApply) {
        qDebug() << "错误: m_placeComboApply 未初始化";
        return;
    }

    qDebug() << "清理现有卡片...";

    // 安全地清理现有卡片
    try {
        // 从布局中移除并删除所有子控件
        QLayoutItem *item;
        while ((item = m_placeCardsLayout->takeAt(0)) != nullptr) {
            if (item->widget()) {
                QWidget *widget = item->widget();
                widget->setParent(nullptr);
                widget->deleteLater();
            }
            delete item;
        }

        // 删除所有卡片对象
        QList<PlaceCard*> cards = m_placeCards.values();
        m_placeCards.clear();

        for (PlaceCard *card : cards) {
            if (card) {
                // 先断开所有连接
                card->disconnect();
                card->setParent(nullptr);
                card->deleteLater();
            }
        }

    } catch (const std::exception &e) {
        qDebug() << "清理卡片时发生异常:" << e.what();
    } catch (...) {
        qDebug() << "清理卡片时发生未知异常";
    }

    int comboCount = m_placeComboApply->count();
    qDebug() << "下拉框中的场所数量:" << comboCount;

    // 如果下拉框为空，显示提示信息
    if (comboCount <= 0) {
        qDebug() << "没有场所数据，显示空状态";
        QLabel *emptyLabel = new QLabel("暂无场所信息", m_placeCardsContainer);
        emptyLabel->setObjectName("emptyPlaceLabel");
        emptyLabel->setAlignment(Qt::AlignCenter);
        emptyLabel->setStyleSheet("color: #999; font-size: 14px; padding: 40px;");
        m_placeCardsLayout->addWidget(emptyLabel, 0, 0, 1, 3, Qt::AlignCenter);
        return;
    }

    qDebug() << "开始创建场所卡片...";

    // 计算每行卡片数量
    int containerWidth = m_placeCardsContainer->width();
    if (containerWidth <= 0) {
        containerWidth = 800; // 默认宽度
    }
    int cardsPerRow = qMax(1, containerWidth / 220); // 每张卡片约220px宽

    int row = 0;
    int col = 0;
    int createdCards = 0;

    try {
        for (int i = 0; i < comboCount; ++i) {
            QString placeId = m_placeComboApply->itemData(i).toString();
            QString placeName = m_placeComboApply->itemText(i);

            if (placeId.isEmpty() || placeName.isEmpty()) {
                qDebug() << "跳过空的场所数据，索引:" << i;
                continue;
            }

            QVariant equipmentData = m_placeComboApply->itemData(i, Qt::UserRole + 1);
            QStringList equipmentList;

            if (equipmentData.isValid() && equipmentData.canConvert<QStringList>()) {
                equipmentList = equipmentData.toStringList();
            }

            qDebug() << "创建卡片 - 场所:" << placeName << "ID:" << placeId
                     << "设备数量:" << equipmentList.size();

            // 创建设备列表字符串显示
            QStringList displayEquipmentList;
            for (const QString &equipment : equipmentList) {
                if (!equipment.trimmed().isEmpty()) {
                    displayEquipmentList.append(equipment.trimmed());
                }
            }

            PlaceCard *card = new PlaceCard(placeId, placeName, displayEquipmentList, m_placeCardsContainer);
            if (!card) {
                qDebug() << "创建卡片失败，场所:" << placeName;
                continue;
            }

            // 连接信号槽
            connect(card, &PlaceCard::cardClicked, this, &ReservationWidget::onPlaceCardClicked);

            m_placeCards[placeId] = card;
            m_placeCardsLayout->addWidget(card, row, col);

            col++;
            if (col >= cardsPerRow) {
                col = 0;
                row++;
            }

            createdCards++;
        }
    } catch (const std::exception &e) {
        qDebug() << "创建卡片时发生异常:" << e.what();
    } catch (...) {
        qDebug() << "创建卡片时发生未知异常";
    }

    qDebug() << "成功创建卡片数量:" << createdCards;

    // 如果没有成功创建卡片，显示提示信息
    if (createdCards == 0) {
        QLabel *emptyLabel = new QLabel("暂无场所信息", m_placeCardsContainer);
        emptyLabel->setObjectName("emptyPlaceLabel");
        emptyLabel->setAlignment(Qt::AlignCenter);
        emptyLabel->setStyleSheet("color: #999; font-size: 14px; padding: 40px;");
        m_placeCardsLayout->addWidget(emptyLabel, 0, 0, 1, cardsPerRow, Qt::AlignCenter);
    } else {
        // 默认选择第一个卡片
        QString firstPlaceId = m_placeCards.keys().first();
        qDebug() << "默认选择第一个场所:" << firstPlaceId;

        // 延迟选中，确保UI已更新
        QTimer::singleShot(100, this, [this, firstPlaceId]() {
            if (m_placeCards.contains(firstPlaceId)) {
                onPlaceCardClicked(firstPlaceId);
            }
        });
    }

    qDebug() << "场所卡片更新完成";
}


