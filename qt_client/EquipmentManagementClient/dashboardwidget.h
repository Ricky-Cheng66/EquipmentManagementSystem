#ifndef DASHBOARDWIDGET_H
#define DASHBOARDWIDGET_H

#include <QWidget>
#include <QLabel>
#include <QGridLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTextEdit>
#include <QMap>
#include <QDateTime>

class DashboardWidget : public QWidget
{
    Q_OBJECT
public:
    explicit DashboardWidget(const QString &role, QWidget *parent = nullptr);

    // 更新卡片数值
    void setTodayMyReservations(int count);
    void setTotalMyReservations(int count);
    void setPendingApprovalCount(int count);

    // 更新底部信息栏
    void setRecentMyReservations(const QStringList &items);      // 个人预约列表（格式：时间-场所-用途）
    void setRecentPendingReservations(const QStringList &items); // 待审批列表（仅老师）

signals:
    void cardClicked(int cardIndex); // 0:今日预约, 1:我的预约总数, 2:待审批

protected:
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void onCardClicked();

private:
    void setupUI();
    void updateCardStyles();

    QString m_role;
    bool m_showRightColumn; // 老师显示右侧列，学生隐藏

    // 卡片
    QWidget *m_cardToday;
    QLabel *m_todayCountLabel;
    QWidget *m_cardTotal;
    QLabel *m_totalCountLabel;
    QWidget *m_cardPending;
    QLabel *m_pendingCountLabel;

    // 底部信息栏
    QTextEdit *m_leftTextEdit;  // 个人预约
    QTextEdit *m_rightTextEdit; // 待审批（仅老师）
};

#endif // DASHBOARDWIDGET_H
