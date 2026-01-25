#ifndef FILTERTOOLBAR_H
#define FILTERTOOLBAR_H

#include <QWidget>
#include <QComboBox>
#include <QPushButton>
#include <QLineEdit>
#include <QHBoxLayout>
#include <QCheckBox>

class FilterToolBar : public QWidget
{
    Q_OBJECT

public:
    explicit FilterToolBar(QWidget *parent = nullptr);

    // 获取筛选条件
    QString selectedPlace() const;
    QString selectedType() const;
    QString selectedStatus() const;
    QString searchText() const;
    bool showOnlineOnly() const;

    // 设置选项
    void setPlaces(const QStringList &places);
    void setTypes(const QStringList &types);
    void setStatuses(const QStringList &statuses);

signals:
    void filterChanged();

private:
    // UI控件
    QComboBox *m_placeCombo;
    QComboBox *m_typeCombo;
    QComboBox *m_statusCombo;
    QLineEdit *m_searchEdit;
    QCheckBox *m_onlineOnlyCheck;
    QPushButton *m_resetButton;

    // 布局
    QHBoxLayout *m_mainLayout;
};

#endif // FILTERTOOLBAR_H
