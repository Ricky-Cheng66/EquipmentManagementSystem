#ifndef ENERGYSTATISTICSWIDGET_H
#define ENERGYSTATISTICSWIDGET_H

#include <QWidget>
#include <QComboBox>
#include <QPushButton>
#include <QTableWidget>
#include <QChartView>
#include <QDateEdit>
#include "protocol_parser.h"

class EnergyStatisticsWidget : public QWidget
{
    Q_OBJECT

public:
    explicit EnergyStatisticsWidget(QWidget *parent = nullptr);

    void setEquipmentList(const QStringList &equipmentIds);

    // 添加获取日期的方法
    QDate getStartDate() const { return m_startDateEdit->date(); }
    QDate getEndDate() const { return m_endDateEdit->date(); }

signals:
    void energyQueryRequested(const QString &equipmentId, const QString &timeRange); // timeRange: "day/week/month/year"
    void equipmentEnergyQueryRequested(const QString &equipmentId);

public slots:
    void updateEnergyChart(const QString &data); // 接收服务端返回的能耗数据

private slots:
    void onQueryButtonClicked();
    void onExportButtonClicked();
    void onTimeRangeChanged(int index);

private:
    void setupUI();
    void parseAndDisplayData(const QString &data);

    QComboBox *m_equipmentCombo;
    QComboBox *m_timeRangeCombo;
    QDateEdit *m_startDateEdit;  // 改为QDateEdit
    QDateEdit *m_endDateEdit;    // 改为QDateEdit
    QPushButton *m_queryButton;
    QPushButton *m_exportButton;
    QTableWidget *m_statisticsTable;
    QChartView *m_chartView;

    QString m_currentTimeRange;
};

#endif // ENERGYSTATISTICSWIDGET_H
