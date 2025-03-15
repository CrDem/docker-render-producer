# Используем официальный образ для C++
FROM gcc:latest

# Устанавливаем зависимости
RUN apt-get update && apt-get install -y \
    librabbitmq-dev \
    && rm -rf /var/lib/apt/lists/*

# Создаём рабочую директорию
WORKDIR /app

# Копируем файлы проекта в контейнер
COPY producer.cpp .

# Компилируем producer
RUN g++ producer.cpp -o producer -lrabbitmq

# Запускаем producer
CMD ["./producer"]
