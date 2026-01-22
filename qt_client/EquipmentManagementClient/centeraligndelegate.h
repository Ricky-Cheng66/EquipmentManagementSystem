#ifndef CENTERALIGNDELEGATE_H
#define CENTERALIGNDELEGATE_H

#include <QStyledItemDelegate>

class CenterAlignDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    explicit CenterAlignDelegate(QObject *parent = nullptr);

protected:
    void initStyleOption(QStyleOptionViewItem *option, const QModelIndex &index) const override;
};

#endif // CENTERALIGNDELEGATE_H
