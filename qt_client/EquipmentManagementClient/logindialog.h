#ifndef LOGINDIALOG_H
#define LOGINDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QLabel>
#include <QVBoxLayout>
#include <QPushButton>
#include <QResizeEvent>  // 添加这个头文件
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

protected:  // 修改这里：将resizeEvent移到protected区域
    void resizeEvent(QResizeEvent *event) override;

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

    // 背景相关控件
    QLabel *m_backgroundLabel;
    QWidget *m_overlayWidget;  // 右侧半透明遮罩
    QWidget *m_formContainer;  // 表单容器

    // 布局相关
    QVBoxLayout *m_mainLayout;

    // 其他成员变量
    TcpClient* m_tcpClient;
    MessageDispatcher* m_dispatcher;
    QString m_userRole;
    int m_userId;

    // 私有函数
    void setupUI();
    void setupBackground();     // 新增：设置背景
    void updateFormPosition();  // 新增：更新表单位置
};

#endif // LOGINDIALOG_H
