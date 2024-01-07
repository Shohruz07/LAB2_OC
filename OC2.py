
import socket
import signal
import threading
import sys

# Мьютекс
m_mutex = threading.Lock()
# Уведомление для ожидающиъ потоков о изменении состояния
m_notification = threading.Condition()
# Обработчик сигнала (флаг)
wasSignalHangUp = 0

# Обработка сигнала
def SignalHangUpHandler(signal, frame):
    # Когда сигнал будет получен, установим обработчику значение 1
    global wasSignalHangUp
    wasSignalHangUp = 1

# Обработка данных (полученных от клиента через сокет)
def ConnectionManagment(clientSocket):
    # Создаём массив для хранения данных, полученных от клиента
    buffer = bytearray(1024)
    try:
        # Считываем данные из сокета в буфер
        bytesReceived = clientSocket.recv_into(buffer)
        # Проверяем, были ли фактически получены какие либо данные от клиента
        if bytesReceived > 0:
            # Захватываем мьютекс перед выводом в консоль
            with m_mutex:
                # Если данные были получены, выводим количество байт
                print(f"Полученные данные от клиента: {bytesReceived} байт")
    except socket.error as e:
        # Выводим сообщение об ошибке при чтении из сокета
        print(f"Ошибка при чтении из сокета: {e}", file=sys.stderr)

# Принятие новых соединений от клиентов
def AcceptConnections(serverSocket):
    try:
        # Создаём структуру для хранения информации о клиенте, подключившемся к серверу
        clientSocket, clientAddress = serverSocket.accept()
        # Проверяем, успешно ли был создан сокет для нового клиента
        if clientSocket:
            with m_mutex:
                # Если сокет успешно создан, выводится сообщение о том, что новое соединение было принято
                print("Новое соединение принято")
                # Выводим IP адрес и порт клиента
                print(f"IP адрес клиента: {clientAddress[0]}")
                print(f"Порт клиента: {clientAddress[1]}")
            # Создаётся новый поток, который будет управлять соединением с клиентом
            connectionThread = threading.Thread(target=ConnectionManagment, args=(clientSocket,))
            # Поток запускается независимо от главного потока и работает параллельно с ним
            connectionThread.start()
            # Сокет закрывается, так как он больше не нужен для принятия новых соединений
            clientSocket.close()
        else:
            # Вывод сообщения об ошибке в случае неудачной попытки принятия соединения
            print("Ошибка при попытке принять соединение", file=sys.stderr)
    except socket.error as e:
        # Выводим сообщение об ошибке при принятии соединения
        print(f"Ошибка при принятии соединения: {e}", file=sys.stderr)


def main():
    # Запускаем в работу сокеты
    result = socket.has_ipv6
    # Проверяем ошибки при инициализации библиотеки сокетов
    if not result:
        # Выводим сообщение об ошибке
        print("Не удалось выполнить WSAStartup", file=sys.stderr)
        return 1
    # Создаём серверный сокет
    serverSocket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    # Проверяем ошибки при создании серверного сокета
    if serverSocket == -1:
        # Выводим сообщение об ошибке
        print("Не удалось создать серверный сокет", file=sys.stderr)
        return 1
    # Задаём параметры серверного сокета
    serverAddress = ("", 15200) # Адрес и порт для прослушивания
    try:
        # Связываем серверный сокет с адресом и портом
        serverSocket.bind(serverAddress)
        # Переводим серверный сокет в режим прослушивания
        serverSocket.listen(socket.SOMAXCONN)
    except socket.error as e:
        # Выводим сообщение об ошибке при связывании или прослушивании сокета
        print(f"Ошибка при связывании или прослушивании сокета: {e}", file=sys.stderr)
        return 1
    # Устанавливаем обработчик сигнала
   # signal.signal(signal.SIGINT, SignalHangUpHandler)
    # Выводим сообщение о том, что сервер запущен и ожидает подключений
    print("Сервер запущен и ожидает подключений")
    # Запускаем бесконечный цикл для принятия новых соединений
    while True:
        # Проверяем, был ли получен сигнал
        if wasSignalHangUp == 1:
            # Если сигнал был получен, выходим из цикла
            break
        # Принимаем новые соединения от клиентов
        AcceptConnections(serverSocket)
    # Закрываем серверный сокет
    serverSocket.close()
    return 0

if __name__ == "__main__":
    main()