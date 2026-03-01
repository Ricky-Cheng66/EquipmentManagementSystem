#include "reservationwidget.h"
#include "reservationcard.h"
#include "reservationfiltertoolbar.h"
#include "placecard.h"
#include "placequerycard.h"

#include <QMessageBox>
#include <QPushButton>
#include <QDebug>
#include <QTimer>
#include <QElapsedTimer>
#include <QScrollArea>
#include <QGroupBox>
#include <QDateEdit>
#include <QTimeEdit>
#include <QLineEdit>
#include <QCheckBox>
#include <QTableWidget>
#include <QStackedWidget>
#include <QLabel>
#include <QApplication>
#include <QHeaderView>

ReservationWidget::ReservationWidget(QWidget *parent)
    : QWidget(parent)
    , m_tabWidget(new QTabWidget(this))
    , m_placeComboApply(nullptr)
    , m_placeComboQuery(nullptr)
    , m_placeCardsContainer(nullptr)
    , m_placeCardsLayout(nullptr)
    , m_selectedPlaceId("")
    , m_selectedEquipmentText(nullptr)
    , m_queryFilterBar(nullptr)
    , m_queryFilterBarDetail(nullptr)
    , m_queryViewStack(nullptr)
    , m_placeListPage(nullptr)
    , m_placeDetailPage(nullptr)
    , m_placeListLayout(nullptr)
    , m_placeDetailLayout(nullptr)
    , m_placeDetailNameLabel(nullptr)
    , m_placeDetailStatsLabel(nullptr)
    , m_queryScrollArea(nullptr)
    , m_queryCardContainer(nullptr)
    , m_queryCardLayout(nullptr)
    , m_queryButton(nullptr)
    , m_queryResultTable(nullptr)
    , m_currentPlaceId("")
    , m_currentPlaceName("")
    , m_approveFilterBar(nullptr)          // 新增：审批页筛选工具栏
    , m_approveDetailFilterBar(nullptr)    // 新增：审批页详情筛选工具栏
    , m_approveViewStack(nullptr)          // 新增：审批页堆栈
    , m_approvePlaceListPage(nullptr)      // 新增：审批场所列表页面
    , m_approveDetailPage(nullptr)         // 新增：审批详情页面
    , m_approvePlaceListLayout(nullptr)    // 新增：审批场所列表布局
    , m_approveDetailLayout(nullptr)       // 新增：审批详情布局
    , m_approvePlaceListContainer(nullptr) // 新增：审批场所列表容器
    , m_approvePendingCountLabel(nullptr)  // 新增：待审批数量标签
    , m_approvePlaceNameLabel(nullptr)     // 新增：审批场所名称标签
    , m_approvePlaceStatsLabel(nullptr)    // 新增：审批场所统计标签
    , m_currentApprovePlaceId("")          // 新增：当前审批场所ID
    , m_currentApprovePlaceName("")        // 新增：当前审批场所名称
    , m_selectAllCheck(nullptr)            // 新增：全选复选框
    , m_batchApproveButton(nullptr)        // 新增：批量批准按钮
    , m_batchRejectButton(nullptr)         // 新增：批量拒绝按钮
    , m_isRefreshingQueryView(false)
    , m_isRefreshingApproveView(false)     // 新增：审批视图刷新状态
    , m_approvePlaceListRefreshTimer(nullptr) // 新增：审批场所列表刷新定时器
{
    qDebug() << "ReservationWidget 构造函数开始";

    setWindowTitle("预约管理");
    resize(800, 600);

    // 创建三个标签页
    setupApplyTab();
    setupQueryTab();

    // 注意：setupApproveTab() 会在用户切换到审批页时延迟创建
    // 但我们仍然需要初始化基本控件
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

    // 创建审批场所列表刷新定时器
    m_approvePlaceListRefreshTimer = new QTimer(this);
    m_approvePlaceListRefreshTimer->setSingleShot(true);
    connect(m_approvePlaceListRefreshTimer, &QTimer::timeout, this, &ReservationWidget::refreshApprovePlaceListView);

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

    if (m_approvePlaceListRefreshTimer) {
        m_approvePlaceListRefreshTimer->stop();
        delete m_approvePlaceListRefreshTimer;
        m_approvePlaceListRefreshTimer = nullptr;
    }

    // 清理审批页卡片列表
    for (PlaceQueryCard *card : m_approvePlaceCards) {
        if (card) {
            card->deleteLater();
        }
    }
    m_approvePlaceCards.clear();

    // 清理所有审批卡片
    for (ReservationCard *card : m_allApproveCards) {
        if (card) {
            card->deleteLater();
        }
    }
    m_allApproveCards.clear();

    // 清理当前显示的审批卡片
    m_approveCards.clear();
    m_approveCardMap.clear();

    // 清理场所查询卡片
    for (PlaceQueryCard *card : m_placeQueryCards) {
        if (card) {
            card->deleteLater();
        }
    }
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
    qDebug() << "设置审批页（复用查询界面架构）";

    // 安全检查：如果已经创建，先清理
    if (m_approveViewStack) {
        qDebug() << "审批页已存在，清理重建";
        m_approveViewStack->deleteLater();
        m_approveViewStack = nullptr;
    }

    QWidget *approveTab = new QWidget(this);
    QVBoxLayout *mainLayout = new QVBoxLayout(approveTab);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    try {
        // ===== 创建审批视图堆栈 =====
        m_approveViewStack = new QStackedWidget(approveTab);

        // ==== 第一级：待审批场所列表页面 ====
        setupApprovePlaceListPage();

        // ==== 第二级：场所待审批详情页面 ====
        setupApproveDetailPage();

        // 默认显示场所列表页面
        if (m_approveViewStack->count() > 0) {
            m_approveViewStack->setCurrentIndex(0);
        }

        // ==== 底部批量操作栏 ====
        QWidget *batchWidget = new QWidget(approveTab);
        batchWidget->setStyleSheet("background-color: #f5f6fa; border-top: 1px solid #e0e0e0;");
        QHBoxLayout *batchLayout = new QHBoxLayout(batchWidget);
        batchLayout->setContentsMargins(10, 10, 10, 10);

        m_selectAllCheck = new QCheckBox("全选", batchWidget);
        m_batchApproveButton = new QPushButton("✅ 批量批准", batchWidget);
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

        m_batchRejectButton = new QPushButton("❌ 批量拒绝", batchWidget);
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

        // 添加到主布局
        mainLayout->addWidget(m_approveViewStack);
        mainLayout->addWidget(batchWidget);

        m_tabWidget->addTab(approveTab, "📋 预约审批");

        // 连接信号 - 使用新的槽函数
        if (m_selectAllCheck) {
            connect(m_selectAllCheck, &QCheckBox::stateChanged,
                    this, &ReservationWidget::onSelectAllChanged);
        }

        if (m_batchApproveButton) {
            connect(m_batchApproveButton, &QPushButton::clicked,
                    this, &ReservationWidget::onBatchApprove);
        }

        if (m_batchRejectButton) {
            connect(m_batchRejectButton, &QPushButton::clicked,
                    this, &ReservationWidget::onBatchReject);
        }

        qDebug() << "审批页设置完成，堆栈页面数:" << m_approveViewStack->count();

    } catch (const std::exception &e) {
        qCritical() << "设置审批页时异常:" << e.what();
        QMessageBox::warning(this, "错误", QString("创建审批页时发生错误: %1").arg(e.what()));
    } catch (...) {
        qCritical() << "设置审批页时未知异常";
        QMessageBox::warning(this, "错误", "创建审批页时发生未知错误");
    }
}

