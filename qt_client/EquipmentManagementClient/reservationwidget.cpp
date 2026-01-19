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
    QVBoxLayout *mainLayout = new QVBoxLayout(applyTab);  // ✅ 改用垂直布局

    // 基本信息表单
    QFormLayout *formLayout = new QFormLayout();
    m_placeComboApply = new QComboBox(this);
    m_startTimeEdit = new QDateTimeEdit(QDateTime::currentDateTime(), this);
    m_endTimeEdit = new QDateTimeEdit(QDateTime::currentDateTime().addSecs(3600), this);
    m_purposeEdit = new QLineEdit(this);
    m_applyButton = new QPushButton("提交预约", this);

    m_startTimeEdit->setDisplayFormat("yyyy-MM-dd HH:mm");
    m_endTimeEdit->setDisplayFormat("yyyy-MM-dd HH:mm");

    formLayout->addRow("场所:", m_placeComboApply);
    formLayout->addRow("开始时间:", m_startTimeEdit);
    formLayout->addRow("结束时间:", m_endTimeEdit);
    formLayout->addRow("用途:", m_purposeEdit);
    formLayout->addRow("", m_applyButton);

    mainLayout->addLayout(formLayout);

    // ✅ 新增：设备列表分组框（更醒目的显示）
    QGroupBox *equipmentGroup = new QGroupBox("场所包含设备", this);
    QVBoxLayout *equipmentLayout = new QVBoxLayout(equipmentGroup);

    m_equipmentListText = new QTextEdit(this);
    m_equipmentListText->setReadOnly(true);
    m_equipmentListText->setMinimumHeight(120);  // ✅ 设置最小高度
    m_equipmentListText->setMaximumHeight(150);
    m_equipmentListText->setPlaceholderText("选择场所后自动加载设备列表");

    equipmentLayout->addWidget(m_equipmentListText);
    mainLayout->addWidget(equipmentGroup);  // ✅ 添加到主布局

    m_tabWidget->addTab(applyTab, "预约申请");

    connect(m_applyButton, &QPushButton::clicked, this, &ReservationWidget::onApplyButtonClicked);

    // 场所选择变化时自动加载设备列表
    connect(m_placeComboApply, QOverload<int>::of(&QComboBox::currentIndexChanged),
            [this](int index) {
                Q_UNUSED(index);
                updateEquipmentListDisplay();
            });

     m_placeComboApply->setCurrentIndex(-1);  // 重置索引
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
    m_approveTable->setHorizontalHeaderLabels({"预约ID", "场所ID", "用户ID", "用途", "开始时间", "结束时间", "状态", "操作"});
    m_approveTable->horizontalHeader()->setStretchLastSection(true);
    m_approveTable->setEditTriggers(QAbstractItemView::NoEditTriggers);

    vLayout->addWidget(m_approveTable);

    m_tabWidget->addTab(approveTab, "预约审批");
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
    qDebug() << "DEBUG: updateQueryResultTable, data:" << data;

    // 清空表格
    m_queryResultTable->setRowCount(0);

    if (data.isEmpty() || data == "暂无预约记录") {
        QMessageBox::information(this, "查询结果", "暂无预约记录");
        return;
    }

    // ✅ 解析数据并添加设备信息列
    QStringList reservations = data.split(';', Qt::SkipEmptyParts);

    // ✅ 修改表头：增加"包含设备"列
    m_queryResultTable->setHorizontalHeaderLabels({"预约ID", "场所ID", "用户ID", "用途", "开始时间", "结束时间", "状态", "包含设备"});

    for (int i = 0; i < reservations.size(); ++i) {
        QStringList fields = reservations[i].split('|');
        if (fields.size() >= 7) {
            m_queryResultTable->insertRow(i);

            // 填充前7列
            for (int j = 0; j < 7; ++j) {
                m_queryResultTable->setItem(i, j, new QTableWidgetItem(fields[j]));
            }

            // ✅ 第8列：查询并显示该场所包含的设备
            QString placeId = fields[1];  // 场所ID在第2列
            QStringList equipmentList = getEquipmentListForPlace(placeId);
            m_queryResultTable->setItem(i, 7, new QTableWidgetItem(equipmentList.join(", ")));
        }
    }

    // 调整列宽
    m_queryResultTable->resizeColumnsToContents();
    m_queryResultTable->horizontalHeader()->setStretchLastSection(true);
    m_queryResultTable->setColumnWidth(7, 200);  // 设备列宽度
}

