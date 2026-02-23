#include "thresholdsettingswidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QDoubleValidator>
#include <QMessageBox>

ThresholdSettingsWidget::ThresholdSettingsWidget(QWidget *parent)
    : QWidget(parent)
{
    setupUI();
}

void ThresholdSettingsWidget::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->setSpacing(15);

    // 标题
    QLabel *title = new QLabel("能耗阈值设置");
    title->setStyleSheet("font-size: 18px; font-weight: bold; color: #2c3e50;");
    mainLayout->addWidget(title);

    // 说明
    QLabel *desc = new QLabel("设置设备的能耗告警阈值，当设备实时功耗超过阈值时系统将发出告警。");
    desc->setWordWrap(true);
    desc->setStyleSheet("color: #7f8c8d;");
    mainLayout->addWidget(desc);

    // 设置卡片
    QWidget *card = new QWidget();
    card->setStyleSheet("QWidget { background-color: white; border-radius: 8px; border: 1px solid #e0e0e0; }");
    QVBoxLayout *cardLayout = new QVBoxLayout(card);
    cardLayout->setContentsMargins(20, 20, 20, 20);
    cardLayout->setSpacing(15);

    // 设备选择
    QHBoxLayout *deviceRow = new QHBoxLayout();
    QLabel *deviceLabel = new QLabel("选择设备:");
    deviceLabel->setFixedWidth(80);
    m_deviceCombo = new QComboBox();
    m_deviceCombo->setMinimumWidth(250);
    deviceRow->addWidget(deviceLabel);
    deviceRow->addWidget(m_deviceCombo);
    deviceRow->addStretch();
    cardLayout->addLayout(deviceRow);

    // 阈值输入
    QHBoxLayout *thresholdRow = new QHBoxLayout();
    QLabel *thresholdLabel = new QLabel("阈值 (W):");
    thresholdLabel->setFixedWidth(80);
    m_thresholdEdit = new QLineEdit();
    m_thresholdEdit->setPlaceholderText("输入功率阈值");
    m_thresholdEdit->setValidator(new QDoubleValidator(0, 10000, 1, this));
    QLabel *unitLabel = new QLabel("瓦");
    thresholdRow->addWidget(thresholdLabel);
    thresholdRow->addWidget(m_thresholdEdit);
    thresholdRow->addWidget(unitLabel);
    thresholdRow->addStretch();
    cardLayout->addLayout(thresholdRow);

    // 应用按钮
    QHBoxLayout *buttonRow = new QHBoxLayout();
    m_applyButton = new QPushButton("应用设置");
    m_applyButton->setMinimumSize(120, 36);
    m_applyButton->setStyleSheet(
        "QPushButton { background-color: #4a69bd; color: white; border: none; border-radius: 4px; }"
        "QPushButton:hover { background-color: #3b55a0; }"
        "QPushButton:disabled { background-color: #b0b0b0; }"
        );
    buttonRow->addWidget(m_applyButton);
    buttonRow->addStretch();
    cardLayout->addLayout(buttonRow);

    // 状态提示
    m_statusLabel = new QLabel();
    m_statusLabel->setStyleSheet("color: #27ae60;");
    cardLayout->addWidget(m_statusLabel);

    mainLayout->addWidget(card);

    // 当前阈值列表卡片
    QWidget *listCard = new QWidget();
    listCard->setStyleSheet("QWidget { background-color: white; border-radius: 8px; border: 1px solid #e0e0e0; }");
    QVBoxLayout *listLayout = new QVBoxLayout(listCard);
    listLayout->setContentsMargins(20, 20, 20, 20);

    QLabel *listTitle = new QLabel("当前配置的阈值");
    listTitle->setStyleSheet("font-size: 16px; font-weight: bold; color: #2c3e50;");
    listLayout->addWidget(listTitle);

    m_thresholdTable = new QTableWidget(0, 2);
    m_thresholdTable->setHorizontalHeaderLabels({"设备ID", "阈值 (W)"});
    m_thresholdTable->horizontalHeader()->setStretchLastSection(true);
    m_thresholdTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_thresholdTable->setAlternatingRowColors(true);
    listLayout->addWidget(m_thresholdTable);

    mainLayout->addWidget(listCard);

    // 信号连接
    connect(m_applyButton, &QPushButton::clicked, this, &ThresholdSettingsWidget::onApplyButtonClicked);
    connect(m_deviceCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ThresholdSettingsWidget::onDeviceSelected);
}

void ThresholdSettingsWidget::setEquipmentList(const QHash<QString, QString> &devices)
{
    m_deviceMap = devices;
    m_deviceCombo->clear();
    m_deviceCombo->addItem("请选择设备", "");
    for (auto it = devices.begin(); it != devices.end(); ++it) {
        m_deviceCombo->addItem(it.key() + " - " + it.value(), it.key());
    }
}

void ThresholdSettingsWidget::setCurrentThresholds(const QHash<QString, double> &thresholds)
{
    m_thresholdMap = thresholds;
    updateThresholdTable();
}

void ThresholdSettingsWidget::updateThresholdTable()
{
    m_thresholdTable->setRowCount(0);
    for (auto it = m_thresholdMap.begin(); it != m_thresholdMap.end(); ++it) {
        int row = m_thresholdTable->rowCount();
        m_thresholdTable->insertRow(row);
        m_thresholdTable->setItem(row, 0, new QTableWidgetItem(it.key()));
        m_thresholdTable->setItem(row, 1, new QTableWidgetItem(QString::number(it.value(), 'f', 1)));
    }
    m_thresholdTable->resizeColumnsToContents();
}

void ThresholdSettingsWidget::onDeviceSelected(int index)
{
    if (index <= 0) {
        m_thresholdEdit->clear();
        return;
    }
    QString devId = m_deviceCombo->currentData().toString();
    if (m_thresholdMap.contains(devId)) {
        m_thresholdEdit->setText(QString::number(m_thresholdMap[devId]));
    } else {
        m_thresholdEdit->clear();
    }
}

void ThresholdSettingsWidget::onApplyButtonClicked()
{
    QString devId = m_deviceCombo->currentData().toString();
    if (devId.isEmpty()) {
        QMessageBox::warning(this, "提示", "请选择设备");
        return;
    }
    QString text = m_thresholdEdit->text().trimmed();
    if (text.isEmpty()) {
        QMessageBox::warning(this, "提示", "请输入阈值");
        return;
    }
    bool ok;
    double val = text.toDouble(&ok);
    if (!ok || val < 0) {
        QMessageBox::warning(this, "提示", "请输入有效的正数");
        return;
    }

    emit setThresholdRequested(devId, val);
    m_applyButton->setEnabled(false);
    m_statusLabel->setText("正在发送请求...");
}

void ThresholdSettingsWidget::handleSetThresholdResponse(bool success, const QString &message)
{
    m_applyButton->setEnabled(true);
    if (success) {
        m_statusLabel->setStyleSheet("color: #27ae60;");
        m_statusLabel->setText("设置成功");
        // 更新本地缓存
        QString devId = m_deviceCombo->currentData().toString();
        double val = m_thresholdEdit->text().toDouble();
        m_thresholdMap[devId] = val;
        updateThresholdTable();
    } else {
        m_statusLabel->setStyleSheet("color: #e74c3c;");
        m_statusLabel->setText("设置失败: " + message);
    }
}