void ReservationWidget::setupApprovePlaceListPage()
{
    qDebug() << "设置待审批场所列表页面";

    if (!m_approveViewStack) {
        qCritical() << "错误: m_approveViewStack 未初始化";
        return;
    }

    try {
        m_approvePlaceListPage = new QWidget(m_approveViewStack);
        QVBoxLayout *mainLayout = new QVBoxLayout(m_approvePlaceListPage);
        mainLayout->setContentsMargins(0, 0, 0, 0);
        mainLayout->setSpacing(0);

        // 页面标题
        QWidget *titleWidget = new QWidget(m_approvePlaceListPage);
        titleWidget->setObjectName("approveTitleWidget");
        titleWidget->setStyleSheet(
            "QWidget#approveTitleWidget {"
            "    background: qlineargradient(x1:0, y1:0, x2:1, y2:0,"
            "        stop:0 #ff6b6b, stop:1 #ee5a52);"
            "    padding: 15px;"
            "    border-bottom: 2px solid #c23616;"
            "}"
            );

        QHBoxLayout *titleLayout = new QHBoxLayout(titleWidget);
        titleLayout->setContentsMargins(10, 5, 10, 5);

        QLabel *titleLabel = new QLabel("📋 待审批预约", titleWidget);
        titleLabel->setStyleSheet(
            "QLabel {"
            "    color: white;"
            "    font-size: 18px;"
            "    font-weight: bold;"
            "}"
            );

        m_approvePendingCountLabel = new QLabel("待处理: 0", titleWidget);
        m_approvePendingCountLabel->setStyleSheet(
            "QLabel {"
            "    color: white;"
            "    font-size: 14px;"
            "    background-color: rgba(0,0,0,0.2);"
            "    padding: 4px 12px;"
            "    border-radius: 12px;"
            "}"
            );

        titleLayout->addWidget(titleLabel);
        titleLayout->addStretch();
        titleLayout->addWidget(m_approvePendingCountLabel);

        mainLayout->addWidget(titleWidget);

        // 创建筛选工具栏
        m_approveFilterBar = new ReservationFilterToolBar(m_approvePlaceListPage);
        m_approveFilterBar->setMode(true, ""); // 设置为场所列表模式
        mainLayout->addWidget(m_approveFilterBar);

        // 创建滚动区域
        QScrollArea *placeScrollArea = new QScrollArea(m_approvePlaceListPage);
        placeScrollArea->setWidgetResizable(true);
        placeScrollArea->setFrameShape(QFrame::NoFrame);

        // 创建场所列表容器
        m_approvePlaceListContainer = new QWidget();
        m_approvePlaceListContainer->setObjectName("approvePlaceListContainer");
        m_approvePlaceListLayout = new QGridLayout(m_approvePlaceListContainer);
        m_approvePlaceListLayout->setContentsMargins(20, 20, 20, 20);
        m_approvePlaceListLayout->setHorizontalSpacing(20);
        m_approvePlaceListLayout->setVerticalSpacing(20);
        m_approvePlaceListLayout->setAlignment(Qt::AlignTop);

        placeScrollArea->setWidget(m_approvePlaceListContainer);
        mainLayout->addWidget(placeScrollArea);

        m_approveViewStack->addWidget(m_approvePlaceListPage);

        // 连接信号
        if (m_approveFilterBar) {
            // 使用单次连接，避免重复触发
            static bool filterConnected = false;
            if (!filterConnected) {
                connect(m_approveFilterBar, &ReservationFilterToolBar::filterChanged,
                        this, &ReservationWidget::onApprovePlaceFilterChanged);
                connect(m_approveFilterBar, &ReservationFilterToolBar::refreshRequested,
                        this, &ReservationWidget::onApproveListRefreshRequested);
                filterConnected = true;
                qDebug() << "审批筛选工具栏信号已连接";
            }
        }

        qDebug() << "待审批场所列表页面设置完成";

    } catch (const std::exception &e) {
        qCritical() << "设置待审批场所列表页面时异常:" << e.what();
    } catch (...) {
        qCritical() << "设置待审批场所列表页面时未知异常";
    }
}

void ReservationWidget::setupApproveDetailPage()
{
    qDebug() << "设置场所待审批详情页面";

    if (!m_approveViewStack) {
        qCritical() << "错误: m_approveViewStack 未初始化";
        return;
    }

    try {
        m_approveDetailPage = new QWidget(m_approveViewStack);
        m_approveDetailLayout = new QVBoxLayout(m_approveDetailPage);
        m_approveDetailLayout->setContentsMargins(0, 0, 0, 0);
        m_approveDetailLayout->setSpacing(0);

        // 创建筛选工具栏
        m_approveDetailFilterBar = new ReservationFilterToolBar(m_approveDetailPage);
        m_approveDetailFilterBar->setMode(false); // 设置为详情模式

        // 设置状态下拉框默认为"待审批"
        if (m_approveDetailFilterBar->findChild<QComboBox*>("statusCombo")) {
            QComboBox *statusCombo = m_approveDetailFilterBar->findChild<QComboBox*>("statusCombo");
            if (statusCombo) {
                for (int i = 0; i < statusCombo->count(); i++) {
                    if (statusCombo->itemData(i).toString() == "pending") {
                        statusCombo->setCurrentIndex(i);
                        break;
                    }
                }
            }
        }

        m_approveDetailLayout->addWidget(m_approveDetailFilterBar);

        // 场所信息概览区域
        QWidget *placeOverviewWidget = new QWidget(m_approveDetailPage);
        placeOverviewWidget->setObjectName("approvePlaceOverviewWidget");
        placeOverviewWidget->setStyleSheet(
            "QWidget#approvePlaceOverviewWidget {"
            "    background: linear-gradient(to right, #ffeaa7, #fab1a0);"
            "    border-bottom: 1px solid #e0e0e0;"
            "    padding: 15px;"
            "}"
            );
        QVBoxLayout *overviewLayout = new QVBoxLayout(placeOverviewWidget);
        overviewLayout->setContentsMargins(10, 5, 10, 5);

        m_approvePlaceNameLabel = new QLabel("未选择场所", placeOverviewWidget);
        m_approvePlaceNameLabel->setStyleSheet(
            "QLabel {"
            "    font-size: 18px;"
            "    font-weight: bold;"
            "    color: #2c3e50;"
            "}"
            );

        m_approvePlaceStatsLabel = new QLabel("", placeOverviewWidget);
        m_approvePlaceStatsLabel->setStyleSheet(
            "QLabel {"
            "    font-size: 13px;"
            "    color: #666;"
            "    padding: 2px 0;"
            "}"
            );

        overviewLayout->addWidget(m_approvePlaceNameLabel);
        overviewLayout->addWidget(m_approvePlaceStatsLabel);

        m_approveDetailLayout->addWidget(placeOverviewWidget);

        // 创建审批卡片容器 - 这是关键修复点
        m_approveCardContainer = new QWidget();
        m_approveCardContainer->setObjectName("approveCardContainer");
        m_approveCardLayout = new QVBoxLayout(m_approveCardContainer);
        m_approveCardLayout->setContentsMargins(20, 20, 20, 20);
        m_approveCardLayout->setSpacing(20);
        m_approveCardLayout->addStretch(); // 添加拉伸因子

        // 创建待审批预约记录滚动区域
        QScrollArea *reservationScrollArea = new QScrollArea(m_approveDetailPage);
        reservationScrollArea->setWidgetResizable(true);
        reservationScrollArea->setFrameShape(QFrame::NoFrame);
        reservationScrollArea->setWidget(m_approveCardContainer);

        m_approveDetailLayout->addWidget(reservationScrollArea, 1); // 设置为可拉伸

        m_approveViewStack->addWidget(m_approveDetailPage);

        // 连接信号
        if (m_approveDetailFilterBar) {
            connect(m_approveDetailFilterBar, &ReservationFilterToolBar::filterChanged,
                    this, &ReservationWidget::onApproveDetailFilterChanged);
            connect(m_approveDetailFilterBar, &ReservationFilterToolBar::refreshRequested,
                    this, &ReservationWidget::onApproveDetailRefreshRequested);
            connect(m_approveDetailFilterBar, &ReservationFilterToolBar::backToPlaceListRequested,
                    this, &ReservationWidget::onApproveBackToPlaceList);
        }

        qDebug() << "场所待审批详情页面设置完成，卡片容器已创建";

    } catch (const std::exception &e) {
        qCritical() << "设置场所待审批详情页面时异常:" << e.what();
    } catch (...) {
        qCritical() << "设置场所待审批详情页面时未知异常";
    }
}

