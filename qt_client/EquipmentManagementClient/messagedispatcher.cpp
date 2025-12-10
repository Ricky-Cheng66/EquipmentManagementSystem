#include "messagedispatcher.h"
#include "tcpclient.h" // 现在可以包含，因为头文件中已前向声明
#include <QDebug>

MessageDispatcher::MessageDispatcher(TcpClient *tcpClient, QObject *parent)
    : QObject(parent)
    , m_tcpClient(tcpClient)
{
    if (!m_tcpClient) {
        qCritical() << "MessageDispatcher: TcpClient is null!";
        return;
    }
    // 关键连接：将网络层的消息信号，连接到本分发器的处理槽。
    connect(m_tcpClient, &TcpClient::protocolMessageReceived,
            this, &MessageDispatcher::onProtocolMessageReceived);
    qDebug() << "MessageDispatcher initialized and connected to TcpClient.";
}

MessageDispatcher::~MessageDispatcher()
{
    m_handlerMap.clear();
}

void MessageDispatcher::registerHandler(ProtocolParser::MessageType type, std::function<void(const ProtocolParser::ParseResult&)> handler)
{
    if (handler) {
        m_handlerMap[type] = handler;
        qDebug() << "Registered handler for message type:" << type;
    } else {
        qWarning() << "Attempted to register a null handler for type:" << type;
    }
}

void MessageDispatcher::unregisterHandler(ProtocolParser::MessageType type)
{
    if (m_handlerMap.remove(type) > 0) {
        qDebug() << "Unregistered handler for message type:" << type;
    }
}

void MessageDispatcher::onProtocolMessageReceived(const ProtocolParser::ParseResult &result)
{
    qDebug() << "Dispatcher received message type:" << result.type;
    // 1. 查找是否有注册的处理函数
    auto it = m_handlerMap.find(result.type);
    if (it != m_handlerMap.end()) {
        // 2. 找到，调用对应的处理函数
        try {
            (it.value())(result); // 执行函数
        } catch (const std::exception &e) {
            qCritical() << "Exception in handler for type" << result.type << ":" << e.what();
        }
    } else {
        // 3. 没有找到，输出警告（后续可统一处理未知消息）
        qWarning() << "No handler registered for message type:" << result.type
                   << ", from:" << QString::fromStdString(result.equipment_id);
    }
}
