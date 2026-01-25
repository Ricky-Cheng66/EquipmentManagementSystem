#ifndef DEVICECARD_H
#define DEVICECARD_H

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPropertyAnimation>
#include <QMouseEvent>

class DeviceCard : public QWidget
{
    Q_OBJECT

public:
    explicit DeviceCard(const QString &deviceId, const QString &type,
                        const QString &location, const QString &status,
                        const QString &power, QWidget *parent = nullptr);

    // 获取设备信息
    QString deviceId() const { return m_deviceId; }
    QString deviceType() const { return m_deviceType; }
    QString location() const { return m_location; }
    QString status() const { return m_status; }
    QString power() const { return m_power; }

    // 更新设备状态
    void updateStatus(const QString &status, const QString &power);

    // 设置选中状态
    void setSelected(bool selected);
    bool isSelected() const { return m_selected; }



signals:
    // 卡片被点击
    void cardClicked(const QString &deviceId);

    // 电源控制请求
    void powerControlRequested(const QString &deviceId, bool turnOn);

    // 新增：选中状态变化信号
    void selectionChanged(bool selected);
protected:
    // 鼠标事件 - 修改参数类型
    void mousePressEvent(QMouseEvent *event) override;
    void enterEvent(QEnterEvent *event) override;  // Qt6 使用 QEnterEvent
    void leaveEvent(QEvent *event) override;       // 改为 QEvent*

    // 绘制事件
    void paintEvent(QPaintEvent *event) override;

private slots:
    // 电源按钮点击
    void onPowerButtonClicked();

private:
    // 初始化UI
    void setupUI();

    // 更新卡片样式
    void updateCardStyle();

    // 根据设备类型获取图标
    QString getTypeIcon(const QString &type) const;

    // 根据状态获取颜色
    QColor getStatusColor(const QString &status) const;

    void updatePowerLabel();

    void updatePowerButton();

    // 成员变量
    QString m_deviceId;
    QString m_deviceType;
    QString m_location;
    QString m_status;
    QString m_power;
    bool m_selected;

    QLabel *m_powerLabel;  // 电源状态标签

    // UI控件
    QWidget *m_contentWidget;
    QLabel *m_iconLabel;
    QLabel *m_nameLabel;
    QLabel *m_locationLabel;
    QLabel *m_statusLabel;
    QPushButton *m_powerButton;
    QWidget *m_statusIndicator;

    // 布局
    QVBoxLayout *m_mainLayout;
    QHBoxLayout *m_headerLayout;
    QVBoxLayout *m_infoLayout;
    QHBoxLayout *m_footerLayout;
};

#endif // DEVICECARD_H
