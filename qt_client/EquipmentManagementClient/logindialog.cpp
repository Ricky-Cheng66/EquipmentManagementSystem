#include "logindialog.h"
#include "ui_logindialog.h"
#include <QMessageBox>

LoginDialog::LoginDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::LoginDialog)
{
    ui->setupUi(this);
    setWindowTitle("设备管理系统 - 登录");

    connect(ui->loginButton, &QPushButton::clicked, this, &LoginDialog::on_loginButton_clicked);
    connect(ui->cancelButton, &QPushButton::clicked, this, &LoginDialog::on_cancelButton_clicked);

    // 按Enter键触发登录
    ui->passwordEdit->setFocus();
}

LoginDialog::~LoginDialog()
{
    delete ui;
}

void LoginDialog::setLoginButtonEnabled(bool enabled)
{
    ui->loginButton->setEnabled(enabled);
}

void LoginDialog::setCancelButtonEnabled(bool enabled)
{
    ui->cancelButton->setEnabled(enabled);
}

QString LoginDialog::getUsername() const {
    return ui->usernameEdit->text().trimmed();
}

QString LoginDialog::getPassword() const {
    return ui->passwordEdit->text();
}

void LoginDialog::setStatusMessage(const QString& message, bool isError) {
    ui->statusLabel->setText(message);
    if (isError) {
        ui->statusLabel->setStyleSheet("color: red;");
    } else {
        ui->statusLabel->setStyleSheet("color: black;");
    }
}

void LoginDialog::on_loginButton_clicked() {
    QString username = getUsername();
    QString password = getPassword();

    if (username.isEmpty() || password.isEmpty()) {
        setStatusMessage("用户名和密码不能为空", true);
        return;
    }

    // 禁用按钮，防止重复点击
    ui->loginButton->setEnabled(false);
    ui->cancelButton->setEnabled(false);
    setStatusMessage("正在登录...", false);

    // 发出登录请求信号，由MainWindow处理
    emit loginRequested(username, password);
}

void LoginDialog::on_cancelButton_clicked() {
    reject(); // 关闭对话框并返回QDialog::Rejected
}
