#include "centeraligndelegate.h"
#include <QStyleOptionViewItem>

CenterAlignDelegate::CenterAlignDelegate(QObject *parent)
    : QStyledItemDelegate(parent)
{
}

void CenterAlignDelegate::initStyleOption(QStyleOptionViewItem *option, const QModelIndex &index) const
{
    QStyledItemDelegate::initStyleOption(option, index);

    // 设置所有单元格居中显示，除了状态列（状态列有特殊委托）
    if (index.column() != 3) { // 第3列是状态列，由StatusItemDelegate处理
        option->displayAlignment = Qt::AlignCenter;
    }
}
