#ifndef RESERVATIONFILTERTOOLBAR_H
#define RESERVATIONFILTERTOOLBAR_H

#include <QWidget>
#include <QComboBox>
#include <QPushButton>
#include <QLineEdit>
#include <QHBoxLayout>
#include <QDateEdit>

class ReservationFilterToolBar : public QWidget
{
    Q_OBJECT

public:
    explicit ReservationFilterToolBar(QWidget *parent = nullptr);

    // 获取筛选条件
    QString selectedPlace() const;
    QString selectedStatus() const;
    QString selectedDate() const;
    QString searchText() const;

    // 设置选项
    void setPlaces(const QStringList &places);
    void setStatuses(const QStringList &statuses);

    // 获取日期范围
    QDate startDate() const;
    QDate endDate() const;
    QDateEdit* startDateEdit() const { return m_startDateEdit; }
    QDateEdit* endDateEdit() const { return m_endDateEdit; }

    // 设置日期范围
    void setDateRange(const QDate &start, const QDate &end);

signals:
    void filterChanged();
    void refreshRequested();

private:
    QComboBox *m_placeCombo;
    QComboBox *m_statusCombo;
    QComboBox *m_dateFilterCombo;
    QLineEdit *m_searchEdit;
    QPushButton *m_refreshButton;

    QDateEdit *m_startDateEdit;
    QDateEdit *m_endDateEdit;
    QHBoxLayout *m_mainLayout;
};

#endif // RESERVATIONFILTERTOOLBAR_H
