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
#include <QThread>
#include <QMap>
#include <QCheckBox>
#include <QTimer>
#include <QElapsedTimer>
#include <QMutex>
#include "protocol_parser.h"
#include "reservationcard.h"
#include "reservationfiltertoolbar.h"
#include "placecard.h"

// 前向声明
class PlaceCard;
class PlaceQueryCard;

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

    // 线程安全检查方法
    bool isInMainThread() const { return thread() == QThread::currentThread(); }
    //初始化检查函数
    bool isApprovePageInitialized() const;
    QComboBox *m_placeComboApply;
    QComboBox *m_placeComboQuery;
    QTabWidget *m_tabWidget;

protected:
    void resizeEvent(QResizeEvent *event) override;
    bool event(QEvent *event) override;

signals:
    void reservationApplyRequested(const QString &placeId, const QString &purpose,
                                   const QString &startTime, const QString &endTime);
    void reservationQueryRequested(const QString &placeId);
    void reservationApproveRequested(int reservationId, const QString &placeId, bool approve);
    void loadAllReservationsRequested();
    void placeListLoaded();

public slots:
    void updatePlaceCards();

private slots:
    void onTabChanged(int index);
    // 申请页
    void onApplyButtonClicked();

    // 查询页
    void onQueryButtonClicked();
    void onReservationCardClicked(const QString &reservationId);
    void onStatusActionRequested(const QString &reservationId, const QString &action);
    void onFilterChanged();
    void onRefreshQueryRequested();
    void onPlaceCardClicked(const QString &placeId);

    // 查询页二级导航
    void onPlaceQueryCardClicked(const QString &placeId);
    void onQuickReserveRequested(const QString &placeId);
    void onBackToPlaceList();

    // 审批页旧函数（可能需要删除）
    void onApproveButtonClicked();
    void onDenyButtonClicked();
    void onSelectAllChanged(int state);
    void onBatchApprove();
    void onBatchReject();
    void onApproveFilterChanged();
    void onApproveRefreshRequested();

    // 审批页新函数（两级导航）
    void onApprovePlaceCardClicked(const QString &placeId);
    void onApprovePlaceFilterChanged();
    void onApproveDetailFilterChanged();
    void onApproveBackToPlaceList();

    void safeUpdateQueryResultTable(const QString &data);

private:
    // 申请页初始化
    void setupApplyTab();

    // 查询页初始化
    void setupQueryTab();
    void setupPlaceListPage();
    void setupPlaceDetailPage();

    // 审批页初始化（新架构）
    void setupApproveTab();
    void setupApprovePlaceListPage();
    void setupApproveDetailPage();

    // 查询页刷新函数
    void refreshPlaceListView();
    void refreshPlaceDetailView();
    void refreshQueryCardView();
    void refreshQueryCardViewForPlace(const QString &placeId);
    void calculatePlaceStats();

    // 审批页刷新函数（新架构）
    void refreshApprovePlaceListView();
    void refreshApproveDetailView();
    void refreshCurrentApproveView();

    // 清理函数
    void clearPlaceListView();
    void clearQueryCardView();
    void clearApproveCardView();

    // 辅助函数
    void updatePlaceCardsLayout();
    QString detectPlaceType(const QString &placeName);
    void refreshApproveFilterPlaces();
    QString getPlaceTypeDisplayName(const QString &placeTypeCode);
    void recalculatePendingCounts();
    void removeLoadingLabels();
    // ==================== 成员变量 ====================

    // 当前用户信息
    QString m_currentUserId;
    QString m_userRole;

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

    // 申请页场所卡片
    QWidget *m_placeCardsContainer;
    QGridLayout *m_placeCardsLayout;
    QMap<QString, PlaceCard*> m_placeCards;
    QString m_selectedPlaceId;
    QTextEdit *m_selectedEquipmentText;

    // ==================== 查询页相关 ====================
    // 查询页筛选工具栏
    ReservationFilterToolBar *m_queryFilterBar;
    ReservationFilterToolBar *m_queryFilterBarDetail;

    // 查询页堆栈和页面
    QStackedWidget *m_queryViewStack;
    QWidget *m_placeListPage;
    QWidget *m_placeDetailPage;
    QGridLayout *m_placeListLayout;
    QVBoxLayout *m_placeDetailLayout;

    // 查询页卡片容器
    QScrollArea *m_queryScrollArea;
    QWidget *m_queryCardContainer;
    QVBoxLayout *m_queryCardLayout;

    // 查询页卡片列表
    QList<PlaceQueryCard*> m_placeQueryCards;
    QList<ReservationCard*> m_queryCards;
    QMap<QString, ReservationCard*> m_queryCardMap;

    // 查询页当前选中场所
    QString m_currentPlaceId;
    QString m_currentPlaceName;

    // 查询页标签
    QLabel *m_placeDetailNameLabel;
    QLabel *m_placeDetailStatsLabel;

    // ==================== 审批页相关 ====================
    // 审批页筛选工具栏（新架构）
    QList<ReservationCard*> m_allApproveCards;    // 所有待审批预约卡片（用于筛选）
    ReservationFilterToolBar *m_approveFilterBar;          // 审批页场所列表筛选
    ReservationFilterToolBar *m_approveDetailFilterBar;    // 审批页详情筛选

    // 审批页堆栈和页面（新架构）
    QStackedWidget *m_approveViewStack;
    QWidget *m_approvePlaceListPage;
    QWidget *m_approveDetailPage;
    QGridLayout *m_approvePlaceListLayout;
    QVBoxLayout *m_approveDetailLayout;
    QWidget *m_approvePlaceListContainer;

    // 审批页卡片列表（新架构）
    QList<PlaceQueryCard*> m_approvePlaceCards;
    QList<ReservationCard*> m_approveCards;
    QMap<QString, ReservationCard*> m_approveCardMap;

    // 审批页当前选中场所（新架构）
    QString m_currentApprovePlaceId;
    QString m_currentApprovePlaceName;

    // 审批页标签（新架构）
    QLabel *m_approvePendingCountLabel;
    QLabel *m_approvePlaceNameLabel;
    QLabel *m_approvePlaceStatsLabel;

    // 审批页批量操作控件
    QCheckBox *m_selectAllCheck;
    QPushButton *m_batchApproveButton;
    QPushButton *m_batchRejectButton;

    // ==================== 通用成员 ====================
    // 统计数据
    QMap<QString, int> m_placeReservationCount;
    QMap<QString, QStringList> m_placeReservations;
    QMap<QString, int> m_approvePlacePendingCount;

    // 定时器
    QTimer *m_placeListRefreshTimer;
    QTimer *m_approvePlaceListRefreshTimer;

    // 刷新状态控制
    bool m_isRefreshingQueryView;
    bool m_isRefreshingApproveView;

    // 保护机制
    mutable QMutex m_refreshMutex;

    // ==================== 旧控件（可能需要删除） ====================
    // 注意：以下控件可能在重构后不再需要，但暂时保留以保持编译通过
    QTableWidget *m_queryResultTable;
    QPushButton *m_queryButton;
    QTableWidget *m_approveTable;
    QWidget *m_approveCardContainer;
    QVBoxLayout *m_approveCardLayout;
};

#endif // RESERVATIONWIDGET_H
