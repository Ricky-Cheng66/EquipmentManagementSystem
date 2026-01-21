#ifndef STATUSITEMDELEGATE_H
#define STATUSITEMDELEGATE_H
#include <QStyledItemDelegate>
#include <QPainter>
class StatusItemDelegate : public QStyledItemDelegate {
public:
    StatusItemDelegate(QObject *parent = nullptr) : QStyledItemDelegate(parent) {}

    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const override {
        // 第3列是状态列
        if (index.column() == 3) {
            QString status = index.data(Qt::DisplayRole).toString();
            painter->save();

            // 设置颜色
            if (status == "online") {
                painter->setPen(QColor("#27ae60"));
            } else if (status == "offline") {
                painter->setPen(QColor("#e74c3c"));
            } else if (status == "reserved") {
                painter->setPen(QColor("#f39c12"));
            } else {
                painter->setPen(QColor("#333333"));
            }

            // 绘制文本
            painter->drawText(option.rect, Qt::AlignCenter, status);
            painter->restore();
        } else {
            QStyledItemDelegate::paint(painter, option, index);
        }
    }
};

#endif // STATUSITEMDELEGATE_H
