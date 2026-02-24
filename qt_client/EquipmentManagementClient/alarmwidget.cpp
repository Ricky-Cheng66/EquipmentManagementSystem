#include "alarmwidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QFrame>
#include <QDateTime>
#include <QDebug>
#include <algorithm>

AlarmWidget::AlarmWidget(QWidget *parent) : QWidget(parent), m_showAll(false)
{
    setupUI();
}

void AlarmWidget::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->setSpacing(15);

    // 标题
    QLabel *title = new QLabel("告警中心");
    title->setStyleSheet("font-size: 18px; font-weight: bold; color: #2c3e50;");
    mainLayout->addWidget(title);

    // 说明
    QLabel *desc = new QLabel("查看和处理系统告警。未处理的告警需要及时确认。");
    desc->setWordWrap(true);
    desc->setStyleSheet("color: #7f8c8d;");
    mainLayout->addWidget(desc);

    // 切换按钮
    QHBoxLayout *switchLayout = new QHBoxLayout();
    m_btnUnprocessed = new QPushButton("未处理");
    m_btnUnprocessed->setCheckable(true);
    m_btnUnprocessed->setChecked(true);
    m_btnAll = new QPushButton("全部");
    m_btnAll->setCheckable(true);
    m_btnAll->setChecked(false);

    switchLayout->addWidget(m_btnUnprocessed);
    switchLayout->addWidget(m_btnAll);
    switchLayout->addStretch();

    mainLayout->addLayout(switchLayout);

    // 按钮互斥
    connect(m_btnUnprocessed, &QPushButton::toggled, this, [this](bool checked){
        if (checked) {
            m_btnAll->setChecked(false);
            m_showAll = false;
            refreshDisplay();
        }
    });
    connect(m_btnAll, &QPushButton::toggled, this, [this](bool checked){
        if (checked) {
            m_btnUnprocessed->setChecked(false);
            m_showAll = true;
            refreshDisplay();
        }
    });

    // 滚动区域
    m_scrollArea = new QScrollArea(this);
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setFrameShape(QFrame::NoFrame);

    m_containerWidget = new QWidget();
    m_cardsLayout = new QVBoxLayout(m_containerWidget);
    m_cardsLayout->setContentsMargins(5, 5, 5, 5);
    m_cardsLayout->setSpacing(10);
    m_cardsLayout->addStretch();  // 占位，使卡片从顶部开始

    m_scrollArea->setWidget(m_containerWidget);
    mainLayout->addWidget(m_scrollArea);
}

void AlarmWidget::setAlarms(const QList<AlarmInfo> &alarms)
{
    m_allAlarms = alarms;
    refreshDisplay();
}

void AlarmWidget::addAlarm(const AlarmInfo &alarm)
{
    qDebug() << "Adding alarm ID:" << alarm.id;
    // 去重（根据id）
    for (const AlarmInfo &a : m_allAlarms) {
        if (a.id == alarm.id) return;
    }
    m_allAlarms.append(alarm);
    // 如果当前显示模式包含该告警，刷新
    if (!alarm.acknowledged || m_showAll) {
        refreshDisplay();
    }
}

