#include "reservationwidget.h"

#include <QMessageBox>
#include <QHeaderView>
#include <QLabel>

ReservationWidget::ReservationWidget(QWidget *parent)
    : QWidget(parent), m_tabWidget(new QTabWidget(this))
{
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
}

void ReservationWidget::setupApplyTab()
{
    QWidget *applyTab = new QWidget(this);
    QFormLayout *formLayout = new QFormLayout(applyTab);

    m_placeComboApply = new QComboBox(this);  // 改动1：变量名
    m_startTimeEdit = new QDateTimeEdit(QDateTime::currentDateTime(), this);
    m_endTimeEdit = new QDateTimeEdit(QDateTime::currentDateTime().addSecs(3600), this);
    m_purposeEdit = new QLineEdit(this);
    m_applyButton = new QPushButton("提交预约", this);

    m_startTimeEdit->setDisplayFormat("yyyy-MM-dd HH:mm");
    m_endTimeEdit->setDisplayFormat("yyyy-MM-dd HH:mm");

    formLayout->addRow("场所:", m_placeComboApply);  // 改动2：标签文本
    formLayout->addRow("开始时间:", m_startTimeEdit);
    formLayout->addRow("结束时间:", m_endTimeEdit);
    formLayout->addRow("用途:", m_purposeEdit);
    formLayout->addRow("", m_applyButton);

    m_tabWidget->addTab(applyTab, "预约申请");

    connect(m_applyButton, &QPushButton::clicked, this, &ReservationWidget::onApplyButtonClicked);
}

void ReservationWidget::setupQueryTab()
{
    QWidget *queryTab = new QWidget(this);
    QVBoxLayout *vLayout = new QVBoxLayout(queryTab);
    QHBoxLayout *hLayout = new QHBoxLayout();

    m_placeComboQuery = new QComboBox(this);  // 改动1：变量名
    m_placeComboQuery->addItem("全部场所", "all");  // 改动2：默认文本
    m_queryButton = new QPushButton("查询", this);

    hLayout->addWidget(new QLabel("场所:", this));  // 改动3：标签文本
    hLayout->addWidget(m_placeComboQuery);
    hLayout->addWidget(m_queryButton);
    hLayout->addStretch();

    m_queryResultTable = new QTableWidget(0, 7, this);
    m_queryResultTable->setHorizontalHeaderLabels({"预约ID", "设备ID", "用户ID", "用途", "开始时间", "结束时间", "状态"});
    m_queryResultTable->horizontalHeader()->setStretchLastSection(true);
    m_queryResultTable->setEditTriggers(QAbstractItemView::NoEditTriggers);

    vLayout->addLayout(hLayout);
    vLayout->addWidget(m_queryResultTable);

    m_tabWidget->addTab(queryTab, "预约查询");

    connect(m_queryButton, &QPushButton::clicked, this, &ReservationWidget::onQueryButtonClicked);
}

void ReservationWidget::setupApproveTab()
{
    QWidget *approveTab = new QWidget(this);
    QVBoxLayout *vLayout = new QVBoxLayout(approveTab);

    m_approveTable = new QTableWidget(0, 8, this);
    m_approveTable->setHorizontalHeaderLabels({"预约ID", "设备ID", "用户ID", "用途", "开始时间", "结束时间", "状态", "操作"});
    m_approveTable->horizontalHeader()->setStretchLastSection(true);
    m_approveTable->setEditTriggers(QAbstractItemView::NoEditTriggers);

    vLayout->addWidget(m_approveTable);

    m_tabWidget->addTab(approveTab, "预约审批");

    m_approveTable->insertRow(0);
    m_approveTable->setItem(0, 0, new QTableWidgetItem("暂无待审批记录"));
}

void ReservationWidget::setUserRole(const QString &role, const QString &userId)
{

    m_userRole = role;
    qDebug() << "DEBUG: setUserRole called, role=" << role << ", userId=" << userId;
    // 确保 setupApproveTab 已创建审批页（在构造函数中已调用）
    if (role == "admin") {
        qDebug() << "DEBUG: 管理员，显示审批页";
        // 如果审批页被移除了，重新添加
        if (m_tabWidget->count() <= 2) {
            setupApproveTab();  // 重新创建审批页
        }
    } else {
        qDebug() << "DEBUG: 非管理员，移除审批页";
        if (m_tabWidget->count() > 2) {
            m_tabWidget->removeTab(2);
        }
    }
}

void ReservationWidget::onApplyButtonClicked()
{
    if (m_placeComboApply->currentIndex() == -1) {  // 改动1：变量名
        QMessageBox::warning(this, "提示", "请先选择场所");  // 改动2：提示文本
        return;
    }

    emit reservationApplyRequested(
        m_placeComboApply->currentData().toString(),  // 改动3：变量名
        m_purposeEdit->text(),
        m_startTimeEdit->dateTime().toString("yyyy-MM-dd HH:mm:ss"),
        m_endTimeEdit->dateTime().toString("yyyy-MM-dd HH:mm:ss")
        );
}

