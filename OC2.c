

#include <stdio.h> // Включаем стандартную библиотеку ввода/вывода
#include <stdlib.h> // Включаем стандартную библиотеку для выделения памяти и функций выхода
#include <unistd.h> // Включаем POSIX API для системных вызовов
#include <signal.h> // Подключаем библиотеку обработки сигналов
#include <arpa/inet.h> // Включаем библиотеку для манипулирования интернет-адресами
#include <sys/socket.h> // Подключаем библиотеку для операций с сокетами
#include <sys/types.h> // Подключаем библиотеку для типов данных
#include <errno.h> // Подключаем библиотеку для обработки ошибок

volatile sig_atomic_t g_got_sighup = 0; // Объявляем глобальную переменную для хранения флага сигнала SIGHUP
int g_accepted_socket = -1; // Объявляем глобальную переменную для хранения принятого дескриптора сокета

void handle_signal(int in_signum) { // Определяем функцию для обработки сигналов
    if (in_signum == SIGHUP) { // Если сигнал SIGHUP
        g_got_sighup = 1;// Устанавливаем флаг в 1
    } else { 
        printf("Received signal %d\n", in_signum); // Распечатываем номер сигнала
    }
}

int main() { // Определяем главную функцию
    struct sigaction signal_action; // Объявляем структуру для хранения действия сигнала
    signal_action.sa_handler = handle_signal;// Устанавливаем функцию-обработчик
    signal_action.sa_flags = SA_RESTART;// Устанавливаем флаг перезапуска системного вызова после обработки сигнала
    sigaction(SIGHUP, &signal_action, NULL); // Регистрируем действие сигнала для SIGHUP

    // Блок создания сокета
    int server_socket, client_socket; // Объявляем две переменные для хранения дескрипторов сокетов сервера и клиента
    struct sockaddr_in server_addr, client_addr; // Объявляем две структуры для хранения информации об адресах сервера и клиента
    socklen_t client_len = sizeof(client_addr); // Объявляем переменную для хранения размера структуры адреса клиента

    if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1) { // Создаем сокет для сервера с использованием протоколов IPv4 и TCP и проверяем наличие ошибок
        perror("Error creating socket");// Распечатываем сообщение об ошибке
        exit(EXIT_FAILURE); // Выходим из программы со статусом сбоя
    }

    server_addr.sin_family = AF_INET; // Устанавливаем семейство адресов IPv4
    server_addr.sin_addr.s_addr = INADDR_ANY; // Устанавливаем любой IP-адрес
    server_addr.sin_port = htons(8080); // Устанавливаем номер порта 8080 в сетевом порядке байтов

    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) { // Привязываем сокет к адресу сервера и проверяем наличие ошибок
        perror("Error binding socket"); 
        exit(EXIT_FAILURE); // Выходим из программы со статусом сбоя
    }

    if (listen(server_socket, 1) == -1) { // Прослушиваем входящие соединения на сокете и проверяем наличие ошибок
        perror("Error listening for connections"); 
        exit(EXIT_FAILURE); // Выходим из программы со статусом сбоя
    }

    printf("Server listening on port 8080\n"); // Распечатываем сообщение, указывающее, что сервер прослушивает

    char buffer[1024]; // Объявляем буфер для хранения полученных данных

    fd_set read_fds; // Объявляем набор дескрипторов файлов для хранения сокетов, которые будут отслеживаться на чтение
    FD_ZERO(&read_fds); // Инициализируем набор равным нулю
    // Блок обработки
    while (1) { 
        if (g_got_sighup) { // Если установлен флаг SIGHUP
            printf("Received SIGHUP signal. Closing the server.\n"); // Распечатываем сообщение, указывающее, что сервер закрывается
            if (g_accepted_socket != -1) {// Если есть принятый сокет
                close(g_accepted_socket); // Закрываем сокет
            }
            exit(0); // Выходим из программы со статусом успеха
        }

FD_SET(server_socket, &read_fds); // Добавляем сокет сервера в набор файловых дескрипторов

        struct timespec timeout; // Объявляем структуру для хранения значения таймаута
        timeout.tv_sec = 1; // Устанавливаем таймаут на 1 секунду
        timeout.tv_nsec = 0; // Устанавливаем наносекунды в 0
        
        sigset_t mask; // Объявляем набор сигналов для хранения сигналов для блокировки
    // Вызов функции pselect, чтобы отслеживать набор файловых дескрипторов для чтения и сохранять результа
        int result = pselect(serversocket + 1, &readfds, NULL, NULL, &timeout, &mask); 
        
        if (result == -1) { // Если результат равен -1, это указывает на ошибку
            if (errno == EINTR) { // Если ошибка вызвана прерванным системным вызовом
                printf("'pselect' was interrupted by a signal.\n"); // pselect был прерван
                continue;
            } else {
                perror("Error in pselect");
                break;// Разрыв цикла
            }
        }

        if (FD_ISSET(server_socket, &read_fds)) {// Если сокет сервера готов к чтению, что указывает на входящее соединение
            // принимаем соединение и сохраняем дескриптор сокета клиента, а также проверяем наличие ошибок
            if ((client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &client_len)) == -1) {
                perror("Error accepting connection");
                continue;
            }
            // Распечатываем адрес клиента и номер порта
            printf("Accepted connection from %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

            if (g_accepted_socket == -1) {// Если принятого сокета нет
                g_accepted_socket = client_socket;// Устанавливаем глобальную переменную в клиентский сокет
            } else {
                close(client_socket);// Закрываем клиентский сокет
            }
        }
    // Получаем данные из принятого сокета и сохраняем количество полученных байтов
        size_t bytes_received = recv(g_accepted_socket, buffer, sizeof(buffer), 0);
        if (bytes_received > 0) { // Если количество полученных байт положительное
            buffer[bytes_received] = '\0'; // Добавляем нулевой терминатор в буфер
            printf("Received %ld bytes: %s\n", bytes_received, buffer);// Выводим количество байт и полученные данные
        } else if (bytes_received == 0) { // Если количество полученных байтов равно нулю, это означает, что соединение закрыто
            printf("Connection closed by client.\n");
            close(g_accepted_socket);// Закрываем принятый сокет
            g_accepted_socket = -1; // Устанавливаем глобальную переменную в -1
        } 
        else { // В противном случае, что указывает на ошибку
            perror("Error receiving data");
        }
    }
    return 0; // Возвращаем 0, чтобы указать на успех
}
         
