#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <wiringPi.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <sqlite3.h>
#include "database.h"

#define DHTPIN 7  
#define MAX 85
#define PORT 9090
#define BUF_SIZE 2048
#define MAX_CLIENTS 20

void error_handling(char *message);
void send_history(int cli_sock);
void *client_handler(void *arg);

int dht_val[5] = {0, 0, 0, 0, 0};

int read_dht22_data() {
    uint8_t last_state = HIGH;
    uint8_t counter = 0;
    uint8_t j = 0, i;

    for (i = 0; i < 5; i++) {
        dht_val[i] = 0;
    }

    pinMode(DHTPIN, OUTPUT);
    digitalWrite(DHTPIN, LOW);
    delay(18);
    digitalWrite(DHTPIN, HIGH);
    delayMicroseconds(30);
    pinMode(DHTPIN, INPUT);

    for (i = 0; i < MAX; i++) {
        counter = 0;
        while (digitalRead(DHTPIN) == last_state) {
            counter++;
            delayMicroseconds(1);
            if (counter == 255) {
                break;
            }
        }
        last_state = digitalRead(DHTPIN);

        if (counter == 255) {
            break;
        }

        if ((i >= 4) && (i % 2 == 0)) {
            dht_val[j / 8] <<= 1;
            if (counter > 30) {
                dht_val[j / 8] |= 1;
            }
            j++;
        }
    }

    if ((j >= 40) && (dht_val[4] == ((dht_val[0] + dht_val[1] + dht_val[2] + dht_val[3]) & 0xFF))) {
        return 1;
    } else {
        return 0;
    }
}

int read_dht22_data_with_retry(int max_retries) { //재시도 횟수 받아서
    int success = 0;
    for (int attempt = 0; attempt < max_retries; attempt++) {
        if (read_dht22_data()) { //데이터를 성공적으로 읽었다면?
            success = 1;    // 1 반환
            break;
        }
        printf("센서 읽기 실패. 재시도 중 (%d/%d)\n", attempt + 1, max_retries); //데이터 읽기 실패시 재시도 알림
        delay(2000);  //2초 대기 후 다시 데이터 읽기
    }
    return success;
}

void handle_client(int cli_sock) {
    char buffer[BUF_SIZE];
    int valread = read(cli_sock, buffer, sizeof(buffer) - 1);

    if (valread < 0) {
        perror("Read error");
        close(cli_sock);
        return;
    }

    buffer[valread] = '\0';

    if (strncmp(buffer, "GET /history", 12) == 0) {
        send_history(cli_sock);  // /history 요청에 대한 처리
    } else {
        char response[BUF_SIZE];
        char html_content[BUF_SIZE];
        float humidity, temperature;

        if (read_dht22_data_with_retry(3)) {
            humidity = ((dht_val[0] << 8) + dht_val[1]) / 10.0;
            temperature = ((dht_val[2] << 8) + dht_val[3]) / 10.0;

            save_to_database(temperature, humidity);

            snprintf(html_content, sizeof(html_content), 
                     "<html><body>"
                     "<h1>현재 방의 온도와 습도</h1>"
                     "<p>온도: %.1f &deg;C</p>"
                     "<p>습도: %.1f%%</p>"
                     "</body></html>", temperature, humidity);

            snprintf(response, sizeof(response),
                     "HTTP/1.1 200 OK\r\n"
                     "Content-Type: text/html; charset=UTF-8\r\n"
                     "Content-Length: %zu\r\n"
                     "Connection: close\r\n"
                     "\r\n"
                     "%s",
                     strlen(html_content), html_content);
        } else {
            snprintf(response, sizeof(response),
                     "HTTP/1.1 500 Internal Server Error\r\n"
                     "Content-Type: text/html; charset=UTF-8\r\n"
                     "Connection: close\r\n"
                     "\r\n"
                     "데이터 오류 발생\n");
        }

        write(cli_sock, response, strlen(response));
        close(cli_sock);
    }
}

void send_history(int cli_sock) {
    sqlite3 *db;
    sqlite3_stmt *stmt;
    char *sql = "SELECT timestamp, temperature, humidity FROM temperature_humidity ORDER BY timestamp ASC";
    char response[BUF_SIZE * 4];
    char row[256];
    int rc;

    // db 연결
    rc = sqlite3_open("temperature_humidity.db", &db);
    if (rc != SQLITE_OK) {
        snprintf(response, sizeof(response),
                 "HTTP/1.1 500 Internal Server Error\r\n"
                 "Content-Type: text/html; charset=UTF-8\r\n"
                 "Connection: close\r\n"
                 "\r\n"
                 "<html><body><h1>DATABASE ERROR OCCURED.</h1></body></html>");
        write(cli_sock, response, strlen(response));
        close(cli_sock);
        return;
    }

    
    snprintf(response, sizeof(response),
             "HTTP/1.1 200 OK\r\n"
             "Content-Type: text/html; charset=UTF-8\r\n"
             "Connection: close\r\n"
             "\r\n"
             "<html><body><h1>온습도 기록표</h1><table border=\"1\">"
             "<tr><th>측정시간</th><th>온도 (°C)</th><th>습도 (%)</th></tr>");
    
    
    rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
    if (rc != SQLITE_OK) {
        snprintf(response + strlen(response), sizeof(response) - strlen(response),
                 "<tr><td colspan='3'>Error retrieving data</td></tr>");
    } else {
        
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            const unsigned char *timestamp = sqlite3_column_text(stmt, 0);
            double temperature = sqlite3_column_double(stmt, 1);
            double humidity = sqlite3_column_double(stmt, 2);

            snprintf(row, sizeof(row),
                     "<tr><td>%s</td><td>%.1f</td><td>%.1f</td></tr>",
                     timestamp, temperature, humidity);
            strncat(response, row, sizeof(response) - strlen(response) - 1);
        }
        sqlite3_finalize(stmt);
    }
    strncat(response, "</table></body></html>", sizeof(response) - strlen(response) - 1);

    write(cli_sock, response, strlen(response));

    sqlite3_close(db);
    close(cli_sock);
}

void *client_handler(void *arg) {
    int cli_sock = *(int *)arg;
    free(arg);
    handle_client(cli_sock);
    return NULL;
}

int main(void) {
    int serv_sock, cli_sock;
    struct sockaddr_in serv_addr, cli_addr;
    socklen_t cli_addr_len;
    int option = 1;
    pthread_t t1;

    if (wiringPiSetup() == -1) {
        error_handling("wiringPiSetup() error");
    }

    serv_sock = socket(AF_INET, SOCK_STREAM, 0);
    if(serv_sock==-1){
        error_handling("socket() error");
    }

    setsockopt(serv_sock, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));
    
    memset(&serv_addr, 0x00, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(PORT);

    if(bind(serv_sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr))==-1){
        error_handling("bind() error");
    }

    if(listen(serv_sock, 5)==-1){
        error_handling("listen() error");
    }

    printf("Server running on port %d....\n", PORT);

    memset(client_sockets, 0, sizeof(client_sockets));
    while (1) {
        cli_addr_len = sizeof(cli_addr);
        cli_sock = accept(serv_sock, (struct sockaddr*)&cli_addr, &cli_addr_len);
        if (cli_sock == -1)
            continue;

        printf("새로운 클라이언트 연결: %s:%d\n", inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port));

        int *new_sock = malloc(sizeof(int));
        *new_sock = cli_sock;

        pthread_create(&t1, NULL, client_handler, (void*)new_sock);
        pthread_detach(t1);

    }

    close(serv_sock);
    return 0;
}

void error_handling(char *message){
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}
