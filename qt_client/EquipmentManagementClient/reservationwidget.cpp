#include "reservationwidget.h"

#include <QMessageBox>
#include <QHeaderView>
#include <QLabel>

#include <QTimer>

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

    m_approveTable = new QTableWidget(0, 9, this);

    m_approveTable->setHorizontalHeaderLabels({
        "预约ID", "场所", "用户ID", "用途", "开始时间", "结束时间", "状态", "包含设备", "操作"
    });
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

    // ✅ 第1步：彻底清空表格，但保留表头
    m_approveTable->clearContents();
    m_approveTable->setRowCount(0);

    // ✅ 第2步：重新定义9列表头（确保与setupApproveTab一致）
    m_approveTable->setHorizontalHeaderLabels({
        "预约ID", "场所", "用户ID", "用途", "开始时间", "结束时间", "状态", "包含设备", "操作"
    });

    // ✅ 第3步：处理空数据情况
    if (data.isEmpty() || data == "暂无预约记录" || data == "fail|暂无数据") {
        m_approveTable->insertRow(0);
        QTableWidgetItem *emptyItem = new QTableWidgetItem("暂无预约记录");
        emptyItem->setTextAlignment(Qt::AlignCenter);
        m_approveTable->setItem(0, 0, emptyItem);
        m_approveTable->setSpan(0, 0, 1, 9); // 跨9列显示
        qDebug() << "审批数据为空，显示提示信息";
        return;
    }

    // ✅ 第4步：解析并填充数据
    QStringList reservations = data.split(';', Qt::SkipEmptyParts);
    int validRows = 0;

    for (int i = 0; i < reservations.size(); ++i) {
        QStringList fields = reservations[i].split('|');
        // ✅ 保护：确保至少有7个基础字段
        if (fields.size() >= 7) {
            m_approveTable->insertRow(validRows);

            // 第0列：预约ID
            m_approveTable->setItem(validRows, 0, new QTableWidgetItem(fields[0]));

            // 第1列：场所名称（从ID转换）
            QString placeId = fields[1];
            QString placeName = getPlaceNameById(placeId);
            m_approveTable->setItem(validRows, 1, new QTableWidgetItem(placeName));

            // 第2-6列：其他信息
            for (int j = 2; j < 7; ++j) {
                m_approveTable->setItem(validRows, j, new QTableWidgetItem(fields[j]));
            }

            // 第7列：设备列表
            QStringList equipmentList = getEquipmentListForPlace(placeId);
            QString equipmentText = equipmentList.isEmpty() ? "无设备" : equipmentList.join(", ");
            m_approveTable->setItem(validRows, 7, new QTableWidgetItem(equipmentText));

            // 第8列：操作按钮（关键修复）
            QString status = fields[6].trimmed().toLower();
            QWidget *opWidget = new QWidget(this);
            QHBoxLayout *opLayout = new QHBoxLayout(opWidget);
            opLayout->setContentsMargins(5, 0, 5, 0);
            opLayout->setSpacing(5);

            // ✅ 增强判断：pending/待审批/未审批 都显示按钮
            if (status == "pending" || status == "待审批" || status == "未审批") {
                QPushButton *approveBtn = new QPushButton("批准", this);
                approveBtn->setProperty("reservationId", fields[0].toInt());
                approveBtn->setProperty("placeId", placeId);
                approveBtn->setStyleSheet("QPushButton { background-color: #4CAF50; color: white; padding: 5px 10px; border: none; border-radius: 3px; }");
                connect(approveBtn, &QPushButton::clicked, this, &ReservationWidget::onApproveButtonClicked);

                QPushButton *rejectBtn = new QPushButton("拒绝", this);
                rejectBtn->setProperty("reservationId", fields[0].toInt());
                rejectBtn->setProperty("placeId", placeId);
                rejectBtn->setStyleSheet("QPushButton { background-color: #f44336; color: white; padding: 5px 10px; border: none; border-radius: 3px; }");
                connect(rejectBtn, &QPushButton::clicked, this, &ReservationWidget::onDenyButtonClicked);

                opLayout->addWidget(approveBtn);
                opLayout->addWidget(rejectBtn);
            } else {
                // ✅ 已审批的显示状态标签
                QLabel *statusLabel = new QLabel(status.toUpper(), this);
                statusLabel->setAlignment(Qt::AlignCenter);
                if (status == "approved" || status == "通过") {
                    statusLabel->setStyleSheet("color: green; font-weight: bold;");
                } else if (status == "rejected" || status == "拒绝") {
                    statusLabel->setStyleSheet("color: red; font-weight: bold;");
                }
                opLayout->addWidget(statusLabel);
            }

            m_approveTable->setCellWidget(validRows, 8, opWidget); // ✅ 第9列索引为8
            validRows++;
        } else {
            qDebug() << "跳过格式错误的记录:" << reservations[i];
        }
    }

    // ✅ 第5步：调整列宽和样式
    m_approveTable->resizeColumnsToContents();
    m_approveTable->horizontalHeader()->setStretchLastSection(false);
    m_approveTable->setColumnWidth(0, 80);   // 预约ID
    m_approveTable->setColumnWidth(1, 120);  // 场所
    m_approveTable->setColumnWidth(2, 80);   // 用户ID
    m_approveTable->setColumnWidth(4, 140);  // 开始时间
    m_approveTable->setColumnWidth(5, 140);  // 结束时间
    m_approveTable->setColumnWidth(6, 80);   // 状态
    m_approveTable->setColumnWidth(7, 200);  // 设备列表
    m_approveTable->setColumnWidth(8, 120);  // 操作

    qDebug() << "审批数据加载完成，有效行数:" << validRows;
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


    // ✅ 切换到审批页（索引2）时，自动请求所有预约数据
    if (index == 2 && m_userRole == "admin") {
        qDebug() << "DEBUG: 切换到审批页，准备请求数据...";

        // ✅ 延迟100ms确保页面渲染完成
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


