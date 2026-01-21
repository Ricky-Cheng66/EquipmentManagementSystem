#ifndef STATUSITEMDELEGATE_H
#define STATUSITEMDELEGATE_H
#include <QStyledItemDelegate>
#include <QPainter>
class StatusItemDelegate : public QStyledItemDelegate {
public:
    StatusItemDelegate(QObject *parent = nullptr);

    void paint(QPainter *painter, const QStyleOptionViewItem &option,
                                   const QModelIndex &index) const;
};

#endif // STATUSITEMDELEGATE_H