void AlarmWidget::refreshDisplay()
{
    // 清除现有卡片（保留第一个stretch）
    QLayoutItem *child;
    while ((child = m_cardsLayout->takeAt(0)) != nullptr) {
        if (child->widget()) child->widget()->deleteLater();
        delete child;
    }

    // 过滤告警
    QList<AlarmInfo> filtered;
    for (const AlarmInfo &a : m_allAlarms) {
        if (m_showAll || !a.acknowledged) {
            filtered << a;
        }
    }

    if (filtered.isEmpty()) {
        QLabel *emptyLabel = new QLabel("暂无告警信息");
        emptyLabel->setAlignment(Qt::AlignCenter);
        emptyLabel->setStyleSheet("color: #95a5a6; padding: 30px;");
        m_cardsLayout->addWidget(emptyLabel);
        m_cardsLayout->addStretch();
        return;
    }

    // 按时间倒序（最新的在前）
    std::sort(filtered.begin(), filtered.end(),
              [](const AlarmInfo &a, const AlarmInfo &b) {
                  return a.timestamp > b.timestamp;
              });

    for (const AlarmInfo &alarm : filtered) {
        // 卡片容器
        QWidget *card = new QWidget(m_containerWidget);
        card->setStyleSheet("QWidget { background-color: white; border-radius: 8px; border: 1px solid #e0e0e0; }");
        QHBoxLayout *cardLayout = new QHBoxLayout(card);
        cardLayout->setContentsMargins(15, 15, 15, 15);
        cardLayout->setSpacing(15);

        // 左侧颜色条（根据严重程度）
        QFrame *colorBar = new QFrame(card);
        colorBar->setFixedWidth(6);
        colorBar->setFixedHeight(80);
        QString severityColor;
        if (alarm.severity == "critical") severityColor = "#e74c3c";
        else if (alarm.severity == "error") severityColor = "#e67e22";
        else if (alarm.severity == "warning") severityColor = "#f39c12";
        else severityColor = "#3498db";
        colorBar->setStyleSheet(QString("background-color: %1; border-radius: 3px;").arg(severityColor));
        cardLayout->addWidget(colorBar);

        // 图标
        QLabel *iconLabel = new QLabel(card);
        iconLabel->setFixedSize(32, 32);
        iconLabel->setStyleSheet("font-size: 24px;");
        if (alarm.type == "offline") iconLabel->setText("🔴");
        else if (alarm.type == "energy_threshold") iconLabel->setText("⚡");
        else iconLabel->setText("⚠️");
        cardLayout->addWidget(iconLabel);

        // 内容区域
        QVBoxLayout *contentLayout = new QVBoxLayout();
        contentLayout->setSpacing(5);

        // 标题行：设备ID + 类型
        QHBoxLayout *titleRow = new QHBoxLayout();
        QLabel *deviceLabel = new QLabel(alarm.equipmentId, card);
        deviceLabel->setStyleSheet("font-weight: bold; font-size: 16px; color: #2c3e50;");
        QLabel *typeLabel = new QLabel("[" + alarm.type + "]", card);
        typeLabel->setStyleSheet("color: #7f8c8d; font-size: 12px;");
        titleRow->addWidget(deviceLabel);
        titleRow->addWidget(typeLabel);
        titleRow->addStretch();

        // 消息
        QLabel *msgLabel = new QLabel(alarm.message, card);
        msgLabel->setWordWrap(true);
        msgLabel->setStyleSheet("color: #34495e;");

        // 时间
        QLabel *timeLabel = new QLabel(alarm.timestamp.toString("yyyy-MM-dd hh:mm:ss"), card);
        timeLabel->setStyleSheet("color: #95a5a6; font-size: 11px;");

        contentLayout->addLayout(titleRow);
        contentLayout->addWidget(msgLabel);
        contentLayout->addWidget(timeLabel);

        cardLayout->addLayout(contentLayout, 1);

        // 右侧操作按钮（如果未处理）
        if (!alarm.acknowledged) {
            QPushButton *processBtn = new QPushButton("标记处理", card);
            processBtn->setFixedSize(80, 30);
            processBtn->setStyleSheet(
                "QPushButton { background-color: #27ae60; color: white; border: none; border-radius: 4px; }"
                "QPushButton:hover { background-color: #229954; }"
                );
            connect(processBtn, &QPushButton::clicked, [this, alarm](){
                emit acknowledgeAlarm(alarm.id);
                // 本地标记为已处理
                for (AlarmInfo &a : m_allAlarms) {
                    if (a.id == alarm.id) {
                        a.acknowledged = true;
                        break;
                    }
                }
                refreshDisplay();
            });
            cardLayout->addWidget(processBtn);
        } else {
            QLabel *processedLabel = new QLabel("✓ 已处理", card);
            processedLabel->setStyleSheet("color: #27ae60;");
            cardLayout->addWidget(processedLabel);
        }

        m_cardsLayout->addWidget(card);
    }

    m_cardsLayout->addStretch();
}

void AlarmWidget::onShowAllToggled(bool checked)
{
    // 已在lambda中处理
}

void AlarmWidget::onMarkProcessedClicked(int alarmId)
{
    emit acknowledgeAlarm(alarmId);
}
