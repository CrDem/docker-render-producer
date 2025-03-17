#include <iostream>
#include <array>
#include <cstring>
#include <stdlib.h>
#include <amqp.h>
#include <amqp_tcp_socket.h>

struct Vec3 {
    double x, y, z;
};

struct PathRenderContext {
    char filename[3];
    uint32_t width;
    uint32_t height;
    double vfov;
    uint32_t spp;
    Vec3 light_col;
    double absorption;
    double scattering;
    double g;
};

// Сериализация структуры в массив байтов
void serialize(const PathRenderContext& ctx, std::array<char, sizeof(PathRenderContext)>& buffer) {
    std::memcpy(buffer.data(), &ctx, sizeof(PathRenderContext));
}

// Отправка данных в RabbitMQ
void send_to_rabbitmq(const std::array<char, sizeof(PathRenderContext)>& data) {
    const char* hostname = "rabbitmq";
    const int port = 5672;
    const char* queue_name = "render_queue";
    
    // Создание соединения и сокета
    amqp_connection_state_t conn = amqp_new_connection();
    amqp_socket_t* socket = amqp_tcp_socket_new(conn);
    
    if (!socket) {
        std::cerr << "Failed to create socket\n";
        return;
    }
    
    if (amqp_socket_open(socket, hostname, port)) {
        std::cerr << "Failed to open socket\n";
        return;
    }
    // Авторизация и открытие канала
    amqp_login(conn, "/", 0, 131072, 0, AMQP_SASL_METHOD_PLAIN, "bibaboba", "bimbimbambam");
    amqp_channel_open(conn, 1);
    amqp_get_rpc_reply(conn);
    
    std::cout << "Declaring queue...\n";
    // Объявление очереди
    amqp_queue_declare(
        conn, 
        1, 
        amqp_cstring_bytes(queue_name), // Имя очереди
        1, // проверять, существует ли очередь
        0, // Durable - если очередь не переживает перезагрузку
        1, // Exclusive - очередь будет использоваться только одним потребителем
        1, // Auto-delete - очередь будет удалена, когда не будет использована
        amqp_empty_table // Дополнительные параметры
    );
    std::cout << "Queue declared.\n";
    
    // Создание сообщения
    amqp_bytes_t message;
    message.len = sizeof(PathRenderContext);
    message.bytes = malloc(message.len);
    
    if (!message.bytes) {
        std::cerr << "Memory allocation failed\n";
        return;
    }

    std::memcpy(message.bytes, data.data(), message.len);
    
    
    std::cout << "Publishing message...\n";
    // Отправка сообщения
    amqp_basic_publish(
        conn, 1, amqp_empty_bytes, amqp_cstring_bytes(queue_name), 0, 0,
        nullptr, message
    );
    std::cout << "Message sent to RabbitMQ!\n";

    // Освобождение памяти
    amqp_bytes_free(message);

    // Закрытие соединения
    amqp_channel_close(conn, 1, AMQP_REPLY_SUCCESS);
    amqp_connection_close(conn, AMQP_REPLY_SUCCESS);
    amqp_destroy_connection(conn);
}

int main() {

    srand(time(nullptr));

    PathRenderContext ctx = {
        {'a', 'b', '\0'},
        1920,
        1080,
        90.0,
        100,
        {1.0, 1.0, 1.0},
        0.1,
        0.2,
        0.3
    };
    
    const int structureCount = std::getenv("STRUCTURE_COUNT");
    std::array<char, sizeof(PathRenderContext)> buffer;
    for (int i = 0; i < structureCount; ++i)
    {
        char c = i +'0';
        ctx.filename[2] = c;
        ctx.light_col = {(double)(rand()%101) / 100, (double)(rand()%101) / 100, (double)(rand()%101) / 100};
        ctx.absorption = (double)(rand()%81) / 80 + 0.05;
        ctx.scattering = (double)(rand()%81) / 80 + 0.05;
        ctx.g = (double)(rand()%81) / 80 + 0.05;
        serialize(ctx, buffer);
        send_to_rabbitmq(buffer);
    }
    return 0;
}
