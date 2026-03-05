#ifndef RESERVATIONFILTERTOOLBAR_H
#define RESERVATIONFILTERTOOLBAR_H

#include <QWidget>
#include <QComboBox>
#include <QPushButton>
#include <QLineEdit>
#include <QHBoxLayout>
#include <QDateEdit>
#include <QTimer>
#include <QLabel>

class ReservationFilterToolBar : public QWidget
{
    Q_OBJECT

public:
    explicit ReservationFilterToolBar(QWidget *parent = nullptr);

    void setStatusComboDefault(const QString &status);
    QPushButton *refreshButton() const { return m_refreshButton; }
    // 获取筛选条件
    QString selectedPlace() const;
    QString selectedStatus() const;
    QString selectedDate() const;
    QString searchText() const;
    QString selectedPlaceType() const;     // 场所类型筛选
    QString selectedRole() const; // 新增：获取选中角色
    // 设置选项
    void setPlaces(const QStringList &places);
    void setStatuses(const QStringList &statuses);
    void setPlaceTypes(const QStringList &types);  // 设置场所类型

    // 获取日期范围
    QDate startDate() const;
    QDate endDate() const;
    QDateEdit* startDateEdit() const { return m_startDateEdit; }
    QDateEdit* endDateEdit() const { return m_endDateEdit; }

    // 设置日期范围
    void setDateRange(const QDate &start, const QDate &end);

    // 新增：设置模式（场所列表模式/预约记录模式）
    void setMode(bool isPlaceListMode, const QString &placeName = QString());

signals:
    void filterChanged();
    void refreshRequested();
    void backToPlaceListRequested();  // 新增：返回场所列表信号

private:
    QComboBox *m_placeCombo;
    QComboBox *m_statusCombo;
    QComboBox *m_dateFilterCombo;
    QComboBox *m_placeTypeCombo;     // 场所类型筛选
    QLineEdit *m_searchEdit;
    QPushButton *m_refreshButton;
    QPushButton *m_backButton;       // 新增：返回按钮

    QDateEdit *m_startDateEdit;
    QDateEdit *m_endDateEdit;
    QHBoxLayout *m_mainLayout;

    QTimer *m_filterTimer;
    bool m_isPlaceListMode;          // 新增：模式标识

    QComboBox *m_roleCombo;      // 新增：角色筛选下拉框
    QLabel    *m_roleLabel;      // 新增：角色标签（用于控制显示/隐藏）
};

#endif // RESERVATIONFILTERTOOLBAR_H
