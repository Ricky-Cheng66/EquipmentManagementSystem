#include "statusitemdelegate.h"
#include <QPainter>
#include <QStyle>
#include <QApplication>
#include <QDebug>

StatusItemDelegate::StatusItemDelegate(QObject *parent) : QStyledItemDelegate(parent) {}

void StatusItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option,
                               const QModelIndex &index) const {
    // 如果是第3列（状态列）
    if (index.column() == 3) {
        // 获取状态文本
        QString status = index.data(Qt::DisplayRole).toString();

        // 清理状态文本
        if (status.contains("online", Qt::CaseInsensitive)) {
            status = "online";
        } else if (status.contains("offline", Qt::CaseInsensitive)) {
            status = "offline";
        } else if (status.contains("reserved", Qt::CaseInsensitive)) {
            status = "reserved";
        }

        // 设置颜色
        QColor textColor;
        if (status == "online") {
            textColor = QColor("#27ae60");
        } else if (status == "offline") {
            textColor = QColor("#e74c3c");
        } else if (status == "reserved") {
            textColor = QColor("#f39c12");
        } else {
            textColor = QColor("#333333");
        }

        // 保存painter状态
        painter->save();

        // 绘制背景，但确保状态文字颜色不受选中状态影响
        if (option.state & QStyle::State_Selected) {
            // 只绘制浅色背景，不改变文字颜色
            painter->fillRect(option.rect, QColor(227, 242, 253, 100)); // 半透明浅蓝色
        } else if (option.state & QStyle::State_MouseOver) {
            painter->fillRect(option.rect, option.palette.alternateBase());
        }

        // 设置文字颜色（保持原来的颜色）
        painter->setPen(textColor);
        painter->setFont(option.font);

        // 绘制文字
        painter->drawText(option.rect, Qt::AlignCenter, status);

        // 恢复painter状态
        painter->restore();
    } else {
        // 其他列使用默认绘制
        QStyledItemDelegate::paint(painter, option, index);
    }
}
