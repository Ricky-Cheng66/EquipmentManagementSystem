#include "energystatisticswidget.h"

// ----- 显式包含所有使用到的 Qt 类头文件 -----
#include <QLabel>
#include <QHeaderView>
#include <QFormLayout>
#include <QGroupBox>
#include <QDateEdit>
#include <QComboBox>
#include <QPushButton>
#include <QTableWidget>
#include <QToolTip>
#include <QCursor>

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QFileDialog>
#include <QDebug>
#include <QDate>

// Qt Charts 头文件
#include <QtCharts/QChartView>
#include <QtCharts/QPieSeries>
#include <QtCharts/QPieSlice>
#include <QtCharts/QChart>


EnergyStatisticsWidget::EnergyStatisticsWidget(QWidget *parent)
    : QWidget(parent)
{
    setupUI();
    // 初始化当前时间粒度（与下拉框默认值一致）
    m_currentTimeRange = "day";

    // 修复：设置为独立窗口，避免背景透明
    setWindowFlags(Qt::Window);
    setAttribute(Qt::WA_DeleteOnClose, false); // 防止关闭时析构

    // 添加窗口标题
    setWindowTitle("能耗统计分析");
}

void EnergyStatisticsWidget::setDeviceInfoMap(const QHash<QString, QPair<QString, QString>> &map)
{
    m_deviceInfoMap = map;
}

void EnergyStatisticsWidget::setDeviceTypeList(const QStringList &types)
{
    m_typeCombo->clear();
    m_typeCombo->addItem("全部类型", "all");
    for (const QString &type : types) {
        m_typeCombo->addItem(type, type);
    }
}

void EnergyStatisticsWidget::setPlaceList(const QStringList &places)
{
    m_placeCombo->clear();
    m_placeCombo->addItem("全部场所", "all");
    for (const QString &place : places) {
        m_placeCombo->addItem(place, place);
    }
}