void ReservationWidget::loadAllReservationsForApproval(const QString &data)
{
    qDebug() << "=== 审批页数据加载 ===";
    qDebug() << "原始数据:" << data;

    // ✅ 彻底清空并重置表格
    m_approveTable->clearContents();
    m_approveTable->setRowCount(0);

    // ✅ 确保表头正确
    m_approveTable->setHorizontalHeaderLabels({"预约ID", "场所ID", "用户ID", "用途", "开始时间", "结束时间", "状态", "操作"});

    if (data.isEmpty() || data == "暂无预约记录") {
        m_approveTable->insertRow(0);
        m_approveTable->setItem(0, 0, new QTableWidgetItem("暂无预约记录"));
        return;
    }

    // 解析所有预约记录
    QStringList reservations = data.split(';', Qt::SkipEmptyParts);
    int row = 0;

    for (int i = 0; i < reservations.size(); ++i) {
        QStringList fields = reservations[i].split('|');
        if (fields.size() >= 7) {
            m_approveTable->insertRow(row);

            // 填充前7列
            for (int j = 0; j < 7; ++j) {
                m_queryResultTable->setItem(row, j, new QTableWidgetItem(fields[j]));
            }

            // ✅ 第8列：操作列（显示所有状态）
            QString status = fields[6];

            QWidget *opWidget = new QWidget(this);
            QHBoxLayout *opLayout = new QHBoxLayout(opWidget);
            opLayout->setContentsMargins(0, 0, 0, 0);
            opLayout->setSpacing(2);

            if (status == "pending") {
                QPushButton *approveBtn = new QPushButton("批准", this);
                approveBtn->setProperty("reservationId", fields[0]);
                approveBtn->setProperty("action", "approve");
                connect(approveBtn, &QPushButton::clicked, this, &ReservationWidget::onApproveButtonClicked);

                QPushButton *rejectBtn = new QPushButton("拒绝", this);
                rejectBtn->setProperty("reservationId", fields[0]);
                rejectBtn->setProperty("action", "reject");
                connect(rejectBtn, &QPushButton::clicked, this, &ReservationWidget::onDenyButtonClicked);

                opLayout->addWidget(approveBtn);
                opLayout->addWidget(rejectBtn);
            } else {
                QLabel *statusLabel = new QLabel(status.toUpper(), this);
                statusLabel->setAlignment(Qt::AlignCenter);
                if (status == "approved") {
                    statusLabel->setStyleSheet("color: green; font-weight: bold;");
                } else if (status == "rejected") {
                    statusLabel->setStyleSheet("color: red; font-weight: bold;");
                }
                opLayout->addWidget(statusLabel);
            }

            m_approveTable->setCellWidget(row, 7, opWidget);
            row++;
        }
    }

    // ✅ 强制刷新视图
    m_approveTable->viewport()->update();

    // ✅ 调整列宽
    m_approveTable->resizeColumnsToContents();
    m_approveTable->horizontalHeader()->setStretchLastSection(true);
}

QString ReservationWidget::getPlaceNameById(const QString &placeId) {
    // 从m_placeComboApply中查找对应名称
    int index = m_placeComboApply->findData(placeId);
    if (index >= 0) {
        return m_placeComboApply->itemText(index);
    }
    return placeId; // 如果没找到，返回ID本身
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

    // ✅ 修复：使用已连接的 reservationQueryRequested 信号
    if (index == 2 && m_userRole == "admin") {
        qDebug() << "DEBUG: 切换到审批页，请求加载所有预约记录";

        // 发出查询所有场所的请求（包括已审批的）
        emit reservationQueryRequested("all");
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

    // 从combo box的itemData中获取预存的设备列表（需在加载场所时存储）
    QVariant placeData = m_placeComboApply->currentData(Qt::UserRole + 1);
    QStringList equipmentList = placeData.toStringList();

    if (equipmentList.isEmpty()) {
        m_equipmentListText->setText("该场所暂无设备信息");
    } else {
        m_equipmentListText->setText(equipmentList.join("\n"));
    }
}

// ✅ 新增：根据placeId获取设备列表（从combo box缓存中获取）
QStringList ReservationWidget::getEquipmentListForPlace(const QString &placeId) const
{
    // 从m_placeComboApply的itemData中查找
    for (int i = 0; i < m_placeComboApply->count(); ++i) {
        if (m_placeComboApply->itemData(i).toString() == placeId) {
            QVariant placeData = m_placeComboApply->itemData(i, Qt::UserRole + 1);
            return placeData.toStringList();
        }
    }
    return QStringList();  // 未找到返回空列表
}
