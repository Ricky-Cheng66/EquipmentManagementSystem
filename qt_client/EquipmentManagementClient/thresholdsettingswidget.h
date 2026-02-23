#ifndef THRESHOLDSETTINGSWIDGET_H
#define THRESHOLDSETTINGSWIDGET_H

#include <QWidget>
#include <QComboBox>
#include <QLineEdit>
#include <QPushButton>
#include <QTableWidget>
#include <QLabel>
#include <QHash>

class ThresholdSettingsWidget : public QWidget
{
    Q_OBJECT
public:
    explicit ThresholdSettingsWidget(QWidget *parent = nullptr);

    // 设置设备列表（设备ID -> 显示名称）
    void setEquipmentList(const QHash<QString, QString> &devices);
    // 设置当前已有阈值（从服务端加载）
    void setCurrentThresholds(const QHash<QString, double> &thresholds);

signals:
    // 发送设置阈值请求
    void setThresholdRequested(const QString &equipmentId, double value);

public slots:
    // 处理设置响应
    void handleSetThresholdResponse(bool success, const QString &message);

private slots:
    void onApplyButtonClicked();
    void onDeviceSelected(int index);

private:
    void setupUI();
    void updateThresholdTable();

    QComboBox *m_deviceCombo;
    QLineEdit *m_thresholdEdit;
    QPushButton *m_applyButton;
    QLabel *m_statusLabel;
    QTableWidget *m_thresholdTable;

    QHash<QString, QString> m_deviceMap;      // id -> display name
    QHash<QString, double> m_thresholdMap;    // id -> current threshold
};

#endif // THRESHOLDSETTINGSWIDGET_H
