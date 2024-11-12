#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <wiringPi.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>

#define DHTPIN 7  
#define MAX 85
#define PORT 9090
#define BUF_SIZE 1024
#define MAX_CLIENTS 20

void error_handling(char *message);

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

    
    if ((j >= 40) && (dht_val[4] == ((dht_val[0] + dht_val[1] + dht_val[2] + dht_val[3]) & 0xFF))) {//데이터 에러 없다면 1, 있으면 0
        return 1;  
    } else {
        return 0;  
    }
}



void handle_client(int cli_sock){
    char response[BUF_SIZE];
    char html_content[BUF_SIZE];

    if(read_dht22_data()){
        float humidity = ((dht_val[0]<<8) + dht_val[1]) / 10.0;
        float celsius = ((dht_val[2]<<8) + dht_val[3]) /10.0;

        snprintf(html_content, sizeof(html_content), 
            "<html><body>"
            "<h1>N4동 413호 온도와 습도</h1>"
            "<p>온도: %.1f &deg;C</p>"
            "<p>습도: %.1f%%</p>"
            "</body></html>", celsius, humidity);
        
        snprintf(response, sizeof(response),
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/html; charset=UTF-8\r\n"
            "Content-Length: %zu\r\n"
            "Connection: close\r\n"
            "\r\n"
            "%s",
            strlen(html_content), html_content);
    }
    else{   //실패시 -> 읽을 데이터 없을때
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

int main(void){

    int serv_sock, cli_sock;
    struct sockaddr_in serv_addr, cli_addr;
    socklen_t cli_addr_len;
    int max_fd, active, i;
    int client_sockets[MAX_CLIENTS];
    fd_set read_fds;

    if (wiringPiSetup() == -1) {
        error_handling("wiringPiSetup() error");
    }

    serv_sock = socket(AF_INET, SOCK_STREAM, 0);
    if(serv_sock==-1){
        error_handling("socket() error");
    }
    memset(&serv_addr, 0x00, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(PORT);

    if(bind(serv_sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr))==-1){
        error_handling("bind() erorr");
    }

    if(listen(serv_sock, 5)==-1){
        error_handling("listen() error");
    }

    printf("Server running on port %d....\n", PORT);

    memset(client_sockets, 0x00, sizoef(client_sockets));
    while (1) {
        FD_ZERO(&read_fds);
        FD_SET(serv_sock, &read_fds); //서버 소켓 감시 시작
        max_fd = serv_sock;
        
        for(i=0; i<MAX_CLIENTS; i++){
            int sd = client_sockets[i];
            if(sd>0){
                FD_SET(sd, &read_fds);
            }  
            if(sd > max_fd){
                max_fd = sd;
            }
        }

        active = select(max_fd+1, &read_fds, 0, 0, 0);
        if(active<0){
            error_handling("select() error");
            continue;
        }
        
        if(FD_ISSET(serv_sock, &read_fds)){
            cli_addr_len = sizeof(cli_addr);
            cli_sock = accept(serv_sock, (struct sockaddr*)&cli_addr, &cli_addr_len);
            if(cli_sock==-1){
                error_handling("accept() error");
                continue;
            }
        
        printf("새로운 클라이언트 연결. client fd : %d, ip : %s, port : %d\n",
                cli_sock, inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port));

        for(i=0; i<MAX_CLIENTS; i++){
            if(client_sockets[i]==0){
                client_sockets[i] = cli_sock;
                printf("클라이언트 소켓 [%d]에 추가됨\n", i);
                break;
                }
            }
        }

        for(i=0; i< MAX_CLIENTS; i++){
            int sd = client_sockets[i];
            
            if(FD_ISSET(sd, &read_fds)){

                handle_client(sd);
                client_sockets[i] = 0;
            }
        }

    }

    close(serv_sock);
    return 0;
}

void error_handling(char *message){
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}