#include "messagedispatcher.h"
#include "tcpclient.h"
#include <QDebug>

MessageDispatcher::MessageDispatcher(TcpClient *tcpClient, QObject *parent)
    : QObject(parent), m_tcpClient(tcpClient)
{
    if (!m_tcpClient) {
        qCritical() << "MessageDispatcher: TcpClient is null!";
        return;
    }
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
        m_handlerMap[type].append(handler);
        qDebug() << "Registered handler for message type:" << type << ", total handlers now:" << m_handlerMap[type].size();
    } else {
        qWarning() << "Attempted to register a null handler for type:" << type;
    }
}

void MessageDispatcher::unregisterHandler(ProtocolParser::MessageType type)
{
    if (m_handlerMap.remove(type) > 0) {
        qDebug() << "Unregistered all handlers for message type:" << type;
    }
}

void MessageDispatcher::onProtocolMessageReceived(const ProtocolParser::ParseResult &result)
{
    qDebug() << "Dispatcher received message type:" << result.type;
    auto it = m_handlerMap.find(result.type);
    if (it != m_handlerMap.end()) {
        const auto &handlers = it.value();
        for (const auto &handler : handlers) {
            try {
                handler(result);
            } catch (const std::exception &e) {
                qCritical() << "Exception in handler for type" << result.type << ":" << e.what();
            }
        }
    } else {
        qWarning() << "No handler registered for message type:" << result.type;
    }
}