void EnergyStatisticsWidget::setupUI()
{
    setWindowTitle("能耗统计分析");
    resize(1100, 800);  // 增加初始高度，给饼图预留空间

    // ----- 设备选择 -----
    m_equipmentCombo = new QComboBox(this);
    m_equipmentCombo->addItem("全部设备", "all");

    // ----- 时间粒度 -----
    m_timeRangeCombo = new QComboBox(this);
    m_timeRangeCombo->addItem("日", "day");
    m_timeRangeCombo->addItem("周", "week");
    m_timeRangeCombo->addItem("月", "month");
    m_timeRangeCombo->addItem("年", "year");

    // ----- 基准日期 -----
    QLabel *baseDateLabel = new QLabel("基准日期:", this);
    m_startDateEdit = new QDateEdit(QDate::currentDate(), this);
    m_startDateEdit->setDisplayFormat("yyyy-MM-dd");
    m_startDateEdit->setCalendarPopup(true);

    // ----- 设备类型筛选 -----
    QLabel *typeLabel = new QLabel("设备类型:", this);
    m_typeCombo = new QComboBox(this);
    m_typeCombo->addItem("全部类型", "all");

    // ----- 场所筛选 -----
    QLabel *placeLabel = new QLabel("场所:", this);
    m_placeCombo = new QComboBox(this);
    m_placeCombo->addItem("全部场所", "all");

    // ----- 按钮 -----
    m_queryButton = new QPushButton("查询", this);
    m_exportButton = new QPushButton("导出CSV", this);

    // ----- 表格 -----
    m_statisticsTable = new QTableWidget(0, 5, this);
    m_statisticsTable->setHorizontalHeaderLabels({"设备ID", "日期", "能耗(kWh)", "平均功率(W)", "费用(¥)"});
    m_statisticsTable->horizontalHeader()->setStretchLastSection(true);
    m_statisticsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);

    // ----- 饼图区域（关键修改）-----
    QHBoxLayout *pieLayout = new QHBoxLayout();
    pieLayout->setSpacing(15);                     // 饼图间距
    pieLayout->setContentsMargins(0, 10, 0, 10);   // 上下边距

    // 类型占比饼图
    QGroupBox *typeGroup = new QGroupBox("设备类型能耗占比", this);
    typeGroup->setMinimumHeight(300);              // 保证饼图显示高度
    typeGroup->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_pieChartTypeView = new QChartView(typeGroup);
    m_pieChartTypeView->setRenderHint(QPainter::Antialiasing);
    m_pieChartTypeView->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    QVBoxLayout *typeGroupLayout = new QVBoxLayout(typeGroup);
    typeGroupLayout->setContentsMargins(6, 6, 6, 6);
    typeGroupLayout->addWidget(m_pieChartTypeView);

    // 场所占比饼图
    QGroupBox *placeGroup = new QGroupBox("场所能耗占比", this);
    placeGroup->setMinimumHeight(300);
    placeGroup->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_pieChartPlaceView = new QChartView(placeGroup);
    m_pieChartPlaceView->setRenderHint(QPainter::Antialiasing);
    m_pieChartPlaceView->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    QVBoxLayout *placeGroupLayout = new QVBoxLayout(placeGroup);
    placeGroupLayout->setContentsMargins(6, 6, 6, 6);
    placeGroupLayout->addWidget(m_pieChartPlaceView);

    pieLayout->addWidget(typeGroup, 1);   // 拉伸因子1，等宽
    pieLayout->addWidget(placeGroup, 1);  // 拉伸因子1，等宽

    // ----- 表单布局（筛选栏压缩高度）-----
    QFormLayout *formLayout = new QFormLayout();
    formLayout->setContentsMargins(0, 0, 0, 0);   // 移除额外边距
    formLayout->setSpacing(8);                   // 紧凑布局
    formLayout->addRow("设备:", m_equipmentCombo);
    formLayout->addRow("时间粒度:", m_timeRangeCombo);
    formLayout->addRow(baseDateLabel, m_startDateEdit);
    formLayout->addRow(typeLabel, m_typeCombo);
    formLayout->addRow(placeLabel, m_placeCombo);

    // ----- 按钮布局 -----
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->setContentsMargins(0, 5, 0, 5);
    buttonLayout->addWidget(m_queryButton);
    buttonLayout->addWidget(m_exportButton);
    buttonLayout->addStretch();

    // ----- 主布局（设置拉伸权重）-----
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(10, 10, 10, 10);
    mainLayout->setSpacing(10);

    mainLayout->addLayout(formLayout);          // 筛选栏（不拉伸）
    mainLayout->addLayout(buttonLayout);        // 按钮栏（不拉伸）
    mainLayout->addLayout(pieLayout, 2);        // 饼图区域（拉伸因子2，获得更多空间）
    mainLayout->addWidget(m_statisticsTable, 1);// 表格（拉伸因子1）

    // ----- 信号连接 -----
    connect(m_queryButton, &QPushButton::clicked, this, &EnergyStatisticsWidget::onQueryButtonClicked);
    connect(m_exportButton, &QPushButton::clicked, this, &EnergyStatisticsWidget::onExportButtonClicked);
    connect(m_timeRangeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &EnergyStatisticsWidget::onTimeRangeChanged);

    m_currentTimeRange = "day";
}

void EnergyStatisticsWidget::setEquipmentList(const QStringList &equipmentIds)
{
    m_equipmentCombo->clear();
    m_equipmentCombo->addItem("All Equipments", "all");

    for (const QString &id : equipmentIds) {
        m_equipmentCombo->addItem(id, id);
    }
}

QDate EnergyStatisticsWidget::getStartDate() const
{
    QDate base = m_startDateEdit->date();
    if (m_currentTimeRange == "month") {
        return QDate(base.year(), base.month(), 1);
    } else if (m_currentTimeRange == "year") {
        return QDate(base.year(), 1, 1);
    } else if (m_currentTimeRange == "week") {
        // 自然周：周一为一周开始
        int dayOfWeek = base.dayOfWeek();  // Qt6: 周一=1, 周日=7
        return base.addDays(-(dayOfWeek - 1));
    }
    return base; // day 返回基准日期本身
}

