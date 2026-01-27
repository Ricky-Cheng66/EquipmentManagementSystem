#ifndef RESERVATIONWIDGET_H
#define RESERVATIONWIDGET_H

#include <QWidget>
#include <QTabWidget>
#include <QLabel>
#include <QTextEdit>
#include <QTableWidget>
#include <QComboBox>
#include <QGroupBox>
#include <QDateTimeEdit>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QScrollArea>
#include <QGridLayout>
#include <QStackedWidget>
#include <QMap>
#include <QCheckBox>
#include "protocol_parser.h"
#include "reservationcard.h"
#include "reservationfiltertoolbar.h"

// 前向声明
class PlaceCard;

class ReservationWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ReservationWidget(QWidget *parent = nullptr);
    ~ReservationWidget();

    void setUserRole(const QString &role, const QString &userId);
    void updateQueryResultTable(const QString &data);
    void loadAllReservationsForApproval(const QString &data);
    void refreshCurrentPlaceEquipment();
    void clearEquipmentList();
    QString getCurrentSelectedPlaceId() const;
    int getCurrentSelectedReservationId() const;
    QString getPlaceNameById(const QString &placeId);
    QStringList getEquipmentListForPlace(const QString &placeId) const;
    void updateEquipmentListDisplay();


    QComboBox *m_placeComboApply;
    QComboBox *m_placeComboQuery;
    QTabWidget *m_tabWidget;
protected:

    void resizeEvent(QResizeEvent *event) override;

signals:
    void reservationApplyRequested(const QString &placeId, const QString &purpose,
                                   const QString &startTime, const QString &endTime);
    void reservationQueryRequested(const QString &placeId);
    void reservationApproveRequested(int reservationId, const QString &placeId, bool approve);
    void loadAllReservationsRequested();
    void placeListLoaded();
public slots:
    // 更新场所卡片的公有方法
    void updatePlaceCards();
private slots:
    void onApplyButtonClicked();
    void onQueryButtonClicked();
    void onApproveButtonClicked();
    void onDenyButtonClicked();
    void onTabChanged(int index);
    void onReservationCardClicked(const QString &reservationId);
    void onStatusActionRequested(const QString &reservationId, const QString &action);
    void onFilterChanged();
    void onRefreshQueryRequested();

    void onPlaceCardClicked(const QString &placeId);
    void onSelectAllChanged(int state);
    void onBatchApprove();
    void onBatchReject();
    void onApproveFilterChanged();
    void onApproveRefreshRequested();

private:
    void setupApplyTab();
    void setupQueryTab();
    void setupApproveTab();
    void setupQueryCardView();
    void refreshQueryCardView();
    void clearQueryCardView();
    void refreshApproveCardView();
    void clearApproveCardView();
    void applyQueryFilters();
    void updatePlaceCardsLayout();

    // 申请页控件
    QLabel *m_equipmentListLabel;
    QTextEdit *m_equipmentListText;
    QGroupBox *m_equipmentGroup;
    QDateEdit *m_startDateEdit;
    QTimeEdit *m_startTimeEdit;
    QDateEdit *m_endDateEdit;
    QTimeEdit *m_endTimeEdit;
    QLineEdit *m_purposeEdit;
    QPushButton *m_applyButton;

    // 申请页新成员
    QWidget *m_placeCardsContainer;
    QGridLayout *m_placeCardsLayout;
    QMap<QString, PlaceCard*> m_placeCards;  // 修正为正确的类型
    QString m_selectedPlaceId;
    QTextEdit *m_selectedEquipmentText;

    // 查询页控件
    ReservationFilterToolBar *m_queryFilterBar;
    QScrollArea *m_queryScrollArea;
    QWidget *m_queryCardContainer;
    QVBoxLayout *m_queryCardLayout;
    QPushButton *m_queryButton;
    QStackedWidget *m_queryViewStack;
    QTableWidget *m_queryResultTable;

    // 查询页卡片相关
    QList<ReservationCard*> m_queryCards;
    QMap<QString, ReservationCard*> m_queryCardMap;

    // 审批页新成员
    ReservationFilterToolBar *m_approveFilterBar;
    QWidget *m_approveCardContainer;
    QVBoxLayout *m_approveCardLayout;
    QList<ReservationCard*> m_approveCards;
    QMap<QString, ReservationCard*> m_approveCardMap;
    QCheckBox *m_selectAllCheck;  // 修正为 QCheckBox*
    QPushButton *m_batchApproveButton;
    QPushButton *m_batchRejectButton;

    // 审批页原有控件
    QTableWidget *m_approveTable;

    QString m_currentUserId;
    QString m_userRole;
};

#endif // RESERVATIONWIDGET_H
