#ifndef ALARMWIDGET_H
#define ALARMWIDGET_H

#include <QWidget>
#include <QList>
#include <QDateTime>
#include <QPushButton>
#include <QScrollArea>
#include <QVBoxLayout>

// 告警信息结构体
struct AlarmInfo {
    int id;                     // 告警ID（可用时间戳临时替代）
    QString type;               // 告警类型 offline/energy_threshold
    QString equipmentId;
    QString severity;           // warning/error/critical
    QString message;
    QDateTime timestamp;
    bool acknowledged;          // 是否已处理
};

class AlarmWidget : public QWidget
{
    Q_OBJECT
public:
    explicit AlarmWidget(QWidget *parent = nullptr);

    // 批量设置告警列表（用于初始化）
    void setAlarms(const QList<AlarmInfo> &alarms);
    // 添加单条新告警（自动去重）
    void addAlarm(const AlarmInfo &alarm);

signals:
    // 用户点击“标记处理”时发出的信号
    void acknowledgeAlarm(int alarmId);

private slots:
    void onShowAllToggled(bool checked);   // 切换“全部”/“未处理”
    void onMarkProcessedClicked(int alarmId);

private:
    void setupUI();
    void refreshDisplay();                  // 刷新卡片显示

    QList<AlarmInfo> m_allAlarms;           // 所有告警（包括已处理）
    bool m_showAll;                          // true=显示全部，false=只显示未处理

    // UI控件
    QPushButton *m_btnUnprocessed;
    QPushButton *m_btnAll;
    QScrollArea *m_scrollArea;
    QWidget *m_containerWidget;
    QVBoxLayout *m_cardsLayout;
};

#endif // ALARMWIDGET_H
