#include "tcpclient.h"
#include <QDebug>

TcpClient::TcpClient(QObject *parent) : QObject(parent)
    , m_socket(new QTcpSocket(this))
    , m_isProcessingData(false) // 新增初始化
{
    // 连接信号与槽：当socket有数据可读时，调用我们的处理函数
    connect(m_socket, &QTcpSocket::readyRead, this, &TcpClient::onSocketReadyRead);
    // 连接信号与槽：当socket发生错误时
    connect(m_socket, &QTcpSocket::errorOccurred, this, &TcpClient::onSocketErrorOccurred);
    // 连接信号与槽：当socket成功连接时，转发connected信号
    connect(m_socket, &QTcpSocket::connected, this, &TcpClient::connected);
    // 连接信号与槽：当socket断开连接时，转发disconnected信号
    connect(m_socket, &QTcpSocket::disconnected, this, &TcpClient::disconnected);
}

TcpClient::~TcpClient()
{
    disconnectFromServer();
    m_socket->deleteLater();
}

bool TcpClient::connectToServer(const QString& host, quint16 port)
{
    if (m_socket->state() == QAbstractSocket::ConnectedState) {
        return true;
    }
    m_socket->connectToHost(host, port);
    // 简单实现：等待连接完成（生产环境建议异步）
    return m_socket->waitForConnected(3000); // 等待3秒
}

void TcpClient::disconnectFromServer()
{
    if (m_socket->state() == QAbstractSocket::ConnectedState) {
        m_socket->disconnectFromHost();
    }
}

qint64 TcpClient::sendData(const QByteArray& data)
{
    if (!isConnected()) {
        qWarning() << "Cannot send data, socket not connected.";
        return -1;
    }
    qint64 bytesWritten = m_socket->write(data);
    m_socket->flush(); // 尝试立即发送
    return bytesWritten;
}

bool TcpClient::sendProtocolMessage(ProtocolParser::ClientType client_type, ProtocolParser::MessageType type,
                                    const QString& equipmentId,
                                    const QString& payload)
{
    // 复用你现有的协议构建逻辑（这里需要根据你的协议调整）
    // 示例：构建一个心跳消息
    std::vector<std::string> fields;
    // 根据协议类型构建fields...
    // 这是一个示例，你需要根据实际协议调整
    std::string body = std::to_string(static_cast<int>(client_type)) + "|" + std::to_string(static_cast<int>(type)) + "|" + equipmentId.toStdString();
    if (!payload.isEmpty()) {
        body += "|" + payload.toStdString();
    }

    std::vector<char> packedMsg = ProtocolParser::pack_message(body);
    QByteArray data(packedMsg.data(), packedMsg.size());

    return sendData(data) > 0;
}

bool TcpClient::isConnected() const
{
    return m_socket->state() == QAbstractSocket::ConnectedState;
}

void TcpClient::onSocketReadyRead()
{
    // 如果已经在处理数据，则直接返回，避免信号队列堆积导致的递归调用
    if (m_isProcessingData) {
        return;
    }
    m_isProcessingData = true;

    // 使用循环读取，确保一次性读取所有当前可用的数据
    qint64 bytesAvailable = 0;
    do {
        // 1. 预估本次读取的字节数（避免分配过大内存）
        bytesAvailable = m_socket->bytesAvailable();
        if (bytesAvailable <= 0) {
            bytesAvailable = 1024; // 默认读取块大小
        }

        // 2. 准备缓冲区并读取数据
        QByteArray buffer;
        buffer.resize(bytesAvailable);
        qint64 bytesRead = m_socket->read(buffer.data(), buffer.size());

        if (bytesRead > 0) {
            // 3. 调整缓冲区实际大小并处理数据
            buffer.resize(bytesRead);
            processReceivedData(buffer);
        } else if (bytesRead == 0) {
            // 读取到0字节，远程端已优雅关闭
            qDebug() << "Remote host closed the connection gracefully.";
            disconnectFromServer();
            m_isProcessingData = false;
            return;
        } else {
            // bytesRead < 0，发生错误
            qWarning() << "Error reading from socket:" << m_socket->errorString();
            m_isProcessingData = false;
            return;
        }

        // 4. 检查是否还有数据可读（针对非阻塞socket，可能一次readyRead信号包含多个包）
    } while (m_socket->bytesAvailable() > 0 && m_socket->state() == QAbstractSocket::ConnectedState);

    m_isProcessingData = false;
}

void TcpClient::onSocketErrorOccurred(QAbstractSocket::SocketError error)
{
    Q_UNUSED(error);
    emit errorOccurred(m_socket->errorString());
}

void TcpClient::processReceivedData(const QByteArray &data)
{
    // 1. 将收到的原始数据追加到消息缓冲区
    m_messageBuffer.append_data(data.constData(), data.size());

    // 2. 循环提取并处理所有完整的消息
    std::vector<std::string> completeMessages;
    while (true) {
        completeMessages.clear(); // 清空上次的结果
        size_t extractedCount = m_messageBuffer.extract_messages(completeMessages);

        if (extractedCount == 0) {
            // 没有提取到完整消息，说明数据还不完整，等待下次接收
            break;
        }

        // 3. 处理每一个提取出的完整消息
        for (const auto& msg : completeMessages) {
            ProtocolParser::ParseResult result = ProtocolParser::parse_message(msg);
            if (result.success) {
                // 成功解析，发出信号
                qDebug() << "Successfully parsed a message, type:" << result.type;
                emit protocolMessageReceived(result);
            } else {
                // 解析失败，可能是协议错误或数据损坏
                qWarning() << "Failed to parse a message. Raw data (hex):"
                           << QByteArray(msg.data(), msg.size()).toHex().constData();
                // 可选：如果协议错误严重，可以断开连接
                // emit errorOccurred("Protocol error: invalid message format.");
                // disconnectFromServer();
                // return;
            }
        }

        // 4. 如果缓冲区提取后还有残留数据，且还能继续提取，则继续循环
        // (extract_messages 内部已处理，此处while循环条件保证继续尝试)
    }

    // 5. (重要) 安全阀：检查缓冲区是否异常增长（防止恶意或错误数据）
    if (m_messageBuffer.is_too_large()) {
        qCritical() << "MessageBuffer exceeded safety limit. Clearing buffer.";
        m_messageBuffer.clear();
        emit errorOccurred("Received too much data without a valid message. Buffer cleared.");
        // 根据策略，可以选择不断开连接，只清空缓冲区
    }
}
