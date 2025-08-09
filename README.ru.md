# c-web-toy

[🇷🇺 Русская версия](README.ru.md) | [🇬🇧 English version](README.md)

Учебный **микро-веб-фреймворк на чистом C**. Цель — понять, как работает веб: сокеты, парсинг HTTP, роутинг, статика, простейший шаблонизатор.

> Кодовые комментарии и сообщения — **на английском**. Документация — **на русском и английском**.

## Структура

```
include/   — заголовочные файлы (public API)
src/       — исходники
public/    — статические файлы (HTML/CSS/JS)
templates/ — шаблоны (для последующих этапов)
```

## Сборка и запуск

```bash
make
./server
```

По умолчанию сервер слушает `http://localhost:8080`.

## Поддержка HTTPS

Для запуска с HTTPS параллельно с HTTP:

```bash
# Генерация самоподписанного сертификата
openssl req -x509 -nodes -newkey rsa:2048 -keyout key.pem -out cert.pem -days 365

# Запуск с HTTP (8080) + HTTPS (8443)
PORT=8080 HTTPS=1 HTTPS_PORT=8443 CERT_FILE=./cert.pem KEY_FILE=./key.pem ./server
```

## Возможности текущего этапа

- Простая маршрутизация по методу + пути (`GET /hello`)
- Отдача статики с корректным `Content-Type`: `mount /static/ -> ./public`
- Простейший шаблонизатор `{{var}}` для HTML-строк (без загрузки из файлов)
- Минимальный парсинг HTTP (метод, путь, заголовки, query-параметры)
- Рендеринг шаблонов из файлов (`render_template_file`)
- POST/формы (`application/x-www-form-urlencoded`)
- Куки/сессии
- Параллельные слушатели HTTP + HTTPS

## Дорожная карта

- [x] Базовый сервер, парсинг запроса
- [x] Роутинг `method + path`
- [x] Статика (`/static/ -> ./public`)
- [x] Шаблонизатор (строки)
- [x] Шаблонизатор из файлов (`render_template_file`)
- [x] POST/формы (`application/x-www-form-urlencoded`)
- [x] Куки/сессии
- [x] Поддержка HTTPS
- [ ] Демо-приложение

## Лицензия

MIT
