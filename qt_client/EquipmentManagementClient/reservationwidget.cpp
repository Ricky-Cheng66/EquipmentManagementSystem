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
    QVBoxLayout *mainLayout = new QVBoxLayout(applyTab);
    mainLayout->setSpacing(16);
    mainLayout->setContentsMargins(20, 20, 20, 20);

    // 基本信息表单
    QFormLayout *formLayout = new QFormLayout();
    formLayout->setSpacing(16);
    formLayout->setContentsMargins(0, 0, 0, 0);

    // 场所选择
    QLabel *placeLabel = new QLabel("场所:", this);
    m_placeComboApply = new QComboBox(this);
    m_placeComboApply->setProperty("class", "form-control");
    m_placeComboApply->setMinimumHeight(36);
    m_placeComboApply->setPlaceholderText("请选择场所");

    formLayout->addRow(placeLabel, m_placeComboApply);

    // 开始时间 - 使用分开的日期和时间选择
    QLabel *startLabel = new QLabel("开始时间:", this);

    // 创建开始时间容器
    QWidget *startTimeWidget = new QWidget(this);
    QHBoxLayout *startTimeLayout = new QHBoxLayout(startTimeWidget);
    startTimeLayout->setContentsMargins(0, 0, 0, 0);
    startTimeLayout->setSpacing(8);

    // 开始日期选择
    m_startDateEdit = new QDateEdit(QDate::currentDate(), this);
    m_startDateEdit->setProperty("class", "form-control");
    m_startDateEdit->setMinimumHeight(36);
    m_startDateEdit->setDisplayFormat("yyyy-MM-dd");
    m_startDateEdit->setCalendarPopup(true);
    m_startDateEdit->setDate(QDate::currentDate());
    m_startDateEdit->setMinimumDate(QDate::currentDate()); // 不能选择过去的日期

    // 开始时间选择 - 使用QTimeEdit并设置方便的时间间隔
    m_startTimeEdit = new QTimeEdit(this);
    m_startTimeEdit->setProperty("class", "form-control");
    m_startTimeEdit->setMinimumHeight(36);
    m_startTimeEdit->setDisplayFormat("HH:mm");
    m_startTimeEdit->setTime(QTime(QTime::currentTime().hour(), 0, 0)); // 整点

    // 设置时间步长为15分钟，让用户更方便选择
    m_startTimeEdit->setTimeSpec(Qt::LocalTime);

    startTimeLayout->addWidget(m_startDateEdit);
    startTimeLayout->addWidget(new QLabel(" ", this));  // 分隔符
    startTimeLayout->addWidget(m_startTimeEdit);

    formLayout->addRow(startLabel, startTimeWidget);

    // 结束时间 - 同样的控件
    QLabel *endLabel = new QLabel("结束时间:", this);

    QWidget *endTimeWidget = new QWidget(this);
    QHBoxLayout *endTimeLayout = new QHBoxLayout(endTimeWidget);
    endTimeLayout->setContentsMargins(0, 0, 0, 0);
    endTimeLayout->setSpacing(8);

    m_endDateEdit = new QDateEdit(QDate::currentDate(), this);
    m_endDateEdit->setProperty("class", "form-control");
    m_endDateEdit->setMinimumHeight(36);
    m_endDateEdit->setDisplayFormat("yyyy-MM-dd");
    m_endDateEdit->setCalendarPopup(true);
    m_endDateEdit->setDate(QDate::currentDate());
    m_endDateEdit->setMinimumDate(QDate::currentDate());

    m_endTimeEdit = new QTimeEdit(this);
    m_endTimeEdit->setProperty("class", "form-control");
    m_endTimeEdit->setMinimumHeight(36);
    m_endTimeEdit->setDisplayFormat("HH:mm");
    m_endTimeEdit->setTime(QTime(QTime::currentTime().hour() + 1, 0, 0)); // 1小时后

    endTimeLayout->addWidget(m_endDateEdit);
    endTimeLayout->addWidget(new QLabel(" ", this));
    endTimeLayout->addWidget(m_endTimeEdit);

    formLayout->addRow(endLabel, endTimeWidget);

    // 添加时间选择提示
    QLabel *timeHint = new QLabel("提示：日期请点击日历图标选择，时间请点击上下箭头或直接输入", this);
    timeHint->setStyleSheet("color: #666; font-size: 11px; font-style: italic;");
    formLayout->addRow("", timeHint);

    // 用途
    QLabel *purposeLabel = new QLabel("用途:", this);
    m_purposeEdit = new QLineEdit(this);
    m_purposeEdit->setProperty("class", "form-control");
    m_purposeEdit->setMinimumHeight(36);
    m_purposeEdit->setPlaceholderText("请输入预约用途");

    formLayout->addRow(purposeLabel, m_purposeEdit);

    // 提交按钮
    m_applyButton = new QPushButton("✓ 提交预约", this);
    m_applyButton->setProperty("class", "primary-button");
    m_applyButton->setMinimumHeight(40);
    m_applyButton->setMinimumWidth(120);

    // 按钮布局
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    buttonLayout->addWidget(m_applyButton);
    buttonLayout->addStretch();

    formLayout->addRow("", buttonLayout);

    mainLayout->addLayout(formLayout);

    // 设备列表分组框
    QGroupBox *equipmentGroup = new QGroupBox("场所包含设备", this);
    equipmentGroup->setStyleSheet(
        "QGroupBox {"
        "    font-weight: bold;"
        "    border: 1px solid #e0e0e0;"
        "    border-radius: 4px;"
        "    margin-top: 10px;"
        "    padding-top: 10px;"
        "}");

    QVBoxLayout *equipmentLayout = new QVBoxLayout(equipmentGroup);
    equipmentLayout->setContentsMargins(8, 20, 8, 8);

    m_equipmentListText = new QTextEdit(this);
    m_equipmentListText->setReadOnly(true);
    m_equipmentListText->setMinimumHeight(80);
    m_equipmentListText->setPlaceholderText("选择场所后自动加载设备列表");
    m_equipmentListText->setStyleSheet(
        "QTextEdit {"
        "    border: 1px solid #dcdde1;"
        "    border-radius: 3px;"
        "    background-color: #f8f9fa;"
        "    padding: 8px;"
        "    font-size: 12px;"
        "}");

    equipmentLayout->addWidget(m_equipmentListText);
    mainLayout->addWidget(equipmentGroup);
    mainLayout->addStretch();

    m_tabWidget->addTab(applyTab, "预约申请");

    connect(m_applyButton, &QPushButton::clicked, this, &ReservationWidget::onApplyButtonClicked);

    // 场所选择变化时自动加载设备列表
    connect(m_placeComboApply, QOverload<int>::of(&QComboBox::currentIndexChanged),
            [this](int index) {
                if (index >= 0) {
                    updateEquipmentListDisplay();
                }
            });
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

    // 彻底清空表格
    m_approveTable->clear();
    m_approveTable->setRowCount(0);
    m_approveTable->setColumnCount(0);

    // 重新设置表格
    m_approveTable->setColumnCount(9);
    m_approveTable->setHorizontalHeaderLabels({
        "预约ID", "场所", "用户ID", "用途", "开始时间", "结束时间", "状态", "包含设备", "操作"
    });

    // 处理空数据情况
    if (data.isEmpty() || data == "暂无预约记录" || data == "fail|暂无数据") {
        m_approveTable->insertRow(0);
        m_approveTable->setItem(0, 0, new QTableWidgetItem("暂无预约记录"));
        m_approveTable->item(0, 0)->setTextAlignment(Qt::AlignCenter);
        m_approveTable->setSpan(0, 0, 1, 9);
        return;
    }

    // 解析并填充数据
    QStringList reservations = data.split(';', Qt::SkipEmptyParts);
    int validRows = 0;

    for (int i = 0; i < reservations.size(); ++i) {
        QStringList fields = reservations[i].split('|');
        if (fields.size() >= 7) {
            m_approveTable->insertRow(validRows);

            // 填充前6列数据
            for (int j = 0; j < 6; ++j) {
                QTableWidgetItem *item = new QTableWidgetItem(fields[j]);
                item->setTextAlignment(Qt::AlignCenter);
                m_approveTable->setItem(validRows, j, item);
            }

            // 第6列：状态
            QString status = fields[6].trimmed().toLower();
            QString statusDisplay;

            if (status == "approved" || status == "通过") {
                statusDisplay = "已批准";
            } else if (status == "rejected" || status == "拒绝") {
                statusDisplay = "已拒绝";
            } else if (status == "pending" || status == "待审批" || status == "未审批") {
                statusDisplay = "待审批";
            } else {
                statusDisplay = status;
            }

            QTableWidgetItem *statusItem = new QTableWidgetItem(statusDisplay);
            statusItem->setTextAlignment(Qt::AlignCenter);

            if (statusDisplay == "已批准") {
                statusItem->setForeground(QBrush(QColor("#27ae60")));
                statusItem->setFont(QFont("Microsoft YaHei", 9, QFont::Bold));
            } else if (statusDisplay == "已拒绝") {
                statusItem->setForeground(QBrush(QColor("#e74c3c")));
                statusItem->setFont(QFont("Microsoft YaHei", 9, QFont::Bold));
            } else if (statusDisplay == "待审批") {
                statusItem->setForeground(QBrush(QColor("#f39c12")));
                statusItem->setFont(QFont("Microsoft YaHei", 9, QFont::Bold));
            }

            m_approveTable->setItem(validRows, 6, statusItem);

            // 第7列：设备列表
            QString placeId = fields[1];
            QStringList equipmentList = getEquipmentListForPlace(placeId);
            QString equipmentText = equipmentList.isEmpty() ? "无设备" : equipmentList.join(", ");
            QTableWidgetItem *equipmentItem = new QTableWidgetItem(equipmentText);
            equipmentItem->setTextAlignment(Qt::AlignCenter);
            m_approveTable->setItem(validRows, 7, equipmentItem);

            // 第8列：操作按钮 - 使用更大的容器确保按钮完整显示
            QWidget *opWidget = new QWidget(this);
            QHBoxLayout *opLayout = new QHBoxLayout(opWidget);
            opLayout->setContentsMargins(2, 2, 2, 2);  // 减少内边距
            opLayout->setSpacing(4);
            opLayout->setAlignment(Qt::AlignCenter);

            // 设置操作列的最小宽度
            opWidget->setMinimumWidth(140);
            opWidget->setMaximumWidth(160);

            // 判断是否显示操作按钮
            if (status == "pending" || status == "待审批" || status == "未审批") {
                // 批准按钮 - 使用更紧凑的样式
                QPushButton *approveBtn = new QPushButton("批准", opWidget);
                approveBtn->setFixedSize(55, 26);  // 减小按钮尺寸
                approveBtn->setStyleSheet(
                    "QPushButton {"
                    "    background-color: #27ae60;"
                    "    color: white;"
                    "    border: none;"
                    "    border-radius: 3px;"
                    "    padding: 2px 4px;"
                    "    font-size: 10px;"
                    "    font-weight: bold;"
                    "}"
                    "QPushButton:hover {"
                    "    background-color: #219653;"
                    "}"
                    "QPushButton:pressed {"
                    "    background-color: #1e8449;"
                    "}");
                approveBtn->setProperty("reservationId", fields[0].toInt());
                approveBtn->setProperty("placeId", placeId);
                connect(approveBtn, &QPushButton::clicked, this, &ReservationWidget::onApproveButtonClicked);

                // 拒绝按钮
                QPushButton *rejectBtn = new QPushButton("拒绝", opWidget);
                rejectBtn->setFixedSize(55, 26);
                rejectBtn->setStyleSheet(
                    "QPushButton {"
                    "    background-color: #e74c3c;"
                    "    color: white;"
                    "    border: none;"
                    "    border-radius: 3px;"
                    "    padding: 2px 4px;"
                    "    font-size: 10px;"
                    "    font-weight: bold;"
                    "}"
                    "QPushButton:hover {"
                    "    background-color: #c0392b;"
                    "}"
                    "QPushButton:pressed {"
                    "    background-color: #a93226;"
                    "}");
                rejectBtn->setProperty("reservationId", fields[0].toInt());
                rejectBtn->setProperty("placeId", placeId);
                connect(rejectBtn, &QPushButton::clicked, this, &ReservationWidget::onDenyButtonClicked);

                opLayout->addWidget(approveBtn);
                opLayout->addWidget(rejectBtn);
            } else {
                // 已审批的显示状态标签
                QLabel *statusLabel = new QLabel(statusDisplay, opWidget);
                statusLabel->setAlignment(Qt::AlignCenter);
                statusLabel->setFixedSize(70, 24);

                if (statusDisplay == "已批准") {
                    statusLabel->setStyleSheet(
                        "QLabel {"
                        "    color: #27ae60;"
                        "    font-weight: bold;"
                        "    background-color: #e8f6f3;"
                        "    border: 1px solid #27ae60;"
                        "    border-radius: 3px;"
                        "    padding: 2px 4px;"
                        "    font-size: 10px;"
                        "}");
                } else if (statusDisplay == "已拒绝") {
                    statusLabel->setStyleSheet(
                        "QLabel {"
                        "    color: #e74c3c;"
                        "    font-weight: bold;"
                        "    background-color: #fdedec;"
                        "    border: 1px solid #e74c3c;"
                        "    border-radius: 3px;"
                        "    padding: 2px 4px;"
                        "    font-size: 10px;"
                        "}");
                } else {
                    statusLabel->setStyleSheet(
                        "QLabel {"
                        "    color: #7f8c8d;"
                        "    font-weight: bold;"
                        "    background-color: #ecf0f1;"
                        "    border: 1px solid #bdc3c7;"
                        "    border-radius: 3px;"
                        "    padding: 2px 4px;"
                        "    font-size: 10px;"
                        "}");
                }
                opLayout->addWidget(statusLabel);
            }

            // 设置操作列样式
            opWidget->setStyleSheet("background-color: transparent;");
            m_approveTable->setCellWidget(validRows, 8, opWidget);
            validRows++;
        }
    }

    // 调整列宽，为操作列留出更多空间
    m_approveTable->resizeColumnsToContents();
    m_approveTable->horizontalHeader()->setStretchLastSection(false);

    // 设置最小列宽，确保操作列有足够空间
    m_approveTable->setColumnWidth(0, 60);   // 预约ID
    m_approveTable->setColumnWidth(1, 90);   // 场所
    m_approveTable->setColumnWidth(2, 60);   // 用户ID
    m_approveTable->setColumnWidth(3, 120);  // 用途
    m_approveTable->setColumnWidth(4, 130);  // 开始时间
    m_approveTable->setColumnWidth(5, 130);  // 结束时间
    m_approveTable->setColumnWidth(6, 70);   // 状态
    m_approveTable->setColumnWidth(7, 160);  // 设备列表
    m_approveTable->setColumnWidth(8, 120);  // 操作（减小宽度）

    // 设置表格整体样式
    m_approveTable->setStyleSheet(
        "QTableWidget {"
        "    border: 1px solid #e0e0e0;"
        "    background-color: white;"
        "    alternate-background-color: #f9f9f9;"
        "    gridline-color: #f0f0f0;"
        "}"
        "QTableWidget::item {"
        "    padding: 4px;"
        "    border-bottom: 1px solid #f0f0f0;"
        "    font-size: 10px;"
        "}"
        "QTableWidget::item:selected {"
        "    background-color: #e3f2fd;"
        "    color: #1976d2;"
        "}"
        "QHeaderView::section {"
        "    background-color: #f5f6fa;"
        "    padding: 6px;"
        "    border: none;"
        "    border-right: 1px solid #e0e0e0;"
        "    border-bottom: 2px solid #e0e0e0;"
        "    font-weight: bold;"
        "    font-size: 10px;"
        "}");
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


