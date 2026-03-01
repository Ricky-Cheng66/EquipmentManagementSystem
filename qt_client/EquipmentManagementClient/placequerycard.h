#ifndef PLACEQUERYCARD_H
#define PLACEQUERYCARD_H

#include <QWidget>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMouseEvent>
#include <QStyleOption>
#include <QPainter>
#include <QPushButton>

class PlaceQueryCard : public QWidget
{
    Q_OBJECT

public:
    explicit PlaceQueryCard(const QString &placeId, const QString &placeName,
                            const QStringList &equipmentList, int reservationCount,
                            bool showQuickReserve = true,   // 新增参数，默认显示
                            QWidget *parent = nullptr);

    QString placeId() const { return m_placeId; }
    QString placeName() const { return m_placeName; }
    QStringList equipmentList() const { return m_equipmentList; }
    int reservationCount() const { return m_reservationCount; }
    QString placeType() const { return m_placeType; }

    void setReservationCount(int count);
    void setSelected(bool selected);
    bool isSelected() const { return m_selected; }

signals:
    void cardClicked(const QString &placeId);
    void quickReserveRequested(const QString &placeId);

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void paintEvent(QPaintEvent *event) override;

private:
    void updateCardStyle();
    void setupUI();
    QString detectPlaceType(const QString &placeName);
    QString getPlaceIcon(const QString &placeType) const;

    QString m_placeId;
    QString m_placeName;
    QStringList m_equipmentList;
    int m_reservationCount;
    QString m_placeType;
    bool m_selected;
    bool m_showQuickReserve;      // 新增：控制快速预约按钮显示

    // UI控件
    QLabel *m_iconLabel;
    QLabel *m_nameLabel;
    QLabel *m_countLabel;
    QLabel *m_equipmentLabel;
    QPushButton *m_quickReserveButton;
};

#endif // PLACEQUERYCARD_H