// 审批场所卡片点击
void ReservationWidget::onApprovePlaceCardClicked(const QString &placeId)
{
    qDebug() << "审批场所卡片点击:" << placeId;

    m_currentApprovePlaceId = placeId;
    m_currentApprovePlaceName = getPlaceNameById(placeId);

    // 切换到场所详情页面
    if (m_approveViewStack && m_approveViewStack->count() > 1) {
        m_approveViewStack->setCurrentIndex(1);
        qDebug() << "已切换到审批详情页面";
    } else {
        qCritical() << "错误: 审批详情页面未创建或不可用";
        return;
    }

    // 更新场所信息概览
    if (m_approvePlaceNameLabel) {
        m_approvePlaceNameLabel->setText("🏢 " + m_currentApprovePlaceName);
    }

    if (m_approvePlaceStatsLabel) {
        QStringList equipmentList = getEquipmentListForPlace(placeId);
        int pendingCount = m_approvePlacePendingCount.value(placeId, 0);
        QString equipmentText = equipmentList.isEmpty() ? "无设备" : equipmentList.join(", ");
        m_approvePlaceStatsLabel->setText(
            QString("🔧 设备: %1 | 📅 待审批预约: %2 个").arg(equipmentText).arg(pendingCount)
            );
    }

    // 延迟刷新该场所的待审批预约，确保UI已更新
    QTimer::singleShot(50, this, [this, placeId]() {
        qDebug() << "延迟刷新审批详情视图，场所:" << placeId;
        refreshApproveDetailView();
    });
}


// 刷新审批场所列表视图
void ReservationWidget::refreshApprovePlaceListView()
{
    qDebug() << "刷新审批场所列表视图 - 开始";

    // 检查审批页是否已初始化
    if (!isApprovePageInitialized()) {
        qWarning() << "审批页未完全初始化，跳过刷新";
        return;
    }

    // 安全检查：确保页面存在且未销毁
    if (!m_approvePlaceListPage || !m_approvePlaceListPage->isVisible()) {
        qDebug() << "审批场所列表页面不可用，跳过刷新";
        return;
    }

    if (!m_approvePlaceListLayout) {
        qCritical() << "错误: m_approvePlaceListLayout 未初始化";
        return;
    }
    // 在创建卡片前重新计算待审批数量
    recalculatePendingCounts();

    // 使用 QApplication::processEvents() 确保UI更新
    QApplication::processEvents();

    try {
        // 1. 完全清空现有布局
        QLayoutItem *child;
        while ((child = m_approvePlaceListLayout->takeAt(0)) != nullptr) {
            if (child->widget()) {
                child->widget()->hide();
                child->widget()->setParent(nullptr);
                child->widget()->deleteLater();
            }
            delete child;
        }

        // 2. 删除所有场所卡片对象
        for (PlaceQueryCard *card : m_approvePlaceCards) {
            if (card) {
                card->disconnect();
                card->setParent(nullptr);
                card->deleteLater();
            }
        }
        m_approvePlaceCards.clear();

        // 3. 如果没有待审批数据，显示提示
        if (m_approvePlacePendingCount.isEmpty()) {
            qDebug() << "没有待审批数据，显示空状态";

            QLabel *emptyLabel = new QLabel("✅ 暂无待审批预约", m_approvePlaceListPage);
            emptyLabel->setObjectName("emptyApproveLabel");
            emptyLabel->setAlignment(Qt::AlignCenter);
            emptyLabel->setStyleSheet(
                "QLabel {"
                "    color: #27ae60;"
                "    font-size: 16px;"
                "    padding: 60px;"
                "    background-color: #f8f9fa;"
                "    border-radius: 8px;"
                "}"
                );
            m_approvePlaceListLayout->addWidget(emptyLabel, 0, 0, 1, 1);

            // 更新待审批总数
            if (m_approvePendingCountLabel) {
                m_approvePendingCountLabel->setText("待处理: 0");
            }
            return;
        }

        // 4. 获取筛选条件
        QString selectedPlaceType = "all";
        QString searchText = "";

        if (m_approveFilterBar) {
            // 获取筛选工具栏的类型选择（可能是中文或英文代码）
            selectedPlaceType = m_approveFilterBar->selectedPlaceType();
            searchText = m_approveFilterBar->searchText();
        }

        qDebug() << "筛选条件 - 类型:" << selectedPlaceType << "搜索:" << searchText;

        // 5. 计算每行卡片数量
        int containerWidth = m_approvePlaceListContainer ? m_approvePlaceListContainer->width() : 800;
        if (containerWidth <= 0) containerWidth = 800;
        int cardsPerRow = qMax(1, containerWidth / 300);

        int row = 0, col = 0, visibleCards = 0;
        int totalPendingCount = 0;

        // 6. 创建场所卡片
        QList<QString> placeIds = m_approvePlacePendingCount.keys();

        for (const QString &placeId : placeIds) {
            int pendingCount = m_approvePlacePendingCount.value(placeId, 0);

            if (pendingCount <= 0) continue; // 只显示有待审批预约的场所

            totalPendingCount += pendingCount;

            QString placeName = getPlaceNameById(placeId);
            if (placeName.isEmpty()) {
                qWarning() << "场所名称为空，跳过场所ID:" << placeId;
                continue;
            }

            QStringList equipmentList = getEquipmentListForPlace(placeId);

            // 使用检测到的场所类型（英文代码）
            QString placeType = detectPlaceType(placeName);
            qDebug() << "场所:" << placeName << "检测到的类型:" << placeType;

            // 应用筛选条件
            bool shouldShow = true;

            // 类型筛选 - 修复：处理 "all" 和具体类型
            if (selectedPlaceType != "all") {
                QString placeTypeChinese = getPlaceTypeDisplayName(placeType);
                qDebug() << "场所中文类型:" << placeTypeChinese << "筛选条件:" << selectedPlaceType;

                if (placeTypeChinese != selectedPlaceType) {
                    shouldShow = false;
                    qDebug() << "类型不匹配，跳过场所:" << placeName;
                }
            }

            // 搜索筛选
            if (shouldShow && !searchText.isEmpty()) {
                if (!placeName.contains(searchText, Qt::CaseInsensitive) &&
                    !placeId.contains(searchText, Qt::CaseInsensitive)) {
                    shouldShow = false;
                }
            }

            if (shouldShow) {
                try {
                    PlaceQueryCard *card = new PlaceQueryCard(
                        placeId, placeName, equipmentList,
                        pendingCount, false, m_approvePlaceListPage);

                    if (!card) {
                        qWarning() << "创建卡片失败";
                        continue;
                    }

                    // 设置特殊样式表示待审批
                    card->setStyleSheet(
                        "QWidget#placeQueryCard {"
                        "    border: 2px solid #e74c3c;"  // 红色边框表示待审批
                        "    background-color: white;"
                        "    border-radius: 10px;"
                        "}"
                        );

                    // 连接信号
                    connect(card, &PlaceQueryCard::cardClicked,
                            this, &ReservationWidget::onApprovePlaceCardClicked,
                            Qt::QueuedConnection);

                    m_approvePlaceCards.append(card);
                    m_approvePlaceListLayout->addWidget(card, row, col);
                    visibleCards++;

                    col++;
                    if (col >= cardsPerRow) {
                        col = 0;
                        row++;
                    }

                    qDebug() << "显示场所卡片:" << placeName << "类型:" << placeType;
                } catch (const std::exception &e) {
                    qCritical() << "创建审批场所卡片时异常:" << e.what();
                } catch (...) {
                    qCritical() << "创建审批场所卡片时未知异常";
                }
            }
        }

        // 7. 更新待审批总数
        if (m_approvePendingCountLabel) {
            m_approvePendingCountLabel->setText(QString("待处理: %1").arg(totalPendingCount));
        }

        // 8. 如果没有可见卡片，显示提示
        if (visibleCards == 0) {
            QLabel *noMatchLabel = new QLabel(
                "🔍 没有符合条件的待审批场所",
                m_approvePlaceListPage);
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
            m_approvePlaceListLayout->addWidget(noMatchLabel, 0, 0, 1, cardsPerRow, Qt::AlignCenter);
        } else {
            qDebug() << "成功创建" << visibleCards << "个审批场所卡片";
        }

        // 强制更新布局
        m_approvePlaceListContainer->updateGeometry();
        m_approvePlaceListPage->update();

        qDebug() << "刷新审批场所列表视图 - 完成，显示" << visibleCards << "个场所有待审批预约";

    } catch (const std::exception &e) {
        qCritical() << "刷新审批场所列表视图时异常:" << e.what();
        QMessageBox::warning(nullptr, "错误", QString("刷新审批列表时发生异常: %1").arg(e.what()));
    } catch (...) {
        qCritical() << "刷新审批场所列表视图时未知异常";
        QMessageBox::warning(nullptr, "错误", "刷新审批列表时发生未知异常");
    }
}