QDate EnergyStatisticsWidget::getEndDate() const
{
    QDate base = m_startDateEdit->date();
    if (m_currentTimeRange == "day") {
        return base;
    } else if (m_currentTimeRange == "week") {
        // 自然周：周日为一周结束
        int dayOfWeek = base.dayOfWeek();
        return base.addDays(7 - dayOfWeek);
    } else if (m_currentTimeRange == "month") {
        return QDate(base.year(), base.month(), base.daysInMonth());
    } else if (m_currentTimeRange == "year") {
        return QDate(base.year(), 12, 31);
    }
    return base;
}

void EnergyStatisticsWidget::onQueryButtonClicked()
{
    // 存储当前筛选条件
    m_selectedType = m_typeCombo->currentData().toString();
    m_selectedPlace = m_placeCombo->currentData().toString();

    // 更新当前粒度（确保与下拉框一致）
    m_currentTimeRange = m_timeRangeCombo->currentData().toString();

    QString equipmentId = m_equipmentCombo->currentData().toString();
    emit energyQueryRequested(equipmentId, m_currentTimeRange);
}

void EnergyStatisticsWidget::updateEnergyChart(const QString &data)
{
    qDebug() << "收到能耗数据:" << data;
    parseAndDisplayData(data);
}

void EnergyStatisticsWidget::parseAndDisplayData(const QString &data)
{
    // 原始分割
    QStringList allRecords = data.split(';', Qt::SkipEmptyParts);
    // 应用本地过滤
    QStringList records = filterRecords(allRecords);

    // 清空表格
    m_statisticsTable->setRowCount(0);

    if (records.isEmpty()) {
        QMessageBox::information(this, "查询结果", "当前筛选条件下无数据");
        // 清空饼图
        m_pieChartTypeView->setChart(nullptr);
        m_pieChartPlaceView->setChart(nullptr);
        return;
    }

    // 填充表格（与原有逻辑一致）
    for (int i = 0; i < records.size(); ++i) {
        QStringList fields = records[i].split('|');
        if (fields.size() >= 5) {
            m_statisticsTable->insertRow(i);
            for (int j = 0; j < 5; ++j) {
                QTableWidgetItem *item = new QTableWidgetItem(fields.value(j, "0"));
                m_statisticsTable->setItem(i, j, item);
            }
        }
    }

    m_statisticsTable->resizeColumnsToContents();
    m_statisticsTable->horizontalHeader()->setStretchLastSection(true);

    // 绘制饼图（基于过滤后的数据）
    drawTypePieChart(records);
    drawPlacePieChart(records);

    qDebug() << "能耗数据解析完成，共" << records.size() << "条记录（过滤后）";
}

QStringList EnergyStatisticsWidget::filterRecords(const QStringList &allRecords)
{
    // 如果类型和场所都是“全部”，则不过滤（直接返回全部记录）
    if (m_selectedType == "all" && m_selectedPlace == "all") {
        return allRecords;
    }

    QStringList filtered;
    for (const QString &record : allRecords) {
        QStringList fields = record.split('|');
        if (fields.size() < 1) continue;
        QString devId = fields[0];
        auto it = m_deviceInfoMap.find(devId);

        // 设备不在映射表中：若筛选条件非“全部”，则丢弃（无法匹配）
        // 若映射表缺失严重，此处可能导致大量丢弃；可根据需要调整策略
        if (it == m_deviceInfoMap.end()) {
            // 如果类型或场所指定了非“全部”，无法判断，只能丢弃
            if (m_selectedType != "all" || m_selectedPlace != "all") {
                continue;
            }
            // 如果都是“全部”，本应在上面直接返回，不会走到这里
        }

        bool typeMatch = true, placeMatch = true;
        if (it != m_deviceInfoMap.end()) {
            QString devType = it.value().first;
            QString devPlace = it.value().second;
            typeMatch = (m_selectedType == "all" || devType == m_selectedType);
            placeMatch = (m_selectedPlace == "all" || devPlace == m_selectedPlace);
        }

        if (typeMatch && placeMatch) {
            filtered << record;
        }
    }
    return filtered;
}

