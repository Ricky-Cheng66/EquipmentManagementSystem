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
#include <QMutex>  // 添加QMutex头文件
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

    // 新增槽函数
    void onPlaceQueryCardClicked(const QString &placeId);
    void onQuickReserveRequested(const QString &placeId);
    void onBackToPlaceList();

    void safeUpdateQueryResultTable(const QString &data);

private:
    void setupApplyTab();
    void setupQueryTab();
    void setupApproveTab();
    void setupPlaceListPage();        // 新增：设置场所列表页面
    void setupPlaceDetailPage();      // 新增：设置场所详情页面
    void refreshPlaceListView();      // 新增：刷新场所列表视图
    void refreshPlaceDetailView();    // 新增：刷新场所详情视图
    void clearPlaceListView();        // 新增：清空场所列表
    void calculatePlaceStats();       // 新增：计算场所统计数据

    void refreshQueryCardView();
    void clearQueryCardView();
    void refreshApproveCardView();
    void refreshQueryCardViewForPlace(const QString &placeId);
    void clearApproveCardView();
    void applyQueryFilters();
    void updatePlaceCardsLayout();

    // 新增：场所统计数据
    QMap<QString, int> m_placeReservationCount; // 场所ID -> 预约数量
    QMap<QString, QStringList> m_placeReservations; // 场所ID -> 预约记录列表
    // 新增：辅助函数声明
    QString detectPlaceType(const QString &placeName);
    void refreshApproveFilterPlaces();  // 刷新审批页场所筛选列表
    QString getPlaceTypeDisplayName(const QString &placeTypeCode);  // 获取场所类型显示名称
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
    QMap<QString, PlaceCard*> m_placeCards;
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

    // 新增：查询页两级导航
    QWidget *m_placeListPage;          // 场所列表页面（第一级）
    QWidget *m_placeDetailPage;        // 场所详情页面（第二级）
    QGridLayout *m_placeListLayout;    // 场所列表布局
    QVBoxLayout *m_placeDetailLayout;  // 场所详情布局
    QList<PlaceQueryCard*> m_placeQueryCards; // 场所查询卡片列表
    QString m_currentPlaceId;          // 当前选中的场所ID
    QString m_currentPlaceName;        // 当前选中的场所名称

    // 新增：场所详情页控件
    ReservationFilterToolBar *m_queryFilterBarDetail; // 场所详情页的筛选工具栏
    QLabel *m_placeDetailNameLabel;   // 场所详情页的场所名称标签
    QLabel *m_placeDetailStatsLabel;  // 场所详情页的统计信息标签

    // 查询页卡片相关
    QList<ReservationCard*> m_queryCards;
    QMap<QString, ReservationCard*> m_queryCardMap;

    // 刷新控制成员 - 这些是原代码中已经存在的
    bool m_isRefreshingQueryView;  // 原代码已有
    QTimer *m_refreshQueryTimer;
    QTimer *m_placeListRefreshTimer;   // 新增：场所列表刷新定时器

    // 新增：审批页刷新控制
    bool m_isRefreshingApproveView; // 新增：审批视图刷新状态

    // 新增：保护机制
    mutable QMutex m_refreshMutex;  // 用于保护刷新操作的互斥锁

    // 审批页新成员
    ReservationFilterToolBar *m_approveFilterBar;
    QWidget *m_approveCardContainer;
    QVBoxLayout *m_approveCardLayout;
    QList<ReservationCard*> m_approveCards;
    QMap<QString, ReservationCard*> m_approveCardMap;
    QCheckBox *m_selectAllCheck;
    QPushButton *m_batchApproveButton;
    QPushButton *m_batchRejectButton;

    // 审批页原有控件
    QTableWidget *m_approveTable;

    QString m_currentUserId;
    QString m_userRole;

};

#endif // RESERVATIONWIDGET_H
