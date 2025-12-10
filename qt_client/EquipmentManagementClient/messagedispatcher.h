#ifndef MESSAGEDISPATCHER_H
#define MESSAGEDISPATCHER_H

#include <QObject>
#include <QMap>
#include <functional>
#include "protocol_parser.h"

// 前向声明，避免循环包含
class TcpClient;

class MessageDispatcher : public QObject
{
    Q_OBJECT
public:
    explicit MessageDispatcher(TcpClient *tcpClient, QObject *parent = nullptr);
    ~MessageDispatcher();

    // 注册处理函数：将特定的消息类型绑定到一个处理函数上
    void registerHandler(ProtocolParser::MessageType type, std::function<void(const ProtocolParser::ParseResult&)> handler);

    // 注销处理函数
    void unregisterHandler(ProtocolParser::MessageType type);

signals:
    // 可以定义一些高级信号，例如“登录成功”、“收到设备列表”等，供UI层使用（后续阶段使用）
    void heartbeatResponseReceived(const QString &fromEquipmentId);

public slots:
    // 这个槽函数将连接到 TcpClient::protocolMessageReceived 信号
    void onProtocolMessageReceived(const ProtocolParser::ParseResult &result);

private:
    TcpClient *m_tcpClient;
    // 核心映射：消息类型 -> 处理函数
    QMap<ProtocolParser::MessageType, std::function<void(const ProtocolParser::ParseResult&)>> m_handlerMap;
};

#endif // MESSAGEDISPATCHER_H
