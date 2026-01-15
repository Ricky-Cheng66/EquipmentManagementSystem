#ifndef TCPCLIENT_H
#define TCPCLIENT_H
#pragma once
#include <QObject>
#include <QTimer>
#include <QTcpSocket>
#include <QHostAddress>
#include "protocol_parser.h"
#include "message_buffer.h"
class TcpClient : public QObject
{
    Q_OBJECT
public:
    explicit TcpClient(QObject *parent = nullptr);
    ~TcpClient();

    // 连接服务器
    bool connectToServer(const QString& host, quint16 port);
    // 断开连接
    void disconnectFromServer();
    // 发送原始数据（已打包的消息）
    qint64 sendData(const QByteArray& data);
    // 发送协议消息（自动打包）
    bool sendProtocolMessage(ProtocolParser::ClientType client_type,
        ProtocolParser::MessageType message_type,
                             const QString& equipmentId = "",
                             const QString& payload = "");

    bool isConnected() const;

    // 新增：启动/停止自动心跳
    void startHeartbeat(const QString& equipmentId = "qt_client", int intervalSeconds = 5);
    void stopHeartbeat();
    bool sendHeartbeat(const QString& equipmentId);
signals:
    // 当成功连接到服务器时发出
    void connected();
    // 当与服务器断开连接时发出
    void disconnected();
    // 当从服务器接收到一个完整的、已解析的应用层消息时发出
    void protocolMessageReceived(const ProtocolParser::ParseResult& result);
    // 当发生网络错误时发出
    void errorOccurred(const QString& errorString);

private slots:

    void onSocketReadyRead();
    void onSocketErrorOccurred(QAbstractSocket::SocketError error);
private:
    // 新增：一个专门用于处理接收数据的私有方法
    void processReceivedData(const QByteArray &data);
    QTcpSocket* m_socket;
    MessageBuffer m_messageBuffer; // 用于处理消息边界
    // 新增：防止在极端情况下递归处理导致栈溢出
    bool m_isProcessingData;

    // 新增：心跳相关
    QTimer* m_heartbeatTimer;          // 心跳定时器
    int m_heartbeatInterval;           // 心跳间隔（秒）
    QString m_lastEquipmentId;         // 用于心跳的设备ID
};

#endif // TCPCLIENT_H
