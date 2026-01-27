#include "reservationwidget.h"
#include "reservationcard.h"
#include "reservationfiltertoolbar.h"
#include "placecard.h"

#include <QMessageBox>
#include <QHeaderView>
#include <QLabel>
#include <QTimer>
#include <QDebug>
#include <QGridLayout>

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
{
    qDebug() << "ReservationWidget constructor start";

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

    qDebug() << "ReservationWidget constructor finished";
    qDebug() << "m_placeComboApply pointer:" << m_placeComboApply;
    qDebug() << "m_placeComboQuery pointer:" << m_placeComboQuery;
}

ReservationWidget::~ReservationWidget()
{
    qDebug() << "ReservationWidget destructor called";

    // 清理申请页的场所卡片
    for (PlaceCard *card : m_placeCards.values()) {
        if (card) {
            card->disconnect();
            card->deleteLater();
        }
    }
    m_placeCards.clear();

    // 清理查询页的预约卡片
    for (ReservationCard *card : m_queryCards) {
        if (card) {
            card->disconnect();
            card->deleteLater();
        }
    }
    m_queryCards.clear();
    m_queryCardMap.clear();

    // 清理审批页的预约卡片
    for (ReservationCard *card : m_approveCards) {
        if (card) {
            card->disconnect();
            card->deleteLater();
        }
    }
    m_approveCards.clear();
    m_approveCardMap.clear();
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
    QWidget *queryTab = new QWidget(this);
    QVBoxLayout *mainLayout = new QVBoxLayout(queryTab);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // 创建视图堆栈
    m_queryViewStack = new QStackedWidget(queryTab);

    // ==== 卡片视图页面 ====
    QWidget *cardViewPage = new QWidget(m_queryViewStack);
    QVBoxLayout *cardViewLayout = new QVBoxLayout(cardViewPage);
    cardViewLayout->setContentsMargins(0, 0, 0, 0);
    cardViewLayout->setSpacing(0);

    // 创建筛选工具栏
    m_queryFilterBar = new ReservationFilterToolBar(cardViewPage);
    cardViewLayout->addWidget(m_queryFilterBar);

    // 创建滚动区域
    m_queryScrollArea = new QScrollArea(cardViewPage);
    m_queryScrollArea->setWidgetResizable(true);
    m_queryScrollArea->setFrameShape(QFrame::NoFrame);

    // 创建卡片容器
    m_queryCardContainer = new QWidget();
    m_queryCardContainer->setObjectName("queryCardContainer");
    m_queryCardLayout = new QVBoxLayout(m_queryCardContainer);
    m_queryCardLayout->setContentsMargins(20, 20, 20, 20);
    m_queryCardLayout->setSpacing(20);
    m_queryCardLayout->addStretch();

    m_queryScrollArea->setWidget(m_queryCardContainer);
    cardViewLayout->addWidget(m_queryScrollArea);

    m_queryViewStack->addWidget(cardViewPage);

    // ==== 表格视图页面（保留原有功能）====
    QWidget *tableViewPage = new QWidget(m_queryViewStack);
    QVBoxLayout *tableViewLayout = new QVBoxLayout(tableViewPage);

    // 原有的查询控件
    QHBoxLayout *hLayout = new QHBoxLayout();
    m_placeComboQuery = new QComboBox(tableViewPage);  // 在这里初始化 m_placeComboQuery
    m_placeComboQuery->addItem("全部场所", "all");
    m_queryButton = new QPushButton("查询", tableViewPage);

    hLayout->addWidget(new QLabel("场所:", tableViewPage));
    hLayout->addWidget(m_placeComboQuery);
    hLayout->addWidget(m_queryButton);
    hLayout->addStretch();

    m_queryResultTable = new QTableWidget(0, 7, tableViewPage);
    m_queryResultTable->setHorizontalHeaderLabels({"预约ID", "设备ID", "用户ID", "用途", "开始时间", "结束时间", "状态"});
    m_queryResultTable->horizontalHeader()->setStretchLastSection(true);
    m_queryResultTable->setEditTriggers(QAbstractItemView::NoEditTriggers);

    tableViewLayout->addLayout(hLayout);
    tableViewLayout->addWidget(m_queryResultTable);

    m_queryViewStack->addWidget(tableViewPage);

    // 默认显示卡片视图
    m_queryViewStack->setCurrentIndex(0);

    // 添加到主布局
    mainLayout->addWidget(m_queryViewStack);

    m_tabWidget->addTab(queryTab, "预约查询");

    // 连接信号
    connect(m_queryButton, &QPushButton::clicked, this, &ReservationWidget::onQueryButtonClicked);
    connect(m_queryFilterBar, &ReservationFilterToolBar::filterChanged,
            this, &ReservationWidget::onFilterChanged);
    connect(m_queryFilterBar, &ReservationFilterToolBar::refreshRequested,
            this, &ReservationWidget::onRefreshQueryRequested);

    qDebug() << "Query tab setup finished";
    qDebug() << "m_placeComboQuery created:" << (m_placeComboQuery != nullptr);
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

void ReservationWidget::refreshQueryCardView()
{
    qDebug() << "=== 刷新预约查询卡片视图 ===";
    qDebug() << "当前卡片数量:" << m_queryCards.size();

    if (!m_queryCardLayout) {
        qDebug() << "错误: m_queryCardLayout 为空";
        return;
    }

    // 清理现有布局
    QLayoutItem *item;
    while ((item = m_queryCardLayout->takeAt(0)) != nullptr) {
        if (item->widget()) {
            item->widget()->setParent(nullptr);
        }
        delete item;
    }

    // 如果没有卡片，显示提示信息
    if (m_queryCards.isEmpty()) {
        qDebug() << "没有预约卡片，显示空状态";
        QLabel *emptyLabel = new QLabel("暂无预约记录", m_queryCardContainer);
        emptyLabel->setAlignment(Qt::AlignCenter);
        emptyLabel->setStyleSheet(
            "QLabel {"
            "    color: #999;"
            "    font-size: 16px;"
            "    padding: 60px;"
            "}"
            );
        m_queryCardLayout->addWidget(emptyLabel);
        return;
    }

    // 获取筛选条件
    QString selectedPlace = m_queryFilterBar->selectedPlace();
    QString selectedStatus = m_queryFilterBar->selectedStatus();
    QString searchText = m_queryFilterBar->searchText();
    QDate startDate = m_queryFilterBar->startDate();
    QDate endDate = m_queryFilterBar->endDate();

    qDebug() << "筛选条件: 场所=" << selectedPlace
             << "状态=" << selectedStatus
             << "搜索=" << searchText;

    // 计算每行卡片数量
    int containerWidth = m_queryCardContainer->width();
    if (containerWidth <= 0) containerWidth = 800;
    int cardsPerRow = qMax(1, containerWidth / 340);

    qDebug() << "容器宽度:" << containerWidth << "每行卡片数:" << cardsPerRow;

    // 创建网格布局
    QGridLayout *gridLayout = new QGridLayout();
    gridLayout->setContentsMargins(0, 0, 0, 0);
    gridLayout->setHorizontalSpacing(20);
    gridLayout->setVerticalSpacing(20);

    int row = 0;
    int col = 0;
    int visibleCards = 0;

    for (ReservationCard *card : m_queryCards) {
        // 应用筛选条件
        bool shouldShow = true;

        // 场所筛选
        if (selectedPlace != "all") {
            if (card->place() != selectedPlace) {
                shouldShow = false;
            }
        }

        // 状态筛选
        if (selectedStatus != "all") {
            QString cardStatus = card->status().toLower();
            QString filterStatus = selectedStatus.toLower();
            if (cardStatus != filterStatus) {
                shouldShow = false;
            }
        }

        // 搜索文本筛选
        if (!searchText.isEmpty()) {
            if (!card->reservationId().contains(searchText, Qt::CaseInsensitive) &&
                !card->place().contains(searchText, Qt::CaseInsensitive) &&
                !card->userId().contains(searchText, Qt::CaseInsensitive) &&
                !card->purpose().contains(searchText, Qt::CaseInsensitive)) {
                shouldShow = false;
            }
        }

        // 设置卡片可见性
        card->setVisible(shouldShow);

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

    // 将网格布局添加到容器布局
    QWidget *gridWidget = new QWidget(m_queryCardContainer);
    gridWidget->setLayout(gridLayout);
    m_queryCardLayout->addWidget(gridWidget);
    m_queryCardLayout->addStretch();

    // 如果没有可见卡片，显示提示信息
    if (visibleCards == 0) {
        qDebug() << "没有符合条件的预约记录";
        QLabel *noMatchLabel = new QLabel("没有符合条件的预约记录", m_queryCardContainer);
        noMatchLabel->setAlignment(Qt::AlignCenter);
        noMatchLabel->setStyleSheet(
            "QLabel {"
            "    color: #999;"
            "    font-size: 14px;"
            "    padding: 40px;"
            "}"
            );
        gridLayout->addWidget(noMatchLabel, 0, 0, 1, cardsPerRow, Qt::AlignCenter);
    }

    qDebug() << "刷新完成，可见卡片数量:" << visibleCards;
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

void ReservationWidget::onFilterChanged()
{
    refreshQueryCardView();
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
    // 类似refreshQueryCardView，但只显示待审批的预约
    if (!m_approveCardLayout) return;

    // 清空现有布局
    QLayoutItem *item;
    while ((item = m_approveCardLayout->takeAt(0)) != nullptr) {
        if (item->widget()) {
            item->widget()->setParent(nullptr);
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

    for (ReservationCard *card : m_approveCards) {
        // 只显示待审批的预约
        QString status = card->status().toLower();
        if (status == "pending" || status == "待审批" || status == "未审批") {
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
        QLabel *noPendingLabel = new QLabel("没有待审批的预约", m_approveCardContainer);
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
    qDebug() << "清空查询卡片视图，原卡片数量:" << m_queryCards.size();

    for (ReservationCard *card : m_queryCards) {
        if (card) {
            card->disconnect();
            card->deleteLater();
        }
    }
    m_queryCards.clear();
    m_queryCardMap.clear();

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


void ReservationWidget::updateQueryResultTable(const QString &data)
{
    qDebug() << "=== updateQueryResultTable ===";
    qDebug() << "传入数据:" << data;

    // 清空表格和卡片
    if (m_queryResultTable) {
        m_queryResultTable->setRowCount(0);
    }
    clearQueryCardView();

    // 检查数据是否有效
    if (data.isEmpty() || data == "暂无预约记录" || data == "0") {
        qDebug() << "数据为空或无效";
        refreshQueryCardView();
        return;
    }

    // 解析数据
    QStringList reservations = data.split(';', Qt::SkipEmptyParts);
    qDebug() << "预约记录数量:" << reservations.size();

    // 更新表格视图
    m_queryResultTable->setHorizontalHeaderLabels({"预约ID", "场所ID", "用户ID", "用途", "开始时间", "结束时间", "状态", "包含设备"});

    // 用于收集场所信息
    QSet<QString> uniquePlaces;

    for (int i = 0; i < reservations.size(); ++i) {
        QStringList fields = reservations[i].split('|');
        qDebug() << "记录" << i << "字段数:" << fields.size() << "字段:" << fields;

        if (fields.size() >= 7) {
            QString reservationId = fields[0];
            QString placeId = fields[1];
            QString userId = fields[2];
            QString purpose = fields[3];
            QString startTime = fields[4];
            QString endTime = fields[5];
            QString status = fields[6];

            qDebug() << "创建预约卡片: ID=" << reservationId
                     << "场所=" << placeId
                     << "用户=" << userId
                     << "状态=" << status;

            // 获取场所名称
            QString placeName = getPlaceNameById(placeId);
            uniquePlaces.insert(placeName);

            // 获取设备列表
            QStringList equipmentList = getEquipmentListForPlace(placeId);
            QString equipmentText = equipmentList.join(", ");

            // 添加到表格
            m_queryResultTable->insertRow(i);
            for (int j = 0; j < 7; ++j) {
                m_queryResultTable->setItem(i, j, new QTableWidgetItem(fields[j]));
            }
            m_queryResultTable->setItem(i, 7, new QTableWidgetItem(equipmentText));

            // 创建设备卡片
            ReservationCard *card = new ReservationCard(
                reservationId, placeName, userId, purpose,
                startTime, endTime, status, equipmentText, m_queryCardContainer);

            connect(card, &ReservationCard::cardClicked,
                    this, &ReservationWidget::onReservationCardClicked);
            connect(card, &ReservationCard::statusActionRequested,
                    this, &ReservationWidget::onStatusActionRequested);

            m_queryCards.append(card);
            m_queryCardMap[reservationId] = card;

            qDebug() << "卡片创建成功，已添加到列表";
        } else {
            qDebug() << "跳过无效记录，字段数不足:" << fields.size();
        }
    }

    qDebug() << "共创建卡片数:" << m_queryCards.size();

    // 更新筛选工具栏的场所选项
    if (m_queryFilterBar) {
        m_queryFilterBar->setPlaces(uniquePlaces.values());
        qDebug() << "更新筛选工具栏，场所数量:" << uniquePlaces.size();
    }

    // 确保显示卡片视图
    if (m_queryViewStack) {
        m_queryViewStack->setCurrentIndex(0);
        qDebug() << "切换到卡片视图";
    }

    // 刷新卡片视图
    refreshQueryCardView();
    qDebug() << "updateQueryResultTable 完成";
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

    // 解析数据并创建卡片
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

            // 创建审批卡片
            ReservationCard *card = new ReservationCard(
                reservationId, placeName, userId, purpose,
                startTime, endTime, status, equipmentText, m_approveCardContainer);

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
    int index = m_placeComboApply->findData(placeId);
    if (index >= 0) {
        return m_placeComboApply->itemText(index);
    }

    // 从查询页下拉框查找
    index = m_placeComboQuery->findData(placeId);
    if (index >= 0) {
        return m_placeComboQuery->itemText(index);
    }

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

        // 延迟请求，确保UI完全加载
        QTimer::singleShot(200, [this]() {
            qDebug() << "DEBUG: 自动发射 reservationQueryRequested('all')";
            emit reservationQueryRequested("all");
        });
    }

    // ✅ 切换到审批页（索引2）时，自动请求所有预约数据
    if (index == 2 && m_userRole == "admin") {
        qDebug() << "DEBUG: 切换到审批页，准备请求数据...";
        QTimer::singleShot(100, [this]() {
            qDebug() << "DEBUG: 发射 reservationQueryRequested('all')";
            emit reservationQueryRequested("all");
            qDebug() << "DEBUG: 信号已发射";
        });
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

void ReservationWidget::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);

    // 窗口大小变化时刷新卡片布局
    if (m_tabWidget) {
        int currentTab = m_tabWidget->currentIndex();

        if (currentTab == 1) { // 查询页
            QTimer::singleShot(100, this, &ReservationWidget::refreshQueryCardView);
        }
        else if (currentTab == 0 && !m_placeCards.isEmpty()) { // 申请页且有场所卡片
            // 刷新场所卡片布局
            updatePlaceCardsLayout();
        }
    }
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


