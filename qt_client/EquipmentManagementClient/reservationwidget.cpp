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
}

void ReservationWidget::setupApplyTab()
{
    QWidget *applyTab = new QWidget(this);
    QFormLayout *formLayout = new QFormLayout(applyTab);

    m_equipmentComboApply = new QComboBox(this);
    m_startTimeEdit = new QDateTimeEdit(QDateTime::currentDateTime(), this);
    m_endTimeEdit = new QDateTimeEdit(QDateTime::currentDateTime().addSecs(3600), this);
    m_purposeEdit = new QLineEdit(this);
    m_applyButton = new QPushButton("提交预约", this);

    m_startTimeEdit->setDisplayFormat("yyyy-MM-dd HH:mm");
    m_endTimeEdit->setDisplayFormat("yyyy-MM-dd HH:mm");

    formLayout->addRow("设备:", m_equipmentComboApply);
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

    m_equipmentComboQuery = new QComboBox(this);
    m_equipmentComboQuery->addItem("全部设备", "all");
    m_queryButton = new QPushButton("查询", this);

    hLayout->addWidget(new QLabel("设备:", this));
    hLayout->addWidget(m_equipmentComboQuery);
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
}

void ReservationWidget::setUserRole(const QString &role, const QString &userId)
{
    m_userRole = role;
    m_currentUserId = userId;

    // 非管理员隐藏审批页
    if (role != "admin") {
        m_tabWidget->removeTab(2); // 移除审批页
    }
}

void ReservationWidget::onApplyButtonClicked()
{
    if (m_equipmentComboApply->currentIndex() == -1) {
        QMessageBox::warning(this, "提示", "请先选择设备");
        return;
    }

    emit reservationApplyRequested(
        m_equipmentComboApply->currentData().toString(),
        m_purposeEdit->text(),
        m_startTimeEdit->dateTime().toString("yyyy-MM-dd HH:mm:ss"),
        m_endTimeEdit->dateTime().toString("yyyy-MM-dd HH:mm:ss")
        );
}

void ReservationWidget::onQueryButtonClicked()
{
    emit reservationQueryRequested(m_equipmentComboQuery->currentData().toString());
}

// 后续再实现审批按钮功能
void ReservationWidget::onApproveButtonClicked()
{
    // TODO: 获取选中的预约ID，发送批准请求
    QMessageBox::information(this, "提示", "批准功能开发中");
}

void ReservationWidget::onDenyButtonClicked()
{
    // TODO: 获取选中的预约ID，发送拒绝请求
    QMessageBox::information(this, "提示", "拒绝功能开发中");
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
