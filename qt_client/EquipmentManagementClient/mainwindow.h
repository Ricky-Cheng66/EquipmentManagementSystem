#ifndef MAINWINDOW_H
#define MAINWINDOW_H
#pragma once
#include <QMainWindow>
#include "protocol_parser.h"
#include "message_buffer.h"
#include "tcpclient.h"
#include "messagedispatcher.h"
QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onConnectButtonClicked();
    void onSendHeartbeatButtonClicked();
    void onClientConnected();
    void onClientDisconnected();
    void onProtocolMessageReceived(const ProtocolParser::ParseResult& result);
    void onClientErrorOccurred(const QString& errorString);
     void handleHeartbeatResponse(const ProtocolParser::ParseResult &result);

private:
    Ui::MainWindow *ui;
    TcpClient* m_tcpClient; // 声明TCP客户端指针
    MessageDispatcher* m_dispatcher;
    void logMessage(const QString& msg); // 辅助日志函数
};
#endif // MAINWINDOW_H
