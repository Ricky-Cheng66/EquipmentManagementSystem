#ifndef LOGINDIALOG_H
#define LOGINDIALOG_H

#include <QDialog>

namespace Ui {
class LoginDialog;
}

class LoginDialog : public QDialog
{
    Q_OBJECT

public:
    explicit LoginDialog(QWidget *parent = nullptr);
    ~LoginDialog();

    void setLoginButtonEnabled(bool enabled);
    void setCancelButtonEnabled(bool enabled);

    QString getUsername() const;
    QString getPassword() const;
    void setStatusMessage(const QString& message, bool isError = false);

signals:
    void loginRequested(const QString& username, const QString& password);

private slots:
    void on_loginButton_clicked();
    void on_cancelButton_clicked();

private:
    Ui::LoginDialog *ui;
};

#endif // LOGINDIALOG_H