// 刷新审批详情视图
void ReservationWidget::refreshApproveDetailView()
{
    qDebug() << "刷新审批详情视图，场所:" << m_currentApprovePlaceId;

    // 安全检查
    if (m_currentApprovePlaceId.isEmpty()) {
        qDebug() << "当前审批场所ID为空";
        return;
    }

    if (!m_approveCardContainer || !m_approveCardLayout) {
        qCritical() << "错误: 审批卡片容器或布局未初始化";
        return;
    }

    // 清理当前显示的卡片 - 修改：只隐藏，不删除
    QLayoutItem* child;
    while ((child = m_approveCardLayout->takeAt(0)) != nullptr) {
        if (child->widget()) {
            child->widget()->setVisible(false);
            child->widget()->setParent(nullptr);
            // 不删除widget，只是从布局中移除
        }
        delete child;
    }

    // 清除当前显示的卡片列表
    m_approveCards.clear();
    m_approveCardMap.clear();

    // 如果没有待审批预约数据，显示提示
    if (m_allApproveCards.isEmpty()) {
        qDebug() << "没有待审批预约数据";

        QLabel *emptyLabel = new QLabel("✅ 暂无待审批预约", m_approveCardContainer);
        emptyLabel->setAlignment(Qt::AlignCenter);
        emptyLabel->setStyleSheet(
            "QLabel {"
            "    color: #27ae60;"
            "    font-size: 16px;"
            "    padding: 60px;"
            "    background-color: #f8f9fa;"
            "    border-radius: 8px;"
            "}"
            );
        m_approveCardLayout->addWidget(emptyLabel);
        m_approveCardLayout->addStretch();
        return;
    }

    // 获取筛选条件
    QString searchText = "";
    if (m_approveDetailFilterBar) {
        searchText = m_approveDetailFilterBar->searchText();
    }

    qDebug() << "搜索文本:" << searchText;
    qDebug() << "总审批卡片数:" << m_allApproveCards.size();

    // 创建网格布局容器
    QWidget *gridContainer = new QWidget(m_approveCardContainer);
    gridContainer->setObjectName("approveDetailGridContainer");
    QGridLayout *gridLayout = new QGridLayout(gridContainer);
    gridLayout->setContentsMargins(0, 0, 0, 0);
    gridLayout->setHorizontalSpacing(20);
    gridLayout->setVerticalSpacing(20);
    gridLayout->setAlignment(Qt::AlignTop);

    // 计算每行卡片数量
    int containerWidth = m_approveCardContainer->width();
    if (containerWidth <= 0) containerWidth = 800;
    int cardsPerRow = qMax(1, containerWidth / 340);

    qDebug() << "每行卡片数:" << cardsPerRow;

    int row = 0, col = 0, visibleCards = 0;

    // 筛选并显示该场所的待审批预约
    for (ReservationCard *card : m_allApproveCards) {
        if (!card) {
            qDebug() << "卡片指针为空，跳过";
            continue;
        }

        // 安全检查：检查卡片数据是否有效
        if (card->reservationId().isEmpty() || card->reservationId() == "\u0000") {
            qWarning() << "卡片预约ID无效或为空，跳过";
            continue;
        }

        // 只显示当前场所且状态为待审批的预约
        QString cardPlaceId = card->placeId();
        QString cardStatus = card->status().toLower();

        qDebug() << "检查卡片 - 预约ID:" << card->reservationId()
                 << "卡片场所ID:" << cardPlaceId
                 << "目标场所ID:" << m_currentApprovePlaceId
                 << "卡片状态:" << cardStatus;

        if (cardPlaceId != m_currentApprovePlaceId) {
            continue;
        }

        if (!cardStatus.contains("pending")) {
            qDebug() << "卡片状态不是待审批，跳过";
            continue;
        }

        // 搜索筛选
        bool shouldShow = true;
        if (!searchText.isEmpty()) {
            QString searchLower = searchText.toLower();
            QString cardText = card->reservationId() + "|" +
                               card->userId() + "|" +
                               card->purpose() + "|" +
                               card->equipmentList();

            if (!cardText.toLower().contains(searchLower)) {
                shouldShow = false;
            }
        }

        if (shouldShow) {
            qDebug() << "显示卡片 - 预约ID:" << card->reservationId() << "场所ID:" << cardPlaceId;

            // 重新设置卡片的父控件
            card->setParent(gridContainer);
            card->setVisible(true);
            card->update(); // 强制更新卡片显示

            // 确保卡片有正确的大小
            card->setFixedSize(320, 220);

            // 添加到网格布局
            gridLayout->addWidget(card, row, col);
            visibleCards++;

            // 将卡片添加到当前显示的列表中
            m_approveCards.append(card);
            m_approveCardMap[card->reservationId()] = card;

            // 确保信号连接 - 使用Qt::UniqueConnection避免重复连接
            connect(card, &ReservationCard::statusActionRequested,
                    this, &ReservationWidget::onStatusActionRequested,
                    Qt::UniqueConnection);

            col++;
            if (col >= cardsPerRow) {
                col = 0;
                row++;
            }
        }
    }

    qDebug() << "筛选完成，可见卡片数:" << visibleCards;

    // 如果没有待审批预约，显示提示
    if (visibleCards == 0) {
        delete gridContainer;

        QString message = searchText.isEmpty()
                              ? "✅ 该场所暂无待审批预约"
                              : QString("🔍 没有符合条件的待审批预约\n搜索: %1").arg(searchText);

        QLabel *noMatchLabel = new QLabel(message, m_approveCardContainer);
        noMatchLabel->setAlignment(Qt::AlignCenter);
        noMatchLabel->setStyleSheet(
            "QLabel {"
            "    color: #27ae60;"
            "    font-size: 16px;"
            "    padding: 60px;"
            "    background-color: #f8f9fa;"
            "    border-radius: 8px;"
            "}"
            );
        m_approveCardLayout->addWidget(noMatchLabel);
        m_approveCardLayout->addStretch();

        qDebug() << "显示空状态提示";
    } else {
        m_approveCardLayout->addWidget(gridContainer);

        // 添加统计标签
        QLabel *resultLabel = new QLabel(
            QString("共找到 %1 个待审批预约").arg(visibleCards),
            gridContainer);
        resultLabel->setStyleSheet(
            "QLabel {"
            "    color: #e74c3c;"
            "    font-size: 12px;"
            "    font-weight: bold;"
            "    padding: 5px 15px;"
            "    background-color: #ffebee;"
            "    border-radius: 15px;"
            "    margin: 5px;"
            "}"
            );
        gridLayout->addWidget(resultLabel, row + 1, 0, 1, cardsPerRow, Qt::AlignCenter);

        qDebug() << "已添加" << visibleCards << "个待审批预约卡片";
    }

    // 强制更新布局
    m_approveCardContainer->updateGeometry();
    if (m_approveDetailPage) {
        m_approveDetailPage->update();
    }
}

