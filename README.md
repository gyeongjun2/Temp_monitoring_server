# 소켓 프로그래밍을 활용한 라즈베리파이 기반 온습도 모니터링 서버 프로그램

## 프로젝트 주제

- 라즈베리파이와 DHT22 센서를 통해 실시간으로 온도와 습도 데이터를 수집하고 소켓 프로그래밍을 통해 클라이언트-서버 간 데이터를 전송하여 웹 브라우저에서 실시간 및 과거 데이터를 확인할 수 있는 모니터링 시스템을 구축

## 프로젝트 목적

- 라즈베리파이 서버와 클라이언트 간의 통신을 구현하여 실시간으로 온도와 습도 데이터를 안정적으로 전송하고 모니터링할 수 있는 시스템을 구현
- DHT22센서에서 수집된 데이터들을 DB에 저장하여 클라이언트가 요청했을때 과거 온습도 데이터를 전달
- 네트워크 소켓을 활용한 서버-클라이언트 통신을 이용하여 네트워크 프로그래밍의 기초와 소켓 통신 방식을 이해

## 프로젝트 유형

- 개인 프로젝트

## 개발 인원 / 개발 기간

개발 인원 : 박경준

개발 기간 : 2024.10.27 ~ 2024.11.13

## 기술 스택

주요 언어 : C 언어

개발 도구: RaspberryPi 4, DHT22 sensor

데이터베이스 : SQLite3

라이브러리 : WiringPi - 라즈베리파이 GPIO 제어

## 핵심 기능

**실시간 온습도 측정**

- DHT22 센서를 통해 실시간으로 온도와 습도 데이터를 측정하여 라즈베리파이 서버로 전달.

라즈베리파이 연결 사진

<img src="https://github.com/user-attachments/assets/a0e282ae-6e29-413a-907b-b2a09b6d57c9" width="300" height="400"/>

- Raspberry pi 4 model B
- DHT22 온습도 센서
- 브레드보드
- 점퍼와이어
- 10kΩ 저항

라즈베리파이 연결 회로도

<img src="https://github.com/user-attachments/assets/fc51a723-cab8-4a95-b7eb-bea81e2bd770" width="500" height="400"/>

브레드보드를 통해 dht22와 라즈베리파이 연결 → 온습도 데이터 전송

**서버-클라이언트 소켓 연결** 

- 클라이언트가 서버로 접속 시 새로운 클라이언트 소켓 할당
- select()를 사용하여 다중 클라이언트의 요청을 수용


<img src="https://github.com/user-attachments/assets/3eba4851-7390-4f8a-b68e-6cc5f8fc3468" width="500" height="400"/>

포트번호 9090으로 서버 실행

<img src="https://github.com/user-attachments/assets/cd0aa79f-a42e-437d-ad61-ca5742d772f4" width="500" height="400"/>

serv_sock 감시 → 들어오는 요청이 있다면 → 클라이언트 소켓 생성 (main_server.c 코드 일부) 

**웹 기반 데이터 제공**

- HTTP 요청에 따라 현재 온습도 데이터를 HTML 페이지로 전송하여 클라이언트가 웹 브라우저를 통해 실시간 데이터 확인 가능

<img src="https://github.com/user-attachments/assets/c2162a70-f145-4790-bb1f-b75090d203a8" width="400" height="400"/>

간단한 HTML페이지를 HTTP형식으로 클라이언트로 전송 (main_server.c 코드 일부) 


<img src="https://github.com/user-attachments/assets/6ca86442-276a-42c0-95f1-e77689a7be19" width="300" height="400"/>

클라이언트가 웹 브라우저로 접속 시 온습도 데이터를 받아옴(클라이언트와 서버는 같은 로컬 네트워크에 있음)


**데이터베이스 저장 & 조회**

<img src="https://github.com/user-attachments/assets/2cdfdf62-b157-4356-bde5-710e51666561" width="200" height="400"/>

클라이언트가 /history endpoint로 접속시 데이터베이스에 저장된 온습도 데이터를 간단한 HTML 테이블 형식으로 조회 가능

<img src="https://github.com/user-attachments/assets/b1083a0f-5213-4cc9-bc50-52a9bf3fad6f" width="400" height="400"/>

데이터베이스에 온습도 데이터를 저장하여 이후 조회 및 분석 가능

### 서버코드

main_server.c

- 라즈베리파이에서 온습도 메인 서버 코드. dht22 설정과 소켓 통신 설정, 클라이언트로 데이터 전송, DB 저장 기능을 포함

database.c

- 온습도 데이터를 데이터베이스에 저장하는 DB코드

database.h

- database.c에서 정의된 함수를 포함하는 헤더파일