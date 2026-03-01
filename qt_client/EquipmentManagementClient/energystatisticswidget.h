#ifndef ENERGYSTATISTICSWIDGET_H
#define ENERGYSTATISTICSWIDGET_H

#include <QWidget>
#include <QComboBox>
#include <QPushButton>
#include <QTableWidget>
#include <QDateEdit>
#include <QGroupBox>

// 直接包含 Qt Charts 头文件，不加命名空间
#include <QtCharts/QChartView>
#include <QtCharts/QPieSeries>
#include <QtCharts/QPieSlice>
#include <QtCharts/QChart>

class EnergyStatisticsWidget : public QWidget
{
    Q_OBJECT

public:
    explicit EnergyStatisticsWidget(QWidget *parent = nullptr);

    void setDeviceInfoMap(const QHash<QString, QPair<QString, QString>> &map);
    void setDeviceTypeList(const QStringList &types);
    void setPlaceList(const QStringList &places);
    void setEquipmentList(const QStringList &equipmentIds);

    void autoQueryToday();  // 自动查询当天能耗

    QDate getStartDate() const;
    QDate getEndDate() const;

signals:
    void energyQueryRequested(const QString &equipmentId, const QString &timeRange);

public slots:
    void updateEnergyChart(const QString &data);

private slots:
    void onQueryButtonClicked();
    void onExportButtonClicked();
    void onTimeRangeChanged(int index);

private:
    void setupUI();
    void parseAndDisplayData(const QString &data);
    QStringList filterRecords(const QStringList &allRecords);
    QHash<QString, double> mergeSmallSlices(const QHash<QString, double> &data, double minPercentage);
    // 饼图绘制函数
    void drawTypePieChart(const QStringList &records);
    void drawPlacePieChart(const QStringList &records);

    // 控件
    QComboBox *m_equipmentCombo;
    QComboBox *m_timeRangeCombo;
    QDateEdit *m_startDateEdit;
    QPushButton *m_queryButton;
    QPushButton *m_exportButton;
    QTableWidget *m_statisticsTable;

    // 筛选控件
    QComboBox *m_typeCombo;
    QComboBox *m_placeCombo;

    // 饼图控件 - 使用 QChartView（无命名空间）
    QChartView *m_pieChartTypeView;
    QChartView *m_pieChartPlaceView;

    // 数据
    QHash<QString, QPair<QString, QString>> m_deviceInfoMap;
    QString m_selectedType;
    QString m_selectedPlace;
    QString m_currentTimeRange;
};

#endif // ENERGYSTATISTICSWIDGET_H
