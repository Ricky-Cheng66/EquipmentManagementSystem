#ifndef RESERVATIONCARD_H
#define RESERVATIONCARD_H

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPropertyAnimation>
#include <QMouseEvent>

class ReservationCard : public QWidget
{
    Q_OBJECT

public:
    explicit ReservationCard(const QString &reservationId, const QString &placeId, const QString &placeName,
                             const QString &userId, const QString &purpose,
                             const QString &startTime, const QString &endTime,
                             const QString &status, const QString &equipmentList,
                            const QString &applicantRole,
                             bool approveMode = false,
                             QWidget *parent = nullptr);

    // 获取预约信息的公共方法
    QString reservationId() const { return m_reservationId; }
    QString placeId() const { return m_placeId; }
    QString placeName() const { return m_placeName; }
    QString userId() const { return m_userId; }
    QString purpose() const { return m_purpose; }
    QString startTime() const { return m_startTime; }
    QString endTime() const { return m_endTime; }
    QString status() const { return m_status; }
    QString equipmentList() const { return m_equipmentList; }

    QString applicantRole() const { return m_applicantRole; }

    QDate getStartDate() const;
    QDate getEndDate() const;

    // 更新状态
    void updateStatus(const QString &status);

    // 设置选中状态
    void setSelected(bool selected);
    bool isSelected() const { return m_selected; }

signals:
    void cardClicked(const QString &reservationId);
    void statusActionRequested(const QString &reservationId, const QString &action);

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void paintEvent(QPaintEvent *event) override;

private slots:
    void onActionButtonClicked();

private:
    void setupUI();
    void updateCardStyle();
    QString getStatusColor(const QString &status) const;
    QString getStatusText(const QString &status) const;

    // 成员变量
    QString m_reservationId;
    QString m_placeId;
    QString m_placeName;
    QString m_userId;
    QString m_purpose;
    QString m_startTime;
    QString m_endTime;
    QString m_status;
    QString m_equipmentList;
    QString m_applicantRole;  // 新增：申请人角色
    bool m_selected;

    bool m_approveMode;

    // UI控件
    QWidget *m_contentWidget;
    QLabel *m_statusLabel;
    QLabel *m_idLabel;
    QLabel *m_placeLabel;
    QLabel *m_timeLabel;
    QLabel *m_purposeLabel;
    QLabel *m_equipmentLabel;
    QLabel *m_userLabel;
    QPushButton *m_actionButton;

    // 布局
    QVBoxLayout *m_mainLayout;
};

#endif // RESERVATIONCARD_H
