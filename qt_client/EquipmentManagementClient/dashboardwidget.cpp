#include "dashboardwidget.h"
#include <QDebug>
#include <QEvent>

DashboardWidget::DashboardWidget(const QString &role, QWidget *parent)
    : QWidget(parent), m_role(role)
{
    m_showRightColumn = (role == "teacher");
    setupUI();
}

void DashboardWidget::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(20, 15, 20, 15);
    mainLayout->setSpacing(20);

    // ===== 顶部欢迎区域 =====
    QWidget *welcomeSection = new QWidget(this);
    welcomeSection->setObjectName("welcomeSection");
    QHBoxLayout *welcomeLayout = new QHBoxLayout(welcomeSection);
    welcomeLayout->setContentsMargins(0, 0, 0, 0);

    QString welcomeText = (m_role == "teacher") ? "老师" : "同学";
    QLabel *welcomeLabel = new QLabel(
        QString("<h1 style='margin:0;'>欢迎回来，%1</h1>"
                "<p style='color:#7f8c8d; margin:5px 0 0 0;'>上次登录时间: %2</p>")
            .arg(welcomeText)
            .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss")),
        welcomeSection);
    welcomeLayout->addWidget(welcomeLabel);
    welcomeLayout->addStretch();
    mainLayout->addWidget(welcomeSection);

    // ===== 三卡片区域 =====
    QWidget *cardsSection = new QWidget(this);
    QHBoxLayout *cardsLayout = new QHBoxLayout(cardsSection);
    cardsLayout->setContentsMargins(0, 0, 0, 0);
    cardsLayout->setSpacing(15);

    // 卡片1：今日我的预约
    m_cardToday = new QWidget(cardsSection);
    m_cardToday->setFixedHeight(120);
    m_cardToday->setStyleSheet(
        "QWidget { background-color: white; border-radius: 8px; border: 1px solid #e0e0e0; }"
        "QWidget:hover { border-color: #3498db; background-color: #f8f9fa; cursor: pointer; }");
    m_cardToday->installEventFilter(this);
    QVBoxLayout *todayLayout = new QVBoxLayout(m_cardToday);
    todayLayout->setContentsMargins(20, 15, 20, 15);
    QLabel *todayTitle = new QLabel("今日我的预约", m_cardToday);
    todayTitle->setStyleSheet("font-weight: bold; color: #666; font-size: 14px;");
    m_todayCountLabel = new QLabel("0", m_cardToday);
    m_todayCountLabel->setStyleSheet("font-size: 28px; font-weight: bold; color: #2c3e50;");
    QLabel *todayDesc = new QLabel("点击查看详情", m_cardToday);
    todayDesc->setStyleSheet("color: #95a5a6; font-size: 12px;");
    todayLayout->addWidget(todayTitle);
    todayLayout->addWidget(m_todayCountLabel);
    todayLayout->addWidget(todayDesc);
    todayLayout->addStretch();

    // 卡片2：我的预约总数
    m_cardTotal = new QWidget(cardsSection);
    m_cardTotal->setFixedHeight(120);
    m_cardTotal->setStyleSheet(
        "QWidget { background-color: white; border-radius: 8px; border: 1px solid #e0e0e0; }"
        "QWidget:hover { border-color: #27ae60; background-color: #f8f9fa; cursor: pointer; }");
    m_cardTotal->installEventFilter(this);
    QVBoxLayout *totalLayout = new QVBoxLayout(m_cardTotal);
    totalLayout->setContentsMargins(20, 15, 20, 15);
    QLabel *totalTitle = new QLabel("我的预约总数", m_cardTotal);
    totalTitle->setStyleSheet("font-weight: bold; color: #666; font-size: 14px;");
    m_totalCountLabel = new QLabel("0", m_cardTotal);
    m_totalCountLabel->setStyleSheet("font-size: 28px; font-weight: bold; color: #2c3e50;");
    QLabel *totalDesc = new QLabel("点击查看详情", m_cardTotal);
    totalDesc->setStyleSheet("color: #95a5a6; font-size: 12px;");
    totalLayout->addWidget(totalTitle);
    totalLayout->addWidget(m_totalCountLabel);
    totalLayout->addWidget(totalDesc);
    totalLayout->addStretch();

    // 卡片3：待审批数
    m_cardPending = new QWidget(cardsSection);
    m_cardPending->setFixedHeight(120);
    m_cardPending->setStyleSheet(
        "QWidget { background-color: white; border-radius: 8px; border: 1px solid #e0e0e0; }"
        "QWidget:hover { border-color: #e67e22; background-color: #f8f9fa; cursor: pointer; }");
    m_cardPending->installEventFilter(this);
    QVBoxLayout *pendingLayout = new QVBoxLayout(m_cardPending);
    pendingLayout->setContentsMargins(20, 15, 20, 15);
    QString pendingTitle = (m_role == "teacher") ? "待我审批" : "待审批";
    QLabel *pendingTitleLabel = new QLabel(pendingTitle, m_cardPending);
    pendingTitleLabel->setStyleSheet("font-weight: bold; color: #666; font-size: 14px;");
    m_pendingCountLabel = new QLabel("0", m_cardPending);
    m_pendingCountLabel->setStyleSheet("font-size: 28px; font-weight: bold; color: #2c3e50;");
    QLabel *pendingDesc = new QLabel("点击查看详情", m_cardPending);
    pendingDesc->setStyleSheet("color: #95a5a6; font-size: 12px;");
    pendingLayout->addWidget(pendingTitleLabel);
    pendingLayout->addWidget(m_pendingCountLabel);
    pendingLayout->addWidget(pendingDesc);
    pendingLayout->addStretch();

    cardsLayout->addWidget(m_cardToday);
    cardsLayout->addWidget(m_cardTotal);
    cardsLayout->addWidget(m_cardPending);
    mainLayout->addWidget(cardsSection);

    // ===== 底部信息栏 =====
    QWidget *infoSection = new QWidget(this);
    QHBoxLayout *infoLayout = new QHBoxLayout(infoSection);
    infoLayout->setContentsMargins(0, 0, 0, 0);
    infoLayout->setSpacing(15);

    // 左侧：最近个人预约
    QWidget *leftCard = new QWidget(infoSection);
    leftCard->setMinimumHeight(200);
    leftCard->setStyleSheet("background-color: white; border-radius: 8px; border: 1px solid #e0e0e0;");
    QVBoxLayout *leftLayout = new QVBoxLayout(leftCard);
    leftLayout->setContentsMargins(20, 15, 20, 15);
    QLabel *leftTitle = new QLabel("最近个人预约", leftCard);
    leftTitle->setStyleSheet("font-size: 16px; font-weight: bold; color: #2c3e50; margin-bottom: 10px;");
    m_leftTextEdit = new QTextEdit(leftCard);
    m_leftTextEdit->setReadOnly(true);
    m_leftTextEdit->setPlaceholderText("暂无预约记录");
    m_leftTextEdit->setStyleSheet("border: none; background-color: transparent; font-size: 12px;");
    leftLayout->addWidget(leftTitle);
    leftLayout->addWidget(m_leftTextEdit);

    infoLayout->addWidget(leftCard);

    // 右侧：最近待审批（仅老师显示）
    if (m_showRightColumn) {
        QWidget *rightCard = new QWidget(infoSection);
        rightCard->setMinimumHeight(200);
        rightCard->setStyleSheet("background-color: white; border-radius: 8px; border: 1px solid #e0e0e0;");
        QVBoxLayout *rightLayout = new QVBoxLayout(rightCard);
        rightLayout->setContentsMargins(20, 15, 20, 15);
        QLabel *rightTitle = new QLabel("最近待我审批", rightCard);
        rightTitle->setStyleSheet("font-size: 16px; font-weight: bold; color: #2c3e50; margin-bottom: 10px;");
        m_rightTextEdit = new QTextEdit(rightCard);
        m_rightTextEdit->setReadOnly(true);
        m_rightTextEdit->setPlaceholderText("暂无待审批记录");
        m_rightTextEdit->setStyleSheet("border: none; background-color: transparent; font-size: 12px;");
        rightLayout->addWidget(rightTitle);
        rightLayout->addWidget(m_rightTextEdit);
        infoLayout->addWidget(rightCard);
    }

    mainLayout->addWidget(infoSection);
    mainLayout->addStretch();
}

