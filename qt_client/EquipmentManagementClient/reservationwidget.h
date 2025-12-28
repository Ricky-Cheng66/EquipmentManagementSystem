#ifndef RESERVATIONWIDGET_H
#define RESERVATIONWIDGET_H

#include <QWidget>
#include <QTabWidget>
#include <QTableWidget>
#include <QComboBox>
#include <QDateTimeEdit>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include "protocol_parser.h"

class ReservationWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ReservationWidget(QWidget *parent = nullptr);
    void setUserRole(const QString &role, const QString &userId);
    void updateQueryResultTable(const QString &data);
    void loadPendingReservations(const QString &data);
    QComboBox *m_equipmentComboQuery;
    QComboBox *m_equipmentComboApply;
    QTabWidget *m_tabWidget;
signals:
    void reservationApplyRequested(const QString &equipmentId, const QString &purpose,
                                   const QString &startTime, const QString &endTime);
    void reservationQueryRequested(const QString &equipmentId);

    void reservationApproveRequested(int reservationId, bool approve);
private slots:
    void onApplyButtonClicked();
    void onQueryButtonClicked();
    void onApproveButtonClicked();
    void onDenyButtonClicked();

private:
    void setupApplyTab();
    void setupQueryTab();
    void setupApproveTab();



    // 申请页控件
    QDateTimeEdit *m_startTimeEdit;
    QDateTimeEdit *m_endTimeEdit;
    QLineEdit *m_purposeEdit;
    QPushButton *m_applyButton;

    // 查询页控件

    QPushButton *m_queryButton;
    QTableWidget *m_queryResultTable;

    // 审批页控件
    QTableWidget *m_approveTable;

    QString m_currentUserId;
    QString m_userRole;
};

#endif // RESERVATIONWIDGET_H