void ReservationWidget::refreshCurrentApproveView()
{
    qDebug() << "刷新当前审批视图";

    if (!m_approveViewStack) {
        return;
    }

    int currentIndex = m_approveViewStack->currentIndex();
    qDebug() << "当前审批视图索引:" << currentIndex;

    if (currentIndex == 0) {
        // 当前在场所列表页面，刷新列表
        refreshApprovePlaceListView();
    } else if (currentIndex == 1) {
        // 当前在详情页面，刷新详情
        if (!m_currentApprovePlaceId.isEmpty()) {
            refreshApproveDetailView();
        }
    }

    // 重新计算待审批数量
    recalculatePendingCounts();
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
            this, &ReservationWidget::onQueryListRefreshRequested);
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
            this, &ReservationWidget::onQueryDetailRefreshRequested);  // 改为第二级刷新
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
    QString selectedPlace = m_queryFilterBar ? m_queryFilterBar->selectedPlace() : "all";
    QString selectedStatus = m_queryFilterBar ? m_queryFilterBar->selectedStatus() : "all";
    QString selectedDateRange = m_queryFilterBar ? m_queryFilterBar->selectedDate() : "all";
    QString searchText = m_queryFilterBar ? m_queryFilterBar->searchText() : "";

    QDate startDate, endDate;
    if (selectedDateRange != "all") {
        startDate = m_queryFilterBar->startDate();
        endDate = m_queryFilterBar->endDate();
    }

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

        // 场所筛选（新增）
        if (shouldShow && selectedPlace != "all") {
            // 检查是否是完整的场所ID匹配或名称匹配
            bool placeMatch = false;

            // 直接ID匹配
            if (placeId == selectedPlace) {
                placeMatch = true;
            }
            // 检查场所名称是否包含筛选文本
            else if (placeName.contains(selectedPlace, Qt::CaseInsensitive)) {
                placeMatch = true;
            }

            if (!placeMatch) {
                shouldShow = false;
            }
        }

        // 状态筛选（新增）- 需要检查该场所的预约记录状态
        if (shouldShow && selectedStatus != "all") {
            bool hasMatchingStatus = false;

            // 遍历所有预约记录，检查该场所是否有符合条件的预约
            for (ReservationCard *card : m_queryCards) {
                if (card && card->placeId() == placeId) {
                    QString cardStatus = card->status().toLower();
                    QStringList statusMap = getStatusMap(selectedStatus);

                    for (const QString &status : statusMap) {
                        if (cardStatus.contains(status, Qt::CaseInsensitive)) {
                            hasMatchingStatus = true;
                            break;
                        }
                    }

                    if (hasMatchingStatus) break;
                }
            }

            if (!hasMatchingStatus) {
                shouldShow = false;
            }
        }

        // 日期筛选（新增）
        if (shouldShow && selectedDateRange != "all" && startDate.isValid() && endDate.isValid()) {
            bool hasMatchingDate = false;

            // 遍历所有预约记录，检查该场所是否有在日期范围内的预约
            for (ReservationCard *card : m_queryCards) {
                if (card && card->placeId() == placeId) {
                    QDate cardStartDate = card->getStartDate();
                    QDate cardEndDate = card->getEndDate();

                    // 检查卡片的开始或结束日期是否在筛选范围内
                    bool dateInRange = (cardStartDate >= startDate && cardStartDate <= endDate) ||
                                       (cardEndDate >= startDate && cardEndDate <= endDate) ||
                                       (cardStartDate <= startDate && cardEndDate >= endDate);

                    if (dateInRange) {
                        hasMatchingDate = true;
                        break;
                    }
                }
            }

            if (!hasMatchingDate) {
                shouldShow = false;
            }
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
                                                      reservationCount, true, m_placeListPage);

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
                    startTime, endTime, status, equipmentText,
                    false,  // 明确指定非审批模式
                    nullptr
                    );

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
    QMutexLocker locker(&m_refreshMutex);  // 使用互斥锁保护

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

    // 检查是否正在刷新，避免重复操作
    if (m_isRefreshingQueryView) {
        qDebug() << "正在刷新中，跳过此次刷新";
        return;
    }

    m_isRefreshingQueryView = true;

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
    m_isRefreshingQueryView = false;
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

    QString placeId = "";

    // 从审批卡片中获取场所ID
    if (m_approveCardMap.contains(reservationId)) {
        ReservationCard *card = m_approveCardMap[reservationId];
        if (card) {
            placeId = card->placeId();
            qDebug() << "从审批卡片获取场所ID:" << placeId;
        }
    }

    // 如果审批卡片中没有找到，尝试从所有卡片中查找
    if (placeId.isEmpty()) {
        for (ReservationCard *card : m_allApproveCards) {
            if (card && card->reservationId() == reservationId) {
                placeId = card->placeId();
                qDebug() << "从所有审批卡片获取场所ID:" << placeId;
                break;
            }
        }
    }

    // 如果还是没找到，使用当前选中的场所ID
    if (placeId.isEmpty() && !m_currentApprovePlaceId.isEmpty()) {
        placeId = m_currentApprovePlaceId;
        qDebug() << "使用当前审批场所ID:" << placeId;
    }

    if (placeId.isEmpty()) {
        QMessageBox::warning(this, "审批失败", "无法确定审批的场所，请重新选择");
        return;
    }

    // 验证场所ID是否有效
    if (placeId == "default_place") {
        QMessageBox::warning(this, "审批失败", "场所ID无效，请联系管理员");
        return;
    }

    // 发送审批请求
    bool approve = (action == "approve");
    qDebug() << "发送审批请求 - 预约ID:" << reservationId << "场所ID:" << placeId << "操作:" << (approve ? "批准" : "拒绝");

    // 立即更新卡片状态（本地预览）
    for (ReservationCard *card : m_allApproveCards) {
        if (card && card->reservationId() == reservationId) {
            card->updateStatus(approve ? "已批准" : "已拒绝");
            break;
        }
    }

    // 发送网络请求
    emit reservationApproveRequested(reservationId.toInt(), placeId, approve);

    // 延迟刷新界面
    QTimer::singleShot(500, this, [this]() {
        refreshCurrentApproveView();
    });
}