QHash<QString, double> EnergyStatisticsWidget::mergeSmallSlices(const QHash<QString, double> &data, double minPercentage)
{
    double total = 0;
    for (double v : data) total += v;

    QHash<QString, double> merged;
    double otherEnergy = 0.0;
    int otherCount = 0;

    for (auto it = data.begin(); it != data.end(); ++it) {
        double p = (it.value() / total) * 100.0;
        if (p >= minPercentage) {
            merged.insert(it.key(), it.value());
        } else {
            otherEnergy += it.value();
            otherCount++;
        }
    }

    if (otherEnergy > 0.001) {
        merged.insert(QString("其他 (%1项)").arg(otherCount), otherEnergy);
    }
    return merged;
}

// ---------- 饼图绘制函数（完全无命名空间）----------

void EnergyStatisticsWidget::drawTypePieChart(const QStringList &records)
{
    QHash<QString, double> typeEnergy;
    double unknownEnergy = 0.0;
    int unknownCount = 0;

    for (const QString &record : records) {
        QStringList fields = record.split('|');
        if (fields.size() < 3) continue;
        QString devId = fields[0];
        double energy = fields[2].toDouble();

        auto it = m_deviceInfoMap.find(devId);
        if (it != m_deviceInfoMap.end()) {
            QString type = it.value().first;
            typeEnergy[type] += energy;
        } else {
            unknownEnergy += energy;
            unknownCount++;
        }
    }

    if (unknownEnergy > 0.001) {
        typeEnergy.insert(QString("未知类型 (%1)").arg(unknownCount), unknownEnergy);
    }

    if (typeEnergy.isEmpty()) {
        m_pieChartTypeView->setChart(nullptr);
        return;
    }

    // 可选：合并极小切片
    typeEnergy = mergeSmallSlices(typeEnergy, 1.0);

    QPieSeries *series = new QPieSeries();
    double totalEnergy = 0.0;
    for (auto it = typeEnergy.begin(); it != typeEnergy.end(); ++it) {
        totalEnergy += it.value();
    }

    for (auto it = typeEnergy.begin(); it != typeEnergy.end(); ++it) {
        QPieSlice *slice = series->append(it.key(), it.value());

        double percentage = (it.value() / totalEnergy) * 100.0;
        QString percentText;
        if (percentage < 0.1) {
            percentText = "<0.1%";
        } else {
            percentText = QString::number(percentage, 'f', 1) + "%";
        }

        slice->setLabelVisible(true);
        slice->setLabel(QString("%1\n%2").arg(it.key()).arg(percentText));


        // 鼠标悬停提示
        QString tooltipText = QString("%1: %2 kWh (%3%)")
                                  .arg(it.key())
                                  .arg(it.value(), 0, 'f', 2)
                                  .arg(percentage, 0, 'f', 2);
        connect(slice, &QPieSlice::hovered, this, [tooltipText](bool hovered){
            if (hovered) QToolTip::showText(QCursor::pos(), tooltipText);
            else QToolTip::hideText();
        });
    }

    QChart *chart = new QChart();
    chart->addSeries(series);
    chart->setTitle(QString("设备类型能耗占比 (总计: %1 kWh)").arg(totalEnergy, 0, 'f', 2));
    chart->setAnimationOptions(QChart::SeriesAnimations);
    chart->legend()->setVisible(true);
    chart->legend()->setAlignment(Qt::AlignRight);
    chart->legend()->setLabelColor(Qt::black);
    chart->legend()->setFont(QFont("Microsoft YaHei", 9));

    m_pieChartTypeView->setChart(chart);

    if (unknownCount > 0) {
        qWarning() << "[能耗] 类型饼图: 有" << unknownCount << "条记录的设备ID未在映射表中";
    }
}