void ReservationWidget::onQueryButtonClicked()
{
    emit reservationQueryRequested(m_placeComboQuery->currentData().toString());
}


void ReservationWidget::updateQueryResultTable(const QString &data)
{
    // 清空表格
    m_queryResultTable->setRowCount(0);

    if (data.isEmpty()) {
        QMessageBox::information(this, "查询结果", "暂无预约记录");
        return;
    }

    // 解析数据格式: "id|equipment_id|user_id|purpose|start_time|end_time|status;..."
    QStringList reservations = data.split(';', Qt::SkipEmptyParts);

    for (int i = 0; i < reservations.size(); ++i) {
        QStringList fields = reservations[i].split('|');
        if (fields.size() >= 7) {
            m_queryResultTable->insertRow(i);
            for (int j = 0; j < 7; ++j) {
                QTableWidgetItem *item = new QTableWidgetItem(fields[j]);
                m_queryResultTable->setItem(i, j, item);
            }
        }
    }
    // 调整列宽（关键修复）
    m_queryResultTable->resizeColumnsToContents();  // 自动调整所有列宽
    m_queryResultTable->horizontalHeader()->setStretchLastSection(true);  // 最后一列拉伸

    qDebug() << "[ReservationWidget] 预约查询完成，共" << reservations.size() << "条记录";

    qDebug() << "预约查询完成，共" << reservations.size() << "条记录";
}

void ReservationWidget::loadAllReservationsForApproval(const QString &data)
{
    m_approveTable->setRowCount(0);

    if (data.isEmpty()) {
        m_approveTable->insertRow(0);
        m_approveTable->setItem(0, 0, new QTableWidgetItem("暂无预约记录"));
        for (int i = 1; i < 8; ++i) {
            m_approveTable->setItem(0, i, new QTableWidgetItem(""));
        }
        return;
    }

    // 解析所有预约记录
    QStringList reservations = data.split(';', Qt::SkipEmptyParts);

    for (int i = 0; i < reservations.size(); ++i) {
        QStringList fields = reservations[i].split('|');
        if (fields.size() >= 7) {
            m_approveTable->insertRow(i);

            // 填充前7列数据
            for (int j = 0; j < 7; ++j) {
                m_approveTable->setItem(i, j, new QTableWidgetItem(fields[j]));
            }

            // 第8列：操作列
            QString status = fields[6];  // status: pending/approved/rejected

            QWidget *opWidget = new QWidget(this);
            QHBoxLayout *opLayout = new QHBoxLayout(opWidget);
            opLayout->setContentsMargins(0, 0, 0, 0);
            opLayout->setSpacing(2);

            if (status == "pending") {
                // 待审批：显示批准和拒绝按钮
                QPushButton *approveBtn = new QPushButton("Approve", this);
                approveBtn->setProperty("reservationId", fields[0]);
                approveBtn->setProperty("action", "approve");
                connect(approveBtn, &QPushButton::clicked, this, &ReservationWidget::onApproveButtonClicked);

                QPushButton *rejectBtn = new QPushButton("Reject", this);
                rejectBtn->setProperty("reservationId", fields[0]);
                rejectBtn->setProperty("action", "reject");
                connect(rejectBtn, &QPushButton::clicked, this, &ReservationWidget::onDenyButtonClicked);

                opLayout->addWidget(approveBtn);
                opLayout->addWidget(rejectBtn);
            } else {
                // 已审批：显示状态文本
                QLabel *statusLabel = new QLabel(status.toUpper(), this);
                statusLabel->setAlignment(Qt::AlignCenter);
                if (status == "approved") {
                    statusLabel->setStyleSheet("color: green; font-weight: bold;");
                } else if (status == "rejected") {
                    statusLabel->setStyleSheet("color: red; font-weight: bold;");
                }
                opLayout->addWidget(statusLabel);
            }

            m_approveTable->setCellWidget(i, 7, opWidget);
        }
    }

    // 调整列宽
    m_approveTable->resizeColumnsToContents();
    m_approveTable->horizontalHeader()->setStretchLastSection(true);
    m_approveTable->setColumnWidth(4, 150);  // 开始时间
    m_approveTable->setColumnWidth(5, 150);  // 结束时间
}

void ReservationWidget::onApproveButtonClicked()
{
    QPushButton *btn = qobject_cast<QPushButton*>(sender());
    if (!btn) return;

    int reservationId = btn->property("reservationId").toInt();
    emit reservationApproveRequested(reservationId, true);
}

void ReservationWidget::onDenyButtonClicked()
{
    QPushButton *btn = qobject_cast<QPushButton*>(sender());
    if (!btn) return;

    int reservationId = btn->property("reservationId").toInt();
    emit reservationApproveRequested(reservationId, false);
}

void ReservationWidget::onTabChanged(int index)
{
    qDebug() << "DEBUG: Tab changed to index" << index;

    // 当切换到审批页（索引2）时，触发加载请求
    if (index == 2 && m_userRole == "admin") {
        qDebug() << "DEBUG: 切换到审批页，请求加载待审批列表";
        emit loadAllReservationsRequested();
    }
}
