#ifndef LOGINDIALOG_H
#define LOGINDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QLabel>
#include <QVBoxLayout>
#include <QPushButton>
#include "protocol_parser.h"
#include "tcpclient.h"
#include "messagedispatcher.h"

namespace Ui {
class LoginDialog;
}

class LoginDialog : public QDialog
{
    Q_OBJECT

public:
    explicit LoginDialog(TcpClient* tcpClient, MessageDispatcher* dispatcher, QWidget *parent = nullptr);
    ~LoginDialog();

    QString getUsername() const;
    QString getPassword() const;
    void setStatusMessage(const QString& message, bool isError);
    void setLoginButtonEnabled(bool enabled);
    void setCancelButtonEnabled(bool enabled);

    QString getUserRole() const { return m_userRole; }
    int getUserId() const { return m_userId; }

private slots:
    void on_loginButton_clicked();
    void on_cancelButton_clicked();
    void handleLoginResponse(const ProtocolParser::ParseResult& result);

signals:
    void loginRequested(const QString& username, const QString& password);

private:
    // UI控件指针
    QLineEdit *m_usernameEdit;
    QLineEdit *m_passwordEdit;
    QLabel *m_statusLabel;
    QPushButton *m_loginButton;
    QPushButton *m_cancelButton;

    // 布局相关
    QVBoxLayout *m_mainLayout;

    // 其他成员变量
    TcpClient* m_tcpClient;
    MessageDispatcher* m_dispatcher;
    QString m_userRole;
    int m_userId;

    // 私有函数
    void setupUI();
};

#endif // LOGINDIALOG_H
