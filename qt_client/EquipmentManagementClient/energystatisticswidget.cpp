
#include "energystatisticswidget.h"


#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QDateTime>
#include <QMessageBox>
#include <QFileDialog>
#include <QTextStream>
#include <QDebug>
#include <QtCharts>


EnergyStatisticsWidget::EnergyStatisticsWidget(QWidget *parent)
    : QWidget(parent)
{
    setupUI();

    // 修复：设置为独立窗口，避免背景透明
    setWindowFlags(Qt::Window);
    setAttribute(Qt::WA_DeleteOnClose, false); // 防止关闭时析构

    // 添加窗口标题
    setWindowTitle("能耗统计分析");
}

void EnergyStatisticsWidget::setupUI()
{
    setWindowTitle("Equipment Energy Statistics");
    resize(1000, 700);

    // 创建控件
    m_equipmentCombo = new QComboBox(this);
    m_equipmentCombo->addItem("All Equipments", "all");

    m_timeRangeCombo = new QComboBox(this);
    m_timeRangeCombo->addItem("Day", "day");
    m_timeRangeCombo->addItem("Week", "week");
    m_timeRangeCombo->addItem("Month", "month");
    m_timeRangeCombo->addItem("Year", "year");

    m_startDateEdit = new QDateEdit(QDate::currentDate().addDays(-7), this);
    m_endDateEdit = new QDateEdit(QDate::currentDate(), this);

    // 修复：必须设置objectName，否则MainWindow::findChild会返回nullptr导致崩溃
    m_startDateEdit->setObjectName("m_startDateEdit");
    m_endDateEdit->setObjectName("m_endDateEdit");

    m_queryButton = new QPushButton("Query", this);
    m_exportButton = new QPushButton("Export CSV", this);

    m_statisticsTable = new QTableWidget(0, 5, this);
    m_statisticsTable->setHorizontalHeaderLabels({"Equipment ID", "Date", "Energy (kWh)", "Avg Power (W)", "Cost (¥)"});
    m_statisticsTable->horizontalHeader()->setStretchLastSection(true);
    m_statisticsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);

    m_chartView = new QChartView(this);
    m_chartView->setRenderHint(QPainter::Antialiasing);

    // 布局
    QFormLayout *formLayout = new QFormLayout();
    formLayout->addRow("Equipment:", m_equipmentCombo);
    formLayout->addRow("Time Range:", m_timeRangeCombo);
    formLayout->addRow("Start Date:", m_startDateEdit);
    formLayout->addRow("End Date:", m_endDateEdit);

    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addWidget(m_queryButton);
    buttonLayout->addWidget(m_exportButton);
    buttonLayout->addStretch();

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->addLayout(formLayout);
    mainLayout->addLayout(buttonLayout);
    mainLayout->addWidget(m_statisticsTable);
    mainLayout->addWidget(m_chartView);

    // 连接信号
    connect(m_queryButton, &QPushButton::clicked, this, &EnergyStatisticsWidget::onQueryButtonClicked);
    connect(m_exportButton, &QPushButton::clicked, this, &EnergyStatisticsWidget::onExportButtonClicked);
}

void EnergyStatisticsWidget::setEquipmentList(const QStringList &equipmentIds)
{
    m_equipmentCombo->clear();
    m_equipmentCombo->addItem("All Equipments", "all");

    for (const QString &id : equipmentIds) {
        m_equipmentCombo->addItem(id, id);
    }
}

void EnergyStatisticsWidget::onQueryButtonClicked()
{
    QString equipmentId = m_equipmentCombo->currentData().toString();
    QString timeRange = m_timeRangeCombo->currentData().toString();

    // 构建符合协议的payload: "timeRange|startDate|endDate"
    QString startDate = m_startDateEdit->date().toString("yyyy-MM-dd");
    QString endDate = m_endDateEdit->date().toString("yyyy-MM-dd");

    qDebug() << "发送能耗查询请求:" << equipmentId << timeRange << startDate << endDate;

    // 发射信号，MainWindow会处理并发送到服务端
    emit energyQueryRequested(equipmentId, timeRange);
}

void EnergyStatisticsWidget::updateEnergyChart(const QString &data)
{
    qDebug() << "收到能耗数据:" << data;
    parseAndDisplayData(data);
}

void EnergyStatisticsWidget::parseAndDisplayData(const QString &data)
{
    // 清空表格
    m_statisticsTable->setRowCount(0);

    if (data.isEmpty() || data == "0") {
        QMessageBox::information(this, "查询结果", "暂无能耗数据（数据库可能为空）");
        return;
    }

    if (!data.contains("|")) {
        qDebug() << "警告：数据格式不正确，不包含分隔符:" << data;
        QMessageBox::warning(this, "数据错误", "服务端返回的数据格式不正确");
        return;
    }

    // 解析数据格式: "equipment_id|period|energy|avg_power|cost;..."
    QStringList records = data.split(';', Qt::SkipEmptyParts);

    qDebug() << "正在解析" << records.size() << "条能耗记录";

    for (int i = 0; i < records.size(); ++i) {
        QStringList fields = records[i].split('|');
        if (fields.size() >= 5) {
            m_statisticsTable->insertRow(i);
            for (int j = 0; j < 5; ++j) {
                QTableWidgetItem *item = new QTableWidgetItem(fields.value(j, "0"));
                m_statisticsTable->setItem(i, j, item);
            }
        } else {
            qDebug() << "警告：记录格式不正确:" << records[i];
        }
    }

    // 调整列宽
    m_statisticsTable->resizeColumnsToContents();
    m_statisticsTable->horizontalHeader()->setStretchLastSection(true);

    qDebug() << "能耗数据解析完成，共" << records.size() << "条记录";
}

void EnergyStatisticsWidget::onExportButtonClicked() {}
