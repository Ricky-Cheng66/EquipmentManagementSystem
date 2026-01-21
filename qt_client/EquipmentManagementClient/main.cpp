#include "mainwindow.h"
#include <QApplication>
#include <QFontDatabase>
#include <QFile>
#include <QFileInfo>
#include <QDebug>
#include <QPushButton>
#include <QVBoxLayout>
#include <QWidget>

void testSimpleFontAwesome()
{
    qDebug() << "=== 简单字体图标测试 ===";

    // 1. 先加载字体到系统
    QString fontPath = "/home/liuliuqiu/EquipmentManagementSystem/qt_client/EquipmentManagementClient/resources/fonts/fontawesome-webfont.ttf";
    QFontDatabase::addApplicationFont(fontPath);

    // 2. 创建测试窗口
    QWidget* window = new QWidget();
    window->setWindowTitle("字体图标测试");
    QVBoxLayout* layout = new QVBoxLayout(window);

    // 3. 创建标签1 - 使用icon-font类
    QLabel* label1 = new QLabel();
    label1->setProperty("class", "icon-font");  // 关键！告诉QSS这是图标控件
    label1->setText("链接: " + QString(QChar(0xF0C1)));
    layout->addWidget(label1);

    // 4. 创建标签2 - 同样方法
    QLabel* label2 = new QLabel();
    label2->setProperty("class", "icon-font");
    label2->setText("WiFi: " + QString(QChar(0xF1EB)));
    layout->addWidget(label2);

    // 5. 创建标签3
    QLabel* label3 = new QLabel();
    label3->setProperty("class", "icon-font");
    label3->setText("用户: " + QString(QChar(0xF007)));
    layout->addWidget(label3);

    // 6. 创建普通标签（对比）
    QLabel* normalLabel = new QLabel();
    normalLabel->setText("这是普通文本，不是图标");
    layout->addWidget(normalLabel);

    window->setLayout(layout);
    window->resize(300, 200);
    window->show();

    qDebug() << "=== 测试完成 ===";
}
int main(int argc, char *argv[]) {
    QApplication a(argc, argv);

    // ✅ 加载字体文件
    QString fontPath = "/home/liuliuqiu/EquipmentManagementSystem/qt_client/EquipmentManagementClient/resources/fonts/fontawesome-webfont.ttf";

    qDebug() << "========================================";
    qDebug() << "字体文件路径:" << fontPath;
    qDebug() << "文件存在:" << QFile::exists(fontPath);

    // 加载字体
    int fontId = QFontDatabase::addApplicationFont(fontPath);
    if (fontId == -1) {
        qDebug() << "❌ 字体加载失败!";
        return -1;
    }

    // 获取加载的字体族
    QStringList loadedFamilies = QFontDatabase::applicationFontFamilies(fontId);
    qDebug() << "✅ 字体加载成功!";
    qDebug() << "加载的字体族 (" << loadedFamilies.size() << " 个):";
    for (int i = 0; i < loadedFamilies.size(); ++i) {
        qDebug() << "  [" << i << "]" << loadedFamilies.at(i);
    }

    // 如果没有找到任何字体族
    if (loadedFamilies.isEmpty()) {
        qDebug() << "❌ 警告: 字体已加载但未找到字体族!";
    } else {
        qDebug() << "✅ 建议使用字体族: \"" << loadedFamilies.first() << "\"";
    }
    qDebug() << "========================================";

    // ✅ 加载QSS样式
    QString qssPath = "/home/liuliuqiu/EquipmentManagementSystem/qt_client/EquipmentManagementClient/resources/style.qss";
    QFile styleFile(qssPath);
    if (styleFile.open(QFile::ReadOnly)) {
        a.setStyleSheet(QString::fromUtf8(styleFile.readAll()));
        styleFile.close();
        qDebug() << "✅ QSS样式加载成功";
    } else {
        qDebug() << "❌ QSS样式加载失败";
    }
   // testSimpleFontAwesome();

    // 创建主窗口
    MainWindow w;
    w.show();

    return a.exec();
}
