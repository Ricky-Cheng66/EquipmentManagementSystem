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
    void loadAllReservationsForApproval(const QString &data);
    QString getPlaceNameById(const QString &placeId);

    // ✅ 新增：获取当前选中的预约记录的场所ID
    QString getCurrentSelectedPlaceId() const;
    // 改为：
    QComboBox *m_placeComboApply;  // 改动：重命名
    QComboBox *m_placeComboQuery;  // 改动：重命名
    QTabWidget *m_tabWidget;
signals:
    void reservationApplyRequested(const QString &placeId, const QString &purpose,
                                   const QString &startTime, const QString &endTime);
    void reservationQueryRequested(const QString &placeId);

    void reservationApproveRequested(int reservationId, bool approve);
    void loadAllReservationsRequested();
private slots:
    void onApplyButtonClicked();
    void onQueryButtonClicked();
    void onApproveButtonClicked();
    void onDenyButtonClicked();
    void onTabChanged(int index);

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