void ReservationWidget::onRefreshQueryRequested()
{
    qDebug() << "旧的查询刷新函数被调用，自动重定向到当前页面的刷新";

    // 检查当前处于哪个视图
    if (m_queryViewStack) {
        int currentIndex = m_queryViewStack->currentIndex();
        if (currentIndex == 0) {
            // 在第一级（场所列表）
            onQueryListRefreshRequested();
        } else if (currentIndex == 1) {
            // 在第二级（详情页）
            onQueryDetailRefreshRequested();
        }
    } else {
        // 如果查询视图堆栈不存在，默认调用第一级刷新
        onQueryListRefreshRequested();
    }
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

void ReservationWidget::onBatchApprove()
{
    qDebug() << "批量批准";

    // 收集选中的预约ID
    QList<int> selectedIds;
    for (ReservationCard *card : m_approveCards) {
        if (card && card->isSelected() && card->status().toLower().contains("pending")) {
            int reservationId = card->reservationId().toInt();
            if (reservationId > 0) {
                selectedIds.append(reservationId);
            }
        }
    }

    if (selectedIds.isEmpty()) {
        QMessageBox::information(this, "提示", "请先选择要批准的预约");
        return;
    }

    int result = QMessageBox::question(this, "批量批准",
                                       QString("确定要批量批准 %1 个预约吗？").arg(selectedIds.size()),
                                       QMessageBox::Yes | QMessageBox::No);

    if (result == QMessageBox::Yes) {
        // 批量批准逻辑
        for (int reservationId : selectedIds) {
            // 这里需要实现批量批准的逻辑
            // 暂时先输出日志
            qDebug() << "批准预约ID:" << reservationId;

            // 模拟批准操作
            ReservationCard *card = m_approveCardMap.value(QString::number(reservationId));
            if (card) {
                card->updateStatus("已批准");
            }
        }

        // 刷新视图
        refreshApproveDetailView();

        QMessageBox::information(this, "操作完成",
                                 QString("已批量批准 %1 个预约").arg(selectedIds.size()));
    }
}

void ReservationWidget::onBatchReject()
{
    qDebug() << "批量拒绝";

    // 收集选中的预约ID
    QList<int> selectedIds;
    for (ReservationCard *card : m_approveCards) {
        if (card && card->isSelected() && card->status().toLower().contains("pending")) {
            int reservationId = card->reservationId().toInt();
            if (reservationId > 0) {
                selectedIds.append(reservationId);
            }
        }
    }

    if (selectedIds.isEmpty()) {
        QMessageBox::information(this, "提示", "请先选择要拒绝的预约");
        return;
    }

    int result = QMessageBox::question(this, "批量拒绝",
                                       QString("确定要批量拒绝 %1 个预约吗？").arg(selectedIds.size()),
                                       QMessageBox::Yes | QMessageBox::No);

    if (result == QMessageBox::Yes) {
        // 批量拒绝逻辑
        for (int reservationId : selectedIds) {
            // 这里需要实现批量拒绝的逻辑
            // 暂时先输出日志
            qDebug() << "拒绝预约ID:" << reservationId;

            // 模拟拒绝操作
            ReservationCard *card = m_approveCardMap.value(QString::number(reservationId));
            if (card) {
                card->updateStatus("已拒绝");
            }
        }

        // 刷新视图
        refreshApproveDetailView();

        QMessageBox::information(this, "操作完成",
                                 QString("已批量拒绝 %1 个预约").arg(selectedIds.size()));
    }
}

// 旧的审批筛选变化函数（兼容性实现）
void ReservationWidget::onApproveFilterChanged()
{
    qDebug() << "旧的审批筛选变化函数被调用，使用新函数代替";
    // 调用新的函数，保持兼容
    onApprovePlaceFilterChanged();
}

// 新的审批场所筛选变化
void ReservationWidget::onApprovePlaceFilterChanged()
{
    qDebug() << "审批场所筛选变化";

    if (!m_approvePlaceListRefreshTimer) {
        qCritical() << "错误: m_approvePlaceListRefreshTimer 未初始化";
        return;
    }

    // 停止当前定时器
    if (m_approvePlaceListRefreshTimer->isActive()) {
        m_approvePlaceListRefreshTimer->stop();
    }

    // 重新启动定时器
    m_approvePlaceListRefreshTimer->start(300);
}

// 新的审批详情筛选变化
void ReservationWidget::onApproveDetailFilterChanged()
{
    qDebug() << "审批详情筛选变化";

    if (!m_currentApprovePlaceId.isEmpty()) {
        // 使用定时器避免频繁刷新
        static QTimer detailFilterTimer;
        detailFilterTimer.setSingleShot(true);

        if (detailFilterTimer.isActive()) {
            detailFilterTimer.stop();
        }

        connect(&detailFilterTimer, &QTimer::timeout, this, [this]() {
            if (!m_currentApprovePlaceId.isEmpty()) {
                refreshApproveDetailView();
            }
        });

        detailFilterTimer.start(300);
    }
}

// 返回审批场所列表
void ReservationWidget::onApproveBackToPlaceList()
{
    qDebug() << "返回审批场所列表 - 用户触发";

    // 检查当前是否在详情页面
    if (m_approveViewStack && m_approveViewStack->currentIndex() == 1) {
        // 只有从详情页面返回才执行切换
        m_approveViewStack->setCurrentIndex(0);
        qDebug() << "已返回到审批场所列表";
    } else {
        qDebug() << "当前不在审批详情页面，无需返回";
    }
}

// 第一级导航刷新：重新获取所有数据并刷新场所列表
void ReservationWidget::onQueryListRefreshRequested()
{
    qDebug() << "查询页第一级导航刷新请求 - 重新获取所有数据";

    // 检查当前是否在查询页
    if (!m_tabWidget || m_tabWidget->currentIndex() != 1) {  // 查询页是第1个标签
        qDebug() << "当前不在查询页，忽略刷新";
        return;
    }

    // 确保切换到场所列表页面
    if (m_queryViewStack && m_queryViewStack->currentIndex() == 1) {
        m_queryViewStack->setCurrentIndex(0);
        m_currentPlaceId.clear();
        m_currentPlaceName.clear();
    }

    // 发送查询请求（查询所有场所）
    emit reservationQueryRequested("all");

    qDebug() << "已发送查询请求";
}

// 第二级导航刷新：只刷新当前场所的详情
void ReservationWidget::onQueryDetailRefreshRequested()
{
    qDebug() << "查询页第二级导航刷新请求 - 刷新当前场所详情";

    // 检查当前是否在查询详情页
    if (!m_queryViewStack || m_queryViewStack->currentIndex() != 1) {
        qDebug() << "当前不在查询详情页，忽略刷新";
        return;
    }

    if (m_currentPlaceId.isEmpty()) {
        qDebug() << "当前场所ID为空，无法刷新";
        return;
    }

    // 刷新当前场所的详情
    refreshPlaceDetailView();

    qDebug() << "已刷新场所详情";
}

// 第一级导航刷新：重新获取所有数据并刷新场所列表
void ReservationWidget::onApproveListRefreshRequested()
{
    qDebug() << "第一级导航刷新请求 - 重新获取所有数据";

    // 检查当前是否在审批页
    if (!m_tabWidget || m_tabWidget->currentIndex() != 2) {
        qDebug() << "当前不在审批页，忽略刷新";
        return;
    }

    // 确保切换到场所列表页面
    if (m_approveViewStack && m_approveViewStack->currentIndex() == 1) {
        m_approveViewStack->setCurrentIndex(0);
        m_currentApprovePlaceId.clear();
        m_currentApprovePlaceName.clear();
    }

    // 显示加载提示并发送查询请求
    showApproveListLoading();
    emit reservationQueryRequested("all");
}

// 显示场所列表加载提示
void ReservationWidget::showApproveListLoading()
{
    qDebug() << "显示场所列表加载提示";

    if (!m_approvePlaceListLayout || !m_approvePlaceListPage) {
        return;
    }

    // 清空现有布局
    clearApproveListData();

    // 显示加载提示
    QLabel *loadingLabel = new QLabel("🔄 正在重新加载审批数据...", m_approvePlaceListPage);
    loadingLabel->setObjectName("approveListLoadingLabel");
    loadingLabel->setAlignment(Qt::AlignCenter);
    loadingLabel->setStyleSheet(
        "QLabel {"
        "    color: #4a69bd;"
        "    font-size: 16px;"
        "    font-weight: bold;"
        "    padding: 60px;"
        "    background-color: #f8f9fa;"
        "    border-radius: 8px;"
        "}"
        );
    m_approvePlaceListLayout->addWidget(loadingLabel, 0, 0, 1, 1, Qt::AlignCenter);

    // 更新待审批总数标签
    if (m_approvePendingCountLabel) {
        m_approvePendingCountLabel->setText("待处理: 加载中...");
    }
}

// 清空场所列表数据
void ReservationWidget::clearApproveListData()
{
    if (!m_approvePlaceListLayout) {
        return;
    }

    // 清空布局
    QLayoutItem *child;
    while ((child = m_approvePlaceListLayout->takeAt(0)) != nullptr) {
        if (child->widget()) {
            child->widget()->hide();
            child->widget()->setParent(nullptr);
            child->widget()->deleteLater();
        }
        delete child;
    }

    // 清空场所卡片对象
    for (PlaceQueryCard *card : m_approvePlaceCards) {
        if (card) {
            card->disconnect();
            card->setParent(nullptr);
            card->deleteLater();
        }
    }
    m_approvePlaceCards.clear();
}

// 第二级导航刷新：只刷新当前场所的详情
void ReservationWidget::onApproveDetailRefreshRequested()
{
    qDebug() << "第二级导航刷新请求 - 刷新当前场所详情";

    // 检查当前是否在审批详情页
    if (!m_approveViewStack || m_approveViewStack->currentIndex() != 1) {
        qDebug() << "当前不在审批详情页，忽略刷新";
        return;
    }

    if (m_currentApprovePlaceId.isEmpty()) {
        qDebug() << "当前审批场所ID为空，无法刷新";
        return;
    }

    // 直接刷新详情，不显示加载提示
    refreshApproveDetailView();
}

// 显示详情页加载提示
void ReservationWidget::showApproveDetailLoading()
{
    qDebug() << "显示详情页加载提示";

    if (!m_approveCardLayout || !m_approveCardContainer) {
        return;
    }

    // 清空现有布局
    clearApproveDetailData();

    // 显示加载提示
    QLabel *loadingLabel = new QLabel("🔄 正在刷新当前场所审批记录...", m_approveCardContainer);
    loadingLabel->setAlignment(Qt::AlignCenter);
    loadingLabel->setStyleSheet(
        "QLabel {"
        "    color: #4a69bd;"
        "    font-size: 14px;"
        "    font-weight: bold;"
        "    padding: 40px;"
        "    background-color: #f8f9fa;"
        "    border-radius: 8px;"
        "}"
        );
    m_approveCardLayout->addWidget(loadingLabel);
    m_approveCardLayout->addStretch();
}

// 清空详情数据
void ReservationWidget::clearApproveDetailData()
{
    if (!m_approveCardLayout) {
        return;
    }

    // 清空布局
    QLayoutItem* child;
    while ((child = m_approveCardLayout->takeAt(0)) != nullptr) {
        if (child->widget()) {
            child->widget()->setParent(nullptr);
            delete child->widget();
        }
        delete child;
    }

    // 清空当前显示的卡片列表（不删除卡片对象）
    m_approveCards.clear();
    m_approveCardMap.clear();
}

void ReservationWidget::onSelectAllChanged(int state)
{
    bool checked = (state == Qt::Checked);

    qDebug() << "全选状态改变:" << checked;

    for (ReservationCard *card : m_approveCards) {
        if (card && card->status().toLower().contains("pending")) {
            card->setSelected(checked);
        }
    }

    // 更新批量操作按钮状态
    int selectedCount = 0;
    for (ReservationCard *card : m_approveCards) {
        if (card && card->isSelected()) {
            selectedCount++;
        }
    }

    bool hasSelected = (selectedCount > 0);
    if (m_batchApproveButton) {
        m_batchApproveButton->setEnabled(hasSelected);
    }
    if (m_batchRejectButton) {
        m_batchRejectButton->setEnabled(hasSelected);
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
    // 只清空当前显示的卡片，不删除 m_allApproveCards 中的卡片
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
    qDebug() << "=== 审批页数据加载开始 ===";

    // 安全检查
    if (!this) {
        qCritical() << "错误: this 指针为空";
        return;
    }

    // 移除第一级导航的加载提示
    if (m_approvePlaceListLayout) {
        for (int i = 0; i < m_approvePlaceListLayout->count(); ++i) {
            QLayoutItem* item = m_approvePlaceListLayout->itemAt(i);
            if (item && item->widget()) {
                QLabel* label = qobject_cast<QLabel*>(item->widget());
                if (label && label->objectName() == "approveListLoadingLabel") {
                    m_approvePlaceListLayout->removeWidget(label);
                    label->deleteLater();
                    break;
                }
            }
        }
    }

    // 清空统计数据
    m_approvePlacePendingCount.clear();

    if (data.isEmpty() || data == "暂无预约记录" || data == "fail|暂无数据") {
        qDebug() << "审批数据为空";

        // 清空现有卡片显示，但不删除卡片对象
        for (PlaceQueryCard *card : m_approvePlaceCards) {
            if (card) {
                card->setParent(nullptr);
                card->deleteLater();
            }
        }
        m_approvePlaceCards.clear();

        // 延迟刷新
        QTimer::singleShot(100, this, &ReservationWidget::refreshApprovePlaceListView);
        return;
    }

    // 清空现有卡片（注意：m_allApproveCards 可能在其他地方被使用）
    // 先备份，然后安全删除
    QList<ReservationCard*> oldCards = m_allApproveCards;
    m_allApproveCards.clear();

    for (ReservationCard *card : oldCards) {
        if (card) {
            // 检查卡片是否还在被使用
            if (!m_approveCards.contains(card)) {
                card->deleteLater();
            }
            // 如果卡片正在被显示，不删除它
        }
    }

    // 解析数据
    QStringList reservations = data.split(';', Qt::SkipEmptyParts);
    qDebug() << "解析到预约记录数量:" << reservations.size();

    // 收集所有场所名称和场所类型用于筛选
    QSet<QString> uniquePlaceNames;
    QSet<QString> uniquePlaceTypes;

    for (int i = 0; i < reservations.size(); ++i) {
        QStringList fields = reservations[i].split('|');
        if (fields.size() >= 7) {
            QString reservationId = fields[0].trimmed();
            QString placeId = fields[1].trimmed();
            QString userId = fields[2].trimmed();
            QString purpose = fields[3].trimmed();
            QString startTime = fields[4].trimmed();
            QString endTime = fields[5].trimmed();
            QString status = fields[6].trimmed();

            // 检查预约ID是否有效
            if (reservationId.isEmpty() || reservationId == "\u0000") {
                qWarning() << "无效的预约ID，跳过记录:" << fields;
                continue;
            }

            // 只处理待审批的预约
            if (!status.toLower().contains("pending")) {
                continue;
            }

            QString placeName = getPlaceNameById(placeId);
            if (placeName.isEmpty()) {
                placeName = QString("场所%1").arg(placeId);
            }

            QStringList equipmentList = getEquipmentListForPlace(placeId);
            QString equipmentText = equipmentList.isEmpty() ? "无设备" : equipmentList.join(", ");

            // 收集场所名称和类型
            uniquePlaceNames.insert(placeName);
            QString placeType = detectPlaceType(placeName);
            uniquePlaceTypes.insert(placeType);

            // 统计待审批数量
            m_approvePlacePendingCount[placeId]++;

            qDebug() << "创建审批卡片 - 预约ID:" << reservationId
                     << "场所ID:" << placeId
                     << "场所名称:" << placeName
                     << "状态:" << status;

            // 创建审批卡片
            ReservationCard *card = new ReservationCard(
                reservationId, placeId, placeName, userId, purpose,
                startTime, endTime, status, equipmentText,
                true,   // 审批模式
                nullptr
                );

            if (card) {
                // 将卡片添加到所有卡片列表
                m_allApproveCards.append(card);
                qDebug() << "创建审批卡片成功 - 预约ID:" << reservationId;
            } else {
                qWarning() << "创建审批卡片失败 - 预约ID:" << reservationId;
            }
        } else {
            qWarning() << "数据格式错误，字段数:" << fields.size() << "数据:" << reservations[i];
        }
    }

    qDebug() << "创建了" << m_allApproveCards.size() << "个审批卡片";

    // 延迟更新筛选工具栏
    QTimer::singleShot(50, this, [this, uniquePlaceNames, uniquePlaceTypes]() {
        if (!m_approveFilterBar) {
            qWarning() << "m_approveFilterBar 未初始化";
            return;
        }

        // 设置场所列表
        QStringList placeNames = uniquePlaceNames.values();
        placeNames.sort();
        placeNames.prepend("全部场所");
        m_approveFilterBar->setPlaces(placeNames);

        // 设置场所类型列表
        QStringList placeTypes = uniquePlaceTypes.values();
        QStringList placeTypeNames;
        for (const QString &type : placeTypes) {
            if (type == "classroom") placeTypeNames << "教室";
            else if (type == "lab") placeTypeNames << "实验室";
            else if (type == "meeting") placeTypeNames << "会议室";
            else if (type == "office") placeTypeNames << "办公室";
            else if (type == "gym") placeTypeNames << "体育馆";
            else if (type == "library") placeTypeNames << "图书馆";
            else placeTypeNames << "其他";
        }
        placeTypeNames.sort();
        placeTypeNames.prepend("全部类型");
        m_approveFilterBar->setPlaceTypes(placeTypeNames);

        qDebug() << "更新审批页筛选列表完成";
    });

    // 延迟刷新视图
    QTimer::singleShot(50, this, &ReservationWidget::refreshApprovePlaceListView);

    qDebug() << "=== 审批页数据加载完成 ===";
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
            if (m_queryViewStack && m_queryViewStack->currentIndex() == 0) {
                refreshPlaceListView();
            }
        }
    }

    // ✅ 切换到审批页（索引2）时，自动请求所有预约数据
    if (index == 2 && m_userRole == "admin") {
        qDebug() << "DEBUG: 切换到审批页，准备请求数据...";

        // 安全检查：确保审批页已创建
        if (!m_approveViewStack) {
            qWarning() << "审批页未创建，正在创建...";
            setupApproveTab();
        }

        // 只有在当前没有数据时才查询
        if (m_allApproveCards.isEmpty()) {
            QTimer::singleShot(100, [this]() {
                qDebug() << "DEBUG: 发射 reservationQueryRequested('all')";
                emit reservationQueryRequested("all");
                qDebug() << "DEBUG: 信号已发射";
            });
        } else {
            qDebug() << "DEBUG: 已有审批数据，刷新场所列表";
            // 刷新审批场所列表视图
            if (m_approveViewStack && m_approveViewStack->currentIndex() == 0) {
                refreshApprovePlaceListView();
            }
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

bool ReservationWidget::isApprovePageInitialized() const
{
    bool initialized = true;

    if (!m_approveViewStack) {
        qWarning() << "m_approveViewStack 未初始化";
        initialized = false;
    }

    if (!m_approvePlaceListPage) {
        qWarning() << "m_approvePlaceListPage 未初始化";
        initialized = false;
    }

    if (!m_approvePlaceListLayout) {
        qWarning() << "m_approvePlaceListLayout 未初始化";
        initialized = false;
    }

    if (!m_approvePlaceListContainer) {
        qWarning() << "m_approvePlaceListContainer 未初始化";
        initialized = false;
    }

    qDebug() << "审批页初始化状态:" << (initialized ? "已初始化" : "未完全初始化");
    return initialized;
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


void ReservationWidget::refreshApproveFilterPlaces()
{
    if (!m_approveFilterBar || m_approveCards.isEmpty()) {
        return;
    }

    // 收集所有场所名称
    QSet<QString> uniquePlaceNames;
    for (ReservationCard *card : m_approveCards) {
        if (card && !card->placeName().isEmpty()) {
            uniquePlaceNames.insert(card->placeName());
        }
    }

    // 转换为列表并排序
    QStringList placeNames = uniquePlaceNames.values();
    placeNames.sort();
    placeNames.prepend("全部场所");

    // 更新筛选工具栏
    m_approveFilterBar->setPlaces(placeNames);

    qDebug() << "刷新审批页场所筛选列表，场所数量:" << placeNames.size();
}

QString ReservationWidget::getPlaceTypeDisplayName(const QString &placeTypeCode)
{
    if (placeTypeCode == "classroom") return "教室";
    else if (placeTypeCode == "lab") return "实验室";
    else if (placeTypeCode == "meeting") return "会议室";
    else if (placeTypeCode == "office") return "办公室";
    else if (placeTypeCode == "gym") return "体育馆";
    else if (placeTypeCode == "library") return "图书馆";
    else if (placeTypeCode == "other") return "其他";
    else return "未知类型";
}

void ReservationWidget::recalculatePendingCounts()
{
    qDebug() << "重新计算待审批数量";

    // 清空当前统计
    m_approvePlacePendingCount.clear();

    // 重新统计每个场所的待审批数量
    for (ReservationCard *card : m_allApproveCards) {
        if (!card) continue;

        QString cardStatus = card->status().toLower();
        if (cardStatus.contains("pending")) {
            QString placeId = card->placeId();
            m_approvePlacePendingCount[placeId]++;
        }
    }

    // 更新待审批总数标签
    int totalPending = 0;
    for (int count : m_approvePlacePendingCount.values()) {
        totalPending += count;
    }

    if (m_approvePendingCountLabel) {
        m_approvePendingCountLabel->setText(QString("待处理: %1").arg(totalPending));
    }

    qDebug() << "重新计算完成，总待审批数:" << totalPending;
}

void ReservationWidget::removeLoadingLabels()
{
    // 移除详情页面的加载提示
    if (m_approveCardContainer && m_approveCardLayout) {
        // 检查第一个项目是否是加载提示
        QLayoutItem* firstItem = m_approveCardLayout->itemAt(0);
        if (firstItem && firstItem->widget()) {
            QLabel* label = qobject_cast<QLabel*>(firstItem->widget());
            if (label && (label->text().contains("正在重新加载") || label->text().contains("正在刷新"))) {
                m_approveCardLayout->removeWidget(label);
                label->deleteLater();
            }
        }
    }

    // 移除场所列表页面的加载提示
    if (m_approvePlaceListLayout) {
        for (int i = 0; i < m_approvePlaceListLayout->count(); ++i) {
            QLayoutItem* item = m_approvePlaceListLayout->itemAt(i);
            if (item && item->widget()) {
                QLabel* label = qobject_cast<QLabel*>(item->widget());
                if (label && label->objectName() == "loadingLabel") {
                    m_approvePlaceListLayout->removeWidget(label);
                    label->deleteLater();
                    break;
                }
            }
        }
    }
}

QStringList ReservationWidget::getStatusMap(const QString &statusCode) const
{
    QMap<QString, QStringList> statusMap = {
        {"all", {"all", "全部状态"}},
        {"pending", {"pending", "待审批", "未审批", "pending", "待审核", "未审核"}},
        {"approved", {"approved", "已批准", "通过", "approved", "已同意", "已授权"}},
        {"rejected", {"rejected", "已拒绝", "拒绝", "rejected", "驳回", "未通过"}},
        {"completed", {"completed", "已完成", "completed", "已结束", "已完成"}},
        {"cancelled", {"cancelled", "已取消", "cancelled", "取消", "已作废"}}
    };

    return statusMap.value(statusCode, QStringList());
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
                    if (m_placeListRefreshTimer && m_placeListRefreshTimer->isActive()) {
                        m_placeListRefreshTimer->stop();
                    }
                    if (m_placeListRefreshTimer) {
                        m_placeListRefreshTimer->start(100);
                    }
                } else if (currentView == 1) { // 场所详情页面
                    QTimer::singleShot(50, this, &ReservationWidget::refreshPlaceDetailView);
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


