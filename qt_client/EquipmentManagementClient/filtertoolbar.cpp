#include "filtertoolbar.h"
#include <QLabel>

FilterToolBar::FilterToolBar(QWidget *parent)
    : QWidget(parent)
{
    setObjectName("filterToolBar");

    // 创建主布局
    m_mainLayout = new QHBoxLayout(this);
    m_mainLayout->setContentsMargins(10, 5, 10, 5);
    m_mainLayout->setSpacing(15);

    // 场所筛选
    QLabel *placeLabel = new QLabel("场所:", this);
    m_placeCombo = new QComboBox(this);
    m_placeCombo->addItem("全部场所", "all");
    m_placeCombo->setFixedWidth(150);

    // 类型筛选
    QLabel *typeLabel = new QLabel("类型:", this);
    m_typeCombo = new QComboBox(this);
    m_typeCombo->addItem("全部类型", "all");
    m_typeCombo->setFixedWidth(120);

    // 状态筛选
    QLabel *statusLabel = new QLabel("状态:", this);
    m_statusCombo = new QComboBox(this);
    m_statusCombo->addItem("全部状态", "all");
    m_statusCombo->addItem("在线", "online");
    m_statusCombo->addItem("离线", "offline");
    m_statusCombo->addItem("预约中", "reserved");
    m_statusCombo->setFixedWidth(100);

    // 搜索框
    QLabel *searchLabel = new QLabel("搜索:", this);
    m_searchEdit = new QLineEdit(this);
    m_searchEdit->setPlaceholderText("设备ID或位置");
    m_searchEdit->setFixedWidth(180);
    m_searchEdit->setClearButtonEnabled(true);

    // 仅显示在线设备
    m_onlineOnlyCheck = new QCheckBox("仅显示在线设备", this);

    // 重置按钮
    m_resetButton = new QPushButton("重置筛选", this);
    m_resetButton->setFixedWidth(80);

    // 添加到布局
    m_mainLayout->addWidget(placeLabel);
    m_mainLayout->addWidget(m_placeCombo);
    m_mainLayout->addWidget(typeLabel);
    m_mainLayout->addWidget(m_typeCombo);
    m_mainLayout->addWidget(statusLabel);
    m_mainLayout->addWidget(m_statusCombo);
    m_mainLayout->addWidget(searchLabel);
    m_mainLayout->addWidget(m_searchEdit);
    m_mainLayout->addWidget(m_onlineOnlyCheck);
    m_mainLayout->addStretch();
    m_mainLayout->addWidget(m_resetButton);

    // 设置样式
    setStyleSheet(
        "QWidget#filterToolBar {"
        "    background-color: #f5f6fa;"
        "    border-bottom: 1px solid #e0e0e0;"
        "}"
        "QLabel {"
        "    color: #666;"
        "    font-weight: bold;"
        "}"
        "QPushButton {"
        "    background-color: #95a5a6;"
        "    color: white;"
        "    border: none;"
        "    border-radius: 4px;"
        "    padding: 6px 12px;"
        "}"
        "QPushButton:hover {"
        "    background-color: #7f8c8d;"
        "}"
        );

    // 连接信号
    connect(m_placeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &FilterToolBar::filterChanged);
    connect(m_typeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &FilterToolBar::filterChanged);
    connect(m_statusCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &FilterToolBar::filterChanged);
    connect(m_searchEdit, &QLineEdit::textChanged,
            this, &FilterToolBar::filterChanged);
    connect(m_onlineOnlyCheck, &QCheckBox::stateChanged,
            this, &FilterToolBar::filterChanged);
    connect(m_resetButton, &QPushButton::clicked, [this]() {
        m_placeCombo->setCurrentIndex(0);
        m_typeCombo->setCurrentIndex(0);
        m_statusCombo->setCurrentIndex(0);
        m_searchEdit->clear();
        m_onlineOnlyCheck->setChecked(false);
        emit filterChanged();
    });
}

QString FilterToolBar::selectedPlace() const
{
    return m_placeCombo->currentData().toString();
}

QString FilterToolBar::selectedType() const
{
    return m_typeCombo->currentData().toString();
}

QString FilterToolBar::selectedStatus() const
{
    return m_statusCombo->currentData().toString();
}

QString FilterToolBar::searchText() const
{
    return m_searchEdit->text().trimmed();
}

bool FilterToolBar::showOnlineOnly() const
{
    return m_onlineOnlyCheck->isChecked();
}

void FilterToolBar::setPlaces(const QStringList &places)
{
    m_placeCombo->clear();
    m_placeCombo->addItem("全部场所", "all");

    for (const QString &place : places) {
        m_placeCombo->addItem(place, place);
    }
}

void FilterToolBar::setTypes(const QStringList &types)
{
    m_typeCombo->clear();
    m_typeCombo->addItem("全部类型", "all");

    for (const QString &type : types) {
        m_typeCombo->addItem(type, type);
    }
}

void FilterToolBar::setStatuses(const QStringList &statuses)
{
    // 这个方法可以根据需要添加更多状态
    Q_UNUSED(statuses);
}