void EnergyStatisticsWidget::drawPlacePieChart(const QStringList &records)
{
    QHash<QString, double> placeEnergy;
    double unknownEnergy = 0.0;
    int unknownCount = 0;

    for (const QString &record : records) {
        QStringList fields = record.split('|');
        if (fields.size() < 3) continue;
        QString devId = fields[0];
        double energy = fields[2].toDouble();

        auto it = m_deviceInfoMap.find(devId);
        if (it != m_deviceInfoMap.end()) {
            QString place = it.value().second;
            placeEnergy[place] += energy;
        } else {
            unknownEnergy += energy;
            unknownCount++;
        }
    }

    if (unknownEnergy > 0.001) {
        placeEnergy.insert(QString("未知场所 (%1)").arg(unknownCount), unknownEnergy);
    }

    if (placeEnergy.isEmpty()) {
        m_pieChartPlaceView->setChart(nullptr);
        return;
    }

    // 可选：合并占比 < 1% 的类别
    placeEnergy = mergeSmallSlices(placeEnergy, 1.0);

    QPieSeries *series = new QPieSeries();
    double totalEnergy = 0.0;
    for (auto it = placeEnergy.begin(); it != placeEnergy.end(); ++it) {
        totalEnergy += it.value();
    }

    for (auto it = placeEnergy.begin(); it != placeEnergy.end(); ++it) {
        QPieSlice *slice = series->append(it.key(), it.value());

        double percentage = (it.value() / totalEnergy) * 100.0;
        QString percentText;
        if (percentage < 0.1) {
            percentText = "<0.1%";
        } else {
            percentText = QString::number(percentage, 'f', 1) + "%";
        }

        slice->setLabelVisible(true);
        slice->setLabel(QString("%1\n%2").arg(it.key()).arg(percentText));

        QString tooltipText = QString("%1: %2 kWh (%3%)")
                                  .arg(it.key())
                                  .arg(it.value(), 0, 'f', 2)
                                  .arg(percentage, 0, 'f', 2);
        connect(slice, &QPieSlice::hovered, this, [tooltipText](bool hovered){
            if (hovered) QToolTip::showText(QCursor::pos(), tooltipText);
            else QToolTip::hideText();
        });
    }

    QChart *chart = new QChart();
    chart->addSeries(series);
    chart->setTitle(QString("场所能耗占比 (总计: %1 kWh)").arg(totalEnergy, 0, 'f', 2));
    chart->setAnimationOptions(QChart::SeriesAnimations);
    chart->legend()->setVisible(true);
    chart->legend()->setAlignment(Qt::AlignRight);
    chart->legend()->setLabelColor(Qt::black);
    chart->legend()->setFont(QFont("Microsoft YaHei", 9));

    m_pieChartPlaceView->setChart(chart);

    if (unknownCount > 0) {
        qWarning() << "[能耗] 场所饼图: 有" << unknownCount << "条记录的设备ID未在映射表中";
    }
}

void EnergyStatisticsWidget::onExportButtonClicked()
{
    // 1. 检查是否有数据
    if (m_statisticsTable->rowCount() == 0) {
        QMessageBox::information(this, "提示", "暂无可导出的数据");
        return;
    }

    // 2. 弹出文件保存对话框
    QString defaultName = QString("能耗统计_%1.csv").arg(QDate::currentDate().toString("yyyyMMdd"));
    QString filePath = QFileDialog::getSaveFileName(this, "导出CSV", defaultName, "CSV (*.csv)");

    if (filePath.isEmpty()) {
        return;
    }

    // 3. 打开文件
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "导出失败", "无法写入文件");
        return;
    }

    // 4. 写入UTF-8 BOM（Excel兼容性）
    file.write("\xEF\xBB\xBF");

    // 5. 写入表头
    QStringList headers = {"设备ID", "日期", "Energy (kWh)", "Avg Power (W)", "Cost (¥)"};
    file.write(headers.join(",").toUtf8() + "\n");

    // 6. 写入数据
    for (int row = 0; row < m_statisticsTable->rowCount(); ++row) {
        QStringList rowData;
        for (int col = 0; col < m_statisticsTable->columnCount(); ++col) {
            QString text = m_statisticsTable->item(row, col)->text();
            // CSV转义：处理逗号和引号
            if (text.contains(",") || text.contains("\"")) {
                text = "\"" + text.replace("\"", "\"\"") + "\"";
            }
            rowData << text;
        }
        file.write(rowData.join(",").toUtf8() + "\n");
    }

    file.close();
    QMessageBox::information(this, "导出成功", "数据已保存到:\n" + filePath);
}

void EnergyStatisticsWidget::onTimeRangeChanged(int index)
{
    if (index < 0) return;
    m_currentTimeRange = m_timeRangeCombo->itemData(index).toString();
    // 可选：基准日期复位到今天
    m_startDateEdit->setDate(QDate::currentDate());
}
