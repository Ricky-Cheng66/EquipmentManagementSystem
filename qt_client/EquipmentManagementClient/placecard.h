#ifndef PLACECARD_H
#define PLACECARD_H

#include <QWidget>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMouseEvent>
#include <QStyleOption>
#include <QPainter>
#include <QPaintEvent>

class PlaceCard : public QWidget
{
    Q_OBJECT

public:
    explicit PlaceCard(const QString &placeId, const QString &placeName,
                       const QStringList &equipmentList, QWidget *parent = nullptr);

    QString placeId() const { return m_placeId; }
    QString placeName() const { return m_placeName; }
    QStringList equipmentList() const { return m_equipmentList; }
    bool isSelected() const { return m_selected; }

    void setSelected(bool selected);

signals:
    void cardClicked(const QString &placeId);

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void paintEvent(QPaintEvent *event) override;  // 添加这个声明

private:
    void updateCardStyle();

    QString m_placeId;
    QString m_placeName;
    QStringList m_equipmentList;
    bool m_selected;

    QLabel *m_placeNameLabel;
    QLabel *m_equipmentLabel;
};

#endif // PLACECARD_H
