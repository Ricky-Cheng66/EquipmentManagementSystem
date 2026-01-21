#include "statusitemdelegate.h"
#include <QPainter>
#include <QStyle>
#include <QApplication>
#include <QDebug>

StatusItemDelegate::StatusItemDelegate(QObject *parent) : QStyledItemDelegate(parent) {}

void StatusItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option,
                               const QModelIndex &index) const {
    // 如果是第3列（状态列），完全自己绘制
    if (index.column() == 3) {
        // 获取状态文本并清理
        QString status = index.data(Qt::DisplayRole).toString();

        // 彻底清理状态文本
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
            textColor = option.palette.color(QPalette::Text);
        }

        // 完全自己绘制，不使用基类
        // 1. 绘制背景
        if (option.state & QStyle::State_Selected) {
            painter->fillRect(option.rect, option.palette.highlight());
        } else if (option.state & QStyle::State_MouseOver) {
            painter->fillRect(option.rect, option.palette.alternateBase());
        } else {
            painter->fillRect(option.rect, option.palette.base());
        }

        // 2. 绘制焦点框
        if (option.state & QStyle::State_HasFocus) {
            QStyleOptionFocusRect focusOption;
            focusOption.rect = option.rect;
            focusOption.state = option.state | QStyle::State_KeyboardFocusChange;
            focusOption.backgroundColor = option.palette.window().color();
            QApplication::style()->drawPrimitive(QStyle::PE_FrameFocusRect,
                                                 &focusOption, painter);
        }

        // 3. 绘制网格线（如果需要）
        if (option.showDecorationSelected && (option.state & QStyle::State_Selected)) {
            QPen oldPen = painter->pen();
            painter->setPen(option.palette.highlightedText().color());
            painter->drawRect(option.rect.adjusted(0, 0, -1, -1));
            painter->setPen(oldPen);
        }

        // 4. 绘制文本
        painter->save(); // 只保存一次

        painter->setPen(textColor);
        painter->setFont(option.font);

        // 调整绘制区域，避免绘制到边框上
        QRect textRect = option.rect.adjusted(2, 0, -2, 0);
        painter->drawText(textRect, Qt::AlignCenter, status);

        painter->restore(); // 恢复一次

    } else {
        // 其他列使用基类绘制
        QStyledItemDelegate::paint(painter, option, index);
    }
}