void DashboardWidget::setTodayMyReservations(int count)
{
    m_todayCountLabel->setText(QString::number(count));
}

void DashboardWidget::setTotalMyReservations(int count)
{
    m_totalCountLabel->setText(QString::number(count));
}

void DashboardWidget::setPendingApprovalCount(int count)
{
    m_pendingCountLabel->setText(QString::number(count));
}

void DashboardWidget::setRecentMyReservations(const QStringList &items)
{
    m_leftTextEdit->clear();
    if (items.isEmpty()) {
        m_leftTextEdit->setPlainText("暂无预约记录");
        return;
    }
    for (const QString &item : items) {
        m_leftTextEdit->append(item);
    }
}

void DashboardWidget::setRecentPendingReservations(const QStringList &items)
{
    if (!m_showRightColumn || !m_rightTextEdit) return;
    m_rightTextEdit->clear();
    if (items.isEmpty()) {
        m_rightTextEdit->setPlainText("暂无待审批记录");
        return;
    }
    for (const QString &item : items) {
        m_rightTextEdit->append(item);
    }
}


void DashboardWidget::onCardClicked()
{
    QObject *senderObj = sender();
    if (senderObj == m_cardToday) {
        emit cardClicked(0);
    } else if (senderObj == m_cardTotal) {
        emit cardClicked(1);
    } else if (senderObj == m_cardPending) {
        emit cardClicked(2);
    }
}

void DashboardWidget::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    // 可在此处调整卡片布局
}

bool DashboardWidget::eventFilter(QObject *watched, QEvent *event)
{
    if (event->type() == QEvent::MouseButtonPress) {
        if (watched == m_cardToday) {
            emit cardClicked(0);
            return true;
        } else if (watched == m_cardTotal) {
            emit cardClicked(1);
            return true;
        } else if (watched == m_cardPending) {
            emit cardClicked(2);
            return true;
        }
    }
    return QWidget::eventFilter(watched, event);
}
